#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "params/ParameterLayout.h"
#include "dsp/DspCore.h"
#include "presets/PresetManager.h"
#include <array>
#include <atomic>

//==============================================================================
/** The host adapter, and nothing more.

    Everything that touches audio lives in bmosat::DspCore, which has no
    dependency on JUCE's plugin layer -- that is what lets the whole signal
    path be measured and tested on a bare container. This class binds
    parameters to it, reports latency, and publishes metering.
*/
class BmoSaturatorAudioProcessor final : public juce::AudioProcessor,
                                         private juce::AudioProcessorValueTreeState::Listener,
                                         private juce::AsyncUpdater
{
public:
    BmoSaturatorAudioProcessor();
    ~BmoSaturatorAudioProcessor() override;

    //== AudioProcessor ========================================================
    void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                          { return true; }

    const juce::String getName() const override              { return JucePlugin_Name; }
    bool acceptsMidi() const override                        { return false; }
    bool producesMidi() const override                       { return false; }
    bool isMidiEffect() const override                       { return false; }
    double getTailLengthSeconds() const override             { return 0.0; }

    int getNumPrograms() override                            { return 1; }
    int getCurrentProgram() override                         { return 0; }
    void setCurrentProgram (int) override                    {}
    const juce::String getProgramName (int) override         { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    //== Ours ==================================================================
    juce::AudioProcessorValueTreeState& getApvts() noexcept  { return apvts; }
    bmosat::PresetManager& getPresets() noexcept             { return presets; }

    /** Metering, written by the audio thread and polled by the editor on a
        timer. Publish-and-sample; never push from audio to UI. */
    float getInputPeak  (int ch) const noexcept { return read (inputPeak,  ch); }
    float getOutputPeak (int ch) const noexcept { return read (outputPeak, ch); }

    /** RMS as well as peak, because a VU reading is not a peak reading on a
        different scale -- it integrates, which is what makes it read weight.
        On a saturator the two move in opposite directions, so having only one
        of them is actively misleading. */
    float getOutputRms (int ch) const noexcept { return read (outputRms, ch); }

private:
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void handleAsyncUpdate() override;

    /** Oversampling choice index to factor: Off, 2x, 4x, HQ. */
    static int oversamplingFactor (int index) noexcept
    {
        constexpr int factors[] { 1, 2, 4, 8 };
        return factors[juce::jlimit (0, 3, index)];
    }

    static float read (const std::array<std::atomic<float>, 2>& a, int ch) noexcept
    {
        return a[(size_t) juce::jlimit (0, 1, ch)].load (std::memory_order_relaxed);
    }

    juce::AudioProcessorValueTreeState apvts;

    // Declared after apvts: it registers listeners on it, so it must be torn
    // down first.
    bmosat::PresetManager presets { apvts };

    // Resolved once in the constructor. Looking parameters up by string ID on
    // the audio thread would be a hash lookup per block.
    std::atomic<float>* inputGainParam    = nullptr;
    std::atomic<float>* driveParam        = nullptr;
    std::atomic<float>* mixParam          = nullptr;
    std::atomic<float>* outputLevelParam  = nullptr;
    std::atomic<float>* satInParam        = nullptr;
    std::atomic<float>* phaseParam        = nullptr;
    std::atomic<float>* autoGainParam     = nullptr;
    std::atomic<float>* oversamplingParam = nullptr;
    std::atomic<float>* toneParam         = nullptr;

    bmosat::DspCore dsp;

    // Latency is only pushed to the host when it actually changes. Automating
    // the oversampling otherwise floods the host with setLatencySamples calls
    // on every move, which is enough to destabilise it.
    std::atomic<int> reportedLatency { -1 };

    std::array<std::atomic<float>, 2> inputPeak  { };
    std::array<std::atomic<float>, 2> outputPeak { };
    std::array<std::atomic<float>, 2> outputRms  { };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BmoSaturatorAudioProcessor)
};
