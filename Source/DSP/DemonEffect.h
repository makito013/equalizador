#pragma once

#include "IVoiceEffect.h"
#include "PitchShifter.h"
#include "RobotEffect.h"
#include "State/AppState.h"

namespace eq
{
/**
    Demon voice: a deep PitchShifter offset in series with a low-carrier ring
    modulator (reusing RobotEffect's implementation), for a growling,
    metallic-and-deep voice. Composition of two already-proven DSP nodes.

    The ring-mod carrier is fixed (kCarrierHz) and not exposed to the UI;
    setTimbre() only moves the pitch delta around kBaseSemitones, same
    pattern as MonsterEffect/ChipmunkEffect/AlienEffect.
*/
class DemonEffect : public IVoiceEffect
{
public:
    DemonEffect();

    void prepare(double sampleRate, int maxBlockSize, int numChannels) override;
    void process(juce::AudioBuffer<float>& buffer) noexcept override;
    void reset() noexcept override;
    void setTimbre(float semitoneDelta) noexcept override;
    const char* name() const noexcept override { return "Demon"; }

private:
    static constexpr float kBaseSemitones = -11.0f;
    static constexpr float kCarrierHz = 35.0f;

    PitchShifter pitch;
    RobotEffect ringMod;
    float lastDelta { 0.0f };
};
} // namespace eq
