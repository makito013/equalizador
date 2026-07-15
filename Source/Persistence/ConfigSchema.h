#pragma once

#include "State/AppState.h"

#include <array>
#include <vector>

#include <juce_core/juce_core.h>

namespace eq
{
/** A soundboard pad's persisted mapping. */
struct SlotConfig
{
    juce::String filePath;
    int keyCode { 0 };       // platform virtual-key code, 0 = unbound
    uint32_t modifiers { 0 };

    bool operator== (const SlotConfig& o) const
    {
        return filePath == o.filePath && keyCode == o.keyCode && modifiers == o.modifiers;
    }
};

/** Everything the app persists between runs. */
struct AppConfig
{
    juce::String inputDeviceName;
    juce::String outputDeviceName;
    VoiceEffectType effect { VoiceEffectType::Normal };
    bool bypass { false };
    float voiceGain { 1.0f };
    float soundboardGain { 1.0f };
    juce::String soundboardFolder;

    // Hotkey for toggling bypass.
    int bypassKeyCode { 0 };
    uint32_t bypassModifiers { 0 };

    // Per-effect "Timbre" slider value (RF02), indexed by
    // static_cast<size_t>(VoiceEffectType). Defaults mirror
    // AppState::effectTimbre's initial values so a config missing this field
    // entirely (older save file) round-trips to the same behaviour as today.
    std::array<float, kNumVoiceEffects> effectTimbre {{
        timbreRangeFor(VoiceEffectType::Normal).defaultValue,
        timbreRangeFor(VoiceEffectType::Robot).defaultValue,
        timbreRangeFor(VoiceEffectType::Monster).defaultValue,
        timbreRangeFor(VoiceEffectType::Alien).defaultValue,
        timbreRangeFor(VoiceEffectType::Chipmunk).defaultValue,
        timbreRangeFor(VoiceEffectType::Demon).defaultValue
    }};

    std::vector<SlotConfig> slots;

    bool operator== (const AppConfig& o) const
    {
        return inputDeviceName == o.inputDeviceName
            && outputDeviceName == o.outputDeviceName
            && effect == o.effect
            && bypass == o.bypass
            && juce::approximatelyEqual(voiceGain, o.voiceGain)
            && juce::approximatelyEqual(soundboardGain, o.soundboardGain)
            && soundboardFolder == o.soundboardFolder
            && bypassKeyCode == o.bypassKeyCode
            && bypassModifiers == o.bypassModifiers
            && effectTimbreEqual(o.effectTimbre)
            && slots == o.slots;
    }

private:
    bool effectTimbreEqual(const std::array<float, kNumVoiceEffects>& other) const
    {
        for (size_t i = 0; i < effectTimbre.size(); ++i)
            if (! juce::approximatelyEqual(effectTimbre[i], other[i]))
                return false;
        return true;
    }
};
} // namespace eq
