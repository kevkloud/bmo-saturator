#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "LookAndFeel.h"
#include <functional>

namespace bmosat::gui
{

/** A control with its name underneath and nothing else: no number, a plus at
    one end and, where the control cuts as well as boosts, a minus at the
    other. Every knob on this panel is one of these; only the size and the
    accent differ. */
class PlainKnob final : public juce::Component
{
public:
    PlainKnob (juce::AudioProcessorValueTreeState&, const juce::String& parameterId,
               const juce::String& caption, Knob::Style = Knob::Style::utility,
               float faceScale = 0.50f);

    void paint (juce::Graphics&) override;
    void resized() override;

    void setKnobEnabled (bool);

private:
    /** Room under the knob for its name. */
    static constexpr int kCaptionRow = 22;

    juce::String caption;
    Knob knob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlainKnob)
};

//==============================================================================
class SwitchButton final : public juce::Component
{
public:
    /** `accent` picks the module's own tint rather than the suite pink. */
    SwitchButton (juce::AudioProcessorValueTreeState&, const juce::String& parameterId,
                  const juce::String& text, bool accent = false);

    void resized() override;
    void setSwitchEnabled (bool);

private:
    juce::ToggleButton button;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SwitchButton)
};

//==============================================================================
/** Output meter, switchable between peak dBFS and VU by clicking it.

    VU is not a different scale on the same number: it is an RMS reading with
    slow ballistics, 0 VU at -18 dBFS, which is why it reads weight where a peak
    meter reads headroom. On this plugin that distinction earns its place --
    saturation moves peak and average level in opposite directions, and a peak
    meter alone will tell you the level fell while the thing gets louder.
*/
class OutputMeter final : public juce::Component,
                          private juce::Timer
{
public:
    OutputMeter (std::function<float()> peakSource, std::function<float()> rmsSource);

    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    void timerCallback() override;

    std::function<float()> peak, rms;
    float displayed = 0.0f;
    bool  vuMode = false;

    static constexpr float kVuReference = -18.0f;
    static constexpr int   kBarWidth    = 14;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OutputMeter)
};

} // namespace bmosat::gui
