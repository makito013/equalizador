#include "State/AppState.h"

#include <thread>
#include <vector>

#include <juce_core/juce_core.h>

using namespace eq;

class AppStateTests : public juce::UnitTest
{
public:
    AppStateTests() : juce::UnitTest("AppState", "eq") {}

    void runTest() override
    {
        beginTest("toggleBypass flips the flag");
        {
            AppState state;
            expect(! state.bypass.load());
            state.toggleBypass();
            expect(state.bypass.load());
            state.toggleBypass();
            expect(! state.bypass.load());
        }

        beginTest("effect and gains are settable");
        {
            AppState state;
            state.currentEffect.store(VoiceEffectType::Monster);
            expect(state.currentEffect.load() == VoiceEffectType::Monster);
            state.voiceGain.store(0.5f);
            expectEquals(state.voiceGain.load(), 0.5f);
        }

        beginTest("concurrent toggles never lose an update");
        {
            AppState state;
            constexpr int perThread = 100000;
            constexpr int numThreads = 4;

            std::vector<std::thread> threads;
            for (int t = 0; t < numThreads; ++t)
                threads.emplace_back([&]
                {
                    for (int i = 0; i < perThread; ++i)
                        state.toggleBypass();
                });
            for (auto& th : threads)
                th.join();

            // An even total number of toggles must return to the initial state;
            // a racy (non-atomic) toggle would lose updates and land on true.
            expect(! state.bypass.load(),
                   "lost toggle update under contention");
        }
    }
};

static AppStateTests appStateTests;
