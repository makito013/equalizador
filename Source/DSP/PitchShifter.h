#pragma once

#include "IVoiceEffect.h"

#include <memory>
#include <vector>

namespace soundtouch { class SoundTouch; }

namespace eq
{
/**
    Pitch shifter built on SoundTouch. Shifts pitch by a number of semitones
    without changing tempo. Used directly for pitched presets and reused by
    MonsterEffect for its deep-voice setting.
*/
class PitchShifter : public IVoiceEffect
{
public:
    PitchShifter();
    ~PitchShifter() override;

    /** Pitch offset in semitones (negative = lower). Applied on prepare(). */
    void setSemitones(float semitones) noexcept;

    void prepare(double sampleRate, int maxBlockSize, int numChannels) override;
    void process(juce::AudioBuffer<float>& buffer) noexcept override;
    void reset() noexcept override;
    const char* name() const noexcept override { return "Pitch"; }

private:
    std::unique_ptr<soundtouch::SoundTouch> engine;
    float semitones { 0.0f };
    int numChannels { 0 };
    int maxBlockSize { 0 };

    // Interleaving scratch, sized in prepare() so process() never allocates.
    std::vector<float> interleaved;
};
} // namespace eq
