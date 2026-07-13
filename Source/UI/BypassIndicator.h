#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace eq
{
/**
    Large status light. Green = effect active, grey = bypassed, red = active but
    no mic signal detected. Purely presentational; the owner drives setState().
*/
class BypassIndicator : public juce::Component
{
public:
    enum class State
    {
        Active,
        Bypassed,
        NoSignal
    };

    void setState(State newState);
    void paint(juce::Graphics& g) override;

private:
    State state { State::Bypassed };
};
} // namespace eq
