#include "Installer/VBCableDetector.h"

#include <juce_core/juce_core.h>

using namespace eq;

// RNF03: cover VBCableDetector::findCableInputName() - the pure, matching-only
// half of detection. The other half (enumerating real Windows render
// endpoints via juce::AudioDeviceManager, in detect()) needs actual hardware
// and is deliberately not unit-tested here.
class VBCableDetectorTests : public juce::UnitTest
{
public:
    VBCableDetectorTests() : juce::UnitTest("VBCableDetector", "eq") {}

    void runTest() override
    {
        beginTest("exact CABLE Input name matches");
        {
            const juce::StringArray names { "Speakers (Realtek)",
                                            "CABLE Input (VB-Audio Virtual Cable)",
                                            "Headphones" };
            expectEquals(VBCableDetector::findCableInputName(names),
                        juce::String("CABLE Input (VB-Audio Virtual Cable)"));
        }

        beginTest("name with index/version suffix still matches (prefix, not full-string)");
        {
            const juce::StringArray names { "CABLE Input (VB-Audio Virtual Cable) 2" };
            expectEquals(VBCableDetector::findCableInputName(names),
                        juce::String("CABLE Input (VB-Audio Virtual Cable) 2"));
        }

        beginTest("match is case-insensitive");
        {
            const juce::StringArray names { "cable input (vb-audio virtual cable)" };
            expectEquals(VBCableDetector::findCableInputName(names),
                        juce::String("cable input (vb-audio virtual cable)"));
        }

        beginTest("no match returns empty string");
        {
            const juce::StringArray names { "Speakers (Realtek)", "Headphones" };
            expect(VBCableDetector::findCableInputName(names).isEmpty());
        }

        beginTest("empty device list returns empty string");
        {
            const juce::StringArray names;
            expect(VBCableDetector::findCableInputName(names).isEmpty());
        }

        beginTest("multiple matches: first one wins");
        {
            const juce::StringArray names { "Speakers",
                                            "CABLE Input (VB-Audio Virtual Cable)",
                                            "CABLE Input (VB-Audio Virtual Cable) 2" };
            expectEquals(VBCableDetector::findCableInputName(names),
                        juce::String("CABLE Input (VB-Audio Virtual Cable)"));
        }

        beginTest("prefix match only - 'CABLE Input' occurring mid-string does not match");
        {
            const juce::StringArray names { "Line 1 (CABLE Input)" };
            expect(VBCableDetector::findCableInputName(names).isEmpty());
        }
    }
};

static VBCableDetectorTests vbCableDetectorTests;
