#pragma once

#include "State/AppState.h"

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

namespace eq
{
/**
    Six mutually-exclusive buttons (Normal / Robo / Monstro / ET / Esquilo /
    Demonio) rendered as a two-row segmented control (3x2, no scrolling - see
    CONTEXTO.md's soundboard-grid convention, applied here too). Reports the
    selected effect via onEffectChosen.
*/
class EffectSegmentedControl : public juce::Component
{
public:
    EffectSegmentedControl();

    void setSelected(VoiceEffectType effect);
    void resized() override;

    std::function<void(VoiceEffectType)> onEffectChosen;

private:
    juce::TextButton normalButton { "Normal" };
    juce::TextButton robotButton { "Robo" };
    juce::TextButton monsterButton { "Monstro" };
    juce::TextButton alienButton { "ET" };
    juce::TextButton chipmunkButton { "Esquilo" };
    juce::TextButton demonButton { "Demonio" };
};
} // namespace eq
