#include "EffectSegmentedControl.h"

namespace eq
{
EffectSegmentedControl::EffectSegmentedControl()
{
    struct Entry { juce::TextButton* button; VoiceEffectType effect; };
    const Entry entries[] = {
        { &normalButton,   VoiceEffectType::Normal },
        { &robotButton,    VoiceEffectType::Robot },
        { &monsterButton,  VoiceEffectType::Monster },
        { &alienButton,    VoiceEffectType::Alien },
        { &chipmunkButton, VoiceEffectType::Chipmunk },
        { &demonButton,    VoiceEffectType::Demon }
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
        case VoiceEffectType::Robot:    robotButton.setToggleState(true, juce::dontSendNotification); break;
        case VoiceEffectType::Monster:  monsterButton.setToggleState(true, juce::dontSendNotification); break;
        case VoiceEffectType::Alien:    alienButton.setToggleState(true, juce::dontSendNotification); break;
        case VoiceEffectType::Chipmunk: chipmunkButton.setToggleState(true, juce::dontSendNotification); break;
        case VoiceEffectType::Demon:    demonButton.setToggleState(true, juce::dontSendNotification); break;
        case VoiceEffectType::Normal:
        default:                        normalButton.setToggleState(true, juce::dontSendNotification); break;
    }
}

void EffectSegmentedControl::resized()
{
    auto area = getLocalBounds();
    const int rowHeight = area.getHeight() / 2;

    auto topRow = area.removeFromTop(rowHeight);
    const int topW = topRow.getWidth() / 3;
    normalButton.setBounds(topRow.removeFromLeft(topW));
    robotButton.setBounds(topRow.removeFromLeft(topW));
    monsterButton.setBounds(topRow);

    auto bottomRow = area;
    const int bottomW = bottomRow.getWidth() / 3;
    alienButton.setBounds(bottomRow.removeFromLeft(bottomW));
    chipmunkButton.setBounds(bottomRow.removeFromLeft(bottomW));
    demonButton.setBounds(bottomRow);
}
} // namespace eq
