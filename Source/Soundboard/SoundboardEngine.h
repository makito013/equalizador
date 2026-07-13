#pragma once

#include "SoundSlot.h"

#include <array>
#include <memory>

#include <juce_audio_formats/juce_audio_formats.h>

namespace eq
{
/**
    Fixed grid of soundboard pads mixed into a single output buffer.

    Loading (loadSlot) happens off the audio thread and may allocate/resample.
    trigger() and renderNextBlock() are real-time safe: trigger() just sets an
    atomic; renderNextBlock() sums the active pads with no allocation or locks.
*/
class SoundboardEngine
{
public:
    static constexpr int numSlots = 8;

    SoundboardEngine();

    /** Must be called (off the audio thread) before loadSlot(). */
    void prepare(double sampleRate, int maxBlockSize, int numChannels);

    /** Load and resample a file into a slot. Returns false on failure. */
    bool loadSlot(int index, const juce::File& file);

    /** Request playback of a slot (UI/hotkey thread). Restarts if already playing. */
    void trigger(int index) noexcept;

    /** Sum all active pads into output (audio thread). Output is added to, not cleared. */
    void renderNextBlock(juce::AudioBuffer<float>& output, int numSamples) noexcept;

    const juce::String& getSlotName(int index) const noexcept;
    bool isSlotLoaded(int index) const noexcept;

private:
    std::array<SoundSlot, numSlots> slots;
    juce::AudioFormatManager formatManager;
    double engineSampleRate { 44100.0 };
    int engineChannels { 2 };
    bool prepared { false };
};
} // namespace eq
