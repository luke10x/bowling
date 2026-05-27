# Tracker Song Files

Tracker song files are text files with `.txt` extension. They are intentionally valid C++ so a good user song can later be copied into `sounds/songs_data.h` with minimal cleanup.

Current exported symbols:

```cpp
static constexpr const char *XFM_TRACKER_SONG_NAME = R"xfmname(My Song)xfmname";
static constexpr const char *XFM_TRACKER_SONG_PATTERN = R"xfmsong(32
C-4007F|.......|.......|.......|.......|.......
)xfmsong";
static constexpr const char *XFM_TRACKER_CUSTOM_INSTRUMENTS = R"xfmins(
INST 12
PATCH 0 0 0 0
OP 1 0 1 48 0 31 0 8 0 15 8 0
ENDINST
)xfmins";
```

`XFM_TRACKER_SONG_PATTERN` uses the same tracker/Furnace-like text cells the runtime already plays: row count first, then six `|` separated channels per row. Each cell stores note/special value, optional instrument, optional volume, and up to four effects.

`XFM_TRACKER_CUSTOM_INSTRUMENTS` stores only custom instruments outside the built-in music instrument range. Instruments are patch plus attached macros. Built-in instruments are not saved because built-in songs own them.

## Names

Built-in songs are never overwritten. The first edit to a built-in song copies it into the hidden fifth user-song slot.

Saving a built-in song uses the current date as `SONG_YYMMDD.txt`; for example December 31, 2026 becomes `SONG_261231.txt`, displayed in game as `Song 261231`.

Saving a user song uses its display name converted to uppercase underscore form plus `.txt`, for example `My Cool Song` becomes `MY_COOL_SONG.txt`.

Loading rejects filenames that are too short, too long, contain characters other than letters, numbers, or `_`, or match a built-in song name.

## Emscripten And Native

Emscripten currently uses browser primitives:

- Save creates a Blob and triggers browser download.
- Load opens an `<input type="file">`, reads text with `FileReader`, then calls back into C++.

Native builds deliberately do not implement load/save dialogs yet. The C++ path is isolated so native can later plug in:

- a directory-backed file picker for load
- a keypad-backed filename dialog for save
- the same `TrackerSongIO_ParseFile` and `TrackerSongIO_BuildFileText` helpers
