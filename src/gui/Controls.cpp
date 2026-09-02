#include "Controls.h"

namespace bmosat::gui
{

//==============================================================================
PlainKnob::PlainKnob (juce::AudioProcessorValueTreeState& state,
                      const juce::String& parameterId,
                      const juce::String& captionText,
                      Knob::Style style,
                      float faceScale)
    : caption (captionText)
{
    knob.setStyle (style);
    knob.setFaceScale (faceScale);
    addAndMakeVisible (knob);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        state, parameterId, knob);
}

void PlainKnob::paint (juce::Graphics& g)
{
    const auto& p = theme::palette();
    const auto tint = knob.getStyle() == Knob::Style::drive ? p.drive : p.azure;

    // The name sits under the knob, as the suite has it.
    theme::drawOutlinedText (g, caption,
                             getLocalBounds().removeFromBottom (kCaptionRow)
                                             .withTrimmedBottom (4).toFloat(),
                             juce::Justification::centred, theme::captionFont (15.0f),
                             knob.isEnabled() ? tint : tint.withAlpha (0.4f));
}

void PlainKnob::resized()
{
    knob.setBounds (getLocalBounds().withTrimmedBottom (kCaptionRow));
}

void PlainKnob::setKnobEnabled (bool shouldBeEnabled)
{
    knob.setEnabled (shouldBeEnabled);
    repaint();
}

//==============================================================================
SwitchButton::SwitchButton (juce::AudioProcessorValueTreeState& state,
                            const juce::String& parameterId,
                            const juce::String& text, bool accent)
{
    button.setButtonText (text);
    button.setName (accent ? "accent" : "pink");
    addAndMakeVisible (button);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        state, parameterId, button);
}

void SwitchButton::resized()                 { button.setBounds (getLocalBounds()); }
void SwitchButton::setSwitchEnabled (bool e) { button.setEnabled (e); }

//==============================================================================
OutputMeter::OutputMeter (std::function<float()> peakSource, std::function<float()> rmsSource)
    : peak (std::move (peakSource)), rms (std::move (rmsSource))
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    startTimerHz (30);
}

void OutputMeter::mouseUp (const juce::MouseEvent&)
{
    vuMode = ! vuMode;
    displayed = 0.0f;
    repaint();
}

void OutputMeter::timerCallback()
{
    const auto level = vuMode ? (rms ? rms() : 0.0f) : (peak ? peak() : 0.0f);

    // A VU meter integrates; a peak meter jumps and falls back slowly.
    const auto rate = vuMode ? 0.28f : (level > displayed ? 1.0f : 0.16f);

    displayed += rate * (level - displayed);
    repaint();
}

void OutputMeter::paint (juce::Graphics& g)
{
    const auto& p = theme::palette();

    auto bounds = getLocalBounds();
    const auto labelArea = bounds.removeFromBottom (12);
    const auto well = bounds.withSizeKeepingCentre (kBarWidth, bounds.getHeight()).toFloat();

    g.setColour (p.meterWell);
    g.fillRoundedRectangle (well, 2.0f);

    const auto db = juce::Decibels::gainToDecibels (displayed, -70.0f);
    const auto lo = vuMode ? -20.0f : -60.0f;
    const auto hi = vuMode ? 3.0f : 0.0f;
    const auto reading = vuMode ? db - kVuReference : db;
    const auto norm = juce::jlimit (0.0f, 1.0f, (reading - lo) / (hi - lo));

    if (norm > 0.002f)
    {
        auto bar = well.reduced (1.5f);
        bar = bar.removeFromBottom (bar.getHeight() * norm);

        const auto hot  = vuMode ? reading > 0.0f  : reading > -1.0f;
        const auto warm = vuMode ? reading > -3.0f : reading > -9.0f;

        g.setColour (hot ? p.meterClip : warm ? p.meterHigh : p.meterLow);
        g.fillRoundedRectangle (bar, 1.5f);
    }

    g.setColour (p.outline.withAlpha (0.6f));
    g.drawRoundedRectangle (well.reduced (0.5f), 2.0f, 1.0f);

    g.setColour (p.textDim);
    g.setFont (theme::labelFont (9.0f));
    g.drawText (vuMode ? "VU" : "dBFS", labelArea.toFloat(), juce::Justification::centred, false);
}

} // namespace bmosat::gui
