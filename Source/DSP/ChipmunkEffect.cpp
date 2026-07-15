#include "ChipmunkEffect.h"

namespace eq
{
ChipmunkEffect::ChipmunkEffect()
{
    pitch.setSemitones(kBaseSemitones);
}

void ChipmunkEffect::prepare(double sampleRate, int maxBlockSize, int numChannels)
{
    pitch.prepare(sampleRate, maxBlockSize, numChannels);
}

void ChipmunkEffect::process(juce::AudioBuffer<float>& buffer) noexcept
{
    pitch.process(buffer);
}

void ChipmunkEffect::reset() noexcept
{
    pitch.reset();
}

void ChipmunkEffect::setTimbre(float semitoneDelta) noexcept
{
    const auto range = timbreRangeFor(VoiceEffectType::Chipmunk);
    const float clamped = juce::jlimit(range.min, range.max, semitoneDelta);
    if (juce::approximatelyEqual(clamped, lastDelta))
        return;

    lastDelta = clamped;
    pitch.setSemitones(kBaseSemitones + clamped);
}
} // namespace eq
