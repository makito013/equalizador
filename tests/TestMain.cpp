#include <juce_core/juce_core.h>

// Console runner for the JUCE UnitTest suites.
//
// Test framework choice: JUCE's built-in UnitTest. Rationale: the code under
// test already depends on JUCE types (juce::AudioBuffer, juce::var, juce::File),
// so UnitTest needs zero extra dependency and integrates with those types
// directly. The runner is plain and CTest invokes this executable.
int main()
{
    juce::UnitTestRunner runner;
    runner.setAssertOnFailure(false);
    runner.runAllTests();

    int totalPasses = 0;
    int totalFailures = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        if (const auto* result = runner.getResult(i))
        {
            totalPasses += result->passes;
            totalFailures += result->failures;
        }
    }

    std::cout << "\n==== Summary: " << totalPasses << " passed, "
              << totalFailures << " failed ====" << std::endl;

    return totalFailures > 0 ? 1 : 0;
}
