#pragma once

#include <cstdint>

#include <juce_core/juce_core.h>

namespace eq
{
// Native Win32 RegisterHotKey modifier bits (winuser.h: MOD_ALT=0x1,
// MOD_CONTROL=0x2, MOD_SHIFT=0x4). Kept as local constants rather than
// #include <windows.h> so this header - and everything that depends on it -
// stays buildable on non-Windows dev machines (RNF02). MOD_WIN (0x8) is
// deliberately absent: capturing the Windows key reliably in a normal app is
// out of scope (RF07).
constexpr uint32_t kModAlt     = 0x0001;
constexpr uint32_t kModControl = 0x0002;
constexpr uint32_t kModShift   = 0x0004;

/**
    Pure translation from held-modifier flags to the native Win32
    RegisterHotKey bitmask (RF08). Takes plain bools rather than
    juce::ModifierKeys so it has no JUCE GUI-module dependency and is
    testable via juce::UnitTest - including on non-Windows dev machines -
    without linking juce_gui_basics into the test target. The
    juce::ModifierKeys -> bool extraction happens at the one real call site,
    HotkeyCaptureButton::keyPressed().
*/
constexpr uint32_t translateModifiers(bool isCtrlDown, bool isAltDown, bool isShiftDown) noexcept
{
    uint32_t mods = 0;
    if (isAltDown)   mods |= kModAlt;
    if (isCtrlDown)  mods |= kModControl;
    if (isShiftDown) mods |= kModShift;
    return mods;
}

/**
    RF07: this project supports at most a single modifier (Ctrl, Alt or
    Shift) combined with an F1-F12 base key - not a chord of several at
    once. Returns true for zero or exactly one bit set.
*/
constexpr bool isSingleOrNoModifier(uint32_t modifiers) noexcept
{
    return (modifiers & (modifiers - 1)) == 0; // clears the lowest set bit
}

/**
    RF09: readable prefix for a modifier bitmask, e.g. "Ctrl+", or "" when no
    modifier is set. Only meaningful for isSingleOrNoModifier() masks - with
    more than one bit set it just reports the highest-priority one found
    (Ctrl, then Alt, then Shift), which should never happen given RF07.
*/
inline juce::String modifierLabel(uint32_t modifiers)
{
    if ((modifiers & kModControl) != 0) return "Ctrl+";
    if ((modifiers & kModAlt) != 0)     return "Alt+";
    if ((modifiers & kModShift) != 0)   return "Shift+";
    return {};
}
} // namespace eq
