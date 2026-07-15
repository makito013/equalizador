cmake_minimum_required(VERSION 3.22)

project(Equalizador VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# ---------------------------------------------------------------------------
# Dependencies (fetched at configure time)
# ---------------------------------------------------------------------------
include(FetchContent)

# JUCE gives us cross-platform audio (CoreAudio on macOS, WASAPI on Windows)
# behind juce::AudioDeviceManager, plus the GUI toolkit. This is what lets the
# capture/DSP/mixer/output pipeline actually run locally on the Mac.
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.14
    GIT_SHALLOW    TRUE)

# SoundTouch (LGPL) provides the pitch-shift engine wrapped by PitchShifter.
# Built from source here for local dev; a Windows release must link it
# dynamically to comply with the LGPL (see PitchShifter.cpp note).
set(SOUNDSTRETCH   OFF CACHE BOOL "" FORCE) # skip the bundled CLI utility
set(INTEGER_SAMPLES OFF CACHE BOOL "" FORCE) # use float samples
FetchContent_Declare(SoundTouch
    GIT_REPOSITORY https://codeberg.org/soundtouch/soundtouch.git
    GIT_TAG        2.4.1
    GIT_SHALLOW    TRUE)

FetchContent_MakeAvailable(JUCE SoundTouch)

# ---------------------------------------------------------------------------
# Source lists
# ---------------------------------------------------------------------------
set(EQ_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Source")

# Cross-platform core logic (no GUI). Compiled into both the app and the tests.
set(EQ_CORE_SOURCES
    "${EQ_SOURCE_DIR}/DSP/PitchShifter.cpp"
    "${EQ_SOURCE_DIR}/DSP/RobotEffect.cpp"
    "${EQ_SOURCE_DIR}/DSP/MonsterEffect.cpp"
    "${EQ_SOURCE_DIR}/DSP/AlienEffect.cpp"
    "${EQ_SOURCE_DIR}/DSP/ChipmunkEffect.cpp"
    "${EQ_SOURCE_DIR}/DSP/DemonEffect.cpp"
    "${EQ_SOURCE_DIR}/DSP/EffectChain.cpp"
    "${EQ_SOURCE_DIR}/Mixer/SoftLimiter.cpp"
    "${EQ_SOURCE_DIR}/Mixer/AudioMixer.cpp"
    "${EQ_SOURCE_DIR}/Soundboard/SoundboardEngine.cpp"
    "${EQ_SOURCE_DIR}/Capture/AudioCaptureSource.cpp"
    "${EQ_SOURCE_DIR}/Output/OutputRouter.cpp"
    "${EQ_SOURCE_DIR}/Persistence/ConfigManager.cpp"
    "${EQ_SOURCE_DIR}/Hotkeys/Win32HotkeyBackend.cpp"
    "${EQ_SOURCE_DIR}/Installer/VBCableDetector.cpp")

set(EQ_UI_SOURCES
    "${EQ_SOURCE_DIR}/UI/BypassIndicator.cpp"
    "${EQ_SOURCE_DIR}/UI/EffectSegmentedControl.cpp"
    "${EQ_SOURCE_DIR}/UI/HotkeyCaptureButton.cpp"
    "${EQ_SOURCE_DIR}/UI/SoundboardSlotView.cpp"
    "${EQ_SOURCE_DIR}/MainComponent.cpp"
    "${EQ_SOURCE_DIR}/Main.cpp")

# ---------------------------------------------------------------------------
# Application target
# ---------------------------------------------------------------------------
juce_add_gui_app(EqualizadorApp
    PRODUCT_NAME "Equalizador"
    COMPANY_NAME "Bruno"
    NEEDS_CURL FALSE
    NEEDS_WEB_BROWSER FALSE)

target_sources(EqualizadorApp PRIVATE ${EQ_CORE_SOURCES} ${EQ_UI_SOURCES})

target_include_directories(EqualizadorApp PRIVATE "${EQ_SOURCE_DIR}")

target_compile_definitions(EqualizadorApp PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:EqualizadorApp,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:EqualizadorApp,JUCE_VERSION>"
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(EqualizadorApp PRIVATE
    SoundTouch
    juce::juce_audio_utils
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

# ---------------------------------------------------------------------------
# Unit tests (JUCE UnitTest + a small console runner)
# ---------------------------------------------------------------------------
enable_testing()

juce_add_console_app(EqualizadorTests
    PRODUCT_NAME "EqualizadorTests"
    NEEDS_CURL FALSE)

target_sources(EqualizadorTests PRIVATE
    "${EQ_SOURCE_DIR}/DSP/PitchShifter.cpp"
    "${EQ_SOURCE_DIR}/DSP/RobotEffect.cpp"
    "${EQ_SOURCE_DIR}/DSP/MonsterEffect.cpp"
    "${EQ_SOURCE_DIR}/DSP/AlienEffect.cpp"
    "${EQ_SOURCE_DIR}/DSP/ChipmunkEffect.cpp"
    "${EQ_SOURCE_DIR}/DSP/DemonEffect.cpp"
    "${EQ_SOURCE_DIR}/DSP/EffectChain.cpp"
    "${EQ_SOURCE_DIR}/Mixer/SoftLimiter.cpp"
    "${EQ_SOURCE_DIR}/Mixer/AudioMixer.cpp"
    "${EQ_SOURCE_DIR}/Soundboard/SoundboardEngine.cpp"
    "${EQ_SOURCE_DIR}/Persistence/ConfigManager.cpp"
    "${EQ_SOURCE_DIR}/Installer/VBCableDetector.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/TestMain.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/HotkeyEventQueueTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/AppStateTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/SoftLimiterTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/AudioMixerTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/ConfigManagerTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/EffectChainTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/SoundboardEngineTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/VBCableDetectorTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/ModifierTranslationTests.cpp")

target_include_directories(EqualizadorTests PRIVATE "${EQ_SOURCE_DIR}")

target_compile_definitions(EqualizadorTests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_STANDALONE_APPLICATION=1)

target_link_libraries(EqualizadorTests PRIVATE
    SoundTouch
    juce::juce_core
    juce::juce_data_structures
    juce::juce_audio_basics
    juce::juce_audio_formats
    juce::juce_audio_devices
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)

add_test(NAME EqualizadorUnitTests COMMAND EqualizadorTests)
