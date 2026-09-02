#include "Theme.h"

namespace bmosat::theme
{

const Palette& palette() noexcept { return kLight; }

namespace
{
    juce::Font build (float height, float tracking, bool bold)
    {
        auto options = juce::FontOptions {}.withHeight (height);

        if (bold)
            options = options.withStyle ("Bold");

        return juce::Font (options).withExtraKerningFactor (tracking);
    }
}

juce::Font labelFont (float height, bool bold)
{
    // The tracking is the suite's: panel legends are set noticeably open.
    return build (height, 0.08f, bold);
}

juce::Font captionFont (float height)
{
    return build (height, 0.04f, false);
}

void drawOutlinedText (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area,
                       juce::Justification justification, const juce::Font& font,
                       juce::Colour fill, float outlineThickness)
{
    if (text.isEmpty())
        return;

    juce::GlyphArrangement glyphs;
    glyphs.addFittedText (font, text, area.getX(), area.getY(),
                          area.getWidth(), area.getHeight(), justification, 1);

    juce::Path path;
    glyphs.createPath (path);

    g.setColour (juce::Colours::black.withAlpha (fill.getFloatAlpha()));
    g.strokePath (path, juce::PathStrokeType (outlineThickness,
                                              juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));

    g.setColour (fill);
    g.fillPath (path);
}

} // namespace bmosat::theme
