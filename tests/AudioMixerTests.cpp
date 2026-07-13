#include "Mixer/AudioMixer.h"

#include <cmath>

#include <juce_core/juce_core.h>

using namespace eq;

namespace
{
juce::AudioBuffer<float> makeConstBuffer(int channels, int frames, float value)
{
    juce::AudioBuffer<float> b(channels, frames);
    for (int ch = 0; ch < channels; ++ch)
        for (int n = 0; n < frames; ++n)
            b.setSample(ch, n, value);
    return b;
}

bool allSamplesApprox(const juce::AudioBuffer<float>& b, float expected, float tol = 1.0e-5f)
{
    for (int ch = 0; ch < b.getNumChannels(); ++ch)
        for (int n = 0; n < b.getNumSamples(); ++n)
            if (std::abs(b.getSample(ch, n) - expected) > tol)
                return false;
    return true;
}
}

class AudioMixerTests : public juce::UnitTest
{
public:
    AudioMixerTests() : juce::UnitTest("AudioMixer", "eq") {}

    void runTest() override
    {
        AudioMixer mixer;
        mixer.prepare(48000.0, 512, 2);

        beginTest("sums both sources below the limiter threshold");
        {
            auto voice = makeConstBuffer(2, 64, 0.2f);
            auto sfx = makeConstBuffer(2, 64, 0.1f);
            juce::AudioBuffer<float> out(2, 64);

            mixer.mix(voice, sfx, out, 1.0f, 1.0f);
            expect(allSamplesApprox(out, 0.3f)); // 0.2 + 0.1, untouched by limiter
        }

        beginTest("applies independent gains");
        {
            auto voice = makeConstBuffer(2, 64, 0.4f);
            auto sfx = makeConstBuffer(2, 64, 0.4f);
            juce::AudioBuffer<float> out(2, 64);

            mixer.mix(voice, sfx, out, 0.5f, 0.25f);
            expect(allSamplesApprox(out, 0.4f * 0.5f + 0.4f * 0.25f)); // 0.3
        }

        beginTest("fans a mono source across a stereo output");
        {
            auto voice = makeConstBuffer(1, 64, 0.3f); // mono
            auto sfx = makeConstBuffer(2, 64, 0.0f);
            juce::AudioBuffer<float> out(2, 64);

            mixer.mix(voice, sfx, out, 1.0f, 1.0f);
            expect(allSamplesApprox(out, 0.3f));
        }

        beginTest("engages the limiter when the sum would clip");
        {
            auto voice = makeConstBuffer(2, 64, 1.0f);
            auto sfx = makeConstBuffer(2, 64, 1.0f);
            juce::AudioBuffer<float> out(2, 64);

            mixer.mix(voice, sfx, out, 1.0f, 1.0f); // raw sum = 2.0
            for (int ch = 0; ch < 2; ++ch)
                for (int n = 0; n < 64; ++n)
                    expect(std::abs(out.getSample(ch, n)) <= 1.0f);
        }
    }
};

static AudioMixerTests audioMixerTests;
