# BMO Saturator

A VST3 / AU saturator for Ableton Live, Logic, and other hosts. Part of the BMO
suite; built to a specification derived from before-and-after measurements of a
real vocal take rather than from a circuit.

Two stages, both fitted to that reference and both on the panel. An asymmetric
waveshaper generates the harmonics, and only the harmonics are filtered before
being added back, so the new energy lands above 2.5 kHz rather than in the
midrange. A voicing stage — a bell around 7 kHz — supplies the rest, because
measuring the reference properly showed that most of what it does to the
spectrum is an equaliser rather than distortion. See
[what the reference actually is](#what-the-reference-actually-is).

There is no compressor, limiter, or peak reduction anywhere in it.

![The panel](docs/ui.png)

## Status

Builds as VST3, AU, and Standalone, and passes its own test suite. Not yet
checked by ear against the reference material, which is the only test that
finally matters — see [Verifying by ear](#verifying-by-ear).

**Downloads:** macOS (universal, VST3 + AU) and Windows (VST3) builds are
attached to each [release](https://github.com/kevkloud/bmo-saturator/releases).
Neither is code-signed yet, so both operating systems will complain the first
time — see the release notes for the two-click way past it.

## Measured

Against the reference vocal itself — the actual dry and processed files, not a
synthetic stand-in. Reference preset: Drive 40 %, Tone 100 %, Auto Gain on.

| band | BMO 0.2.0 | Fuji target | 0.1.0, for comparison |
|---|---|---|---|
| 20 Hz – 150 Hz | −0.59 dB | −1.22 dB | −0.13 dB |
| 150 Hz – 600 Hz | −1.18 dB | −1.19 dB | +0.32 dB |
| 600 Hz – 2.5 kHz | −0.98 dB | −0.80 dB | −1.47 dB |
| **2.5 kHz – 6 kHz** | **+6.15 dB** | **+6.56 dB** | −0.99 dB |
| **6 kHz – 18 kHz** | **+8.13 dB** | **+8.51 dB** | +0.35 dB |
| crest factor change | +3.32 dB | +1.67 dB | −1.84 dB |

Every band is within half a decibel. Crest factor is the one metric still out:
the voicing lifts transient high end, so the result opens up more than the
reference does. Second harmonic leads third by about 6 dB at the default drive.

```bash
./build/measure compare dry.wav processed.wav   # any before/after pair
./build/measure fitfile dry.wav target.wav      # fit the character to a pair
./build/measure render in.wav out.wav 40 1 1    # put a file through the plugin
./build/measure verify                          # synthetic regression check
./build/measure harmonics                       # even against odd, by drive
```

`fitfile` is the one that matters. `verify` runs on a synthetic signal and is a
regression check, not a fit target — see the warning in
[known gaps](docs/TEST_PLAN.md).

## How it works

**The curve.** `shape(x) = (tanh(a·x + b) − tanh(b)) / a`, drive `a`, fixed
offset `b`, anti-aliased by first-order ADAA. The offset is the whole point: a
symmetric curve is an odd function and makes only odd harmonics, which is grit
and, up top, sizzle. Offsetting the operating point breaks that symmetry and
brings in even orders — second harmonic first, which is an octave and so stays
consonant with whatever produced it. `b` is held in the driven domain, so Drive
scales the curve without reshaping it: the asymmetry ratio and the even-to-odd
balance are identical at every setting and only the intensity moves.

**Where the harmonics go.** A waveshaper applied the ordinary way puts its
harmonics wherever they fall, which on a voice is mostly 600 Hz to 2.5 kHz.
The reference does the opposite — flat below 2.5 kHz, 6 to 8 dB above it — and
no single curve applied to a whole signal does that at any drive setting. So
the curve's output is split into the part that was already there and the part
it added (the residual, `shape(x) − x`), and only the residual is filtered.
Two further generators, each fed the signal below a corner and read only above
it, refill 600 Hz – 2.5 kHz and fill 2.5 kHz upwards.

That last detail matters more than it looks. The residual of a compressive
curve is not purely new harmonics: part of it is a negative copy of its input.
Amplify a full-band residual and that part subtracts the programme's own top
end, so an earlier version of this measured nearly 2 dB *darker* at 6–18 kHz
while claiming to add sheen. Generating from a band that has no top end in it
leaves nothing up there to subtract from.

**Dynamics.** The dry path through the stage is never attenuated and the added
residual is largest where the waveform moves fastest — on transients. The crest
factor therefore rises rather than falls. Nothing here follows the programme
with a level detector; Auto Gain is a fixed number per Drive setting, because
anything else would make this a compressor.

## What the reference actually is

The plugin was specified from measurements of a vocal before and after "Fuji",
described as waveshaping with a particular asymmetry. Given the files
themselves, that description does not survive measurement.

Fit the best linear time-invariant filter from the dry file to the processed
one — any amplitude and phase response, no nonlinearity at all — and it
explains the transformation to within **−16 to −17 dB in every band**. Around
97 % of what Fuji does is a filter. The fitted response:

```
    63 Hz  −2.0 dB        2.0 kHz  −0.2 dB
   250 Hz  −1.7 dB        3.2 kHz  +3.0 dB
   500 Hz  −2.4 dB        5.0 kHz  +6.5 dB
  1.0 kHz  −1.7 dB        6.4 kHz  +9.7 dB
                          8.0 kHz  +10.1 dB
                           10 kHz  +6.1 dB
                           16 kHz  +1.2 dB
```

A broad bell at about 7 kHz, roughly 2 dB of broadband trim, and a high-pass at
the bottom. The +6 and +8 dB band lifts in the brief are that bell. They are
not harmonic generation, and no waveshaper reaches them — 0.1.0 tried, and
managed +0.4 dB on the real file while measuring +8 dB on the synthetic signal
it had been fitted against.

So the plugin now has a **TONE** control carrying that voicing, and it is on
the panel with a name rather than baked into the curve. A plugin that applies
8 dB of fixed EQ while calling itself a saturator is lying to whoever loads it.
Turn TONE down and the saturation is on its own; turn it up and it matches the
reference.

Measured the same way, this build generates rather more genuine harmonic
content than Fuji does: 10 % new content in the midrange against Fuji's 3 %.

## The two references

The specification came with a target ("Fuji") and an explicit anti-target
("Preesh BG", judged harsh and sizzly by ear). Both are checked in
`tools/measure/main.cpp`, and `measure verify` warns if the result drifts
towards the second: near-zero asymmetry, or a lift confined to the top band
with nothing below it.

Three things are worth reporting back about that specification.

**The asymmetry figures do not come from these files.** Measured on the actual
dry/processed pair the way the brief describes, Fuji's positive and negative
average gains are 0.965 and 1.015 — an asymmetry of 0.05, not 0.22. Whatever
produced 0.62 / 0.84 was a different calculation or a different pair. Since the
band and crest figures reproduce exactly, the rest of the brief checks out; this
one number does not.

**And the measurement is mostly a DC measurement anyway.** An asymmetric curve
leaves a DC offset behind — one half of the waveform is compressed and the
other is not, so the mean moves. Split the signal by polarity and compare
average gains, and what you are largely measuring is that offset. Block the DC
and the two halves come back into balance on paper while the even-order content
that actually makes the difference is untouched. This plugin blocks DC (an
offset costs headroom, thumps when the drive is automated, and accumulates
through a chain), and consequently measures 0.06 end to end where its curve
measures 0.220.

Which means **Preesh BG's 0.004 does not establish that its curve was
symmetric.** That figure is also what any DC-blocked saturator measures. If the
two references were processed by chains that differ in DC handling, this
particular number cannot tell them apart. `measure harmonics` — second and
fourth against third and fifth — is the measurement that does.

**Two different curves hit the same average gains.** A curve made asymmetric by
driving its halves at different rates fits 0.62 / 0.84 just as well as the
offset form and measures its second harmonic ~4 dB *below* its third: a
mostly-odd distortion, which is the anti-reference's character. Same asymmetry
on paper, opposite sound. Both were built here; the offset form is the one that
shipped.

**Band deltas depend heavily on the source.** How many decibels a band lifts is
a statement about how much the source already had there. The same processing
measures +5.7 dB at 2.5–6 kHz on the reference voice and +3 dB on a bare
harmonic series with more native top end. The figures above are reproducible
for the signal named; on a different take, expect the shape to hold and the
numbers to move.

One target is missed: 150 Hz – 600 Hz and 600 Hz – 2.5 kHz come out around
flat where the reference has them near −1 dB. Getting them down there means
either more compression, which costs the crest factor the reference also wants,
or a static shelf, which is a tone control this plugin does not have. Flat was
the better of the two.

## Controls

| | |
|---|---|
| **INPUT** | How hard the signal arrives at the curve. The curve is level-dependent, so this is a second drive control in everything but name. |
| **DRIVE** | Saturation intensity, 0–100 %. The character does not change across the range — only how much of it there is. |
| **TONE** | The fitted voicing: a bell around 7 kHz, a high-pass at the bottom, scaled from nothing to the reference's own curve. At 0 it is out of circuit entirely, filter and all. |
| **MIX** | Wet/dry, delay-matched so a partial blend cannot comb. |
| **OUTPUT** | Level. |
| **SAT** | Takes the saturation out of circuit. A true null. |
| **Ø** | Polarity. |
| **AUTO** | Static level match, so Drive can be judged on tone rather than loudness. |

No numeric readouts, by design — a plus, a minus where there is something to
subtract, and nothing else. Nothing on the panel adjusts the asymmetry or the
frequency weighting: those are the plugin rather than settings of it.

Oversampling is a parameter (Off / 2x / 4x / HQ 8x) and reports its latency
honestly. **Off by default**, which is zero latency: the curve is anti-aliased
by ADAA rather than by rate, so folded images sit at −51 dB with no
oversampling at all, −88 dB at 2x and −103 dB at 4x. The suite's rule is that
every module reports zero latency in its default state.

Past about Drive 55 the balance tips and odd harmonics lead even: it stops
being a warm saturator and becomes an overdrive. That is the top half of the
range doing what the top half of a range should, not a defect, but the
character the plugin was fitted to lives in the bottom half.

## Presets

Every factory preset measures within half a decibel of unity gain on a voice at
a working level, and a test holds them there — a preset that arrives louder
gets credit for the loudness, which is the oldest way there is to make a change
sound like an improvement.

Presets are plain files in a folder you can open:
`~/Library/Audio/Presets/LT3 Audio/BMO Saturator`. Sharing one is sending a
file.

## Building

```bash
git clone --recursive https://github.com/kevkloud/bmo-saturator.git
cd bmo-saturator
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --parallel
```

A development build installs itself into `~/Library/Audio/Plug-Ins`;
`./scripts/build.sh` builds and then verifies that it actually landed, which
Live will silently prevent if it is running.

The DSP core has no JUCE dependency, so everything the plugin claims about
itself can be built and checked with no framework at all:

```bash
cmake -B build-dsp -DBMO_DSP_ONLY=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-dsp --parallel
ctest --test-dir build-dsp --output-on-failure
```

## For testers

`docs/TEST_PLAN.md` is the listening plan and the honest list of known gaps,
written for someone who does not read code — seven tests in order, what to
report, and what is already known so nobody spends their ears finding it
again. It ships alongside the binaries.

## Verifying by ear

The metrics are a strong proxy and not a substitute. To check it properly:

1. Bounce the reference vocal through the plugin at the default, Auto Gain on.
2. `./build/measure verify that-bounce.wav` for the numbers.
3. Listen against the Fuji reference file. What to listen for is whether it
   reads as *warm* rather than *bright* — the failure mode this was specified
   against measures nearly the same up top and sounds harsh.

## Licence

AGPLv3 — see `LICENSE`. JUCE is used under the AGPLv3 branch of its dual
licence. The VST3 SDK (bundled inside JUCE) is MIT-licensed.

Unlike FrostyEQ, no font files are embedded: the panel asks the system for a
sans face. That keeps an unsettled font-redistribution question out of a public
AGPL repository, at the cost of the panel not being glyph-identical across
platforms. `src/gui/Theme.cpp` is the one file that changes if the suite settles
that question.
