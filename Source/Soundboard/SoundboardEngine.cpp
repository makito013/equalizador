#include "SoundboardEngine.h"

namespace eq
{
SoundboardEngine::SoundboardEngine()
{
    formatManager.registerBasicFormats();
}

void SoundboardEngine::prepare(double sampleRate, int, int numChannels)
{
    engineSampleRate = sampleRate;
    engineChannels = juce::jmax(1, numChannels);
    prepared = true;
}

bool SoundboardEngine::loadSlot(int index, const juce::File& file)
{
    if (index < 0 || index >= numSlots || ! prepared)
        return false;

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader == nullptr)
        return false;

    const int fileChannels = juce::jmax(1, static_cast<int>(reader->numChannels));
    const auto fileLength = static_cast<int>(reader->lengthInSamples);
    if (fileLength <= 0)
        return false;

    juce::AudioBuffer<float> raw(fileChannels, fileLength);
    reader->read(&raw, 0, fileLength, 0, true, true);

    // Resample to the engine rate so renderNextBlock() is a plain copy/add.
    juce::AudioBuffer<float> resampled;
    if (! juce::approximatelyEqual(reader->sampleRate, engineSampleRate))
    {
        const double ratio = reader->sampleRate / engineSampleRate;
        const int outLength = static_cast<int>(std::ceil(fileLength / ratio));
        resampled.setSize(fileChannels, outLength);

        for (int ch = 0; ch < fileChannels; ++ch)
        {
            juce::LagrangeInterpolator interpolator;
            interpolator.process(ratio, raw.getReadPointer(ch),
                                 resampled.getWritePointer(ch), outLength);
        }
    }
    else
    {
        resampled = std::move(raw);
    }

    auto& slot = slots[static_cast<size_t>(index)];
    slot.playing = false;
    slot.playbackPosition = 0;
    slot.triggerRequested.store(false);
    slot.sample = std::move(resampled);
    slot.name = file.getFileNameWithoutExtension();
    return true;
}

void SoundboardEngine::trigger(int index) noexcept
{
    if (index < 0 || index >= numSlots)
        return;
    slots[static_cast<size_t>(index)].triggerRequested.store(true, std::memory_order_release);
}

void SoundboardEngine::renderNextBlock(juce::AudioBuffer<float>& output, int numSamples) noexcept
{
    const int outChannels = output.getNumChannels();

    for (auto& slot : slots)
    {
        if (slot.triggerRequested.exchange(false, std::memory_order_acquire))
        {
            slot.playing = true;
            slot.playbackPosition = 0;
        }

        if (! slot.playing || ! slot.hasSample())
            continue;

        const int sampleChannels = slot.sample.getNumChannels();
        const int remaining = slot.sample.getNumSamples() - slot.playbackPosition;
        const int toRender = juce::jmin(numSamples, remaining);

        for (int ch = 0; ch < outChannels; ++ch)
        {
            const int srcCh = juce::jmin(ch, sampleChannels - 1);
            output.addFrom(ch, 0, slot.sample, srcCh, slot.playbackPosition, toRender);
        }

        slot.playbackPosition += toRender;
        if (slot.playbackPosition >= slot.sample.getNumSamples())
            slot.playing = false;
    }
}

const juce::String& SoundboardEngine::getSlotName(int index) const noexcept
{
    static const juce::String empty;
    if (index < 0 || index >= numSlots)
        return empty;
    return slots[static_cast<size_t>(index)].name;
}

bool SoundboardEngine::isSlotLoaded(int index) const noexcept
{
    if (index < 0 || index >= numSlots)
        return false;
    return slots[static_cast<size_t>(index)].hasSample();
}
} // namespace eq
