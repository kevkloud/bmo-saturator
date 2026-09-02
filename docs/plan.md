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

Two constants carry the character. Both were fitted numerically against the
reference figures rather than chosen, and `measure fit` re-derives them:

- `AsymmetricShaper::kBias = 0.263` — the offset, in the driven domain.
- `DRIVE 40 %` maps to a curve drive of 3.62, via `kDriveMin` / `kDriveMax` in
  `dsp/DriveTables.h`.

At that pair, over the harness's reference voice at −18 dBFS RMS, the curve's
average gains are 0.620 and 0.840, an asymmetry of 0.220. The targets are 0.62,
0.84, 0.22.

The remaining constants shape where the harmonics land, and were fitted to the
band deltas at the same drive:

| constant | value | fitted to |
|---|---|---|
| `kBodySourceHz` / `kBodyGain` | 600 Hz, −3.0 | 600 Hz – 2.5 kHz near flat |
| `kResidualSplitHz` | 2400 Hz | where the target's flat bands end |
| `kSheenSourceHz` / `kSheenGain` | 6000 Hz, 4.75 | +6.25 dB at 2.5–6 kHz |
| `kSheenHz` / `kSheenTilt` | 6000 Hz, 1.25 | +8.25 dB at 6–18 kHz |
| `tables::kMakeupDb` | measured curve | `measure makeup` |

`kBodyGain` is negative, and that is not a typo. The generator's residual in
600 Hz – 2.5 kHz is in antiphase with the programme's own harmonics there, so
added in the obvious polarity it *cancels* them and the band measured 3 dB down
rather than flat. Inverted, it reinforces. Polarity matters whenever a
harmonic generator is fed a band the source already occupies.

## 3. Four things the specification got wrong or under-determined

Worth Frosty's time; the first two change how the two references should be
compared.

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
