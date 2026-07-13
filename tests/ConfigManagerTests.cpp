#include "Persistence/ConfigManager.h"

#include <juce_core/juce_core.h>

using namespace eq;

namespace
{
AppConfig makeSampleConfig()
{
    AppConfig config;
    config.inputDeviceName = "Built-in Microphone";
    config.outputDeviceName = "CABLE Input";
    config.effect = VoiceEffectType::Monster;
    config.bypass = true;
    config.voiceGain = 0.75f;
    config.soundboardGain = 1.25f;
    config.soundboardFolder = "/Users/bruno/sounds";
    config.bypassKeyCode = 122;      // e.g. F11
    config.bypassModifiers = 0x0002; // e.g. control

    config.slots.push_back({ "/Users/bruno/sounds/airhorn.wav", 49, 0x0004 });
    config.slots.push_back({ "/Users/bruno/sounds/laugh.wav", 50, 0 });
    return config;
}
}

class ConfigManagerTests : public juce::UnitTest
{
public:
    ConfigManagerTests() : juce::UnitTest("ConfigManager", "eq") {}

    void runTest() override
    {
        beginTest("in-memory var round-trip");
        {
            const auto original = makeSampleConfig();
            const auto restored = ConfigManager::fromVar(ConfigManager::toVar(original));
            expect(restored == original, "var round-trip changed the config");
        }

        beginTest("file save/load round-trip");
        {
            const auto original = makeSampleConfig();
            auto file = juce::File::getSpecialLocation(juce::File::tempDirectory)
                            .getChildFile("equalizador_test_config.json");
            file.deleteFile();

            expect(ConfigManager::save(original, file), "save failed");
            expect(file.existsAsFile());

            bool ok = false;
            const auto loaded = ConfigManager::load(file, ok);
            expect(ok, "load reported failure on a valid file");
            expect(loaded == original, "file round-trip changed the config");

            file.deleteFile();
        }

        beginTest("missing file yields defaults and reports failure");
        {
            auto missing = juce::File::getSpecialLocation(juce::File::tempDirectory)
                               .getChildFile("equalizador_does_not_exist.json");
            missing.deleteFile();

            bool ok = true;
            const auto loaded = ConfigManager::load(missing, ok);
            expect(! ok, "missing file should report failure");
            expect(loaded == AppConfig {}, "missing file should return defaults");
        }

        beginTest("malformed JSON yields defaults");
        {
            auto file = juce::File::getSpecialLocation(juce::File::tempDirectory)
                            .getChildFile("equalizador_bad_config.json");
            file.replaceWithText("{ this is not valid json ");

            bool ok = true;
            const auto loaded = ConfigManager::load(file, ok);
            expect(! ok, "malformed file should report failure");
            expect(loaded == AppConfig {});

            file.deleteFile();
        }
    }
};

static ConfigManagerTests configManagerTests;
