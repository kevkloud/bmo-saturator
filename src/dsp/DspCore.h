#pragma once

#include "Filters.h"
#include "Oversampler.h"
#include "Shaper.h"
#include <array>
#include <vector>

namespace bmosat
{

//==============================================================================
/** One-pole parameter smoother.

    Snaps to the target once it is within epsilon, so a settled parameter
    compares exactly equal and the coefficient recomputation can be skipped.
*/
class Smoother
{
public:
    void prepare (double controlRateHz, double timeMs) noexcept
    {
        const auto tau = std::max (timeMs, 0.01) * 0.001;
        coeff = (float) (1.0 - std::exp (-1.0 / (std::max (controlRateHz, 1.0) * tau)));
    }

    void snap (float v) noexcept        { current = target = v; }
    void setTarget (float t) noexcept   { target = t; }
    float value() const noexcept        { return current; }

    float tick() noexcept
    {
        current += coeff * (target - current);

        if (std::abs (target - current) < 1.0e-6f)
            current = target;

        return current;
    }

private:
    float coeff = 1.0f, current = 0.0f, target = 0.0f;
};

//==============================================================================
/** Everything the plugin does to audio, with no dependency on JUCE's plugin
    layer or on a host. Takes plain values and raw buffers, so the measurement
    harness and the unit tests can drive the real signal path directly.

    The chain, per channel, inside the oversampled region:

        input trim -> the fitted curve
                   -> plus two harmonic generators, each filling one band
                   -> DC blocker -> makeup -> output trim

    The generators are the unusual part, and they are the reason this hits the
    brief rather than merely saturating.

    A waveshaper applied the ordinary way -- signal in, shaped signal out --
    distributes its new harmonics wherever they fall. On a voice that is mostly
    the 600 Hz to 2.5 kHz octaves, since that is where the second and third
    harmonics of the fundamental land. The reference this plugin is fitted to
    does the opposite: it leaves everything below 2.5 kHz within about a dB of
    where it started and puts 6 to 8 dB of new energy above it. No single curve
    applied to the whole signal does that at any drive setting. Push it hard
    enough to fill the top and the midrange fills with it.

    So the fitted curve runs across the whole signal and carries the character
    -- the asymmetry, the even-order content, the compression the reference
    measured -- and two further instances of the same curve at the same drive
    place the new energy. Each is fed only the band below a corner and read
    only above it, so what it contributes is harmonic content that was not
    there before rather than a scaled copy of the programme. They are still
    harmonics of the programme, made by the same asymmetric curve and carrying
    its even-order signature; they are simply weighted towards the part of the
    spectrum the target puts them in.

    That structure also settles the dynamics. The dry path through the stage is
    never attenuated, so the programme's own peaks arrive intact and the added
    residual is largest exactly where the waveform moves fastest -- on
    transients. The crest factor comes out slightly up, which is what was asked
    for. There is no compressor, limiter, or peak reduction anywhere in here,
    and adding one would be a change of design rather than a feature.
*/
class DspCore
{
public:
    struct Params
    {
        float inputGainDb   = 0.0f;
        float driveAmount   = 40.0f;   // per cent, the panel's DRIVE
        float mixPercent    = 100.0f;
        float outputLevelDb = 0.0f;

        bool  saturationIn = true;
        bool  phaseInvert  = false;
        bool  autoGain     = false;

        int   oversampling = 2;        // 1, 2, 4 or 8
    };

    void prepare (double sampleRate, int maxBlockSize, int numChannels, int oversampleFactor = 2);
    void reset() noexcept;

    /** Round-trip delay of the oversampling filters, in samples at the host's
        rate. Reported to the host so plugin delay compensation can undo it. */
    int getLatencySamples() const noexcept { return latencySamples; }

    /** Called once per block, before process(). Cheap: stores targets only. */
    void setParams (const Params&) noexcept;

    void process (float* const* channels, int numChannels, int numSamples) noexcept;

    //== The character, in one place ==========================================

    /** DRIVE, 0-100 on the panel, mapped to the curve's drive.

        Geometric, because what the ear follows is the ratio between where the
        signal sits and where the knee is, not the difference. The bottom of
        the range is deliberately not zero: at DRIVE 0 the curve is still the
        curve, just far enough below the knee to be almost linear, so turning
        the control up changes the amount of a character rather than fading one
        in. */
    static float driveFor (float amountPercent) noexcept;

    /** Static output compensation for a given DRIVE, in decibels.

        Static, and derived from the curve rather than from a level detector
        following the programme. A detector would make this a compressor, which
        is the one thing this plugin must not become. */
    static float makeupGainDb (float amountPercent) noexcept;

    static constexpr int kSubBlock = 32;

    /** Where the target's flat bands stop and its lifted ones begin. Both the
        band the sheen generator is fed from and the band its output is taken
        in are bounded here. */
    static constexpr double kResidualSplitHz = 2400.0;

    /** The two harmonic generators, each a second instance of the same curve
        at the same drive, fed only from below a corner and read only above it.

        Both exist for the same reason, one octave apart. The `body` generator
        refills 600 Hz to 2.5 kHz, which a compressive curve suppresses: the
        midrange of a voice is far weaker than its fundamentals, so it rides
        through whatever instantaneous gain the fundamentals impose and comes
        out around 3 dB down, against the reference's 0.7. The `sheen`
        generator fills 2.5 kHz upwards, which is where the reference puts most
        of its new energy and where nothing else in the chain can put it.

        Fed from a band that has nothing in it above the corner, and read only
        above that corner, so what each contributes is harmonic content that
        was not there before -- not a scaled copy of the programme. That
        distinction is the whole reason this works: the residual of a
        compressive curve is part new harmonics and part a negative copy of its
        input, and amplifying the second part subtracts the programme's own top
        end. Generating from a band with no top end in it leaves nothing up
        there to subtract from.
    */
    static constexpr double kBodySourceHz = 600.0;

    /** Negative, and not a typo.

        The body generator's harmonics land where the programme already has
        harmonics of its own, and they arrive in antiphase with them: added in
        the obvious polarity they cancel, and the band measured 3 dB down
        rather than flat. Inverted, they reinforce. Polarity is load-bearing
        whenever a generator is fed a band the source already occupies, which
        is why the sheen generator -- reading a band where the source has
        almost nothing -- does not care about it and is positive. */
    static constexpr float kBodyGain = -3.0f;

    /** The top of the band the sheen generator is fed from. Wide enough that
        the 2.4-6 kHz octaves get to make harmonics of their own, which is the
        only way to put related energy in the top two. */
    static constexpr double kSheenSourceHz = 6000.0;

    /** How much of the sheen generator's harmonics are added. Fitted to the
        target's +6.25 dB in 2.5-6 kHz at the default drive. */
    static constexpr float kSheenGain = 4.75f;

    /** A shelf on the high residual only, tilting the top octaves up so that
        6-18 kHz lifts further than 2.5-6 kHz, as the reference does. It shapes
        distortion the plugin generated, never the programme, which is why it
        is not a tone control and is not on the panel. */
    static constexpr double kSheenHz   = 6000.0;
    static constexpr float  kSheenTilt = 1.25f;

private:
    void applyOversampling (int factor);

    struct Channel
    {
        AsymmetricShaper shaper, bodyShaper, sheenShaper;
        OnePole          bodyInput[2], bodySplit;
        OnePole          sheenInput[2], sheenSplit;
        Shelf            sheenTilt;
        DcBlocker        dc;
        Oversampler      oversampler;

        void prepare (double rate) noexcept;
        void reset() noexcept;
        void setDrive (float drive) noexcept;
        float process (float x) noexcept;

        /** One harmonic generator: shape the band below the corner, keep what
            appears above it. */
        static float generate (AsymmetricShaper&, OnePole (&input)[2], OnePole& split, float x) noexcept;
    };

    double sampleRate = 44100.0;
    double effectiveRate = 44100.0;
    int    latencySamples = 0;

    std::array<Channel, 2> channels;

    // The dry path of the Mix control has to be delayed to match, or a partial
    // blend combs and a full bypass fails to null.
    std::vector<float> dryDelay;
    int dryWrite = 0, dryLength = 1, dryStride = 0;

    Smoother inputGainSm, driveSm, mixSm, outputLevelSm, makeupSm;

    Params params;
    bool primed = false;
    int maxBlock = 0, maxChannels = 0;
    int currentFactor = 0;
};

} // namespace bmosat
