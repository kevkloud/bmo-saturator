#pragma once

#include "Theme.h"

namespace bmosat::gui
{

/** A rotary control drawn as a potentiometer: a face and a pointer.

    No numeric readout on anything. That came from an engineer who has spent
    years on the hardware: numbers make people mix with their eyes, hunting a
    tidy figure and flinching from a large move. A gain control gets a plus and
    a minus; Drive and Mix, which only go one way, get a plus and nothing at
    the other end, because a minus on a control whose bottom is "none" says
    something untrue.
*/
class Knob : public juce::Slider
{
public:
    enum class Style
    {
        utility,    ///< azure face. Input and output.
        drive       ///< the module's accent, larger. Drive and Mix.
    };

    Knob() : juce::Slider (juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox) {}

    void setStyle (Style s) noexcept   { style = s; }
    Style getStyle() const noexcept    { return style; }

    void setFaceScale (float s) noexcept { faceScale = s; }
    float getFaceScale() const noexcept  { return faceScale; }

    /** Where the dotted track sits, in pixels from the centre. Zero means
        "just outside my own face". */
    void setTrackRadius (float r) noexcept { trackRadius = r; }
    float getTrackRadius() const noexcept  { return trackRadius; }

private:
    Style style = Style::utility;
    float faceScale = 1.0f;
    float trackRadius = 0.0f;
};

//==============================================================================
class BmoLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    BmoLookAndFeel() { refreshColours(); }

    void refreshColours();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle,
                           juce::Slider&) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawHighlighted, bool shouldDrawDown) override;

    juce::Font getLabelFont (juce::Label&) override;

    /** The preset strip is built from TextButtons, and it is panel text like
        any other. The popup list of preset names is not: a heavy display face
        makes a list of names slower to read, so that keeps the system font. */
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;

    /** How far out a track sits from the edge of the face it surrounds, and how
        far a legend then sits beyond the track. The same two gaps everywhere,
        which is what makes the controls read as one family. */
    static constexpr float kTrackGap  = 10.0f;
    static constexpr float kLegendGap = 12.0f;

    /** A ring of dots, used for the track around every control. */
    static void drawDottedArc (juce::Graphics&, juce::Point<float> centre, float radius,
                               float startAngle, float endAngle, juce::Colour, float dotSize);
};

} // namespace bmosat::gui
