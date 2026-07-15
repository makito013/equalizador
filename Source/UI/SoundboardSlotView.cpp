#include "SoundboardSlotView.h"

namespace eq
{
SoundboardSlotView::SoundboardSlotView(int slotIndex) : index(slotIndex)
{
    triggerButton.setButtonText("Vazio");
    triggerButton.onClick = [this] { if (onTrigger) onTrigger(index); };
    addAndMakeVisible(triggerButton);

    pickFileButton.onClick = [this] { if (onPickFile) onPickFile(index); };
    addAndMakeVisible(pickFileButton);

    hotkeyButton.onHotkeyChosen = [this](int keyCode, uint32_t modifiers)
    {
        if (onHotkeyChosen)
            onHotkeyChosen(index, keyCode, modifiers);
    };
    addAndMakeVisible(hotkeyButton);
}

void SoundboardSlotView::setLabel(const juce::String& text)
{
    triggerButton.setButtonText(text);
}

void SoundboardSlotView::resized()
{
    auto area = getLocalBounds();
    hotkeyButton.setBounds(area.removeFromRight(72));
    area.removeFromRight(4);
    pickFileButton.setBounds(area.removeFromRight(28));
    area.removeFromRight(4);
    triggerButton.setBounds(area);
}
} // namespace eq
