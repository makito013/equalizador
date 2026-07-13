#include "AudioCaptureSource.h"

namespace eq
{
void AudioCaptureSource::prepare(int maxBlockSize, int numChannels)
{
    channels = juce::jmax(1, numChannels);
    capacitySamples = maxBlockSize;
    buffer.setSize(channels, maxBlockSize);
    buffer.clear();
    lastRms = 0.0f;
}

juce::AudioBuffer<float>& AudioCaptureSource::readFrom(const float* const* inputChannelData,
                                                       int numInputChannels,
                                                       int numSamples) noexcept
{
    // Shrink the logical size to this block without reallocating (capacity was
    // reserved in prepare()), so downstream sees exactly numSamples frames.
    const int frames = juce::jmin(numSamples, capacitySamples);
    buffer.setSize(channels, frames, /*keepContent*/ false, /*clearExtra*/ false,
                   /*avoidReallocating*/ true);
    float sumSquares = 0.0f;

    for (int ch = 0; ch < channels; ++ch)
    {
        float* dst = buffer.getWritePointer(ch);

        if (ch < numInputChannels && inputChannelData[ch] != nullptr)
        {
            const float* src = inputChannelData[ch];
            for (int n = 0; n < frames; ++n)
            {
                dst[n] = src[n];
                sumSquares += src[n] * src[n];
            }
        }
        else
        {
            juce::FloatVectorOperations::clear(dst, frames);
        }
    }

    const int totalSamples = frames * channels;
    lastRms = totalSamples > 0 ? std::sqrt(sumSquares / static_cast<float>(totalSamples)) : 0.0f;
    return buffer;
}
} // namespace eq
