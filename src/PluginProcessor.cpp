#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace bmosat;
namespace P = bmosat::params;

//==============================================================================
BmoSaturatorAudioProcessor::BmoSaturatorAudioProcessor()
    : juce::AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", P::create())
{
    const auto bind = [this] (const char* id) { return apvts.getRawParameterValue (id); };

    inputGainParam    = bind (P::kInputGain);
    driveParam        = bind (P::kDrive);
    mixParam          = bind (P::kMix);
    outputLevelParam  = bind (P::kOutputLevel);
    satInParam        = bind (P::kSatIn);
    phaseParam        = bind (P::kPhase);
    autoGainParam     = bind (P::kAutoGain);
    oversamplingParam = bind (P::kOversampling);
    toneParam         = bind (P::kTone);

    jassert (driveParam != nullptr && mixParam != nullptr);

    apvts.addParameterListener (P::kOversampling, this);
}

BmoSaturatorAudioProcessor::~BmoSaturatorAudioProcessor()
{
    apvts.removeParameterListener (P::kOversampling, this);
    cancelPendingUpdate();
}

//==============================================================================
void BmoSaturatorAudioProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    // Fired from whichever thread moved the parameter, so this stays
    // allocation- and lock-free: an async trigger and nothing else.
    juce::ignoreUnused (parameterID, newValue);
    triggerAsyncUpdate();
}

void BmoSaturatorAudioProcessor::handleAsyncUpdate()
{
    // Changing the oversampling factor changes the latency of the anti-imaging
    // filters, which the host needs so its delay compensation stays right.
    // Read from the parameter rather than the DSP, which may not have picked
    // the change up yet.
    const auto factor  = oversamplingFactor ((int) oversamplingParam->load (std::memory_order_relaxed));
    const auto latency = Oversampler::latencyForFactor (factor);

    if (reportedLatency.exchange (latency, std::memory_order_relaxed) != latency)
        setLatencySamples (latency);
}

//==============================================================================
void BmoSaturatorAudioProcessor::prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock)
{
    const auto factor = oversamplingFactor ((int) oversamplingParam->load (std::memory_order_relaxed));

    dsp.prepare (sampleRate, maximumExpectedSamplesPerBlock, getTotalNumOutputChannels(), factor);

    for (auto* a : { &inputPeak, &outputPeak, &outputRms })
        for (auto& p : *a)
            p.store (0.0f, std::memory_order_relaxed);

    setLatencySamples (dsp.getLatencySamples());
}

void BmoSaturatorAudioProcessor::releaseResources()
{
    dsp.reset();
}

bool BmoSaturatorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    // Mono or stereo, but no converting between them.
    return layouts.getMainInputChannelSet() == out;
}

//==============================================================================
void BmoSaturatorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    // Denormals in filter tails cost roughly 100x CPU and are a classic source
    // of mystery dropouts. Must be first.
    juce::ScopedNoDenormals noDenormals;

    const auto numSamples = buffer.getNumSamples();
    const auto numIn      = getTotalNumInputChannels();
    const auto numOut     = getTotalNumOutputChannels();

    for (int ch = numIn; ch < numOut; ++ch)
        buffer.clear (ch, 0, numSamples);

    for (int ch = 0; ch < juce::jmin (2, numOut); ++ch)
        inputPeak[(size_t) ch].store (buffer.getMagnitude (ch, 0, numSamples),
                                      std::memory_order_relaxed);

    const auto load = [] (const std::atomic<float>* p) { return p->load (std::memory_order_relaxed); };

    DspCore::Params p;
    p.inputGainDb   = load (inputGainParam);
    p.driveAmount   = load (driveParam);
    p.toneAmount    = load (toneParam);
    p.mixPercent    = load (mixParam);
    p.outputLevelDb = load (outputLevelParam);
    p.saturationIn  = load (satInParam) > 0.5f;
    p.phaseInvert   = load (phaseParam) > 0.5f;
    p.autoGain      = load (autoGainParam) > 0.5f;
    p.oversampling  = oversamplingFactor ((int) load (oversamplingParam));

    dsp.setParams (p);
    dsp.process (buffer.getArrayOfWritePointers(), numOut, numSamples);

    for (int ch = 0; ch < juce::jmin (2, numOut); ++ch)
    {
        outputPeak[(size_t) ch].store (buffer.getMagnitude (ch, 0, numSamples),
                                       std::memory_order_relaxed);
        outputRms[(size_t) ch].store (buffer.getRMSLevel (ch, 0, numSamples),
                                      std::memory_order_relaxed);
    }
}

//==============================================================================
juce::AudioProcessorEditor* BmoSaturatorAudioProcessor::createEditor()
{
    return new BmoSaturatorAudioProcessorEditor (*this);
}

void BmoSaturatorAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("stateVersion", P::kStateVersion, nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void BmoSaturatorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);

    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return;

    // Future versions migrate here, keyed off the stored stateVersion property.
    apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BmoSaturatorAudioProcessor();
}
