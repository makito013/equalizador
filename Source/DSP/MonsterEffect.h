#pragma once

#include "IVoiceEffect.h"
#include "PitchShifter.h"

namespace eq
{
/**
    Monster voice: a deep, pitched-down voice. Implemented by reusing
    PitchShifter with a fixed negative semitone offset, so all the SoundTouch
    handling lives in one place.
*/
class MonsterEffect : public IVoiceEffect
{
public:
    MonsterEffect();

    void prepare(double sampleRate, int maxBlockSize, int numChannels) override;
    void process(juce::AudioBuffer<float>& buffer) noexcept override;
    void reset() noexcept override;
    const char* name() const noexcept override { return "Monster"; }

private:
    PitchShifter pitch;
};
} // namespace eq
