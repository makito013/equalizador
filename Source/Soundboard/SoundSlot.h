#pragma once

#include <atomic>
#include <memory>

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

namespace eq
{
/**
    One soundboard pad.

    Thread-safety contract (RF10 fix): the decoded sample is published as a
    std::shared_ptr<const AudioBuffer<float>>, swapped via
    std::atomic_store_explicit/std::atomic_load_explicit (the free-function
    overloads for shared_ptr). This project targets C++17, where
    std::atomic<std::shared_ptr<T>> is not guaranteed to exist, so these free
    functions are the standard-correct way to publish/read a shared_ptr from
    multiple threads (in practice implemented with a lightweight internal
    lock that only ever guards the pointer swap, never audio data access).

    SoundboardEngine::loadSlot() (off the audio thread) always builds a brand
    new AudioBuffer and publishes it with atomic_store_explicit(..., release)
    - it never mutates a buffer that was already published. The audio thread
    (renderNextBlock) takes its own local shared_ptr copy via
    atomic_load_explicit(..., acquire) exactly once per slot per block, and
    uses only that local copy for the rest of the block's processing. Taking
    a local copy bumps the refcount, so the buffer a concurrent loadSlot()
    replaces stays alive for as long as the audio thread's local copy is
    alive - including the case that broke the old double-buffer scheme: two
    (or more) loadSlot() calls on the same slot landing back-to-back while
    renderNextBlock() is still mid-block on an older buffer. The old buffer
    is only actually freed once every holder (including the reader's local
    copy) has released it, so there is no use-after-free/data-race window.

    Playback bookkeeping (position/playing) is touched only by the audio
    thread; triggering crosses the thread boundary via a single atomic flag.
*/
struct SoundSlot
{
    juce::String name; // message-thread only (UI reads/writes happen on the same thread)

    // Published sample buffer; null until loadSlot() first succeeds. Never
    // touch this field directly - go through loadActiveSample()/
    // publishSample() so the atomic_load/atomic_store contract above holds.
    std::shared_ptr<const juce::AudioBuffer<float>> sample;

    // UI/hotkey thread requests playback; audio thread consumes with exchange().
    std::atomic<bool> triggerRequested { false };

    // Audio-thread-only playback cursor state.
    int playbackPosition { 0 };
    bool playing { false };

    /** Thread-safe: takes a local strong reference to the currently published buffer. */
    std::shared_ptr<const juce::AudioBuffer<float>> loadActiveSample() const noexcept
    {
        return std::atomic_load_explicit(&sample, std::memory_order_acquire);
    }

    /** Thread-safe: publishes a new buffer, replacing whatever was published before. */
    void publishSample(std::shared_ptr<const juce::AudioBuffer<float>> newSample) noexcept
    {
        std::atomic_store_explicit(&sample, std::move(newSample), std::memory_order_release);
    }

    bool hasSample() const noexcept
    {
        const auto local = loadActiveSample();
        return local != nullptr && local->getNumSamples() > 0;
    }
};
} // namespace eq
