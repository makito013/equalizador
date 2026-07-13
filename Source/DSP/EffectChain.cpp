#include "EffectChain.h"

namespace eq
{
EffectChain::EffectChain(AppState& s) : state(s) {}

void EffectChain::prepare(double sampleRate, int maxBlockSize, int numChannels)
{
    robot.prepare(sampleRate, maxBlockSize, numChannels);
    monster.prepare(sampleRate, maxBlockSize, numChannels);
}

void EffectChain::process(juce::AudioBuffer<float>& buffer) noexcept
{
    if (state.bypass.load(std::memory_order_relaxed))
        return; // passthrough

    switch (state.currentEffect.load(std::memory_order_relaxed))
    {
        case VoiceEffectType::Robot:
            robot.process(buffer);
            break;
        case VoiceEffectType::Monster:
            monster.process(buffer);
            break;
        case VoiceEffectType::Normal:
        default:
            break; // passthrough
    }
}
} // namespace eq
