# Memory Limits

The app uses a few fixed startup allocations for UI layout, renderer instances, and song storage. When the tracker got larger, the main fix was to raise these caps so the UI could build and render the extra rows and buttons without hitting Clay or renderer limits.

- [`clayton/clayton.h`](../clayton/clayton.h): sets Clay's max element count with `Clay_SetMaxElementCount(32768)` and initializes the GLES quad instance pool with `Gles3_Initialize(..., 65536)`.
- [`clayton/clayarena.h`](../clayton/clayarena.h): sets the app's Clay string arena size with `CLAY_ARENA_CAPACITY` for all temporary UI strings.
- [`sounds/sounds.h`](../sounds/sounds.h): sets `userSongPattern` to a larger fixed buffer for the exported user song pattern.
- [`sounds/sounds.cpp`](../sounds/sounds.cpp): checks the user song pattern length before copying it into the fixed buffer.
- [`tracker/tracker_song_io.h`](../tracker/tracker_song_io.h): sets the song row limit with `TRACKER_USER_SONG_MAX_ROWS` and the serialized pattern capacity with `TRACKER_USER_SONG_PATTERN_CAPACITY`.
