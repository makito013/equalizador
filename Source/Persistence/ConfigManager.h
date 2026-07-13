#pragma once

#include "ConfigSchema.h"

#include <juce_core/juce_core.h>

namespace eq
{
/**
    Loads/saves AppConfig as JSON.

    JSON is produced with juce::var / juce::JSON rather than a third-party
    library (e.g. nlohmann/json): JUCE is already a hard dependency, so this
    avoids adding another one, and juce::var round-trips cleanly to a file.

    Errors are never thrown or swallowed silently: load() returns a default
    config (and reports failure via the bool overload) when the file is missing
    or malformed.
*/
class ConfigManager
{
public:
    /** Default location under the user's app-data directory. */
    static juce::File getDefaultConfigFile();

    static bool save(const AppConfig& config, const juce::File& file);

    /** Returns the parsed config, or a default if the file is absent/invalid. */
    static AppConfig load(const juce::File& file);

    /** As load(), but reports via ok whether a valid file was actually read. */
    static AppConfig load(const juce::File& file, bool& ok);

    // Exposed for testing the pure serialization step.
    static juce::var toVar(const AppConfig& config);
    static AppConfig fromVar(const juce::var& v);
};
} // namespace eq
