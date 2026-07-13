#pragma once

#include <atomic>

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

namespace eq
{
/**
    One soundboard pad. The sample buffer is loaded off the audio thread and is
    considered immutable once assigned. Playback bookkeeping (position/playing)
    is touched only by the audio thread; triggering crosses the thread boundary
    via a single atomic flag.
*/
struct SoundSlot
{
    juce::String name;
    juce::AudioBuffer<float> sample; // resampled to the engine rate on load

    // UI/hotkey thread requests playback; audio thread consumes with exchange().
    std::atomic<bool> triggerRequested { false };

    // Audio-thread-only playback cursor state.
    int playbackPosition { 0 };
    bool playing { false };

    bool hasSample() const noexcept { return sample.getNumSamples() > 0; }
};
} // namespace eq
