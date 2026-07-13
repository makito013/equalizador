#pragma once

#include "IVoiceEffect.h"

namespace eq
{
/**
    Robot voice via ring modulation: the signal is multiplied by a low-frequency
    sine carrier, producing the classic metallic/robotic timbre. Fully
    real-time safe (a phase accumulator, no allocation).
*/
class RobotEffect : public IVoiceEffect
{
public:
    void prepare(double sampleRate, int maxBlockSize, int numChannels) override;
    void process(juce::AudioBuffer<float>& buffer) noexcept override;
    void reset() noexcept override;
    const char* name() const noexcept override { return "Robot"; }

private:
    double carrierHz { 55.0 };
    double phase { 0.0 };
    double phaseIncrement { 0.0 };
};
} // namespace eq
