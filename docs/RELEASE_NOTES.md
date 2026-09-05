**Fourth build, from your 0.3.0 results.** Two fixes and a confirmation.

## The polarity bug — and there was more of it than you could see

You found that the button does nothing at Mix 0. Correct: polarity was applied
to the input, so the dry path of the Mix control never saw it.

Measuring it turned up a second fault behind the first. With SAT in, the flip
was happening *before* the saturation — and because the curve is asymmetric,
flipping the input is not the same as flipping the output. Two instances with
one flipped summed to −14 dB instead of to silence. The button was quietly
changing which harmonics came out, which is the one thing a polarity control
must never do.

Fixed exactly as you suggested: **polarity is now the last stage before
Output.** There is a test that requires two instances, one flipped, to sum to
*exact* silence across 36 combinations of SAT, MIX, DRIVE and OUTPUT.

**Output moved with it** and now applies to the blend rather than to the wet
path alone — at Mix 50 it used to be a wet trim, which is not what the name
says.

## Vocal Front

Pulled back to **Drive 46 with +1.5 dB input**, from Drive 52 with +5 dB. You
were right that the gain was doing the damage rather than the drive: 5 dB on
top of 52 put it past the crossover on every source.

## Confirmed, no change needed

- **Crossover at 55–58**, against the predicted 45–55. Close enough that the
  drive scale is settled. Thank you for re-running that one — it was the
  measurement that could not be made from here.
- **TONE stays**, on your verdict. That closes the open design question: this
  is a saturator with a voicing, not a pure-harmonics box.

## Sibilance — noted, not fixed

You heard "S" sounds sitting slightly forward of the Fuji file, and the
measurement agrees (crest factor +2.28 against +1.67). Moving the voicing bell
from 7 kHz to 8 kHz to get off the sibilance range was tried and measures worse
on both upper bands and on crest, so it stays. Pulling TONE back reduces it at
the cost of the band match, which is a per-source trade. Left as a known
difference on your call that it is low priority.

Band match against the reference is unchanged from 0.3.0 — 2.5–6 kHz exact,
everything else within half a decibel.

## Downloads

| | contains |
|---|---|
| **BMO-Saturator-macOS.zip** | VST3, AU, and a standalone app. Universal — Apple Silicon and Intel. |
| **BMO-Saturator-Windows.zip** | VST3 and a standalone, 64-bit. |

Neither is code-signed; `INSTALL.md` inside the zip has the two clicks past it.

## Where to start

Test 7 — the polarity null, in the states you found it failing. Then anything
you have not tried yet; the tonal side has been stable for two builds and the
remaining open items are all things only ears can settle.

<https://github.com/kevkloud/bmo-saturator#readme>
