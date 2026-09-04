**Second build, from Frosty's Test 1 report and the three files that came with
it.** The tonal miss he measured was real; this fixes it, and the fix turned
out not to be the waveshaper.

## What changed

**Fitted to the reference files themselves.** 0.1.0 had been fitted to a
synthetic test signal carrying 22 dB less energy above 6 kHz than the actual
vocal, so the same processing measured +8 dB there and +0.4 dB on the real
take. Measured on the real files now:

| band | target | 0.2.0 | 0.1.0 |
|---|---|---|---|
| 20–150 Hz | −1.22 dB | −0.59 | −0.13 |
| 150–600 Hz | −1.19 dB | −1.18 | +0.32 |
| 600 Hz–2.5 kHz | −0.80 dB | −0.98 | −1.47 |
| **2.5–6 kHz** | **+6.56 dB** | **+6.15** | −0.99 |
| **6–18 kHz** | **+8.51 dB** | **+8.13** | +0.35 |

**There is a new control, TONE, and it is an equaliser.** Fitting the best
possible linear filter from the dry file to the Fuji file explains 97 % of what
Fuji does — a bell of about +10 dB at 7 kHz. Those band lifts are not harmonic
generation, and no waveshaper reaches them. TONE carries that voicing, on the
panel with a name rather than hidden inside the curve, and at 0 it is out of
circuit entirely. Whether the plugin should carry an EQ at all is the biggest
open question in this build — Test 6 in `TEST PLAN.md` is about exactly that.

**The scratching on DRIVE is fixed.** It was worse than reported: single
samples over thirty times full scale, five thousand times the largest step the
music itself was making. Parameter smoothing was already there and could not
have helped — the smoother is what supplied the changing value; the fault was
in the anti-aliasing state not being rebuilt when the drive moved. There is now
a test that sweeps the control at three speeds and checks the audio, rather
than checking that smoothing code exists.

**Oversampling is Off by default**, at zero reported latency, as asked. The
curve is anti-aliased by its own maths rather than by rate, so folded images
sit at −51 dB with oversampling off, −88 dB at 2x, −103 dB at 4x.

## Downloads

| | contains |
|---|---|
| **BMO-Saturator-macOS.zip** | VST3, AU, and a standalone app. Universal — Apple Silicon and Intel. |
| **BMO-Saturator-Windows.zip** | VST3 and a standalone, 64-bit. |

Neither is code-signed, so both operating systems will complain the first time.
`INSTALL.md` inside the zip has the two clicks that get past it.

## Where to start

Preset **Reference** — Drive 40, Tone 100, Auto Gain on. That is the setting
the fit was made at, and the one that matches the measurements above.

Then Test 1 and Test 6 in `TEST PLAN.md`: does it land in the same place as the
Fuji file by ear, and is the plugin better with the voicing or without it.

## Still open

- **Crest factor opens up more than the reference** — +3.32 dB against +1.67.
  Both go the right way; this one is livelier. Pulling TONE back reduces it.
- **The crest factor result in the Test 1 report does not reproduce here.**
  Every render raises it at every setting, and that bounce does not null against
  either the dry file or a local render of it — so something else may have been
  in the chain. Worth checking.
- **The asymmetry figures in the original brief are not in the files.** Measured
  on the actual pair, Fuji's average gains are 0.965 / 1.015 — an asymmetry of
  0.05, not 0.22. Every other figure reproduces exactly.
- **Still not validated by ear.** By anyone. That is what this build is for.

Full write-up, including how each constant was fitted:
<https://github.com/kevkloud/bmo-saturator#readme>
