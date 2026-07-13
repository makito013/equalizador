#include "AudioMixer.h"

namespace eq
{
void AudioMixer::prepare(double, int, int)
{
    // The limiter is stateless; nothing to allocate. Kept for symmetry and in
    // case future mixer state (metering, smoothed gains) needs preparation.
}

void AudioMixer::addScaled(const juce::AudioBuffer<float>& src,
                           juce::AudioBuffer<float>& dst,
                           float gain) noexcept
{
    const int srcChannels = src.getNumChannels();
    if (srcChannels <= 0 || gain == 0.0f)
        return;

    const int frames = juce::jmin(src.getNumSamples(), dst.getNumSamples());

    for (int ch = 0; ch < dst.getNumChannels(); ++ch)
    {
        // Fan a mono/narrower source out across the wider destination.
        const int srcCh = juce::jmin(ch, srcChannels - 1);
        dst.addFrom(ch, 0, src, srcCh, 0, frames, gain);
    }
}

void AudioMixer::mix(const juce::AudioBuffer<float>& voice,
                     const juce::AudioBuffer<float>& soundboard,
                     juce::AudioBuffer<float>& output,
                     float voiceGain,
                     float soundboardGain) noexcept
{
    output.clear();
    addScaled(voice, output, voiceGain);
    addScaled(soundboard, output, soundboardGain);
    limiter.process(output);
}
} // namespace eq
