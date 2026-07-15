#include "DemonEffect.h"

namespace eq
{
DemonEffect::DemonEffect()
{
    pitch.setSemitones(kBaseSemitones);
}

void DemonEffect::prepare(double sampleRate, int maxBlockSize, int numChannels)
{
    pitch.prepare(sampleRate, maxBlockSize, numChannels);
    ringMod.prepare(sampleRate, maxBlockSize, numChannels);
    ringMod.setTimbre(kCarrierHz); // fixed low carrier; not driven by AppState
}

void DemonEffect::process(juce::AudioBuffer<float>& buffer) noexcept
{
    pitch.process(buffer);
    ringMod.process(buffer);
}

void DemonEffect::reset() noexcept
{
    pitch.reset();
    ringMod.reset();
}

void DemonEffect::setTimbre(float semitoneDelta) noexcept
{
    const auto range = timbreRangeFor(VoiceEffectType::Demon);
    const float clamped = juce::jlimit(range.min, range.max, semitoneDelta);
    if (juce::approximatelyEqual(clamped, lastDelta))
        return;

    lastDelta = clamped;
    pitch.setSemitones(kBaseSemitones + clamped);
}
} // namespace eq
