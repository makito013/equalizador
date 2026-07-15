#include "ConfigManager.h"

namespace eq
{
namespace
{
    constexpr int kMinEffect = static_cast<int>(VoiceEffectType::Normal);
    constexpr int kMaxEffect = static_cast<int>(VoiceEffectType::Demon);

    VoiceEffectType effectFromInt(int value) noexcept
    {
        return static_cast<VoiceEffectType>(juce::jlimit(kMinEffect, kMaxEffect, value));
    }
}

juce::File ConfigManager::getDefaultConfigFile()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Equalizador")
        .getChildFile("config.json");
}

juce::var ConfigManager::toVar(const AppConfig& config)
{
    auto* root = new juce::DynamicObject();
    root->setProperty("inputDeviceName", config.inputDeviceName);
    root->setProperty("outputDeviceName", config.outputDeviceName);
    root->setProperty("effect", static_cast<int>(config.effect));
    root->setProperty("bypass", config.bypass);
    root->setProperty("voiceGain", config.voiceGain);
    root->setProperty("soundboardGain", config.soundboardGain);
    root->setProperty("soundboardFolder", config.soundboardFolder);
    root->setProperty("bypassKeyCode", config.bypassKeyCode);
    root->setProperty("bypassModifiers", static_cast<juce::int64>(config.bypassModifiers));

    juce::Array<juce::var> timbreArray;
    for (const float value : config.effectTimbre)
        timbreArray.add(value);
    root->setProperty("effectTimbre", timbreArray);

    juce::Array<juce::var> slotArray;
    for (const auto& slot : config.slots)
    {
        auto* s = new juce::DynamicObject();
        s->setProperty("filePath", slot.filePath);
        s->setProperty("keyCode", slot.keyCode);
        s->setProperty("modifiers", static_cast<juce::int64>(slot.modifiers));
        slotArray.add(juce::var(s));
    }
    root->setProperty("slots", slotArray);

    return juce::var(root);
}

AppConfig ConfigManager::fromVar(const juce::var& v)
{
    AppConfig config;
    if (! v.isObject())
        return config;

    config.inputDeviceName = v.getProperty("inputDeviceName", "").toString();
    config.outputDeviceName = v.getProperty("outputDeviceName", "").toString();
    config.effect = effectFromInt(static_cast<int>(v.getProperty("effect", 0)));
    config.bypass = static_cast<bool>(v.getProperty("bypass", false));
    config.voiceGain = static_cast<float>(static_cast<double>(v.getProperty("voiceGain", 1.0)));
    config.soundboardGain = static_cast<float>(static_cast<double>(v.getProperty("soundboardGain", 1.0)));
    config.soundboardFolder = v.getProperty("soundboardFolder", "").toString();
    config.bypassKeyCode = static_cast<int>(v.getProperty("bypassKeyCode", 0));
    config.bypassModifiers = static_cast<uint32_t>(
        static_cast<juce::int64>(v.getProperty("bypassModifiers", (juce::int64) 0)));

    // config.effectTimbre already holds the defaults from AppConfig's own
    // member initializer; only overwrite the entries actually present in the
    // JSON so an older save file (missing this field, or with fewer entries
    // than kNumVoiceEffects) still round-trips to today's defaults for the
    // effects it doesn't know about.
    if (const auto* timbreArray = v.getProperty("effectTimbre", juce::var()).getArray())
    {
        const auto count = juce::jmin(static_cast<size_t>(timbreArray->size()), config.effectTimbre.size());
        for (size_t i = 0; i < count; ++i)
            config.effectTimbre[i] = static_cast<float>(
                static_cast<double>((*timbreArray)[static_cast<int>(i)]));
    }

    if (const auto* slotArray = v.getProperty("slots", juce::var()).getArray())
    {
        for (const auto& item : *slotArray)
        {
            SlotConfig slot;
            slot.filePath = item.getProperty("filePath", "").toString();
            slot.keyCode = static_cast<int>(item.getProperty("keyCode", 0));
            slot.modifiers = static_cast<uint32_t>(
                static_cast<juce::int64>(item.getProperty("modifiers", (juce::int64) 0)));
            config.slots.push_back(slot);
        }
    }

    return config;
}

bool ConfigManager::save(const AppConfig& config, const juce::File& file)
{
    const auto parent = file.getParentDirectory();
    if (! parent.exists() && ! parent.createDirectory())
        return false;

    const auto json = juce::JSON::toString(toVar(config), true);
    return file.replaceWithText(json);
}

AppConfig ConfigManager::load(const juce::File& file, bool& ok)
{
    ok = false;
    if (! file.existsAsFile())
        return {};

    juce::var parsed;
    const auto result = juce::JSON::parse(file.loadFileAsString(), parsed);
    if (result.failed() || ! parsed.isObject())
        return {};

    ok = true;
    return fromVar(parsed);
}

AppConfig ConfigManager::load(const juce::File& file)
{
    bool ok = false;
    return load(file, ok);
}
} // namespace eq
