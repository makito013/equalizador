#pragma once

#include "Hotkeys/ModifierTranslation.h"

#include <cstdint>
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

namespace eq
{
/**
    Small button that shows a bound hotkey (e.g. "F5", "Ctrl+F1") or
    "Definir tecla" when unbound. Clicking enters listening mode ("Pressione
    uma tecla..."); the next F1-F12 keypress - optionally held together with
    a single modifier (Ctrl, Alt or Shift; see RF07/ModifierTranslation.h) -
    is captured and reported via onHotkeyChosen. Escape cancels listening
    without changing the binding; any other key (including F1-F12 chorded
    with more than one modifier) is ignored (still listening) so a stray
    keypress can't silently bind the wrong key.

    UI-only, platform-agnostic input handling (juce::KeyPress); not real-time,
    not unit tested per project convention (interactive JUCE components are
    exercised manually). The modifier-translation math it delegates to
    (translateModifiers/isSingleOrNoModifier/modifierLabel) is pure and IS
    unit tested - see ModifierTranslation.h and tests/ModifierTranslationTests.cpp.
*/
class HotkeyCaptureButton : public juce::TextButton
{
public:
    HotkeyCaptureButton();

    /** Reflects an externally-loaded/persisted binding. keyCode 0 = unbound. */
    void setBinding(int newKeyCode, uint32_t newModifiers);
    int getKeyCode() const noexcept { return keyCode; }
    uint32_t getModifiers() const noexcept { return modifiers; }

    bool keyPressed(const juce::KeyPress& key) override;
    void clicked() override;

    /** Fired once the user successfully captures a new F1-F12 key (+ at most one modifier). */
    std::function<void(int /*keyCode*/, uint32_t /*modifiers*/)> onHotkeyChosen;

private:
    void updateLabel();

    int keyCode { 0 };
    uint32_t modifiers { 0 };
    bool listening { false };
};
} // namespace eq
