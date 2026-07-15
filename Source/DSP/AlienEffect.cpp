#include "AlienEffect.h"

#include <cmath>

namespace eq
{
AlienEffect::AlienEffect()
{
    pitch.setSemitones(kBaseSemitones);
}

void AlienEffect::prepare(double sampleRate, int maxBlockSize, int numChannels)
{
    pitch.prepare(sampleRate, maxBlockSize, numChannels);
    lfoPhaseIncrement = 2.0 * juce::MathConstants<double>::pi * kLfoHz / sampleRate;
    lfoPhase = 0.0;
}

void AlienEffect::process(juce::AudioBuffer<float>& buffer) noexcept
{
    pitch.process(buffer);

    const int channels = buffer.getNumChannels();
    const int frames = buffer.getNumSamples();

    double localPhase = lfoPhase;
    for (int n = 0; n < frames; ++n)
    {
        const float gain = 1.0f - kTremoloDepth * 0.5f
            + kTremoloDepth * 0.5f * static_cast<float>(std::sin(localPhase));

        for (int ch = 0; ch < channels; ++ch)
        {
            float* data = buffer.getWritePointer(ch);
            data[n] *= gain;
        }

        localPhase += lfoPhaseIncrement;
        if (localPhase >= 2.0 * juce::MathConstants<double>::pi)
            localPhase -= 2.0 * juce::MathConstants<double>::pi;
    }
    lfoPhase = localPhase;
}

void AlienEffect::reset() noexcept
{
    pitch.reset();
    lfoPhase = 0.0;
}

void AlienEffect::setTimbre(float semitoneDelta) noexcept
{
    const auto range = timbreRangeFor(VoiceEffectType::Alien);
    const float clamped = juce::jlimit(range.min, range.max, semitoneDelta);
    if (juce::approximatelyEqual(clamped, lastDelta))
        return;

    lastDelta = clamped;
    pitch.setSemitones(kBaseSemitones + clamped);
}
} // namespace eq
