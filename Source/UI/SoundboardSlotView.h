#pragma once

#include "HotkeyCaptureButton.h"

#include <cstdint>
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

namespace eq
{
/**
    One soundboard pad row: a trigger button showing the slot's name (or
    "Vazio"), a small "..." button to pick a sound file, and a
    HotkeyCaptureButton to bind an F1-F12 key. Purely a dumb view - all the
    actual file-loading/hotkey-registration/persistence logic lives in
    MainComponent, reached via the three callbacks below.
*/
class SoundboardSlotView : public juce::Component
{
public:
    explicit SoundboardSlotView(int slotIndex);

    void setLabel(const juce::String& text);
    void setBinding(int keyCode, uint32_t modifiers) { hotkeyButton.setBinding(keyCode, modifiers); }

    void resized() override;

    std::function<void(int /*index*/)> onTrigger;
    std::function<void(int /*index*/)> onPickFile;
    std::function<void(int /*index*/, int /*keyCode*/, uint32_t /*modifiers*/)> onHotkeyChosen;

private:
    int index;
    juce::TextButton triggerButton;
    juce::TextButton pickFileButton { "..." };
    HotkeyCaptureButton hotkeyButton;
};
} // namespace eq
