#pragma once

#include "IVoiceEffect.h"
#include "PitchShifter.h"
#include "State/AppState.h"

namespace eq
{
/**
    Chipmunk voice: a high, squeaky voice. Same pattern as MonsterEffect
    (PitchShifter with a base offset that setTimbre() nudges with a delta),
    just with a large positive base offset instead of a negative one.

    setTimbre() adds a semitone delta on top of kBaseSemitones (clamped to
    timbreRangeFor(VoiceEffectType::Chipmunk)); safe to call from the audio
    thread and a no-op when the value hasn't changed.
*/
class ChipmunkEffect : public IVoiceEffect
{
public:
    ChipmunkEffect();

    void prepare(double sampleRate, int maxBlockSize, int numChannels) override;
    void process(juce::AudioBuffer<float>& buffer) noexcept override;
    void reset() noexcept override;
    void setTimbre(float semitoneDelta) noexcept override;
    const char* name() const noexcept override { return "Chipmunk"; }

private:
    static constexpr float kBaseSemitones = 9.0f;

    PitchShifter pitch;
    float lastDelta { 0.0f };
};
} // namespace eq
