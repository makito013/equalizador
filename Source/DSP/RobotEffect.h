#pragma once

#include "IVoiceEffect.h"
#include "State/AppState.h"

namespace eq
{
/**
    Robot voice via ring modulation: the signal is multiplied by a low-frequency
    sine carrier, producing the classic metallic/robotic timbre. Fully
    real-time safe (a phase accumulator, no allocation).

    setTimbre() sets the carrier frequency directly (absolute Hz, clamped to
    timbreRangeFor(VoiceEffectType::Robot)); it is safe to call from the audio
    thread (no allocation) and is a no-op when the value hasn't changed.
*/
class RobotEffect : public IVoiceEffect
{
public:
    void prepare(double sampleRate, int maxBlockSize, int numChannels) override;
    void process(juce::AudioBuffer<float>& buffer) noexcept override;
    void reset() noexcept override;
    void setTimbre(float carrierHzTarget) noexcept override;
    const char* name() const noexcept override { return "Robot"; }

private:
    void recomputePhaseIncrement() noexcept;

    double carrierHz { 55.0 };
    double phase { 0.0 };
    double phaseIncrement { 0.0 };
    double sampleRateHz { 48000.0 };
};
} // namespace eq
