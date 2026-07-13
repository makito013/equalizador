#include "BypassIndicator.h"

namespace eq
{
void BypassIndicator::setState(State newState)
{
    if (state == newState)
        return;
    state = newState;
    repaint();
}

void BypassIndicator::paint(juce::Graphics& g)
{
    juce::Colour colour;
    juce::String label;

    switch (state)
    {
        case State::Active:   colour = juce::Colour(0xff2ecc71); label = "ATIVO";     break;
        case State::NoSignal: colour = juce::Colour(0xffe74c3c); label = "SEM SINAL"; break;
        case State::Bypassed:
        default:              colour = juce::Colour(0xff555555); label = "BYPASS";    break;
    }

    auto bounds = getLocalBounds().toFloat().reduced(6.0f);
    const float diameter = juce::jmin(bounds.getWidth(), bounds.getHeight());
    auto circle = juce::Rectangle<float>(diameter, diameter).withCentre(bounds.getCentre());

    g.setColour(colour.withAlpha(0.25f));
    g.fillEllipse(circle);
    g.setColour(colour);
    g.drawEllipse(circle.reduced(4.0f), 4.0f);
    g.fillEllipse(circle.reduced(circle.getWidth() * 0.28f));

    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));
    g.drawText(label, getLocalBounds().removeFromBottom(24), juce::Justification::centred);
}
} // namespace eq
