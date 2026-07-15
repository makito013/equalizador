#include "Hotkeys/ModifierTranslation.h"

#include <juce_core/juce_core.h>

using namespace eq;

// RF08: translateModifiers() (and its two small siblings, isSingleOrNoModifier()
// and modifierLabel()) are pure functions with no JUCE GUI-module dependency,
// so they're fully testable here without linking juce_gui_basics into this
// console test target and without any Windows hardware - see
// ModifierTranslation.h for why juce::ModifierKeys itself isn't the parameter
// type.
class ModifierTranslationTests : public juce::UnitTest
{
public:
    ModifierTranslationTests() : juce::UnitTest("ModifierTranslation", "eq") {}

    void runTest() override
    {
        beginTest("no modifiers held translates to 0");
        {
            expectEquals(translateModifiers(false, false, false), static_cast<uint32_t>(0));
        }

        beginTest("each single modifier translates to its own Win32 RegisterHotKey bit");
        {
            expectEquals(translateModifiers(true, false, false), kModControl);
            expectEquals(translateModifiers(false, true, false), kModAlt);
            expectEquals(translateModifiers(false, false, true), kModShift);
        }

        beginTest("Win32 bit values match winuser.h's RegisterHotKey constants exactly");
        {
            expectEquals(kModAlt, static_cast<uint32_t>(0x0001));
            expectEquals(kModControl, static_cast<uint32_t>(0x0002));
            expectEquals(kModShift, static_cast<uint32_t>(0x0004));
        }

        beginTest("multiple modifiers held simultaneously OR together (translation itself doesn't reject them)");
        {
            expectEquals(translateModifiers(true, true, false), kModControl | kModAlt);
            expectEquals(translateModifiers(true, true, true), kModControl | kModAlt | kModShift);
        }

        beginTest("isSingleOrNoModifier: true for 0 or exactly one bit (RF07's supported scope)");
        {
            expect(isSingleOrNoModifier(0));
            expect(isSingleOrNoModifier(kModAlt));
            expect(isSingleOrNoModifier(kModControl));
            expect(isSingleOrNoModifier(kModShift));
        }

        beginTest("isSingleOrNoModifier: false for any chord of two or more modifiers (out of scope, RF07)");
        {
            expect(! isSingleOrNoModifier(kModControl | kModAlt));
            expect(! isSingleOrNoModifier(kModControl | kModShift));
            expect(! isSingleOrNoModifier(kModAlt | kModShift));
            expect(! isSingleOrNoModifier(kModControl | kModAlt | kModShift));
        }

        beginTest("modifierLabel: readable prefix per modifier, empty when unbound (RF09)");
        {
            expectEquals(modifierLabel(0), juce::String());
            expectEquals(modifierLabel(kModControl), juce::String("Ctrl+"));
            expectEquals(modifierLabel(kModAlt), juce::String("Alt+"));
            expectEquals(modifierLabel(kModShift), juce::String("Shift+"));
        }
    }
};

static ModifierTranslationTests modifierTranslationTests;
