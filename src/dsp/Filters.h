#pragma once

#include <cmath>

namespace bmosat
{

//==============================================================================
/** One-pole topology-preserving (TPT) filter, after Zavalishin.

    Two properties matter here and neither holds for a direct-form one-pole.

    Its state stays meaningful while the coefficients move, so the band split
    can follow a smoothed control without the coefficient discontinuity that
    makes a direct form click.

    And its low-pass and high-pass outputs sum to the input exactly, sample for
    sample, at every frequency. That is what makes the two-band split in
    DspCore a split rather than a filter: with the saturation switched out the
    two bands recombine to the signal that went in, so a bypass nulls and a
    partial Mix cannot comb.
*/
struct OnePole
{
    enum class Output { lowpass, highpass };

    float g = 0.0f, bigG = 0.0f, state = 0.0f;

    void setCutoff (double frequencyHz, double sampleRate) noexcept
    {
        // Prewarp, so the digital corner lands on the analogue one.
        const auto gg = std::tan (kPi * frequencyHz / sampleRate);
        g    = (float) gg;
        bigG = (float) (gg / (1.0 + gg));
    }

    void reset() noexcept { state = 0.0f; }

    /** Both outputs at once. lowpass + highpass == input, exactly. */
    void process (float input, float& lowpass, float& highpass) noexcept
    {
        const auto v = (input - state) * bigG;
        lowpass  = state + v;
        highpass = input - lowpass;
        state    = lowpass + v;
    }

    float process (Output type, float input) noexcept
    {
        float lp = 0.0f, hp = 0.0f;
        process (input, lp, hp);
        return type == Output::lowpass ? lp : hp;
    }

private:
    static constexpr double kPi = 3.14159265358979323846;
};

//==============================================================================
/** First-order shelf built from a one-pole, used in exactly invertible pairs.

        H(s) = (1 + m*s/w) / (1 + s/w)

    Unity at DC, m at high frequency. The inverse is the same structure with
    corner w/m and gain 1/m, so a pair cancels to unity wherever nothing in
    between it is misbehaving.
*/
class Shelf
{
public:
    void set (double cornerHz, float highFrequencyGain, double sampleRate) noexcept
    {
        gain = highFrequencyGain;
        pole.setCutoff (std::fmin (std::fmax (cornerHz, 1.0), sampleRate * 0.45), sampleRate);
    }

    void reset() noexcept { pole.reset(); }

    float process (float x) noexcept
    {
        float lp = 0.0f, hp = 0.0f;
        pole.process (x, lp, hp);
        return lp + gain * hp;
    }

private:
    OnePole pole;
    float   gain = 1.0f;
};

//==============================================================================
/** DC blocker.

    An asymmetric shaper is not mean-preserving: feed it a symmetric signal and
    the output sits off zero, by an amount that moves with the drive and with
    the programme. Left in, that offset eats headroom, thumps whenever the
    drive is automated, and shows up in the crest factor as a peak that is not
    music. It is removed here rather than left for whatever is downstream.

    Deliberately far below the audio band: this is the one filter in the chain
    that is not part of the sound.
*/
class DcBlocker
{
public:
    void prepare (double sampleRate) noexcept
    {
        pole.setCutoff (kCornerHz, sampleRate);
        reset();
    }

    void reset() noexcept { pole.reset(); }

    float process (float x) noexcept { return pole.process (OnePole::Output::highpass, x); }

private:
    static constexpr double kCornerHz = 6.0;

    OnePole pole;
};

} // namespace bmosat
