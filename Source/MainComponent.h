#pragma once

#include "Capture/AudioCaptureSource.h"
#include "DSP/EffectChain.h"
#include "Hotkeys/HotkeyEventQueue.h"
#include "Hotkeys/Win32HotkeyBackend.h"
#include "Mixer/AudioMixer.h"
#include "Output/OutputRouter.h"
#include "Soundboard/SoundboardEngine.h"
#include "State/AppState.h"
#include "UI/BypassIndicator.h"
#include "UI/EffectSegmentedControl.h"

#include <memory>

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace eq
{
/**
    Top-level component. Owns the audio device, wires the real-time pipeline
    (Capture -> EffectChain -> Mixer <- Soundboard -> Output) inside a real
    juce::AudioIODeviceCallback, and hosts the minimal control UI.

    Threading contract:
      - Audio thread: only the callback. It loads AppState atomics, drains the
        hotkey queue, and touches preallocated buffers. No allocation, no locks.
      - UI thread: writes AppState atomics directly and triggers soundboard pads.
      - Hotkey/OS thread: the sole producer pushing onto the SPSC hotkey queue.
*/
class MainComponent : public juce::Component,
                      public juce::AudioIODeviceCallback,
                      private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    // Component
    void resized() override;
    void paint(juce::Graphics& g) override;

    // AudioIODeviceCallback
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                          int numInputChannels,
                                          float* const* outputChannelData,
                                          int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext& context) override;

private:
    void timerCallback() override;
    void applyCommand(const Command& command) noexcept;
    void resizeWorkBuffer(juce::AudioBuffer<float>& buffer, int numSamples) noexcept;

    static constexpr int kBypassHotkeyId = 1;
    static constexpr int kSlotHotkeyBase = 100;

    // --- Audio engine ---
    juce::AudioDeviceManager deviceManager;
    AppState state;
    EffectChain effectChain { state };
    AudioMixer mixer;
    SoundboardEngine soundboard;
    AudioCaptureSource capture;
    OutputRouter output;

    juce::AudioBuffer<float> sfxBuffer;
    juce::AudioBuffer<float> mixBuffer;
    int workCapacity { 0 };
    int workChannels { 2 };

    // --- Hotkeys ---
    HotkeyEventQueue hotkeyQueue;
    std::unique_ptr<IHotkeyBackend> hotkeys;

    // --- UI ---
    BypassIndicator bypassIndicator;
    EffectSegmentedControl effectControl;
    juce::TextButton bypassButton { "Bypass" };
    juce::TextButton soundButton { "Sound 1" };
    juce::Slider voiceSlider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    juce::Slider soundboardSlider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    juce::Label voiceLabel { {}, "Voz" };
    juce::Label soundboardLabel { {}, "Sons" };
    juce::Label deviceLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
} // namespace eq
