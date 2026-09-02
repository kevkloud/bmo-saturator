#pragma once

#include "PluginProcessor.h"
#include "gui/Controls.h"
#include "gui/PresetBar.h"
#include <memory>
#include <vector>

/** A vertical strip, in the suite's shape: one narrow column, input at the top,
    the character control in the middle, output at the bottom.

    Five controls and three switches, which is the whole plugin. There is no
    curve display, no analyser, and no numeric readout on anything -- a control
    is marked with a plus and, where it cuts as well, a minus. The numbers make
    people mix with their eyes, hunting a tidy figure and flinching from a
    large move.

    Nothing on the panel adjusts the asymmetry or the frequency weighting,
    because those are the plugin rather than settings of it: Drive scales the
    intensity of one fitted curve and leaves its character alone. A control
    that could flatten the asymmetry would let this sound like the thing it was
    specified not to sound like.

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

        bmosat::gui::PlainKnob inputGain, drive, mix, outputLevel;
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
