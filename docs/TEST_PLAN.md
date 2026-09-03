# BMO Saturator 0.1.0 — test plan and known gaps

**For Frosty.** This is what to listen for, in what order, and what to send
back. The second half is an honest list of what is wrong or unsettled, so you
are not spending your ears finding things that are already known.

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
avoid. Four numbers were fitted:

| | what it means | target | what this build measures |
|---|---|---|---|
| **Asymmetry, 0.22** | how lopsided the distortion is, which is what makes it warm rather than edgy | 0.62 / 0.84 | 0.620 / 0.840 |
| **Where the new energy lands** | 2.5–18 kHz lifts 6–8 dB, everything below stays put | +6.25 / +8.25 dB | +5.73 / +8.24 dB |
| **Dynamics** | transients survive and get slightly *more* peaky, not squashed | +1.7 dB | +2.43 dB |
| **Even vs odd harmonics** | the octave-related ones should lead | — | second harmonic leads third by 6.3 dB |

So on paper it hits the target. **Nobody has heard it against your reference
file.** That is the entire purpose of this build.

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

### Test 1 — Against the reference (the important one)

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

**Report:** the number. If your ear says 40, the drive range is scaled wrong
and that is a one-line fix. If your ear says 75, the same. This is the single
most actionable thing you can report.

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

### Test 6 — Does it stay out of the way

1. **SAT** off should be a perfect bypass. Any change in level or tone with it
   off is a bug.
2. **MIX** at 0 should be the dry signal, exactly. Any phasey, comb-filtered
   quality is a bug.
3. Automate DRIVE across a phrase. Any clicks, zips or thumps are bugs.
4. Leave it on a track and play for ten minutes. Any crackle that appears over
   time is a bug.

### Test 7 — Does it behave as software

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
4. Bugs from Tests 6 and 7.
5. Anything about the panel — layout, sizes, whether DRIVE reads as the main
   control at a glance.

Bounced examples are gold if they are easy: dry and processed, level-matched,
same settings stated. There is a measurement tool in the source that can be
pointed at a bounce and will report all four numbers for that specific file.

---

## 4. Known gaps

Things already found. **You do not need to look for these** — but if any of
them bothers you in practice, say so, because that changes the priority.

### 4.1 Two of the five frequency bands miss the target

The Fuji reference had 150–600 Hz and 600 Hz–2.5 kHz sitting about 1 dB *down*.
This build leaves them flat instead.

Fixing that means either compressing harder — which would cost the "transients
survive" behaviour the same reference asks for — or building in a fixed EQ dip,
which is a tone control the plugin deliberately does not have. Flat was judged
the better miss. **If it sounds a bit thick or forward in the low mids on real
material, this is why, and it becomes a visible control rather than a hidden
one.**

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

### 4.5 Not code-signed

macOS and Windows will both warn on first open. The read-me has the two clicks.
Removing the macOS warning properly costs $99/year for an Apple developer
account — worth deciding before this goes to anyone who will not follow
instructions.

### 4.6 The typeface is not the suite's

The panel uses the system sans instead of the suite's display faces, because
embedding those in a public open-source repo is a licensing question that is
not settled. **Layout, sizes and spacing are final; the letterforms are not.**
It will look slightly different from FrostyEQ, and that gets fixed when the
font question is answered for the whole suite.

### 4.7 The default preset is not level-matched

**Init** — what you get when the plugin first loads — comes out about 1.6 dB
quieter than the input, because AUTO is off by default. Every other preset is
matched. AUTO defaults to off on purpose (a level match is a decision about how
you are auditioning, not about how it should sound), but if that trips you up
in practice it can be changed.

### 4.8 Nothing has been validated by ear. At all.

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
