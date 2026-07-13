#pragma once

#include "State/AppState.h"

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

namespace eq
{
/**
    Three mutually-exclusive buttons (Normal / Robô / Monstro) rendered as a
    segmented control. Reports the selected effect via onEffectChosen.
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
};
} // namespace eq
