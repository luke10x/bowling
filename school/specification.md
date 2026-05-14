# School Mode Specification

This document defines the intended behavior of **School mode** in the bowling game, plus the **current status quo** as implemented today. It is written to be unambiguous for both humans and future AI edits.

## Goals

- Add an alternate game mode, **School**, focused on practice lessons instead of scoring.
- School is guided by **Story dialogs** (typing effect + choices) and by a **School panel** UI.
- Lessons are **unlockable** (must complete lesson N to unlock N+1).
- Even if a lesson is completed in the past, **clicking its lesson button must reload that lesson from the start** (replay).
- School should be touch-friendly (large buttons/slider).

## Non-goals (for now)

- Persisting school progress to disk.
- Implementing lessons 3–5 (these are reserved).

## Terms / vocabulary

- **School mode**: `GameMode::SCHOOL` vs `GameMode::NORMAL_GAME`.
- **Lesson**: A numbered training module (1..5).
- **Unlocked**: A lesson can be selected only if it is unlocked.
- **Done / Completed**: The player has passed the lesson at least once this run.
- **Replay**: Selecting a lesson always resets its *session state* and runs its setup from scratch, regardless of “done”.
- **Session state**: Per-lesson progress that is reset when replaying the lesson (counters, spawned coins, etc).
- **Story dialog**: Modal typed text (Angel / Me), optionally with a choice group.

## Entry / exit

### Entering School

- School is entered via:
  - Story event `EVENT_GO_TO_SCHOOL` (from game-over story), or
  - Menu window action “Go to school”.
- When entering School, the game selects a target lesson:
  - If there is any uncompleted lesson, enter the **first uncompleted**.
  - Otherwise default to lesson 1.
- The chosen lesson is **selected** and its setup runs.

### Exiting School

- Exiting School returns to normal bowling game.
- The player’s selected ball should be restored to whatever was selected **before** entering School.
- If School is not fully completed (not all 5 lessons done), exiting may show a reminder story:
  - `storyline_id=1030` “You can come back to the school anytime.”

## UI rules

### School window/panel

- School mode draws a dedicated School panel (Clay UI):
  - Title: `School :: Lesson N. <Lesson Name>`.
  - Row of lesson buttons `1..5`:
    - Disabled style for locked lessons.
    - Enabled style for unlocked lessons.
    - Active style for selected lesson.
  - Close/exit button (danger style) to exit school.
- Other normal-game UI (scoreboard and scoring) is not used in School mode.

### Lesson selection behavior (important)

- Clicking an enabled lesson button:
  - Selects that lesson.
  - **Replays it from scratch**:
    - Clears that lesson’s session progress counters.
    - Re-applies any lesson-specific physics presets.
    - Re-spawns any lesson targets (coins).
    - Shows the lesson intro story again (if no windows are open and no dialog is active).
- Selecting a lesson does **not** erase that the lesson was “done”; it only resets the session state for replay.

## Lesson 1 — Ball Mass

### Purpose

Teach that ball mass changes feel and roll, and make the player try extremes.

### Controls

- A touch-friendly slider (Clay) labeled `Mass (kg)`.
- Slider range:
  - `2.7kg .. 7.8kg` (legal-ish practice range).
- “Light” and “Heavy” test zones:
  - A throw counts as **Light** if mass is within `+2 lbs` of minimum.
  - A throw counts as **Heavy** if mass is within `-2 lbs` of maximum.
  - Any mass in the middle does **not count**.

### Passing criteria

- Required hits per side:
  - `REQUIRED_HITS_EACH = 4`.
- A “hit” counts only if the throw knocks down at least one pin.
- Passing requires both:
  - Light hits >= required AND Heavy hits >= required.

### Feedback / “harass” stories

When in lesson 1 and a throw completes:

- If the player used mid-range mass (neither light nor heavy), show story `1012`:
  - Must repeat after every throw as long as the player keeps throwing mid-range.
- If the player already passed one side, but is not using the other side, show:
  - `1013` (switch to heavy) or `1014` (switch to light),
  - repeat after every throw until the player switches appropriately.

### HUD

- Bottom-left non-interactive HUD shows two progress bars:
  - `Light ball test` and `Heavy ball test`.
  - Each bar has a green `Passed` label when completed.
- HUD is visible while the lesson 1 session is active; it disappears when the session is marked completed.

## Lesson 2 — Spin Ball (Coins)

### Purpose

Teach spinning to “drive” the ball by collecting coins along the lane.

### Visual / playfield rules

- Pins are hidden for this lesson (coins-only).
- The run ends when:
  - All coins in the current level are collected, OR
  - The ball falls off / forgiveness triggers, OR
  - The throw times out (stalled).

### Coins structure

- `LEVELS = 3`, `COINS_PER_LEVEL = 3`, `TOTAL_REQUIRED = 9`.
- Coins appear closer to the far end of the lane (toward world origin/pins side).
- Coins are spaced along Z with a large gap:
  - Current tuning uses a `Z_STEP` and places the last coin near the pin deck.

### Failure rule (“annul round”)

- If the run ends and the player did not collect all 3 coins for the current level:
  - Collected coins in that level are discarded.
  - The 3 coins respawn for the same level.
  - “Safe” coins from previous completed levels remain.

### Passing criteria

- Completing all levels (collecting 9/9 safe coins) marks the lesson as done.
- Completion shows story `1020`.

### Lesson preset

Lesson 2 applies a physics “preset” to make the ball more responsive:

- Higher bite/spin, low skid, reduced oil thickness.
- Launch speed is capped:
  - `LAUNCH_SPEED_CAP = 7.5 m/s` (current).
- Stall/timeout threshold is longer than normal:
  - `15s` (current).

### HUD

- Bottom-left HUD shows coin progress `Coins: X / 9` plus a progress bar.
- HUD is visible while lesson 2 session is active; it disappears when the session is marked completed.

## Lessons 3–5

- Reserved for future work.
- Buttons exist; initially locked.

## Cheats

- Renaming the player to:
  - `SC1` unlocks lesson 2 (lesson 1 done).
  - `SC2` unlocks lesson 3 (lessons 1–2 done).
  - ...
  - `SC5` marks all lessons done.

## Current status quo (implementation snapshot)

- School state is stored in `UserContext` and updated in `game.cpp`.
- Lesson selection now resets per-lesson session state on every selection (replay behavior).
- Lesson 1 uses mass slider + per-side hit counters and repeats hint stories when misconfigured.
- Lesson 2 uses coins-only run, 3x3 coins total, annul-per-level failures, and early run end when coins collected.
- School progress is not persisted across runs.

