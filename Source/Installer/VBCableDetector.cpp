#include "VBCableDetector.h"

namespace eq
{
namespace
{
    constexpr const char* kCableInputPrefix = "CABLE Input";
}

juce::String VBCableDetector::findCableInputName(const juce::StringArray& deviceNames)
{
    juce::StringArray candidates;
    for (const auto& name : deviceNames)
        if (name.startsWithIgnoreCase(kCableInputPrefix))
            candidates.add(name);

    if (candidates.isEmpty())
        return {};

    if (candidates.size() > 1)
        juce::Logger::writeToLog("VBCableDetector: multiplos candidatos \"CABLE Input\" encontrados ("
                                 + candidates.joinIntoString("; ") + "); usando o primeiro.");

    return candidates[0];
}
} // namespace eq

// The Windows implementation below enumerates real render endpoints (WASAPI/
// DirectSound/ASIO, whatever JUCE registers on this machine) and is only
// meaningful - and only compiles - on Windows. Same guard/structure as
// Win32HotkeyBackend.cpp: on this macOS dev machine only the trivial
// fallback further below is built.
#if defined(_WIN32)

    #include <juce_audio_devices/juce_audio_devices.h>

namespace eq
{
VBCableDetector::Result VBCableDetector::detect(juce::AudioDeviceManager& deviceManager)
{
    for (auto* type : deviceManager.getAvailableDeviceTypes())
    {
        if (type == nullptr)
            continue;

        // Device names aren't populated until a scan has run at least once;
        // the AudioDeviceManager normally does this lazily inside
        // initialise(), but we're deliberately called before that.
        type->scanForDevices();

        const auto match = findCableInputName(type->getDeviceNames(false /* wantInputNames */));
        if (match.isNotEmpty())
        {
            juce::Logger::writeToLog("VBCableDetector: VB-CABLE encontrado (\"" + match
                                     + "\") via driver \"" + type->getTypeName() + "\".");
            return { true, match };
        }
    }

    juce::Logger::writeToLog("VBCableDetector: VB-CABLE nao encontrado entre os dispositivos de saida.");
    return {};
}
} // namespace eq

#else

namespace eq
{
VBCableDetector::Result VBCableDetector::detect(juce::AudioDeviceManager& deviceManager)
{
    juce::ignoreUnused(deviceManager);
    return {};
}
} // namespace eq

#endif // _WIN32
