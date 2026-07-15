#include "EffectChain.h"

namespace eq
{
EffectChain::EffectChain(AppState& s) : state(s) {}

void EffectChain::prepare(double sampleRate, int maxBlockSize, int numChannels)
{
    robot.prepare(sampleRate, maxBlockSize, numChannels);
    monster.prepare(sampleRate, maxBlockSize, numChannels);
    alien.prepare(sampleRate, maxBlockSize, numChannels);
    chipmunk.prepare(sampleRate, maxBlockSize, numChannels);
    demon.prepare(sampleRate, maxBlockSize, numChannels);
}

void EffectChain::process(juce::AudioBuffer<float>& buffer) noexcept
{
    if (state.bypass.load(std::memory_order_relaxed))
        return; // passthrough

    const auto effect = state.currentEffect.load(std::memory_order_relaxed);
    const float timbre = state.effectTimbre[static_cast<size_t>(effect)]
                              .load(std::memory_order_relaxed);

    switch (effect)
    {
        case VoiceEffectType::Robot:
            robot.setTimbre(timbre);
            robot.process(buffer);
            break;
        case VoiceEffectType::Monster:
            monster.setTimbre(timbre);
            monster.process(buffer);
            break;
        case VoiceEffectType::Alien:
            alien.setTimbre(timbre);
            alien.process(buffer);
            break;
        case VoiceEffectType::Chipmunk:
            chipmunk.setTimbre(timbre);
            chipmunk.process(buffer);
            break;
        case VoiceEffectType::Demon:
            demon.setTimbre(timbre);
            demon.process(buffer);
            break;
        case VoiceEffectType::Normal:
        default:
            break; // passthrough
    }
}
} // namespace eq
