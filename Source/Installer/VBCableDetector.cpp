#include "VBCableDetector.h"

namespace eq
{
VBCableDetector::Result VBCableDetector::detect()
{
    // TODO(windows): enumerate render endpoints via the JUCE device manager (or
    // WASAPI IMMDeviceEnumerator) and match the VB-Audio Virtual Cable name.
    // Deliberately unimplemented this round; returns "not installed" so the app
    // falls back to the default output device on macOS dev.
    return {};
}
} // namespace eq
