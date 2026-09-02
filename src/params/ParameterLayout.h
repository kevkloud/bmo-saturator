#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace bmosat::params
{

//==============================================================================
// Parameter IDs.
//
// This is a permanent, append-only schema. Once a user saves a session,
// automation lanes and stored state are keyed by these exact strings. Renaming
// or reordering silently loses settings in every existing project. Treat a
// change here the way you would treat a wire-protocol change: don't, and if you
// must, add a new ID and migrate on load.
//
// Ranges are as permanent as the IDs, and that is the part people forget.
// Stored automation is normalised 0 to 1, so widening Drive from 0-100 to
// 0-120 would silently rescale every automation point ever written -- a 40 %
// move becoming a 48 % one, with nothing to warn anybody. The golden schema
// test in tests/ParameterTests.cpp exists to turn any such edit into a red
// build.
//==============================================================================

inline constexpr auto kInputGain    = "input_gain";
inline constexpr auto kDrive        = "drive";
inline constexpr auto kMix          = "mix";
inline constexpr auto kOutputLevel  = "output_level";
inline constexpr auto kSatIn        = "sat_in";
inline constexpr auto kPhase        = "phase";
inline constexpr auto kAutoGain     = "auto_gain";
inline constexpr auto kOversampling = "oversampling";

/** Bump only when adding parameters; existing entries keep their original hint. */
inline constexpr int kVersionHint  = 1;
inline constexpr int kStateVersion = 1;

/** Every parameter id, in panel order. Presets reset everything to its default
    before applying their own settings, so a preset cannot leave a stray value
    behind from whatever was loaded before it.

    A parameter missing from this list stops being reset between presets, and
    nothing else goes wrong -- which is why a test asserts that this matches
    the layout below rather than leaving it to be noticed. */
inline juce::StringArray allIds()
{
    return { kInputGain, kDrive, kMix, kOutputLevel,
             kSatIn, kPhase, kAutoGain, kOversampling };
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout create();

} // namespace bmosat::params
