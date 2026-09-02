#include "DspCore.h"
#include "DriveTables.h"
#include <algorithm>

namespace bmosat
{

namespace
{
    float dbToGain (float db) noexcept { return std::pow (10.0f, db * 0.05f); }
}

//==============================================================================
float DspCore::driveFor (float amountPercent) noexcept
{
    const auto t = std::clamp (amountPercent, 0.0f, 100.0f) * 0.01f;

    return tables::kDriveMin * std::pow (tables::kDriveMax / tables::kDriveMin, t);
}

float DspCore::makeupGainDb (float amountPercent) noexcept
{
    return tables::makeupDb (amountPercent);
}

//==============================================================================
void DspCore::Channel::prepare (double rate) noexcept
{
    for (auto& pole : bodyInput)
        pole.setCutoff (kBodySourceHz, rate);

    bodySplit.setCutoff (kBodySourceHz, rate);

    for (auto& pole : sheenInput)
        pole.setCutoff (kSheenSourceHz, rate);

    sheenSplit.setCutoff (kResidualSplitHz, rate);
    sheenTilt.set (kSheenHz, kSheenTilt, rate);

    dc.prepare (rate);
    reset();
}

void DspCore::Channel::reset() noexcept
{
    shaper.reset();
    bodyShaper.reset();
    sheenShaper.reset();

    for (auto& pole : bodyInput)
        pole.reset();

    for (auto& pole : sheenInput)
        pole.reset();

    bodySplit.reset();
    sheenSplit.reset();
    sheenTilt.reset();
    dc.reset();
    oversampler.reset();
}

void DspCore::Channel::setDrive (float drive) noexcept
{
    shaper.setDrive (drive);
    bodyShaper.setDrive (drive);
    sheenShaper.setDrive (drive);
}

float DspCore::Channel::process (float x) noexcept
{
    // The fitted curve, exactly: x + (shape(x) - x), with the residual
    // anti-aliased. Everything the calibration in Shaper.h says about the
    // positive and negative average gains is a statement about this line.
    auto y = x + shaper.processResidual (x);

    // One generator per band that needs filling. Each is fed the signal below
    // its corner, and only what it makes above that corner is kept.
    y += kBodyGain  * generate (bodyShaper,  bodyInput,  bodySplit,  x);
    y += kSheenGain * sheenTilt.process (generate (sheenShaper, sheenInput, sheenSplit, x));

    return dc.process (y);
}

float DspCore::Channel::generate (AsymmetricShaper& generator, OnePole (&input)[2],
                                  OnePole& split, float x) noexcept
{
    auto source = x;

    for (auto& pole : input)
        source = pole.process (OnePole::Output::lowpass, source);

    float below = 0.0f, above = 0.0f;
    split.process (generator.processResidual (source), below, above);
    return above;
}

//==============================================================================
void DspCore::prepare (double newSampleRate, int maxBlockSize, int numChannels,
                       int oversampleFactor)
{
    sampleRate  = newSampleRate > 0.0 ? newSampleRate : 44100.0;
    maxBlock    = std::max (maxBlockSize, 1);
    maxChannels = std::clamp (numChannels, 1, (int) channels.size());

    // Sized for the highest factor, so a change of oversampling at run time
    // never has to allocate on the audio thread.
    dryDelay.assign ((size_t) ((Oversampler::kMaxLatency + 2) * (int) channels.size()), 0.0f);
    dryStride = Oversampler::kMaxLatency + 2;

    const auto controlRate = sampleRate / (double) kSubBlock;

    for (auto* s : { &inputGainSm, &outputLevelSm, &mixSm, &makeupSm })
        s->prepare (controlRate, 20.0);

    // Drive is smoothed more slowly than a gain. It moves the shape of the
    // curve rather than a level, and a curve that changes shape quickly under
    // an automation ramp is audible as a flutter on sustained material.
    driveSm.prepare (controlRate, 40.0);

    applyOversampling (oversampleFactor);

    primed = false;
    reset();
}

void DspCore::applyOversampling (int factor)
{
    factor = factor >= 8 ? 8 : factor >= 4 ? 4 : factor >= 2 ? 2 : 1;

    for (auto& c : channels)
        c.oversampler.setFactor (factor);

    currentFactor  = factor;
    latencySamples = Oversampler::latencyForFactor (factor);
    effectiveRate  = sampleRate * (double) factor;
    dryLength      = latencySamples + 1;

    // None of this allocates; it only recomputes coefficients, so it is safe
    // to call from the audio thread when the factor changes.
    for (auto& c : channels)
        c.prepare (effectiveRate);
}

void DspCore::reset() noexcept
{
    for (auto& c : channels)
        c.reset();

    std::fill (dryDelay.begin(), dryDelay.end(), 0.0f);
    dryWrite = 0;
}

//==============================================================================
void DspCore::setParams (const Params& p) noexcept
{
    params = p;

    const auto drive  = driveFor (p.driveAmount);
    const auto makeup = p.autoGain ? dbToGain (makeupGainDb (p.driveAmount)) : 1.0f;

    inputGainSm  .setTarget (dbToGain (p.inputGainDb));
    outputLevelSm.setTarget (dbToGain (p.outputLevelDb));
    mixSm        .setTarget (std::clamp (p.mixPercent, 0.0f, 100.0f) * 0.01f);
    driveSm      .setTarget (drive);
    makeupSm     .setTarget (makeup);

    if (primed)
        return;

    inputGainSm  .snap (dbToGain (p.inputGainDb));
    outputLevelSm.snap (dbToGain (p.outputLevelDb));
    mixSm        .snap (std::clamp (p.mixPercent, 0.0f, 100.0f) * 0.01f);
    driveSm      .snap (drive);
    makeupSm     .snap (makeup);
    primed = true;
}

//==============================================================================
void DspCore::process (float* const* channelData, int numChannels, int numSamples) noexcept
{
    const auto activeChannels = std::clamp (numChannels, 0, maxChannels);

    if (activeChannels == 0 || numSamples <= 0)
        return;

    if (params.oversampling != currentFactor)
        applyOversampling (params.oversampling);

    const auto polarity = params.phaseInvert ? -1.0f : 1.0f;
    const auto factor   = currentFactor;

    for (int start = 0; start < numSamples; start += kSubBlock)
    {
        const auto n = std::min (kSubBlock, numSamples - start);

        const auto inGain   = inputGainSm.tick();
        const auto outGain  = outputLevelSm.tick() * makeupSm.tick();
        const auto wet      = mixSm.tick();
        const auto dryLevel = 1.0f - wet;
        const auto drive    = driveSm.tick();

        for (int ch = 0; ch < activeChannels; ++ch)
            channels[(size_t) ch].setDrive (drive);

        // Samples outermost so the shared dry-delay cursor advances once per
        // frame rather than once per channel.
        for (int i = 0; i < n; ++i)
        {
            const auto readIndex = (dryWrite + 1) % dryLength;

            for (int ch = 0; ch < activeChannels; ++ch)
            {
                auto& channel = channels[(size_t) ch];
                auto* data = channelData[ch] + start;
                auto* dry  = dryDelay.data() + (size_t) ch * (size_t) dryStride;

                const auto input = data[i];

                dry[(size_t) dryWrite] = input;
                const auto delayed = dry[(size_t) readIndex];

                float buffer[Oversampler::kMaxFactor] {};
                channel.oversampler.upsample (input * inGain * polarity, buffer);

                for (int j = 0; j < factor; ++j)
                    buffer[j] = params.saturationIn ? channel.process (buffer[j]) : buffer[j];

                const auto processed = channel.oversampler.downsample (buffer) * outGain;

                data[i] = processed * wet + delayed * dryLevel;
            }

            dryWrite = (dryWrite + 1) % dryLength;
        }
    }
}

} // namespace bmosat
