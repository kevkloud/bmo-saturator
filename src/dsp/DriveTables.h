#pragma once

#include <algorithm>
#include <array>
#include <cmath>

namespace bmosat::tables
{

//==============================================================================
/** DRIVE, as the panel reads it, mapped to the shaper's positive-half drive.

    The two ends are the design decision. kDriveMin is not zero: at DRIVE 0 the
    curve is still the curve, sitting far enough below its knee to be nearly
    linear, so the control changes how much of a character there is rather than
    fading one in. kDriveMax is where a vocal at a sensible working level is
    unmistakably saturated and no further -- past it the curve stops adding
    harmonics and starts removing signal.

    Geometric between them, because what the ear follows is the ratio between
    where the signal sits and where the knee is, not the difference.
*/
inline constexpr float kDriveMin = 0.6f;
inline constexpr float kDriveMax = 53.6f;

//==============================================================================
/** Static output compensation, in decibels, at eleven points across the DRIVE
    range and linearly interpolated between them.

    Measured, not guessed: `measure makeup` drives the real signal path with
    the harness's voice-like reference at -18 dBFS RMS and reports the broadband
    loss at each step. Regenerate it with that command if the curve changes.

    A level match matters more here than on most plugins. Saturation that
    arrives louder gets credit for the loudness, which is the oldest way there
    is to make a change sound like an improvement; without this, every A/B of
    the Drive control would be confounded by it.

    Static by construction. Following the programme with a level detector would
    make this a compressor, and the one thing this plugin must not do is
    reduce dynamics.
*/
inline constexpr std::array<float, 11> kMakeupDb
{
    +0.55f, +0.70f, +0.92f, +1.14f, +1.21f, +1.04f, +0.74f, +0.42f, +0.14f, -0.06f, -0.20f
};

inline float makeupDb (float amountPercent) noexcept
{
    const auto t = std::clamp (amountPercent, 0.0f, 100.0f) * 0.01f
                     * (float) (kMakeupDb.size() - 1);
    const auto i = (size_t) t;
    const auto f = t - (float) i;

    if (i + 1 >= kMakeupDb.size())
        return kMakeupDb.back();

    return kMakeupDb[i] + f * (kMakeupDb[i + 1] - kMakeupDb[i]);
}

} // namespace bmosat::tables
