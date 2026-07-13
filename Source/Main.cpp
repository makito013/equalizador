#include "MainComponent.h"

#include <juce_gui_extra/juce_gui_extra.h>

namespace eq
{
/**
    Fixed 380x520, non-resizable, dark window (per the Designer spec). No overlay.
*/
class MainWindow : public juce::DocumentWindow
{
public:
    MainWindow()
        : juce::DocumentWindow("Equalizador",
                               juce::Colour(0xff1e1e1e),
                               juce::DocumentWindow::closeButton | juce::DocumentWindow::minimiseButton)
    {
        setUsingNativeTitleBar(true);
        setResizable(false, false);
        setContentOwned(new MainComponent(), true);
        centreWithSize(getWidth(), getHeight());
        setVisible(true);
    }

    void closeButtonPressed() override
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};

class EqualizadorApplication : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "Equalizador"; }
    const juce::String getApplicationVersion() override { return "0.1.0"; }
    bool moreThanOneInstanceAllowed() override { return false; }

    void initialise(const juce::String&) override { mainWindow = std::make_unique<MainWindow>(); }
    void shutdown() override { mainWindow = nullptr; }

    void systemRequestedQuit() override { quit(); }

private:
    std::unique_ptr<MainWindow> mainWindow;
};
} // namespace eq

START_JUCE_APPLICATION(eq::EqualizadorApplication)
