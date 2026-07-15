#include "HotkeyCaptureButton.h"

namespace eq
{
HotkeyCaptureButton::HotkeyCaptureButton() : juce::TextButton("Definir tecla")
{
    setWantsKeyboardFocus(true);
    updateLabel();
}

void HotkeyCaptureButton::setBinding(int newKeyCode, uint32_t newModifiers)
{
    keyCode = newKeyCode;
    modifiers = newKeyCode == 0 ? 0 : newModifiers; // unbound never carries a stale modifier
    listening = false;
    updateLabel();
}

void HotkeyCaptureButton::clicked()
{
    listening = true;
    updateLabel();
    grabKeyboardFocus();
}

bool HotkeyCaptureButton::keyPressed(const juce::KeyPress& key)
{
    if (! listening)
        return false;

    if (key.getKeyCode() == juce::KeyPress::escapeKey)
    {
        listening = false;
        updateLabel();
        return true;
    }

    const auto code = key.getKeyCode();
    if (code < juce::KeyPress::F1Key || code > juce::KeyPress::F12Key)
        return true; // swallow: keep listening, only F1-F12 is a valid base key

    // RF07/RF08: translate the held modifiers via the pure, unit-tested
    // helper, then reject (keep listening) anything but zero-or-one
    // modifier - a chord like Ctrl+Alt+F1 is out of scope, and silently
    // dropping the extra modifiers would bind the wrong combination.
    const auto mods = key.getModifiers();
    const auto newModifiers = translateModifiers(mods.isCtrlDown(), mods.isAltDown(), mods.isShiftDown());
    if (! isSingleOrNoModifier(newModifiers))
        return true;

    listening = false;
    keyCode = code;
    modifiers = newModifiers;
    updateLabel();

    if (onHotkeyChosen)
        onHotkeyChosen(keyCode, modifiers);

    return true;
}

void HotkeyCaptureButton::updateLabel()
{
    if (listening)
    {
        setButtonText("Pressione uma tecla...");
        return;
    }
    if (keyCode == 0)
    {
        setButtonText("Definir tecla");
        return;
    }
    const int fNumber = keyCode - juce::KeyPress::F1Key + 1;
    setButtonText(modifierLabel(modifiers) + "F" + juce::String(fNumber));
}
} // namespace eq
