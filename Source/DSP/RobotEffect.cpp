#include "RobotEffect.h"

#include <cmath>

namespace eq
{
void RobotEffect::prepare(double sampleRate, int, int)
{
    phaseIncrement = 2.0 * juce::MathConstants<double>::pi * carrierHz / sampleRate;
    phase = 0.0;
}

void RobotEffect::process(juce::AudioBuffer<float>& buffer) noexcept
{
    const int channels = buffer.getNumChannels();
    const int frames = buffer.getNumSamples();

    // Advance one shared phase across the block, applying the same carrier to
    // every channel so stereo imaging is preserved.
    double localPhase = phase;
    for (int n = 0; n < frames; ++n)
    {
        const float carrier = static_cast<float>(std::sin(localPhase));
        for (int ch = 0; ch < channels; ++ch)
        {
            float* data = buffer.getWritePointer(ch);
            data[n] *= carrier;
        }

        localPhase += phaseIncrement;
        if (localPhase >= 2.0 * juce::MathConstants<double>::pi)
            localPhase -= 2.0 * juce::MathConstants<double>::pi;
    }
    phase = localPhase;
}

void RobotEffect::reset() noexcept
{
    phase = 0.0;
}
} // namespace eq
