/*
    Offline measurement harness for BMO Saturator.

    It drives the real DSP core -- the same code the plugin runs, with no host,
    no JUCE and no audio device -- and reports the four numbers the plugin was
    specified with:

        1. waveshaping asymmetry: average gain over the positive excursions of
           the waveform against the average gain over the negative ones
        2. band-energy deltas across five bands, level-matched
        3. crest factor, dry against saturated
        4. aliasing, as the worst inharmonic product a pure tone produces

    The reference targets in kFuji come from before/after analysis of a real
    vocal take supplied with the design brief, and the anti-targets in kPreesh
    come from a second pass that was judged harsh by ear. The point of keeping
    both here is that the failure mode has a shape: a near-symmetric curve with
    an isolated high-band lift measures "brighter" without measuring "warmer",
    and it is only the asymmetry that separates the two.

        measure verify [file.wav]   the full report, against both references
        measure sweep               every metric across the Drive range
        measure makeup              regenerate the table in dsp/DriveTables.h
        measure curve               the static shaping curve, as numbers
        measure alias               folded images at each oversampling factor

    With a .wav path, verify uses that file instead of the synthetic reference
    voice. Numbers from a real take are the ones that count; the synthetic one
    exists so CI has something deterministic to hold the plugin to.
*/

#include "dsp/DspCore.h"
#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

using namespace bmosat;

namespace
{

constexpr double kPi = 3.14159265358979323846;
constexpr double kSampleRate = 48000.0;

//==============================================================================
// The references.
//==============================================================================

struct BandTarget { const char* name; double lowHz, highHz, deltaDb; };

/** "Fuji" -- what this plugin is built toward. */
constexpr double kFujiPositiveGain = 0.62;
constexpr double kFujiNegativeGain = 0.84;
constexpr double kFujiAsymmetry    = 0.22;
constexpr double kFujiCrestChange  = 1.7;    // 20.5 dB dry -> 22.2 dB saturated

constexpr BandTarget kFuji[]
{
    { "20 Hz - 150 Hz",      20.0,   150.0,   -1.1 },
    { "150 Hz - 600 Hz",    150.0,   600.0,   -1.1 },
    { "600 Hz - 2.5 kHz",   600.0,  2500.0,   -0.7 },
    { "2.5 kHz - 6 kHz",   2500.0,  6000.0,    6.25 },
    { "6 kHz - 18 kHz",    6000.0, 18000.0,    8.25 },
};

/** "Preesh BG" -- the failure mode, kept as a negative constraint rather than
    merely ignored. Near-symmetric, with the lift confined to the top band. */
constexpr double kPreeshAsymmetry   = 0.004;
constexpr double kPreeshHighBandDb  = 1.85;

//==============================================================================
// Signals.
//==============================================================================

/** A voice-like reference signal.

    Not a substitute for a vocal take -- see the .wav path on `verify` -- but it
    has the properties the measurement depends on and a sine does not: energy
    concentrated below 1 kHz with a harmonic series running well above it,
    formants, breath noise in the top octaves, and a syllabic envelope with
    silence between phrases so the crest factor lands where a real take's does.

    Deterministic, so CI compares like with like.
*/
std::vector<float> referenceVoice (double seconds = 8.0, double rmsDbfs = -18.0)
{
    const auto n = (size_t) (seconds * kSampleRate);
    std::vector<float> out (n, 0.0f);

    // Three formants, roughly an open vowel.
    struct Formant { double hz, bandwidth, gain; };
    constexpr Formant formants[] { { 600.0, 90.0, 1.0 }, { 1400.0, 130.0, 0.5 }, { 2600.0, 190.0, 0.22 } };

    uint32_t rng = 0x1234567u;
    const auto noise = [&rng]
    {
        rng = rng * 1664525u + 1013904223u;
        return (double) (int32_t) rng / 2147483648.0;
    };

    double phase[48] {};
    double breath = 0.0;

    for (size_t i = 0; i < n; ++i)
    {
        const auto t = (double) i / kSampleRate;

        // Syllables: a fast attack, a decay, and silence between phrases. The
        // silence is what puts the crest factor where a real take has it.
        const auto beat = std::fmod (t, 0.62);
        const auto phrase = std::fmod (t, 3.4) < 1.7 ? 1.0 : 0.0;
        const auto envelope = phrase * (beat < 0.010 ? beat / 0.010
                                                     : std::exp (-(beat - 0.010) * 7.0));

        // Pitch: a slow phrase contour with vibrato on it.
        const auto f0 = 132.0 * std::pow (2.0, 0.35 * std::sin (2.0 * kPi * 0.31 * t))
                              * (1.0 + 0.012 * std::sin (2.0 * kPi * 5.4 * t));

        double sum = 0.0;

        for (int h = 1; h <= 48; ++h)
        {
            const auto hz = f0 * (double) h;

            if (hz > 18000.0)
                break;

            phase[h - 1] = std::fmod (phase[h - 1] + 2.0 * kPi * hz / kSampleRate, 2.0 * kPi);

            // A glottal source falls at about 12 dB per octave, but a sung or
            // projected voice is not a glottal source: the higher harmonics
            // are what carry through a mix, and a take that has been through a
            // preamp and a compressor has more of them still. At 1/h^1.4 the
            // reference's band profile lands where a real vocal take's does,
            // which is what the fit depends on -- with a steeper source the
            // 2.5-6 kHz band is nearly empty and every delta measured in it is
            // an artefact of the test signal rather than of the plugin.
            auto amplitude = std::pow ((double) h, -1.4);

            double shaped = 0.0;

            for (const auto& f : formants)
            {
                const auto d = (hz - f.hz) / f.bandwidth;
                shaped += f.gain / (1.0 + d * d);
            }

            sum += amplitude * (0.35 + shaped) * std::sin (phase[h - 1]);
        }

        // Breath and sibilance: high-passed noise, which is what fills the top
        // two octaves on a voice and what the saturation has to work with up
        // there. Kept quiet -- an over-airy reference makes every high-band
        // delta read small, and the plugin would then be fitted to compensate
        // for the test signal.
        const auto white = noise();
        breath += 0.35 * (white - breath);
        sum += 0.006 * (white - breath);

        out[i] = (float) (envelope * sum);
    }

    // Normalise to the requested RMS.
    double sumSquares = 0.0;

    for (auto v : out)
        sumSquares += (double) v * (double) v;

    const auto rms = std::sqrt (sumSquares / (double) n);
    const auto gain = rms > 0.0 ? std::pow (10.0, rmsDbfs / 20.0) / rms : 1.0;

    for (auto& v : out)
        v = (float) (v * gain);

    return out;
}

std::vector<float> sine (double hz, double seconds, double amplitude)
{
    const auto n = (size_t) (seconds * kSampleRate);
    std::vector<float> out (n);

    for (size_t i = 0; i < n; ++i)
        out[i] = (float) (amplitude * std::sin (2.0 * kPi * hz * (double) i / kSampleRate));

    return out;
}

//==============================================================================
/** Minimal WAV reader: PCM 16/24/32 and IEEE float 32, any channel count,
    summed to mono. Enough to put a real take through the plugin, and no more.
*/
bool readWav (const std::string& path, std::vector<float>& out, double& rate)
{
    std::ifstream file (path, std::ios::binary);

    if (! file)
        return false;

    const std::vector<char> bytes { std::istreambuf_iterator<char> (file),
                                    std::istreambuf_iterator<char>() };

    if (bytes.size() < 44 || std::memcmp (bytes.data(), "RIFF", 4) != 0
                          || std::memcmp (bytes.data() + 8, "WAVE", 4) != 0)
        return false;

    const auto u16 = [&bytes] (size_t at) { return (uint32_t) (uint8_t) bytes[at]
                                                 | ((uint32_t) (uint8_t) bytes[at + 1] << 8); };
    const auto u32 = [&u16] (size_t at)   { return u16 (at) | (u16 (at + 2) << 16); };

    uint32_t format = 1, channels = 1, bits = 16;
    size_t at = 12;

    while (at + 8 <= bytes.size())
    {
        const std::string id (bytes.data() + at, 4);
        const auto size = (size_t) u32 (at + 4);
        const auto body = at + 8;

        if (id == "fmt " && body + 16 <= bytes.size())
        {
            format   = u16 (body);
            channels = std::max (1u, u16 (body + 2));
            rate     = (double) u32 (body + 4);
            bits     = u16 (body + 14);
        }
        else if (id == "data")
        {
            const auto bytesPerSample = bits / 8;

            if (bytesPerSample == 0)
                return false;

            const auto frames = std::min (size, bytes.size() - body) / (bytesPerSample * channels);
            out.assign (frames, 0.0f);

            for (size_t f = 0; f < frames; ++f)
            {
                double sum = 0.0;

                for (uint32_t c = 0; c < channels; ++c)
                {
                    const auto p = body + (f * channels + c) * bytesPerSample;
                    double v = 0.0;

                    if (format == 3 && bits == 32)
                    {
                        float bits32 = 0.0f;
                        std::memcpy (&bits32, bytes.data() + p, 4);
                        v = bits32;
                    }
                    else if (bits == 16)
                    {
                        v = (double) (int16_t) (uint16_t) u16 (p) / 32768.0;
                    }
                    else if (bits == 24)
                    {
                        auto raw = (int32_t) (u16 (p) | ((uint32_t) (uint8_t) bytes[p + 2] << 16));
                        if (raw & 0x800000) raw -= 0x1000000;
                        v = (double) raw / 8388608.0;
                    }
                    else if (bits == 32)
                    {
                        v = (double) (int32_t) u32 (p) / 2147483648.0;
                    }
                    else
                    {
                        return false;
                    }

                    sum += v;
                }

                out[f] = (float) (sum / (double) channels);
            }

            return ! out.empty();
        }

        at = body + size + (size & 1);
    }

    return false;
}

//==============================================================================
// Metrics.
//==============================================================================

double rms (const std::vector<float>& x)
{
    double sum = 0.0;

    for (auto v : x)
        sum += (double) v * (double) v;

    return x.empty() ? 0.0 : std::sqrt (sum / (double) x.size());
}

double peak (const std::vector<float>& x)
{
    double m = 0.0;

    for (auto v : x)
        m = std::max (m, (double) std::abs (v));

    return m;
}

double crestFactorDb (const std::vector<float>& x)
{
    const auto r = rms (x);
    return r > 0.0 ? 20.0 * std::log10 (peak (x) / r) : 0.0;
}

/** Average gain over the excursions of one polarity.

    Defined on the polarity of the *input*, so the two halves are the same
    stretches of programme in both signals: the sum of |output| over the
    samples where the input was positive, against the sum of |input| over the
    same samples. That is the measurement the reference numbers come from.
*/
void excursionGains (const std::vector<float>& dry, const std::vector<float>& wet,
                     double& positive, double& negative)
{
    double posIn = 0.0, posOut = 0.0, negIn = 0.0, negOut = 0.0;
    const auto n = std::min (dry.size(), wet.size());

    for (size_t i = 0; i < n; ++i)
    {
        // Silence between phrases carries no information about the curve and,
        // being where the ratio is least well conditioned, would dominate an
        // unweighted average.
        if (std::abs (dry[i]) < 1.0e-4f)
            continue;

        if (dry[i] > 0.0f) { posIn += dry[i];  posOut += std::abs (wet[i]); }
        else               { negIn += -dry[i]; negOut += std::abs (wet[i]); }
    }

    positive = posIn > 0.0 ? posOut / posIn : 0.0;
    negative = negIn > 0.0 ? negOut / negIn : 0.0;
}

//==============================================================================
void fft (std::vector<std::complex<double>>& a)
{
    const auto n = a.size();

    for (size_t i = 1, j = 0; i < n; ++i)
    {
        size_t bit = n >> 1;

        for (; j & bit; bit >>= 1)
            j ^= bit;

        j ^= bit;

        if (i < j)
            std::swap (a[i], a[j]);
    }

    for (size_t len = 2; len <= n; len <<= 1)
    {
        const auto theta = -2.0 * kPi / (double) len;

        for (size_t i = 0; i < n; i += len)
            for (size_t k = 0; k < len / 2; ++k)
            {
                const std::complex<double> w { std::cos (theta * (double) k), std::sin (theta * (double) k) };
                const auto u = a[i + k];
                const auto v = a[i + k + len / 2] * w;
                a[i + k] = u + v;
                a[i + k + len / 2] = u - v;
            }
    }
}

/** Welch-averaged power spectrum: Hann windows, half overlap. */
std::vector<double> spectrum (const std::vector<float>& x, size_t size = 16384)
{
    std::vector<double> power (size / 2 + 1, 0.0);

    if (x.size() < size)
        return power;

    const auto hop = size / 2;
    int frames = 0;

    for (size_t start = 0; start + size <= x.size(); start += hop, ++frames)
    {
        std::vector<std::complex<double>> frame (size);

        for (size_t i = 0; i < size; ++i)
        {
            const auto w = 0.5 - 0.5 * std::cos (2.0 * kPi * (double) i / (double) size);
            frame[i] = { (double) x[start + i] * w, 0.0 };
        }

        fft (frame);

        for (size_t k = 0; k < power.size(); ++k)
            power[k] += std::norm (frame[k]);
    }

    if (frames > 0)
        for (auto& v : power)
            v /= (double) frames;

    return power;
}

double bandEnergy (const std::vector<double>& power, double lowHz, double highHz, size_t size = 16384)
{
    const auto binHz = kSampleRate / (double) size;
    double sum = 0.0;

    for (size_t k = 1; k < power.size(); ++k)
    {
        const auto hz = (double) k * binHz;

        if (hz >= lowHz && hz < highHz)
            sum += power[k];
    }

    return sum;
}

//==============================================================================
// Driving the real signal path.
//==============================================================================

std::vector<float> render (const std::vector<float>& input, DspCore::Params params,
                           double rate = kSampleRate)
{
    DspCore core;
    core.prepare (rate, 512, 1, params.oversampling);
    core.setParams (params);

    // The oversampling filters delay the signal, so dry and wet have to be
    // realigned before anything compares them sample for sample -- an
    // excursion gain measured against a signal that is 80 samples out is a
    // measurement of the delay, not of the curve. The tail is padded so the
    // end of the programme survives the shift.
    const auto latency = (size_t) core.getLatencySamples();

    auto out = input;
    out.insert (out.end(), latency, 0.0f);

    for (size_t at = 0; at < out.size(); at += 512)
    {
        auto* p = out.data() + at;
        const auto n = (int) std::min<size_t> (512, out.size() - at);
        core.process (&p, 1, n);
    }

    out.erase (out.begin(), out.begin() + (long) latency);
    out.resize (input.size(), 0.0f);
    return out;
}

DspCore::Params defaultParams()
{
    DspCore::Params p;
    p.driveAmount = 40.0f;
    p.autoGain = false;
    return p;
}

//==============================================================================
/** The average gain of the shipped curve over each polarity of a signal's
    excursions -- the reference's asymmetry measurement, applied to the curve
    itself rather than to the finished audio.

    Two measurements, because they answer different questions and only one of
    them is about the curve. See the note in verify().
*/
void curveGains (const std::vector<float>& x, double drive, double bias,
                 double& positive, double& negative)
{
    double posIn = 0.0, posOut = 0.0, negIn = 0.0, negOut = 0.0;

    for (auto v : x)
    {
        const double sample = v;

        if (std::abs (sample) < 1.0e-4)
            continue;

        const auto y = (std::tanh (drive * sample + bias) - std::tanh (bias)) / drive;

        if (sample > 0.0) { posIn += sample;  posOut += std::abs (y); }
        else              { negIn += -sample; negOut += std::abs (y); }
    }

    positive = posIn > 0.0 ? posOut / posIn : 0.0;
    negative = negIn > 0.0 ? negOut / negIn : 0.0;
}

//==============================================================================
struct Report
{
    double positiveGain = 0.0, negativeGain = 0.0, asymmetry = 0.0;
    double bandDb[5] {};
    double dryCrest = 0.0, wetCrest = 0.0;
    double matchDb = 0.0;
};

Report analyse (const std::vector<float>& dry, const std::vector<float>& wet)
{
    Report r;

    excursionGains (dry, wet, r.positiveGain, r.negativeGain);
    r.asymmetry = std::abs (r.positiveGain - r.negativeGain);

    r.dryCrest = crestFactorDb (dry);
    r.wetCrest = crestFactorDb (wet);

    // Band deltas are read level-matched. Saturation that arrives louder
    // measures as a lift in every band, which says nothing about its character.
    const auto dryRms = rms (dry), wetRms = rms (wet);
    const auto match = wetRms > 0.0 ? dryRms / wetRms : 1.0;
    r.matchDb = 20.0 * std::log10 (match > 0.0 ? match : 1.0);

    auto matched = wet;

    for (auto& v : matched)
        v = (float) (v * match);

    const auto dryPower = spectrum (dry);
    const auto wetPower = spectrum (matched);

    for (int b = 0; b < 5; ++b)
    {
        const auto d = bandEnergy (dryPower, kFuji[b].lowHz, kFuji[b].highHz);
        const auto w = bandEnergy (wetPower, kFuji[b].lowHz, kFuji[b].highHz);
        r.bandDb[b] = d > 0.0 && w > 0.0 ? 10.0 * std::log10 (w / d) : 0.0;
    }

    return r;
}

//==============================================================================
int verify (const std::string& wavPath)
{
    std::vector<float> dry;
    auto rate = kSampleRate;

    if (! wavPath.empty())
    {
        if (! readWav (wavPath, dry, rate))
        {
            std::printf ("could not read %s\n", wavPath.c_str());
            return 1;
        }

        std::printf ("source: %s (%.0f Hz, %zu samples)\n\n", wavPath.c_str(), rate, dry.size());
    }
    else
    {
        dry = referenceVoice();
        std::printf ("source: synthetic reference voice, 8 s at -18 dBFS RMS\n\n");
    }

    const auto params = defaultParams();
    const auto wet = render (dry, params, rate);
    const auto r = analyse (dry, wet);

    std::printf ("Drive %.0f %%, %dx oversampling, Auto Gain off\n\n",
                 params.driveAmount, params.oversampling);

    double curvePositive = 0.0, curveNegative = 0.0;
    curveGains (dry, DspCore::driveFor (params.driveAmount), AsymmetricShaper::kBias,
                curvePositive, curveNegative);

    const auto curveAsymmetry = std::abs (curvePositive - curveNegative);

    std::printf ("waveshaping asymmetry, at the curve       measured     Fuji\n");
    std::printf ("  positive-half average gain               %6.3f     %6.3f\n",
                 curvePositive, kFujiPositiveGain);
    std::printf ("  negative-half average gain               %6.3f     %6.3f\n",
                 curveNegative, kFujiNegativeGain);
    std::printf ("  asymmetry                                %6.3f     %6.3f\n\n",
                 curveAsymmetry, kFujiAsymmetry);

    std::printf ("the same measurement end to end            %6.3f / %.3f, asymmetry %.3f\n",
                 r.positiveGain, r.negativeGain, r.asymmetry);
    std::printf ("  Lower, and it is worth knowing why rather than adjusting until it\n"
                 "  agrees. An asymmetric curve leaves a DC offset behind -- one half of\n"
                 "  the waveform is compressed and the other is not, so the mean moves --\n"
                 "  and this measurement is largely a measurement of that offset. Block\n"
                 "  the DC and the two halves come back into balance on paper while the\n"
                 "  even-order content that actually makes the difference is untouched.\n"
                 "  This plugin blocks DC, because an offset costs headroom, thumps when\n"
                 "  the drive is automated, and accumulates through a chain.\n"
                 "  Which means Preesh BG's 0.004 does not establish that its curve was\n"
                 "  symmetric: it is also what any DC-blocked saturator measures. Whatever\n"
                 "  separates the two references, this number alone will not find it --\n"
                 "  `measure harmonics` looks at the even-order content directly.\n\n");

    std::printf ("band energy, level-matched (%+.2f dB)      measured     Fuji\n", r.matchDb);

    for (int b = 0; b < 5; ++b)
        std::printf ("  %-22s              %+6.2f     %+6.2f\n",
                     kFuji[b].name, r.bandDb[b], kFuji[b].deltaDb);

    std::printf ("\ncrest factor                              measured     Fuji\n");
    std::printf ("  dry                                      %6.2f     %6.2f\n", r.dryCrest, 20.5);
    std::printf ("  saturated                                %6.2f     %6.2f\n", r.wetCrest, 22.2);
    std::printf ("  change                                   %+6.2f     %+6.2f\n\n",
                 r.wetCrest - r.dryCrest, kFujiCrestChange);

    // The anti-target, checked rather than merely described.
    int warnings = 0;

    if (curveAsymmetry < 0.05)
    {
        std::printf ("WARNING: the curve's asymmetry, %.3f, is near Preesh BG's %.3f. A curve this\n"
                     "         close to symmetric makes almost no even-order content, and the\n"
                     "         result reads as harsh rather than warm however much high end it\n"
                     "         adds.\n", curveAsymmetry, kPreeshAsymmetry);
        ++warnings;
    }

    const auto highOnly = r.bandDb[4] > 1.0 && r.bandDb[3] < 1.0;

    if (highOnly)
    {
        std::printf ("WARNING: the lift is confined to the top band (%.2f dB against %.2f dB in\n"
                     "         2.5-6 kHz), which is the Preesh BG shape -- it measured %.2f dB up\n"
                     "         top and nothing anywhere else. Brighter without warmer.\n",
                     r.bandDb[4], r.bandDb[3], kPreeshHighBandDb);
        ++warnings;
    }

    if (r.wetCrest < r.dryCrest - 0.2)
    {
        std::printf ("WARNING: crest factor fell by %.2f dB. Nothing here should be reducing\n"
                     "         dynamics; the curve is being driven past adding harmonics into\n"
                     "         removing signal.\n", r.dryCrest - r.wetCrest);
        ++warnings;
    }

    if (warnings == 0)
        std::printf ("No anti-target warnings: asymmetric, lift spread across both upper bands,\n"
                     "crest factor preserved or better.\n");

    std::printf ("\nThe numbers are a proxy. Confirm against the reference file by ear before\n"
                 "calling any of this settled.\n");

    return 0;
}

//==============================================================================
int sweep()
{
    const auto dry = referenceVoice();

    std::printf ("drive   pos     neg    asym    20-150  150-600  600-2k5  2k5-6k   6k-18k"
                 "   crest   loss\n");

    for (int amount = 0; amount <= 100; amount += 10)
    {
        auto params = defaultParams();
        params.driveAmount = (float) amount;

        const auto wet = render (dry, params);
        const auto r = analyse (dry, wet);

        std::printf ("%4d %7.3f %7.3f %7.3f  %+7.2f %+8.2f %+8.2f %+8.2f %+8.2f %+7.2f %+6.2f\n",
                     amount, r.positiveGain, r.negativeGain, r.asymmetry,
                     r.bandDb[0], r.bandDb[1], r.bandDb[2], r.bandDb[3], r.bandDb[4],
                     r.wetCrest - r.dryCrest, r.matchDb);
    }

    return 0;
}

//==============================================================================
int makeup()
{
    const auto dry = referenceVoice();
    const auto dryRms = rms (dry);

    std::printf ("// Regenerated by `measure makeup`.\n");
    std::printf ("inline constexpr std::array<float, 11> kMakeupDb\n{\n   ");

    for (int i = 0; i <= 10; ++i)
    {
        auto params = defaultParams();
        params.driveAmount = (float) (i * 10);
        params.autoGain = false;

        const auto wet = render (dry, params);
        const auto db = 20.0 * std::log10 (dryRms / std::max (rms (wet), 1.0e-9));

        std::printf (" %+.2ff%s", db, i < 10 ? "," : "");
    }

    std::printf ("\n};\n");
    return 0;
}

//==============================================================================
int curve()
{
    AsymmetricShaper shaper;
    shaper.setDrive (DspCore::driveFor (40.0f));

    std::printf ("      x     f(x)   gain\n");

    for (double x = -1.0; x <= 1.0001; x += 0.1)
    {
        if (std::abs (x) < 1.0e-9)
            x = 0.0;

        const auto y = shaper.shape (x);
        std::printf ("%7.2f %8.4f %6.3f\n", x, y, std::abs (x) > 1.0e-9 ? y / x : 1.0);
    }

    std::printf ("\nnegative-half drive ratio %.3f, positive-half drive %.3f at Drive 40 %%\n",
                 AsymmetricShaper::kBias, DspCore::driveFor (40.0f));
    return 0;
}

//==============================================================================
int alias()
{
    // A tone high enough that its own harmonics are all above Nyquist, so
    // anything in the band that is not the tone is a folded image.
    constexpr double toneHz = 7500.0;
    const auto input = sine (toneHz, 2.0, 0.5);

    std::printf ("factor   worst image   latency\n");

    for (int factor : { 1, 2, 4, 8 })
    {
        auto params = defaultParams();
        params.driveAmount = 80.0f;
        params.oversampling = factor;

        const auto wet = render (input, params);
        const auto power = spectrum (wet);

        const auto binHz = kSampleRate / 16384.0;
        double fundamental = 0.0, worst = 0.0;

        for (size_t k = 1; k < power.size(); ++k)
        {
            const auto hz = (double) k * binHz;

            // Everything below the tone is an image: the tone's own harmonics
            // all sit above Nyquist and cannot legitimately appear down there.
            if (std::abs (hz - toneHz) < 60.0)
                fundamental = std::max (fundamental, power[k]);
            else if (hz < toneHz - 200.0)
                worst = std::max (worst, power[k]);
        }

        DspCore probe;
        probe.prepare (kSampleRate, 512, 1, factor);

        std::printf ("%4dx   %9.1f dB   %3d samples\n", factor,
                     10.0 * std::log10 (std::max (worst, 1.0e-30) / std::max (fundamental, 1.0e-30)),
                     probe.getLatencySamples());
    }

    return 0;
}

} // namespace

//==============================================================================
int main (int argc, char** argv)
{
    const std::string command = argc > 1 ? argv[1] : "verify";
    const std::string argument = argc > 2 ? argv[2] : "";

    if (command == "harmonics")
    {
        // Even against odd is the measurement that separates the two
        // references, and the one the excursion metric cannot make.
        constexpr double toneHz = 220.0;

        std::printf ("220 Hz at -18 dBFS, relative to the fundamental\n\n");
        std::printf ("drive     2nd     3rd     4th     5th    even-odd\n");

        for (int amount : { 20, 40, 60, 80, 100 })
        {
            auto params = defaultParams();
            params.driveAmount = (float) amount;

            const auto wet = render (sine (toneHz, 4.0, 0.126), params);
            const auto power = spectrum (wet);
            const auto binHz = kSampleRate / 16384.0;

            double harmonic[6] {};

            for (int h = 1; h <= 5; ++h)
                for (size_t k = 1; k < power.size(); ++k)
                    if (std::abs ((double) k * binHz - toneHz * h) < 40.0)
                        harmonic[h] = std::max (harmonic[h], power[k]);

            const auto db = [&harmonic] (int h)
            {
                return 10.0 * std::log10 (std::max (harmonic[h], 1.0e-30)
                                            / std::max (harmonic[1], 1.0e-30));
            };

            const auto even = harmonic[2] + harmonic[4];
            const auto odd  = harmonic[3] + harmonic[5];

            std::printf ("%4d  %+7.1f %+7.1f %+7.1f %+7.1f   %+7.1f dB\n", amount,
                         db (2), db (3), db (4), db (5),
                         10.0 * std::log10 (std::max (even, 1.0e-30) / std::max (odd, 1.0e-30)));
        }

        std::printf ("\nEven-order leading odd is the whole point: the second harmonic is an\n"
                     "octave, so it stays consonant with what produced it. A symmetric curve\n"
                     "makes no even harmonics at all and reads as edge rather than warmth.\n");
        return 0;
    }

    if (command == "fit")
    {
        const auto dry = referenceVoice();
        double best = 1.0e9, bestDrive = 0.0, bestBias = 0.0;

        // Coarse pass, then a fine one around the winner. The fit runs over
        // eight seconds of audio per evaluation, so a single fine grid over
        // the whole plane would take minutes to say the same thing.
        for (int pass = 0; pass < 2; ++pass)
        {
            const auto driveStep = pass == 0 ? 0.2 : 0.01;
            const auto biasStep  = pass == 0 ? 0.02 : 0.001;
            const auto driveFrom = pass == 0 ? 2.0 : std::max (2.0, bestDrive - 0.4);
            const auto driveTo   = pass == 0 ? 14.0 : bestDrive + 0.4;
            const auto biasFrom  = pass == 0 ? 0.0 : std::max (0.0, bestBias - 0.04);
            const auto biasTo    = pass == 0 ? 0.8 : bestBias + 0.04;

            best = 1.0e9;

            for (double a = driveFrom; a <= driveTo; a += driveStep)
                for (double b = biasFrom; b <= biasTo; b += biasStep)
                {
                    double pos = 0.0, neg = 0.0;
                    curveGains (dry, a, b, pos, neg);
                    const auto error = std::abs (pos - kFujiPositiveGain)
                                     + std::abs (neg - kFujiNegativeGain);

                    if (error < best) { best = error; bestDrive = a; bestBias = b; }
                }
        }

        double pos = 0.0, neg = 0.0;
        curveGains (dry, bestDrive, bestBias, pos, neg);
        std::printf ("best fit to Fuji: drive %.2f, bias %.3f\n", bestDrive, bestBias);
        std::printf ("  positive %.3f  negative %.3f  asymmetry %.3f\n\n", pos, neg, std::abs (pos - neg));

        const auto shipped = DspCore::driveFor (defaultParams().driveAmount);
        curveGains (dry, shipped, AsymmetricShaper::kBias, pos, neg);
        std::printf ("as shipped, at Drive %.0f %% (curve drive %.2f, bias %.3f):\n",
                     defaultParams().driveAmount, shipped, AsymmetricShaper::kBias);
        std::printf ("  positive %.3f  negative %.3f  asymmetry %.3f\n", pos, neg, std::abs (pos - neg));
        return 0;
    }

    if (command == "null")
    {
        const auto dry = referenceVoice();
        auto p = defaultParams();
        p.saturationIn = false;
        const auto wet = render (dry, p);
        double pos = 0.0, neg = 0.0, worst = 0.0;
        excursionGains (dry, wet, pos, neg);
        for (size_t i = 0; i < dry.size(); ++i) worst = std::max (worst, (double) std::abs (dry[i] - wet[i]));
        std::printf ("saturation out: pos %.4f neg %.4f  worst sample difference %.2e\n", pos, neg, worst);
        return 0;
    }

    if (command == "bands")
    {
        const auto dry = referenceVoice();
        const auto power = spectrum (dry);
        double total = 0.0;
        for (int b = 0; b < 5; ++b) total += bandEnergy (power, kFuji[b].lowHz, kFuji[b].highHz);
        for (int b = 0; b < 5; ++b)
            std::printf ("%-22s %7.2f dB relative to total\n", kFuji[b].name,
                         10.0 * std::log10 (bandEnergy (power, kFuji[b].lowHz, kFuji[b].highHz) / total));
        return 0;
    }

    if (command == "verify") return verify (argument);
    if (command == "sweep")  return sweep();
    if (command == "makeup") return makeup();
    if (command == "curve")  return curve();
    if (command == "alias")  return alias();

    std::printf ("usage: measure [verify [file.wav] | sweep | harmonics | makeup"
                 " | curve | alias | fit | bands | null]\n");
    return 1;
}
