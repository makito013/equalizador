#include "Soundboard/SoundboardEngine.h"

#include <atomic>
#include <cmath>
#include <memory>
#include <thread>

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>

using namespace eq;

namespace
{
constexpr double kSampleRate = 48000.0;

/** Writes a small 16-bit mono WAV (a 440Hz tone) to a temp file for test fixtures. */
juce::File writeTempWav(const juce::String& baseName, int numSamples, float amplitude)
{
    auto file = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile(baseName + ".wav");
    file.deleteFile();

    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::OutputStream> outStream(file.createOutputStream().release());
    if (outStream == nullptr)
        return {};

    const auto options = juce::AudioFormatWriterOptions {}
                              .withSampleRate(kSampleRate)
                              .withNumChannels(1)
                              .withBitsPerSample(16);

    std::unique_ptr<juce::AudioFormatWriter> writer(wavFormat.createWriterFor(outStream, options));
    if (writer == nullptr)
        return {};

    juce::AudioBuffer<float> buffer(1, numSamples);
    for (int n = 0; n < numSamples; ++n)
        buffer.setSample(0, n, amplitude * static_cast<float>(
            std::sin(2.0 * juce::MathConstants<double>::pi * 440.0 * n / kSampleRate)));

    writer->writeFromAudioSampleBuffer(buffer, 0, numSamples);
    writer.reset(); // flush and close before the reader in the test re-opens the file
    return file;
}
}

/**
    Covers RF10: SoundboardEngine::loadSlot() must never mutate or free the
    buffer the audio thread (renderNextBlock) is concurrently reading. The
    last two tests specifically try to reproduce the two ways this can
    misbehave: (a) a buffer replaced mid-playback with a *shorter* one
    running the playback cursor out of bounds, and (b) genuine concurrent
    loadSlot()/renderNextBlock() calls - including *two* loader threads
    racing loadSlot() on the same slot back-to-back, which is what broke the
    original double-buffer scheme (see SoundSlot.h for the shared_ptr-based
    fix and why it's safe under that exact pattern).
*/
class SoundboardEngineTests : public juce::UnitTest
{
public:
    SoundboardEngineTests() : juce::UnitTest("SoundboardEngine", "eq") {}

    void runTest() override
    {
        beginTest("loadSlot on a missing file fails without touching slot state");
        {
            SoundboardEngine engine;
            engine.prepare(kSampleRate, 64, 1);

            const auto missing = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                      .getChildFile("eq_does_not_exist.wav");
            missing.deleteFile();

            expect(! engine.loadSlot(0, missing));
            expect(! engine.isSlotLoaded(0));
            expect(engine.getSlotName(0).isEmpty());
        }

        beginTest("loadSlot before prepare() fails");
        {
            SoundboardEngine engine;
            auto file = writeTempWav("eq_before_prepare", 100, 0.5f);
            expect(! engine.loadSlot(0, file));
        }

        beginTest("loadSlot + trigger + renderNextBlock plays back the loaded sample");
        {
            SoundboardEngine engine;
            engine.prepare(kSampleRate, 64, 1);

            auto file = writeTempWav("eq_basic", 200, 0.5f);
            expect(engine.loadSlot(0, file));
            expect(engine.isSlotLoaded(0));
            expectEquals(engine.getSlotName(0), file.getFileNameWithoutExtension());

            engine.trigger(0);
            juce::AudioBuffer<float> out(1, 64);
            out.clear();
            engine.renderNextBlock(out, 64);

            // The written tone starts at sample 0 with sin(0) == 0, rises after
            // that; just check it's not silence (double-buffer read succeeded).
            bool anyNonZero = false;
            for (int n = 0; n < 64; ++n)
                if (std::abs(out.getSample(0, n)) > 1.0e-4f)
                    anyNonZero = true;
            expect(anyNonZero, "expected the triggered sample to produce non-silent output");
        }

        beginTest("an unrecognised/corrupt file leaves the slot as it was (RNF03: no crash)");
        {
            SoundboardEngine engine;
            engine.prepare(kSampleRate, 64, 1);

            auto bogus = juce::File::getSpecialLocation(juce::File::tempDirectory)
                             .getChildFile("eq_bogus.wav");
            bogus.replaceWithText("not actually a wav file");

            expect(! engine.loadSlot(0, bogus));
            expect(! engine.isSlotLoaded(0));

            bogus.deleteFile();
        }

        beginTest("reloading a slot with a shorter sample mid-playback doesn't overrun (RF10 guard)");
        {
            SoundboardEngine engine;
            engine.prepare(kSampleRate, 64, 1);

            auto longFile = writeTempWav("eq_long", 4000, 0.5f);
            auto shortFile = writeTempWav("eq_short", 10, 0.5f);

            expect(engine.loadSlot(0, longFile));
            engine.trigger(0);

            juce::AudioBuffer<float> out(1, 64);

            // Advance well past frame 10 (the short sample's whole length).
            for (int i = 0; i < 20; ++i)
            {
                out.clear();
                engine.renderNextBlock(out, 64);
            }

            expect(engine.loadSlot(0, shortFile));

            // Still mid-"playing" at a position beyond the new sample's length;
            // must clamp to silence instead of reading out of bounds.
            for (int i = 0; i < 5; ++i)
            {
                out.clear();
                engine.renderNextBlock(out, 64);
                for (int n = 0; n < 64; ++n)
                    expect(std::isfinite(out.getSample(0, n)), "non-finite sample after shrink-reload");
            }

            // A fresh trigger resets the cursor, so the short sample should play.
            engine.trigger(0);
            out.clear();
            engine.renderNextBlock(out, 64);
            bool anyNonZero = false;
            for (int n = 0; n < 10; ++n)
                if (std::abs(out.getSample(0, n)) > 1.0e-4f)
                    anyNonZero = true;
            expect(anyNonZero, "retriggering after a shrink should still play the new (short) sample");
        }

        beginTest("two loaders racing loadSlot() on the same slot while renderNextBlock() runs do not use-after-free (RF10 stress test)");
        {
            SoundboardEngine engine;
            engine.prepare(kSampleRate, 128, 1);

            constexpr float kAmplitude = 0.4f;
            // Any *valid* sample can only ever come from one of the two sine
            // fixtures below, both written at exactly kAmplitude - so a
            // sample outside [-kAmplitude - tolerance, kAmplitude + tolerance]
            // cannot be real audio data. It's a much stronger signal than
            // isfinite(): stale/freed memory read via a use-after-free is
            // overwhelmingly likely to violate this bound (whereas it could
            // easily happen to be finite by chance), so this check is what
            // actually gives the shared_ptr-refcounting fix teeth in a test
            // that runs without ASan/TSan.
            constexpr float kAmplitudeTolerance = 0.02f;

            auto fileA = writeTempWav("eq_stress_a", 4000, kAmplitude);
            auto fileB = writeTempWav("eq_stress_b", 3000, kAmplitude);
            expect(engine.loadSlot(0, fileA));

            std::atomic<bool> stop { false };

            // Two independent loader threads hammering the *same* slot is
            // exactly the pattern the Revisor's report called out: a second
            // loadSlot() landing on a slot before the audio thread has
            // finished reading the buffer published by the first one. With
            // the old double-buffer scheme this could free/reuse memory
            // renderNextBlock() was still reading via a retained reference;
            // with the shared_ptr fix each renderNextBlock() call holds its
            // own strong reference for the whole block, so the buffer it's
            // reading can never be freed underneath it no matter how many
            // loadSlot() calls race past in the meantime.
            auto loaderFn = [&](bool startWithA)
            {
                bool useA = startWithA;
                while (! stop.load(std::memory_order_relaxed))
                {
                    engine.loadSlot(0, useA ? fileA : fileB);
                    useA = ! useA;
                }
            };
            std::thread loaderOne([&] { loaderFn(false); });
            std::thread loaderTwo([&] { loaderFn(true); });

            juce::AudioBuffer<float> out(1, 128);
            bool sawFiniteViolation = false;
            bool sawAmplitudeViolation = false;
            for (int i = 0; i < 6000; ++i)
            {
                if (i % 25 == 0)
                    engine.trigger(0);

                out.clear();
                engine.renderNextBlock(out, 128);

                for (int n = 0; n < 128; ++n)
                {
                    const float s = out.getSample(0, n);
                    if (! std::isfinite(s))
                        sawFiniteViolation = true;
                    else if (std::abs(s) > kAmplitude + kAmplitudeTolerance)
                        sawAmplitudeViolation = true;
                }
            }

            stop.store(true, std::memory_order_relaxed);
            loaderOne.join();
            loaderTwo.join();

            expect(! sawFiniteViolation, "non-finite sample produced under concurrent load/render");
            expect(! sawAmplitudeViolation,
                   "out-of-range sample (indicative of a stale/freed-memory read) produced under concurrent load/render");
            expect(engine.isSlotLoaded(0), "slot should still report a loaded sample after the stress test");
        }
    }
};

static SoundboardEngineTests soundboardEngineTests;
