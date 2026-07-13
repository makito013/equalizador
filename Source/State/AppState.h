#pragma once

#include <atomic>

namespace eq
{
/** Which voice effect the chain applies (matches the UI segmented control). */
enum class VoiceEffectType
{
    Normal = 0, // passthrough
    Robot  = 1,
    Monster = 2
};

/**
    Single source of truth for the live, mutable app state.

    Every field is a std::atomic so it can be read by the audio thread and
    written by the UI / hotkey threads without locks. The audio thread only
    ever loads; the UI/hotkey side stores or toggles.
*/
struct AppState
{
    std::atomic<bool> bypass { false };
    std::atomic<VoiceEffectType> currentEffect { VoiceEffectType::Normal };
    std::atomic<float> voiceGain { 1.0f };
    std::atomic<float> soundboardGain { 1.0f };

    // Set by the audio thread from the input RMS; read by the UI to show the
    // "no signal" (red) indicator. Audio thread is the only writer.
    std::atomic<bool> signalPresent { false };

    /** Atomically flip the bypass flag. Safe under concurrent callers. */
    void toggleBypass() noexcept
    {
        bool expected = bypass.load(std::memory_order_relaxed);
        while (! bypass.compare_exchange_weak(expected, ! expected,
                                              std::memory_order_acq_rel,
                                              std::memory_order_relaxed))
        {
            // expected is refreshed by compare_exchange_weak on failure
        }
    }
};
} // namespace eq
