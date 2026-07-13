#include "PitchShifter.h"

#include <SoundTouch.h>

// LGPL NOTE: SoundTouch is LGPL. For local dev we build it from source (fully
// compliant). A distributed Windows build must link it *dynamically* (or ship
// object files enabling relink) to satisfy the LGPL — see CMakeLists.txt.

namespace eq
{
PitchShifter::PitchShifter()
    : engine(std::make_unique<soundtouch::SoundTouch>())
{
}

PitchShifter::~PitchShifter() = default;

void PitchShifter::setSemitones(float s) noexcept
{
    semitones = s;
    if (engine != nullptr)
        engine->setPitchSemiTones(semitones);
}

void PitchShifter::prepare(double sampleRate, int maxBlock, int channels)
{
    numChannels = channels;
    maxBlockSize = maxBlock;

    engine->setSampleRate(static_cast<unsigned int>(sampleRate));
    engine->setChannels(static_cast<unsigned int>(channels));
    engine->setPitchSemiTones(semitones);
    engine->clear();

    interleaved.assign(static_cast<size_t>(maxBlock) * static_cast<size_t>(channels), 0.0f);

    // Warm up the internal FIFOs at the real block size so that, once running,
    // putSamples/receiveSamples reuse capacity instead of reallocating. This is
    // why process() is *effectively* (not provably) real-time safe.
    for (int i = 0; i < 8; ++i)
    {
        engine->putSamples(interleaved.data(), static_cast<unsigned int>(maxBlock));
        engine->receiveSamples(interleaved.data(), static_cast<unsigned int>(maxBlock));
    }
    engine->clear();
    std::fill(interleaved.begin(), interleaved.end(), 0.0f);
}

void PitchShifter::process(juce::AudioBuffer<float>& buffer) noexcept
{
    const int channels = juce::jmin(numChannels, buffer.getNumChannels());
    const int frames = buffer.getNumSamples();
    if (channels <= 0 || frames <= 0 || frames > maxBlockSize)
        return;

    // Planar -> interleaved.
    for (int ch = 0; ch < channels; ++ch)
    {
        const float* src = buffer.getReadPointer(ch);
        for (int n = 0; n < frames; ++n)
            interleaved[static_cast<size_t>(n * channels + ch)] = src[n];
    }

    engine->putSamples(interleaved.data(), static_cast<unsigned int>(frames));
    const auto received = static_cast<int>(
        engine->receiveSamples(interleaved.data(), static_cast<unsigned int>(frames)));

    // Interleaved -> planar. During the initial latency window SoundTouch
    // returns fewer frames than requested; zero-fill the remainder so the
    // block size stays constant downstream.
    for (int ch = 0; ch < channels; ++ch)
    {
        float* dst = buffer.getWritePointer(ch);
        for (int n = 0; n < received; ++n)
            dst[n] = interleaved[static_cast<size_t>(n * channels + ch)];
        for (int n = received; n < frames; ++n)
            dst[n] = 0.0f;
    }
}

void PitchShifter::reset() noexcept
{
    if (engine != nullptr)
        engine->clear();
}
} // namespace eq
