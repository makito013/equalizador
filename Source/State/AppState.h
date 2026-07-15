#pragma once

#include <array>
#include <atomic>
#include <cstddef>

namespace eq
{
/** Which voice effect the chain applies (matches the UI segmented control). */
enum class VoiceEffectType
{
    Normal   = 0, // passthrough
    Robot    = 1,
    Monster  = 2,
    Alien    = 3,
    Chipmunk = 4,
    Demon    = 5
};

/** Total number of entries in VoiceEffectType; keep in sync with the enum. */
inline constexpr size_t kNumVoiceEffects = 6;

/**
    Describes the "Timbre" slider's meaning for a given effect: its usable
    range and the value it starts at. Robot's range is an absolute carrier
    frequency in Hz; the pitch-shifted voices (Monster/Alien/Chipmunk/Demon)
    use a semitone delta added on top of the preset's base pitch. Normal has
    no adjustable timbre (the UI disables the slider).

    Single source of truth shared by the DSP layer (clamping in setTimbre())
    and the UI layer (slider range/enablement) so the two can never disagree.
*/
struct TimbreRange
{
    float min { 0.0f };
    float max { 0.0f };
    float defaultValue { 0.0f };
    bool enabled { false };
};

inline TimbreRange timbreRangeFor(VoiceEffectType effect) noexcept
{
    switch (effect)
    {
        case VoiceEffectType::Robot:    return { 30.0f, 150.0f, 55.0f, true };
        case VoiceEffectType::Monster:  return { -5.0f, 5.0f, 0.0f, true };
        case VoiceEffectType::Alien:    return { -5.0f, 5.0f, 0.0f, true };
        case VoiceEffectType::Chipmunk: return { -5.0f, 5.0f, 0.0f, true };
        case VoiceEffectType::Demon:    return { -5.0f, 5.0f, 0.0f, true };
        case VoiceEffectType::Normal:
        default:                        return { 0.0f, 0.0f, 0.0f, false };
    }
}

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

    // Per-effect "Timbre" value (see TimbreRange), indexed by
    // static_cast<size_t>(VoiceEffectType). Written by the UI thread when the
    // slider moves (or a persisted config is applied at startup); read once
    // per block by the audio thread inside EffectChain::process(), which
    // applies it to the selected effect's internal state only if it changed
    // (see IVoiceEffect::setTimbre). Remembered independently per effect so
    // switching effects restores each one's own last value.
    std::array<std::atomic<float>, kNumVoiceEffects> effectTimbre {{
        timbreRangeFor(VoiceEffectType::Normal).defaultValue,
        timbreRangeFor(VoiceEffectType::Robot).defaultValue,
        timbreRangeFor(VoiceEffectType::Monster).defaultValue,
        timbreRangeFor(VoiceEffectType::Alien).defaultValue,
        timbreRangeFor(VoiceEffectType::Chipmunk).defaultValue,
        timbreRangeFor(VoiceEffectType::Demon).defaultValue
    }};

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
