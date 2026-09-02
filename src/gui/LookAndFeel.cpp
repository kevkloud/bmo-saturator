#include "LookAndFeel.h"

namespace bmosat::gui
{

void BmoLookAndFeel::refreshColours()
{
    const auto& p = theme::palette();

    setColour (juce::ResizableWindow::backgroundColourId, p.background);
    setColour (juce::Label::textColourId,                 p.text);

    setColour (juce::ComboBox::backgroundColourId,        p.background);
    setColour (juce::ComboBox::textColourId,              p.text);
    setColour (juce::ComboBox::outlineColourId,           p.outline);
    setColour (juce::ComboBox::arrowColourId,             p.blue);

    setColour (juce::PopupMenu::backgroundColourId,       p.panel);
    setColour (juce::PopupMenu::textColourId,             p.text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, p.blueFill);
    setColour (juce::PopupMenu::highlightedTextColourId,  p.white);

    // The preset strip is built from TextButtons, which otherwise come out in
    // JUCE's default blue and fight the scheme.
    setColour (juce::TextButton::buttonColourId,   p.background);
    setColour (juce::TextButton::buttonOnColourId, p.blueFill);
    setColour (juce::TextButton::textColourOffId,  p.text);
    setColour (juce::TextButton::textColourOnId,   p.white);

    setColour (juce::AlertWindow::backgroundColourId, p.panel);
    setColour (juce::AlertWindow::textColourId,       p.text);
    setColour (juce::AlertWindow::outlineColourId,    p.outline);
    setColour (juce::TextEditor::backgroundColourId,  p.background);
    setColour (juce::TextEditor::textColourId,        p.text);
    setColour (juce::TextEditor::outlineColourId,     p.outline);
    setColour (juce::TextEditor::highlightColourId,   p.blueFill);
}

juce::Font BmoLookAndFeel::getLabelFont (juce::Label& label)
{
    return theme::labelFont (label.getHeight() > 0 ? juce::jmin (12.0f, (float) label.getHeight())
                                                   : 11.0f);
}

juce::Font BmoLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return theme::labelFont (juce::jmin (13.0f, (float) buttonHeight * 0.55f));
}

//==============================================================================
void BmoLookAndFeel::drawDottedArc (juce::Graphics& g, juce::Point<float> centre, float radius,
                                    float startAngle, float endAngle, juce::Colour colour,
                                    float dotSize)
{
    const auto span = endAngle - startAngle;
    const auto count = juce::jlimit (8, 96, juce::roundToInt (radius * span * 0.16f));

    g.setColour (colour);

    for (int i = 0; i <= count; ++i)
    {
        const auto a = startAngle + span * (float) i / (float) count;
        const juce::Point<float> at { centre.x + radius * std::sin (a),
                                      centre.y - radius * std::cos (a) };

        g.fillEllipse (juce::Rectangle<float> (dotSize, dotSize).withCentre (at));
    }
}

//==============================================================================
void BmoLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                       float sliderPos, float startAngle, float endAngle,
                                       juce::Slider& slider)
{
    const auto& p = theme::palette();
    auto* knob = dynamic_cast<Knob*> (&slider);
    const auto style = knob != nullptr ? knob->getStyle() : Knob::Style::utility;

    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();
    const auto centre = bounds.getCentre();
    const auto scale  = knob != nullptr ? knob->getFaceScale() : 1.0f;
    const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f * scale;
    const auto angle  = startAngle + sliderPos * (endAngle - startAngle);
    const auto enabled = slider.isEnabled();

    const auto dim = [enabled] (juce::Colour c) { return enabled ? c : c.withAlpha (0.35f); };

    const auto at = [centre] (float a, float r)
    {
        return juce::Point<float> { centre.x + r * std::sin (a), centre.y - r * std::cos (a) };
    };

    const auto isDrive = style == Knob::Style::drive;
    const auto face    = isDrive ? p.pinkFill : p.azure;
    const auto accent  = isDrive ? p.drive : p.blue;
    const auto faceBox = juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre);

    // The dotted track, with the rest position marked on it.
    {
        const auto given = knob != nullptr ? knob->getTrackRadius() : 0.0f;
        const auto track = given > 0.0f ? given : radius + kTrackGap;

        // The track stops just clear of each symbol rather than running dots
        // through it.
        constexpr float symbolClearance = 0.11f;

        drawDottedArc (g, centre, track, startAngle + symbolClearance, endAngle - symbolClearance,
                       dim (accent.withAlpha (enabled ? 0.55f : 0.2f)), 1.6f);

        // The heavy dot marks the control's rest position and stays there: zero
        // on a control that cuts and boosts, the bottom of the sweep on one
        // that only goes up.
        const auto range = slider.getRange();
        const auto zero  = range.getLength() > 0.0
                             ? (float) juce::jlimit (0.0, 1.0, (0.0 - range.getStart()) / range.getLength())
                             : 0.5f;

        g.setColour (dim (accent));
        g.fillEllipse (juce::Rectangle<float> (5.0f, 5.0f)
                           .withCentre (at (startAngle + zero * (endAngle - startAngle), track)));

        // A plus, and a minus only where there is something to subtract.
        //
        // Drawn rather than set. The panel has no minus sign that matches its
        // plus at every size on every machine, and two strokes and a bar are
        // the one case where drawing beats setting. No outline on them either:
        // they are a few strokes wide and an outline around them reads as a
        // smudge.
        {
            const auto arm = 5.0f;
            const auto weight = 2.6f;

            g.setColour (dim (accent));

            if (range.getStart() < 0.0)
            {
                const auto minusAt = at (startAngle, track);
                g.fillRect (juce::Rectangle<float> (arm * 2.0f, weight).withCentre (minusAt));
            }

            const auto plusAt = at (endAngle, track);
            g.fillRect (juce::Rectangle<float> (arm * 2.0f, weight).withCentre (plusAt));
            g.fillRect (juce::Rectangle<float> (weight, arm * 2.0f).withCentre (plusAt));
        }
    }

    g.setColour (dim (face));
    g.fillEllipse (faceBox);
    g.setColour (dim (isDrive ? p.outline : p.grey));
    g.drawEllipse (faceBox.reduced (0.8f), isDrive ? 1.6f : 2.2f);

    // Pointer.
    {
        const auto tip  = radius - 3.0f;
        const auto tail = radius * 0.05f;

        g.setColour (dim (p.white));
        g.drawLine ({ at (angle, tail), at (angle, tip) }, 2.6f);
    }
}

//==============================================================================
void BmoLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                       bool shouldDrawHighlighted, bool shouldDrawDown)
{
    const auto& p = theme::palette();
    const auto bounds = button.getLocalBounds().toFloat().reduced (3.0f);
    const auto on = button.getToggleState();

    // Auto Gain takes the module's accent; the rest are the suite's pink.
    const auto tint = button.getName() == "accent" ? p.drive : p.engagedPink;

    auto fill = on ? tint : p.grey;

    if (shouldDrawDown)             fill = fill.darker (0.12f);
    else if (shouldDrawHighlighted) fill = fill.brighter (0.06f);

    if (! button.isEnabled())
        fill = fill.withAlpha (0.35f);

    // An engaged switch glows: a few rounded rectangles stepping outwards at
    // falling alpha. Kept faint, because the text has to stay first.
    if (on && button.isEnabled())
        for (int i = 3; i >= 1; --i)
        {
            g.setColour (tint.withAlpha (0.10f * (float) i / 3.0f));
            g.fillRoundedRectangle (bounds.expanded ((float) i), theme::corner + (float) i);
        }

    g.setColour (fill);
    g.fillRoundedRectangle (bounds, theme::corner);

    const auto ink = p.white.withAlpha (button.isEnabled() ? 1.0f : 0.4f);

    // The polarity switch is drawn, not set: typing the slashed O gives back
    // whatever the machine maps it to, which on several faces is a plain O and
    // says nothing.
    if (button.getButtonText() == juce::String (juce::CharPointer_UTF8 ("\xc3\x98")))
    {
        const auto centre = bounds.getCentre();
        const auto r = bounds.getHeight() * 0.30f;
        const auto weight = juce::jmax (1.6f, r * 0.22f);

        juce::Path symbol;
        symbol.addEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (centre));
        symbol.startNewSubPath (centre.x - r * 0.95f, centre.y + r * 0.95f);
        symbol.lineTo         (centre.x + r * 0.95f, centre.y - r * 0.95f);

        g.setColour (juce::Colours::black.withAlpha (ink.getFloatAlpha() * 0.9f));
        g.strokePath (symbol, juce::PathStrokeType (weight + 1.6f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
        g.setColour (ink);
        g.strokePath (symbol, juce::PathStrokeType (weight, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
        return;
    }

    theme::drawOutlinedText (g, button.getButtonText(), bounds, juce::Justification::centred,
                             theme::labelFont (bounds.getHeight() * 0.62f, true), ink);
}

} // namespace bmosat::gui
