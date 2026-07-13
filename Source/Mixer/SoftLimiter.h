#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

namespace eq
{
/**
    Soft-knee limiter. Below the threshold the signal is untouched; above it the
    excess is compressed with tanh so the output stays within [-1, 1] without
    the harsh artefacts of hard clipping. Stateless and real-time safe.
*/
class SoftLimiter
{
public:
    void setThreshold(float newThreshold) noexcept { threshold = newThreshold; }
    float getThreshold() const noexcept { return threshold; }

    /** Limit a single sample. */
    float processSample(float x) const noexcept;

    /** Limit an entire buffer in place. */
    void process(juce::AudioBuffer<float>& buffer) const noexcept;

private:
    float threshold { 0.9f };
};
} // namespace eq
