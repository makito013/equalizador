#include "MainComponent.h"

namespace eq
{
MainComponent::MainComponent()
{
    // --- Audio device ---
    if (const auto error = deviceManager.initialiseWithDefaultDevices(2, 2); error.isNotEmpty())
        juce::Logger::writeToLog("Audio init warning: " + error);

    // --- Hotkeys (Noop on macOS, Win32 on Windows) ---
    hotkeys = createPlatformHotkeyBackend();
    hotkeys->setCallback([this](int id)
    {
        // Runs on the OS input thread: the sole producer of the SPSC queue.
        if (id == kBypassHotkeyId)
            hotkeyQueue.push(Command::toggleBypass());
        else if (id >= kSlotHotkeyBase)
            hotkeyQueue.push(Command::playSlot(id - kSlotHotkeyBase));
    });
    hotkeys->registerHotkey({ kBypassHotkeyId, 0, 0 });
    hotkeys->registerHotkey({ kSlotHotkeyBase + 0, 0, 0 });

    // --- UI ---
    addAndMakeVisible(deviceLabel);
    deviceLabel.setJustificationType(juce::Justification::centredLeft);
    deviceLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    deviceLabel.setText("Dispositivo: " + deviceManager.getCurrentAudioDeviceType(),
                        juce::dontSendNotification);

    addAndMakeVisible(bypassIndicator);

    addAndMakeVisible(bypassButton);
    bypassButton.setClickingTogglesState(true);
    bypassButton.onClick = [this]
    {
        // UI thread writes the atomic directly (not via the hotkey queue).
        state.bypass.store(bypassButton.getToggleState(), std::memory_order_relaxed);
    };

    addAndMakeVisible(effectControl);
    effectControl.onEffectChosen = [this](VoiceEffectType effect)
    {
        state.currentEffect.store(effect, std::memory_order_relaxed);
    };

    addAndMakeVisible(soundButton);
    soundButton.onClick = [this] { soundboard.trigger(0); };

    auto setupSlider = [this](juce::Slider& slider, std::atomic<float>& target)
    {
        addAndMakeVisible(slider);
        slider.setRange(0.0, 2.0, 0.01);
        slider.setValue(1.0, juce::dontSendNotification);
        slider.onValueChange = [&slider, &target]
        {
            target.store(static_cast<float>(slider.getValue()), std::memory_order_relaxed);
        };
    };
    setupSlider(voiceSlider, state.voiceGain);
    setupSlider(soundboardSlider, state.soundboardGain);

    for (auto* label : { &voiceLabel, &soundboardLabel })
    {
        addAndMakeVisible(*label);
        label->setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    }

    setSize(380, 520);

    deviceManager.addAudioCallback(this);
    startTimerHz(15);
}

MainComponent::~MainComponent()
{
    stopTimer();
    deviceManager.removeAudioCallback(this);
    if (hotkeys != nullptr)
        hotkeys->unregisterAll();
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(16);

    deviceLabel.setBounds(area.removeFromTop(24));
    area.removeFromTop(8);

    bypassIndicator.setBounds(area.removeFromTop(150));
    bypassButton.setBounds(area.removeFromTop(32).reduced(60, 0));
    area.removeFromTop(12);

    effectControl.setBounds(area.removeFromTop(40));
    area.removeFromTop(12);

    soundButton.setBounds(area.removeFromTop(64).reduced(80, 0));
    area.removeFromTop(12);

    auto sliderRow = [&area](juce::Label& label, juce::Slider& slider)
    {
        auto row = area.removeFromTop(32);
        label.setBounds(row.removeFromLeft(48));
        slider.setBounds(row);
    };
    sliderRow(voiceLabel, voiceSlider);
    area.removeFromTop(6);
    sliderRow(soundboardLabel, soundboardSlider);
}

void MainComponent::timerCallback()
{
    // Reflect state that hotkeys may have changed behind the UI's back.
    bypassButton.setToggleState(state.bypass.load(std::memory_order_relaxed),
                                juce::dontSendNotification);
    effectControl.setSelected(state.currentEffect.load(std::memory_order_relaxed));

    BypassIndicator::State indicatorState;
    if (state.bypass.load(std::memory_order_relaxed))
        indicatorState = BypassIndicator::State::Bypassed;
    else if (! state.signalPresent.load(std::memory_order_relaxed))
        indicatorState = BypassIndicator::State::NoSignal;
    else
        indicatorState = BypassIndicator::State::Active;
    bypassIndicator.setState(indicatorState);
}

void MainComponent::applyCommand(const Command& command) noexcept
{
    switch (command.type)
    {
        case Command::Type::ToggleBypass: state.toggleBypass(); break;
        case Command::Type::PlaySlot:     soundboard.trigger(command.slot); break;
        case Command::Type::None:
        default:                          break;
    }
}

void MainComponent::resizeWorkBuffer(juce::AudioBuffer<float>& buffer, int numSamples) noexcept
{
    buffer.setSize(workChannels, juce::jmin(numSamples, workCapacity),
                   /*keepContent*/ false, /*clearExtra*/ false, /*avoidReallocating*/ true);
}

void MainComponent::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    const double sampleRate = device->getCurrentSampleRate();
    const int blockSize = device->getCurrentBufferSizeSamples();
    const int ins = device->getActiveInputChannels().countNumberOfSetBits();
    const int outs = device->getActiveOutputChannels().countNumberOfSetBits();

    workChannels = juce::jmax(1, juce::jmax(ins, outs));
    workCapacity = blockSize;

    capture.prepare(blockSize, workChannels);
    sfxBuffer.setSize(workChannels, blockSize);
    sfxBuffer.clear();
    mixBuffer.setSize(workChannels, blockSize);
    mixBuffer.clear();

    effectChain.prepare(sampleRate, blockSize, workChannels);
    mixer.prepare(sampleRate, blockSize, workChannels);
    soundboard.prepare(sampleRate, blockSize, workChannels);
}

void MainComponent::audioDeviceStopped() {}

void MainComponent::audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                                     int numInputChannels,
                                                     float* const* outputChannelData,
                                                     int numOutputChannels,
                                                     int numSamples,
                                                     const juce::AudioIODeviceCallbackContext&)
{
    // 1. Drain hotkey commands (bounded, allocation-free).
    Command command;
    while (hotkeyQueue.pop(command))
        applyCommand(command);

    // 2. Capture microphone input into the working voice buffer.
    auto& voice = capture.readFrom(inputChannelData, numInputChannels, numSamples);
    state.signalPresent.store(capture.getLastRms() > 0.003f, std::memory_order_relaxed);

    // 3. Voice effect (honours bypass internally).
    effectChain.process(voice);

    // 4. Soundboard render.
    resizeWorkBuffer(sfxBuffer, numSamples);
    sfxBuffer.clear();
    soundboard.renderNextBlock(sfxBuffer, sfxBuffer.getNumSamples());

    // 5. Mix voice + soundboard with independent gains and limiting.
    resizeWorkBuffer(mixBuffer, numSamples);
    mixer.mix(voice, sfxBuffer, mixBuffer,
              state.voiceGain.load(std::memory_order_relaxed),
              state.soundboardGain.load(std::memory_order_relaxed));

    // 6. Route to the output device (VB-CABLE endpoint on Windows).
    output.writeTo(outputChannelData, numOutputChannels, numSamples, mixBuffer);
}
} // namespace eq
