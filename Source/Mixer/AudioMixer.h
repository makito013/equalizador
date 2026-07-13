#pragma once

#include "SoftLimiter.h"

#include <juce_audio_basics/juce_audio_basics.h>

namespace eq
{
/**
    Sums the processed voice and the soundboard output into a single stream with
    independent gains, then runs a soft limiter to guard against clipping.

    Deliberately knows nothing about WASAPI/CoreAudio: it only touches PCM
    buffers and plain gain values (the caller reads the std::atomic gains from
    AppState and passes them in). Real-time safe: no allocation, no locks.
*/
class AudioMixer
{
public:
    void prepare(double sampleRate, int maxBlockSize, int numChannels);

    /** Mix voice + soundboard into output. Output is fully overwritten. */
    void mix(const juce::AudioBuffer<float>& voice,
             const juce::AudioBuffer<float>& soundboard,
             juce::AudioBuffer<float>& output,
             float voiceGain,
             float soundboardGain) noexcept;

    SoftLimiter& getLimiter() noexcept { return limiter; }

private:
    static void addScaled(const juce::AudioBuffer<float>& src,
                          juce::AudioBuffer<float>& dst,
                          float gain) noexcept;

    SoftLimiter limiter;
};
} // namespace eq
