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

    /**
        Applies the "Timbre" slider value (see TimbreRange in AppState.h) to
        this effect's internal state. Called from the audio thread, once per
        block, with the value currently stored in AppState::effectTimbre for
        this effect; implementations must clamp to their own range and skip
        the actual mutation when the value hasn't changed since the last call
        (RNF01: real mutation of DSP internals happens on the audio thread,
        never directly from the UI thread). Effects with no adjustable timbre
        (e.g. passthrough) simply don't override this no-op default.
    */
    virtual void setTimbre(float /*value*/) noexcept {}
};
} // namespace eq
