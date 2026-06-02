# Tracker Macros (Loop / Release)

This tracker supports per-instrument “macros”: small step-sequences that automate synth parameters over time (for example, volume/timbre changes, operator tweaks, etc.).

In the **Macro Editor** you will see two flags:

- **Loop** (shown as `Loop off` or `Loop NN`)
- **Rel** (shown as `Rel off` or `Rel NN`)

Step numbers in the UI are **1-based** (`01..32`).

## What “Loop off” means

`Loop off` means the macro has **no sustain loop** (`has_loop = false`).

When a note is held:

- The macro plays from **step 01 → step N** (N is the macro length).
- There is no looping behavior defined by the macro.

When you click the **Loop** button:

- If looping is currently **off**, it turns looping **on** and sets the loop start to the **currently selected step** (so it becomes `Loop NN`).
- If looping is **on** and you click **Loop** on a *different* selected step, it moves the loop start to that step.
- If looping is **on** and you click **Loop** again on the *same* selected step, it toggles back to `Loop off`.

## What “Rel off” means

`Rel off` means the macro has **no dedicated release segment** (`release_start = 0xFF`).

When the note is released (key-off):

- The macro does **not** jump to a special “release part”.
- The macro does not have a defined release behavior beyond whatever the synth itself does on key-off.

When you click the **Rel** button:

- It sets `Rel NN` where `NN` is the **currently selected step**.
- Clicking **Rel** again on the same selected step toggles it back to `Rel off`.

## How Loop and Rel behave together

Think of the macro timeline like this:

- **Held phase** (note is pressed/held):
  - If `Loop off`: play forward 01..N.
  - If `Loop NN`: play forward, then **loop starting at NN** while the note remains held.
- **Release phase** (after key-off):
  - If `Rel off`: there is **no macro release segment**.
  - If `Rel NN`: when the note is released, the macro **jumps to step NN** and continues forward (usually used for fade-outs or “tail” shaping).

### Example

- `Loop 05`, `Rel 13`
  - While held: macro runs 01..12, then loops 05..12
  - On release: macro jumps to 13..N and plays that tail

## Tips

- Use `Loop NN` for sustained sounds (pads, held bass notes) where you want an animated “steady-state”.
- Use `Rel NN` for controlled fade-outs or timbre changes after release.
- If you’re not sure, start with `Loop off` and `Rel off`, then add Loop or Rel only when you want explicit behavior.

