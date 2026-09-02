#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace bmosat::theme
{

/** The suite's light scheme, as FrostyEQ established it: pink for the section
    legends, azure for the gain controls, white rings, everything on a
    near-white panel, and every piece of text drawn with a thin black outline
    so a pale fill still reads against a pale ground.

    A module of the suite keeps the structure and the plate and differs in its
    accent, so that a rack of these reads as one instrument and you can still
    tell one from another at a glance. This one's accent is `drive` -- the
    warmer pink on the Drive control, which is the only control on the panel
    that does anything a level control does not.
*/
struct Palette
{
    juce::Colour background, panel, outline, hairline;
    juce::Colour text, textDim;
    juce::Colour blue, blueFill, pink, pinkFill, white;
    juce::Colour azure, drive, engagedPink, labelPink, grey;
    juce::Colour meterLow, meterHigh, meterClip, meterWell;
};

inline const Palette kLight
{
    juce::Colour (0xffefefef),   // background
    juce::Colour (0xffe4e4e4),   // panel: header and preset strip
    juce::Colour (0xff9e9e9e),   // outline: knob edges
    juce::Colour (0xffb4b4b4),   // hairline: section rules

    juce::Colour (0xff6f6f6f),   // text
    juce::Colour (0xff9a9a9a),   // textDim

    juce::Colour (0xff4fb8e8),   // blue: dotted tracks
    juce::Colour (0xff7fd0f2),   // blueFill
    juce::Colour (0xfff08cb4),   // pink
    juce::Colour (0xfffbc8d9),   // pinkFill: the Drive face
    juce::Colour (0xffffffff),   // white: rings, pointers

    juce::Colour (0xff97ddff),   // azure: knob caps, INPUT/OUTPUT
    juce::Colour (0xfff2a0bf),   // drive: this module's accent
    juce::Colour (0xfff08eb5),   // engagedPink: switches when engaged
    juce::Colour (0xffffc8dd),   // labelPink: section legends
    juce::Colour (0xffa6a6a6),   // grey: disengaged switches

    juce::Colour (0xff6bbf7a),
    juce::Colour (0xffe0b040),
    juce::Colour (0xffe0685a),
    juce::Colour (0xffd6d6d6)    // meterWell
};

const Palette& palette() noexcept;

inline constexpr float corner = 3.0f;

/** The two places a typeface is named.

    FrostyEQ embeds two commercial display faces, which is a licensing question
    its own README leaves open: embedding a font in an AGPLv3 repository
    redistributes the file to everyone who clones it, and an ordinary desktop
    licence does not permit that. Rather than inherit an unsettled question,
    this plugin ships with no font files at all and asks the system for a sans
    face, tracked out to match the suite's spacing.

    That does mean the panel is not glyph-identical between a Mac and a Windows
    machine. It is the same layout, the same weights and the same spacing --
    the design survives; the exact letterforms do not. When the licence
    question is settled for the suite, this is the one file that changes: drop
    the files in, add them to the binary-data target, and point these two
    functions at them.
*/
juce::Font labelFont (float height, bool bold = false);
juce::Font captionFont (float height);

/** Text with a thin black outline around it, which is how every label on the
    panel is drawn. The outline takes the fill's alpha, so dimming a label
    dims its outline with it rather than leaving a hard black ghost. */
void drawOutlinedText (juce::Graphics&, const juce::String&, juce::Rectangle<float>,
                       juce::Justification, const juce::Font&, juce::Colour fill,
                       float outlineThickness = 1.0f);

} // namespace bmosat::theme
