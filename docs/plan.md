# BMO Saturator — how it was fitted, and what is frozen

Working notes for Kevin and Frosty. The README says what the plugin is; this
says how the numbers were arrived at, which of them may never change, and what
the specification got wrong.

---

## 1. The specification

Frosty's brief gave four measurements taken from before-and-after analysis of a
real vocal take ("Fuji"), and four from a pass judged harsh by ear ("Preesh
BG") as an explicit anti-target:

| | Fuji | Preesh BG |
|---|---|---|
| positive-half average gain | 0.62 | 0.949 |
| negative-half average gain | 0.84 | 0.946 |
| asymmetry | 0.22 | 0.004 |
| 20–150 Hz | −1.1 dB | −1.1 to −0.45 dB |
| 150–600 Hz | −1.1 dB | " |
| 600–2500 Hz | −0.7 dB | " |
| 2.5–6 kHz | +6.25 dB | " |
| 6–18 kHz | +8.25 dB | +1.85 dB |
| crest factor | +1.7 dB | — |

Both sets live in `tools/measure/main.cpp` as constants, and `measure verify`
reports against both. The anti-target is checked, not merely described: the
harness warns if the asymmetry falls near zero, if the lift is confined to the
top band, or if the crest factor drops.

## 2. What was fitted, and to what

Everything that decides the sound is in `DspCore::Character` and
`DriveTables.h`, and every value in them was fitted numerically rather than
chosen. `measure fitfile dry.wav target.wav` re-derives the lot against a real
before/after pair; `measure fit` does the curve alone.

| constant | 0.2.0 value | fitted to |
|---|---|---|
| `AsymmetricShaper::kBias` | 0.263 | the curve's average gains, 0.62 / 0.84 |
| curve drive at Drive 40 % | 4.00 | band deltas on the reference pair |
| `bellHz` / `bellQ` / `bellGainDb` | 7500 Hz, 0.90, +8 dB | the +6.5 and +8.5 dB band lifts |
| `highPassHz` | 50 Hz | the reference's bottom-end roll-off |
| `bodySourceHz` / `bodyGain` | 600 Hz, +3.0 | 600 Hz – 2.5 kHz near flat |
| `residualSplitHz` | 2400 Hz | where the target's flat bands end |
| `sheenSourceHz` / `sheenGain` | 6000 Hz, 4.75 | 2.5–6 kHz |
| `sheenHz` / `sheenTilt` | 6000 Hz, 1.30 | 6–18 kHz above 2.5–6 kHz |
| `tables::kMakeupDb` | measured curve | `measure makeup` |

`bodyGain`'s sign is load-bearing and had to be re-fitted when the voicing
arrived: it was −3.0 when the plugin was all waveshaper and is +3.0 now. The
body generator's harmonics land where the programme already has harmonics of
its own, so they either reinforce or cancel depending on which way round they
are added. Whenever a generator is fed a band the source already occupies, that
sign is a measurement, not a decision.

## 2a. What the reference files showed (0.2.0)

Frosty ran Test 1 and sent the three files: the dry vocal, the Fuji processed
version, and a bounce through BMO 0.1.0. Measuring them settled several things
at once.

**The harness agrees with his measurements.** Running the dry/target pair
through `measure compare` reproduces the brief's band figures to within a
tenth of a decibel (−1.22 / −1.19 / −0.80 / +6.56 / +8.51, crest +1.67). So
the measurement method is not in dispute.

**0.1.0 genuinely missed, and his numbers are right.** Rendering his dry file
through the shipped build reproduces his result: no high-band lift at any Drive
or Input setting. Not a knob problem.

**The cause was the test signal, not the curve.** The plugin had been fitted to
the harness's synthetic voice, which carries 22 dB less energy above 6 kHz than
the real take. A band delta is as much a statement about what the source
already had in that band as about what the process added, so the same harmonic
generation measured +8 dB on the synthetic and +0.4 dB on the real thing. Gap
4.4 of the test plan predicted exactly this failure and was then walked into.

**The reference is mostly an equaliser.** Fitting the best linear time-invariant
filter from dry to processed explains 97 % of Fuji: a bell of about +10 dB at
7 kHz, 2 dB of broadband trim, a high-pass at the bottom. The band lifts in the
brief are that bell. No waveshaper produces them, which is why no amount of
adjustment to the curve reached them.

**His crest factor result does not reproduce.** He measured 18.68 dB against a
dry 20.52. Every render here raises the crest factor at every setting; his
bounce lowers it, and it does not null against either the dry file or a local
render at any setting. Something else was in that chain — worth asking before
treating it as a plugin bug.

**The zipper noise was real, and worse than reported.** Changing Drive while
audio flowed produced single samples over thirty times full scale, five
thousand times the largest step the programme was making. Cause: ADAA carries
the antiderivative of the previous sample across calls, and changing the drive
without rebuilding that state subtracts two different functions and divides by
a possibly tiny dx. Smoothing was already present and could not have helped --
the smoother is what delivered the changing value. Fixed in
`AsymmetricShaper::setDrive`, with a test that sweeps the control at three
speeds and checks the output for steps rather than checking that smoothing code
exists.

### What changed in 0.2.0

| | 0.1.0 | 0.2.0 |
|---|---|---|
| Fitted against | synthetic voice | the reference files themselves |
| Voicing | none | TONE: 7.5 kHz bell, +8 dB, high-pass at 50 Hz |
| Curve drive at Drive 40 | 3.62 | 4.00 |
| sheenGain / sheenTilt | 4.75 / 1.25 | 4.75 / 1.30 |
| bodyGain | −3.0 | +3.0 |
| Oversampling default | 2x | Off (zero latency) |
| Drive changes | 32× full-scale spike | no step larger than the programme's |

## 3. Four things the specification got wrong or under-determined

Worth Frosty's time; the first two change how the two references should be
compared.

**3.0 The asymmetry figures are not reproducible from the files.** Measured on
the actual pair, the way the brief describes, Fuji's average gains are 0.965
and 1.015 — an asymmetry of 0.05 against the stated 0.22. Every other figure in
the brief reproduces exactly, so this is not a measurement-method disagreement;
it is one number that does not come from these files.

**3.1 The asymmetry metric is mostly a DC-offset metric.** An asymmetric curve
moves the mean of its output. Split by polarity and compare average gains and
that offset dominates the answer. A DC-blocked chain measures near zero on this
metric no matter how asymmetric its curve is — this plugin measures 0.220 at
the curve and 0.062 end to end, and the difference is entirely the DC blocker.

**3.2 So Preesh BG's 0.004 may say nothing about its curve.** That figure is
what any DC-blocked saturator measures. If the two references differ in DC
handling — which nothing in the brief rules out — then this number is comparing
their output stages, not their curves, and the conclusion that Preesh BG's
shaping was "essentially a flat linear trim" does not follow from it. The
measurement that does separate them is even-order against odd-order harmonic
content: `measure harmonics`. Fuji's description (warmth, second and fourth)
implies even-dominant; harsh and sizzly implies odd-dominant. That is testable
directly on both source files, and it would be worth doing before treating the
0.004 as established.

**3.3 The average gains do not determine the shape.** Two curve families were
built here. Both fit 0.62 / 0.84 to three decimal places:

- piecewise drive (each polarity gets its own drive): second harmonic ~4 dB
  **below** third — a mostly-odd distortion, which is the character the brief
  warns against;
- fixed offset (what shipped): second harmonic ~6 dB **above** third.

Same asymmetry on paper, opposite sound. Any future spec of this kind should
carry a harmonic-order figure alongside the average gains, or it does not pin
down what it is asking for.

**3.4 Band deltas are a property of the source as much as the process.** A
+8 dB lift at 6–18 kHz means "the source had little up there". The same
processing measures +8.24 dB on the harness's reference voice and about +3 dB
on a bare harmonic series with more native top end. The plugin's numbers are
reproducible for a named signal; comparing them to a figure from a different
take is comparing the takes.

## 4. The target that is missed

150 Hz – 600 Hz and 600 Hz – 2.5 kHz come out around flat, against −1.1 and
−0.7 dB. Reaching those needs either more compression — which costs the crest
factor increase the same specification asks for — or a static shelf, which is a
tone control this plugin deliberately does not have. Flat was judged the better
miss. If Frosty's ear disagrees, the honest fix is a shelf, and it should then
be on the panel rather than hidden.

## 5. Frozen

Final as of the first public binary. Everything here is permanent in the sense
of §8.4 of the suite plan: changing any of it breaks existing sessions,
silently.

1. **Identity.** Product name `BMO Saturator`, plugin code `Bsat`, manufacturer
   code `LT3a`, bundle ID `com.lt3audio.bmosaturator`, company `LT3 Audio`.
   The manufacturer code is shared with the rest of the suite and is an input
   to the VST3 and AU class IDs; changing it orphans every session that
   references any LT3 Audio plugin.
2. **Parameter schema.** Eight parameters, their IDs, order, ranges, step
   counts and defaults, as written out in `tests/ParameterTests.cpp`. Ranges
   are as permanent as IDs: stored automation is normalised, so widening Drive
   from 0–100 silently rescales every automation point ever written.
3. **Preset format.** Extension `.bmosat`, folder
   `~/Library/Audio/Presets/LT3 Audio/BMO Saturator`, state root `PARAMS`,
   `stateVersion` property.
4. **Sound.** Free to change before 1.0. After 1.0, per §8.6 of the suite plan,
   a change that alters existing mixes ships as a new product with a new plugin
   code rather than as an update.

## 6. Open

- **Ear check against the reference file.** Nobody has done it. Everything
  above is a proxy.
- **Harmonic-order analysis of both reference files** (§3.2). Cheap, and it
  settles whether the anti-target was diagnosed correctly.
- **Fonts.** The suite's display faces are not embedded here; see the note at
  the end of the README and `src/gui/Theme.cpp`. When the licensing question is
  settled for the suite, this plugin adopts whatever the answer is.
- **Suite integration.** This is built as a standalone repo in the shape
  FrostyEQ established, not against the shared `bmo-core` libraries, because
  those do not exist yet (M1 in the suite plan). Roughly 40 % of `src/gui` and
  `src/presets` here is FrostyEQ's code with the namespace changed, which is
  the duplication M1 exists to remove. When it happens, this is one of the
  repos that folds in.
