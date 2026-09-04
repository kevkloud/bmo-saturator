#include "PluginEditor.h"

using namespace bmosat;
using namespace bmosat::gui;
namespace P = bmosat::params;

namespace
{
    constexpr int kHeader    = 28;
    constexpr int kPresetRow = 24;
    constexpr int kPad       = 10;

    constexpr int kRuleRow   = 20;
    constexpr int kGainRow   = 96;    // knob plus the name under it
    constexpr int kPairRow   = 108;   // Tone and Mix, side by side
    constexpr int kDriveRow  = 176;   // the one control that gets room
    constexpr int kSwitchRow = 40;
    constexpr int kOutputRow = 120;

    // One size for all three switches. Three different widths makes them read
    // as three unrelated things rather than a row of switches.
    constexpr int kSwitchWidth  = 62;
    constexpr int kSwitchHeight = 26;

    const juce::String phaseGlyph = juce::String (juce::CharPointer_UTF8 ("\xc3\x98"));
}

//==============================================================================
BmoSaturatorAudioProcessorEditor::Panel::Panel (BmoSaturatorAudioProcessor& p)
    : presetBar   (p.getPresets()),
      inputGain   (p.getApvts(), P::kInputGain,   "INPUT"),
      drive       (p.getApvts(), P::kDrive,       "DRIVE", Knob::Style::drive, 0.66f),
      tone        (p.getApvts(), P::kTone,        "TONE",  Knob::Style::drive, 0.46f),
      mix         (p.getApvts(), P::kMix,         "MIX",   Knob::Style::drive, 0.46f),
      outputLevel (p.getApvts(), P::kOutputLevel, "OUTPUT"),
      satIn    (p.getApvts(), P::kSatIn,    "SAT"),
      phase    (p.getApvts(), P::kPhase,    phaseGlyph),
      autoGain (p.getApvts(), P::kAutoGain, "AUTO", true),
      meter  ([&p] { return juce::jmax (p.getOutputPeak (0), p.getOutputPeak (1)); },
              [&p] { return juce::jmax (p.getOutputRms  (0), p.getOutputRms  (1)); })
{
    for (auto* c : std::initializer_list<juce::Component*> {
             &presetBar, &inputGain, &drive, &tone, &mix,
             &satIn, &phase, &autoGain, &outputLevel, &meter })
        addAndMakeVisible (c);
}

//==============================================================================
void BmoSaturatorAudioProcessorEditor::Panel::paint (juce::Graphics& g)
{
    const auto& p = theme::palette();
    g.fillAll (p.background);

    auto header = getLocalBounds().removeFromTop (kHeader);
    g.setColour (p.panel);
    g.fillRect (header);

    g.setColour (p.text);
    g.setFont (theme::labelFont (12.5f, true));
    g.drawText ("BMO SATURATOR", header.reduced (kPad, 0), juce::Justification::centredLeft, false);

    // Section legends, with a hairline running out to either side.
    for (const auto& rule : rules)
    {
        const auto font = theme::labelFont (14.0f, true);

        const auto width = juce::jmax (44, juce::roundToInt (
            juce::GlyphArrangement::getStringWidth (font, rule.text)) + 16);
        const auto box = juce::Rectangle<int> (0, rule.y, getWidth(), kRuleRow)
                             .withSizeKeepingCentre (width, kRuleRow);

        theme::drawOutlinedText (g, rule.text, box.toFloat(), juce::Justification::centred,
                                 font, p.labelPink);

        if (! rule.lines)
            continue;

        const auto y = (float) (rule.y + kRuleRow / 2);
        const auto dashes = std::array<float, 2> { 2.0f, 3.0f };

        g.setColour (p.hairline);
        g.drawDashedLine ({ (float) kPad, y, (float) box.getX() - 6.0f, y },
                          dashes.data(), 2, 1.0f);
        g.drawDashedLine ({ (float) box.getRight() + 6.0f, y, (float) (getWidth() - kPad), y },
                          dashes.data(), 2, 1.0f);
    }
}

void BmoSaturatorAudioProcessorEditor::Panel::resized()
{
    rules.clear();
    auto area = getLocalBounds();

    area.removeFromTop (kHeader);

    presetBar.setBounds (area.removeFromTop (kPresetRow));
    area.reduce (kPad, 6);

    const auto rule = [&] (const juce::String& text, bool lines)
    {
        rules.push_back ({ area.removeFromTop (kRuleRow).getY(), text, lines });
    };

    inputGain.setBounds (area.removeFromTop (kGainRow));

    // A little air between a control's name and the legend of the section
    // below it, or the two read as one block of text.
    area.removeFromTop (6);

    // Drive is the plugin. It gets the middle of the panel and the largest
    // face, and the eye should land on it before anything else.
    rule ("SATURATION", true);
    drive.setBounds (area.removeFromTop (kDriveRow));

    // Tone and Mix share a row: neither is the reason you reached for this,
    // and side by side they read as the two things you adjust after the fact.
    rule ("TONE   /   BLEND", true);

    {
        auto pair = area.removeFromTop (kPairRow);
        const auto half = pair.getWidth() / 2;
        tone.setBounds (pair.removeFromLeft (half));
        mix.setBounds (pair);
    }

    rule ({}, true);

    {
        auto switches = area.removeFromTop (kSwitchRow);
        constexpr int gap = 6;

        auto group = switches.withSizeKeepingCentre (kSwitchWidth * 3 + gap * 2, kSwitchHeight);
        satIn.setBounds (group.removeFromLeft (kSwitchWidth));
        group.removeFromLeft (gap);
        phase.setBounds (group.removeFromLeft (kSwitchWidth));
        group.removeFromLeft (gap);
        autoGain.setBounds (group);
    }

    {
        // Every knob on the panel shares one centre line, output included. The
        // meter is not a knob and does not join it: it goes out to the right
        // margin, where it reads as an indicator beside the strip rather than
        // as something that shoves the output knob off the axis.
        auto bottom = area.removeFromTop (kOutputRow);

        meter.setBounds (bottom.withTrimmedTop (4).withTrimmedBottom (22).removeFromRight (44));
        outputLevel.setBounds (bottom);
    }
}

//==============================================================================
BmoSaturatorAudioProcessorEditor::BmoSaturatorAudioProcessorEditor (BmoSaturatorAudioProcessor& p)
    : juce::AudioProcessorEditor (&p), panel (p)
{
    lookAndFeel.refreshColours();
    setLookAndFeel (&lookAndFeel);

    // The panel is always this size; the editor scales it.
    panel.setBounds (0, 0, kDesignWidth, kDesignHeight);
    addAndMakeVisible (panel);

    setResizable (true, true);
    getConstrainer()->setFixedAspectRatio ((double) kDesignWidth / (double) kDesignHeight);
    setResizeLimits (kDesignWidth * 2 / 3, kDesignHeight * 2 / 3,
                     kDesignWidth * 2,     kDesignHeight * 2);
    setSize (kDesignWidth, kDesignHeight);
}

BmoSaturatorAudioProcessorEditor::~BmoSaturatorAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

//==============================================================================
void BmoSaturatorAudioProcessorEditor::resized()
{
    // One uniform scale, so everything keeps its proportions.
    panel.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) kDesignWidth));
}
