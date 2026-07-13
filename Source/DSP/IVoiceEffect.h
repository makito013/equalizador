#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

namespace eq
{
/**
    A voice effect processes a PCM buffer in place.

    Contract:
      - prepare() is called off the audio thread and does all allocation.
      - process() runs on the real-time audio thread: no allocation, no locks.
        (PitchShifter is the one node that cannot fully guarantee this because
        SoundTouch owns internal FIFOs; see PitchShifter.cpp.)
*/
class IVoiceEffect
{
public:
    virtual ~IVoiceEffect() = default;

    virtual void prepare(double sampleRate, int maxBlockSize, int numChannels) = 0;
    virtual void process(juce::AudioBuffer<float>& buffer) noexcept = 0;
    virtual void reset() noexcept = 0;
    virtual const char* name() const noexcept = 0;
};
} // namespace eq
