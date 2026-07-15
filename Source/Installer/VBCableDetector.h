#pragma once

#include <juce_core/juce_core.h>

// Forward-declared (not #included) so this header stays free of the
// juce_audio_devices dependency - findCableInputName() is pure juce_core and
// is exercised directly by VBCableDetectorTests, which doesn't link that
// module. detect() only needs a reference, so the forward declaration is
// enough here; the .cpp includes the real header where it's actually used.
namespace juce
{
class AudioDeviceManager;
}

namespace eq
{
/**
    Detects whether the VB-CABLE virtual audio device is installed, so
    MainComponent can force audio output to it instead of whatever Windows'
    current default output device happens to be (ADR-001).

    Real detection (detect()) enumerates Windows render endpoints via the
    caller's juce::AudioDeviceManager and is genuinely Windows-specific; see
    VBCableDetector.cpp for the platform guard. On macOS (and any other
    non-Windows dev build) detect() always reports "not installed" so the
    app falls back to the default output device.
*/
class VBCableDetector
{
public:
    struct Result
    {
        bool installed { false };
        juce::String renderEndpointName; // device to route output to when present
    };

    /**
        Must be called before `deviceManager` is initialised (so
        getAvailableDeviceTypes()/scanForDevices() see a clean slate) and off
        the audio thread - this only ever runs during MainComponent
        construction, well before addAudioCallback().
    */
    static Result detect(juce::AudioDeviceManager& deviceManager);

    /**
        Pure, testable matching logic: given a list of output device names,
        returns the first one that looks like the VB-CABLE "CABLE Input"
        render endpoint. Match is a case-insensitive *prefix* match (not a
        full-string match), so version/index suffixes such as
        "CABLE Input (VB-Audio Virtual Cable) 2" still count. Returns an
        empty string when nothing matches. If more than one device matches,
        logs every candidate and returns the first.
    */
    static juce::String findCableInputName(const juce::StringArray& deviceNames);
};
} // namespace eq
