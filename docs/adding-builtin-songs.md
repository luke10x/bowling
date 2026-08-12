# Adding Built-In Songs

This project now treats built-in tracker songs as source DSL files that are
compiled directly into the game. The tracker-only custom song stays separate:
it is always the last song slot, it autosaves on tracker close, and it is not
available in WAV mode.

Built-in songs are self-contained:
- the song header provides the UI pattern, instruments, and tracker metadata
- synth playback uses a flattened playback pattern built from that tracker data
- the old music patchbank is not used for songs anymore; it is only for SFX

## Rules

- Built-in songs live in [`/Users/lape/workspace/bowling/sounds/builtin_songs`](/Users/lape/workspace/bowling/sounds/builtin_songs).
- The built-in song list is defined in [`/Users/lape/workspace/bowling/sounds/builtin_song_registry.h`](/Users/lape/workspace/bowling/sounds/builtin_song_registry.h).
- The custom user song is not compiled in. It is loaded from a saved DSL file
  next to the main save/settings file and occupies the last slot only in synth
  mode.
- WAV mode only exposes built-in songs, so every built-in song must remain
  exportable through the registry.

## How To Add A Built-In Song

1. Create a new DSL header in [`/Users/lape/workspace/bowling/sounds/builtin_songs`](/Users/lape/workspace/bowling/sounds/builtin_songs).
   Name the file after the song stem, like `gutter_groove.h` or `alley_cat.h`, and use this shape:
   `XFM_TRACKER_SONG_NAME`, `XFM_TRACKER_SONG_PATTERN`,
   `XFM_TRACKER_CUSTOM_INSTRUMENTS`, and the tracker metadata constants.
   A file saved directly from the tracker is valid input here as-is; you do not
   need to hand-trim extra declared instruments just to make it builtin.
   The runtime understands both legacy `OP ...` rows and the newer `FM ...`
   operator rows saved by the tracker.
2. Add a new namespace include block in
   [`/Users/lape/workspace/bowling/sounds/builtin_song_registry.h`](/Users/lape/workspace/bowling/sounds/builtin_song_registry.h).
3. Add a matching registry entry to `BUILTIN_SONG_REGISTRY` in that same file.
   The registry is the source of truth for:
   - song count
   - display names
   - tracker metadata
   - built-in stem reservation
   - WAV export/caching
4. Extend the explicit WAV asset conversion list in [`/Users/lape/workspace/bowling/Makefile`](/Users/lape/workspace/bowling/Makefile).
   Today those `xxd` lines are still enumerated one by one, so a new built-in
   song also needs a matching `song_XX.wav -> song_XX_xxd.cpp` entry there.
5. Rebuild the app. The song will automatically appear in:
   - the synth song selector
   - tracker built-in loading
   - synth playback with correct flattened rows for `PART` / `SKIP` songs
   - adaptive WAV caching/export
   - the standalone WAV exporter

## Custom Song Behavior

- If the player selects a built-in song and opens the tracker, the tracker
  copies that built-in into the custom slot and edits the custom slot.
- If the player already has the custom slot selected, reopening the tracker
  continues editing that same custom song.
- Closing the tracker autosaves the custom song DSL.
- On app start, the custom DSL is loaded automatically if the file exists.

## Things To Avoid

- Do not hardcode built-in song counts in new code. Use
  `BUILTIN_SONG_REGISTRY_COUNT`, `TRACKER_BUILTIN_SONG_COUNT`, and
  `TRACKER_USER_SONG_SLOT`.
- Do not expose the custom song in WAV mode.
- Do not add built-in song metadata anywhere except the registry and the song's
  own DSL header.
- Do not add new songs through the old music patchbank path. Songs are supposed
  to carry their own instruments now.
- If a built-in song sounds different from the same file loaded as a custom
  song, first check that the builtin path is using the flattened playback
  pattern and the song's own instrument DSL, not just the raw header text.
