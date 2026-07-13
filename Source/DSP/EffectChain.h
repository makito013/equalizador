#pragma once

#include "IVoiceEffect.h"
#include "MonsterEffect.h"
#include "RobotEffect.h"
#include "State/AppState.h"

namespace eq
{
/**
    Owns the voice-effect strategies and dispatches to the one currently
    selected in AppState. Honours the bypass flag: when bypassed (or when
    "Normal" is selected) the buffer passes through untouched.

    Reads AppState atomically on the audio thread; never writes it.
*/
class EffectChain
{
public:
    explicit EffectChain(AppState& state);

    void prepare(double sampleRate, int maxBlockSize, int numChannels);
    void process(juce::AudioBuffer<float>& buffer) noexcept;

private:
    AppState& state;
    RobotEffect robot;
    MonsterEffect monster;
};
} // namespace eq
