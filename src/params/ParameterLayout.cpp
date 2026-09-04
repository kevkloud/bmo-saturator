#include "ParameterLayout.h"

namespace bmosat::params
{

namespace
{
    juce::AudioParameterFloatAttributes dbAttr()
    {
        return juce::AudioParameterFloatAttributes()
            .withLabel ("dB")
            .withStringFromValueFunction ([] (float v, int)
            {
                return (v > 0.0f ? "+" : "") + juce::String (v, 1) + " dB";
            });
    }

    juce::AudioParameterFloatAttributes percentAttr()
    {
        return juce::AudioParameterFloatAttributes()
            .withLabel ("%")
            .withStringFromValueFunction ([] (float v, int)
            {
                return juce::String (juce::roundToInt (v)) + " %";
            });
    }

    std::unique_ptr<juce::AudioParameterFloat> makeFloat (const char* id,
                                                          const juce::String& paramName,
                                                          float min, float max,
                                                          float step, float def,
                                                          juce::AudioParameterFloatAttributes attr = {})
    {
        return std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, kVersionHint }, paramName,
            juce::NormalisableRange<float> { min, max, step }, def, std::move (attr));
    }

    std::unique_ptr<juce::AudioParameterChoice> makeChoice (const char* id,
                                                            const juce::String& paramName,
                                                            juce::StringArray choices,
                                                            int def)
    {
        return std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { id, kVersionHint }, paramName, std::move (choices), def);
    }

    std::unique_ptr<juce::AudioParameterBool> makeBool (const char* id,
                                                        const juce::String& paramName,
                                                        bool def)
    {
        return std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { id, kVersionHint }, paramName, def);
    }
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout create()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Input is how hard the signal arrives at the curve, and the curve is
    // level-dependent -- so this is a second drive control in everything but
    // name, exactly as winding up the mic gain is on a piece of hardware.
    layout.add (makeFloat (kInputGain, "Input", -24.0f, 24.0f, 0.01f, 0.0f, dbAttr()));

    // Drive scales the intensity of the curve and nothing else: the asymmetry
    // ratio and the frequency weighting are the same at every setting, so the
    // character does not change with the amount. See dsp/Shaper.h.
    layout.add (makeFloat (kDrive, "Drive", 0.0f, 100.0f, 0.1f, 40.0f, percentAttr()));

    // Appended after Mix rather than beside Drive, because parameter order is
    // permanent once a session references it and this one arrived later. The
    // panel is free to put it wherever it belongs.
    layout.add (makeFloat (kMix, "Mix", 0.0f, 100.0f, 0.1f, 100.0f, percentAttr()));
    layout.add (makeFloat (kOutputLevel, "Output", -24.0f, 24.0f, 0.01f, 0.0f, dbAttr()));

    layout.add (makeBool (kSatIn, "Sat In", true));
    layout.add (makeBool (kPhase, "Phase", false));

    // Off by default, because a level match is a judgement about how the
    // plugin should be auditioned rather than about how it should sound, and
    // the person turning it on should be the one who decided that.
    layout.add (makeBool (kAutoGain, "Auto Gain", false));

    // Off by default. The curve is anti-aliased by ADAA rather than by rate,
    // so the folded images at 1x sit around -49 dB without it, and the suite's
    // rule is that every module reports zero latency in its default state.
    // Oversampling is there for anyone who wants to spend latency on the last
    // 30 dB.
    layout.add (makeChoice (kOversampling, "Oversampling",
        { "Off", "2x", "4x", "HQ (8x)" }, 0));

    // Appended last: the voicing arrived after the first release, and the
    // parameter list is append-only.
    layout.add (makeFloat (kTone, "Tone", 0.0f, 100.0f, 0.1f, 100.0f, percentAttr()));

    return layout;
}

} // namespace bmosat::params
