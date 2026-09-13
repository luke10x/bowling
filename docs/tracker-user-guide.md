# Tracker User Guide

This guide explains the in-game tracker from the beginning: what the screen is,
how to enter notes, how to shape FM instruments, how to divide songs into parts,
how to save and load work, and how tracker songs become game music.

The tracker is a six-channel FM music editor built into the game. It is meant to
be used on desktop and touch screens, so most editing happens through buttons,
sliders, piano keys, and small pop-up editors instead of typed pattern commands.
If you know trackers like Furnace, the ideas will feel familiar. If not, start
here and ignore the file format until later.

For deeper reference material, see:

- [tracker-song-files.md](tracker-song-files.md) for the saved `.h` file format.
- [fm-song-effects.md](fm-song-effects.md) for effect codes.
- [tracker-macros.md](tracker-macros.md) for macro loop and release behavior.

## Image Placeholders

Use these placeholders when laying out the final manual. Replace each placeholder
with a screenshot or diagram later.

> IMAGE PLACEHOLDER: Full tracker screen  
> Show the song title at top, SAVE/LOAD buttons, the six channel headers, a few
> part headers, the cell grid, the scrollbar, and the bottom transport buttons.

> IMAGE PLACEHOLDER: Cell editor  
> Show a single cell opened for editing with the NOTE tab selected, instrument
> selector, volume slider, piano keyboard, OFF/REL/===/... buttons, and DEL.

> IMAGE PLACEHOLDER: Effects tab  
> Show the cell editor EFFECTS tab with one active effect, its two parameter
> sliders, and the short effect description text.

> IMAGE PLACEHOLDER: Instruments window  
> Show the INSTR. list with several instruments, colored rows, and buttons for
> new, clone, rename, edit, delete, and reorder.

> IMAGE PLACEHOLDER: Instrument patch editor  
> Show the Patch tab with algorithm selector, feedback/AMS/FMS sliders, the
> algorithm diagram, and the four operator panels.

> IMAGE PLACEHOLDER: Operator editor  
> Show one operator opened for detailed editing, highlighting TL, AR, DR, SL, SR,
> RR, SSG, MUL, DT, RS, and AM.

> IMAGE PLACEHOLDER: Macro editor  
> Show the Macros tab with a target such as TL1 or ARP, enabled checkbox, graph
> bars, Loop button, Rel button, and horizontal/vertical scroll controls.

> IMAGE PLACEHOLDER: Parts  
> Show three song parts in the grid: one normal part, one collapsed part, and one
> disabled or repeat part. Include the part control buttons.

> IMAGE PLACEHOLDER: Save and load windows  
> Show SAVE with filename and EXPORT, then LOAD with BUILTIN_SONGS, BUILTIN_SFX,
> MY_SONGS, LOAD, IMPORT, playlist checkboxes, and delete/reset buttons.

## The Big Idea

A song is a vertical timeline. Time moves from top to bottom. Each row is a small
slice of time, and each of the six channels can play one FM voice at a time.

Each cell can contain:

```text
C-4007F0108
```

Read that as:

- `C-4`: note C in octave 4.
- `00`: instrument 00.
- `7F`: volume, with `7F` as full tracker volume.
- `0108`: an effect, here pitch slide up at speed `08`.

Empty fields use dots. For example:

```text
.......
```

means no note, no instrument, and no volume on that row.

The tracker uses hexadecimal for compact values. Instrument `10` means decimal
16, and volume `7F` means decimal 127.

## Getting Around

Open the tracker from the game UI. The tracker takes over the screen and shows
the current music. If the current song is built in, editing copies it into an
editable user song state so the original built-in asset is not destroyed.

The top row shows:

- The song name.
- Current time, total time, row, and tick.
- Estimated BPM and scale.
- `SAVE`, `LOAD`, and close.

The main grid shows channel columns `CH1` through `CH6`. Tap a cell to open the
cell editor. Drag the grid or scrollbar to move around the song.

The bottom controls include:

- `START`: play from the current playhead or selection.
- `STOP`: stop playback.
- `FOLLOW`: keep the view following the playhead during playback.
- `+PART`: add a new song part.
- `SONG`: open song settings.
- `INSTR.`: open instrument management.
- `OSC`: show or hide the oscilloscope.

## Entering Your First Notes

1. Tap an empty cell in `CH1`.
2. In the cell editor, stay on the `NOTE` tab.
3. Tap a piano key. The cell updates immediately to that note.
4. Choose an instrument with the left/right arrows beside the instrument name.
5. Set volume with the `VOL` slider. `7F` is full volume.
6. Close the editor with `x`, or tap another cell to edit the next note.
7. Press `START` to hear the result.

A simple first pattern could look like this:

```text
C-4007F|.......|.......|.......|.......|.......
.......|.......|.......|.......|.......|.......
E-4007F|.......|.......|.......|.......|.......
.......|.......|.......|.......|.......|.......
G-4007F|.......|.......|.......|.......|.......
```

To make a chord, put notes on the same row in different channels. Each channel
can play only one note at a time, but all six channels can sound together.

## Special Note Values

The NOTE tab has four special values:

- `OFF`: key off. Use this to stop a note cleanly.
- `REL`: release. Use this when you want the instrument or macro release segment
  to take over.
- `===`: release marker used by the live recorder.
- `...`: no note value.

Use `DEL` to clear the whole cell.

## Instruments And Inheritance

Every playable note needs an instrument. The instrument column is the two
hexadecimal digits after the note, such as `00` in `C-4007F`.

The checkmark beside the instrument selector controls whether the instrument is
written directly into the cell. If it is unchecked, the cell inherits the most
recent previous instrument in that channel. This keeps patterns clean, but for
beginners it is usually clearer to leave the instrument explicit on important
notes.

Volume works the same way. The volume checkmark decides whether the volume is
written into the cell or inherited from earlier in the channel.

## Recording With The Mini Keyboard

The red `R` button in the channel header area enables the live recorder. When it
is on:

- Choose which channels are armed with the `REC1` through `REC6` buttons.
- Play notes on the mini keyboard.
- Notes are written at the current playhead row.
- The `=` button controls whether releases are recorded.
- Hold `D` to clear armed-channel cells as the playhead moves.
- The octave buttons move the mini keyboard range.

This is useful for sketching, especially on touch devices. For precise edits,
tap individual cells and use the cell editor.

## Editing Effects

Open a cell and switch to the `EFFECTS` tab. Pick an effect with the arrow
buttons, adjust its parameters, and press the checkmark to make it active.

Each cell can save up to two active effects. The tracker remembers many effects
as channel state, so they continue until stopped or replaced. For example:

- `040F` turns on vibrato.
- `0400` turns vibrato off.
- `0A0F` slides volume down.
- `0A00` stops the volume slide.
- `0308` enables portamento.
- `0300` disables portamento.

Effects are written as four characters: two for the effect code and two for the
value. Some values are split into two nibbles. For `04xy`, `x` is vibrato speed
and `y` is depth.

See [fm-song-effects.md](fm-song-effects.md) for the full list.

## Adding And Managing Instruments

Press `INSTR.` to open the instrument window.

From there you can:

- Select an instrument row.
- Create a new instrument.
- Clone the selected instrument into a free slot.
- Rename an instrument.
- Edit its patch and macros.
- Delete it.
- Move instruments up or down.

Instruments have names and colors. The color is used in the tracker grid so you
can quickly see which sound a note uses.

## A Little FM Synthesis

The tracker uses Yamaha-style FM patches. Each instrument has four operators.
An operator is a small oscillator with its own volume, envelope, multiplier, and
modulation settings. The algorithm decides how the four operators connect.

Some operators are carriers, which you hear directly. Others are modulators,
which change the tone of another operator. Modulators create brightness,
growl, bells, metallic edges, and many other FM colors.

Start with these controls:

- `ALGO`: changes the operator routing. Different algorithms can radically
  change the same operator settings.
- `FB`: feedback. Higher feedback adds roughness or noise-like edge.
- `FMS`: how much the chip LFO affects pitch when LFO is enabled.
- `AMS`: how much the chip LFO affects amplitude for AM-enabled operators.
- Operator `TL`: total level. Smaller values are louder; larger values are
  quieter. On modulators, TL changes brightness more than loudness.
- Operator `MUL`: frequency multiplier. `1` follows the played note, higher
  values add harmonics, and `0` can produce very low or special FM behavior.

## Envelopes

Each operator has its own envelope. The envelope controls how that operator
changes over the life of a note.

The most important fields are:

- `AR`: attack rate. Higher values start faster.
- `DR`: decay rate. Higher values move faster from the attack peak toward the
  sustain level.
- `SL`: sustain level. This is the level the decay stage aims for.
- `SR`: sustain rate. Higher values continue fading while the note is held.
- `RR`: release rate. Higher values fade faster after note-off or release.
- `TL`: total level. This is not technically part of the envelope shape, but it
  sets the operator's overall loudness or modulation strength.

For a pluck, use fast attack, noticeable decay, and a release that fades quickly.
For a pad, use slower attack and a longer release. For a bell, use modulators
with higher multipliers and envelopes that fade at different speeds.

## Operator Details

Tap an operator panel to open the detailed operator editor.

Useful controls:

- `AM`: lets the global chip LFO modulate this operator's amplitude.
- `MUL`: changes harmonic content.
- `DT`: detune. Small detune values can thicken a sound.
- `RS`: rate scale. Higher notes can have faster envelopes.
- `SSG`: alternate envelope shapes. These can create looping, inverted, or
  sharper envelope behavior.

Small changes matter in FM. Move one value at a time and audition the note after
each change.

## LFO

The tracker has song-level OPN LFO settings in `SONG` -> `Playback`.

- `LFO` enables or disables the chip LFO.
- `Freq` selects one of the chip's LFO frequencies, `0` through `7`.
- `FMS` on an instrument controls pitch modulation depth.
- `AMS` on an instrument controls amplitude modulation depth.
- Operator `AM` decides which operators can receive amplitude modulation.

The effect `10xy` can also change chip LFO during playback. `x` enables or
disables it, and `y` selects the frequency.

## Macros

Macros automate instrument parameters per note. Open an instrument, switch to
`Macros`, choose a target, enable it, and draw values in the graph.

Common targets include:

- `TL1` through `TL4`: animate operator total levels.
- `MUL1` through `MUL4`: animate multipliers.
- `DT1` through `DT4`: animate detune.
- `FB`: animate feedback.
- `ARP`: play semitone offsets from the note.
- Envelope targets such as `AR`, `DR`, `SR`, `SL`, and `RR` for individual
  operators.

Loop and release controls:

- `Loop` marks the sustain loop start. While the note is held, the macro loops
  from that step.
- `Rel` marks the release segment. When the note is released, the macro jumps
  to that step and continues forward.

Use macros for movement that belongs to the instrument itself, such as a pluck
getting duller, a bass warbling, or an arpeggio built into a lead sound. Use
effects for row-specific commands, such as a one-time slide or vibrato change.

## Song Settings

Press `SONG`.

The `Song` tab contains:

- `Name`: display name for the song.
- `Root` and `Scale`: visual guidance for the keyboards and grid. Out-of-scale
  notes are dimmed, but still allowed.
- `Tuning`: `12-TET` or `JI`.
- `Empty` -> `Load`: replace the current pattern with an empty one.

The `Playback` tab contains:

- `LFO`: chip LFO enable.
- `Freq`: LFO frequency index.
- `Tick Rate`: tracker ticks per second.
- `Ticks/Row`: how many ticks each row lasts.
- `Rows/Beat`: how many tracker rows make one beat.

Estimated BPM is:

```text
(Tick Rate * 60) / (Ticks/Row * Rows/Beat)
```

At the default `60` tick rate, `6` ticks per row, and `4` rows per beat, the
song is about `150 BPM`.

## Parts

Songs are divided into named parts. A part is a block of rows in the timeline:
intro, verse, chorus, bridge, ending, or any structure you like.

Part headers have controls for:

- Collapse or expand.
- Enable or disable. A long press on the enable button marks that part as
  `REP`, so playback focuses on that part until repeat is cleared.
- Move up or down.
- Open part settings.

Press `+PART` to add a new part at the end of the song. Open a part's settings
to rename it, change its row count, clone it, or delete it.

Disabled parts are skipped during playback. They are still saved in the song, so
you can keep alternate ideas, drafts, or muted sections without losing them.

The saved pattern writes parts as text lines:

```text
PART Intro
C-4007F|.......|.......|.......|.......|.......
SKIP Muted sketch
D-4007F|.......|.......|.......|.......|.......
PART Chorus
G-4007F|.......|.......|.......|.......|.......
```

`PART` marks a normal section. `SKIP` marks a skipped section. Skipped parts are
not merely quiet; they are omitted from the playback pattern.

## Looping And Selections

The tracker supports loop ranges and selection playback. Use the left row area
and scrollbar range to mark a loop, then clear it with the small `x` in the
header. When a selection is active, playback can focus on that selected area.

For editing blocks, use `SEL` to enter edit-selection mode, then drag across
cells. `COPY`, `CUT`, and `PASTE` work with selected rectangles. Pasting keeps
instrument references useful by bringing referenced instrument definitions along
when possible.

## Saving Songs

Press `SAVE` at the top of the tracker.

The save window has three important actions:

- `SAVE`: save to the browser/local game storage as a named song.
- `OVERRIDE`: when editing a built-in song or built-in SFX, save your edited
  version as an override for that built-in item.
- `EXPORT`: download the song as a `.h` tracker song file.

Use `SAVE` for normal in-game work. Saved songs appear under `LOAD` -> `MY_SONGS`
and can be included in the music playlist.

Use `EXPORT` when you want a portable file, a backup, or a song that can be
committed into the source tree as a built-in song.

Saved tracker files are valid C++ header files. They contain:

- Song name.
- Tick rate, speed, rows per beat, scale, tuning, and LFO settings.
- Pattern text.
- Instrument patch and macro text.

## Loading Songs

Press `LOAD`.

The load window has three tabs:

- `BUILTIN_SONGS`: game music shipped with the build.
- `BUILTIN_SFX`: built-in sound effects that use the same tracker data path.
- `MY_SONGS`: songs saved locally by you.

Select a row and press `LOAD` to edit it. Use `IMPORT` to load an exported `.h`
tracker file from disk. Use `DEL` to delete a saved local song. Use `RESET` on a
built-in row when you want to remove a saved override and return to the shipped
version.

The checkboxes in `BUILTIN_SONGS` and `MY_SONGS` control the in-game music
playlist. The game can rotate through selected built-in songs and saved user
songs. At least one song must remain selected.

## How Tracker Music Plays In The Game

The game sound system reads tracker patterns through the `eggsfm` FM synth path.
Built-in songs are compiled into the game. User songs are loaded from local
storage or imported `.h` files. Either way, the sound system receives:

- A UI pattern, which preserves parts and skipped sections for editing.
- A playback pattern, which flattens the enabled parts into what the game should
  actually play.
- The instrument bank, including patches and enabled macros.
- Song timing and tuning metadata.
- Song LFO settings.

When you leave the tracker, the current pattern and instrument edits are pushed
back to the sound system and saved to local storage. If the browser audio system
resumes after a suspend, the tracker can push patches and macros back into the
synth so the song keeps using the edited instrument bank.

In normal gameplay, the selected music playlist decides which songs can play.
If you saved a song under `MY_SONGS` and checked it in the load window, it can be
part of the game music rotation. If you saved an override for a built-in song,
that built-in slot plays your version until you reset the override.

## A Good First Exercise

1. Open `SONG`, set the name to `FIRST_TRACK`, and keep the default tempo.
2. Tap the first cell in `CH1`.
3. Choose instrument `00`, volume `7F`, and note `C-4`.
4. Add `E-4` in `CH2` and `G-4` in `CH3` on the same row.
5. Add `OFF` or `REL` a few rows later in the same channels.
6. Press `+PART`, rename the new part `BASS`, and add a simple low note pattern.
7. Open `INSTR.`, clone instrument `00`, rename it `Bass`, and edit the clone.
8. In the patch editor, try another `ALGO`, lower a modulator `TL`, and change
   one operator `MUL`.
9. Save the song with `SAVE`.
10. Export the `.h` file when you want a backup or built-in candidate.

That path touches the whole tracker without requiring you to understand every
effect or FM parameter at once.
