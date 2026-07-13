#include "Mixer/SoftLimiter.h"

#include <cmath>

#include <juce_core/juce_core.h>

using namespace eq;

class SoftLimiterTests : public juce::UnitTest
{
public:
    SoftLimiterTests() : juce::UnitTest("SoftLimiter", "eq") {}

    void runTest() override
    {
        SoftLimiter limiter; // default threshold 0.9

        beginTest("signal below threshold is untouched");
        {
            expectEquals(limiter.processSample(0.5f), 0.5f);
            expectEquals(limiter.processSample(-0.5f), -0.5f);
            expectEquals(limiter.processSample(0.0f), 0.0f);
        }

        beginTest("output never exceeds unity for large input");
        {
            for (float x : { 1.0f, 2.0f, 10.0f, 100.0f })
            {
                expect(std::abs(limiter.processSample(x)) <= 1.0f);
                expect(std::abs(limiter.processSample(-x)) <= 1.0f);
            }
        }

        beginTest("limiter is symmetric");
        {
            for (float x : { 0.95f, 1.5f, 3.0f })
                expect(std::abs(limiter.processSample(x) + limiter.processSample(-x)) < 1.0e-5f);
        }

        beginTest("above-threshold input is compressed, not clipped");
        {
            const float y = limiter.processSample(0.95f);
            expect(y > 0.9f && y < 0.95f); // compressed toward, but above, threshold
        }

        beginTest("buffer processing bounds every sample");
        {
            juce::AudioBuffer<float> buffer(2, 4);
            const float input[] = { 0.1f, 1.5f, -3.0f, 0.8f };
            for (int ch = 0; ch < 2; ++ch)
                for (int n = 0; n < 4; ++n)
                    buffer.setSample(ch, n, input[n]);

            limiter.process(buffer);

            for (int ch = 0; ch < 2; ++ch)
                for (int n = 0; n < 4; ++n)
                    expect(std::abs(buffer.getSample(ch, n)) <= 1.0f);
        }
    }
};

static SoftLimiterTests softLimiterTests;
