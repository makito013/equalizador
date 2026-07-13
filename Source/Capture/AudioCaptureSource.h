#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

namespace eq
{
/**
    Adapts raw device input (as delivered by juce::AudioIODeviceCallback) into a
    planar JUCE buffer the DSP chain can work on, and reports whether there is a
    live signal (for the "no signal" UI indicator).

    Device-API agnostic: it only sees float pointers, never CoreAudio/WASAPI.
    prepare() does the allocation; readFrom() is real-time safe.
*/
class AudioCaptureSource
{
public:
    void prepare(int maxBlockSize, int numChannels);

    /** Copy device input into the internal buffer. Returns the filled buffer. */
    juce::AudioBuffer<float>& readFrom(const float* const* inputChannelData,
                                       int numInputChannels,
                                       int numSamples) noexcept;

    juce::AudioBuffer<float>& getBuffer() noexcept { return buffer; }

    /** RMS of the last block; used to decide whether a mic signal is present. */
    float getLastRms() const noexcept { return lastRms; }

private:
    juce::AudioBuffer<float> buffer;
    int channels { 0 };
    int capacitySamples { 0 }; // reserved block size, used to clamp without realloc
    float lastRms { 0.0f };
};
} // namespace eq
