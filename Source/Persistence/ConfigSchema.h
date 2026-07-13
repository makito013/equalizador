#pragma once

#include "State/AppState.h"

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
            && slotsEqual(o.slots);
    }

private:
    bool slotsEqual(const std::vector<SlotConfig>& other) const
    {
        if (slots.size() != other.size())
            return false;
        for (size_t i = 0; i < slots.size(); ++i)
            if (slots[i].filePath != other[i].filePath
                || slots[i].keyCode != other[i].keyCode
                || slots[i].modifiers != other[i].modifiers)
                return false;
        return true;
    }
};
} // namespace eq
