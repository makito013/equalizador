#include "OutputRouter.h"

namespace eq
{
void OutputRouter::writeTo(float* const* outputChannelData,
                           int numOutputChannels,
                           int numSamples,
                           const juce::AudioBuffer<float>& source) const noexcept
{
    const int srcChannels = source.getNumChannels();
    const int frames = juce::jmin(numSamples, source.getNumSamples());

    for (int ch = 0; ch < numOutputChannels; ++ch)
    {
        float* dst = outputChannelData[ch];
        if (dst == nullptr)
            continue;

        if (srcChannels > 0)
        {
            const int srcCh = juce::jmin(ch, srcChannels - 1);
            juce::FloatVectorOperations::copy(dst, source.getReadPointer(srcCh), frames);
        }
        else
        {
            juce::FloatVectorOperations::clear(dst, frames);
        }

        // Zero any tail the source did not fill.
        if (frames < numSamples)
            juce::FloatVectorOperations::clear(dst + frames, numSamples - frames);
    }
}
} // namespace eq
