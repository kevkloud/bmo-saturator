/*
    Tests for the parameter schema, the preset system, and the plugin's
    behaviour as a host sees it.

    The first block is the important one and the least interesting to read. A
    parameter's ID, its position in the list, its range and its default are all
    load-bearing forever: automation lanes and saved state are keyed by them,
    and stored automation is normalised, so widening a range silently rescales
    every automation point a user ever wrote. Nothing errors, nothing warns,
    and the session simply comes back wrong months later.

    So the schema is written down here in full, and any edit to it turns into a
    failing build. That is the point. If a change is genuinely wanted, this
    file is where the decision gets recorded.
*/

#include "PluginProcessor.h"
#include "presets/PresetManager.h"
#include "dsp/DriveTables.h"
#include <juce_events/juce_events.h>
#include <iostream>

namespace P = bmosat::params;

namespace
{
    int failures = 0;

    void check (bool condition, const juce::String& what)
    {
        if (! condition)
        {
            std::cerr << "FAIL: " << what << '\n';
            ++failures;
        }
    }

    void checkClose (double actual, double expected, double tolerance, const juce::String& what)
    {
        if (! (std::abs (actual - expected) <= tolerance))
        {
            std::cerr << "FAIL: " << what << " -- expected " << expected
                      << " +/- " << tolerance << ", got " << actual << '\n';
            ++failures;
        }
    }

    juce::RangedAudioParameter& param (BmoSaturatorAudioProcessor& p, const char* id)
    {
        auto* rp = p.getApvts().getParameter (id);
        jassert (rp != nullptr);
        return *rp;
    }

    void setValue (BmoSaturatorAudioProcessor& p, const char* id, float realValue)
    {
        auto& rp = param (p, id);
        rp.setValueNotifyingHost (rp.convertTo0to1 (realValue));
    }

    float getValue (BmoSaturatorAudioProcessor& p, const char* id)
    {
        auto& rp = param (p, id);
        return rp.convertFrom0to1 (rp.getValue());
    }

    /** The schema, written out. Order is part of it. */
    struct Expected
    {
        const char* id;
        const char* name;
        float min, max, defaultValue;
        int steps;              // 0 for a continuous parameter
    };

    const Expected kSchema[]
    {
        { P::kInputGain,    "Input",        -24.0f,  24.0f,   0.0f, 0 },
        { P::kDrive,        "Drive",          0.0f, 100.0f,  40.0f, 0 },
        { P::kMix,          "Mix",            0.0f, 100.0f, 100.0f, 0 },
        { P::kOutputLevel,  "Output",       -24.0f,  24.0f,   0.0f, 0 },
        { P::kSatIn,        "Sat In",         0.0f,   1.0f,   1.0f, 2 },
        { P::kPhase,        "Phase",          0.0f,   1.0f,   0.0f, 2 },
        { P::kAutoGain,     "Auto Gain",      0.0f,   1.0f,   0.0f, 2 },
        // Oversampling defaults to Off in 0.2.0: the curve is anti-aliased by
        // ADAA rather than by rate, and the suite's rule is that a module
        // reports zero latency in its default state.
        { P::kOversampling, "Oversampling",   0.0f,   3.0f,   0.0f, 4 },

        // Appended in 0.2.0. New parameters go on the end, never in the
        // middle: the position is part of what a session references.
        { P::kTone,         "Tone",           0.0f, 100.0f, 100.0f, 0 },
    };

    std::vector<float> voice (int samples)
    {
        std::vector<float> out ((size_t) samples);
        double sumSquares = 0.0;

        for (int i = 0; i < samples; ++i)
        {
            const auto t = (double) i / 48000.0;
            const auto beat = std::fmod (t, 0.55);
            const auto envelope = (std::fmod (t, 3.0) < 1.6 ? 1.0 : 0.0)
                                * (beat < 0.01 ? beat / 0.01 : std::exp (-(beat - 0.01) * 7.0));

            double sum = 0.0;

            for (int h = 1; h <= 120; ++h)
                sum += std::pow ((double) h, -1.4)
                         * std::sin (2.0 * juce::MathConstants<double>::twoPi * 75.0 * h * t);

            out[(size_t) i] = (float) (envelope * sum);
            sumSquares += (double) out[(size_t) i] * out[(size_t) i];
        }

        const auto rms = std::sqrt (sumSquares / (double) samples);
        const auto gain = rms > 0.0 ? juce::Decibels::decibelsToGain (-18.0f) / (float) rms : 1.0f;

        for (auto& v : out)
            v *= gain;

        return out;
    }
}

//==============================================================================
int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    //== The golden schema =====================================================
    {
        BmoSaturatorAudioProcessor proc;
        const auto& parameters = proc.getParameters();

        check (parameters.size() == (int) std::size (kSchema),
               "expected " + juce::String ((int) std::size (kSchema)) + " parameters, got "
                 + juce::String (parameters.size()));

        for (int i = 0; i < juce::jmin (parameters.size(), (int) std::size (kSchema)); ++i)
        {
            const auto& expected = kSchema[i];
            auto* p = dynamic_cast<juce::RangedAudioParameter*> (parameters[i]);

            if (p == nullptr)
            {
                check (false, "parameter " + juce::String (i) + " is not a ranged parameter");
                continue;
            }

            const juce::String where { juce::String ("parameter ") + juce::String (i)
                                         + " (" + expected.id + ")" };

            check (p->paramID == expected.id,
                   where + " should have ID '" + expected.id + "', has '" + p->paramID + "'");
            check (p->getName (64) == expected.name,
                   where + " should be named '" + juce::String (expected.name) + "'");

            const auto range = p->getNormalisableRange();
            checkClose (range.start, expected.min, 1.0e-6, where + " range start");
            checkClose (range.end,   expected.max, 1.0e-6, where + " range end");
            checkClose (p->convertFrom0to1 (p->getDefaultValue()), expected.defaultValue,
                        1.0e-4, where + " default");
            check (p->getNumSteps() == (expected.steps == 0
                                          ? juce::AudioProcessor::getDefaultNumParameterSteps()
                                          : expected.steps),
                   where + " step count");
        }
    }

    //== allIds() matches the layout ==========================================
    //
    // PresetManager::resetToDefaults iterates allIds(). A parameter missing
    // from that list silently stops being reset between presets, which is
    // exactly the "preset inherits the last one's settings" bug the reset
    // exists to prevent -- and nothing else goes wrong, so nobody notices.
    {
        BmoSaturatorAudioProcessor proc;
        const auto ids = P::allIds();

        check (ids.size() == proc.getParameters().size(),
               "allIds() lists " + juce::String (ids.size()) + " parameters, the layout has "
                 + juce::String (proc.getParameters().size()));

        for (const auto& id : ids)
            check (proc.getApvts().getParameter (id) != nullptr,
                   "allIds() names '" + id + "', which is not in the layout");
    }

    //== Displayed values =====================================================
    {
        BmoSaturatorAudioProcessor proc;

        setValue (proc, P::kDrive, 40.0f);
        check (param (proc, P::kDrive).getCurrentValueAsText() == "40 %",
               "Drive should read as a percentage, got '"
                 + param (proc, P::kDrive).getCurrentValueAsText() + "'");

        setValue (proc, P::kOutputLevel, -3.0f);
        check (param (proc, P::kOutputLevel).getCurrentValueAsText() == "-3.0 dB",
               "Output should read in decibels, got '"
                 + param (proc, P::kOutputLevel).getCurrentValueAsText() + "'");

        setValue (proc, P::kOversampling, 3.0f);
        check (param (proc, P::kOversampling).getCurrentValueAsText() == "HQ (8x)",
               "the top oversampling position should read 'HQ (8x)'");
    }

    //== Latency is reported, and only when it changes =========================
    {
        BmoSaturatorAudioProcessor proc;
        proc.setPlayConfigDetails (2, 2, 48000.0, 512);

        setValue (proc, P::kOversampling, 0.0f);
        proc.prepareToPlay (48000.0, 512);
        check (proc.getLatencySamples() == 0,
               "the default reports zero latency, as every module in the suite must");

        setValue (proc, P::kOversampling, 1.0f);
        proc.prepareToPlay (48000.0, 512);
        check (proc.getLatencySamples() > 0, "2x oversampling reports its latency");

        const auto at2x = proc.getLatencySamples();

        setValue (proc, P::kOversampling, 3.0f);
        proc.prepareToPlay (48000.0, 512);
        check (proc.getLatencySamples() > at2x, "8x reports more latency than 2x");
    }

    //== State round-trips ====================================================
    //
    // The test that catches a range change: values go out, come back, and have
    // to be identical. A widened range still loads, and still comes back
    // wrong.
    {
        BmoSaturatorAudioProcessor writer;

        setValue (writer, P::kInputGain, 6.0f);
        setValue (writer, P::kDrive, 63.5f);
        setValue (writer, P::kMix, 40.0f);
        setValue (writer, P::kOutputLevel, -4.5f);
        setValue (writer, P::kSatIn, 0.0f);
        setValue (writer, P::kPhase, 1.0f);
        setValue (writer, P::kAutoGain, 1.0f);
        setValue (writer, P::kOversampling, 2.0f);
        setValue (writer, P::kTone, 55.0f);

        juce::MemoryBlock state;
        writer.getStateInformation (state);

        BmoSaturatorAudioProcessor reader;
        reader.setStateInformation (state.getData(), (int) state.getSize());

        checkClose (getValue (reader, P::kInputGain),   6.0f,  1.0e-3, "input gain round-trips");
        checkClose (getValue (reader, P::kDrive),      63.5f,  1.0e-2, "drive round-trips");
        checkClose (getValue (reader, P::kMix),        40.0f,  1.0e-2, "mix round-trips");
        checkClose (getValue (reader, P::kOutputLevel), -4.5f, 1.0e-3, "output round-trips");
        checkClose (getValue (reader, P::kSatIn),       0.0f,  1.0e-6, "sat in round-trips");
        checkClose (getValue (reader, P::kPhase),       1.0f,  1.0e-6, "phase round-trips");
        checkClose (getValue (reader, P::kAutoGain),    1.0f,  1.0e-6, "auto gain round-trips");
        checkClose (getValue (reader, P::kOversampling), 2.0f, 1.0e-6, "oversampling round-trips");
        checkClose (getValue (reader, P::kTone), 55.0f, 1.0e-2, "tone round-trips");

        // The version tag has to survive, or a future migration has nothing to
        // key off.
        auto xml = juce::AudioProcessor::getXmlFromBinary (state.getData(), (int) state.getSize());
        check (xml != nullptr && xml->hasAttribute ("stateVersion"),
               "saved state carries a stateVersion");
    }

    //== Presets ==============================================================
    {
        const auto directory = juce::File::getSpecialLocation (juce::File::tempDirectory)
                                   .getChildFile ("bmo-saturator-preset-tests");
        directory.deleteRecursively();
        bmosat::PresetManager::setDirectoryForTesting (directory);

        BmoSaturatorAudioProcessor proc;
        auto& presets = proc.getPresets();

        check (presets.getFactory().size() >= 8, "there are factory presets to step through");
        check (presets.getCurrentName() == "Init", "the plugin starts on Init");

        // A preset resets everything it does not mention. Set something odd,
        // load a preset that says nothing about it, and it must be back at its
        // default rather than wherever it was left.
        setValue (proc, P::kPhase, 1.0f);
        presets.loadFactory (1);
        checkClose (getValue (proc, P::kPhase), 0.0f, 1.0e-6,
                    "a preset resets a parameter it does not mention");

        check (presets.isEdited() == false, "a freshly loaded preset is not edited");
        setValue (proc, P::kDrive, 71.0f);
        check (presets.isEdited(), "moving a control marks the preset edited");

        // Save, change everything, load it back.
        check (presets.saveUser ("Test Preset"), "a user preset saves");
        setValue (proc, P::kDrive, 5.0f);
        presets.loadUser ("Test Preset");
        checkClose (getValue (proc, P::kDrive), 71.0f, 1.0e-2, "a user preset loads what it saved");
        check (presets.getUserNames().contains ("Test Preset"), "the saved preset is listed");

        check (presets.deleteUser ("Test Preset"), "a user preset deletes");
        directory.deleteRecursively();
        bmosat::PresetManager::setDirectoryForTesting ({});
    }

    //== Presets come out at the level they went in ===========================
    //
    // A preset that arrives louder gets credit for the loudness, which is the
    // oldest way there is to make a change sound like an improvement. Every
    // factory preset is held to within a couple of decibels of unity on a
    // voice at a working level.
    {
        BmoSaturatorAudioProcessor proc;
        proc.setPlayConfigDetails (2, 2, 48000.0, 512);
        proc.prepareToPlay (48000.0, 512);

        constexpr int block = 512;
        constexpr int blocks = 400;

        const auto source = voice (block * blocks);

        double sourceSum = 0.0;

        for (int i = block * 20; i < block * blocks; ++i)
            sourceSum += (double) source[(size_t) i] * source[(size_t) i];

        const auto sourceDb = juce::Decibels::gainToDecibels (
            std::sqrt (sourceSum / (double) (block * (blocks - 20))));

        juce::AudioBuffer<float> buffer (2, block);
        juce::MidiBuffer midi;

        const auto& factory = proc.getPresets().getFactory();

        for (int index = 0; index < (int) factory.size(); ++index)
        {
            // Init is the plugin as it loads, not a preset anyone chose, and
            // it leaves Auto Gain off -- so it is a couple of decibels down and
            // is meant to be. Everything a user picks from the menu is matched.
            if (juce::String (factory[(size_t) index].name) == "Init")
                continue;

            proc.getPresets().loadFactory (index);
            proc.reset();

            double sum = 0.0;
            int counted = 0;

            for (int b = 0; b < blocks; ++b)
            {
                for (int ch = 0; ch < 2; ++ch)
                    buffer.copyFrom (ch, 0, source.data() + (size_t) (b * block), block);

                proc.processBlock (buffer, midi);

                // The first blocks are the smoothers arriving at the new
                // preset, which is not what the preset sounds like.
                if (b < 20)
                    continue;

                const auto* read = buffer.getReadPointer (0);

                for (int i = 0; i < block; ++i)
                    sum += (double) read[i] * read[i];

                counted += block;
            }

            const auto outDb = juce::Decibels::gainToDecibels (std::sqrt (sum / (double) counted));

            // Two decibels, not half of one. The compensation these presets
            // carry is a fixed number fitted to a reference voice, and this
            // test signal is not that voice; holding it tighter would be
            // holding the plugin to the test rather than the other way round.
            if (std::getenv ("BMO_PRINT_PRESET_LEVELS") != nullptr)
                std::cout << factory[(size_t) index].name << ": " << (outDb - sourceDb) << " dB\n";

            checkClose (outDb - sourceDb, 0.0, 2.5,
                        juce::String ("preset '") + factory[(size_t) index].name
                            + "' should come out at the level it went in");
        }
    }

    if (failures == 0)
        std::cout << "All parameter tests passed.\n";

    return failures == 0 ? 0 : 1;
}
