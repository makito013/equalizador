#include "MonsterEffect.h"

namespace eq
{
MonsterEffect::MonsterEffect()
{
    // Roughly a perfect fifth down for a deep, monstrous voice.
    pitch.setSemitones(-7.0f);
}

void MonsterEffect::prepare(double sampleRate, int maxBlockSize, int numChannels)
{
    pitch.prepare(sampleRate, maxBlockSize, numChannels);
}

void MonsterEffect::process(juce::AudioBuffer<float>& buffer) noexcept
{
    pitch.process(buffer);
}

void MonsterEffect::reset() noexcept
{
    pitch.reset();
}
} // namespace eq
