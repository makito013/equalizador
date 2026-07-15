#include "DSP/EffectChain.h"
#include "DSP/RobotEffect.h"
#include "State/AppState.h"

#include <cmath>

#include <juce_core/juce_core.h>

using namespace eq;

namespace
{
constexpr double kSampleRate = 48000.0;
constexpr int kBlockSize = 256;

juce::AudioBuffer<float> makeSineBlock(int channels, int frames, double freqHz, double& phase)
{
    juce::AudioBuffer<float> buf(channels, frames);
    const double inc = 2.0 * juce::MathConstants<double>::pi * freqHz / kSampleRate;
    for (int n = 0; n < frames; ++n)
    {
        const float sample = static_cast<float>(0.5 * std::sin(phase));
        for (int ch = 0; ch < channels; ++ch)
            buf.setSample(ch, n, sample);

        phase += inc;
        if (phase >= 2.0 * juce::MathConstants<double>::pi)
            phase -= 2.0 * juce::MathConstants<double>::pi;
    }
    return buf;
}

bool allFinite(const juce::AudioBuffer<float>& b)
{
    for (int ch = 0; ch < b.getNumChannels(); ++ch)
        for (int n = 0; n < b.getNumSamples(); ++n)
            if (! std::isfinite(b.getSample(ch, n)))
                return false;
    return true;
}

double rms(const juce::AudioBuffer<float>& b)
{
    double sumSq = 0.0;
    int count = 0;
    for (int ch = 0; ch < b.getNumChannels(); ++ch)
        for (int n = 0; n < b.getNumSamples(); ++n)
        {
            const double s = static_cast<double>(b.getSample(ch, n));
            sumSq += s * s;
            ++count;
        }
    return count > 0 ? std::sqrt(sumSq / count) : 0.0;
}
}

/**
    Covers RF03 (Alien/Chipmunk/Demon wiring), RF01/RF02/RNF01 (per-effect
    timbre reaching the DSP via EffectChain::process()), and the pre-existing
    Normal/bypass passthrough contract. Exact sample-for-sample correctness of
    SoundTouch's pitch-shift output isn't asserted (that's SoundTouch's own
    concern) - these tests instead pin down the *plumbing*: enum -> effect
    dispatch, bypass short-circuiting, timbre clamping, and "the chain
    actually produces finite, audible output" for every pitched voice.
*/
class EffectChainTests : public juce::UnitTest
{
public:
    EffectChainTests() : juce::UnitTest("EffectChain", "eq") {}

    void runTest() override
    {
        beginTest("Normal effect is a passthrough");
        {
            AppState state;
            EffectChain chain(state);
            chain.prepare(kSampleRate, kBlockSize, 1);

            double phase = 0.0;
            auto block = makeSineBlock(1, kBlockSize, 220.0, phase);
            const auto reference = block;

            chain.process(block);

            for (int n = 0; n < kBlockSize; ++n)
                expectWithinAbsoluteError(block.getSample(0, n), reference.getSample(0, n), 1.0e-6f);
        }

        beginTest("Bypass forces passthrough regardless of the selected effect");
        {
            AppState state;
            state.bypass.store(true);
            state.currentEffect.store(VoiceEffectType::Robot);
            EffectChain chain(state);
            chain.prepare(kSampleRate, kBlockSize, 1);

            double phase = 0.0;
            auto block = makeSineBlock(1, kBlockSize, 220.0, phase);
            const auto reference = block;

            chain.process(block);

            for (int n = 0; n < kBlockSize; ++n)
                expectWithinAbsoluteError(block.getSample(0, n), reference.getSample(0, n), 1.0e-6f);
        }

        beginTest("Robot ring-modulates a constant signal by sin(phase), default 55Hz carrier");
        {
            AppState state;
            state.currentEffect.store(VoiceEffectType::Robot);
            EffectChain chain(state);
            chain.prepare(kSampleRate, 8, 1);

            juce::AudioBuffer<float> block(1, 8);
            for (int n = 0; n < 8; ++n)
                block.setSample(0, n, 1.0f);

            chain.process(block);

            const double inc = 2.0 * juce::MathConstants<double>::pi * 55.0 / kSampleRate;
            for (int n = 0; n < 8; ++n)
                expectWithinAbsoluteError(block.getSample(0, n),
                                          static_cast<float>(std::sin(inc * n)), 1.0e-4f);
        }

        beginTest("Robot::setTimbre changes the carrier and clamps to [30,150]");
        {
            RobotEffect robot;
            robot.prepare(kSampleRate, 8, 1);
            robot.setTimbre(1000.0f); // out of range -> clamps to 150

            juce::AudioBuffer<float> block(1, 8);
            for (int n = 0; n < 8; ++n)
                block.setSample(0, n, 1.0f);
            robot.process(block);

            const double inc = 2.0 * juce::MathConstants<double>::pi * 150.0 / kSampleRate;
            for (int n = 0; n < 8; ++n)
                expectWithinAbsoluteError(block.getSample(0, n),
                                          static_cast<float>(std::sin(inc * n)), 1.0e-4f);
        }

        beginTest("EffectChain::process() drives every pitch-shifted voice to finite, audible output");
        {
            const VoiceEffectType effects[] = { VoiceEffectType::Monster, VoiceEffectType::Alien,
                                                VoiceEffectType::Chipmunk, VoiceEffectType::Demon };

            for (const auto effect : effects)
            {
                AppState state;
                state.currentEffect.store(effect);
                EffectChain chain(state);
                chain.prepare(kSampleRate, kBlockSize, 1);

                double phase = 0.0;
                double accumulatedEnergy = 0.0;
                for (int i = 0; i < 40; ++i)
                {
                    auto block = makeSineBlock(1, kBlockSize, 220.0, phase);
                    chain.process(block);
                    expect(allFinite(block), "non-finite sample produced");
                    accumulatedEnergy += rms(block);
                }

                expect(accumulatedEnergy > 0.01, "effect produced no audible output after warm-up");
            }
        }

        beginTest("Out-of-range timbre deltas clamp instead of crashing, for every pitched voice");
        {
            AppState state;
            EffectChain chain(state);
            chain.prepare(kSampleRate, kBlockSize, 1);

            const VoiceEffectType effects[] = { VoiceEffectType::Monster, VoiceEffectType::Alien,
                                                VoiceEffectType::Chipmunk, VoiceEffectType::Demon };
            for (const auto effect : effects)
            {
                state.currentEffect.store(effect);
                state.effectTimbre[static_cast<size_t>(effect)].store(999.0f);

                double phase = 0.0;
                auto block = makeSineBlock(1, kBlockSize, 220.0, phase);
                chain.process(block);
                expect(allFinite(block), "non-finite sample after out-of-range timbre");
            }
        }
    }
};

static EffectChainTests effectChainTests;
