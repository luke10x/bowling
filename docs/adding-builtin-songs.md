# Adding Built-In Songs

This project now treats built-in tracker songs as source DSL files that are
compiled directly into the game. The tracker-only custom song stays separate:
it is always the last song slot, it autosaves on tracker close, and it is not
available in WAV mode.

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
   Use the same shape as the existing `song_01.h`...`song_05.h` files:
   `XFM_TRACKER_SONG_NAME`, `XFM_TRACKER_SONG_PATTERN`,
   `XFM_TRACKER_CUSTOM_INSTRUMENTS`, and the tracker metadata constants.
   A file saved directly from the tracker is valid input here as-is; you do not
   need to hand-trim extra declared instruments just to make it builtin.
2. Add a new namespace include block in
   [`/Users/lape/workspace/bowling/sounds/builtin_song_registry.h`](/Users/lape/workspace/bowling/sounds/builtin_song_registry.h).
3. Add a matching registry entry to `BUILTIN_SONG_REGISTRY` in that same file.
   The registry is the source of truth for:
   - song count
   - display names
   - tracker metadata
   - built-in stem reservation
   - WAV export/caching
4. If the project still has any explicit WAV conversion steps, extend them for
   the new built-in song too.
5. Rebuild the app. The song will automatically appear in:
   - the synth song selector
   - tracker built-in loading
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
