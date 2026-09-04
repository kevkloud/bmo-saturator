**Third build, from your 0.2.0 test tracker.** Everything in it was actionable
and one finding turned up three times.

## What changed

**The drive range was about three times too hot — you found it three ways.**
Drive 40 overdriven (Test 1), the audible crossover at 25–30 rather than the
predicted 55 (Test 3), and the bottom third of the knob holding all the usable
colour when it was documented as near-dead travel (Test 4). One finding. The
whole scale is now divided by 2.75, the ratio your ear asked for:

- **Drive 40 now does what Drive 15 did.**
- The crossover from colour to overdrive should land near **45–55**. Test 3
  again, please — if it still comes out low, the same one-line fix applies.
- No preset number changed. Shifting both ends of the mapping rescales every
  position on the knob at once, so the presets moved with it.

Re-fitting the voicing at the quieter setting puts the bands *closer* to the
reference than 0.2.0 managed, and improves the dynamics figure:

| band | target | 0.3.0 | 0.2.0 |
|---|---|---|---|
| 20–150 Hz | −1.22 dB | −1.43 | −0.59 |
| 150–600 Hz | −1.19 dB | −0.91 | −1.18 |
| 600 Hz–2.5 kHz | −0.80 dB | −0.96 | −0.98 |
| **2.5–6 kHz** | **+6.56 dB** | **+6.56** | +6.15 |
| **6–18 kHz** | **+8.51 dB** | **+8.07** | +8.13 |
| crest factor | +1.67 dB | **+2.28** | +3.32 |

**AUTO really did do nothing — fixed properly.** It was a fixed table fitted to
one voice at one level, so on anything else it moved the level by a decibel or
two, and at Drive 60 and 100 it pulled the wrong way. It is now a real
detector: input energy against output energy, and it holds the level within
0.05 dB on material it has never seen.

It is slow on purpose — a 1.5 second time constant, far slower than any
phrase — so it moves the level without touching the dynamics. There is a test
that fails if anyone speeds it up, because a fast one would make this a
compressor, which is the one thing the brief says it must not be.

**Your bypass delta was oversampling, not a bug.** With SAT off at the default
the output is bit-identical to the input — zero difference, measured. With
oversampling on, the anti-imaging filters leave a −62 dBFS residual, which is
what resampling does anywhere. 0.1.0 defaulted to 2x, which is what you had.

**TONE stays**, on your verdict — described as shifting the saturator's
apparent centre frequency, and fine at 100 % once the drive is in range.

## Downloads

| | contains |
|---|---|
| **BMO-Saturator-macOS.zip** | VST3, AU, and a standalone app. Universal — Apple Silicon and Intel. |
| **BMO-Saturator-Windows.zip** | VST3 and a standalone, 64-bit. |

Neither is code-signed; `INSTALL.md` inside the zip has the two clicks past it.

## Where to start

**Test 3 first this time.** Sweep DRIVE on one source with AUTO on and say
where colour becomes distortion. The prediction is 45–55. That one number
decides whether the scale is now right.

Then Test 1 again — Reference preset, A/B against the Fuji file — and Test 2
across the presets, which should no longer need the drive pulled back.

## Still open

- **Crest factor still runs a little hot**: +2.28 against the reference's
  +1.67, down from +3.32. Better, not matched.
- **The asymmetry figures in the original brief are not in the reference
  files.** Measured on the actual pair, Fuji's average gains are 0.965 / 1.015
  — an asymmetry of 0.05, not the 0.22 in the brief. Every other figure
  reproduces exactly. The test that used to pin those numbers now checks the
  shape of the curve instead.
- **Still not validated by ear against the Fuji file itself.** Test 1 is the
  one that closes this.

<https://github.com/kevkloud/bmo-saturator#readme>
