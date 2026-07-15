#include "Persistence/ConfigManager.h"
#include "Soundboard/SoundboardEngine.h"

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

    config.effectTimbre[static_cast<size_t>(VoiceEffectType::Robot)] = 90.0f;
    config.effectTimbre[static_cast<size_t>(VoiceEffectType::Monster)] = -3.0f;
    config.effectTimbre[static_cast<size_t>(VoiceEffectType::Alien)] = 2.5f;
    config.effectTimbre[static_cast<size_t>(VoiceEffectType::Chipmunk)] = -1.0f;
    config.effectTimbre[static_cast<size_t>(VoiceEffectType::Demon)] = 4.0f;

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

        beginTest("effectTimbre round-trips per effect (RF02)");
        {
            const auto original = makeSampleConfig();
            const auto restored = ConfigManager::fromVar(ConfigManager::toVar(original));

            for (size_t i = 0; i < kNumVoiceEffects; ++i)
                expectWithinAbsoluteError(restored.effectTimbre[i], original.effectTimbre[i], 1.0e-5f);
        }

        beginTest("a JSON payload missing effectTimbre falls back to per-effect defaults (back-compat)");
        {
            // Simulates a config saved by the previous round, before RF02
            // existed: no "effectTimbre" key at all.
            auto* root = new juce::DynamicObject();
            root->setProperty("inputDeviceName", "Mic");
            root->setProperty("effect", static_cast<int>(VoiceEffectType::Monster));
            const juce::var parsed(root);

            const auto loaded = ConfigManager::fromVar(parsed);
            const AppConfig defaults;
            for (size_t i = 0; i < kNumVoiceEffects; ++i)
                expectWithinAbsoluteError(loaded.effectTimbre[i], defaults.effectTimbre[i], 1.0e-5f);
        }

        beginTest("effect enum accepts the new voice presets (RF03) and clamps out-of-range values");
        {
            auto* root = new juce::DynamicObject();
            root->setProperty("effect", static_cast<int>(VoiceEffectType::Demon));
            const auto loadedDemon = ConfigManager::fromVar(juce::var(root));
            expect(loadedDemon.effect == VoiceEffectType::Demon);

            auto* outOfRange = new juce::DynamicObject();
            outOfRange->setProperty("effect", 999);
            const auto loadedClamped = ConfigManager::fromVar(juce::var(outOfRange));
            expect(loadedClamped.effect == VoiceEffectType::Demon,
                  "an out-of-range effect index should clamp to the highest known effect");
        }

        beginTest("an old 8-slot config resized to the new numSlots (RF04) preserves the old "
                  "slots and default-initialises the new ones");
        {
            // Simulates loading a config.json saved before RF01 bumped
            // numSlots from 8 to 12: only 8 slots in the JSON. Mirrors what
            // MainComponent::loadPersistedConfig() does right after
            // ConfigManager::load() - currentConfig.slots.resize(numSlots).
            auto* root = new juce::DynamicObject();
            juce::Array<juce::var> slotArray;
            for (int i = 0; i < 8; ++i)
            {
                auto* s = new juce::DynamicObject();
                s->setProperty("filePath", "/sounds/slot" + juce::String(i) + ".wav");
                s->setProperty("keyCode", 49 + i); // F1 + i
                s->setProperty("modifiers", static_cast<juce::int64>(0));
                slotArray.add(juce::var(s));
            }
            root->setProperty("slots", slotArray);

            auto config = ConfigManager::fromVar(juce::var(root));
            expectEquals(config.slots.size(), static_cast<size_t>(8),
                        "sanity: the simulated old config should parse exactly 8 slots");

            config.slots.resize(static_cast<size_t>(SoundboardEngine::numSlots));

            expectEquals(config.slots.size(), static_cast<size_t>(SoundboardEngine::numSlots),
                        "resize should grow to the current numSlots without erroring");

            for (int i = 0; i < 8; ++i)
            {
                const auto& slot = config.slots[static_cast<size_t>(i)];
                expectEquals(slot.filePath, juce::String("/sounds/slot" + juce::String(i) + ".wav"),
                            "pre-existing slot " + juce::String(i) + " must survive the resize untouched");
                expectEquals(slot.keyCode, 49 + i);
            }

            for (size_t i = 8; i < config.slots.size(); ++i)
                expect(config.slots[i] == SlotConfig {},
                      "newly-grown slot " + juce::String(static_cast<int>(i)) + " should default-construct empty");
        }
    }
};

static ConfigManagerTests configManagerTests;
