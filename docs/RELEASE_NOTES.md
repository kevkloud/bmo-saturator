**First build, for feedback.** A saturator fitted to measured targets from a
real vocal take rather than to a circuit — asymmetric curve, harmonics placed
in the 2.5–18 kHz octaves, everything below within a decibel of where it
started, and no compressor or limiter anywhere in it.

## Downloads

| | contains |
|---|---|
| **BMO-Saturator-macOS.zip** | VST3, AU, and a standalone app. Universal — Apple Silicon and Intel. |
| **BMO-Saturator-Windows.zip** | VST3, 64-bit. |

Neither is code-signed, so **both operating systems will complain the first
time.** `INSTALL.md` inside the zip has the two clicks that get past it —
System Settings → Privacy & Security → Open Anyway on macOS, More info → Run
anyway on Windows. Nothing is being hidden from you; a signing certificate is
just something nobody has bought yet.

## Where to start

Load it on a vocal, leave **DRIVE** at 40 %, and turn **AUTO** on so you are
comparing tone rather than loudness. That default is the calibration point: it
is the exact drive at which the curve reproduces the reference's measured
asymmetry.

The factory presets are starting points, all level-matched. **Reference** is
the calibration point itself; **Vocal Sheen** and **Vocal Front** are the two
directions from it; **Ruined** is the far end and is meant to be.

## What to listen for, and what to report

This was built to measurements, and the measurements say it is right. Nobody
has confirmed it by ear against the reference material, which is the only test
that finally matters — that is what this build is for.

The specific thing worth your ears: it should read **warm**, not merely
**bright**. The failure mode it was designed against measures nearly the same
amount of high end and sounds harsh, because it has the brightness without the
even-order harmonic content underneath. If this one sounds sizzly, edgy, or
thin on real material, that is the single most useful thing you can tell us,
along with the source and the Drive setting.

Also worth flagging: anything above **DRIVE 55**, where the balance tips from
even-order to odd and it stops being a warm saturator and becomes an overdrive.
That is intended, but whether the crossover sits in the right place is a
judgement nobody has made by ear yet.

## Known and deliberate

- **Not signed or notarised.** See above.
- **Two target bands are missed.** 150 Hz – 600 Hz and 600 Hz – 2.5 kHz come
  out around flat rather than a decibel down. Closing that costs either the
  dynamics behaviour the same spec asks for or a hidden tone control; flat was
  judged the better miss. Details in `docs/plan.md`.
- **The panel's typeface is the system sans**, not the suite's display face,
  while a font-licensing question is settled. The layout is final; the
  letterforms are not.
- **No presets folder is created until you save one.**

Full measurements, the curve, and three findings about the specification
itself: <https://github.com/kevkloud/bmo-saturator#readme>
