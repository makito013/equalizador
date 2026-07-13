#include "SoftLimiter.h"

#include <cmath>

namespace eq
{
float SoftLimiter::processSample(float x) const noexcept
{
    const float t = threshold;
    const float range = 1.0f - t; // headroom above the knee

    if (range <= 0.0f)
        return juce::jlimit(-1.0f, 1.0f, x); // degenerate: behave as hard clip

    if (x > t)
        return t + range * std::tanh((x - t) / range);
    if (x < -t)
        return -(t + range * std::tanh((-x - t) / range));

    return x;
}

void SoftLimiter::process(juce::AudioBuffer<float>& buffer) const noexcept
{
    const int channels = buffer.getNumChannels();
    const int frames = buffer.getNumSamples();

    for (int ch = 0; ch < channels; ++ch)
    {
        float* data = buffer.getWritePointer(ch);
        for (int n = 0; n < frames; ++n)
            data[n] = processSample(data[n]);
    }
}
} // namespace eq
