#pragma once

#include "AlienEffect.h"
#include "ChipmunkEffect.h"
#include "DemonEffect.h"
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

    Reads AppState atomically on the audio thread; never writes it. Before
    dispatching to the selected effect, also pushes that effect's current
    AppState::effectTimbre value into it via setTimbre() (RNF01: the actual
    DSP-internal mutation happens here, on the audio thread, not from the UI).
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
    AlienEffect alien;
    ChipmunkEffect chipmunk;
    DemonEffect demon;
};
} // namespace eq
