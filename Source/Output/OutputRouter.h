#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

namespace eq
{
/**
    Writes a processed PCM buffer to the device output channels supplied by
    juce::AudioIODeviceCallback, handling channel-count mismatches (e.g. a mono
    mix fanned across a stereo endpoint).

    On Windows the "device" will be the VB-CABLE render endpoint; here it is
    simply the default CoreAudio output. This class treats the target purely as
    a set of float pointers, so the endpoint is fully configurable upstream.
    Real-time safe: no allocation, no locks.
*/
class OutputRouter
{
public:
    void writeTo(float* const* outputChannelData,
                 int numOutputChannels,
                 int numSamples,
                 const juce::AudioBuffer<float>& source) const noexcept;
};
} // namespace eq
