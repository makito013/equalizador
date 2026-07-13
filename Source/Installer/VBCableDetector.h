#pragma once

#include <juce_core/juce_core.h>

namespace eq
{
/**
    Detects whether the VB-CABLE virtual audio device is installed, so the first
    run wizard can offer to install it silently (ADR-001).

    STUB: real detection enumerates Windows audio endpoints looking for the
    "CABLE Input/Output (VB-Audio Virtual Cable)" devices, which is genuinely
    Windows-specific. Not the focus of this round — see the "Não implementado"
    section of the Dev report. On macOS isInstalled() always returns false.
*/
class VBCableDetector
{
public:
    struct Result
    {
        bool installed { false };
        juce::String renderEndpointName; // device to route output to when present
    };

    static Result detect();
};
} // namespace eq
