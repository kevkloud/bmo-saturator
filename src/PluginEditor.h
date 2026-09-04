#pragma once

#include "PluginProcessor.h"
#include "gui/Controls.h"
#include "gui/PresetBar.h"
#include <memory>
#include <vector>

/** A vertical strip, in the suite's shape: one narrow column, input at the top,
    the character control in the middle, output at the bottom.

    Six controls and three switches, which is the whole plugin. There is no
    curve display, no analyser, and no numeric readout on anything -- a control
    is marked with a plus and, where it cuts as well, a minus. The numbers make
    people mix with their eyes, hunting a tidy figure and flinching from a
    large move.

    TONE is on the panel rather than baked in, and that is a deliberate
    reversal. Measuring the reference properly showed that most of what it does
    to the spectrum is an equaliser -- a bell around 7 kHz -- rather than
    harmonic generation. A plugin that applies 8 dB of fixed EQ while calling
    itself a saturator is lying to whoever loads it, so the EQ is a control
    with a name, and turning it down leaves the saturation on its own.

    Nothing on the panel adjusts the asymmetry or the harmonic weighting,
    though: those are the plugin rather than settings of it. Drive scales the
    intensity of one fitted curve and leaves its character alone.

    The panel is drawn once at a fixed size and scaled as a whole. Laying it
    out again at each new size keeps the controls the same size while the gaps
    between them stretch, so the design comes apart as soon as it is not at its
    default size. A single uniform transform scales knobs, legends, fonts and
    spacing together.
*/
class BmoSaturatorAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit BmoSaturatorAudioProcessorEditor (BmoSaturatorAudioProcessor&);
    ~BmoSaturatorAudioProcessorEditor() override;

    void resized() override;

    /** The size everything is laid out at. Any other size is this, scaled.
        740 is the suite's common design height: every module is the same
        height so a rack of them does not look ragged. */
    static constexpr int kDesignWidth  = 260;
    static constexpr int kDesignHeight = 740;

private:
    //==========================================================================
    /** Everything on the front of the plugin, at design size. */
    class Panel final : public juce::Component
    {
    public:
        explicit Panel (BmoSaturatorAudioProcessor&);

        void paint (juce::Graphics&) override;
        void resized() override;

    private:
        bmosat::gui::PresetBar presetBar;

        bmosat::gui::PlainKnob inputGain, drive, tone, mix, outputLevel;
        bmosat::gui::SwitchButton satIn, phase, autoGain;
        bmosat::gui::OutputMeter meter;

        /** A section rule: a legend, optionally with a hairline either side. */
        struct Rule { int y; juce::String text; bool lines; };
        std::vector<Rule> rules;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Panel)
    };

    bmosat::gui::BmoLookAndFeel lookAndFeel;
    Panel panel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BmoSaturatorAudioProcessorEditor)
};
