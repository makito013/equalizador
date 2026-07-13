#include "EffectSegmentedControl.h"

namespace eq
{
EffectSegmentedControl::EffectSegmentedControl()
{
    struct Entry { juce::TextButton* button; VoiceEffectType effect; };
    const Entry entries[] = {
        { &normalButton,  VoiceEffectType::Normal },
        { &robotButton,   VoiceEffectType::Robot },
        { &monsterButton, VoiceEffectType::Monster }
    };

    for (const auto& e : entries)
    {
        e.button->setRadioGroupId(1001);
        e.button->setClickingTogglesState(true);
        e.button->setConnectedEdges(juce::Button::ConnectedOnLeft | juce::Button::ConnectedOnRight);
        const auto effect = e.effect;
        e.button->onClick = [this, effect]
        {
            if (onEffectChosen)
                onEffectChosen(effect);
        };
        addAndMakeVisible(*e.button);
    }

    normalButton.setToggleState(true, juce::dontSendNotification);
}

void EffectSegmentedControl::setSelected(VoiceEffectType effect)
{
    switch (effect)
    {
        case VoiceEffectType::Robot:   robotButton.setToggleState(true, juce::dontSendNotification); break;
        case VoiceEffectType::Monster: monsterButton.setToggleState(true, juce::dontSendNotification); break;
        case VoiceEffectType::Normal:
        default:                       normalButton.setToggleState(true, juce::dontSendNotification); break;
    }
}

void EffectSegmentedControl::resized()
{
    auto area = getLocalBounds();
    const int w = area.getWidth() / 3;
    normalButton.setBounds(area.removeFromLeft(w));
    robotButton.setBounds(area.removeFromLeft(w));
    monsterButton.setBounds(area);
}
} // namespace eq
