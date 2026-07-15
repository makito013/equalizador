#pragma once

#include "IVoiceEffect.h"
#include "PitchShifter.h"
#include "State/AppState.h"

namespace eq
{
/**
    Alien/ET voice: PitchShifter with a high base offset plus a slow amplitude
    LFO (tremolo) layered on top for a subtle warble. The tremolo is fixed
    (rate/depth are not user-adjustable); setTimbre() only moves the pitch
    delta, same pattern as MonsterEffect/ChipmunkEffect.

    Real-time safety: the LFO is a plain phase accumulator (no allocation);
    the PitchShifter stage carries the same caveats documented in
    PitchShifter.cpp.
*/
class AlienEffect : public IVoiceEffect
{
public:
    AlienEffect();

    void prepare(double sampleRate, int maxBlockSize, int numChannels) override;
    void process(juce::AudioBuffer<float>& buffer) noexcept override;
    void reset() noexcept override;
    void setTimbre(float semitoneDelta) noexcept override;
    const char* name() const noexcept override { return "Alien"; }

private:
    static constexpr float kBaseSemitones = 8.0f;
    static constexpr double kLfoHz = 5.0;
    static constexpr float kTremoloDepth = 0.15f; // gain oscillates in [1-depth, 1]

    PitchShifter pitch;
    float lastDelta { 0.0f };
    double lfoPhase { 0.0 };
    double lfoPhaseIncrement { 0.0 };
};
} // namespace eq
