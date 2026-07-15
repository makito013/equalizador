#include "MainComponent.h"

#include "Installer/VBCableDetector.h"

namespace eq
{
MainComponent::MainComponent()
{
    // --- Audio device ---
    // RF01-RF05: detect VB-CABLE *before* the device manager is initialised
    // (RNF01: off the audio thread, well before addAudioCallback() below) so
    // that when it's present we can force the render endpoint to it -
    // otherwise initialiseWithDefaultDevices() would silently pick whatever
    // Windows' current default output happens to be at launch.
    const auto vbCable = VBCableDetector::detect(deviceManager);

    if (const auto error = deviceManager.initialiseWithDefaultDevices(2, 2); error.isNotEmpty())
        juce::Logger::writeToLog("Audio init warning: " + error);

    if (vbCable.installed)
    {
        juce::AudioDeviceManager::AudioDeviceSetup setup;
        deviceManager.getAudioDeviceSetup(setup);
        setup.outputDeviceName = vbCable.renderEndpointName;

        const auto error = deviceManager.setAudioDeviceSetup(setup, /*treatAsChosenDevice*/ true);
        if (error.isNotEmpty() || deviceManager.getCurrentAudioDevice() == nullptr)
        {
            // RF05 (corrigido): ao contrario do que este comentario afirmava
            // antes, AudioDeviceManager::setAudioDeviceSetup() NAO restaura o
            // setup anterior em caso de falha quando uma troca de device e
            // necessaria - ele derruba o device atual antes de tentar abrir o
            // novo, e todo caminho de erro retorna sem reabrir nada (ver
            // juce_AudioDeviceManager.cpp, setAudioDeviceSetup(): stopDevice()/
            // deleteCurrentDevice() rodam antes da tentativa, e os `return
            // error;` intermediarios nao chamam initialiseWithDefaultDevices
            // nem equivalente). Sem essa chamada explicita, uma falha aqui
            // deixaria o app sem nenhum device de audio aberto (silencio
            // total), nao "no padrao do Windows" como pretendido - entao
            // restauramos de verdade chamando initialiseWithDefaultDevices de
            // novo.
            const auto detail = error.isNotEmpty() ? error
                                                    : juce::String("nenhum dispositivo de audio ficou aberto");
            juce::Logger::writeToLog("Falha ao selecionar saida VB-CABLE (\"" + vbCable.renderEndpointName
                                     + "\"): " + detail + " - restaurando dispositivo padrao do sistema.");

            if (const auto restoreError = deviceManager.initialiseWithDefaultDevices(2, 2);
                restoreError.isNotEmpty())
                juce::Logger::writeToLog("Falha ao restaurar dispositivo padrao apos erro de VB-CABLE: "
                                         + restoreError);
        }
        else
        {
            juce::Logger::writeToLog("VB-CABLE detectado; saida forcada para \""
                                     + vbCable.renderEndpointName + "\".");
        }
    }
    else
    {
        juce::Logger::writeToLog("VB-CABLE nao detectado (ou build nao-Windows); usando saida padrao do sistema.");
    }

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

    // --- UI ---
    addAndMakeVisible(deviceLabel);
    deviceLabel.setJustificationType(juce::Justification::centredLeft);
    deviceLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);

    // RF06: show which output device actually got selected, not just the
    // driver type - the only way to tell from the UI whether VB-CABLE
    // selection above succeeded or the app fell back to the system default.
    const auto* currentOutputDevice = deviceManager.getCurrentAudioDevice();
    const auto outputDeviceDescription = currentOutputDevice != nullptr
                                            ? currentOutputDevice->getName()
                                            : juce::String("(nenhum dispositivo)");
    deviceLabel.setText("Dispositivo: " + deviceManager.getCurrentAudioDeviceType()
                        + " | Saida: " + outputDeviceDescription,
                        juce::dontSendNotification);

    addAndMakeVisible(bypassIndicator);

    addAndMakeVisible(bypassButton);
    bypassButton.setClickingTogglesState(true);
    bypassButton.onClick = [this]
    {
        // UI thread writes the atomic directly (not via the hotkey queue).
        state.bypass.store(bypassButton.getToggleState(), std::memory_order_relaxed);
    };

    addAndMakeVisible(bypassHotkeyButton);
    bypassHotkeyButton.onHotkeyChosen = [this](int keyCode, uint32_t modifiers)
    {
        captureBypassHotkey(keyCode, modifiers);
    };

    addAndMakeVisible(effectControl);
    effectControl.onEffectChosen = [this](VoiceEffectType effect)
    {
        state.currentEffect.store(effect, std::memory_order_relaxed);
        refreshTimbreSliderForEffect(effect);
    };

    addAndMakeVisible(timbreLabel);
    timbreLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(timbreSlider);
    timbreSlider.onValueChange = [this]
    {
        const auto effect = state.currentEffect.load(std::memory_order_relaxed);
        const auto idx = static_cast<size_t>(effect);
        state.effectTimbre[idx].store(static_cast<float>(timbreSlider.getValue()),
                                      std::memory_order_relaxed);
    };
    refreshTimbreSliderForEffect(VoiceEffectType::Normal);

    for (int i = 0; i < SoundboardEngine::numSlots; ++i)
    {
        auto view = std::make_unique<SoundboardSlotView>(i);
        view->onTrigger = [this](int index) { soundboard.trigger(index); };
        view->onPickFile = [this](int index) { pickSoundFile(index); };
        view->onHotkeyChosen = [this](int index, int keyCode, uint32_t modifiers)
        {
            captureSlotHotkey(index, keyCode, modifiers);
        };
        addAndMakeVisible(*view);
        slotViews.push_back(std::move(view));
    }

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

    setSize(460, 900);

    // Starting the audio callback runs audioDeviceAboutToStart() synchronously
    // (the device is already open), which calls soundboard.prepare() - only
    // after that is loadSlot() allowed to succeed, so config loading (which
    // may call loadSlot()) must happen after this line.
    deviceManager.addAudioCallback(this);

    loadPersistedConfig();

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

    bypassIndicator.setBounds(area.removeFromTop(120));
    area.removeFromTop(8);

    auto bypassRow = area.removeFromTop(32);
    bypassHotkeyButton.setBounds(bypassRow.removeFromRight(72));
    bypassRow.removeFromRight(4);
    bypassButton.setBounds(bypassRow);
    area.removeFromTop(12);

    effectControl.setBounds(area.removeFromTop(72));
    area.removeFromTop(8);

    auto timbreRow = area.removeFromTop(32);
    timbreLabel.setBounds(timbreRow.removeFromLeft(56));
    timbreSlider.setBounds(timbreRow);
    area.removeFromTop(12);

    for (auto& view : slotViews)
    {
        view->setBounds(area.removeFromTop(28));
        area.removeFromTop(4);
    }
    area.removeFromTop(8);

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

void MainComponent::refreshTimbreSliderForEffect(VoiceEffectType effect)
{
    const auto range = timbreRangeFor(effect);
    timbreSlider.setEnabled(range.enabled);
    if (range.enabled)
        timbreSlider.setRange(static_cast<double>(range.min), static_cast<double>(range.max), 0.5);

    const auto idx = static_cast<size_t>(effect);
    const float current = state.effectTimbre[idx].load(std::memory_order_relaxed);
    timbreSlider.setValue(static_cast<double>(current), juce::dontSendNotification);
}

void MainComponent::updateSlotLabel(int index)
{
    const auto label = soundboard.isSlotLoaded(index) ? soundboard.getSlotName(index)
                                                       : juce::String("Vazio");
    slotViews[static_cast<size_t>(index)]->setLabel(label);
}

void MainComponent::pickSoundFile(int index)
{
    auto chooser = std::make_shared<juce::FileChooser>(
        "Selecionar som", juce::File(), soundboard.getFormatManager().getWildcardForAllFormats());

    constexpr auto chooserFlags = juce::FileBrowserComponent::openMode
                                 | juce::FileBrowserComponent::canSelectFiles;

    juce::Component::SafePointer<MainComponent> safeThis(this);
    chooser->launchAsync(chooserFlags, [safeThis, index, chooser](const juce::FileChooser& fc)
    {
        if (safeThis == nullptr)
            return; // component was closed while the dialog was open

        const auto file = fc.getResult();
        if (! file.existsAsFile())
            return; // user cancelled: leave the slot as it was

        if (! safeThis->soundboard.loadSlot(index, file))
        {
            // RNF03/RF06: a bad/unreadable file must not crash the app and
            // must not leave stale state; keep whatever was loaded before.
            juce::Logger::writeToLog("Falha ao carregar som para o slot " + juce::String(index));
            return;
        }

        safeThis->currentConfig.slots[static_cast<size_t>(index)].filePath = file.getFullPathName();
        safeThis->updateSlotLabel(index);
        safeThis->persistConfig();
    });
}

void MainComponent::registerExclusiveHotkey(int id, int keyCode, uint32_t modifiers)
{
    // Last-write-wins (RF07/RF07b/RF11): if another id already owns this
    // (keyCode, modifiers) *pair*, explicitly steal it - unregister the old
    // owner and clear its UI/config. F1 and Ctrl+F1 are distinct pairs and
    // may coexist on different ids without conflict.
    if (id != kBypassHotkeyId && currentConfig.bypassKeyCode == keyCode
        && currentConfig.bypassModifiers == modifiers)
    {
        hotkeys->unregisterHotkey(kBypassHotkeyId);
        currentConfig.bypassKeyCode = 0;
        currentConfig.bypassModifiers = 0;
        bypassHotkeyButton.setBinding(0, 0);
    }

    for (int i = 0; i < SoundboardEngine::numSlots; ++i)
    {
        const int slotId = kSlotHotkeyBase + i;
        if (slotId == id)
            continue;

        auto& slotConfig = currentConfig.slots[static_cast<size_t>(i)];
        if (slotConfig.keyCode == keyCode && slotConfig.modifiers == modifiers)
        {
            hotkeys->unregisterHotkey(slotId);
            slotConfig.keyCode = 0;
            slotConfig.modifiers = 0;
            slotViews[static_cast<size_t>(i)]->setBinding(0, 0);
        }
    }

    if (! hotkeys->registerHotkey({ id, keyCode, modifiers }))
        juce::Logger::writeToLog("Falha ao registrar hotkey para id " + juce::String(id));
}

void MainComponent::captureSlotHotkey(int index, int keyCode, uint32_t modifiers)
{
    registerExclusiveHotkey(kSlotHotkeyBase + index, keyCode, modifiers);
    currentConfig.slots[static_cast<size_t>(index)].keyCode = keyCode;
    currentConfig.slots[static_cast<size_t>(index)].modifiers = modifiers;
    persistConfig();
}

void MainComponent::captureBypassHotkey(int keyCode, uint32_t modifiers)
{
    registerExclusiveHotkey(kBypassHotkeyId, keyCode, modifiers);
    currentConfig.bypassKeyCode = keyCode;
    currentConfig.bypassModifiers = modifiers;
    persistConfig();
}

void MainComponent::loadPersistedConfig()
{
    currentConfig = ConfigManager::load(ConfigManager::getDefaultConfigFile());
    currentConfig.slots.resize(static_cast<size_t>(SoundboardEngine::numSlots));

    // Apply the simple scalar state.
    state.bypass.store(currentConfig.bypass, std::memory_order_relaxed);
    state.currentEffect.store(currentConfig.effect, std::memory_order_relaxed);
    state.voiceGain.store(currentConfig.voiceGain, std::memory_order_relaxed);
    state.soundboardGain.store(currentConfig.soundboardGain, std::memory_order_relaxed);
    for (size_t i = 0; i < currentConfig.effectTimbre.size(); ++i)
        state.effectTimbre[i].store(currentConfig.effectTimbre[i], std::memory_order_relaxed);

    voiceSlider.setValue(static_cast<double>(currentConfig.voiceGain), juce::dontSendNotification);
    soundboardSlider.setValue(static_cast<double>(currentConfig.soundboardGain), juce::dontSendNotification);
    bypassButton.setToggleState(currentConfig.bypass, juce::dontSendNotification);
    effectControl.setSelected(currentConfig.effect);
    refreshTimbreSliderForEffect(currentConfig.effect);

    // RF09: re-register the bypass hotkey if one was persisted.
    bypassHotkeyButton.setBinding(currentConfig.bypassKeyCode, currentConfig.bypassModifiers);
    if (currentConfig.bypassKeyCode != 0)
    {
        if (! hotkeys->registerHotkey(
                { kBypassHotkeyId, currentConfig.bypassKeyCode, currentConfig.bypassModifiers }))
            juce::Logger::writeToLog("Falha ao re-registrar hotkey de bypass ao iniciar");
    }

    // RF09: load each slot's sound file (missing file -> "Vazio", no crash)
    // and re-register its hotkey.
    for (int i = 0; i < SoundboardEngine::numSlots; ++i)
    {
        const auto& slotConfig = currentConfig.slots[static_cast<size_t>(i)];

        if (slotConfig.filePath.isNotEmpty())
        {
            if (! soundboard.loadSlot(i, juce::File(slotConfig.filePath)))
                juce::Logger::writeToLog("Som ausente/invalido para o slot " + juce::String(i)
                                         + ": " + slotConfig.filePath);
        }
        updateSlotLabel(i);

        slotViews[static_cast<size_t>(i)]->setBinding(slotConfig.keyCode, slotConfig.modifiers);
        if (slotConfig.keyCode != 0)
        {
            if (! hotkeys->registerHotkey({ kSlotHotkeyBase + i, slotConfig.keyCode, slotConfig.modifiers }))
                juce::Logger::writeToLog("Falha ao re-registrar hotkey do slot " + juce::String(i) + " ao iniciar");
        }
    }
}

void MainComponent::persistConfig()
{
    currentConfig.effect = state.currentEffect.load(std::memory_order_relaxed);
    currentConfig.bypass = state.bypass.load(std::memory_order_relaxed);
    currentConfig.voiceGain = state.voiceGain.load(std::memory_order_relaxed);
    currentConfig.soundboardGain = state.soundboardGain.load(std::memory_order_relaxed);
    for (size_t i = 0; i < currentConfig.effectTimbre.size(); ++i)
        currentConfig.effectTimbre[i] = state.effectTimbre[i].load(std::memory_order_relaxed);

    if (! ConfigManager::save(currentConfig, ConfigManager::getDefaultConfigFile()))
        juce::Logger::writeToLog("Falha ao salvar configuracao");
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
