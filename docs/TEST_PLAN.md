# BMO Saturator 0.3.0 — test plan and known gaps

**For Frosty.** This is what to listen for, in what order, and what to send
back. The second half is an honest list of what is wrong or unsettled, so you
are not spending your ears finding things that are already known.

**What changed in 0.3.0, from your 0.2.0 tracker:**

- **You found one bug three times, and you were right.** Drive 40 overdriven,
  crossover at 25–30 not 55, and the bottom third of the knob holding all the
  usable colour are the same finding: the range was about three times too hot.
  The whole scale is divided by 2.75 — the ratio your ear asked for. Drive 40
  now does what Drive 15 did, the crossover should land near the middle of the
  travel, and no preset number changed. Please re-check Test 3: the prediction
  this time is that the crossover sits at **45–55**, and if it does not, the
  same one-line fix applies again.
- **AUTO really did do nothing.** It was a fixed table fitted to one voice at
  one level; measured on anything else it moved the level by a decibel or two
  and at high drive pulled the wrong way. It is now a proper detector and holds
  within 0.05 dB. It is slow on purpose — 1.5 seconds, far slower than a
  phrase — so it cannot squash anything, and there is a test that fails if
  someone speeds it up.
- **The bands got closer at the quieter setting**, because the voicing carries
  them and the saturation no longer has to. Crest factor improved too: +2.28
  against the reference's +1.67, from +3.32.
- **Your bypass delta was oversampling, not a bug.** With SAT off at the
  default the output is bit-identical to the input — zero difference. With
  oversampling on, the anti-imaging filters leave a −62 dBFS residual, which is
  what resampling does. 0.1.0 defaulted to 2x, which is what you measured.
- **TONE stays**, on your verdict.

**What changed in 0.2.0, from your Test 1 report:**

- **Your numbers were right and 0.1.0 genuinely missed.** Rendering your dry
  file through the old build reproduces your result exactly. The cause was not
  the waveshaper: it was that the plugin had been fitted to a synthetic test
  signal carrying 22 dB less energy above 6 kHz than your actual vocal, so the
  same processing measured +8 dB there and +0.4 dB on the real thing.
- **The reference is mostly an equaliser.** Fitting the best possible linear
  filter from your dry file to the Fuji file explains 97 % of it — a bell of
  about +10 dB at 7 kHz. Those +6 and +8 dB band lifts are not harmonic
  generation and no waveshaper reaches them. There is now a **TONE** control
  carrying that voicing, on the panel with a name rather than hidden in the
  curve. All five bands now land within half a decibel of the target.
- **The scratching was real and worse than reported** — over thirty times full
  scale, from ADAA state not being rebuilt when the drive changed. Fixed, with
  a test that sweeps the control at three speeds and checks the audio.
- **Oversampling is now Off by default**, at zero latency, as you asked.
- **Your crest factor result does not reproduce here.** See gap 4.5.

**You can paste this whole file into Claude and ask it anything** — "what does
crest factor mean here", "why does the asymmetry number matter", "explain gap
3 to me like I don't do maths". It is written to be self-contained, so Claude
does not need the code to answer. If you want it to go deeper, the source is
in `source-code.zip` next to this file, and `docs/plan.md` inside that has the
full engineering write-up.

---

## 1. What this plugin is claiming

You gave a target: the before-and-after measurements of a vocal you liked
("Fuji"), and a second example you did not like ("Preesh BG") as a thing to
avoid. Measured on your own files this time, rather than on a stand-in:

| | target | 0.3.0 | 0.2.0 | 0.1.0 |
|---|---|---|---|---|
| 20–150 Hz | −1.22 dB | −1.43 | −0.59 | −0.13 |
| 150–600 Hz | −1.19 dB | −0.91 | −1.18 | +0.32 |
| 600 Hz–2.5 kHz | −0.80 dB | −0.96 | −0.98 | −1.47 |
| **2.5–6 kHz** | **+6.56 dB** | **+6.56** | +6.15 | −0.99 |
| **6–18 kHz** | **+8.51 dB** | **+8.07** | +8.13 | +0.35 |
| crest factor | +1.67 dB | +2.28 | +3.32 | −1.84 |

Every band within half a decibel, now at a drive setting your ear picked rather
than one the fit picked.

**Nobody has heard it against your reference file.** That is still the entire
purpose of this build.

The one sentence that matters: **it should sound warm, not merely bright.** The
example you disliked measured nearly the same amount of high end and sounded
harsh — the difference is what is underneath the brightness, not the brightness
itself. If this build sounds bright-but-thin, sizzly, or brittle, the design
missed, and no measurement here will have caught it.

---

## 2. The tests, in order

Do them in this order. Each one takes a few minutes. **Turn AUTO on for every
listening test** unless a step says otherwise — it level-matches, and louder
always sounds better for the first ten seconds.

### Test 1 — Against the reference (the important one, again)

1. Take the same vocal the Fuji reference came from, dry.
2. Load BMO Saturator, preset **Reference** (DRIVE 40, AUTO on).
3. A/B against the Fuji processed file.

**Report:** does it land in the same place? If not, is it brighter or duller,
harder or softer, more or less "in front"? Anything you notice is useful, even
"it's close but the S sounds are sharper".

### Test 2 — Warm or harsh

On two or three different vocals — one bright, one dull, one already a bit
sibilant.

1. Preset **Vocal Sheen** (DRIVE 34), AUTO on. Toggle **SAT** to compare.
2. Then **Vocal Front** (DRIVE 52).

**Report:** at what point, on which source, does it start to sound harsh rather
than warm? A DRIVE number and a source description is exactly the right amount
of detail.

### Test 3 — The crossover point

The character deliberately changes as DRIVE goes up: it is a warm saturator in
the bottom half of the range and becomes an overdrive in the top half. The
maths says that tips at about **DRIVE 55**.

1. One source, AUTO on. Sweep DRIVE from 20 to 100, slowly.
2. Note where it stops being "colour" and starts being "distortion".

**Report:** the number. Last time you said 25–30 and the fix was to divide the
whole scale by 2.75. The prediction now is **45–55**. If it still comes out
low, say so and it gets divided again — this is the single most actionable
thing you can report, and it worked exactly as intended the first time.

### Test 4 — The bottom of the range

**Report:** does DRIVE 0–20 do anything useful, or is it wasted travel? It is
deliberately almost nothing at the bottom, so the control has somewhere to come
from — but if the first third of the knob is dead in practice, the mapping
should change.

### Test 5 — Things that are not vocals

Drums (**Drum Bus Glue**), bass (**Bass Warmth**), a full mix (**Mix Bus
Colour**), guitars (**Guitar Grit**).

**Report:** anything that falls apart. This was fitted to a voice, so a source
that behaves differently from a voice is where it is most likely to be wrong —
particularly anything with a lot of natural high end already, like cymbals or
acoustic guitar.

### Test 6 — TONE, the new control

TONE is the voicing: the 7.5 kHz bell that makes the band numbers match. It is
the part of this that is an equaliser rather than a saturator.

1. On a vocal, sweep TONE from 0 to 100 with DRIVE at 40.
2. At 0, listen to what the saturation alone does.

**Report:** whether the plugin is better with the voicing or without it, and
whether 100 % is too much. This is the biggest open design question — see gap
4.6 — and your ear settles it, not mine.

### Test 7 — Does it stay out of the way

1. **SAT** off should be a perfect bypass. Any change in level or tone with it
   off is a bug.
2. **MIX** at 0 should be the dry signal, exactly. Any phasey, comb-filtered
   quality is a bug.
3. Automate DRIVE across a phrase. Any clicks, zips or thumps are bugs.
4. Leave it on a track and play for ten minutes. Any crackle that appears over
   time is a bug.

### Test 8 — Does it behave as software

Load it, save the project, close everything, reopen. Settings should come back
exactly. Try in more than one host if you can. Any crash, hang, or "plugin not
found" is worth reporting immediately — that class of problem is much cheaper
to fix now than after people have projects saved with it.

---

## 3. What to send back

Most useful, in order:

1. **"It sounds X on Y at DRIVE Z"** — the one thing measurements cannot
   provide.
2. Where the character crosses from warm to overdriven (Test 3).
3. Any source it does not suit.
4. Whether the plugin should carry an EQ at all (Test 6).
5. Bugs from Tests 7 and 8.
6. Anything about the panel — layout, sizes, whether DRIVE reads as the main
   control at a glance.

Bounced examples are gold if they are easy: dry and processed, level-matched,
same settings stated. There is a measurement tool in the source that can be
pointed at a bounce and will report all four numbers for that specific file.

---

## 4. Known gaps

Things already found. **You do not need to look for these** — but if any of
them bothers you in practice, say so, because that changes the priority.

### 4.1 Crest factor opens up more than the reference

The reference's transients open up by 1.67 dB; this build gives 3.32 dB. Both
go the right way — nothing is being squashed — but this is livelier than Fuji.
The cause is the voicing lifting transient high end. Pulling TONE back reduces
it, at the cost of the band match. If it sounds spiky or ticky on consonants,
that is this, and it is worth telling me.

### 4.2 The asymmetry measurement may have misjudged Preesh BG

This is a finding about the *specification*, not the plugin, and it is worth
your attention because it might change what "avoid this" means.

The asymmetry number in the brief is measured by splitting the waveform into
its positive and negative halves and comparing. The problem: that measurement
is mostly detecting a **DC offset** — a fixed shift in the whole waveform that
lopsided distortion leaves behind. Nearly every plugin removes that offset
before the audio leaves it, because it wastes headroom and causes thumps. This
one removes it too.

Consequence: **any well-behaved saturator measures near-zero on that test**,
however lopsided its distortion actually is. This build measures 0.220 at the
curve itself and 0.062 by the time the audio comes out.

So Preesh BG's 0.004 does not prove its distortion was symmetric. It may just
prove it removed DC, like everything else does. Whatever made it sound harsh,
that number probably was not it.

**What would settle it:** measuring even-vs-odd harmonic content on both
reference files directly. That is a ten-minute job and it either confirms the
diagnosis or replaces it. Worth doing before the next plugin is specified this
way.

### 4.3 Two completely different-sounding curves fit the same numbers

Two candidate designs were built. Both matched 0.62 / 0.84 to three decimal
places. One puts its second harmonic 4 dB *below* its third — a mostly-edgy
distortion, which is the character the brief was warning against. The other
puts it 6 dB above. **Same numbers, opposite sound.** The second one shipped.

The lesson for future briefs: the asymmetry figure alone does not pin down what
you are asking for. A "second harmonic should lead third by roughly N dB" line
would.

### 4.4 The dB figures depend on the source as much as the plugin

"+8 dB at 6–18 kHz" partly means "the source had very little up there". The
same processing measures +8.2 dB on a voice-like test signal and about +3 dB on
something with more natural top end. Nothing is wrong; it just means comparing
this build's numbers against a figure from a different take is really comparing
the takes.

### 4.5 Your crest factor measurement does not reproduce here

You measured 18.68 dB against a dry 20.52 — peaks squashed. Every render here
raises the crest factor at every Drive and Input setting, and your bounce does
not null against either the dry file or a local render of it at any setting,
which means it contains something this plugin does not produce.

Two candidates: something else was on the chain or the master when you bounced,
or the file predates a setting change. Worth checking before treating it as a
plugin bug — if it turns out the bounce was clean, there is a real problem here
that I have not found.

### 4.6 The plugin is now two things, and one of them is an EQ

TONE is a 7.5 kHz bell of up to +8 dB with a high-pass under it. That is an
equaliser, and it is doing most of the work that the band targets measure. It
is on the panel and defeatable precisely so that is visible rather than hidden.

Worth deciding, and it is your call: is a saturator that carries a fixed EQ
voicing what you want, or would you rather the plugin only ever generate
harmonics and leave the tone shaping to BMO EQ next to it? The second is
purer and will never match the Fuji numbers. The first matches, and is what
most "character" plugins actually are under the hood.

### 4.7 The asymmetry figure in the brief is not in the files

Measured on your dry/processed pair the way the brief describes, Fuji's
positive and negative average gains are 0.965 and 1.015 — an asymmetry of 0.05,
not 0.22. Every other figure in the brief reproduces to a tenth of a decibel,
so this is not a disagreement about method. Wherever 0.62 / 0.84 came from, it
was not this pair.

### 4.8 Not code-signed

macOS and Windows will both warn on first open. The read-me has the two clicks.
Removing the macOS warning properly costs $99/year for an Apple developer
account — worth deciding before this goes to anyone who will not follow
instructions.

### 4.9 The typeface is not the suite's

The panel uses the system sans instead of the suite's display faces, because
embedding those in a public open-source repo is a licensing question that is
not settled. **Layout, sizes and spacing are final; the letterforms are not.**
It will look slightly different from FrostyEQ, and that gets fixed when the
font question is answered for the whole suite.

### 4.10 The default preset is not level-matched

**Init** — what you get when the plugin first loads — comes out about 1.6 dB
quieter than the input, because AUTO is off by default. Every other preset is
matched. AUTO defaults to off on purpose (a level match is a decision about how
you are auditioning, not about how it should sound), but if that trips you up
in practice it can be changed.

### 4.11 Nothing has been validated by ear. At all.

Including the presets, which were checked against measurements and never
listened to. Treat every one of them as somewhere to start, not somewhere to
stop.

---

## 5. What is already locked

So you know what feedback is cheap and what is expensive:

**Cheap to change** — panel layout, colours, the DRIVE range and how the
control maps, preset values, how much sheen, where the character crosses over.
Any of that can move on your say-so.

**Expensive** — the parameter list, their ranges, and the plugin's identity.
Those are frozen because sessions saved with this build reference them; changing
one silently breaks anyone's saved project. There is a test that fails the build
if someone tries.

**Free to change until 1.0, expensive after** — the sound itself. Before 1.0 it
can be adjusted freely. After, a change that alters someone's finished mix has
to ship as a separate plugin rather than as an update.

So: strong opinions now are cheap. Strong opinions after 1.0 are not.
