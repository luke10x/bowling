#pragma once

#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <string>
#include <utility>

#include "../clayton/clayton_click.h"
#include "../sounds/builtin_song_registry.h"
#include "../sounds/builtin_sfx_registry.h"
#include "../sounds/songs_data.h"
#include "tracker_song_io.h"

struct Clayton;

static constexpr int TRACKER_CHANNELS = 6;
static constexpr int TRACKER_MAX_ROWS = TRACKER_USER_SONG_MAX_ROWS;
static constexpr int TRACKER_MAX_EFFECT_SLOTS = 4;
static constexpr int TRACKER_CELL_ACTIVE_EFFECT_LIMIT = 2;
static constexpr int TRACKER_MACRO_UI_STEPS = 32;
static constexpr int TRACKER_MACRO_VISIBLE_STEPS = 8;
static constexpr int TRACKER_MACRO_SCROLL_STEP = 4;
static constexpr int TRACKER_MACRO_SELECT_LOOP = 0;
static constexpr int TRACKER_MACRO_SELECT_RELEASE = 1;
static constexpr int TRACKER_CELL_CHARS = 7 + TRACKER_MAX_EFFECT_SLOTS * 4 + 1;
static constexpr int TRACKER_MAX_USED_INSTRUMENTS = 64;
static constexpr int TRACKER_INSTRUMENT_NAME_CAPACITY = 24;
static constexpr int TRACKER_SAVED_SONG_LIST_CAPACITY = 96;
static constexpr int TRACKER_SAVED_SONG_NAME_CAPACITY = 64;
static constexpr int TRACKER_MAX_PARTS = 32;
static constexpr int TRACKER_PART_NAME_CAPACITY = 32;
static constexpr float TRACKER_CLIPBOARD_CUT_COOLDOWN_S = 3.0f;
static constexpr uint64_t TRACKER_CELL_MOVE_HOLD_MS = 400;
static constexpr float TRACKER_CHANGE_FLASH_DURATION_S = 0.82f;
static constexpr float TRACKER_PART_COLLAPSE_ANIM_DURATION_S = 0.22f;
static constexpr int TRACKER_CELL_FLASH_RANGE_COUNT = 24;
static constexpr int TRACKER_PART_FLASH_COUNT = 8;

enum TrackerChangeFlashKind : uint8_t
{
    TRACKER_CHANGE_FLASH_NONE = 0,
    TRACKER_CHANGE_FLASH_EDIT = 1,
    TRACKER_CHANGE_FLASH_ADD = 2,
};

struct TrackerCellFlashRange
{
    float timeLeft = 0.0f;
    uint8_t kind = TRACKER_CHANGE_FLASH_NONE;
    int rowStart = 0;
    int rowEnd = -1;
    int channelStart = 0;
    int channelEnd = -1;
};

struct TrackerPartFlash
{
    float timeLeft = 0.0f;
    uint8_t kind = TRACKER_CHANGE_FLASH_NONE;
    int partIndex = -1;
};

enum TrackerSongScaleMode
{
    TRACKER_SONG_SCALE_CHROMATIC = 0,
    TRACKER_SONG_SCALE_MAJOR,
    TRACKER_SONG_SCALE_DORIAN,
    TRACKER_SONG_SCALE_PHRYGIAN,
    TRACKER_SONG_SCALE_LYDIAN,
    TRACKER_SONG_SCALE_MIXOLYDIAN,
    TRACKER_SONG_SCALE_NATURAL_MINOR,
    TRACKER_SONG_SCALE_LOCRIAN,
    TRACKER_SONG_SCALE_HARMONIC_MINOR,
    TRACKER_SONG_SCALE_MELODIC_MINOR,
    TRACKER_SONG_SCALE_MAJOR_PENTATONIC,
    TRACKER_SONG_SCALE_CHINESE_PENTATONIC,
    TRACKER_SONG_SCALE_MINOR_PENTATONIC,
    TRACKER_SONG_SCALE_INSEN,
    TRACKER_SONG_SCALE_HIRAJOSHI,
    TRACKER_SONG_SCALE_YO,
    TRACKER_SONG_SCALE_IN,
    TRACKER_SONG_SCALE_RYUKYU
};

struct TrackerSongScaleDef
{
    const char *name;
    uint16_t noteMask;
};

static constexpr TrackerSongScaleDef TRACKER_SONG_SCALE_DEFS[] = {
    {"Chromatic", 0x0FFF},
    {"Major", 0x0AB5},
    {"Dorian", 0x06AD},
    {"Phrygian", 0x05AB},
    {"Lydian", 0x0AD5},
    {"Mixolydian", 0x06B5},
    {"Minor", 0x05AD},
    {"Locrian", 0x056B},
    {"Harmonic Minor", 0x09AD},
    {"Melodic Minor", 0x0AAD},
    {"Major Pentatonic", 0x0295},
    {"Chinese Pentatonic", 0x0295},
    {"Minor Pentatonic", 0x04A9},
    {"Insen", 0x04A3},
    {"Hirajoshi", 0x018D},
    {"Yo", 0x02A5},
    // {"In", 0x01A3},
    // {"Ryukyu", 0x08B1},
};

static constexpr int TRACKER_SONG_SCALE_MODE_COUNT =
    (int)(sizeof(TRACKER_SONG_SCALE_DEFS) / sizeof(TRACKER_SONG_SCALE_DEFS[0]));

inline int Tracker_ClampSongScaleMode(int mode)
{
    return std::max(0, std::min(TRACKER_SONG_SCALE_MODE_COUNT - 1, mode));
}

inline int Tracker_NextSongScaleMode(int mode, int direction)
{
    int idx = Tracker_ClampSongScaleMode(mode);
    if (TRACKER_SONG_SCALE_MODE_COUNT <= 0)
        return 0;
    idx = (idx + direction) % TRACKER_SONG_SCALE_MODE_COUNT;
    if (idx < 0)
        idx += TRACKER_SONG_SCALE_MODE_COUNT;
    return idx;
}

inline const char *Tracker_SongScaleModeName(int mode)
{
    return TRACKER_SONG_SCALE_DEFS[Tracker_ClampSongScaleMode(mode)].name;
}

inline int Tracker_ClampSongScaleRoot(int root)
{
    return ((root % 12) + 12) % 12;
}

inline int Tracker_NextSongScaleRoot(int root, int direction)
{
    return Tracker_ClampSongScaleRoot(root + direction);
}

inline const char *Tracker_SongScaleRootName(int root)
{
    static constexpr const char *kNoteNames[12] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
    return kNoteNames[Tracker_ClampSongScaleRoot(root)];
}

inline bool Tracker_SongScaleIncludesNote(int mode, int root, int note)
{
    int normalizedNote = ((note - Tracker_ClampSongScaleRoot(root)) % 12 + 12) % 12;
    uint16_t mask = TRACKER_SONG_SCALE_DEFS[Tracker_ClampSongScaleMode(mode)].noteMask;
    return (mask & (uint16_t)(1u << normalizedNote)) != 0;
}

enum TrackerClipboardBannerKind
{
    TRACKER_CLIPBOARD_BANNER_NONE = 0,
    TRACKER_CLIPBOARD_BANNER_SUCCESS = 1,
    TRACKER_CLIPBOARD_BANNER_ERROR = 2
};

struct TrackerCell
{
    char text[TRACKER_CELL_CHARS] = ".......";
};

struct TrackerClipboard
{
    bool valid = false;
    int rows = 0;
    int channels = 0;
    TrackerCell cells[TRACKER_MAX_ROWS][TRACKER_CHANNELS] = {};
    int instrumentCount = 0;
    bool instrumentOverflow = false;
    int instrumentSourceIds[TRACKER_MAX_USED_INSTRUMENTS] = {};
    int instrumentPasteIds[TRACKER_MAX_USED_INSTRUMENTS] = {};
    bool instrumentPasteMapped[TRACKER_MAX_USED_INSTRUMENTS] = {};
    bool instrumentSourceToEntryValid[256] = {};
    int instrumentSourceToEntry[256] = {};
    char instrumentNames[TRACKER_MAX_USED_INSTRUMENTS][TRACKER_INSTRUMENT_NAME_CAPACITY] = {};
    int32_t instrumentNameLengths[TRACKER_MAX_USED_INSTRUMENTS] = {};
    uint32_t instrumentColors[TRACKER_MAX_USED_INSTRUMENTS] = {};
    xfm_patch_opn instrumentPatches[TRACKER_MAX_USED_INSTRUMENTS] = {};
    bool instrumentPatchValid[TRACKER_MAX_USED_INSTRUMENTS] = {};
    XfmMacro instrumentMacros[TRACKER_MAX_USED_INSTRUMENTS][XFM_MACRO_TARGET_COUNT] = {};
    bool instrumentMacroEnabled[TRACKER_MAX_USED_INSTRUMENTS][XFM_MACRO_TARGET_COUNT] = {};
    bool instrumentMacroValid[TRACKER_MAX_USED_INSTRUMENTS][XFM_MACRO_TARGET_COUNT] = {};
};

struct TrackerPart
{
    int startRow = 0;
    int rowCount = 0;
    bool collapsed = false;
    bool collapseAnimating = false;
    float collapseAnimT = 1.0f;
    float collapseAnimFrom = 1.0f;
    float collapseAnimTo = 1.0f;
    bool enabled = true;
    bool repeat = false;
    char name[TRACKER_PART_NAME_CAPACITY] = "PART 1";
    int32_t nameLen = 6;
};

enum TrackerVisualRowKind
{
    TRACKER_VISUAL_ROW_NONE = 0,
    TRACKER_VISUAL_ROW_PART_TITLE = 1,
    TRACKER_VISUAL_ROW_CELL = 2
};

struct TrackerVisualRow
{
    TrackerVisualRowKind kind = TRACKER_VISUAL_ROW_NONE;
    int part = -1;
    int row = -1;
    int localRow = -1;
};

struct TrackerPartProgressVisual
{
    bool visible = false;
    bool selectionMode = false;
    float segmentStart01 = 0.0f;
    float segmentEnd01 = 1.0f;
    float progress01 = 0.0f;
};

struct TrackerEffectDef
{
    uint8_t code;
    const char *name;
    const char *paramA;
    const char *paramB;
    uint8_t minA;
    uint8_t maxA;
    uint8_t minB;
    uint8_t maxB;
    uint8_t paramCount;
};

static constexpr uint8_t TRACKER_EFFECT_CODE_NONE = 0xFF;
static constexpr int TRACKER_DEFAULT_EFFECT_DEF_INDEX = 2;

static constexpr TrackerEffectDef TRACKER_EFFECT_DEFS[] = {
    {TRACKER_EFFECT_CODE_NONE, "None", "", "", 0, 0, 0, 0, 0},
    {0x00, "Arpeggio", "semi1", "semi2", 0, 15, 0, 15, 2},
    {0x01, "Pitch up", "speed", "", 0, 255, 0, 0, 1},
    {0x02, "Pitch down", "speed", "", 0, 255, 0, 0, 1},
    {0x03, "Portamento", "speed", "", 0, 255, 0, 0, 1},
    {0x04, "Vibrato", "speed", "depth", 0, 15, 0, 15, 2},
    {0x07, "Tremolo", "speed", "depth", 0, 15, 0, 15, 2},
    {0x08, "Panning", "left", "right", 0, 1, 0, 1, 2},
    {0x0A, "Volume slide", "up", "down", 0, 15, 0, 15, 2},
    {0x0C, "Retrigger", "ticks", "", 0, 255, 0, 0, 1},
    {0xE1, "Note slide up", "speed", "semi", 0, 15, 0, 15, 2},
    {0xE2, "Note slide down", "speed", "semi", 0, 15, 0, 15, 2},
    {0xE5, "Fine pitch", "offset", "", 0, 255, 0, 0, 1},
    {0xEA, "Legato", "on", "", 0, 1, 0, 0, 1},
    {0xEE, "Patch morph", "speed", "", 0, 255, 0, 0, 1},
    {0xF5, "Macro off", "target", "", 0, XFM_MACRO_SSG4, 0, 0, 1},
    {0xF6, "Macro on", "target", "", 0, XFM_MACRO_SSG4, 0, 0, 1},
    {0x10, "OPN LFO", "on", "freq", 0, 1, 0, 7, 2},
    {0x11, "Feedback", "fb", "", 0, 7, 0, 0, 1},
    {0x12, "OP1 TL", "tl", "", 0, 127, 0, 0, 1},
    {0x13, "OP2 TL", "tl", "", 0, 127, 0, 0, 1},
    {0x14, "OP3 TL", "tl", "", 0, 127, 0, 0, 1},
    {0x15, "OP4 TL", "tl", "", 0, 127, 0, 0, 1},
    {0x16, "OP MUL", "op", "mul", 0, 4, 0, 15, 2},
    {0x19, "All AR", "ar", "", 0, 31, 0, 0, 1},
    {0x1A, "OP1 AR", "ar", "", 0, 31, 0, 0, 1},
    {0x1B, "OP2 AR", "ar", "", 0, 31, 0, 0, 1},
    {0x1C, "OP3 AR", "ar", "", 0, 31, 0, 0, 1},
    {0x1D, "OP4 AR", "ar", "", 0, 31, 0, 0, 1},
    {0x30, "Hard reset", "on", "", 0, 1, 0, 0, 1},
    {0x50, "OP AM", "op", "on", 0, 4, 0, 1, 2},
    {0x51, "OP SL", "op", "sl", 0, 4, 0, 15, 2},
    {0x52, "OP RR", "op", "rr", 0, 4, 0, 15, 2},
    {0x53, "OP DT", "op", "dt", 0, 4, 0, 7, 2},
    {0x54, "OP RS", "op", "rs", 0, 4, 0, 3, 2},
    {0x55, "OP SSG", "op", "ssg", 0, 4, 0, 8, 2},
    {0x56, "All DR", "dr", "", 0, 31, 0, 0, 1},
    {0x57, "OP1 DR", "dr", "", 0, 31, 0, 0, 1},
    {0x58, "OP2 DR", "dr", "", 0, 31, 0, 0, 1},
    {0x59, "OP3 DR", "dr", "", 0, 31, 0, 0, 1},
    {0x5A, "OP4 DR", "dr", "", 0, 31, 0, 0, 1},
    {0x5B, "All SR", "sr", "", 0, 31, 0, 0, 1},
    {0x5C, "OP1 SR", "sr", "", 0, 31, 0, 0, 1},
    {0x5D, "OP2 SR", "sr", "", 0, 31, 0, 0, 1},
    {0x5E, "OP3 SR", "sr", "", 0, 31, 0, 0, 1},
    {0x5F, "OP4 SR", "sr", "", 0, 31, 0, 0, 1},
    {0x60, "OP mask", "mode", "mask", 0, 4, 0, 15, 2},
    {0x61, "Algorithm", "alg", "", 0, 7, 0, 0, 1},
    {0x62, "FMS", "fms", "", 0, 7, 0, 0, 1},
    {0x63, "AMS", "ams", "", 0, 3, 0, 0, 1},
};

static constexpr int TRACKER_EFFECT_DEF_COUNT = (int)(sizeof(TRACKER_EFFECT_DEFS) / sizeof(TRACKER_EFFECT_DEFS[0]));

struct Tracker
{
    bool active = false;

    int songIndex = 1;
    char songDisplayName[TRACKER_SONG_NAME_CAPACITY] = "Bowling Strike";
    int rowCount = 32;
    TrackerCell cells[TRACKER_MAX_ROWS][TRACKER_CHANNELS] = {};
    int partCount = 1;
    TrackerPart parts[TRACKER_MAX_PARTS] = {};

    float scrollY = 0.0f;
    float scrollVelocity = 0.0f;
    float rowHeight = 44.0f;
    float viewportHeight = 360.0f;
    bool dragging = false;
    bool dragMoved = false;
    bool partProgressScrubPending = false;
    bool partProgressScrubMovedY = false;
    int partProgressScrubPart = -1;
    uint64_t partProgressScrubStartedAtMs = 0;
    float partProgressScrubStartX = 0.0f;
    float partProgressScrubStartY = 0.0f;
    float partProgressScrubRailX = 0.0f;
    float partProgressScrubRailW = 0.0f;
    bool cellMoving = false;
    bool cellMoveValidTarget = false;
    int cellMoveSourceRow = -1;
    int cellMoveSourceChannel = -1;
    int cellMoveHoverRow = -1;
    int cellMoveHoverChannel = -1;
    TrackerCell cellMoveSource = {};
    bool cellMovePending = false;
    bool cellMovePendingSuppressed = false;
    int cellMovePendingRow = -1;
    int cellMovePendingChannel = -1;
    float cellMovePendingStartX = 0.0f;
    float cellMovePendingStartY = 0.0f;
    float cellMovePendingCurrentX = 0.0f;
    float cellMovePendingCurrentY = 0.0f;
    uint64_t cellMovePendingStartedAtMs = 0;
    bool gridNoteAuditionActive = false;
    bool gridNoteAuditionSelectionMode = false;
    int gridNoteAuditionRow = -1;
    int gridNoteAuditionChannel = -1;
    int gridNoteAuditionNote = -1;
    int gridNoteAuditionOctave = -1;
    int gridNoteAuditionInstrument = 0;
    int gridNoteAuditionVolume = 0x7F;
    float dragStartY = 0.0f;
    float dragLastY = 0.0f;
    float dragStartScrollY = 0.0f;
    bool scrollbarDragging = false;
    float scrollbarGrabOffsetY = 0.0f;
    bool loopSelecting = false;
    bool loopMoving = false;
    bool loopEnabled = false;
    bool loopRangeDirty = false;
    bool patternDirty = false;
    bool songLengthDirty = false;
    bool playbackArrangementDirty = false;
    bool copyOnWriteRequested = false;
    bool songSaveRequested = false;
    bool songSaveWindowOpen = false;
    bool songSaveWindowRequested = false;
    bool songSaveOverwriteConfirmWindowOpen = false;
    bool songSaveOverwriteConfirmWindowRequested = false;
    bool songSaveOverwriteConfirmed = false;
    bool songDownloadRequested = false;
    bool songLoadRequested = false;
    bool songUploadRequested = false;
    char songLoadStatus[512] = {};
    bool songLoadErrorWindowOpen = false;
    bool songLoadErrorWindowRequested = false;
    char songLoadErrorText[2048] = {};
    int loadGreetingMuteFrames = 0;
    bool musicStartRequested = false;
    bool musicPlayRequested = false;
    bool musicStopRequested = false;
    bool musicSeekRequested = false;
    int musicSeekRow = 0;
    int musicSeekTick = 0;
    bool previewNoteRequested = false;
    bool previewHeldNoteStartRequested = false;
    bool previewHeldNoteStopRequested = false;
    bool previewHeldNotesStopAllRequested = false;
    int previewNote = 0;
    int previewOctave = 4;
    int previewInstrument = 0;
    int previewVolume = 0x7F;
    bool virtualKeyPointerDown = false;
    bool virtualKeyRootFingerActive = false;
    SDL_FingerID virtualKeyRootFingerId = 0;
    bool furnaceKeyboardSpaceDown = false;
    bool furnaceKeyboardKeyActive[SDL_NUM_SCANCODES] = {};
    int furnaceKeyboardKeyNote[SDL_NUM_SCANCODES] = {};
    int furnaceKeyboardKeyOctave[SDL_NUM_SCANCODES] = {};
    bool effectActivePointerDown = false;
    int loopAnchor = 0;
    int loopMoveGrabOffset = 0;
    int loopMoveLength = 1;
    float loopSelectLocalY = 0.0f;
    float loopSelectViewportHeight = 0.0f;
    bool channelSelectionEnabled = false;
    bool channelSelecting = false;
    int channelAnchor = 0;
    int channelStart = 0;
    int channelEnd = TRACKER_CHANNELS - 1;
    bool channelSoloApplied = false;
    int channelSoloAppliedStart = 0;
    int channelSoloAppliedEnd = TRACKER_CHANNELS - 1;
    bool editSelectionEnabled = false;
    bool editSelectionValid = false;
    bool editSelecting = false;
    bool editMoving = false;
    int editSelectionAnchorRow = 0;
    int editSelectionAnchorChannel = 0;
    int editSelectionCurrentRow = 0;
    int editSelectionCurrentChannel = 0;
    int editSelectionStartRow = 0;
    int editSelectionEndRow = 0;
    int editSelectionStartChannel = 0;
    int editSelectionEndChannel = 0;
    int editSelectionAnchorPart = 0;
    int editMoveGrabRowOffset = 0;
    int editMoveGrabChannelOffset = 0;
    int editMoveBaseStartRow = 0;
    int editMoveBaseStartChannel = 0;
    int editMovePointerStartRow = 0;
    int editMovePointerStartChannel = 0;
    float editSelectLocalX = 0.0f;
    float editSelectLocalY = 0.0f;
    float editSelectViewportWidth = 0.0f;
    float editSelectViewportHeight = 0.0f;
    float clipboardCutCooldown = 0.0f;
    float clipboardBannerFlashTime = 0.0f;
    int clipboardBannerKind = TRACKER_CLIPBOARD_BANNER_NONE;
    bool clipboardBannerUsesEditSelection = false;
    char clipboardBannerText[64] = {};
    TrackerClipboard clipboard = {};
    TrackerCellFlashRange cellFlashes[TRACKER_CELL_FLASH_RANGE_COUNT] = {};
    TrackerPartFlash partFlashes[TRACKER_PART_FLASH_COUNT] = {};
    int nextCellFlash = 0;
    int nextPartFlash = 0;
    float editorInstrumentSelectorFlashTime = 0.0f;
    uint8_t editorInstrumentSelectorFlashKind = TRACKER_CHANGE_FLASH_NONE;
    float instrumentFlashTime[256] = {};
    uint8_t instrumentFlashKind[256] = {};

    bool playing = false;
    bool followCursor = true;
    int playRow = 0;
    int playTick = 0;
    int ticksPerRow = 6;
    int songTickRate = 60;
    int songSpeed = 6;
    int songRowsPerBeat = 4;
    int songScaleRoot = 0;
    int songScaleMode = TRACKER_SONG_SCALE_CHROMATIC;
    bool songLfoEnabled = false;
    int songLfoFrequency = 0;
    int loopStart = 0;
    int loopEnd = 31;

    bool editorOpen = false;
    bool editorWindowRequested = false;
    bool instrumentEditorOpen = false;
    bool instrumentEditorWindowRequested = false;
    bool instrumentEditorOpenedFromCellEditor = false;
    bool instrumentEditorOpenedFromInstrumentsWindow = false;
    bool instrumentColorWindowOpen = false;
    bool instrumentColorWindowRequested = false;
    bool instrumentsWindowOpen = false;
    bool instrumentsWindowRequested = false;
    bool songLoadWindowOpen = false;
    bool songLoadWindowRequested = false;
    bool songDeleteConfirmWindowOpen = false;
    bool songDeleteConfirmWindowRequested = false;
    bool songDeleteRequested = false;
    int songLoadTab = 0; // 0 my songs, 1 builtin songs, 2 builtin sfx
    int songSelectedMySong = -1;
    int songDeleteIndex = -1;
    int songSelectedBuiltinSong = 0;
    int songSelectedBuiltinSfx = 0;
    int savedSongCount = 0;
    char savedSongNames[TRACKER_SAVED_SONG_LIST_CAPACITY][TRACKER_SAVED_SONG_NAME_CAPACITY] = {};
    char songDeleteName[TRACKER_SAVED_SONG_NAME_CAPACITY] = {};
    char songStorageFilename[TRACKER_SAVED_SONG_NAME_CAPACITY] = "MY_SONG";
    int32_t songStorageFilenameLen = 7;
    float songBrowserScrollY = 0.0f;
    float songBrowserScrollVelocity = 0.0f;
    float songBrowserViewportHeight = 300.0f;
    float songBrowserRowHeight = 44.0f;
    float songBrowserContentHeight = 0.0f;
    bool songBrowserDragging = false;
    bool songBrowserDragMoved = false;
    float songBrowserDragStartY = 0.0f;
    float songBrowserDragLastY = 0.0f;
    bool songBrowserScrollbarDragging = false;
    float songBrowserScrollbarGrabOffsetY = 0.0f;
    bool songSettingsWindowOpen = false;
    bool songSettingsWindowRequested = false;
    bool songLoadEmptyRequested = false;
    bool partEditorOpen = false;
    bool partEditorWindowRequested = false;
    int partEditorPart = -1;
    bool operatorEditorOpen = false;
    bool operatorEditorWindowRequested = false;
    bool oscilloscopeVisible = false;
    bool oscilloscopeMaximized = false;
    bool oscilloscopeDragging = false;
    bool oscilloscopeDragMoved = false;
    bool oscilloscopeSnappedToPortrait = true;
    bool oscilloscopeInitialized = false;
    int oscilloscopeSelectedChannel = 0;
    uint64_t oscilloscopeInputCooldownUntil = 0;
    float oscilloscopeX = 0.0f;
    float oscilloscopeY = 0.0f;
    float oscilloscopeDragOffsetX = 0.0f;
    float oscilloscopeDragOffsetY = 0.0f;
    float oscilloscopeDragStartX = 0.0f;
    float oscilloscopeDragStartY = 0.0f;
    int editorTab = 0; // 0 note, 1 effects
    int editRow = 0;
    int editChannel = 0;
    int editOctave = 3;
    int editNote = 0;
    int editInstrument = 0;
    int editVolume = 0x7F;
    bool editInstrumentExplicit = false;
    bool editVolumeExplicit = false;
    int editSpecial = 0; // 0 note, 1 OFF, 2 REL, 3 ===, 4 ...
    int editEffect = 0;
    int editEffectParamA = 0;
    int editEffectParamB = 0;
    bool editEffectActive[TRACKER_MAX_EFFECT_SLOTS] = {};
    uint8_t editEffectCodes[TRACKER_MAX_EFFECT_SLOTS] = {};
    uint8_t editEffectValues[TRACKER_MAX_EFFECT_SLOTS] = {};
    uint8_t editEffectValuesByDef[TRACKER_EFFECT_DEF_COUNT] = {};
    int editOperator = 0;
    int instrumentEditorTab = 0; // 0 patch, 1 effects/macros
    int editMacroTarget = XFM_MACRO_TL1;
    int editMacroValueIndex = 0;
    int macroSelectMode = TRACKER_MACRO_SELECT_LOOP;
    int macroViewFirst = 0;
    float macroViewAnimatedFirst = 0.0f;
    float macroViewportWidth = 0.0f;
    int macroValueViewMin = -12;
    bool macroDrawing = false;
    bool macroRangeSelecting = false;
    int macroRangeAnchor = 0;
    int macroRangeAutoScrollDir = 0;
    float macroRangeAutoScrollTimer = 0.0f;
    bool sliderDragging = false;
    Clay_ElementId sliderActiveId = {};
    int usedInstruments[TRACKER_MAX_USED_INSTRUMENTS] = {};
    int usedInstrumentCount = 0;
    bool availableInstruments[256] = {};
    int availableInstrumentCount = 0;
    char instrumentNames[256][TRACKER_INSTRUMENT_NAME_CAPACITY] = {};
    int32_t instrumentNameLengths[256] = {};
    uint32_t instrumentColors[256] = {};
    float instrumentsScrollY = 0.0f;
    float instrumentsScrollVelocity = 0.0f;
    float instrumentsViewportHeight = 360.0f;
    float instrumentsRowHeight = 54.0f;
    float instrumentsContentHeight = 0.0f;
    bool instrumentsDragging = false;
    bool instrumentsDragMoved = false;
    float instrumentsDragStartY = 0.0f;
    float instrumentsDragLastY = 0.0f;
    bool instrumentsScrollbarDragging = false;
    float instrumentsScrollbarGrabOffsetY = 0.0f;
    int pendingInstrumentAction = 0; // 1 clone, 2 rename, 3 new
    bool pendingInstrumentKeypadOpen = false;
    int pendingInstrument = 0;
    int pendingInstrumentTarget = -1;
    char pendingInstrumentName[TRACKER_INSTRUMENT_NAME_CAPACITY] = {};
    int32_t pendingInstrumentNameLen = 0;
    bool pendingSongNameKeypadOpen = false;
    bool pendingSongNameKeypadActive = false;
    char pendingSongName[TRACKER_SONG_NAME_CAPACITY] = {};
    int32_t pendingSongNameLen = 0;
    int pendingPartAction = 0; // 1 rename
    int pendingPart = -1;
    bool pendingPartNameKeypadOpen = false;
    bool pendingPartNameKeypadActive = false;
    char pendingPartName[TRACKER_PART_NAME_CAPACITY] = {};
    int32_t pendingPartNameLen = 0;
    xfm_patch_opn editPatches[256] = {};
    bool editPatchValid[256] = {};
    bool editPatchDirty[256] = {};
    XfmMacro editMacros[256][XFM_MACRO_TARGET_COUNT] = {};
    bool editMacroEnabled[256][XFM_MACRO_TARGET_COUNT] = {};
    bool editMacroValid[256][XFM_MACRO_TARGET_COUNT] = {};
    bool editMacroDirty[256][XFM_MACRO_TARGET_COUNT] = {};

    Clayton_Click closeButton;
    Clayton_Click playButton;
    Clayton_Click stopButton;
    Clayton_Click followButton;
    Clayton_Click clearLoopButton;
    Clayton_Click addRowButton;
    Clayton_Click removeRowButton;
    Clayton_Click addPartButton;
    Clayton_Click partToggleButtons[TRACKER_MAX_PARTS];
    Clayton_Click partEnableButtons[TRACKER_MAX_PARTS];
    Clayton_Click partRenameButtons[TRACKER_MAX_PARTS];
    Clayton_Click partAddRowButtons[TRACKER_MAX_PARTS];
    Clayton_Click partRemoveRowButtons[TRACKER_MAX_PARTS];
    Clayton_Click partUpButtons[TRACKER_MAX_PARTS];
    Clayton_Click partDownButtons[TRACKER_MAX_PARTS];
    Clayton_Click partDeleteButtons[TRACKER_MAX_PARTS];
    Clayton_Click partSettingsButtons[TRACKER_MAX_PARTS];
    Clayton_Click stickyPartToggleButton;
    Clayton_Click stickyPartEnableButton;
    Clayton_Click stickyPartRenameButton;
    Clayton_Click stickyPartAddRowButton;
    Clayton_Click stickyPartRemoveRowButton;
    Clayton_Click stickyPartUpButton;
    Clayton_Click stickyPartDownButton;
    Clayton_Click stickyPartDeleteButton;
    Clayton_Click stickyPartSettingsButton;
    Clayton_Click songButtons[TRACKER_MAX_SONG_COUNT];
    Clayton_Click saveSongButton;
    Clayton_Click loadSongButton;
    Clayton_Click songSaveCloseButton;
    Clayton_Click songSaveRenameButton;
    Clayton_Click songSaveConfirmButton;
    Clayton_Click songSaveOverwriteConfirmButton;
    Clayton_Click songSaveOverwriteCancelButton;
    Clayton_Click songDownloadButton;
    Clayton_Click songLoadCloseButton;
    Clayton_Click songLoadTabButtons[3];
    Clayton_Click songLoadConfirmButton;
    Clayton_Click songUploadButton;
    Clayton_Click songMySongRowClicks[TRACKER_SAVED_SONG_LIST_CAPACITY];
    Clayton_Click songMySongDeleteButtons[TRACKER_SAVED_SONG_LIST_CAPACITY];
    Clayton_Click songDeleteConfirmButton;
    Clayton_Click songDeleteCancelButton;
    Clayton_Click songBuiltinSongRowClicks[TRACKER_MAX_SONG_COUNT];
    Clayton_Click songBuiltinSfxRowClicks[TRACKER_SAVED_SONG_LIST_CAPACITY];
    Clayton_Click loadErrorOkButton;
    Clayton_Click copyButton;
    Clayton_Click cutButton;
    Clayton_Click pasteButton;
    Clayton_Click editSelectionButton;
    Clayton_Click instrumentsButton;
    Clayton_Click songSettingsButton;
    Clayton_Click oscilloscopeButton;
    Clayton_Click editorCloseButton;
    Clayton_Click editorNoteTabButton;
    Clayton_Click editorEffectsTabButton;
    Clayton_Click editorCancelButton;
    Clayton_Click instrumentExplicitButton;
    Clayton_Click volumeExplicitButton;
    Clayton_Click effectPrevButton;
    Clayton_Click effectNextButton;
    Clayton_Click instrumentPrevButton;
    Clayton_Click instrumentNextButton;
    Clayton_Click instrumentNameButton;
    Clayton_Click instrumentEditorCloseButton;
    Clayton_Click instrumentManagementCloneButton;
    Clayton_Click instrumentManagementRenameButton;
    Clayton_Click instrumentManagementDeleteButton;
    Clayton_Click instrumentManagementNewButton;
    Clayton_Click instrumentManagementEditButton;
    Clayton_Click instrumentRowClicks[256];
    Clayton_Click instrumentColorButton;
    Clayton_Click instrumentColorCloseButton;
    Clayton_Click instrumentsCloseButton;
    Clayton_Click songSettingsCloseButton;
    Clayton_Click songLoadEmptyButton;
    Clayton_Click partEditorCloseButton;
    Clayton_Click partEditorNameButton;
    Clayton_Click partEditorEnableButton;
    Clayton_Click partEditorRowsMinusButton;
    Clayton_Click partEditorRowsPlusButton;
    Clayton_Click partEditorCloneButton;
    Clayton_Click partEditorDeleteButton;
    Clayton_Click songNameButton;
    Clayton_Click songScaleRootPrevButton;
    Clayton_Click songScaleRootNextButton;
    Clayton_Click songScalePrevButton;
    Clayton_Click songScaleNextButton;
    Clayton_Click songLfoButton;
    Clayton_Click instrumentUpButtons[256];
    Clayton_Click instrumentDownButtons[256];
    Clayton_Click instrumentPatchTabButton;
    Clayton_Click instrumentEffectsTabButton;
    Clayton_Click instrumentAlgoPrevButton;
    Clayton_Click instrumentAlgoNextButton;
    Clayton_Click macroTargetPrevButton;
    Clayton_Click macroTargetNextButton;
    Clayton_Click macroEnableButton;
    Clayton_Click macroScrollPrevButton;
    Clayton_Click macroScrollNextButton;
    Clayton_Click macroValueScrollUpButton;
    Clayton_Click macroValueScrollDownButton;
    Clayton_Click macroStepPrevButton;
    Clayton_Click macroStepNextButton;
    Clayton_Click macroLoopButton;
    Clayton_Click macroReleaseButton;
    Clayton_Click operatorButtons[4];
    Clayton_Click operatorEditorPrevButton;
    Clayton_Click operatorEditorNextButton;
    Clayton_Click operatorEditorCloseButton;
    Clayton_Click operatorSsgPrevButton;
    Clayton_Click operatorSsgNextButton;
    Clayton_Click operatorAmButton;

    uint16_t keyHeight;
};

inline void Tracker_CancelCellMovePending(Tracker *self);
inline void Tracker_BeginCellMove(Tracker *self, int row, int channel);
inline void setTrackerCursorState(Tracker *self, int row, int tick, int ticksPerRow);
inline void Tracker_RequestMusicSeekToCursor(Tracker *self);

inline void Tracker_FlashCellRange(
    Tracker *self,
    int rowStart,
    int rowEnd,
    int channelStart,
    int channelEnd,
    TrackerChangeFlashKind kind
)
{
    if (!self || kind == TRACKER_CHANGE_FLASH_NONE) return;
    rowStart = std::max(0, std::min(std::max(0, self->rowCount - 1), rowStart));
    rowEnd = std::max(0, std::min(std::max(0, self->rowCount - 1), rowEnd));
    channelStart = std::max(0, std::min(TRACKER_CHANNELS - 1, channelStart));
    channelEnd = std::max(0, std::min(TRACKER_CHANNELS - 1, channelEnd));
    if (rowEnd < rowStart) std::swap(rowStart, rowEnd);
    if (channelEnd < channelStart) std::swap(channelStart, channelEnd);

    TrackerCellFlashRange &flash = self->cellFlashes[self->nextCellFlash % TRACKER_CELL_FLASH_RANGE_COUNT];
    self->nextCellFlash = (self->nextCellFlash + 1) % TRACKER_CELL_FLASH_RANGE_COUNT;
    flash.timeLeft = TRACKER_CHANGE_FLASH_DURATION_S;
    flash.kind = kind;
    flash.rowStart = rowStart;
    flash.rowEnd = rowEnd;
    flash.channelStart = channelStart;
    flash.channelEnd = channelEnd;
}

inline void Tracker_FlashCell(Tracker *self, int row, int channel, TrackerChangeFlashKind kind)
{
    Tracker_FlashCellRange(self, row, row, channel, channel, kind);
}

inline bool Tracker_PartHasActiveAddFlash(const Tracker *self, int partIndex)
{
    if (!self) return false;
    for (const TrackerPartFlash &flash : self->partFlashes)
    {
        if (flash.timeLeft > 0.0f && flash.partIndex == partIndex && flash.kind == TRACKER_CHANGE_FLASH_ADD)
            return true;
    }
    return false;
}

inline void Tracker_FlashPart(Tracker *self, int partIndex, TrackerChangeFlashKind kind, bool preserveActiveAdd = true)
{
    if (!self || kind == TRACKER_CHANGE_FLASH_NONE || partIndex < 0 || partIndex >= self->partCount)
        return;
    if (preserveActiveAdd && kind == TRACKER_CHANGE_FLASH_EDIT && Tracker_PartHasActiveAddFlash(self, partIndex))
        return;

    TrackerPartFlash &flash = self->partFlashes[self->nextPartFlash % TRACKER_PART_FLASH_COUNT];
    self->nextPartFlash = (self->nextPartFlash + 1) % TRACKER_PART_FLASH_COUNT;
    flash.timeLeft = TRACKER_CHANGE_FLASH_DURATION_S;
    flash.kind = kind;
    flash.partIndex = partIndex;
}

inline void Tracker_FlashInstrument(Tracker *self, int instrument, TrackerChangeFlashKind kind)
{
    if (!self || kind == TRACKER_CHANGE_FLASH_NONE) return;
    instrument = std::max(0, std::min(255, instrument));
    self->instrumentFlashTime[instrument] = TRACKER_CHANGE_FLASH_DURATION_S;
    self->instrumentFlashKind[instrument] = kind;
}

inline void Tracker_FlashEditorInstrumentSelector(Tracker *self, TrackerChangeFlashKind kind)
{
    if (!self || kind == TRACKER_CHANGE_FLASH_NONE) return;
    self->editorInstrumentSelectorFlashTime = TRACKER_CHANGE_FLASH_DURATION_S;
    self->editorInstrumentSelectorFlashKind = kind;
}

inline float Tracker_ChangeFlashAlpha(float timeLeft)
{
    if (timeLeft <= 0.0f) return 0.0f;
    const float remaining01 = std::max(0.0f, std::min(1.0f, timeLeft / TRACKER_CHANGE_FLASH_DURATION_S));
    const float elapsed = TRACKER_CHANGE_FLASH_DURATION_S - timeLeft;
    const float pulse = 0.5f + 0.5f * std::sin(elapsed * 6.28318530718f * 3.0f);
    return remaining01 * (0.32f + pulse * 0.68f);
}

inline float Tracker_CellFlashAlpha(const Tracker *self, int row, int channel, uint8_t *outKind = nullptr, float *outTimeLeft = nullptr)
{
    if (!self) return 0.0f;
    float best = 0.0f;
    float bestTimeLeft = 0.0f;
    uint8_t bestKind = TRACKER_CHANGE_FLASH_NONE;
    for (const TrackerCellFlashRange &flash : self->cellFlashes)
    {
        if (flash.timeLeft <= 0.0f || row < flash.rowStart || row > flash.rowEnd ||
            channel < flash.channelStart || channel > flash.channelEnd)
            continue;
        const float alpha = Tracker_ChangeFlashAlpha(flash.timeLeft);
        if (alpha > best)
        {
            best = alpha;
            bestTimeLeft = flash.timeLeft;
            bestKind = flash.kind;
        }
    }
    if (outKind) *outKind = bestKind;
    if (outTimeLeft) *outTimeLeft = bestTimeLeft;
    return best;
}

inline float Tracker_PartFlashAlpha(const Tracker *self, int partIndex, uint8_t *outKind = nullptr, float *outTimeLeft = nullptr)
{
    if (!self) return 0.0f;
    float best = 0.0f;
    float bestTimeLeft = 0.0f;
    uint8_t bestKind = TRACKER_CHANGE_FLASH_NONE;
    for (const TrackerPartFlash &flash : self->partFlashes)
    {
        if (flash.timeLeft <= 0.0f || flash.partIndex != partIndex)
            continue;
        const float alpha = Tracker_ChangeFlashAlpha(flash.timeLeft);
        if (alpha > best)
        {
            best = alpha;
            bestTimeLeft = flash.timeLeft;
            bestKind = flash.kind;
        }
    }
    if (outKind) *outKind = bestKind;
    if (outTimeLeft) *outTimeLeft = bestTimeLeft;
    return best;
}

inline void Tracker_TickChangeFlashes(Tracker *self, float dt)
{
    if (!self) return;
    for (TrackerCellFlashRange &flash : self->cellFlashes)
    {
        if (flash.timeLeft > 0.0f)
        {
            flash.timeLeft = std::max(0.0f, flash.timeLeft - dt);
            if (flash.timeLeft <= 0.0f)
                flash.kind = TRACKER_CHANGE_FLASH_NONE;
        }
    }
    for (TrackerPartFlash &flash : self->partFlashes)
    {
        if (flash.timeLeft > 0.0f)
        {
            flash.timeLeft = std::max(0.0f, flash.timeLeft - dt);
            if (flash.timeLeft <= 0.0f)
                flash.kind = TRACKER_CHANGE_FLASH_NONE;
        }
    }
    if (self->editorInstrumentSelectorFlashTime > 0.0f)
    {
        self->editorInstrumentSelectorFlashTime = std::max(0.0f, self->editorInstrumentSelectorFlashTime - dt);
        if (self->editorInstrumentSelectorFlashTime <= 0.0f)
            self->editorInstrumentSelectorFlashKind = TRACKER_CHANGE_FLASH_NONE;
    }
    for (int i = 0; i < 256; i++)
    {
        if (self->instrumentFlashTime[i] > 0.0f)
        {
            self->instrumentFlashTime[i] = std::max(0.0f, self->instrumentFlashTime[i] - dt);
            if (self->instrumentFlashTime[i] <= 0.0f)
                self->instrumentFlashKind[i] = TRACKER_CHANGE_FLASH_NONE;
        }
    }
}

inline uint64_t Tracker_NowMs()
{
    return (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

inline const char *Tracker_SongPattern(int songIndex)
{
    const BuiltinSongDefinition *song = BuiltinSong_BySongId(songIndex);
    return song ? song->pattern : BUILTIN_SONG_REGISTRY[0].pattern;
}

inline const char *Tracker_SongInstruments(int songIndex)
{
    const BuiltinSongDefinition *song = BuiltinSong_BySongId(songIndex);
    return song ? song->instruments : BUILTIN_SONG_REGISTRY[0].instruments;
}

inline const char *Tracker_SongName(int songIndex)
{
    const BuiltinSongDefinition *song = BuiltinSong_BySongId(songIndex);
    return song ? song->displayName : BUILTIN_SONG_REGISTRY[0].displayName;
}

inline int Tracker_DefaultSongSpeed(int songIndex)
{
    const BuiltinSongDefinition *song = BuiltinSong_BySongId(songIndex);
    return song ? song->speed : BUILTIN_SONG_REGISTRY[0].speed;
}

inline void Tracker_SetSongMetadata(
    Tracker *self,
    int tickRate,
    int speed,
    int rowsPerBeat,
    int scaleRoot,
    int scaleMode,
    bool lfoEnabled,
    int lfoFrequency)
{
    if (!self)
        return;
    self->songTickRate = std::max(1, std::min(300, tickRate));
    self->songSpeed = std::max(1, std::min(32, speed));
    self->ticksPerRow = self->songSpeed;
    self->songRowsPerBeat = std::max(1, std::min(32, rowsPerBeat));
    self->songScaleRoot = Tracker_ClampSongScaleRoot(scaleRoot);
    self->songScaleMode = Tracker_ClampSongScaleMode(scaleMode);
    self->songLfoEnabled = lfoEnabled;
    self->songLfoFrequency = std::max(0, std::min(7, lfoFrequency));
}

inline void Tracker_SetSongMetadata(Tracker *self, const TrackerSongLoadResult &loaded)
{
    Tracker_SetSongMetadata(
        self,
        loaded.songTickRate,
        loaded.songSpeed,
        loaded.songRowsPerBeat,
        loaded.songScaleRoot,
        loaded.songScaleMode,
        loaded.songLfoEnabled,
        loaded.songLfoFrequency);
}

inline void Tracker_MarkSongMetadataChanged(Tracker *self)
{
    if (!self)
        return;
    self->patternDirty = true;
    self->copyOnWriteRequested = true;
}

inline void Tracker_ApplyBuiltinSongMetadata(Tracker *self, int songIndex)
{
    if (!self)
        return;
    const BuiltinSongDefinition *song = BuiltinSong_BySongId(songIndex);
    if (!song)
        return;
    Tracker_SetSongMetadata(
        self,
        song->tickRate,
        song->speed,
        song->rowsPerBeat,
        song->scaleRoot,
        song->scaleMode,
        song->lfoEnabled,
        song->lfoFrequency);
}

inline const char *Tracker_DefaultInstrumentName(int instrument)
{
    (void)instrument;
    return "Instrument";
}

inline float Tracker_OpnLfoFrequencyHz(int index)
{
    static constexpr float hz[8] = {3.98f, 5.56f, 6.02f, 6.37f, 6.88f, 9.63f, 48.1f, 72.2f};
    return hz[std::max(0, std::min(7, index))];
}

static constexpr uint32_t TRACKER_INSTRUMENT_COLOR_PALETTE[64] = {
    0xFF1744, 0xFF3D00, 0xFF6D00, 0xFFAB00, 0xFFD600, 0xC6FF00, 0x76FF03, 0x00E676,
    0x00E5A8, 0x00E5FF, 0x00B0FF, 0x2979FF, 0x3D5AFE, 0x651FFF, 0xD500F9, 0xFF00A8,
    0xFF5252, 0xFF7043, 0xFF9100, 0xFFC400, 0xFFFF00, 0xB2FF59, 0x69F0AE, 0x1DE9B6,
    0x18FFFF, 0x40C4FF, 0x448AFF, 0x536DFE, 0x7C4DFF, 0xE040FB, 0xFF40C4, 0xFF4081,
    0xFF8A80, 0xFF9E80, 0xFFD180, 0xFFE57F, 0xFFFF8D, 0xCCFF90, 0xB9F6CA, 0xA7FFEB,
    0x84FFFF, 0x80D8FF, 0x82B1FF, 0x8C9EFF, 0xB388FF, 0xEA80FC, 0xFF80AB, 0xFF8AAB,
    0xF50057, 0xDD2C00, 0xFFB300, 0xAEEA00, 0x64DD17, 0x00C853, 0x00BFA5, 0x00B8D4,
    0x0091EA, 0x304FFE, 0x6200EA, 0xAA00FF, 0xC51162, 0xFFEA00, 0x00FF6A, 0xFFFFFF
};

inline uint32_t Tracker_DefaultInstrumentColor(int instrument)
{
    int inst = std::max(0, std::min(255, instrument));
    return TRACKER_INSTRUMENT_COLOR_PALETTE[inst & 63];
}

inline uint32_t Tracker_InstrumentColorU32(const Tracker *self, int instrument)
{
    int inst = std::max(0, std::min(255, instrument));
    if (self && self->instrumentColors[inst] != 0)
        return self->instrumentColors[inst];
    return Tracker_DefaultInstrumentColor(inst);
}

inline void Tracker_SetInstrumentColor(Tracker *self, int instrument, uint32_t rgb)
{
    if (!self) return;
    int inst = std::max(0, std::min(255, instrument));
    self->instrumentColors[inst] = rgb & 0xFFFFFFu;
    self->patternDirty = true;
    self->copyOnWriteRequested = true;
}

inline const char *Tracker_InstrumentName(const Tracker *self, int instrument)
{
    int inst = std::max(0, std::min(255, instrument));
    if (self && self->instrumentNameLengths[inst] > 0)
        return self->instrumentNames[inst];
    return Tracker_DefaultInstrumentName(inst);
}

inline void Tracker_SetInstrumentName(Tracker *self, int instrument, const char *name, int32_t len)
{
    if (!self) return;
    int inst = std::max(0, std::min(255, instrument));
    if (!name) name = "";
    len = std::max((int32_t)0, std::min(len, (int32_t)TRACKER_INSTRUMENT_NAME_CAPACITY - 1));
    std::memset(self->instrumentNames[inst], 0, TRACKER_INSTRUMENT_NAME_CAPACITY);
    if (len > 0)
        std::memcpy(self->instrumentNames[inst], name, (size_t)len);
    self->instrumentNameLengths[inst] = len;
}


inline void Tracker_Clear(Tracker *self)
{
    if (!self) return;
    for (int r = 0; r < TRACKER_MAX_ROWS; r++)
        for (int ch = 0; ch < TRACKER_CHANNELS; ch++)
            std::strncpy(self->cells[r][ch].text, ".......", TRACKER_CELL_CHARS);
}

inline int Tracker_ParseLeadingRowCount(const char *pattern)
{
    if (!pattern) return 32;
    const char *p = pattern;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    int rows = 0;
    while (*p >= '0' && *p <= '9')
    {
        rows = rows * 10 + (*p - '0');
        p++;
    }
    if (rows <= 0) rows = 32;
    return std::min(rows, TRACKER_MAX_ROWS);
}

inline float Tracker_MaxScroll(const Tracker *self);
inline bool Tracker_HasPlaySelection(const Tracker *self);

inline bool Tracker_IsHex(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

inline int Tracker_HexValue(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + c - 'a';
    if (c >= 'A' && c <= 'F') return 10 + c - 'A';
    return 0;
}

inline int Tracker_ParseHexByte(const char *s)
{
    if (!s || !Tracker_IsHex(s[0]) || !Tracker_IsHex(s[1])) return -1;
    return (Tracker_HexValue(s[0]) << 4) | Tracker_HexValue(s[1]);
}

inline void Tracker_WriteHexByte(char *s, int value)
{
    static const char *hex = "0123456789ABCDEF";
    if (!s) return;
    value = std::max(0, std::min(255, value));
    s[0] = hex[(value >> 4) & 0x0F];
    s[1] = hex[value & 0x0F];
}

inline const TrackerEffectDef *Tracker_EffectDefByCode(uint8_t code)
{
    for (int i = 0; i < TRACKER_EFFECT_DEF_COUNT; i++)
        if (TRACKER_EFFECT_DEFS[i].code == code)
            return &TRACKER_EFFECT_DEFS[i];
    return &TRACKER_EFFECT_DEFS[0];
}

inline int Tracker_EffectDefIndexByCode(uint8_t code)
{
    for (int i = 0; i < TRACKER_EFFECT_DEF_COUNT; i++)
        if (TRACKER_EFFECT_DEFS[i].code == code)
            return i;
    return 0;
}

inline int Tracker_NextEffectDefIndex(uint8_t code, int direction)
{
    int idx = Tracker_EffectDefIndexByCode(code);
    if (idx <= 0) idx = 1;
    int count = std::max(1, TRACKER_EFFECT_DEF_COUNT - 1);
    int zeroBased = idx - 1;
    int dir = direction < 0 ? -1 : 1;
    zeroBased = (zeroBased + dir + count) % count;
    return zeroBased + 1;
}

inline int Tracker_SelectedEffectDefIndex(const Tracker *self)
{
    if (!self) return 1;
    int idx = self->editEffect;
    if (idx <= 0 || idx >= TRACKER_EFFECT_DEF_COUNT)
        idx = 1;
    return idx;
}

inline uint8_t Tracker_SelectedEffectCode(const Tracker *self)
{
    return TRACKER_EFFECT_DEFS[Tracker_SelectedEffectDefIndex(self)].code;
}

inline int Tracker_FindActiveEffectSlot(const Tracker *self, uint8_t code)
{
    if (!self || code == TRACKER_EFFECT_CODE_NONE) return -1;
    for (int i = 0; i < TRACKER_MAX_EFFECT_SLOTS; i++)
        if (self->editEffectActive[i] && self->editEffectCodes[i] == code)
            return i;
    return -1;
}

inline int Tracker_ActiveEffectCount(const Tracker *self)
{
    if (!self) return 0;
    int count = 0;
    for (int i = 0; i < TRACKER_MAX_EFFECT_SLOTS; i++)
        if (self->editEffectActive[i] && self->editEffectCodes[i] != TRACKER_EFFECT_CODE_NONE)
            count++;
    return count;
}

inline int Tracker_FirstFreeEffectSlot(const Tracker *self)
{
    if (!self) return -1;
    for (int i = 0; i < TRACKER_MAX_EFFECT_SLOTS; i++)
        if (!self->editEffectActive[i] || self->editEffectCodes[i] == TRACKER_EFFECT_CODE_NONE)
            return i;
    return -1;
}

inline uint8_t Tracker_SelectedEffectValue(const Tracker *self)
{
    if (!self) return 0;
    int idx = Tracker_SelectedEffectDefIndex(self);
    uint8_t code = TRACKER_EFFECT_DEFS[idx].code;
    int slot = Tracker_FindActiveEffectSlot(self, code);
    return slot >= 0 ? self->editEffectValues[slot] : self->editEffectValuesByDef[idx];
}

inline bool Tracker_SelectedEffectActive(const Tracker *self)
{
    return Tracker_FindActiveEffectSlot(self, Tracker_SelectedEffectCode(self)) >= 0;
}

inline void Tracker_SetSelectedEffectValue(Tracker *self, uint8_t value)
{
    if (!self) return;
    int idx = Tracker_SelectedEffectDefIndex(self);
    uint8_t code = TRACKER_EFFECT_DEFS[idx].code;
    self->editEffectValuesByDef[idx] = value;
    int slot = Tracker_FindActiveEffectSlot(self, code);
    if (slot >= 0)
        self->editEffectValues[slot] = value;
}

inline void Tracker_PromoteActiveEffectToFront(Tracker *self, uint8_t code)
{
    if (!self || code == TRACKER_EFFECT_CODE_NONE) return;
    int slot = Tracker_FindActiveEffectSlot(self, code);
    if (slot <= 0) return;
    std::swap(self->editEffectActive[0], self->editEffectActive[slot]);
    std::swap(self->editEffectCodes[0], self->editEffectCodes[slot]);
    std::swap(self->editEffectValues[0], self->editEffectValues[slot]);
}

inline void Tracker_ToggleSelectedEffectActive(Tracker *self)
{
    if (!self) return;
    int idx = Tracker_SelectedEffectDefIndex(self);
    uint8_t code = TRACKER_EFFECT_DEFS[idx].code;
    if (code == TRACKER_EFFECT_CODE_NONE) return;
    int slot = Tracker_FindActiveEffectSlot(self, code);
    if (slot >= 0)
    {
        self->editEffectActive[slot] = false;
        return;
    }
    if (Tracker_ActiveEffectCount(self) >= TRACKER_CELL_ACTIVE_EFFECT_LIMIT)
        return;
    slot = Tracker_FirstFreeEffectSlot(self);
    if (slot < 0) return;
    self->editEffectActive[slot] = true;
    self->editEffectCodes[slot] = code;
    self->editEffectValues[slot] = self->editEffectValuesByDef[idx];
    Tracker_PromoteActiveEffectToFront(self, code);
}

inline int Tracker_EffectParamA(uint8_t value) { return value; }
inline int Tracker_EffectParamB(uint8_t value) { return value & 0x0F; }

inline bool Tracker_EffectUsesNibbles(const TrackerEffectDef *def)
{
    return def && def->paramCount == 2;
}

inline int Tracker_EffectDisplayA(const TrackerEffectDef *def, uint8_t value)
{
    return Tracker_EffectUsesNibbles(def) ? ((value >> 4) & 0x0F) : value;
}

inline int Tracker_EffectDisplayB(const TrackerEffectDef *def, uint8_t value)
{
    return value & 0x0F;
}

inline uint8_t Tracker_EffectSetA(const TrackerEffectDef *def, uint8_t oldValue, int param)
{
    if (Tracker_EffectUsesNibbles(def))
        return (uint8_t)(((param & 0x0F) << 4) | (oldValue & 0x0F));
    return (uint8_t)std::max(0, std::min(255, param));
}

inline uint8_t Tracker_EffectSetB(const TrackerEffectDef *def, uint8_t oldValue, int param)
{
    if (!Tracker_EffectUsesNibbles(def)) return oldValue;
    return (uint8_t)((oldValue & 0xF0) | (param & 0x0F));
}

inline bool Tracker_EffectAInRange(const TrackerEffectDef *def, uint8_t value)
{
    if (!def || def->paramCount == 0) return true;
    int v = Tracker_EffectDisplayA(def, value);
    return v >= def->minA && v <= def->maxA;
}

inline bool Tracker_EffectBInRange(const TrackerEffectDef *def, uint8_t value)
{
    if (!def || def->paramCount < 2) return true;
    int v = Tracker_EffectDisplayB(def, value);
    return v >= def->minB && v <= def->maxB;
}

inline uint8_t Tracker_ClampEffectValueToDef(const TrackerEffectDef *def, uint8_t value)
{
    if (!def || def->paramCount <= 0)
        return 0;
    if (!Tracker_EffectUsesNibbles(def))
    {
        int v = std::max((int)def->minA, std::min((int)def->maxA, (int)value));
        return (uint8_t)v;
    }
    int a = Tracker_EffectDisplayA(def, value);
    int b = Tracker_EffectDisplayB(def, value);
    a = std::max((int)def->minA, std::min((int)def->maxA, a));
    b = std::max((int)def->minB, std::min((int)def->maxB, b));
    return (uint8_t)(((a & 0x0F) << 4) | (b & 0x0F));
}

inline const char *Tracker_EffectDescription(uint8_t code)
{
    switch (code)
    {
    case TRACKER_EFFECT_CODE_NONE: return "No effect in this slot.";
    case 0x00: return "Arpeggio. A and B are semitone offsets from the row note. Pattern is base, +A, +B, one step per tracker tick. Note-scoped: it only applies when the same note row contains 00xy; later notes are plain unless they also carry 00xy.";
    case 0x01: return "Pitch slide up. xx is speed. Current implementation moves continuously at about 25 cents per second for each speed unit. Sticky per channel: keeps sliding until 00, another pitch motion effect, or song reset.";
    case 0x02: return "Pitch slide down. xx is speed. Current implementation moves continuously at about 25 cents per second for each speed unit. Sticky per channel: keeps sliding until 00, another pitch motion effect, or song reset.";
    case 0x03: return "Portamento. xx is speed. On a later note row, the channel glides toward the new note instead of hard retriggering. Current implementation reaches the target in about 16/xx rows. Sticky per channel until set to 00 or replaced by another pitch motion effect.";
    case 0x04: return "Vibrato. A is speed, B is depth. Speed is an LFO rate in Hz (about 0.8 + A*0.75). Depth is about 7 cents per step. Sticky per channel until changed or set to 0400.";
    case 0x07: return "Tremolo. A is speed, B is depth. Speed is an LFO rate in Hz (about 0.8 + A*0.75). Depth modulates carrier level; larger B means stronger volume wobble. Sticky per channel until changed or set to 0700.";
    case 0x08: return "Panning. A toggles left, B toggles right. Non-zero nibble means that side is enabled. This is remembered as channel state and stays until changed, even across later notes.";
    case 0x0A: return "Volume slide. A is up amount, B is down amount, combined as A-B. Current implementation changes channel volume by that amount over one row. Sticky per channel until changed or set to 0A00.";
    case 0x0C: return "Retrigger. xx is tracker ticks between remembered-note rekeys. Sticky per channel until 0C00, song reset, or playback reset. It uses the normal FM key-off/key-on path and replays the remembered note even if the channel is currently silent. We intentionally do not copy Furnace's historical PCM bug where finished samples fail to retrigger.";
    case 0xE1: return "Note slide up. A is speed, B is semitones. Slides once toward note * 2^(B/12). Current implementation reaches the target in about 16/A rows, then stops. Overrides continuous pitch slide while active.";
    case 0xE2: return "Note slide down. A is speed, B is semitones. Slides once toward note / 2^(B/12). Current implementation reaches the target in about 16/A rows, then stops. Overrides continuous pitch slide while active.";
    case 0xE5: return "Fine pitch. xx is signed with 80 as center, so 7F is about -1 cent and 81 about +1 cent. This is remembered per channel until changed by another E5 or by pitch macros.";
    case 0xEA: return "Legato toggle. 00 off, non-zero on. When on, later notes keep the current envelope instead of re-keying if legato playback is possible. Sticky per channel until changed.";
    case 0xEE: return "Patch morph toward the row instrument. 00 cancels morph. Non-zero values set morph speed; current implementation advances on tracker ticks and larger values reach the target faster. Morph edits the live channel patch only, not the stored instrument.";
    case 0xF5: return "Disable macro target. xx is the macro target id. This is remembered in the channel macro mask until the target is re-enabled, all song state is reset, or playback restarts.";
    case 0xF6: return "Enable macro target. xx is the macro target id. This clears the channel-side disable mask for that macro target and remains in effect until disabled again.";
    case 0x10: return "OPN chip LFO. A is on/off, B is chip LFO frequency 0..7. This is chip-global, not instrument-local: changing it affects the whole FM chip until another 10xy changes it.";
    case 0x11: return "Feedback. xx is FB 0..7. Writes live patch feedback on the current channel. It affects the sounding voice now, but the stored instrument is unchanged; a later note reload may replace it.";
    case 0x12: return "Operator 1 TL. xx is total level 0..127, where smaller is louder. Edits the live patch on the current channel only; it persists for the current live voice until another patch write or note reload changes it.";
    case 0x13: return "Operator 2 TL. xx is total level 0..127, where smaller is louder. Live channel patch only; not written back into the instrument.";
    case 0x14: return "Operator 3 TL. xx is total level 0..127, where smaller is louder. Live channel patch only; not written back into the instrument.";
    case 0x15: return "Operator 4 TL. xx is total level 0..127, where smaller is louder. Live channel patch only; not written back into the instrument.";
    case 0x16: return "Operator multiplier. A selects operator 1..4, B is MUL 0..15. Live channel patch only; it affects the sounding voice until changed or a note reload restores instrument values.";
    case 0x19: return "All-operator attack. xx is AR 0..31. Live channel patch only. Higher values make attack faster.";
    case 0x1A: return "Operator 1 attack. xx is AR 0..31. Live channel patch only.";
    case 0x1B: return "Operator 2 attack. xx is AR 0..31. Live channel patch only.";
    case 0x1C: return "Operator 3 attack. xx is AR 0..31. Live channel patch only.";
    case 0x1D: return "Operator 4 attack. xx is AR 0..31. Live channel patch only.";
    case 0x30: return "Hard envelope reset toggle. 00 off, non-zero on. When enabled, note transitions use hard mute before re-keying. Sticky per channel until changed.";
    case 0x50: return "Operator AM toggle. A selects operator 1..4, or 0 for all operators. B is 0 off or non-zero on. Live channel patch only.";
    case 0x51: return "Operator sustain level. A selects operator 1..4, or 0 for all operators. B is SL 0..15. Live channel patch only.";
    case 0x52: return "Operator release rate. A selects operator 1..4, or 0 for all operators. B is RR 0..15. Live channel patch only.";
    case 0x53: return "Operator detune. A selects operator 1..4, or 0 for all operators. B is Furnace DT code 0..7, mapped around center. Live channel patch only.";
    case 0x54: return "Operator rate scale. A selects operator 1..4, or 0 for all operators. B is RS 0..3. Live channel patch only.";
    case 0x55: return "Operator SSG-EG. A selects operator 1..4, or 0 for all operators. B 0..7 enables one SSG mode; 8 disables it. Live channel patch only.";
    case 0x56: return "All-operator decay rate. xx is DR 0..31. Live channel patch only.";
    case 0x57: return "Operator 1 decay rate. xx is DR 0..31. Live channel patch only.";
    case 0x58: return "Operator 2 decay rate. xx is DR 0..31. Live channel patch only.";
    case 0x59: return "Operator 3 decay rate. xx is DR 0..31. Live channel patch only.";
    case 0x5A: return "Operator 4 decay rate. xx is DR 0..31. Live channel patch only.";
    case 0x5B: return "All-operator sustain rate. xx is SR 0..31. Live channel patch only.";
    case 0x5C: return "Operator 1 sustain rate. xx is SR 0..31. Live channel patch only.";
    case 0x5D: return "Operator 2 sustain rate. xx is SR 0..31. Live channel patch only.";
    case 0x5E: return "Operator 3 sustain rate. xx is SR 0..31. Live channel patch only.";
    case 0x5F: return "Operator 4 sustain rate. xx is SR 0..31. Live channel patch only.";
    case 0x60: return "Operator mask. A=0 uses B as bitmask with OP1=1, OP2=2, OP3=4, OP4=8. A=1..4 targets one operator and B=0/1 disables/enables it. This edits the live channel patch mask only.";
    case 0x61: return "Algorithm. xx is ALG 0..7. Writes the live patch on the current channel; the stored instrument remains unchanged.";
    case 0x62: return "FMS. xx is frequency modulation sensitivity 0..7 in the live channel patch. Not instrument-persistent.";
    case 0x63: return "AMS. xx is amplitude modulation sensitivity 0..3 in the live channel patch. Not instrument-persistent.";
    default: return "Effect description missing.";
    }
}

inline int Tracker_ParseCellInstrument(const char *cell)
{
    if (!cell) return -1;
    return Tracker_ParseHexByte(cell + 3);
}

inline int Tracker_ParseCellVolume(const char *cell)
{
    if (!cell) return -1;
    return Tracker_ParseHexByte(cell + 5);
}

inline bool Tracker_CellHasNoteLikeValue(const char *cell)
{
    if (!cell) return false;
    return !(cell[0] == '.' && cell[1] == '.' && cell[2] == '.');
}

inline bool Tracker_CellIsOff(const char *cell)
{
    return cell && std::strncmp(cell, "OFF", 3) == 0;
}

inline bool Tracker_CellIsRel(const char *cell)
{
    return cell && std::strncmp(cell, "REL", 3) == 0;
}

inline bool Tracker_CellIsCut(const char *cell)
{
    return cell && std::strncmp(cell, "===", 3) == 0;
}

inline bool Tracker_CellIsSpecialTerminator(const char *cell)
{
    return Tracker_CellIsOff(cell) || Tracker_CellIsRel(cell) || Tracker_CellIsCut(cell);
}

inline bool Tracker_CellHasPlayableNote(const char *cell)
{
    return Tracker_CellHasNoteLikeValue(cell) && !Tracker_CellIsSpecialTerminator(cell);
}

inline bool Tracker_ParseCellNoteOctave(const char *cell, int *outNote, int *outOctave)
{
    if (!Tracker_CellHasPlayableNote(cell))
        return false;
    static const char *names[12] = {"C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-"};
    int note = -1;
    for (int n = 0; n < 12; n++)
    {
        if (cell[0] == names[n][0] && cell[1] == names[n][1])
        {
            note = n;
            break;
        }
    }
    if (note < 0 || cell[2] < '0' || cell[2] > '9')
        return false;
    if (outNote) *outNote = note;
    if (outOctave) *outOctave = std::max(1, std::min(7, cell[2] - '0'));
    return true;
}

inline bool Tracker_CellIsEmpty(const char *cell)
{
    if (!cell) return true;
    for (int i = 0; i < TRACKER_CELL_CHARS - 1 && cell[i]; i++)
        if (cell[i] != '.')
            return false;
    return true;
}

inline void Tracker_ClearCell(TrackerCell *cell)
{
    if (!cell) return;
    std::strncpy(cell->text, ".......", TRACKER_CELL_CHARS);
    cell->text[TRACKER_CELL_CHARS - 1] = '\0';
}

inline int Tracker_FindPreviousInstrument(const Tracker *self, int row, int channel)
{
    if (!self) return -1;
    channel = std::max(0, std::min(channel, TRACKER_CHANNELS - 1));
    for (int r = std::max(0, row); r >= 0; r--)
    {
        int inst = Tracker_ParseCellInstrument(self->cells[r][channel].text);
        if (inst >= 0) return inst;
    }
    return -1;
}

inline int Tracker_FindPreviousVolume(const Tracker *self, int row, int channel)
{
    if (!self) return -1;
    channel = std::max(0, std::min(channel, TRACKER_CHANNELS - 1));
    for (int r = std::max(0, row); r >= 0; r--)
    {
        int vol = Tracker_ParseCellVolume(self->cells[r][channel].text);
        if (vol >= 0) return vol;
    }
    return -1;
}

inline int Tracker_FindInheritedInstrument(const Tracker *self, int row, int channel)
{
    if (row <= 0) return -1;
    return Tracker_FindPreviousInstrument(self, row - 1, channel);
}

inline int Tracker_FindInheritedVolume(const Tracker *self, int row, int channel)
{
    if (row <= 0) return -1;
    return Tracker_FindPreviousVolume(self, row - 1, channel);
}

inline bool Tracker_CanInheritInstrument(const Tracker *self)
{
    if (!self) return false;
    int prev = Tracker_FindInheritedInstrument(self, self->editRow, self->editChannel);
    return prev >= 0 && self->editInstrument == prev;
}

inline bool Tracker_CanInheritVolume(const Tracker *self)
{
    if (!self) return false;
    int prev = Tracker_FindInheritedVolume(self, self->editRow, self->editChannel);
    return prev >= 0 && self->editVolume == prev;
}

inline bool Tracker_CellPreviewPayload(
    const Tracker *self,
    int row,
    int channel,
    int *outNote,
    int *outOctave,
    int *outInstrument,
    int *outVolume)
{
    if (!self || row < 0 || row >= self->rowCount || channel < 0 || channel >= TRACKER_CHANNELS)
        return false;
    const char *cell = self->cells[row][channel].text;
    int note = 0;
    int octave = 0;
    if (!Tracker_ParseCellNoteOctave(cell, &note, &octave))
        return false;

    int inst = Tracker_ParseCellInstrument(cell);
    if (inst < 0)
        inst = Tracker_FindInheritedInstrument(self, row, channel);
    if (inst < 0)
        inst = self->usedInstruments[0];

    int vol = Tracker_ParseCellVolume(cell);
    if (vol < 0)
        vol = Tracker_FindInheritedVolume(self, row, channel);
    if (vol < 0)
        vol = 0x7F;

    if (outNote) *outNote = std::max(0, std::min(11, note));
    if (outOctave) *outOctave = std::max(1, std::min(7, octave));
    if (outInstrument) *outInstrument = std::max(0, std::min(255, inst));
    if (outVolume) *outVolume = std::max(0, std::min(127, vol));
    return true;
}

inline void Tracker_RequestPreviewNote(
    Tracker *self,
    int note,
    int octave,
    int instrument,
    int volume,
    bool held)
{
    if (!self)
        return;
    self->previewNote = std::max(0, std::min(11, note));
    self->previewOctave = std::max(1, std::min(7, octave));
    self->previewInstrument = std::max(0, std::min(255, instrument));
    self->previewVolume = std::max(0, std::min(127, volume));
    if (held)
        self->previewHeldNoteStartRequested = true;
    else
        self->previewNoteRequested = true;
}

inline void Tracker_RequestEditorPreview(Tracker *self, bool held = false)
{
    if (self && self->editSpecial == 0)
        Tracker_RequestPreviewNote(self, self->editNote, self->editOctave, self->editInstrument, self->editVolume, held);
}

inline void Tracker_StopGridNoteAudition(Tracker *self)
{
    if (!self || !self->gridNoteAuditionActive)
        return;
    self->gridNoteAuditionActive = false;
    self->gridNoteAuditionSelectionMode = false;
    self->gridNoteAuditionRow = -1;
    self->gridNoteAuditionChannel = -1;
    self->gridNoteAuditionNote = -1;
    self->gridNoteAuditionOctave = -1;
    self->previewHeldNoteStopRequested = true;
}

inline void Tracker_ClearFurnaceKeyboardState(Tracker *self)
{
    if (!self)
        return;
    for (int i = 0; i < SDL_NUM_SCANCODES; i++)
    {
        if (self->furnaceKeyboardKeyActive[i])
        {
            self->previewHeldNotesStopAllRequested = true;
            break;
        }
    }
    self->furnaceKeyboardSpaceDown = false;
    std::memset(self->furnaceKeyboardKeyActive, 0, sizeof(self->furnaceKeyboardKeyActive));
}

inline void Tracker_StartGridNoteAudition(Tracker *self, int row, int channel, bool selectionMode)
{
    if (!self)
        return;
    int note = 0;
    int octave = 0;
    int inst = 0;
    int vol = 0x7F;
    if (!Tracker_CellPreviewPayload(self, row, channel, &note, &octave, &inst, &vol))
    {
        if (selectionMode)
            Tracker_StopGridNoteAudition(self);
        return;
    }

    if (self->gridNoteAuditionActive &&
        self->gridNoteAuditionRow == row &&
        self->gridNoteAuditionChannel == channel &&
        self->gridNoteAuditionNote == note &&
        self->gridNoteAuditionOctave == octave &&
        self->gridNoteAuditionInstrument == inst &&
        self->gridNoteAuditionVolume == vol)
    {
        return;
    }

    Tracker_StopGridNoteAudition(self);
    self->gridNoteAuditionActive = true;
    self->gridNoteAuditionSelectionMode = selectionMode;
    self->gridNoteAuditionRow = row;
    self->gridNoteAuditionChannel = channel;
    self->gridNoteAuditionNote = note;
    self->gridNoteAuditionOctave = octave;
    self->gridNoteAuditionInstrument = inst;
    self->gridNoteAuditionVolume = vol;
    Tracker_RequestPreviewNote(self, note, octave, inst, vol, /*held=*/true);
}

inline void Tracker_ToggleEditorInstrumentExplicit(Tracker *self)
{
    if (!self) return;
    int prev = Tracker_FindInheritedInstrument(self, self->editRow, self->editChannel);
    if (self->editInstrumentExplicit && prev >= 0)
    {
        self->editInstrument = prev;
        self->editInstrumentExplicit = false;
    }
    else
    {
        self->editInstrumentExplicit = true;
    }
}

inline void Tracker_ToggleEditorVolumeExplicit(Tracker *self)
{
    if (!self) return;
    int prev = Tracker_FindInheritedVolume(self, self->editRow, self->editChannel);
    if (self->editVolumeExplicit && prev >= 0)
    {
        self->editVolume = std::max(0, std::min(127, prev));
        self->editVolumeExplicit = false;
    }
    else
    {
        self->editVolumeExplicit = true;
    }
}

inline void Tracker_NormalizeExplicitFields(Tracker *self)
{
    if (!self) return;
    if (!Tracker_CanInheritInstrument(self))
        self->editInstrumentExplicit = true;
    if (!Tracker_CanInheritVolume(self))
        self->editVolumeExplicit = true;
}

inline void Tracker_RebuildUsedInstruments(Tracker *self)
{
    if (!self) return;
    self->usedInstrumentCount = 0;
    for (int row = 0; row < self->rowCount; row++)
    {
        for (int ch = 0; ch < TRACKER_CHANNELS; ch++)
        {
            int inst = Tracker_ParseCellInstrument(self->cells[row][ch].text);
            if (inst < 0) continue;
            bool exists = false;
            for (int i = 0; i < self->usedInstrumentCount; i++)
                if (self->usedInstruments[i] == inst) exists = true;
            if (!exists && self->usedInstrumentCount < TRACKER_MAX_USED_INSTRUMENTS)
                self->usedInstruments[self->usedInstrumentCount++] = inst;
        }
    }
    if (self->usedInstrumentCount == 0)
        self->usedInstruments[self->usedInstrumentCount++] = 0;
}

inline bool Tracker_InstrumentUsedInSong(const Tracker *self, int instrument)
{
    if (!self) return false;
    int inst = std::max(0, std::min(255, instrument));
    for (int i = 0; i < self->usedInstrumentCount; i++)
        if (self->usedInstruments[i] == inst) return true;
    return false;
}

inline void Tracker_ClearAvailableInstruments(Tracker *self)
{
    if (!self) return;
    for (int i = 0; i < 256; i++)
    {
        self->availableInstruments[i] = false;
    }
    self->availableInstrumentCount = 0;
}

inline void Tracker_ClearInstrumentState(Tracker *self, bool markDirty)
{
    if (!self) return;
    Tracker_ClearAvailableInstruments(self);
    for (int inst = 0; inst < 256; inst++)
    {
        Tracker_SetInstrumentName(self, inst, "", 0);
        self->instrumentColors[inst] = 0;
        self->editPatches[inst] = {};
        self->editPatchValid[inst] = false;
        self->editPatchDirty[inst] = markDirty;
        for (int target = 0; target < XFM_MACRO_TARGET_COUNT; target++)
        {
            self->editMacros[inst][target] = {};
            self->editMacroEnabled[inst][target] = false;
            self->editMacroValid[inst][target] = false;
            self->editMacroDirty[inst][target] = markDirty;
        }
    }
}

inline void Tracker_SetInstrumentAvailable(Tracker *self, int instrument)
{
    if (!self) return;
    int inst = std::max(0, std::min(255, instrument));
    if (!self->availableInstruments[inst])
    {
        self->availableInstruments[inst] = true;
        self->availableInstrumentCount++;
    }
}

inline bool Tracker_InstrumentAvailable(const Tracker *self, int instrument)
{
    if (!self) return false;
    int inst = std::max(0, std::min(255, instrument));
    if (self->availableInstrumentCount <= 0)
        return inst == 0;
    return self->availableInstruments[inst];
}

inline int Tracker_NextAvailableInstrument(const Tracker *self, int current, int direction)
{
    if (!self) return std::max(0, std::min(255, current));
    int dir = direction < 0 ? -1 : 1;
    int inst = std::max(0, std::min(255, current));
    if (self->availableInstrumentCount <= 0)
        return 0;
    for (int i = 0; i < 256; i++)
    {
        inst = (inst + dir + 256) & 255;
        if (self->availableInstruments[inst])
            return inst;
    }
    return std::max(0, std::min(255, current));
}

inline xfm_patch_opn Tracker_DefaultPatch();
inline void Tracker_DefaultMacro(XfmMacro *macro, int target);

inline int Tracker_FirstFreeInstrumentSlot(const Tracker *self)
{
    if (!self) return -1;
    for (int inst = 0; inst < 256; inst++)
        if (!self->availableInstruments[inst])
            return inst;
    return -1;
}

inline void TrackerClipboard_ClearInstruments(TrackerClipboard *clipboard)
{
    if (!clipboard) return;
    clipboard->instrumentCount = 0;
    clipboard->instrumentOverflow = false;
    for (int inst = 0; inst < 256; inst++)
    {
        clipboard->instrumentSourceToEntryValid[inst] = false;
        clipboard->instrumentSourceToEntry[inst] = -1;
    }
    for (int entry = 0; entry < TRACKER_MAX_USED_INSTRUMENTS; entry++)
    {
        clipboard->instrumentSourceIds[entry] = -1;
        clipboard->instrumentPasteIds[entry] = -1;
        clipboard->instrumentPasteMapped[entry] = false;
        clipboard->instrumentNameLengths[entry] = 0;
        std::memset(clipboard->instrumentNames[entry], 0, TRACKER_INSTRUMENT_NAME_CAPACITY);
        clipboard->instrumentColors[entry] = 0;
        clipboard->instrumentPatches[entry] = {};
        clipboard->instrumentPatchValid[entry] = false;
        for (int target = 0; target < XFM_MACRO_TARGET_COUNT; target++)
        {
            clipboard->instrumentMacros[entry][target] = {};
            clipboard->instrumentMacroEnabled[entry][target] = false;
            clipboard->instrumentMacroValid[entry][target] = false;
        }
    }
}

inline void TrackerClipboard_ResetPasteMap(TrackerClipboard *clipboard)
{
    if (!clipboard) return;
    for (int entry = 0; entry < TRACKER_MAX_USED_INSTRUMENTS; entry++)
    {
        clipboard->instrumentPasteIds[entry] = -1;
        clipboard->instrumentPasteMapped[entry] = false;
    }
}

inline int TrackerClipboard_InstrumentEntry(const TrackerClipboard *clipboard, int sourceInstrument)
{
    if (!clipboard || sourceInstrument < 0 || sourceInstrument > 255)
        return -1;
    if (!clipboard->instrumentSourceToEntryValid[sourceInstrument])
        return -1;
    int entry = clipboard->instrumentSourceToEntry[sourceInstrument];
    if (entry < 0 || entry >= clipboard->instrumentCount)
        return -1;
    return entry;
}

inline bool TrackerClipboard_AddInstrument(TrackerClipboard *clipboard, const Tracker *tracker, int sourceInstrument)
{
    if (!clipboard || !tracker || sourceInstrument < 0 || sourceInstrument > 255)
        return false;
    if (TrackerClipboard_InstrumentEntry(clipboard, sourceInstrument) >= 0)
        return true;
    if (clipboard->instrumentCount >= TRACKER_MAX_USED_INSTRUMENTS)
        return false;

    int entry = clipboard->instrumentCount++;
    clipboard->instrumentSourceIds[entry] = sourceInstrument;
    clipboard->instrumentPasteIds[entry] = sourceInstrument;
    clipboard->instrumentPasteMapped[entry] = false;
    clipboard->instrumentSourceToEntryValid[sourceInstrument] = true;
    clipboard->instrumentSourceToEntry[sourceInstrument] = entry;

    clipboard->instrumentNameLengths[entry] = tracker->instrumentNameLengths[sourceInstrument];
    std::memset(clipboard->instrumentNames[entry], 0, TRACKER_INSTRUMENT_NAME_CAPACITY);
    if (clipboard->instrumentNameLengths[entry] > 0)
        std::memcpy(clipboard->instrumentNames[entry], tracker->instrumentNames[sourceInstrument], (size_t)clipboard->instrumentNameLengths[entry]);
    clipboard->instrumentColors[entry] = Tracker_InstrumentColorU32(tracker, sourceInstrument);
    clipboard->instrumentPatchValid[entry] = tracker->editPatchValid[sourceInstrument];
    clipboard->instrumentPatches[entry] = tracker->editPatchValid[sourceInstrument] ? tracker->editPatches[sourceInstrument] : Tracker_DefaultPatch();
    for (int target = 0; target < XFM_MACRO_TARGET_COUNT; target++)
    {
        clipboard->instrumentMacros[entry][target] = tracker->editMacros[sourceInstrument][target];
        clipboard->instrumentMacroEnabled[entry][target] = tracker->editMacroEnabled[sourceInstrument][target];
        clipboard->instrumentMacroValid[entry][target] = tracker->editMacroValid[sourceInstrument][target];
    }
    return true;
}

inline bool Tracker_ClipboardEntryMatchesInstrument(const Tracker *tracker, const TrackerClipboard *clipboard, int entry, int instrument)
{
    if (!tracker || !clipboard || entry < 0 || entry >= clipboard->instrumentCount || instrument < 0 || instrument > 255)
        return false;
    if (!tracker->availableInstruments[instrument])
        return false;
    if (tracker->instrumentNameLengths[instrument] != clipboard->instrumentNameLengths[entry])
        return false;
    if (std::memcmp(tracker->instrumentNames[instrument], clipboard->instrumentNames[entry], TRACKER_INSTRUMENT_NAME_CAPACITY) != 0)
        return false;
    if (Tracker_InstrumentColorU32(tracker, instrument) != clipboard->instrumentColors[entry])
        return false;
    xfm_patch_opn patch = tracker->editPatchValid[instrument] ? tracker->editPatches[instrument] : Tracker_DefaultPatch();
    if (std::memcmp(&patch, &clipboard->instrumentPatches[entry], sizeof(patch)) != 0)
        return false;
    for (int target = 0; target < XFM_MACRO_TARGET_COUNT; target++)
    {
        if (tracker->editMacroEnabled[instrument][target] != clipboard->instrumentMacroEnabled[entry][target])
            return false;
        if (tracker->editMacroValid[instrument][target] != clipboard->instrumentMacroValid[entry][target])
            return false;
        if (tracker->editMacroValid[instrument][target] &&
            std::memcmp(&tracker->editMacros[instrument][target], &clipboard->instrumentMacros[entry][target], sizeof(XfmMacro)) != 0)
            return false;
    }
    return true;
}

inline bool Tracker_PrepareClipboardForSong(Tracker *self)
{
    if (!self || !self->clipboard.valid)
        return false;
    TrackerClipboard_ResetPasteMap(&self->clipboard);
    bool reserved[256] = {};
    for (int entry = 0; entry < self->clipboard.instrumentCount; entry++)
    {
        int source = self->clipboard.instrumentSourceIds[entry];
        int target = -1;
        if (source >= 0 && source <= 255 &&
            Tracker_ClipboardEntryMatchesInstrument(self, &self->clipboard, entry, source) &&
            !reserved[source])
        {
            target = source;
        }
        else if (source >= 0 && source <= 255 && !self->availableInstruments[source] && !reserved[source])
        {
            target = source;
        }
        else
        {
            for (int inst = 0; inst < 256; inst++)
            {
                if (!self->availableInstruments[inst] && !reserved[inst])
                {
                    target = inst;
                    break;
                }
            }
        }
        if (target < 0)
            return false;
        self->clipboard.instrumentPasteIds[entry] = target;
        self->clipboard.instrumentPasteMapped[entry] = true;
        reserved[target] = true;
    }
    return true;
}

inline void Tracker_ImportClipboardInstrument(Tracker *self, const TrackerClipboard *clipboard, int entry, int target)
{
    if (!self || !clipboard || entry < 0 || entry >= clipboard->instrumentCount || target < 0 || target > 255)
        return;
    if (Tracker_ClipboardEntryMatchesInstrument(self, clipboard, entry, target))
        return;
    Tracker_SetInstrumentAvailable(self, target);
    Tracker_SetInstrumentName(self, target, clipboard->instrumentNames[entry], clipboard->instrumentNameLengths[entry]);
    self->instrumentColors[target] = clipboard->instrumentColors[entry];
    self->editPatches[target] = clipboard->instrumentPatchValid[entry] ? clipboard->instrumentPatches[entry] : Tracker_DefaultPatch();
    self->editPatchValid[target] = true;
    self->editPatchDirty[target] = true;
    for (int macro = 0; macro < XFM_MACRO_TARGET_COUNT; macro++)
    {
        self->editMacros[target][macro] = clipboard->instrumentMacros[entry][macro];
        self->editMacroEnabled[target][macro] = clipboard->instrumentMacroEnabled[entry][macro];
        self->editMacroValid[target][macro] = clipboard->instrumentMacroValid[entry][macro];
        self->editMacroDirty[target][macro] = self->editMacroEnabled[target][macro] || self->editMacroValid[target][macro];
    }
}

inline void Tracker_RemountCellInstrument(char *cell, int from, int to)
{
    int inst = Tracker_ParseCellInstrument(cell);
    if (inst == from)
        Tracker_WriteHexByte(cell + 3, to);
}

inline void Tracker_SwapCellInstruments(char *cell, int a, int b)
{
    int inst = Tracker_ParseCellInstrument(cell);
    if (inst == a)
        Tracker_WriteHexByte(cell + 3, b);
    else if (inst == b)
        Tracker_WriteHexByte(cell + 3, a);
}

inline void Tracker_SwapInstrumentSlots(Tracker *self, int a, int b)
{
    if (!self || a == b) return;
    a = std::max(0, std::min(255, a));
    b = std::max(0, std::min(255, b));
    std::swap(self->availableInstruments[a], self->availableInstruments[b]);
    for (int i = 0; i < TRACKER_INSTRUMENT_NAME_CAPACITY; i++)
        std::swap(self->instrumentNames[a][i], self->instrumentNames[b][i]);
    std::swap(self->instrumentNameLengths[a], self->instrumentNameLengths[b]);
    std::swap(self->instrumentColors[a], self->instrumentColors[b]);
    std::swap(self->editPatches[a], self->editPatches[b]);
    std::swap(self->editPatchValid[a], self->editPatchValid[b]);
    std::swap(self->editPatchDirty[a], self->editPatchDirty[b]);
    for (int target = 0; target < XFM_MACRO_TARGET_COUNT; target++)
    {
        std::swap(self->editMacros[a][target], self->editMacros[b][target]);
        std::swap(self->editMacroEnabled[a][target], self->editMacroEnabled[b][target]);
        std::swap(self->editMacroValid[a][target], self->editMacroValid[b][target]);
        std::swap(self->editMacroDirty[a][target], self->editMacroDirty[b][target]);
    }
}

inline void Tracker_MoveInstrument(Tracker *self, int instrument, int direction)
{
    if (!self) return;
    int from = std::max(0, std::min(255, instrument));
    int to = from + (direction < 0 ? -1 : 1);
    if (to < 0 || to > 255 || !self->availableInstruments[from])
        return;
    Tracker_SwapInstrumentSlots(self, from, to);
    self->editPatchDirty[from] = true;
    self->editPatchDirty[to] = true;
    for (int target = 0; target < XFM_MACRO_TARGET_COUNT; target++)
    {
        self->editMacroDirty[from][target] = true;
        self->editMacroDirty[to][target] = true;
    }
    for (int row = 0; row < self->rowCount; row++)
        for (int ch = 0; ch < TRACKER_CHANNELS; ch++)
            Tracker_SwapCellInstruments(self->cells[row][ch].text, from, to);
    if (self->editInstrument == from) self->editInstrument = to;
    else if (self->editInstrument == to) self->editInstrument = from;
    self->patternDirty = true;
    self->copyOnWriteRequested = true;
    Tracker_RebuildUsedInstruments(self);
    Tracker_FlashInstrument(self, to, TRACKER_CHANGE_FLASH_EDIT);
}

inline void Tracker_DeleteInstrument(Tracker *self, int instrument)
{
    if (!self) return;
    int inst = std::max(0, std::min(255, instrument));
    if (!self->availableInstruments[inst])
        return;
    self->availableInstruments[inst] = false;
    self->availableInstrumentCount = std::max(0, self->availableInstrumentCount - 1);
    Tracker_SetInstrumentName(self, inst, "", 0);
    self->instrumentColors[inst] = 0;
    self->editPatchValid[inst] = false;
    self->editPatchDirty[inst] = true;
    for (int target = 0; target < XFM_MACRO_TARGET_COUNT; target++)
    {
        self->editMacroEnabled[inst][target] = false;
        self->editMacroValid[inst][target] = false;
        self->editMacroDirty[inst][target] = true;
    }
    self->patternDirty = true;
    self->copyOnWriteRequested = true;
}

inline bool Tracker_CloneInstrument(Tracker *self, int source, int target, const char *name, int32_t nameLen)
{
    if (!self) return false;
    source = std::max(0, std::min(255, source));
    target = std::max(0, std::min(255, target));
    if (!self->availableInstruments[source] || self->availableInstruments[target])
        return false;
    Tracker_SetInstrumentAvailable(self, target);
    Tracker_SetInstrumentName(self, target, name, nameLen);
    self->instrumentColors[target] = Tracker_InstrumentColorU32(self, source);
    self->editPatches[target] = self->editPatchValid[source] ? self->editPatches[source] : Tracker_DefaultPatch();
    self->editPatchValid[target] = true;
    self->editPatchDirty[target] = true;
    for (int macro = 0; macro < XFM_MACRO_TARGET_COUNT; macro++)
    {
        self->editMacros[target][macro] = self->editMacros[source][macro];
        self->editMacroEnabled[target][macro] = self->editMacroEnabled[source][macro];
        self->editMacroValid[target][macro] = self->editMacroValid[source][macro];
        self->editMacroDirty[target][macro] = self->editMacroEnabled[target][macro] || self->editMacroValid[target][macro];
    }
    self->patternDirty = true;
    self->copyOnWriteRequested = true;
    return true;
}

inline bool Tracker_CreateInstrumentFromTemplate(Tracker *self, int target, const char *name, int32_t nameLen)
{
    if (!self) return false;
    target = std::max(0, std::min(255, target));
    if (self->availableInstruments[target])
        return false;
    Tracker_SetInstrumentAvailable(self, target);
    Tracker_SetInstrumentName(self, target, name, nameLen);
    self->instrumentColors[target] = Tracker_DefaultInstrumentColor(target);
    self->editPatches[target] = Tracker_DefaultPatch();
    self->editPatchValid[target] = true;
    self->editPatchDirty[target] = true;
    for (int macro = 0; macro < XFM_MACRO_TARGET_COUNT; macro++)
    {
        self->editMacros[target][macro] = {};
        self->editMacroEnabled[target][macro] = false;
        self->editMacroValid[target][macro] = false;
        self->editMacroDirty[target][macro] = true;
    }
    self->patternDirty = true;
    self->copyOnWriteRequested = true;
    return true;
}

inline void Tracker_EnsureDefaultInstrument(Tracker *self, bool markDirty)
{
    if (!self) return;
    const int inst = 0;
    Tracker_SetInstrumentAvailable(self, inst);
    if (self->instrumentNameLengths[inst] <= 0)
        Tracker_SetInstrumentName(
            self,
            inst,
            Tracker_DefaultInstrumentName(inst),
            (int32_t)std::strlen(Tracker_DefaultInstrumentName(inst))
        );
    if (self->instrumentColors[inst] == 0)
        self->instrumentColors[inst] = Tracker_DefaultInstrumentColor(inst);
    self->editPatches[inst] = Tracker_DefaultPatch();
    self->editPatchValid[inst] = true;
    self->editPatchDirty[inst] = markDirty;
    for (int target = 0; target < XFM_MACRO_TARGET_COUNT; target++)
    {
        self->editMacros[inst][target] = {};
        self->editMacroEnabled[inst][target] = false;
        self->editMacroValid[inst][target] = false;
        self->editMacroDirty[inst][target] = markDirty;
    }
}

inline void Tracker_ParseCellForEditor(Tracker *self)
{
    if (!self) return;
    char *cell = self->cells[self->editRow][self->editChannel].text;
    self->editSpecial = 0;
    if (std::strncmp(cell, "OFF", 3) == 0) self->editSpecial = 1;
    else if (std::strncmp(cell, "REL", 3) == 0) self->editSpecial = 2;
    else if (std::strncmp(cell, "===", 3) == 0) self->editSpecial = 3;
    else if (std::strncmp(cell, "...", 3) == 0) self->editSpecial = 4;
    else if (Tracker_CellHasNoteLikeValue(cell))
    {
        static const char *names[12] = {"C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-"};
        for (int n = 0; n < 12; n++)
            if (cell[0] == names[n][0] && cell[1] == names[n][1]) self->editNote = n;
        if (cell[2] >= '0' && cell[2] <= '9') self->editOctave = std::max(1, std::min(7, cell[2] - '0'));
    }
    int inst = Tracker_ParseCellInstrument(cell);
    self->editInstrumentExplicit = inst >= 0;
    if (inst < 0) inst = Tracker_FindInheritedInstrument(self, self->editRow, self->editChannel);
    if (inst < 0) inst = self->usedInstruments[0];
    self->editInstrument = inst;

    int vol = Tracker_ParseCellVolume(cell);
    self->editVolumeExplicit = vol >= 0;
    if (vol < 0) vol = Tracker_FindInheritedVolume(self, self->editRow, self->editChannel);
    self->editVolume = vol >= 0 ? std::max(0, std::min(127, vol)) : 0x7F;
    Tracker_NormalizeExplicitFields(self);

    for (int i = 0; i < TRACKER_MAX_EFFECT_SLOTS; i++)
    {
        self->editEffectActive[i] = false;
        self->editEffectCodes[i] = TRACKER_EFFECT_DEFS[TRACKER_DEFAULT_EFFECT_DEF_INDEX].code;
        self->editEffectValues[i] = 0;
    }
    for (int i = 0; i < TRACKER_EFFECT_DEF_COUNT; i++)
        self->editEffectValuesByDef[i] = 0;
    int slot = 0;
    int pos = 7;
    while (slot < TRACKER_MAX_EFFECT_SLOTS && pos + 3 < TRACKER_CELL_CHARS && cell[pos] && cell[pos] != '.')
    {
        if (!Tracker_IsHex(cell[pos]) || !Tracker_IsHex(cell[pos + 1]) ||
            !Tracker_IsHex(cell[pos + 2]) || !Tracker_IsHex(cell[pos + 3]))
            break;
        self->editEffectCodes[slot] = (uint8_t)Tracker_ParseHexByte(cell + pos);
        self->editEffectValues[slot] = (uint8_t)Tracker_ParseHexByte(cell + pos + 2);
        self->editEffectActive[slot] = self->editEffectCodes[slot] != TRACKER_EFFECT_CODE_NONE;
        int defIdx = Tracker_EffectDefIndexByCode(self->editEffectCodes[slot]);
        if (defIdx > 0)
        {
            const TrackerEffectDef *def = &TRACKER_EFFECT_DEFS[defIdx];
            self->editEffectValues[slot] = Tracker_ClampEffectValueToDef(def, self->editEffectValues[slot]);
        }
        if (defIdx > 0)
            self->editEffectValuesByDef[defIdx] = self->editEffectValues[slot];
        if (slot == 0 && defIdx > 0)
            self->editEffect = defIdx;
        slot++;
        pos += 4;
    }
    if (self->editEffect <= 0 || self->editEffect >= TRACKER_EFFECT_DEF_COUNT)
        self->editEffect = TRACKER_DEFAULT_EFFECT_DEF_INDEX;
}

inline void Tracker_ApplyEditorToCell(Tracker *self)
{
    if (!self) return;
    static const char *names[12] = {"C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-"};
    char *cell = self->cells[self->editRow][self->editChannel].text;
    while ((int)std::strlen(cell) < 7) std::strncat(cell, ".", TRACKER_CELL_CHARS - std::strlen(cell) - 1);
    if (self->editSpecial == 1) std::memcpy(cell, "OFF", 3);
    else if (self->editSpecial == 2) std::memcpy(cell, "REL", 3);
    else if (self->editSpecial == 3) std::memcpy(cell, "===", 3);
    else if (self->editSpecial == 4) std::memcpy(cell, "...", 3);
    else
    {
        int note = std::max(0, std::min(11, self->editNote));
        cell[0] = names[note][0];
        cell[1] = names[note][1];
        cell[2] = (char)('0' + std::max(1, std::min(7, self->editOctave)));
    }
    if (self->editInstrumentExplicit) Tracker_WriteHexByte(cell + 3, self->editInstrument);
    else std::memcpy(cell + 3, "..", 2);
    if (self->editVolumeExplicit) Tracker_WriteHexByte(cell + 5, self->editVolume);
    else std::memcpy(cell + 5, "..", 2);
    int pos = 7;
    int activeWritten = 0;
    for (int i = 0; i < TRACKER_MAX_EFFECT_SLOTS && pos + 3 < TRACKER_CELL_CHARS - 1; i++)
    {
        if (!self->editEffectActive[i] || self->editEffectCodes[i] == TRACKER_EFFECT_CODE_NONE) continue;
        if (activeWritten >= TRACKER_CELL_ACTIVE_EFFECT_LIMIT) break;
        Tracker_WriteHexByte(cell + pos, self->editEffectCodes[i]);
        Tracker_WriteHexByte(cell + pos + 2, self->editEffectValues[i]);
        pos += 4;
        activeWritten++;
    }
    for (; pos < TRACKER_CELL_CHARS - 1; pos++)
        cell[pos] = '\0';
    cell[TRACKER_CELL_CHARS - 1] = '\0';
    self->patternDirty = true;
    self->copyOnWriteRequested = true;
    Tracker_RebuildUsedInstruments(self);
}

inline void Tracker_DeleteEditorCell(Tracker *self)
{
    if (!self) return;
    char *cell = self->cells[self->editRow][self->editChannel].text;
    std::strncpy(cell, ".......", TRACKER_CELL_CHARS);
    cell[TRACKER_CELL_CHARS - 1] = '\0';
    self->editSpecial = 4;
    self->editInstrumentExplicit = false;
    self->editVolumeExplicit = false;
    for (int i = 0; i < TRACKER_MAX_EFFECT_SLOTS; i++)
    {
        self->editEffectActive[i] = false;
        self->editEffectCodes[i] = TRACKER_EFFECT_DEFS[TRACKER_DEFAULT_EFFECT_DEF_INDEX].code;
        self->editEffectValues[i] = 0;
    }
    for (int i = 0; i < TRACKER_EFFECT_DEF_COUNT; i++)
        self->editEffectValuesByDef[i] = 0;
    self->editEffect = TRACKER_DEFAULT_EFFECT_DEF_INDEX;
    self->patternDirty = true;
    self->copyOnWriteRequested = true;
    Tracker_RebuildUsedInstruments(self);
}

inline xfm_patch_opn Tracker_DefaultPatch()
{
    xfm_patch_opn patch = {};
    patch.ALG = 0;
    patch.FB = 0;
    patch.AMS = 0;
    patch.FMS = 0;
    for (int op = 0; op < 4; op++)
    {
        patch.op[op].DT = 0;
        patch.op[op].MUL = 1;
        patch.op[op].TL = op == 3 ? 0 : 48;
        patch.op[op].RS = 0;
        patch.op[op].AR = 31;
        patch.op[op].AM = 0;
        patch.op[op].DR = 8;
        patch.op[op].SR = 0;
        patch.op[op].SL = 15;
        patch.op[op].RR = 8;
        patch.op[op].SSG = 0;
    }
    return patch;
}

inline void Tracker_NormalizeMacroUiState(XfmMacro *macro);

inline void Tracker_LoadCustomInstrumentText(Tracker *tracker, const std::string &text)
{
    if (!tracker || text.empty()) return;
    std::istringstream in(text);
    std::string tag;
    int inst = -1;
    while (in >> tag)
    {
        if (tag == "INST")
        {
            std::string hex;
            in >> hex;
            inst = (int)std::strtol(hex.c_str(), nullptr, 16);
            if (inst >= 0 && inst < 256)
            {
                Tracker_SetInstrumentAvailable(tracker, inst);
                tracker->editPatches[inst] = Tracker_DefaultPatch();
                tracker->editPatchValid[inst] = true;
            }
        }
        else if (tag == "ALG" && inst >= 0 && inst < 256)
        {
            std::string ignored;
            std::getline(in, ignored);
        }
        else if (tag == "PATCH" && inst >= 0 && inst < 256)
        {
            int alg, fb, ams, fms;
            in >> alg >> fb >> ams >> fms;
            tracker->editPatches[inst].ALG = (uint8_t)std::max(0, std::min(7, alg));
            tracker->editPatches[inst].FB = (uint8_t)std::max(0, std::min(7, fb));
            tracker->editPatches[inst].AMS = (uint8_t)std::max(0, std::min(3, ams));
            tracker->editPatches[inst].FMS = (uint8_t)std::max(0, std::min(7, fms));
            tracker->editPatchDirty[inst] = true;
        }
        else if (tag == "NAME" && inst >= 0 && inst < 256)
        {
            std::string name;
            std::getline(in, name);
            while (!name.empty() && (name.front() == ' ' || name.front() == '\t'))
                name.erase(name.begin());
            while (!name.empty() && (name.back() == '\r' || name.back() == '\n'))
                name.pop_back();
            Tracker_SetInstrumentName(tracker, inst, name.c_str(), (int32_t)name.size());
        }
        else if (tag == "COLOR" && inst >= 0 && inst < 256)
        {
            std::string hex;
            in >> hex;
            unsigned long rgb = std::strtoul(hex.c_str(), nullptr, 16);
            tracker->instrumentColors[inst] = (uint32_t)(rgb & 0xFFFFFFu);
        }
        else if (tag == "OP" && inst >= 0 && inst < 256)
        {
            std::string opToken;
            in >> opToken;
            int op = 0;
            if (!TrackerSongIO_ParseIntStrict(opToken, op))
            {
                std::string ignored;
                std::getline(in, ignored);
                continue;
            }
            int dt, mul, tl, rs, ar, am, dr, sr, sl, rr, ssg;
            in >> dt >> mul >> tl >> rs >> ar >> am >> dr >> sr >> sl >> rr >> ssg;
            if (op >= 1 && op <= 4)
            {
                xfm_patch_opn_operator &o = tracker->editPatches[inst].op[op - 1];
                o.DT = (int8_t)std::max(-3, std::min(3, dt));
                o.MUL = (uint8_t)std::max(0, std::min(15, mul));
                o.TL = (uint8_t)std::max(0, std::min(127, tl));
                o.RS = (uint8_t)std::max(0, std::min(3, rs));
                o.AR = (uint8_t)std::max(0, std::min(31, ar));
                o.AM = (uint8_t)std::max(0, std::min(1, am));
                o.DR = (uint8_t)std::max(0, std::min(31, dr));
                o.SR = (uint8_t)std::max(0, std::min(31, sr));
                o.SL = (uint8_t)std::max(0, std::min(15, sl));
                o.RR = (uint8_t)std::max(0, std::min(15, rr));
                o.SSG = (uint8_t)std::max(0, std::min(8, ssg));
            }
        }
        else if (tag == "FM" && inst >= 0 && inst < 256)
        {
            std::string opToken;
            in >> opToken;
            int op = 0;
            if (!TrackerSongIO_ParseIntStrict(opToken, op))
            {
                std::string ignored;
                std::getline(in, ignored);
                continue;
            }
            int tl, ar, dr, sl, sr, rr, ssg, mul, dt, rs, am;
            in >> tl >> ar >> dr >> sl >> sr >> rr >> ssg >> mul >> dt >> rs >> am;
            if (op >= 1 && op <= 4)
            {
                xfm_patch_opn_operator &o = tracker->editPatches[inst].op[op - 1];
                o.TL = (uint8_t)std::max(0, std::min(127, tl));
                o.AR = (uint8_t)std::max(0, std::min(31, ar));
                o.DR = (uint8_t)std::max(0, std::min(31, dr));
                o.SL = (uint8_t)std::max(0, std::min(15, sl));
                o.SR = (uint8_t)std::max(0, std::min(31, sr));
                o.RR = (uint8_t)std::max(0, std::min(15, rr));
                o.SSG = (uint8_t)std::max(0, std::min(8, ssg));
                o.MUL = (uint8_t)std::max(0, std::min(15, mul));
                o.DT = (int8_t)std::max(-3, std::min(3, dt));
                o.RS = (uint8_t)std::max(0, std::min(3, rs));
                o.AM = (uint8_t)std::max(0, std::min(1, am));
            }
        }
        else if (tag == "MACRO" && inst >= 0 && inst < 256)
        {
            int target, length, loopStart, releaseStart;
            in >> target >> length >> loopStart >> releaseStart;
            if (target >= XFM_MACRO_TL1 && target < XFM_MACRO_TARGET_COUNT)
            {
                XfmMacro &macro = tracker->editMacros[inst][target];
                Tracker_DefaultMacro(&macro, target);
                macro.length = (uint8_t)std::max(0, std::min(TRACKER_MACRO_UI_STEPS, length));
                macro.has_loop = macro.length > 0 && loopStart >= 0 && loopStart < macro.length && loopStart != 255;
                macro.loop_start = macro.has_loop ? (uint8_t)loopStart : 0;
                macro.release_start = (releaseStart == 255 || macro.length == 0) ? 0xFF : (uint8_t)std::max(0, std::min((int)macro.length - 1, releaseStart));
                for (int i = 0; i < macro.length; i++)
                {
                    int v = 0;
                    in >> v;
                    macro.values[i] = (int16_t)v;
                }
                Tracker_NormalizeMacroUiState(&macro);
                tracker->editMacroEnabled[inst][target] = true;
                tracker->editMacroValid[inst][target] = true;
                tracker->editMacroDirty[inst][target] = true;
            }
        }
    }
}

inline void Tracker_MarkAllAvailablePatchesAndMacrosDirty(Tracker *self)
{
    if (!self) return;
    for (int inst = 0; inst < 256; inst++)
    {
        if (!self->availableInstruments[inst] || !self->editPatchValid[inst])
            continue;
        self->editPatchDirty[inst] = true;
        for (int target = XFM_MACRO_TL1; target < XFM_MACRO_TARGET_COUNT; target++)
        {
            if (self->editMacroEnabled[inst][target] && self->editMacroValid[inst][target])
                self->editMacroDirty[inst][target] = true;
        }
    }
}

inline void Tracker_RemapInstrumentInCurrentSong(Tracker *self, int fromInst, int toInst)
{
    if (!self || fromInst == toInst) return;
    for (int row = 0; row < self->rowCount; row++)
    {
        for (int ch = 0; ch < TRACKER_CHANNELS; ch++)
        {
            char *cell = self->cells[row][ch].text;
            int inst = Tracker_ParseHexByte(cell + 3);
            if (inst == fromInst)
                Tracker_WriteHexByte(cell + 3, toInst);
        }
    }
    Tracker_RebuildUsedInstruments(self);
    self->patternDirty = true;
    self->copyOnWriteRequested = true;
}

inline int Tracker_FirstUnusedCustomInstrument(const Tracker *self)
{
    for (int inst = 0x00; inst <= 0xFF; inst++)
    {
        bool used = false;
        if (self)
        {
            for (int i = 0; i < self->usedInstrumentCount; i++)
                if (self->usedInstruments[i] == inst)
                    used = true;
            if (self->editPatchValid[inst] || self->editPatchDirty[inst])
                used = true;
        }
        if (!used) return inst;
    }
    return 0xFF;
}

inline void Tracker_RequestSongCopyOnWriteForInstrumentEdit(Tracker *self)
{
    if (!self) return;
    self->copyOnWriteRequested = true;
}

inline xfm_patch_opn &Tracker_EditablePatch(Tracker *self)
{
    static xfm_patch_opn fallback = Tracker_DefaultPatch();
    if (!self) return fallback;
    int inst = std::max(0, std::min(255, self->editInstrument));
    if (!self->editPatchValid[inst])
    {
        self->editPatches[inst] = Tracker_DefaultPatch();
        self->editPatchValid[inst] = true;
    }
    return self->editPatches[inst];
}

inline void Tracker_SetEditOperator(Tracker *self, int op)
{
    if (!self) return;
    self->editOperator = std::max(0, std::min(3, op));
}

inline void Tracker_CycleEditOperator(Tracker *self, int delta)
{
    if (!self) return;
    int op = ((self->editOperator + delta) % 4 + 4) % 4;
    self->editOperator = op;
}

inline void Tracker_MarkPatchDirty(Tracker *self)
{
    if (!self) return;
    Tracker_RequestSongCopyOnWriteForInstrumentEdit(self);
    int inst = std::max(0, std::min(255, self->editInstrument));
    self->editPatchValid[inst] = true;
    self->editPatchDirty[inst] = true;
    self->copyOnWriteRequested = true;
}

inline const char *Tracker_MacroTargetName(int target)
{
    switch (target)
    {
    case XFM_MACRO_TL1: return "OP1 TL";
    case XFM_MACRO_TL2: return "OP2 TL";
    case XFM_MACRO_TL3: return "OP3 TL";
    case XFM_MACRO_TL4: return "OP4 TL";
    case XFM_MACRO_MUL1: return "OP1 MUL";
    case XFM_MACRO_MUL2: return "OP2 MUL";
    case XFM_MACRO_MUL3: return "OP3 MUL";
    case XFM_MACRO_MUL4: return "OP4 MUL";
    case XFM_MACRO_DT1: return "OP1 DT";
    case XFM_MACRO_DT2: return "OP2 DT";
    case XFM_MACRO_DT3: return "OP3 DT";
    case XFM_MACRO_DT4: return "OP4 DT";
    case XFM_MACRO_FB: return "FB";
    case XFM_MACRO_ARP: return "ARP";
    case XFM_MACRO_PAN: return "PAN";
    case XFM_MACRO_PITCH: return "PITCH";
    case XFM_MACRO_RELATIVE: return "REL";
    case XFM_MACRO_PHASE_RESET: return "PHASE";
    case XFM_MACRO_AR1: return "OP1 AR";
    case XFM_MACRO_AR2: return "OP2 AR";
    case XFM_MACRO_AR3: return "OP3 AR";
    case XFM_MACRO_AR4: return "OP4 AR";
    case XFM_MACRO_DR1: return "OP1 DR";
    case XFM_MACRO_DR2: return "OP2 DR";
    case XFM_MACRO_DR3: return "OP3 DR";
    case XFM_MACRO_DR4: return "OP4 DR";
    case XFM_MACRO_SR1: return "OP1 SR";
    case XFM_MACRO_SR2: return "OP2 SR";
    case XFM_MACRO_SR3: return "OP3 SR";
    case XFM_MACRO_SR4: return "OP4 SR";
    case XFM_MACRO_SL1: return "OP1 SL";
    case XFM_MACRO_SL2: return "OP2 SL";
    case XFM_MACRO_SL3: return "OP3 SL";
    case XFM_MACRO_SL4: return "OP4 SL";
    case XFM_MACRO_RR1: return "OP1 RR";
    case XFM_MACRO_RR2: return "OP2 RR";
    case XFM_MACRO_RR3: return "OP3 RR";
    case XFM_MACRO_RR4: return "OP4 RR";
    case XFM_MACRO_SSG1: return "OP1 SSG";
    case XFM_MACRO_SSG2: return "OP2 SSG";
    case XFM_MACRO_SSG3: return "OP3 SSG";
    case XFM_MACRO_SSG4: return "OP4 SSG";
    default: return "MAC";
    }
}

inline int Tracker_MacroMaxTarget()
{
    return XFM_MACRO_SSG4;
}

inline void Tracker_DefaultMacro(XfmMacro *macro, int target)
{
    if (!macro) return;
    *macro = {};
    macro->target = (uint8_t)std::max((int)XFM_MACRO_TL1, std::min(Tracker_MacroMaxTarget(), target));
    macro->length = 0;
    macro->loop_start = 0;
    macro->release_start = 0xFF;
    macro->has_loop = false;
    int16_t value = 0;
    if (macro->target >= XFM_MACRO_MUL1 && macro->target <= XFM_MACRO_MUL4)
        value = 1;
    else if (macro->target == XFM_MACRO_PAN)
        value = 3;
    for (int i = 0; i < XFM_MAX_MACRO_VALUES; i++)
        macro->values[i] = value;
}

inline int16_t Tracker_MacroDefaultValue(int target)
{
    if (target >= XFM_MACRO_MUL1 && target <= XFM_MACRO_MUL4) return 1;
    if (target == XFM_MACRO_PAN) return 3;
    return 0;
}

inline bool Tracker_MacroTargetSupportsRelease(int target)
{
    return !((target >= XFM_MACRO_AR1 && target <= XFM_MACRO_RR4) ||
             (target >= XFM_MACRO_SSG1 && target <= XFM_MACRO_SSG4));
}

inline int Tracker_MacroTargetBaseValue(const Tracker *self, int target)
{
    if (!self) return Tracker_MacroDefaultValue(target);
    int inst = std::max(0, std::min(255, self->editInstrument));
    xfm_patch_opn patch = self->editPatchValid[inst] ? self->editPatches[inst] : Tracker_DefaultPatch();

    auto opValue = [&](int baseTarget, auto getter) -> int {
        int op = target - baseTarget;
        if (op < 0 || op >= 4) return Tracker_MacroDefaultValue(target);
        return getter(patch.op[op]);
    };

    switch (target)
    {
    case XFM_MACRO_TL1:
    case XFM_MACRO_TL2:
    case XFM_MACRO_TL3:
    case XFM_MACRO_TL4:
        return opValue(XFM_MACRO_TL1, [](const xfm_patch_opn_operator &op) { return (int)op.TL; });
    case XFM_MACRO_MUL1:
    case XFM_MACRO_MUL2:
    case XFM_MACRO_MUL3:
    case XFM_MACRO_MUL4:
        return opValue(XFM_MACRO_MUL1, [](const xfm_patch_opn_operator &op) { return (int)op.MUL; });
    case XFM_MACRO_DT1:
    case XFM_MACRO_DT2:
    case XFM_MACRO_DT3:
    case XFM_MACRO_DT4:
        return opValue(XFM_MACRO_DT1, [](const xfm_patch_opn_operator &op) { return (int)op.DT; });
    case XFM_MACRO_FB:
        return (int)patch.FB;
    case XFM_MACRO_ARP:
        return 0;
    case XFM_MACRO_PAN:
        return 3;
    case XFM_MACRO_PITCH:
        return 0;
    case XFM_MACRO_RELATIVE:
        return 0;
    case XFM_MACRO_PHASE_RESET:
        return 0;
    case XFM_MACRO_AR1:
    case XFM_MACRO_AR2:
    case XFM_MACRO_AR3:
    case XFM_MACRO_AR4:
        return opValue(XFM_MACRO_AR1, [](const xfm_patch_opn_operator &op) { return (int)op.AR; });
    case XFM_MACRO_DR1:
    case XFM_MACRO_DR2:
    case XFM_MACRO_DR3:
    case XFM_MACRO_DR4:
        return opValue(XFM_MACRO_DR1, [](const xfm_patch_opn_operator &op) { return (int)op.DR; });
    case XFM_MACRO_SR1:
    case XFM_MACRO_SR2:
    case XFM_MACRO_SR3:
    case XFM_MACRO_SR4:
        return opValue(XFM_MACRO_SR1, [](const xfm_patch_opn_operator &op) { return (int)op.SR; });
    case XFM_MACRO_SL1:
    case XFM_MACRO_SL2:
    case XFM_MACRO_SL3:
    case XFM_MACRO_SL4:
        return opValue(XFM_MACRO_SL1, [](const xfm_patch_opn_operator &op) { return (int)op.SL; });
    case XFM_MACRO_RR1:
    case XFM_MACRO_RR2:
    case XFM_MACRO_RR3:
    case XFM_MACRO_RR4:
        return opValue(XFM_MACRO_RR1, [](const xfm_patch_opn_operator &op) { return (int)op.RR; });
    case XFM_MACRO_SSG1:
    case XFM_MACRO_SSG2:
    case XFM_MACRO_SSG3:
    case XFM_MACRO_SSG4:
        return opValue(XFM_MACRO_SSG1, [](const xfm_patch_opn_operator &op) { return (int)op.SSG; });
    default:
        return Tracker_MacroDefaultValue(target);
    }
}

inline int Tracker_MacroEnabledColumns(const XfmMacro *macro)
{
    if (!macro) return 0;
    return std::max(0, std::min(TRACKER_MACRO_UI_STEPS, (int)macro->length));
}

inline void Tracker_NormalizeMacroUiState(XfmMacro *macro)
{
    if (!macro) return;
    macro->length = (uint8_t)Tracker_MacroEnabledColumns(macro);
    if (macro->length == 0)
    {
        macro->has_loop = false;
        macro->loop_start = 0;
        macro->release_start = 0xFF;
        return;
    }
    if (macro->has_loop && macro->loop_start >= macro->length)
    {
        macro->has_loop = false;
        macro->loop_start = 0;
    }
    if (macro->release_start != 0xFF)
    {
        if (macro->release_start >= macro->length)
            macro->release_start = 0xFF;
        else if (macro->has_loop && macro->release_start <= macro->loop_start)
            macro->release_start = (macro->loop_start + 1 < macro->length) ? (uint8_t)(macro->loop_start + 1) : 0xFF;
    }
    if (!Tracker_MacroTargetSupportsRelease(macro->target))
        macro->release_start = 0xFF;
}

inline XfmMacro &Tracker_EditableMacro(Tracker *self)
{
    static XfmMacro fallback = {};
    if (!self)
    {
        Tracker_DefaultMacro(&fallback, XFM_MACRO_TL1);
        return fallback;
    }
    int inst = std::max(0, std::min(255, self->editInstrument));
    int target = std::max((int)XFM_MACRO_TL1, std::min(Tracker_MacroMaxTarget(), self->editMacroTarget));
    self->editMacroTarget = target;
    if (!self->editMacroValid[inst][target])
    {
        Tracker_DefaultMacro(&self->editMacros[inst][target], target);
        self->editMacroValid[inst][target] = true;
    }
    XfmMacro &macro = self->editMacros[inst][target];
    macro.target = (uint8_t)target;
    Tracker_NormalizeMacroUiState(&macro);
    int enabled = Tracker_MacroEnabledColumns(&macro);
    self->editMacroValueIndex = enabled > 0 ? std::max(0, std::min(enabled - 1, self->editMacroValueIndex)) : 0;
    return macro;
}

inline void Tracker_MarkMacroDirty(Tracker *self)
{
    if (!self) return;
    Tracker_RequestSongCopyOnWriteForInstrumentEdit(self);
    int inst = std::max(0, std::min(255, self->editInstrument));
    int target = std::max((int)XFM_MACRO_TL1, std::min(Tracker_MacroMaxTarget(), self->editMacroTarget));
    self->editMacroValid[inst][target] = true;
    self->editMacroDirty[inst][target] = true;
    self->copyOnWriteRequested = true;
}

inline int Tracker_MacroEnabledCount(const Tracker *self)
{
    if (!self) return 0;
    int inst = std::max(0, std::min(255, self->editInstrument));
    int count = 0;
    for (int target = XFM_MACRO_TL1; target < XFM_MACRO_TARGET_COUNT; target++)
        if (self->editMacroEnabled[inst][target])
            count++;
    return count;
}

inline void Tracker_MacroTargetValueRange(int target, int &valueMin, int &valueMax)
{
    valueMin = -64;
    valueMax = 127;
    if (target >= XFM_MACRO_TL1 && target <= XFM_MACRO_TL4) valueMin = 0, valueMax = 127;
    else if (target >= XFM_MACRO_MUL1 && target <= XFM_MACRO_MUL4) valueMin = 0, valueMax = 15;
    else if (target >= XFM_MACRO_DT1 && target <= XFM_MACRO_DT4) valueMin = -3, valueMax = 3;
    else if (target >= XFM_MACRO_AR1 && target <= XFM_MACRO_SR4) valueMin = 0, valueMax = 31;
    else if (target >= XFM_MACRO_SL1 && target <= XFM_MACRO_RR4) valueMin = 0, valueMax = 15;
    else if (target >= XFM_MACRO_SSG1 && target <= XFM_MACRO_SSG4) valueMin = 0, valueMax = 8;
    else if (target == XFM_MACRO_FB) valueMin = 0, valueMax = 7;
    else if (target == XFM_MACRO_ARP) valueMin = -64, valueMax = 64;
    else if (target == XFM_MACRO_PAN) valueMin = 0, valueMax = 3;
    else if (target == XFM_MACRO_PITCH || target == XFM_MACRO_RELATIVE) valueMin = -2048, valueMax = 2047;
    else if (target == XFM_MACRO_PHASE_RESET) valueMin = 0, valueMax = 1;
}

inline bool Tracker_MacroTargetUsesInvertedVerticalValue(int target)
{
    return (target >= XFM_MACRO_TL1 && target <= XFM_MACRO_TL4) ||
           (target >= XFM_MACRO_AR1 && target <= XFM_MACRO_RR4);
}

inline int Tracker_MacroVisibleValueSpan(int target, int valueMin, int valueMax)
{
    const int fullSpan = std::max(1, valueMax - valueMin);
    if (target == XFM_MACRO_ARP) return 12;
    if (fullSpan <= 24) return fullSpan;
    if (fullSpan <= 128) return 24;
    return 64;
}

inline int Tracker_MacroDefaultValueViewMin(int target, int valueMin, int valueMax)
{
    const int visibleSpan = Tracker_MacroVisibleValueSpan(target, valueMin, valueMax);
    if (valueMin < 0 && valueMax > 0)
        return -visibleSpan / 2;
    return valueMin;
}

inline void Tracker_SetMacroValueViewMin(Tracker *self, int viewMin, int target, int valueMin, int valueMax)
{
    if (!self) return;
    const int visibleSpan = Tracker_MacroVisibleValueSpan(target, valueMin, valueMax);
    const int maxViewMin = std::max(valueMin, valueMax - visibleSpan);
    self->macroValueViewMin = std::max(valueMin, std::min(maxViewMin, viewMin));
}

inline void Tracker_EnsureMacroValueViewForRange(Tracker *self, int target, int valueMin, int valueMax)
{
    if (!self) return;
    Tracker_SetMacroValueViewMin(self, self->macroValueViewMin, target, valueMin, valueMax);
}

inline void Tracker_EnsureMacroCapacity(XfmMacro *macro)
{
    if (!macro) return;
    Tracker_NormalizeMacroUiState(macro);
}

inline void Tracker_EnableMacroThrough(Tracker *self, int lastEnabledIndex)
{
    if (!self) return;
    XfmMacro &macro = Tracker_EditableMacro(self);
    int idx = std::max(0, std::min(TRACKER_MACRO_UI_STEPS - 1, lastEnabledIndex));
    int oldLength = Tracker_MacroEnabledColumns(&macro);
    if (oldLength <= idx)
    {
        int16_t fill = oldLength > 0 ? macro.values[oldLength - 1] : Tracker_MacroTargetBaseValue(self, self->editMacroTarget);
        for (int i = oldLength; i <= idx; i++)
            macro.values[i] = fill;
        macro.length = (uint8_t)(idx + 1);
    }
    Tracker_NormalizeMacroUiState(&macro);
}

inline void Tracker_DisableMacroFrom(Tracker *self, int firstDisabledIndex)
{
    if (!self) return;
    XfmMacro &macro = Tracker_EditableMacro(self);
    int oldLength = Tracker_MacroEnabledColumns(&macro);
    int clamped = std::max(0, std::min(TRACKER_MACRO_UI_STEPS, firstDisabledIndex));
    int oldLoopEnd = macro.release_start == 0xFF ? oldLength : (int)macro.release_start;
    macro.length = (uint8_t)clamped;
    if (macro.has_loop && oldLoopEnd > clamped)
    {
        macro.has_loop = false;
        macro.loop_start = 0;
        macro.release_start = 0xFF;
    }
    Tracker_NormalizeMacroUiState(&macro);
    self->editMacroValueIndex = clamped > 0 ? std::min(self->editMacroValueIndex, clamped - 1) : 0;
    Tracker_MarkMacroDirty(self);
}

inline std::string Tracker_BuildCustomInstrumentText(const Tracker *tracker)
{
    std::string out;
    if (!tracker) return out;
    bool usedByPattern[256] = {};
    for (int row = 0; row < tracker->rowCount; row++)
    {
        for (int ch = 0; ch < TRACKER_CHANNELS; ch++)
        {
            int inst = Tracker_ParseCellInstrument(tracker->cells[row][ch].text);
            if (inst >= 0)
                usedByPattern[inst] = true;
        }
    }
    for (int inst = 0; inst < 256; inst++)
    {
        bool hasMacros = false;
        for (int target = XFM_MACRO_TL1; target < XFM_MACRO_TARGET_COUNT; target++)
            if (tracker->editMacroEnabled[inst][target] && tracker->editMacroValid[inst][target])
            {
                XfmMacro macro = tracker->editMacros[inst][target];
                Tracker_NormalizeMacroUiState(&macro);
                if (macro.length > 0)
                {
                    hasMacros = true;
                    break;
                }
            }
        bool hasName = tracker->instrumentNameLengths[inst] > 0;
        bool shouldSave = tracker->availableInstruments[inst] || usedByPattern[inst] ||
                          tracker->editPatchValid[inst] || hasMacros || hasName;
        if (!shouldSave)
            continue;

        xfm_patch_opn patch = tracker->editPatchValid[inst] ? tracker->editPatches[inst] : Tracker_DefaultPatch();
        char line[512];
        std::snprintf(line, sizeof(line), "INST %02X\n", inst);
        out += line;
        if (tracker->instrumentNameLengths[inst] > 0)
        {
            std::snprintf(line, sizeof(line), "NAME %s\n", tracker->instrumentNames[inst]);
            out += line;
        }
        std::snprintf(line, sizeof(line), "COLOR %06X\n", (unsigned int)Tracker_InstrumentColorU32(tracker, inst));
        out += line;
        TrackerSongIOPatchWidths patchWidths =
            TrackerSongIO_MakePatchWidths((int)patch.ALG, (int)patch.FB, (int)patch.AMS, (int)patch.FMS);
        TrackerSongIOFmWidths fmWidths = TrackerSongIO_DefaultFmWidths();
        for (int op = 0; op < 4; op++)
        {
            const xfm_patch_opn_operator &o = patch.op[op];
            TrackerSongIO_ExpandFmWidths(
                fmWidths,
                op + 1,
                (int)o.TL,
                (int)o.AR,
                (int)o.DR,
                (int)o.SL,
                (int)o.SR,
                (int)o.RR,
                (int)o.SSG,
                (int)o.MUL,
                (int)o.DT,
                (int)o.RS,
                (int)o.AM
            );
        }
        out += TrackerSongIO_FormatLegacyPatchGuideLine(patchWidths);
        out += TrackerSongIO_FormatLegacyPatchLine((int)patch.ALG, (int)patch.FB, (int)patch.AMS, (int)patch.FMS, patchWidths);
        out += TrackerSongIO_FormatLegacyFmGuideLine(fmWidths);
        for (int op = 0; op < 4; op++)
        {
            const xfm_patch_opn_operator &o = patch.op[op];
            out += TrackerSongIO_FormatLegacyFmOpLine(
                op + 1,
                (int)o.TL,
                (int)o.AR,
                (int)o.DR,
                (int)o.SL,
                (int)o.SR,
                (int)o.RR,
                (int)o.SSG,
                (int)o.MUL,
                (int)o.DT,
                (int)o.RS,
                (int)o.AM,
                fmWidths
            );
        }
        for (int target = XFM_MACRO_TL1; target < XFM_MACRO_TARGET_COUNT; target++)
        {
            if (!tracker->editMacroEnabled[inst][target] || !tracker->editMacroValid[inst][target])
                continue;
            XfmMacro macro = tracker->editMacros[inst][target];
            Tracker_NormalizeMacroUiState(&macro);
            if (macro.length == 0)
                continue;
            std::snprintf(
                line,
                sizeof(line),
                "MACRO %d %u %u %u",
                target,
                macro.length,
                macro.has_loop ? macro.loop_start : 255,
                macro.release_start
            );
            out += line;
            for (int i = 0; i < macro.length; i++)
            {
                std::snprintf(line, sizeof(line), " %d", (int)macro.values[i]);
                out += line;
            }
            out += '\n';
        }
        out += "ENDINST\n";
    }
    return out;
}

inline void Tracker_SetMacroLoopRange(Tracker *self, int a, int b)
{
    if (!self) return;
    XfmMacro &macro = Tracker_EditableMacro(self);
    int start = std::max(0, std::min(a, b));
    int end = std::min(TRACKER_MACRO_UI_STEPS - 1, std::max(a, b));
    const bool keepRelease = Tracker_MacroTargetSupportsRelease(macro.target) &&
        macro.release_start != 0xFF &&
        macro.release_start < macro.length;
    int wantedLength = end + 1;
    int wantedReleaseStart = 0xFF;
    if (keepRelease)
    {
        wantedReleaseStart = std::min(TRACKER_MACRO_UI_STEPS - 1, end + 1);
        wantedLength = std::max((int)macro.length, wantedReleaseStart + 1);
    }
    Tracker_EnableMacroThrough(self, std::min(TRACKER_MACRO_UI_STEPS - 1, wantedLength - 1));
    macro.has_loop = true;
    macro.loop_start = (uint8_t)start;
    macro.release_start = keepRelease && wantedReleaseStart < wantedLength ? (uint8_t)wantedReleaseStart : 0xFF;
    Tracker_NormalizeMacroUiState(&macro);
    Tracker_MarkMacroDirty(self);
}

inline void Tracker_ClearMacroLoopRange(Tracker *self)
{
    if (!self) return;
    XfmMacro &macro = Tracker_EditableMacro(self);
    macro.has_loop = false;
    macro.loop_start = 0;
    Tracker_NormalizeMacroUiState(&macro);
    Tracker_MarkMacroDirty(self);
}

inline void Tracker_SetMacroReleaseStart(Tracker *self, int index)
{
    if (!self) return;
    XfmMacro &macro = Tracker_EditableMacro(self);
    if (!Tracker_MacroTargetSupportsRelease(macro.target))
    {
        macro.release_start = 0xFF;
        Tracker_NormalizeMacroUiState(&macro);
        Tracker_MarkMacroDirty(self);
        return;
    }
    int idx = std::max(0, std::min(TRACKER_MACRO_UI_STEPS - 1, index));
    Tracker_EnableMacroThrough(self, idx);
    macro.release_start = (uint8_t)idx;
    Tracker_NormalizeMacroUiState(&macro);
    Tracker_MarkMacroDirty(self);
}

inline void Tracker_SetMacroReleaseRange(Tracker *self, int a, int b)
{
    if (!self) return;
    XfmMacro &macro = Tracker_EditableMacro(self);
    if (!Tracker_MacroTargetSupportsRelease(macro.target))
    {
        macro.release_start = 0xFF;
        Tracker_NormalizeMacroUiState(&macro);
        Tracker_MarkMacroDirty(self);
        return;
    }
    int start = std::max(0, std::min(a, b));
    int end = std::min(TRACKER_MACRO_UI_STEPS - 1, std::max(a, b));
    if (macro.has_loop && start <= macro.loop_start)
        start = std::min(TRACKER_MACRO_UI_STEPS - 1, (int)macro.loop_start + 1);
    end = std::max(start, end);
    Tracker_EnableMacroThrough(self, end);
    macro.release_start = (uint8_t)start;
    Tracker_NormalizeMacroUiState(&macro);
    Tracker_MarkMacroDirty(self);
}

inline void Tracker_ClearMacroReleaseStart(Tracker *self)
{
    if (!self) return;
    XfmMacro &macro = Tracker_EditableMacro(self);
    int oldReleaseStart = macro.release_start;
    bool loopStillActive = macro.has_loop;
    if (oldReleaseStart != 0xFF && loopStillActive)
        macro.length = (uint8_t)std::max(0, oldReleaseStart);
    macro.release_start = 0xFF;
    Tracker_NormalizeMacroUiState(&macro);
    Tracker_MarkMacroDirty(self);
}

inline void Tracker_SetMacroViewFirst(Tracker *self, int first)
{
    if (!self) return;
    int maxFirst = std::max(0, TRACKER_MACRO_UI_STEPS - TRACKER_MACRO_VISIBLE_STEPS);
    first = std::max(0, std::min(maxFirst, first));
    first = (first / TRACKER_MACRO_SCROLL_STEP) * TRACKER_MACRO_SCROLL_STEP;
    self->macroViewFirst = std::max(0, std::min(maxFirst, first));
}

inline bool Tracker_MacroCanScroll(Tracker *self, int direction)
{
    if (!self || direction == 0)
        return false;
    int maxFirst = std::max(0, TRACKER_MACRO_UI_STEPS - TRACKER_MACRO_VISIBLE_STEPS);
    return direction < 0 ? self->macroViewFirst > 0 : self->macroViewFirst < maxFirst;
}

inline void Tracker_UpdateMacroRangeSelectionEndpoint(Tracker *self, int index)
{
    if (!self)
        return;
    index = std::max(0, std::min(TRACKER_MACRO_UI_STEPS - 1, index));
    if (self->macroSelectMode == TRACKER_MACRO_SELECT_LOOP)
        Tracker_SetMacroLoopRange(self, self->macroRangeAnchor, index);
    else
        Tracker_SetMacroReleaseRange(self, self->macroRangeAnchor, index);
}

inline void Tracker_MacroRangeAutoScrollStep(Tracker *self, int direction)
{
    if (!Tracker_MacroCanScroll(self, direction))
        return;
    Tracker_SetMacroViewFirst(self, self->macroViewFirst + direction * TRACKER_MACRO_SCROLL_STEP);
    int edgeIndex = direction < 0 ? self->macroViewFirst :
        std::min(TRACKER_MACRO_UI_STEPS - 1, self->macroViewFirst + TRACKER_MACRO_VISIBLE_STEPS - 1);
    Tracker_UpdateMacroRangeSelectionEndpoint(self, edgeIndex);
}

inline void Tracker_SetMacroRangeAutoScroll(Tracker *self, int direction)
{
    if (!self)
        return;
    if (!Tracker_MacroCanScroll(self, direction))
        direction = 0;
    if (self->macroRangeAutoScrollDir != direction)
    {
        self->macroRangeAutoScrollDir = direction;
        self->macroRangeAutoScrollTimer = 0.0f;
    }
}

inline int Tracker_MacroVisibleIndexAtX(Tracker *self, float pointerX)
{
    if (!self) return 0;
    Clay_BoundingBox b = Clay_GetElementData(CLAY_ID("TrackerMacroGraphClip")).boundingBox;
    float xT = b.width > 0.0f ? (pointerX - b.x) / b.width : 0.0f;
    xT = std::max(0.0f, std::min(0.9999f, xT));
    int local = std::max(0, std::min(TRACKER_MACRO_VISIBLE_STEPS - 1, (int)std::floor(xT * (float)TRACKER_MACRO_VISIBLE_STEPS)));
    return std::max(0, std::min(TRACKER_MACRO_UI_STEPS - 1, self->macroViewFirst + local));
}

inline const char *Tracker_FindPatternRows(const char *pattern)
{
    if (!pattern) return nullptr;
    const char *p = pattern;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    while (*p >= '0' && *p <= '9') p++;
    while (*p == ' ' || *p == '\t' || *p == '\r') p++;
    if (*p == '\n') p++;
    return p;
}

inline void Tracker_SetPartName(TrackerPart *part, const char *name)
{
    if (!part) return;
    const char *src = name && name[0] ? name : "PART";
    std::snprintf(part->name, sizeof(part->name), "%s", src);
    part->nameLen = (int32_t)std::strlen(part->name);
}

inline void Tracker_ResetSinglePart(Tracker *self, const char *name = "PART 1")
{
    if (!self) return;
    self->partCount = 1;
    self->parts[0].startRow = 0;
    self->parts[0].rowCount = std::max(0, self->rowCount);
    self->parts[0].collapsed = false;
    self->parts[0].enabled = true;
    Tracker_SetPartName(&self->parts[0], name);
    for (int i = 1; i < TRACKER_MAX_PARTS; i++)
        self->parts[i] = {};
}

inline void Tracker_NormalizeParts(Tracker *self)
{
    if (!self) return;
    if (self->rowCount < 1) self->rowCount = 1;
    self->rowCount = std::max(1, std::min(TRACKER_MAX_ROWS, self->rowCount));
    if (self->partCount < 1)
        Tracker_ResetSinglePart(self);
    self->partCount = std::max(1, std::min(TRACKER_MAX_PARTS, self->partCount));

    int cursor = 0;
    for (int i = 0; i < self->partCount; i++)
    {
        TrackerPart &part = self->parts[i];
        part.startRow = cursor;
        int remainingRows = self->rowCount - cursor;
        int remainingParts = self->partCount - i - 1;
        int minForLater = remainingParts > 0 ? remainingParts : 0;
        part.rowCount = std::max(0, std::min(part.rowCount, remainingRows - minForLater));
        if (i == self->partCount - 1)
            part.rowCount = std::max(0, self->rowCount - cursor);
        if (part.nameLen <= 0 || part.name[0] == '\0')
        {
            char generated[TRACKER_PART_NAME_CAPACITY];
            std::snprintf(generated, sizeof(generated), "PART %d", i + 1);
            Tracker_SetPartName(&part, generated);
        }
        cursor += part.rowCount;
    }
    if (cursor != self->rowCount)
        self->parts[self->partCount - 1].rowCount += self->rowCount - cursor;
}

inline int Tracker_PartIndexForRow(const Tracker *self, int row)
{
    if (!self || self->partCount <= 0) return 0;
    if (self->partCount == 1 && self->parts[0].rowCount <= 0)
        return 0;
    row = std::max(0, std::min(std::max(0, self->rowCount - 1), row));
    for (int i = 0; i < self->partCount; i++)
    {
        const TrackerPart &part = self->parts[i];
        if (row >= part.startRow && row < part.startRow + part.rowCount)
            return i;
    }
    return self->partCount - 1;
}

inline int Tracker_PartEndRow(const Tracker *self, int partIndex)
{
    if (!self || partIndex < 0 || partIndex >= self->partCount) return 0;
    return self->parts[partIndex].startRow + self->parts[partIndex].rowCount;
}

inline int Tracker_CurrentPartIndex(const Tracker *self)
{
    if (!self) return 0;
    return Tracker_PartIndexForRow(self, self->editRow >= 0 ? self->editRow : self->playRow);
}

inline float Tracker_SmoothStep01(float t)
{
    t = std::max(0.0f, std::min(1.0f, t));
    return t * t * (3.0f - 2.0f * t);
}

inline float Tracker_PartBodyOpenFraction(const Tracker *self, int partIndex)
{
    if (!self || partIndex < 0 || partIndex >= self->partCount) return 0.0f;
    const TrackerPart &part = self->parts[partIndex];
    if (!part.collapseAnimating)
        return part.collapsed ? 0.0f : 1.0f;
    float eased = Tracker_SmoothStep01(part.collapseAnimT);
    return std::max(0.0f, std::min(1.0f, part.collapseAnimFrom + (part.collapseAnimTo - part.collapseAnimFrom) * eased));
}

inline bool Tracker_PartRowsVisibleForLayout(const Tracker *self, int partIndex)
{
    if (!self || partIndex < 0 || partIndex >= self->partCount) return false;
    const TrackerPart &part = self->parts[partIndex];
    return !part.collapsed || part.collapseAnimating;
}

inline bool Tracker_PartCollapseIconShowsCollapsed(const Tracker *self, int partIndex)
{
    if (!self || partIndex < 0 || partIndex >= self->partCount) return false;
    const TrackerPart &part = self->parts[partIndex];
    if (part.collapseAnimating)
        return part.collapseAnimTo <= 0.001f;
    return part.collapsed;
}

inline int Tracker_RepeatPartIndex(const Tracker *self)
{
    if (!self) return -1;
    for (int i = 0; i < self->partCount; i++)
        if (self->parts[i].repeat && self->parts[i].rowCount > 0)
            return i;
    return -1;
}

inline bool Tracker_PartEffectiveEnabledForPlayback(const Tracker *self, int partIndex)
{
    if (!self || partIndex < 0 || partIndex >= self->partCount)
        return true;
    int repeatPart = Tracker_RepeatPartIndex(self);
    if (repeatPart >= 0)
        return partIndex == repeatPart;
    return self->parts[partIndex].enabled;
}

inline void Tracker_ClearPartRepeat(Tracker *self)
{
    if (!self) return;
    for (int i = 0; i < self->partCount; i++)
        self->parts[i].repeat = false;
}

inline void Tracker_SetPartRepeat(Tracker *self, int partIndex, bool repeat)
{
    if (!self || partIndex < 0 || partIndex >= self->partCount)
        return;
    Tracker_ClearPartRepeat(self);
    if (repeat)
        self->parts[partIndex].repeat = true;
}

inline void Tracker_HandlePartEnableButton(Tracker *self, int partIndex, bool longClick)
{
    if (!self || partIndex < 0 || partIndex >= self->partCount)
        return;
    TrackerPart &part = self->parts[partIndex];
    if (part.repeat)
    {
        part.repeat = false;
        self->playbackArrangementDirty = true;
        self->songLengthDirty = true;
        return;
    }
    if (longClick)
    {
        Tracker_SetPartRepeat(self, partIndex, true);
        self->playRow = std::max(0, std::min(part.startRow, std::max(0, self->rowCount - 1)));
        self->playTick = 0;
        self->musicSeekRequested = true;
        self->musicSeekRow = self->playRow;
        self->musicSeekTick = 0;
        self->playbackArrangementDirty = true;
        self->songLengthDirty = true;
        return;
    }
    part.enabled = !part.enabled;
    self->patternDirty = true;
    self->copyOnWriteRequested = true;
}

inline bool Tracker_AnyPartCollapseAnimating(const Tracker *self)
{
    if (!self) return false;
    for (int i = 0; i < self->partCount; i++)
        if (self->parts[i].collapseAnimating)
            return true;
    return false;
}

inline float Tracker_ContentHeight(const Tracker *self)
{
    if (!self) return 0.0f;
    if (self->partCount == 1 && self->parts[0].rowCount <= 0)
        return (float)std::max(1, self->rowCount + 1) * self->rowHeight;
    float height = 0.0f;
    for (int i = 0; i < self->partCount; i++)
    {
        const TrackerPart &part = self->parts[i];
        height += self->rowHeight;
        if (Tracker_PartRowsVisibleForLayout(self, i))
            height += (float)part.rowCount * self->rowHeight * Tracker_PartBodyOpenFraction(self, i);
    }
    return std::max(self->rowHeight, height);
}

inline int Tracker_VisibleRowCount(const Tracker *self)
{
    if (!self) return 0;
    if (self->partCount == 1 && self->parts[0].rowCount <= 0)
        return std::max(1, self->rowCount + 1);
    int visualRows = 0;
    for (int i = 0; i < self->partCount; i++)
        visualRows += 1 + (Tracker_PartRowsVisibleForLayout(self, i) ? self->parts[i].rowCount : 0);
    return std::max(1, visualRows);
}

inline int Tracker_VisualIndexForPartTitle(const Tracker *self, int partIndex)
{
    if (!self) return 0;
    int visual = 0;
    partIndex = std::max(0, std::min(std::max(0, self->partCount - 1), partIndex));
    for (int i = 0; i < partIndex; i++)
        visual += 1 + (Tracker_PartRowsVisibleForLayout(self, i) ? self->parts[i].rowCount : 0);
    return visual;
}

inline int Tracker_VisualIndexForPartBoundary(const Tracker *self, int partIndex)
{
    if (!self || partIndex < 0 || partIndex + 1 >= self->partCount)
        return -1;
    return Tracker_VisualIndexForPartTitle(self, partIndex + 1);
}

inline int Tracker_VisualIndexForRow(const Tracker *self, int row)
{
    if (!self) return 0;
    int partIndex = Tracker_PartIndexForRow(self, row);
    const TrackerPart &part = self->parts[partIndex];
    if (self->partCount == 1 && part.rowCount <= 0)
        return std::max(0, std::min(std::max(0, self->rowCount - 1), row)) + 1;
    int visual = Tracker_VisualIndexForPartTitle(self, partIndex);
    if (!Tracker_PartRowsVisibleForLayout(self, partIndex))
        return visual;
    return visual + 1 + std::max(0, std::min(part.rowCount - 1, row - part.startRow));
}

inline TrackerVisualRow Tracker_MapVisualIndex(const Tracker *self, int visualIndex)
{
    TrackerVisualRow out = {};
    if (!self || self->partCount <= 0) return out;
    if (self->partCount == 1 && self->parts[0].rowCount <= 0)
    {
        int visual = std::max(0, visualIndex);
        if (visual == 0)
        {
            out.kind = TRACKER_VISUAL_ROW_PART_TITLE;
            out.part = 0;
            out.row = 0;
            return out;
        }
        out.kind = TRACKER_VISUAL_ROW_CELL;
        out.part = 0;
        out.localRow = std::max(0, std::min(std::max(0, self->rowCount - 1), visual - 1));
        out.row = out.localRow;
        return out;
    }
    int visual = std::max(0, visualIndex);
    for (int i = 0; i < self->partCount; i++)
    {
        const TrackerPart &part = self->parts[i];
        if (visual == 0)
        {
            out.kind = TRACKER_VISUAL_ROW_PART_TITLE;
            out.part = i;
            out.row = part.startRow;
            out.localRow = -1;
            return out;
        }
        visual--;
        if (Tracker_PartRowsVisibleForLayout(self, i))
        {
            if (visual < part.rowCount)
            {
                out.kind = TRACKER_VISUAL_ROW_CELL;
                out.part = i;
                out.localRow = visual;
                out.row = part.startRow + visual;
                return out;
            }
            visual -= part.rowCount;
        }
    }
    int last = std::max(0, self->partCount - 1);
    out.kind = TRACKER_VISUAL_ROW_PART_TITLE;
    out.part = last;
    out.row = self->parts[last].startRow;
    return out;
}

inline int Tracker_VisualIndexAtViewportY(const Tracker *self, float localY)
{
    if (!self || self->rowHeight <= 0.0f) return 0;
    return std::max(0, std::min(Tracker_VisibleRowCount(self) - 1, (int)std::floor((localY + self->scrollY) / self->rowHeight)));
}

inline int Tracker_FirstEditableRowForVisualY(const Tracker *self, float localY)
{
    TrackerVisualRow visual = Tracker_MapVisualIndex(self, Tracker_VisualIndexAtViewportY(self, localY));
    if (visual.kind == TRACKER_VISUAL_ROW_CELL)
        return visual.row;
    if (visual.part >= 0 && visual.part < self->partCount)
        return self->parts[visual.part].startRow;
    return 0;
}

inline int Tracker_StickyPartIndexAtScroll(const Tracker *self)
{
    if (!self || self->partCount <= 0 || self->rowHeight <= 0.0f)
        return -1;
    int visualIndex = std::max(0, std::min(Tracker_VisibleRowCount(self) - 1, (int)std::floor(std::max(0.0f, self->scrollY) / self->rowHeight)));
    TrackerVisualRow visual = Tracker_MapVisualIndex(self, visualIndex);
    return visual.part >= 0 && visual.part < self->partCount ? visual.part : -1;
}

inline float Tracker_StickyPartTitleTopY(const Tracker *self, int partIndex)
{
    if (!self || partIndex < 0 || partIndex >= self->partCount || self->rowHeight <= 0.0f)
        return 0.0f;
    float titleTop = (float)Tracker_VisualIndexForPartTitle(self, partIndex) * self->rowHeight - self->scrollY;
    return std::max(0.0f, titleTop);
}

inline float Tracker_PartPlaybackProgress(const Tracker *self, int partIndex)
{
    if (!self || partIndex < 0 || partIndex >= self->partCount) return 0.0f;
    const TrackerPart &part = self->parts[partIndex];
    if (part.rowCount <= 0) return 1.0f;
    if (self->playRow < part.startRow) return 0.0f;
    if (self->playRow >= part.startRow + part.rowCount) return 1.0f;
    float tick = self->ticksPerRow > 0 ? (float)self->playTick / (float)self->ticksPerRow : 0.0f;
    return std::max(0.0f, std::min(1.0f, ((float)(self->playRow - part.startRow) + tick) / (float)part.rowCount));
}

inline bool Tracker_PartLoopOverlap(
    const Tracker *self,
    int partIndex,
    int *outStartRow = nullptr,
    int *outEndRow = nullptr,
    float *outStart01 = nullptr,
    float *outEnd01 = nullptr)
{
    if (!Tracker_HasPlaySelection(self) || partIndex < 0 || partIndex >= self->partCount)
        return false;
    const TrackerPart &part = self->parts[partIndex];
    if (part.rowCount <= 0)
        return false;
    const int partStart = part.startRow;
    const int partEnd = part.startRow + part.rowCount - 1;
    const int start = std::max(partStart, self->loopStart);
    const int end = std::min(partEnd, self->loopEnd);
    if (start > end)
        return false;
    if (outStartRow) *outStartRow = start;
    if (outEndRow) *outEndRow = end;
    if (outStart01) *outStart01 = (float)(start - partStart) / (float)part.rowCount;
    if (outEnd01) *outEnd01 = (float)(end - partStart + 1) / (float)part.rowCount;
    return true;
}

inline TrackerPartProgressVisual Tracker_PartProgressVisualForPart(const Tracker *self, int partIndex)
{
    TrackerPartProgressVisual visual = {};
    if (!self || partIndex < 0 || partIndex >= self->partCount)
        return visual;
    const TrackerPart &part = self->parts[partIndex];
    if (part.rowCount <= 0)
        return visual;

    if (Tracker_HasPlaySelection(self))
    {
        int start = 0;
        int end = 0;
        if (!Tracker_PartLoopOverlap(self, partIndex, &start, &end, &visual.segmentStart01, &visual.segmentEnd01))
            return visual;
        const int selectionRows = std::max(1, end - start + 1);
        const float tick = self->ticksPerRow > 0 ? (float)self->playTick / (float)self->ticksPerRow : 0.0f;
        if (self->playRow < start)
            visual.progress01 = 0.0f;
        else if (self->playRow > end)
            visual.progress01 = 1.0f;
        else
            visual.progress01 = ((float)(self->playRow - start) + tick) / (float)selectionRows;
        visual.progress01 = std::max(0.0f, std::min(1.0f, visual.progress01));
        visual.selectionMode = true;
        visual.visible = true;
        return visual;
    }

    if (!Tracker_PartEffectiveEnabledForPlayback(self, partIndex))
        return visual;
    visual.visible = true;
    visual.segmentStart01 = 0.0f;
    visual.segmentEnd01 = 1.0f;
    visual.progress01 = Tracker_PartPlaybackProgress(self, partIndex);
    return visual;
}

inline bool Tracker_PartProgressHitAllowsSeek(const Tracker *self, int partIndex, float pointerX, float railX, float railW)
{
    if (!self || railW <= 1.0f)
        return false;
    TrackerPartProgressVisual visual = Tracker_PartProgressVisualForPart(self, partIndex);
    if (!visual.visible)
        return false;
    if (!visual.selectionMode)
        return true;
    float x01 = (pointerX - railX) / railW;
    return x01 >= visual.segmentStart01 && x01 <= visual.segmentEnd01;
}

inline void Tracker_SetPlayheadFromPartProgressX(Tracker *self, int partIndex, float pointerX, float railX, float railW)
{
    if (!self || partIndex < 0 || partIndex >= self->partCount || railW <= 1.0f)
        return;
    const TrackerPart &part = self->parts[partIndex];
    if (part.rowCount <= 0)
        return;
    int targetStart = part.startRow;
    int targetRows = part.rowCount;
    float progress = std::max(0.0f, std::min(0.9999f, (pointerX - railX) / railW));
    if (Tracker_HasPlaySelection(self))
    {
        int overlapStart = 0;
        int overlapEnd = 0;
        float segmentStart = 0.0f;
        float segmentEnd = 0.0f;
        if (!Tracker_PartLoopOverlap(self, partIndex, &overlapStart, &overlapEnd, &segmentStart, &segmentEnd))
            return;
        if (progress < segmentStart || progress > segmentEnd)
            return;
        const float segmentWidth = std::max(0.0001f, segmentEnd - segmentStart);
        progress = std::max(0.0f, std::min(0.9999f, (progress - segmentStart) / segmentWidth));
        targetStart = overlapStart;
        targetRows = overlapEnd - overlapStart + 1;
    }
    else if (!Tracker_PartEffectiveEnabledForPlayback(self, partIndex))
    {
        return;
    }
    float rowFloat = progress * (float)targetRows;
    int localRow = std::max(0, std::min(targetRows - 1, (int)std::floor(rowFloat)));
    float rowFrac = rowFloat - (float)localRow;
    int ticks = std::max(1, self->ticksPerRow);
    int tick = std::max(0, std::min(ticks - 1, (int)std::floor(rowFrac * (float)ticks)));
    const int targetRow = targetStart + localRow;
    if (self->loopEnabled && (targetRow < self->loopStart || targetRow > self->loopEnd))
    {
        self->loopStart = part.startRow;
        self->loopEnd = std::max(part.startRow, part.startRow + part.rowCount - 1);
        self->loopRangeDirty = true;
    }
    setTrackerCursorState(self, targetRow, tick, self->ticksPerRow);
    Tracker_RequestMusicSeekToCursor(self);
}

inline bool Tracker_RowIsDarkZebraBand(const Tracker *self, int partIndex, int localRow)
{
    if (!self) return false;
    int rowsPerBeat = std::max(1, self->songRowsPerBeat);
    if (partIndex < 0 || partIndex >= self->partCount) return false;
    if (localRow < 0) return false;
    const TrackerPart &part = self->parts[partIndex];
    if (part.rowCount <= 0) return false;
    int band = (localRow / rowsPerBeat) % 2;
    return band == 0;
}

inline bool Tracker_RowIsBrightZebraBand(const Tracker *self, int partIndex, int localRow)
{
    return !Tracker_RowIsDarkZebraBand(self, partIndex, localRow);
}

inline void Tracker_ClampEditSelection(Tracker *self);

inline void Tracker_MarkSongLengthChanged(Tracker *self)
{
    if (!self) return;
    self->patternDirty = true;
    self->songLengthDirty = true;
    self->copyOnWriteRequested = true;
    self->loopEnd = std::min(self->loopEnd, self->rowCount - 1);
    self->loopStart = std::min(self->loopStart, self->loopEnd);
    self->playRow = std::min(self->playRow, self->rowCount - 1);
    self->editRow = std::min(self->editRow, self->rowCount - 1);
    if (self->loopEnabled) self->loopRangeDirty = true;
    Tracker_NormalizeParts(self);
    Tracker_ClampEditSelection(self);
}

inline void Tracker_InsertEmptyRowAt(Tracker *self, int row)
{
    if (!self || self->rowCount >= TRACKER_MAX_ROWS) return;
    row = std::max(0, std::min(row, self->rowCount));
    for (int r = self->rowCount; r > row; r--)
        for (int ch = 0; ch < TRACKER_CHANNELS; ch++)
            self->cells[r][ch] = self->cells[r - 1][ch];
    for (int ch = 0; ch < TRACKER_CHANNELS; ch++)
        Tracker_ClearCell(&self->cells[row][ch]);
    self->rowCount++;
}

inline void Tracker_DeleteRowAt(Tracker *self, int row)
{
    if (!self || self->rowCount <= 1) return;
    row = std::max(0, std::min(row, self->rowCount - 1));
    for (int r = row; r < self->rowCount - 1; r++)
        for (int ch = 0; ch < TRACKER_CHANNELS; ch++)
            self->cells[r][ch] = self->cells[r + 1][ch];
    self->rowCount--;
}

inline void Tracker_AddRowToPart(Tracker *self, int partIndex)
{
    if (!self || self->rowCount >= TRACKER_MAX_ROWS) return;
    Tracker_NormalizeParts(self);
    partIndex = std::max(0, std::min(std::max(0, self->partCount - 1), partIndex));
    int insertAt = self->parts[partIndex].startRow + self->parts[partIndex].rowCount;
    Tracker_InsertEmptyRowAt(self, insertAt);
    self->parts[partIndex].rowCount++;
    Tracker_MarkSongLengthChanged(self);
}

inline void Tracker_RemoveRowFromPart(Tracker *self, int partIndex)
{
    if (!self || self->rowCount <= 1) return;
    Tracker_NormalizeParts(self);
    partIndex = std::max(0, std::min(std::max(0, self->partCount - 1), partIndex));
    if (self->parts[partIndex].rowCount <= 1 && self->partCount <= 1) return;
    if (self->parts[partIndex].rowCount <= 0) return;
    int removeAt = self->parts[partIndex].startRow + self->parts[partIndex].rowCount - 1;
    Tracker_DeleteRowAt(self, removeAt);
    self->parts[partIndex].rowCount--;
    if (self->parts[partIndex].rowCount <= 0 && self->partCount > 1)
    {
        for (int i = partIndex; i < self->partCount - 1; i++)
            self->parts[i] = self->parts[i + 1];
        self->partCount--;
    }
    Tracker_MarkSongLengthChanged(self);
}

inline void Tracker_AddPartAfter(Tracker *self, int partIndex)
{
    if (!self || self->partCount >= TRACKER_MAX_PARTS || self->rowCount >= TRACKER_MAX_ROWS) return;
    Tracker_NormalizeParts(self);
    partIndex = std::max(-1, std::min(self->partCount - 1, partIndex));
    int insertPart = partIndex + 1;
    int insertRow = insertPart < self->partCount ? self->parts[insertPart].startRow : self->rowCount;
    Tracker_InsertEmptyRowAt(self, insertRow);
    for (int i = self->partCount; i > insertPart; i--)
        self->parts[i] = self->parts[i - 1];
    self->partCount++;
    self->parts[insertPart] = {};
    self->parts[insertPart].startRow = insertRow;
    self->parts[insertPart].rowCount = 1;
    self->parts[insertPart].collapsed = false;
    self->parts[insertPart].enabled = true;
    char name[TRACKER_PART_NAME_CAPACITY];
    std::snprintf(name, sizeof(name), "PART %d", self->partCount);
    Tracker_SetPartName(&self->parts[insertPart], name);
    Tracker_MarkSongLengthChanged(self);
}

inline void Tracker_AddPartToEnd(Tracker *self)
{
    if (!self) return;
    Tracker_AddPartAfter(self, std::max(-1, self->partCount - 1));
}

inline bool Tracker_ClonePartAfter(Tracker *self, int partIndex)
{
    if (!self || self->partCount >= TRACKER_MAX_PARTS)
        return false;
    Tracker_NormalizeParts(self);
    partIndex = std::max(0, std::min(std::max(0, self->partCount - 1), partIndex));
    const TrackerPart source = self->parts[partIndex];
    const int cloneRows = source.rowCount;
    if (cloneRows <= 0 || self->rowCount + cloneRows > TRACKER_MAX_ROWS)
        return false;

    Tracker_AddPartAfter(self, partIndex);
    const int clonePart = partIndex + 1;
    while (self->parts[clonePart].rowCount < cloneRows && self->rowCount < TRACKER_MAX_ROWS)
        Tracker_AddRowToPart(self, clonePart);

    Tracker_NormalizeParts(self);
    if (clonePart >= self->partCount || self->parts[clonePart].rowCount != cloneRows)
        return false;

    const int cloneStart = self->parts[clonePart].startRow;
    for (int local = 0; local < cloneRows; local++)
    {
        for (int ch = 0; ch < TRACKER_CHANNELS; ch++)
            self->cells[cloneStart + local][ch] = self->cells[source.startRow + local][ch];
    }

    self->parts[clonePart].enabled = source.enabled;
    self->parts[clonePart].collapsed = false;
    char name[TRACKER_PART_NAME_CAPACITY];
    std::snprintf(name, sizeof(name), "%s COPY", source.name[0] ? source.name : "PART");
    Tracker_SetPartName(&self->parts[clonePart], name);
    Tracker_MarkSongLengthChanged(self);
    return true;
}

inline void Tracker_DeletePart(Tracker *self, int partIndex)
{
    if (!self || self->partCount <= 1) return;
    Tracker_NormalizeParts(self);
    partIndex = std::max(0, std::min(self->partCount - 1, partIndex));
    int removeStart = self->parts[partIndex].startRow;
    int removeCount = self->parts[partIndex].rowCount;
    for (int r = removeStart; r + removeCount < self->rowCount; r++)
        for (int ch = 0; ch < TRACKER_CHANNELS; ch++)
            self->cells[r][ch] = self->cells[r + removeCount][ch];
    self->rowCount = std::max(1, self->rowCount - removeCount);
    for (int i = partIndex; i < self->partCount - 1; i++)
        self->parts[i] = self->parts[i + 1];
    self->partCount--;
    Tracker_MarkSongLengthChanged(self);
}

inline void Tracker_MovePart(Tracker *self, int partIndex, int direction)
{
    if (!self || direction == 0) return;
    Tracker_NormalizeParts(self);
    const int movedTarget = partIndex + (direction < 0 ? -1 : 1);
    int other = partIndex + (direction < 0 ? -1 : 1);
    if (partIndex < 0 || partIndex >= self->partCount || other < 0 || other >= self->partCount) return;
    if (other < partIndex)
    {
        int tmp = partIndex;
        partIndex = other;
        other = tmp;
    }
    TrackerPart first = self->parts[partIndex];
    TrackerPart second = self->parts[other];
    TrackerCell temp[TRACKER_MAX_ROWS][TRACKER_CHANNELS] = {};
    for (int r = 0; r < first.rowCount; r++)
        for (int ch = 0; ch < TRACKER_CHANNELS; ch++)
            temp[r][ch] = self->cells[first.startRow + r][ch];
    for (int r = 0; r < second.rowCount; r++)
        for (int ch = 0; ch < TRACKER_CHANNELS; ch++)
            self->cells[first.startRow + r][ch] = self->cells[second.startRow + r][ch];
    for (int r = 0; r < first.rowCount; r++)
        for (int ch = 0; ch < TRACKER_CHANNELS; ch++)
            self->cells[first.startRow + second.rowCount + r][ch] = temp[r][ch];
    self->parts[partIndex] = second;
    self->parts[other] = first;
    self->patternDirty = true;
    self->copyOnWriteRequested = true;
    Tracker_NormalizeParts(self);
    Tracker_FlashPart(self, movedTarget, TRACKER_CHANGE_FLASH_EDIT, false);
}

inline void Tracker_TogglePartCollapsed(Tracker *self, int partIndex)
{
    if (!self || partIndex < 0 || partIndex >= self->partCount) return;
    TrackerPart &part = self->parts[partIndex];
    float current = Tracker_PartBodyOpenFraction(self, partIndex);
    float target = current > 0.5f ? 0.0f : 1.0f;
    if (target <= 0.001f && self->rowHeight > 0.0f && Tracker_StickyPartIndexAtScroll(self) == partIndex)
    {
        const float titleScrollY = (float)Tracker_VisualIndexForPartTitle(self, partIndex) * self->rowHeight;
        if (self->scrollY > titleScrollY + 0.5f)
        {
            self->scrollY = std::max(0.0f, std::min(Tracker_MaxScroll(self), titleScrollY));
            self->scrollVelocity = 0.0f;
        }
    }
    part.collapseAnimating = true;
    part.collapseAnimT = 0.0f;
    part.collapseAnimFrom = current;
    part.collapseAnimTo = target;
    self->patternDirty = true;
    self->copyOnWriteRequested = true;
    self->scrollY = std::max(0.0f, std::min(Tracker_MaxScroll(self), self->scrollY));
}

inline bool Tracker_RowEnabledForPlayback(const Tracker *tracker, int row)
{
    if (!tracker) return true;
    int partIndex = Tracker_PartIndexForRow(tracker, row);
    return partIndex >= 0 && partIndex < tracker->partCount ? Tracker_PartEffectiveEnabledForPlayback(tracker, partIndex) : true;
}

inline int Tracker_PlaybackRowCount(const Tracker *tracker)
{
    if (!tracker) return 1;
    int rows = 0;
    for (int partIndex = 0; partIndex < tracker->partCount; partIndex++)
        if (Tracker_PartEffectiveEnabledForPlayback(tracker, partIndex))
            rows += tracker->parts[partIndex].rowCount;
    return std::max(1, rows);
}

inline int Tracker_PlaybackRowForSongRow(const Tracker *tracker, int songRow)
{
    if (!tracker) return 0;
    songRow = std::max(0, std::min(std::max(0, tracker->rowCount - 1), songRow));
    int playbackRow = 0;
    for (int partIndex = 0; partIndex < tracker->partCount; partIndex++)
    {
        const TrackerPart &part = tracker->parts[partIndex];
        if (!Tracker_PartEffectiveEnabledForPlayback(tracker, partIndex))
            continue;
        for (int local = 0; local < part.rowCount; local++)
        {
            int row = part.startRow + local;
            if (row >= songRow)
                return playbackRow;
            playbackRow++;
        }
    }
    return std::max(0, Tracker_PlaybackRowCount(tracker) - 1);
}

inline int Tracker_SongRowForPlaybackRow(const Tracker *tracker, int playbackRow)
{
    if (!tracker) return 0;
    playbackRow = std::max(0, playbackRow);
    int playbackCursor = 0;
    for (int partIndex = 0; partIndex < tracker->partCount; partIndex++)
    {
        const TrackerPart &part = tracker->parts[partIndex];
        if (!Tracker_PartEffectiveEnabledForPlayback(tracker, partIndex))
            continue;
        for (int local = 0; local < part.rowCount; local++)
        {
            int row = part.startRow + local;
            if (playbackCursor == playbackRow)
                return row;
            playbackCursor++;
        }
    }
    return 0;
}

inline int Tracker_SongRowForLivePlaybackRow(const Tracker *tracker, int playbackRow, bool selectionOverrideActive)
{
    if (!tracker || tracker->rowCount <= 0)
        return 0;
    playbackRow = std::max(0, playbackRow);
    if (selectionOverrideActive && tracker->loopEnabled)
    {
        int row = tracker->loopStart + playbackRow;
        return std::max(0, std::min(tracker->rowCount - 1, std::min(row, tracker->loopEnd)));
    }
    return Tracker_SongRowForPlaybackRow(tracker, playbackRow);
}

inline bool Tracker_PlaybackLoopRangeForSongRange(const Tracker *tracker, int songStart, int songEnd, int *outStart, int *outEnd)
{
    if (!tracker) return false;
    if (songStart > songEnd) std::swap(songStart, songEnd);
    int playbackCursor = 0;
    int first = -1;
    int last = -1;
    for (int partIndex = 0; partIndex < tracker->partCount; partIndex++)
    {
        const TrackerPart &part = tracker->parts[partIndex];
        if (!Tracker_PartEffectiveEnabledForPlayback(tracker, partIndex))
            continue;
        for (int local = 0; local < part.rowCount; local++)
        {
            int row = part.startRow + local;
            if (row >= songStart && row <= songEnd)
            {
                if (first < 0) first = playbackCursor;
                last = playbackCursor;
            }
            playbackCursor++;
        }
    }
    if (first < 0 || last < 0) return false;
    if (outStart) *outStart = first;
    if (outEnd) *outEnd = last;
    return true;
}

inline int Tracker_PlaybackAreaRowCount(const Tracker *tracker)
{
    if (!tracker)
        return 1;
    if (Tracker_HasPlaySelection(tracker))
        return std::max(1, tracker->loopEnd - tracker->loopStart + 1);
    return Tracker_PlaybackRowCount(tracker);
}

inline float Tracker_PlaybackAreaElapsedRows(const Tracker *tracker)
{
    if (!tracker)
        return 0.0f;
    const int totalRows = Tracker_PlaybackAreaRowCount(tracker);
    const int ticksPerRow = std::max(1, tracker->ticksPerRow);
    const float tick01 = std::max(0.0f, std::min(0.9999f, (float)tracker->playTick / (float)ticksPerRow));
    float elapsedRows = 0.0f;
    if (Tracker_HasPlaySelection(tracker))
    {
        if (tracker->playRow < tracker->loopStart)
            elapsedRows = 0.0f;
        else if (tracker->playRow > tracker->loopEnd)
            elapsedRows = (float)totalRows;
        else
            elapsedRows = (float)(tracker->playRow - tracker->loopStart) + tick01;
    }
    else
    {
        elapsedRows = (float)Tracker_PlaybackRowForSongRow(tracker, tracker->playRow) + tick01;
    }
    return std::max(0.0f, std::min((float)totalRows, elapsedRows));
}

inline bool Tracker_SongRangeTouchesSkippedPart(const Tracker *tracker, int songStart, int songEnd)
{
    if (!tracker || tracker->partCount <= 0 || tracker->rowCount <= 0)
        return false;
    if (songStart > songEnd) std::swap(songStart, songEnd);
    songStart = std::max(0, std::min(tracker->rowCount - 1, songStart));
    songEnd = std::max(0, std::min(tracker->rowCount - 1, songEnd));
    for (int partIndex = 0; partIndex < tracker->partCount; partIndex++)
    {
        const TrackerPart &part = tracker->parts[partIndex];
        const int partStart = part.startRow;
        const int partEnd = std::max(partStart, part.startRow + std::max(1, part.rowCount) - 1);
        if (songEnd < partStart || songStart > partEnd)
            continue;
        if (!part.enabled)
            return true;
    }
    return false;
}

inline std::string Tracker_BuildSongRangePatternText(
    const Tracker *tracker,
    int songStart,
    int songEnd,
    bool channelSolo = false,
    int channelStart = 0,
    int channelEnd = TRACKER_CHANNELS - 1)
{
    if (!tracker || tracker->rowCount <= 0) return {};
    if (songStart > songEnd) std::swap(songStart, songEnd);
    songStart = std::max(0, std::min(tracker->rowCount - 1, songStart));
    songEnd = std::max(0, std::min(tracker->rowCount - 1, songEnd));
    channelStart = std::max(0, std::min(TRACKER_CHANNELS - 1, channelStart));
    channelEnd = std::max(channelStart, std::min(TRACKER_CHANNELS - 1, channelEnd));

    char line[256];
    std::string out;
    std::snprintf(line, sizeof(line), "%d\n", songEnd - songStart + 1);
    out += line;
    for (int row = songStart; row <= songEnd; row++)
    {
        for (int ch = 0; ch < TRACKER_CHANNELS; ch++)
        {
            if (ch > 0) out += '|';
            const bool selected = !channelSolo || (ch >= channelStart && ch <= channelEnd);
            out += selected ? tracker->cells[row][ch].text : ".......";
        }
        out += '\n';
    }
    return out;
}

inline std::string Tracker_BuildFlatPatternText(const Tracker *tracker, bool channelSolo = false, int channelStart = 0, int channelEnd = TRACKER_CHANNELS - 1)
{
    if (!tracker) return {};
    char line[256];
    std::string out;
    std::snprintf(line, sizeof(line), "%d\n", Tracker_PlaybackRowCount(tracker));
    out += line;
    int emittedRows = 0;
    channelStart = std::max(0, std::min(TRACKER_CHANNELS - 1, channelStart));
    channelEnd = std::max(channelStart, std::min(TRACKER_CHANNELS - 1, channelEnd));
    for (int partIndex = 0; partIndex < tracker->partCount; partIndex++)
    {
        const TrackerPart &part = tracker->parts[partIndex];
        if (!Tracker_PartEffectiveEnabledForPlayback(tracker, partIndex))
            continue;
        for (int local = 0; local < part.rowCount; local++)
        {
            int row = part.startRow + local;
            for (int ch = 0; ch < TRACKER_CHANNELS; ch++)
            {
                if (ch > 0) out += '|';
                bool selected = !channelSolo || (ch >= channelStart && ch <= channelEnd);
                out += selected ? tracker->cells[row][ch].text : ".......";
            }
            out += '\n';
            emittedRows++;
        }
    }
    if (emittedRows == 0)
        out += ".......|.......|.......|.......|.......|.......\n";
    return out;
}

inline std::string Tracker_BuildPartPatternText(const Tracker *tracker)
{
    if (!tracker) return {};
    char line[256];
    std::string out;
    std::snprintf(line, sizeof(line), "%d\n", tracker->rowCount);
    out += line;
    for (int partIndex = 0; partIndex < tracker->partCount; partIndex++)
    {
        const TrackerPart &part = tracker->parts[partIndex];
        out += part.enabled ? "PART " : "SKIP ";
        out += part.name[0] ? part.name : "PART";
        out += '\n';
        if (Tracker_PartCollapseIconShowsCollapsed(tracker, partIndex))
            out += "COLLAPSED\n";
        for (int local = 0; local < part.rowCount; local++)
        {
            int row = part.startRow + local;
            for (int ch = 0; ch < TRACKER_CHANNELS; ch++)
            {
                if (ch > 0) out += '|';
                out += tracker->cells[row][ch].text;
            }
            out += '\n';
        }
    }
    return out;
}

inline void setTrackerCursorState(Tracker *self, int row, int tick, int ticksPerRow)
{
    if (!self) return;
    self->ticksPerRow = std::max(1, ticksPerRow);
    self->playRow = std::max(0, std::min(row, std::max(0, self->rowCount - 1)));
    self->playTick = std::max(0, std::min(tick, self->ticksPerRow - 1));
    if (self->followCursor && self->rowHeight > 0.0f)
    {
        const float target = (float)Tracker_VisualIndexForRow(self, self->playRow) * self->rowHeight;
        const float visibleRows = self->viewportHeight > 0.0f ? self->viewportHeight / self->rowHeight : 1.0f;
        self->scrollY = target - std::max(0.0f, visibleRows * 0.45f) * self->rowHeight;
        self->scrollY = std::max(0.0f, std::min(Tracker_MaxScroll(self), self->scrollY));
    }
}

inline void Tracker_RequestMusicSeekToCursor(Tracker *self)
{
    if (!self) return;
    self->musicSeekRequested = true;
    self->musicSeekRow = self->playRow;
    self->musicSeekTick = self->playTick;
}

inline void setTrackerPatternState(Tracker *self, int songIndex, const char *pattern, const char *displayName)
{
    if (!self) return;
    if (!pattern) pattern = Tracker_SongPattern(songIndex);
    self->songIndex = std::max(1, std::min(TRACKER_MAX_SONG_COUNT, songIndex));
    const char *name = displayName && displayName[0] ? displayName : Tracker_SongName(self->songIndex);
    std::snprintf(self->songDisplayName, sizeof(self->songDisplayName), "%s", name);
    self->rowCount = Tracker_ParseLeadingRowCount(pattern);
    self->songTickRate = 60;
    self->songSpeed = 6;
    self->ticksPerRow = self->songSpeed;
    self->songRowsPerBeat = 4;
    self->songScaleRoot = 0;
    self->songScaleMode = TRACKER_SONG_SCALE_CHROMATIC;
    self->songLfoEnabled = false;
    self->songLfoFrequency = 0;
    Tracker_ApplyBuiltinSongMetadata(self, self->songIndex);
    self->loopStart = 0;
    self->loopEnd = std::max(0, self->rowCount - 1);
    self->loopAnchor = 0;
    self->loopSelecting = false;
    self->loopMoving = false;
    self->loopEnabled = false;
    self->channelSelectionEnabled = false;
    self->channelSelecting = false;
    self->editSelectionEnabled = false;
    self->editSelectionValid = false;
    self->editSelecting = false;
    self->editSelectionAnchorRow = 0;
    self->editSelectionAnchorChannel = 0;
    self->editSelectionCurrentRow = 0;
    self->editSelectionCurrentChannel = 0;
    self->editSelectionStartRow = 0;
    self->editSelectionEndRow = 0;
    self->editSelectionStartChannel = 0;
    self->editSelectionEndChannel = 0;
    self->editSelectionAnchorPart = 0;
    self->editMoveGrabRowOffset = 0;
    self->editMoveGrabChannelOffset = 0;
    self->editMoveBaseStartRow = 0;
    self->editMoveBaseStartChannel = 0;
    self->editMovePointerStartRow = 0;
    self->editMovePointerStartChannel = 0;
    self->editSelectLocalX = 0.0f;
    self->editSelectViewportWidth = 0.0f;
    self->editSelectLocalY = 0.0f;
    self->editSelectViewportHeight = 0.0f;
    self->loopRangeDirty = true;
    self->playbackArrangementDirty = false;
    self->playRow = 0;
    self->playTick = 0;
    self->scrollY = 0.0f;
    self->scrollVelocity = 0.0f;
    Tracker_Clear(self);
    Tracker_ResetSinglePart(self);

    const char *p = Tracker_FindPatternRows(pattern);
    if (!p)
    {
        Tracker_NormalizeParts(self);
        return;
    }

    int row = 0;
    int currentPart = -1;
    while (row < self->rowCount && *p)
    {
        const char *lineStart = p;
        const char *lineEnd = p;
        while (*lineEnd && *lineEnd != '\n' && *lineEnd != '\r') lineEnd++;
        if (TrackerSongIO_IsBlankLine(lineStart, lineEnd))
        {
            p = lineEnd;
            while (*p == '\r') p++;
            if (*p == '\n') p++;
            continue;
        }
        if (TrackerSongIO_IsCollapsedDirectiveLine(lineStart, lineEnd))
        {
            if (currentPart >= 0 && currentPart < self->partCount)
                self->parts[currentPart].collapsed = true;
            p = lineEnd;
            while (*p == '\r') p++;
            if (*p == '\n') p++;
            continue;
        }
        bool isPartLine = lineEnd - lineStart >= 5 &&
            lineStart[4] == ' ' &&
            ((lineStart[0] == 'P' && lineStart[1] == 'A' && lineStart[2] == 'R' && lineStart[3] == 'T') ||
             (lineStart[0] == 'S' && lineStart[1] == 'K' && lineStart[2] == 'I' && lineStart[3] == 'P'));
        if (isPartLine)
        {
            bool partEnabled = lineStart[0] == 'P';
            if (currentPart >= 0)
                self->parts[currentPart].rowCount = row - self->parts[currentPart].startRow;
            if (self->partCount < TRACKER_MAX_PARTS)
            {
                currentPart++;
                if (currentPart == 0) self->partCount = 1;
                else self->partCount = currentPart + 1;
                self->parts[currentPart] = {};
                self->parts[currentPart].startRow = row;
                self->parts[currentPart].rowCount = 0;
                self->parts[currentPart].collapsed = false;
                self->parts[currentPart].enabled = partEnabled;
                char name[TRACKER_PART_NAME_CAPACITY] = {};
                int len = std::min((int)sizeof(name) - 1, (int)(lineEnd - (lineStart + 5)));
                std::memcpy(name, lineStart + 5, len);
                name[len] = '\0';
                Tracker_SetPartName(&self->parts[currentPart], name);
            }
            p = lineEnd;
            while (*p == '\r') p++;
            if (*p == '\n') p++;
            continue;
        }
        if (currentPart < 0)
        {
            currentPart = 0;
            self->partCount = 1;
            self->parts[0] = {};
            self->parts[0].startRow = 0;
            self->parts[0].rowCount = 0;
            self->parts[0].collapsed = false;
            self->parts[0].enabled = true;
            Tracker_SetPartName(&self->parts[0], "PART 1");
        }
        for (int ch = 0; ch < TRACKER_CHANNELS; ch++)
        {
            char cell[TRACKER_CELL_CHARS] = ".......";
            int i = 0;
            while (*p && *p != '|' && *p != '\n' && i < TRACKER_CELL_CHARS - 1)
                cell[i++] = *p++;
            cell[i] = '\0';
            if (i > 0)
                std::strncpy(self->cells[row][ch].text, cell, TRACKER_CELL_CHARS);
            if (*p == '|')
            {
                p++;
                continue;
            }
            if (*p == '\n' || *p == '\0')
                break;
        }
        while (*p && *p != '\n') p++;
        if (*p == '\n') p++;
        row++;
    }
    if (currentPart >= 0)
        self->parts[currentPart].rowCount = row - self->parts[currentPart].startRow;
    if (currentPart < 0)
        Tracker_ResetSinglePart(self);
    Tracker_NormalizeParts(self);
    Tracker_RebuildUsedInstruments(self);
    self->patternDirty = false;
    self->songLengthDirty = false;
    self->copyOnWriteRequested = false;
}

inline void setTrackerSongState(Tracker *self, int songIndex)
{
    if (!self) return;
    setTrackerPatternState(self, songIndex, Tracker_SongPattern(songIndex), Tracker_SongName(songIndex));
    Tracker_ClearInstrumentState(self, false);
    Tracker_LoadCustomInstrumentText(self, Tracker_SongInstruments(songIndex));
    self->patternDirty = false;
    self->copyOnWriteRequested = false;
}

inline void Tracker_LoadEmptyPatternState(Tracker *self)
{
    if (!self) return;
    setTrackerPatternState(self, TRACKER_USER_SONG_SLOT, "32\nPART 1\n", "Empty Song");
    Tracker_ResetSinglePart(self, "PART 1");
    Tracker_ClearInstrumentState(self, true);
    Tracker_EnsureDefaultInstrument(self, true);
    Tracker_PrepareClipboardForSong(self);
}

inline void Tracker_LoadSong(Tracker *self, int songIndex)
{
    setTrackerSongState(self, songIndex);
    self->copyOnWriteRequested = (songIndex != TRACKER_USER_SONG_SLOT);
    Tracker_PrepareClipboardForSong(self);
}

inline bool Tracker_ShouldReuseCurrentSongStateOnOpen(
    const Tracker *self,
    int currentSongIndex,
    const char *currentPattern,
    const char *currentDisplayName)
{
    if (!self)
        return false;
    if (self->songIndex != currentSongIndex || self->rowCount <= 0)
        return false;
    const char *displayName = (currentDisplayName && currentDisplayName[0]) ? currentDisplayName : Tracker_SongName(currentSongIndex);
    if (std::strcmp(self->songDisplayName, displayName) != 0)
        return false;
    std::string trackerPattern = Tracker_BuildPartPatternText(self);
    const char *pattern = currentPattern ? currentPattern : Tracker_SongPattern(currentSongIndex);
    return trackerPattern == pattern;
}

inline void Tracker_Init(Tracker *self)
{
    if (!self) return;
    initClaytonClick(&self->closeButton, "TrackerClose");
    initClaytonClick(&self->playButton, "TrackerPlay");
    initClaytonClick(&self->stopButton, "TrackerStop");
    initClaytonClick(&self->followButton, "TrackerFollow");
    initClaytonClick(&self->clearLoopButton, "TrackerClearLoop");
    initClaytonClick(&self->addRowButton, "TrackerAddRow");
    initClaytonClick(&self->removeRowButton, "TrackerRemoveRow");
    initClaytonClick(&self->addPartButton, "TrackerAddPart");
    for (int i = 0; i < TRACKER_MAX_PARTS; i++)
    {
        char id[48];
        (void)std::snprintf(id, sizeof(id), "TrackerPartToggle%02d", i);
        initClaytonClick(&self->partToggleButtons[i], id);
        (void)std::snprintf(id, sizeof(id), "TrackerPartEnable%02d", i);
        initClaytonClick(&self->partEnableButtons[i], id);
        (void)std::snprintf(id, sizeof(id), "TrackerPartRename%02d", i);
        initClaytonClick(&self->partRenameButtons[i], id);
        (void)std::snprintf(id, sizeof(id), "TrackerPartAddRow%02d", i);
        initClaytonClick(&self->partAddRowButtons[i], id);
        (void)std::snprintf(id, sizeof(id), "TrackerPartRemoveRow%02d", i);
        initClaytonClick(&self->partRemoveRowButtons[i], id);
        (void)std::snprintf(id, sizeof(id), "TrackerPartUp%02d", i);
        initClaytonClick(&self->partUpButtons[i], id);
        (void)std::snprintf(id, sizeof(id), "TrackerPartDown%02d", i);
        initClaytonClick(&self->partDownButtons[i], id);
        (void)std::snprintf(id, sizeof(id), "TrackerPartDelete%02d", i);
        initClaytonClick(&self->partDeleteButtons[i], id);
        (void)std::snprintf(id, sizeof(id), "TrackerPartSettings%02d", i);
        initClaytonClick(&self->partSettingsButtons[i], id);
    }
    initClaytonClick(&self->stickyPartToggleButton, "TrackerStickyPartToggle");
    initClaytonClick(&self->stickyPartEnableButton, "TrackerStickyPartEnable");
    initClaytonClick(&self->stickyPartRenameButton, "TrackerStickyPartRename");
    initClaytonClick(&self->stickyPartAddRowButton, "TrackerStickyPartAddRow");
    initClaytonClick(&self->stickyPartRemoveRowButton, "TrackerStickyPartRemoveRow");
    initClaytonClick(&self->stickyPartUpButton, "TrackerStickyPartUp");
    initClaytonClick(&self->stickyPartDownButton, "TrackerStickyPartDown");
    initClaytonClick(&self->stickyPartDeleteButton, "TrackerStickyPartDelete");
    initClaytonClick(&self->stickyPartSettingsButton, "TrackerStickyPartSettings");
    initClaytonClick(&self->saveSongButton, "TrackerSaveSong");
    initClaytonClick(&self->loadSongButton, "TrackerLoadSong");
    initClaytonClick(&self->songSaveCloseButton, "TrackerSongSaveClose");
    initClaytonClick(&self->songSaveRenameButton, "TrackerSongSaveRename");
    initClaytonClick(&self->songSaveConfirmButton, "TrackerSongSaveConfirm");
    initClaytonClick(&self->songSaveOverwriteConfirmButton, "TrackerSongSaveOverwriteConfirm");
    initClaytonClick(&self->songSaveOverwriteCancelButton, "TrackerSongSaveOverwriteCancel");
    initClaytonClick(&self->songDownloadButton, "TrackerSongDownload");
    initClaytonClick(&self->songLoadCloseButton, "TrackerSongLoadClose");
    initClaytonClick(&self->songLoadTabButtons[0], "TrackerSongLoadTabMySongs");
    initClaytonClick(&self->songLoadTabButtons[1], "TrackerSongLoadTabBuiltinSongs");
    initClaytonClick(&self->songLoadTabButtons[2], "TrackerSongLoadTabBuiltinSfx");
    initClaytonClick(&self->songLoadConfirmButton, "TrackerSongLoadConfirm");
    initClaytonClick(&self->songUploadButton, "TrackerSongUpload");
    initClaytonClick(&self->songDeleteConfirmButton, "TrackerSongDeleteConfirm");
    initClaytonClick(&self->songDeleteCancelButton, "TrackerSongDeleteCancel");
    for (int i = 0; i < TRACKER_SAVED_SONG_LIST_CAPACITY; i++)
    {
        char id[48];
        (void)std::snprintf(id, sizeof(id), "TrackerSongMyRow%02d", i);
        initClaytonClick(&self->songMySongRowClicks[i], id);
        (void)std::snprintf(id, sizeof(id), "TrackerSongMyDelete%02d", i);
        initClaytonClick(&self->songMySongDeleteButtons[i], id);
        (void)std::snprintf(id, sizeof(id), "TrackerSongSfxRow%02d", i);
        initClaytonClick(&self->songBuiltinSfxRowClicks[i], id);
    }
    for (int i = 0; i < TRACKER_MAX_SONG_COUNT; i++)
    {
        char id[48];
        (void)std::snprintf(id, sizeof(id), "TrackerSongBuiltinRow%02d", i);
        initClaytonClick(&self->songBuiltinSongRowClicks[i], id);
    }
    initClaytonClick(&self->loadErrorOkButton, "TrackerLoadErrorOk");
    initClaytonClick(&self->copyButton, "TrackerCopy");
    initClaytonClick(&self->cutButton, "TrackerCut");
    initClaytonClick(&self->pasteButton, "TrackerPaste");
    initClaytonClick(&self->editSelectionButton, "TrackerEditSelection");
    initClaytonClick(&self->instrumentsButton, "TrackerInstruments");
    initClaytonClick(&self->songSettingsButton, "TrackerSongSettings");
    initClaytonClick(&self->oscilloscopeButton, "TrackerOscilloscope");
    for (int i = 0; i < TRACKER_MAX_SONG_COUNT; i++)
    {
        char id[32];
        (void)std::snprintf(id, sizeof(id), "TrackerSong%d", i + 1);
        initClaytonClick(&self->songButtons[i], id);
    }
    initClaytonClick(&self->editorCloseButton, "TrackerEditorClose");
    initClaytonClick(&self->editorNoteTabButton, "TrackerEditorNoteTab");
    initClaytonClick(&self->editorEffectsTabButton, "TrackerEditorEffectsTab");
    initClaytonClick(&self->editorCancelButton, "TrackerEditorCancel");
    initClaytonClick(&self->instrumentExplicitButton, "TrackerInstrumentExplicit");
    initClaytonClick(&self->volumeExplicitButton, "TrackerVolumeExplicit");
    initClaytonClick(&self->effectPrevButton, "TrackerEffectPrev");
    initClaytonClick(&self->effectNextButton, "TrackerEffectNext");
    initClaytonClick(&self->instrumentPrevButton, "TrackerInstrumentPrev");
    initClaytonClick(&self->instrumentNextButton, "TrackerInstrumentNext");
    initClaytonClick(&self->instrumentNameButton, "TrackerInstrumentNameClick");
    initClaytonClick(&self->instrumentEditorCloseButton, "TrackerInstrumentEditorClose");
    initClaytonClick(&self->instrumentManagementCloneButton, "TrackerInstrumentMgmtClone");
    initClaytonClick(&self->instrumentManagementRenameButton, "TrackerInstrumentMgmtRename");
    initClaytonClick(&self->instrumentManagementDeleteButton, "TrackerInstrumentMgmtDelete");
    initClaytonClick(&self->instrumentManagementNewButton, "TrackerInstrumentMgmtNew");
    initClaytonClick(&self->instrumentManagementEditButton, "TrackerInstrumentMgmtEdit");
    for (int i = 0; i < 256; i++)
    {
        char id[40];
        (void)std::snprintf(id, sizeof(id), "TrackerInstrumentRow%02X", i);
        initClaytonClick(&self->instrumentRowClicks[i], id);
    }
    initClaytonClick(&self->instrumentColorButton, "TrackerInstrumentColorButton");
    initClaytonClick(&self->instrumentColorCloseButton, "TrackerInstrumentColorClose");
    initClaytonClick(&self->instrumentsCloseButton, "TrackerInstrumentsClose");
    initClaytonClick(&self->songSettingsCloseButton, "TrackerSongSettingsClose");
    initClaytonClick(&self->songLoadEmptyButton, "TrackerSongLoadEmpty");
    initClaytonClick(&self->partEditorCloseButton, "TrackerPartEditorClose");
    initClaytonClick(&self->partEditorNameButton, "TrackerPartEditorName");
    initClaytonClick(&self->partEditorEnableButton, "TrackerPartEditorEnable");
    initClaytonClick(&self->partEditorRowsMinusButton, "TrackerPartEditorRowsMinus");
    initClaytonClick(&self->partEditorRowsPlusButton, "TrackerPartEditorRowsPlus");
    initClaytonClick(&self->partEditorCloneButton, "TrackerPartEditorClone");
    initClaytonClick(&self->partEditorDeleteButton, "TrackerPartEditorDelete");
    initClaytonClick(&self->songNameButton, "TrackerSongNameButton");
    initClaytonClick(&self->songScaleRootPrevButton, "TrackerSongScaleRootPrev");
    initClaytonClick(&self->songScaleRootNextButton, "TrackerSongScaleRootNext");
    initClaytonClick(&self->songScalePrevButton, "TrackerSongScalePrev");
    initClaytonClick(&self->songScaleNextButton, "TrackerSongScaleNext");
    initClaytonClick(&self->songLfoButton, "TrackerSongLfoButton");
    for (int i = 0; i < 256; i++)
    {
        char id[40];
        (void)std::snprintf(id, sizeof(id), "TrackerInstrumentUp%02X", i);
        initClaytonClick(&self->instrumentUpButtons[i], id);
        (void)std::snprintf(id, sizeof(id), "TrackerInstrumentDown%02X", i);
        initClaytonClick(&self->instrumentDownButtons[i], id);
    }
    initClaytonClick(&self->instrumentPatchTabButton, "TrackerInstrumentPatchTab");
    initClaytonClick(&self->instrumentEffectsTabButton, "TrackerInstrumentEffectsTab");
    initClaytonClick(&self->instrumentAlgoPrevButton, "TrackerInstrumentAlgoPrev");
    initClaytonClick(&self->instrumentAlgoNextButton, "TrackerInstrumentAlgoNext");
    initClaytonClick(&self->macroTargetPrevButton, "TrackerMacroTargetPrev");
    initClaytonClick(&self->macroTargetNextButton, "TrackerMacroTargetNext");
    initClaytonClick(&self->macroEnableButton, "TrackerMacroEnable");
    initClaytonClick(&self->macroScrollPrevButton, "TrackerMacroScrollPrev");
    initClaytonClick(&self->macroScrollNextButton, "TrackerMacroScrollNext");
    initClaytonClick(&self->macroValueScrollUpButton, "TrackerMacroValueScrollUp");
    initClaytonClick(&self->macroValueScrollDownButton, "TrackerMacroValueScrollDown");
    initClaytonClick(&self->macroStepPrevButton, "TrackerMacroStepPrev");
    initClaytonClick(&self->macroStepNextButton, "TrackerMacroStepNext");
    initClaytonClick(&self->macroLoopButton, "TrackerMacroLoopToggle");
    initClaytonClick(&self->macroReleaseButton, "TrackerMacroReleaseToggle");
    for (int i = 0; i < 4; i++)
    {
        char id[32];
        (void)std::snprintf(id, sizeof(id), "TrackerOperator%d", i + 1);
        initClaytonClick(&self->operatorButtons[i], id);
    }
    initClaytonClick(&self->operatorEditorPrevButton, "TrackerOperatorEditorPrev");
    initClaytonClick(&self->operatorEditorNextButton, "TrackerOperatorEditorNext");
    initClaytonClick(&self->operatorEditorCloseButton, "TrackerOperatorEditorClose");
    initClaytonClick(&self->operatorSsgPrevButton, "TrackerOperatorSsgPrev");
    initClaytonClick(&self->operatorSsgNextButton, "TrackerOperatorSsgNext");
    initClaytonClick(&self->operatorAmButton, "TrackerOperatorAm");
    setTrackerSongState(self, 1);
}

inline void initTracker(Tracker *self)
{
    Tracker_Init(self);
}

inline void Tracker_Open(Tracker *self)
{
    if (!self) return;
    self->active = true;
    self->clipboardCutCooldown = 0.0f;
    self->clipboardBannerFlashTime = 0.0f;
    self->clipboardBannerKind = TRACKER_CLIPBOARD_BANNER_NONE;
    self->clipboardBannerUsesEditSelection = false;
    self->clipboardBannerText[0] = '\0';
    Tracker_CancelCellMovePending(self);
    self->editorOpen = false;
    self->instrumentEditorOpen = false;
    self->instrumentColorWindowOpen = false;
    self->instrumentsWindowOpen = false;
    self->songSettingsWindowOpen = false;
    self->songSaveWindowOpen = false;
    self->songSaveOverwriteConfirmWindowOpen = false;
    self->songLoadWindowOpen = false;
    self->songDeleteConfirmWindowOpen = false;
    self->songLoadEmptyRequested = false;
    self->partEditorOpen = false;
    self->songLoadErrorWindowOpen = false;
    self->pendingPartNameKeypadOpen = false;
    self->pendingPartNameKeypadActive = false;
    self->partEditorOpen = false;
    self->partEditorWindowRequested = false;
    self->partEditorPart = -1;
    self->operatorEditorOpen = false;
    self->instrumentEditorTab = 0;
    self->dragging = false;
    self->cellMoving = false;
    self->scrollbarDragging = false;
    self->loopSelecting = false;
    self->loopMoving = false;
    self->editSelecting = false;
    self->editMoving = false;
    self->channelSelecting = false;
    self->macroDrawing = false;
    self->macroRangeSelecting = false;
    self->instrumentsDragging = false;
    self->instrumentsScrollbarDragging = false;
}

inline void Tracker_Close(Tracker *self)
{
    if (!self) return;
    self->active = false;
    self->playing = false;
    self->clipboardCutCooldown = 0.0f;
    self->clipboardBannerFlashTime = 0.0f;
    self->clipboardBannerKind = TRACKER_CLIPBOARD_BANNER_NONE;
    self->clipboardBannerUsesEditSelection = false;
    self->clipboardBannerText[0] = '\0';
    self->musicSeekRequested = false;
    Tracker_StopGridNoteAudition(self);
    if (self->virtualKeyPointerDown)
        self->previewHeldNotesStopAllRequested = true;
    self->virtualKeyPointerDown = false;
    self->virtualKeyRootFingerActive = false;
    self->virtualKeyRootFingerId = 0;
    Tracker_ClearFurnaceKeyboardState(self);
    Tracker_CancelCellMovePending(self);
    self->editorOpen = false;
    self->editorWindowRequested = false;
    self->instrumentEditorOpen = false;
    self->instrumentEditorWindowRequested = false;
    self->instrumentEditorOpenedFromCellEditor = false;
    self->instrumentEditorOpenedFromInstrumentsWindow = false;
    self->instrumentColorWindowOpen = false;
    self->instrumentColorWindowRequested = false;
    self->instrumentsWindowOpen = false;
    self->instrumentsWindowRequested = false;
    self->songSettingsWindowOpen = false;
    self->songSettingsWindowRequested = false;
    self->songSaveWindowOpen = false;
    self->songSaveWindowRequested = false;
    self->songSaveOverwriteConfirmWindowOpen = false;
    self->songSaveOverwriteConfirmWindowRequested = false;
    self->songSaveOverwriteConfirmed = false;
    self->songLoadWindowOpen = false;
    self->songLoadWindowRequested = false;
    self->songDeleteConfirmWindowOpen = false;
    self->songDeleteConfirmWindowRequested = false;
    self->songDeleteRequested = false;
    self->songDeleteIndex = -1;
    self->songDeleteName[0] = '\0';
    self->songLoadEmptyRequested = false;
    self->partEditorOpen = false;
    self->partEditorWindowRequested = false;
    self->partEditorPart = -1;
    self->songLoadErrorWindowOpen = false;
    self->songLoadErrorWindowRequested = false;
    self->songLoadErrorText[0] = '\0';
    self->pendingPartAction = 0;
    self->pendingPartNameKeypadOpen = false;
    self->pendingPartNameKeypadActive = false;
    self->partEditorOpen = false;
    self->partEditorWindowRequested = false;
    self->partEditorPart = -1;
    self->operatorEditorOpen = false;
    self->operatorEditorWindowRequested = false;
    self->dragging = false;
    self->partProgressScrubPending = false;
    self->partProgressScrubMovedY = false;
    self->partProgressScrubPart = -1;
    self->scrollbarDragging = false;
    self->loopSelecting = false;
    self->loopMoving = false;
    self->editSelecting = false;
    self->editMoving = false;
    self->channelSelecting = false;
    self->macroDrawing = false;
    self->macroRangeSelecting = false;
    self->instrumentsDragging = false;
    self->instrumentsScrollbarDragging = false;
}

inline float Tracker_MaxScroll(const Tracker *self)
{
    if (!self) return 0.0f;
    return std::max(0.0f, Tracker_ContentHeight(self) - self->viewportHeight);
}

inline float Tracker_SnappedScrollY(const Tracker *self, float scrollY)
{
    if (!self || self->rowHeight <= 0.0f) return 0.0f;
    float maxScroll = Tracker_MaxScroll(self);
    float snapped = std::round(scrollY / self->rowHeight) * self->rowHeight;
    snapped = std::max(0.0f, std::min(maxScroll, snapped));
    float clampedScroll = std::max(0.0f, std::min(maxScroll, scrollY));
    if (std::fabs(maxScroll - clampedScroll) < std::fabs(snapped - clampedScroll))
        snapped = maxScroll;
    return snapped;
}

inline float Tracker_InstrumentsMaxScroll(const Tracker *self)
{
    if (!self) return 0.0f;
    const float rowH = self->instrumentsRowHeight > 1.0f ? self->instrumentsRowHeight : 54.0f;
    float contentHeight = self->instrumentsContentHeight > 1.0f
        ? self->instrumentsContentHeight
        : (float)std::max(0, self->availableInstrumentCount) * rowH;
    return std::max(0.0f, contentHeight - self->instrumentsViewportHeight);
}

inline float Tracker_SnappedInstrumentsScrollY(const Tracker *self, float scrollY)
{
    if (!self) return 0.0f;
    const float rowH = self->instrumentsRowHeight > 1.0f ? self->instrumentsRowHeight : 54.0f;
    float maxScroll = Tracker_InstrumentsMaxScroll(self);
    float snapped = std::round(scrollY / rowH) * rowH;
    snapped = std::max(0.0f, std::min(maxScroll, snapped));
    float clampedScroll = std::max(0.0f, std::min(maxScroll, scrollY));
    if (std::fabs(maxScroll - clampedScroll) < std::fabs(snapped - clampedScroll))
        snapped = maxScroll;
    return snapped;
}

inline void Tracker_SnapToGrid(Tracker *self)
{
    if (!self || self->rowHeight <= 0.0f) return;
    self->scrollY = Tracker_SnappedScrollY(self, self->scrollY);
}

inline void Tracker_SnapInstruments(Tracker *self)
{
    if (!self) return;
    self->instrumentsScrollY = Tracker_SnappedInstrumentsScrollY(self, self->instrumentsScrollY);
}

inline int Tracker_RowAtViewportY(const Tracker *self, float localY)
{
    if (!self || self->rowHeight <= 0.0f) return 0;
    return Tracker_FirstEditableRowForVisualY(self, localY);
}

enum TrackerSelectionSource
{
    TRACKER_SELECTION_NONE = 0,
    TRACKER_SELECTION_PLAY = 1,
    TRACKER_SELECTION_EDIT = 2,
};

inline bool Tracker_HasPlaySelection(const Tracker *self)
{
    return self && self->loopEnabled && self->rowCount > 0 &&
        self->loopEnd >= self->loopStart &&
        self->loopStart >= 0 &&
        self->loopEnd < self->rowCount;
}

inline bool Tracker_HasEditSelection(const Tracker *self)
{
    return self && self->editSelectionEnabled && self->editSelectionValid &&
        self->editSelectionEndRow >= self->editSelectionStartRow &&
        self->editSelectionEndChannel >= self->editSelectionStartChannel;
}

inline TrackerSelectionSource Tracker_ActiveSelectionSource(const Tracker *self)
{
    if (Tracker_HasEditSelection(self)) return TRACKER_SELECTION_EDIT;
    if (Tracker_HasPlaySelection(self)) return TRACKER_SELECTION_PLAY;
    return TRACKER_SELECTION_NONE;
}

inline bool Tracker_SelectionUsesEdit(const Tracker *self)
{
    return Tracker_ActiveSelectionSource(self) == TRACKER_SELECTION_EDIT;
}

inline bool Tracker_EditSelectionContains(const Tracker *self, int row, int channel)
{
    return Tracker_HasEditSelection(self) &&
        row >= self->editSelectionStartRow && row <= self->editSelectionEndRow &&
        channel >= self->editSelectionStartChannel && channel <= self->editSelectionEndChannel;
}

inline int Tracker_ChannelAtGridX(float localX, float gridWidth)
{
    if (gridWidth <= 0.0f) return 0;
    float unit = gridWidth / 13.0f;
    if (localX < unit) return 0;
    int channel = (int)std::floor((localX - unit) / (unit * 2.0f));
    return std::max(0, std::min(TRACKER_CHANNELS - 1, channel));
}

inline void Tracker_ClampEditSelection(Tracker *self)
{
    if (!self) return;
    if (!self->editSelectionValid || self->partCount <= 0 || self->rowCount <= 0)
    {
        if (!self->editSelectionEnabled)
            self->editSelecting = false;
        self->editMoving = false;
        return;
    }
    int partIndex = std::max(0, std::min(std::max(0, self->partCount - 1), self->editSelectionAnchorPart));
    int partStart = self->parts[partIndex].startRow;
    int partEnd = std::max(partStart, Tracker_PartEndRow(self, partIndex) - 1);
    self->editSelectionAnchorRow = std::max(partStart, std::min(partEnd, self->editSelectionAnchorRow));
    self->editSelectionCurrentRow = std::max(partStart, std::min(partEnd, self->editSelectionCurrentRow));
    self->editSelectionStartRow = std::max(partStart, std::min(partEnd, self->editSelectionStartRow));
    self->editSelectionEndRow = std::max(partStart, std::min(partEnd, self->editSelectionEndRow));
    self->editSelectionAnchorChannel = std::max(0, std::min(TRACKER_CHANNELS - 1, self->editSelectionAnchorChannel));
    self->editSelectionCurrentChannel = std::max(0, std::min(TRACKER_CHANNELS - 1, self->editSelectionCurrentChannel));
    self->editSelectionStartChannel = std::max(0, std::min(TRACKER_CHANNELS - 1, self->editSelectionStartChannel));
    self->editSelectionEndChannel = std::max(0, std::min(TRACKER_CHANNELS - 1, self->editSelectionEndChannel));
}

inline void Tracker_ClearEditSelection(Tracker *self)
{
    if (!self) return;
    self->editSelectionValid = false;
    self->editSelecting = false;
    self->editMoving = false;
}

inline void Tracker_SetEditSelection(Tracker *self, int anchorRow, int row, int anchorChannel, int channel)
{
    if (!self || self->rowCount <= 0) return;
    int partIndex = Tracker_PartIndexForRow(self, anchorRow);
    partIndex = std::max(0, std::min(std::max(0, self->partCount - 1), partIndex));
    int partStart = self->parts[partIndex].startRow;
    int partEnd = std::max(partStart, Tracker_PartEndRow(self, partIndex) - 1);
    anchorRow = std::max(partStart, std::min(partEnd, anchorRow));
    row = std::max(partStart, std::min(partEnd, row));
    anchorChannel = std::max(0, std::min(TRACKER_CHANNELS - 1, anchorChannel));
    channel = std::max(0, std::min(TRACKER_CHANNELS - 1, channel));

    self->editSelectionAnchorPart = partIndex;
    self->editSelectionAnchorRow = anchorRow;
    self->editSelectionAnchorChannel = anchorChannel;
    self->editSelectionCurrentRow = row;
    self->editSelectionCurrentChannel = channel;
    self->editSelectionStartRow = std::min(anchorRow, row);
    self->editSelectionEndRow = std::max(anchorRow, row);
    self->editSelectionStartChannel = std::min(anchorChannel, channel);
    self->editSelectionEndChannel = std::max(anchorChannel, channel);
    self->editSelectionValid = true;
}

inline void Tracker_MoveEditSelectionToGrabbedCell(Tracker *self, int grabbedRow, int grabbedChannel)
{
    if (!self || !Tracker_HasEditSelection(self) || self->partCount <= 0 || self->rowCount <= 0)
        return;
    int partIndex = std::max(0, std::min(std::max(0, self->partCount - 1), self->editSelectionAnchorPart));
    int partStart = self->parts[partIndex].startRow;
    int partEnd = std::max(partStart, Tracker_PartEndRow(self, partIndex) - 1);
    int height = std::max(1, self->editSelectionEndRow - self->editSelectionStartRow + 1);
    int width = std::max(1, self->editSelectionEndChannel - self->editSelectionStartChannel + 1);
    int maxStartRow = std::max(partStart, partEnd - height + 1);
    int maxStartChannel = std::max(0, TRACKER_CHANNELS - width);
    grabbedRow = std::max(partStart, std::min(partEnd, grabbedRow));
    grabbedChannel = std::max(0, std::min(TRACKER_CHANNELS - 1, grabbedChannel));
    int startRow = grabbedRow - std::max(0, std::min(height - 1, self->editMoveGrabRowOffset));
    int startChannel = grabbedChannel - std::max(0, std::min(width - 1, self->editMoveGrabChannelOffset));
    startRow = std::max(partStart, std::min(maxStartRow, startRow));
    startChannel = std::max(0, std::min(maxStartChannel, startChannel));
    Tracker_SetEditSelection(self, startRow, startRow + height - 1, startChannel, startChannel + width - 1);
    self->editSelectionAnchorPart = partIndex;
}

inline void Tracker_MoveEditSelectionByPointer(Tracker *self, int pointerRow, int pointerChannel)
{
    if (!self || !Tracker_HasEditSelection(self) || self->partCount <= 0 || self->rowCount <= 0)
        return;
    int rowDelta = pointerRow - self->editMovePointerStartRow;
    int channelDelta = pointerChannel - self->editMovePointerStartChannel;
    int width = std::max(1, self->editSelectionEndChannel - self->editSelectionStartChannel + 1);
    int height = std::max(1, self->editSelectionEndRow - self->editSelectionStartRow + 1);
    int partIndex = std::max(0, std::min(std::max(0, self->partCount - 1), self->editSelectionAnchorPart));
    int partStart = self->parts[partIndex].startRow;
    int partEnd = std::max(partStart, Tracker_PartEndRow(self, partIndex) - 1);
    int maxStartRow = std::max(partStart, partEnd - height + 1);
    int maxStartChannel = std::max(0, TRACKER_CHANNELS - width);
    int startRow = std::max(partStart, std::min(maxStartRow, self->editMoveBaseStartRow + rowDelta));
    int startChannel = std::max(0, std::min(maxStartChannel, self->editMoveBaseStartChannel + channelDelta));
    Tracker_SetEditSelection(self, startRow, startRow + height - 1, startChannel, startChannel + width - 1);
    self->editSelectionAnchorPart = partIndex;
}

inline void Tracker_SetLoopRange(Tracker *self, int a, int b)
{
    if (!self || self->rowCount <= 0) return;
    int anchorPart = Tracker_PartIndexForRow(self, a);
    bool implicitSinglePart = self->partCount == 1 && self->parts[0].rowCount <= 0;
    int partStart = implicitSinglePart ? 0 : self->parts[anchorPart].startRow;
    int partEnd = implicitSinglePart ? self->rowCount - 1 : std::max(partStart, partStart + self->parts[anchorPart].rowCount - 1);
    a = std::max(partStart, std::min(partEnd, a));
    b = std::max(partStart, std::min(partEnd, b));
    int start = std::max(partStart, std::min(a, b));
    int end = std::min(partEnd, std::max(a, b));
    if (!self->loopEnabled || start != self->loopStart || end != self->loopEnd)
    {
        self->loopEnabled = true;
        self->loopStart = start;
        self->loopEnd = end;
        self->loopRangeDirty = true;
    }
}

inline void Tracker_ClearLoopRange(Tracker *self)
{
    if (!self || self->rowCount <= 0) return;
    self->loopSelecting = false;
    self->loopMoving = false;
    if (self->loopEnabled)
    {
        self->loopEnabled = false;
        self->loopRangeDirty = true;
    }
}

inline int Tracker_SelectedRowCount(const Tracker *self)
{
    if (Tracker_HasEditSelection(self))
        return std::max(0, self->editSelectionEndRow - self->editSelectionStartRow + 1);
    return self && self->loopEnabled ? std::max(0, self->loopEnd - self->loopStart + 1) : 0;
}

inline int Tracker_SelectedRowStart(const Tracker *self)
{
    if (Tracker_HasEditSelection(self))
        return self->editSelectionStartRow;
    return self && self->loopEnabled ? self->loopStart : 0;
}

inline int Tracker_SelectedChannelStart(const Tracker *self)
{
    if (Tracker_HasEditSelection(self))
        return self->editSelectionStartChannel;
    return self && self->channelSelectionEnabled ? self->channelStart : 0;
}

inline int Tracker_SelectedChannelCount(const Tracker *self)
{
    if (!self) return 0;
    if (Tracker_HasEditSelection(self))
        return std::max(0, self->editSelectionEndChannel - self->editSelectionStartChannel + 1);
    if (!self->channelSelectionEnabled) return TRACKER_CHANNELS;
    return std::max(0, self->channelEnd - self->channelStart + 1);
}

inline bool Tracker_HasSelection(const Tracker *self)
{
    return Tracker_ActiveSelectionSource(self) != TRACKER_SELECTION_NONE;
}

inline void Tracker_SetClipboardBanner(Tracker *self, const char *text, bool usesEditSelection, bool error)
{
    if (!self) return;
    if (!text) text = "";
    std::snprintf(self->clipboardBannerText, sizeof(self->clipboardBannerText), "%s", text);
    self->clipboardBannerUsesEditSelection = usesEditSelection;
    self->clipboardBannerKind = error ? TRACKER_CLIPBOARD_BANNER_ERROR : TRACKER_CLIPBOARD_BANNER_SUCCESS;
    self->clipboardBannerFlashTime = 1.8f;
}

inline void Tracker_SetChannelSelection(Tracker *self, int a, int b)
{
    if (!self) return;
    int start = std::max(0, std::min(a, b));
    int end = std::min(TRACKER_CHANNELS - 1, std::max(a, b));
    self->channelSelectionEnabled = true;
    self->channelStart = start;
    self->channelEnd = end;
}

inline bool Tracker_CanPaste(const Tracker *self)
{
    return self && self->clipboard.valid && Tracker_HasSelection(self) &&
           !self->clipboard.instrumentOverflow &&
           self->clipboard.rows == Tracker_SelectedRowCount(self) &&
           self->clipboard.channels == Tracker_SelectedChannelCount(self);
}

inline void Tracker_CopySelection(Tracker *self)
{
    if (!self) return;
    if (!Tracker_HasSelection(self))
    {
        Tracker_SetClipboardBanner(self, "CANNOT COPY YET", Tracker_SelectionUsesEdit(self), true);
        return;
    }
    if (self->clipboardCutCooldown > 0.0f)
    {
        Tracker_SetClipboardBanner(self, "ACCIDENTAL COPY PREVENTED", Tracker_SelectionUsesEdit(self), true);
        return;
    }
    bool usesEditSelection = Tracker_SelectionUsesEdit(self);
    int rows = Tracker_SelectedRowCount(self);
    int channels = Tracker_SelectedChannelCount(self);
    int rowStart = Tracker_SelectedRowStart(self);
    int chStart = Tracker_SelectedChannelStart(self);
    self->clipboard.valid = true;
    self->clipboard.rows = rows;
    self->clipboard.channels = channels;
    TrackerClipboard_ClearInstruments(&self->clipboard);
    for (int r = 0; r < rows; r++)
    {
        for (int ch = 0; ch < channels; ch++)
        {
            self->clipboard.cells[r][ch] = self->cells[rowStart + r][chStart + ch];
            int inst = Tracker_ParseCellInstrument(self->clipboard.cells[r][ch].text);
            if (inst >= 0 && !TrackerClipboard_AddInstrument(&self->clipboard, self, inst))
                self->clipboard.instrumentOverflow = true;
        }
    }
    Tracker_PrepareClipboardForSong(self);
    char text[64];
    std::snprintf(text, sizeof(text), "[%dx%d] COPIED", rows, channels);
    Tracker_SetClipboardBanner(self, text, usesEditSelection, false);
}

inline void Tracker_PasteSelection(Tracker *self)
{
    if (!self) return;
    bool usesEditSelection = Tracker_SelectionUsesEdit(self);
    if (!Tracker_CanPaste(self))
    {
        Tracker_SetClipboardBanner(self, "CANNOT PASTE", usesEditSelection, true);
        return;
    }
    if (!Tracker_PrepareClipboardForSong(self))
    {
        Tracker_SetClipboardBanner(self, "CANNOT PASTE", usesEditSelection, true);
        return;
    }
    for (int entry = 0; entry < self->clipboard.instrumentCount; entry++)
    {
        if (!self->clipboard.instrumentPasteMapped[entry])
            return;
        Tracker_ImportClipboardInstrument(self, &self->clipboard, entry, self->clipboard.instrumentPasteIds[entry]);
    }
    int rows = Tracker_SelectedRowCount(self);
    int channels = Tracker_SelectedChannelCount(self);
    int rowStart = Tracker_SelectedRowStart(self);
    int chStart = Tracker_SelectedChannelStart(self);
    for (int r = 0; r < rows; r++)
    {
        for (int ch = 0; ch < channels; ch++)
        {
            TrackerCell pasted = self->clipboard.cells[r][ch];
            int inst = Tracker_ParseCellInstrument(pasted.text);
            int entry = TrackerClipboard_InstrumentEntry(&self->clipboard, inst);
            if (entry >= 0 && self->clipboard.instrumentPasteMapped[entry])
                Tracker_WriteHexByte(pasted.text + 3, self->clipboard.instrumentPasteIds[entry]);
            self->cells[rowStart + r][chStart + ch] = pasted;
        }
    }
    self->patternDirty = true;
    self->copyOnWriteRequested = true;
    Tracker_RebuildUsedInstruments(self);
    Tracker_FlashCellRange(self, rowStart, rowStart + rows - 1, chStart, chStart + channels - 1, TRACKER_CHANGE_FLASH_ADD);
    char text[64];
    std::snprintf(text, sizeof(text), "[%dx%d] PASTED", rows, channels);
    Tracker_SetClipboardBanner(self, text, usesEditSelection, false);
}

inline void Tracker_CutSelection(Tracker *self)
{
    if (!self) return;
    bool usesEditSelection = Tracker_SelectionUsesEdit(self);
    if (!Tracker_HasSelection(self))
    {
        Tracker_SetClipboardBanner(self, "CANNOT CUT YET", usesEditSelection, true);
        return;
    }
    if (self->clipboardCutCooldown > 0.0f)
    {
        Tracker_SetClipboardBanner(self, "ACCIDENTAL CUT PREVENTED", usesEditSelection, true);
        return;
    }
    Tracker_CopySelection(self);
    int rows = Tracker_SelectedRowCount(self);
    int channels = Tracker_SelectedChannelCount(self);
    int rowStart = Tracker_SelectedRowStart(self);
    int chStart = Tracker_SelectedChannelStart(self);
    for (int r = 0; r < rows; r++)
        for (int ch = 0; ch < channels; ch++)
            Tracker_ClearCell(&self->cells[rowStart + r][chStart + ch]);
    self->patternDirty = true;
    self->copyOnWriteRequested = true;
    Tracker_RebuildUsedInstruments(self);
    char text[64];
    std::snprintf(text, sizeof(text), "[%dx%d] CUT", rows, channels);
    Tracker_SetClipboardBanner(self, text, usesEditSelection, false);
    self->clipboardCutCooldown = TRACKER_CLIPBOARD_CUT_COOLDOWN_S;
}

inline bool Tracker_CellMoveCanStart(const Tracker *self, int row, int channel)
{
    if (!self || row < 0 || row >= self->rowCount || channel < 0 || channel >= TRACKER_CHANNELS)
        return false;
    return !Tracker_CellIsEmpty(self->cells[row][channel].text);
}

inline void Tracker_CancelCellMovePending(Tracker *self)
{
    if (!self) return;
    self->cellMovePending = false;
    self->cellMovePendingSuppressed = false;
    self->cellMovePendingRow = -1;
    self->cellMovePendingChannel = -1;
    self->cellMovePendingStartX = 0.0f;
    self->cellMovePendingStartY = 0.0f;
    self->cellMovePendingCurrentX = 0.0f;
    self->cellMovePendingCurrentY = 0.0f;
    self->cellMovePendingStartedAtMs = 0;
}

inline void Tracker_SuppressCellMovePending(Tracker *self)
{
    if (!self) return;
    self->cellMovePending = false;
    self->cellMovePendingSuppressed = true;
    self->cellMovePendingRow = -1;
    self->cellMovePendingChannel = -1;
    self->cellMovePendingStartX = 0.0f;
    self->cellMovePendingStartY = 0.0f;
    self->cellMovePendingCurrentX = 0.0f;
    self->cellMovePendingCurrentY = 0.0f;
    self->cellMovePendingStartedAtMs = 0;
}

inline void Tracker_BeginCellMovePending(Tracker *self, int row, int channel, float px, float py, uint64_t nowMs)
{
    if (!Tracker_CellMoveCanStart(self, row, channel)) return;
    self->cellMovePending = true;
    self->cellMovePendingSuppressed = false;
    self->cellMovePendingRow = row;
    self->cellMovePendingChannel = channel;
    self->cellMovePendingStartX = px;
    self->cellMovePendingStartY = py;
    self->cellMovePendingCurrentX = px;
    self->cellMovePendingCurrentY = py;
    self->cellMovePendingStartedAtMs = nowMs;
    self->followCursor = false;
    self->dragging = false;
    self->dragMoved = false;
    self->dragStartY = py;
    self->dragLastY = py;
    self->dragStartScrollY = self->scrollY;
    self->scrollVelocity = 0.0f;
}

inline void Tracker_BeginScrollDragFromPendingCellMove(Tracker *self, float startY)
{
    if (!self) return;
    self->followCursor = false;
    self->dragging = true;
    self->dragMoved = true;
    self->dragStartY = startY;
    self->dragLastY = startY;
    self->dragStartScrollY = self->scrollY;
    self->scrollVelocity = 0.0f;
}

inline void Tracker_UpdateCellMovePendingPointer(Tracker *self, float px, float py)
{
    if (!self || !self->cellMovePending || self->cellMoving) return;
    self->cellMovePendingCurrentX = px;
    self->cellMovePendingCurrentY = py;
}

inline bool Tracker_IsTapReleaseForPendingCellMove(const Tracker *self, int row, int channel)
{
    if (!self || !self->cellMovePending || self->cellMovePendingSuppressed || self->cellMoving)
        return false;
    if (row != self->cellMovePendingRow || channel != self->cellMovePendingChannel)
        return false;
    const float moveThreshold = 8.0f;
    return std::fabs(self->cellMovePendingCurrentX - self->cellMovePendingStartX) <= moveThreshold &&
        std::fabs(self->cellMovePendingCurrentY - self->cellMovePendingStartY) <= moveThreshold;
}

inline bool Tracker_TryArmCellMovePending(Tracker *self, uint64_t nowMs)
{
    if (!self || !self->cellMovePending || self->cellMovePendingSuppressed || self->cellMoving)
        return false;
    if (nowMs < self->cellMovePendingStartedAtMs + TRACKER_CELL_MOVE_HOLD_MS)
        return false;
    const float moveThreshold = 8.0f;
    if (std::fabs(self->cellMovePendingCurrentX - self->cellMovePendingStartX) > moveThreshold ||
        std::fabs(self->cellMovePendingCurrentY - self->cellMovePendingStartY) > moveThreshold)
    {
        Tracker_CancelCellMovePending(self);
        return false;
    }
    int row = self->cellMovePendingRow;
    int channel = self->cellMovePendingChannel;
    if (!Tracker_CellMoveCanStart(self, row, channel))
    {
        Tracker_CancelCellMovePending(self);
        return false;
    }
    Tracker_BeginCellMove(self, row, channel);
    self->dragMoved = true;
    Tracker_CancelCellMovePending(self);
    return true;
}

inline void Tracker_BeginCellMove(Tracker *self, int row, int channel)
{
    if (!Tracker_CellMoveCanStart(self, row, channel)) return;
    self->cellMoving = true;
    self->cellMoveValidTarget = false;
    self->cellMoveSourceRow = row;
    self->cellMoveSourceChannel = channel;
    self->cellMoveHoverRow = row;
    self->cellMoveHoverChannel = channel;
    self->cellMoveSource = self->cells[row][channel];
}

inline void Tracker_UpdateCellMoveHover(Tracker *self, int row, int channel)
{
    if (!self || !self->cellMoving) return;
    self->cellMoveHoverRow = row;
    self->cellMoveHoverChannel = channel;
    self->cellMoveValidTarget =
        row >= 0 && row < self->rowCount &&
        channel >= 0 && channel < TRACKER_CHANNELS &&
        (row != self->cellMoveSourceRow || channel != self->cellMoveSourceChannel) &&
        Tracker_CellIsEmpty(self->cells[row][channel].text);
}

inline bool Tracker_CommitCellMove(Tracker *self)
{
    if (!self || !self->cellMoving || !self->cellMoveValidTarget)
        return false;
    self->cells[self->cellMoveHoverRow][self->cellMoveHoverChannel] = self->cellMoveSource;
    Tracker_ClearCell(&self->cells[self->cellMoveSourceRow][self->cellMoveSourceChannel]);
    self->cellMoving = false;
    self->cellMoveValidTarget = false;
    self->patternDirty = true;
    self->copyOnWriteRequested = true;
    Tracker_RebuildUsedInstruments(self);
    return true;
}

inline void Tracker_CancelCellMove(Tracker *self)
{
    if (!self) return;
    self->cellMoving = false;
    self->cellMoveValidTarget = false;
    self->cellMoveSourceRow = -1;
    self->cellMoveSourceChannel = -1;
    self->cellMoveHoverRow = -1;
    self->cellMoveHoverChannel = -1;
}

inline void Tracker_MoveLoopRangeToGrabbedRow(Tracker *self, int grabbedRow)
{
    if (!self || self->rowCount <= 0) return;
    int partIndex = Tracker_PartIndexForRow(self, self->loopStart);
    bool implicitSinglePart = self->partCount == 1 && self->parts[0].rowCount <= 0;
    int partStart = implicitSinglePart ? 0 : self->parts[partIndex].startRow;
    int partLength = implicitSinglePart ? self->rowCount : std::max(1, self->parts[partIndex].rowCount);
    int length = std::max(1, std::min(self->loopMoveLength, partLength));
    int offset = std::max(0, std::min(self->loopMoveGrabOffset, length - 1));
    grabbedRow = std::max(partStart, std::min(partStart + partLength - 1, grabbedRow));
    int start = grabbedRow - offset;
    start = std::max(partStart, std::min(partStart + partLength - length, start));
    int end = start + length - 1;
    if (!self->loopEnabled || start != self->loopStart || end != self->loopEnd)
    {
        self->loopEnabled = true;
        self->loopStart = start;
        self->loopEnd = end;
        self->loopRangeDirty = true;
    }
}

inline void Tracker_Tick(Tracker *self, float dt)
{
    if (!self || !self->active) return;
    if (!std::isfinite(dt) || dt <= 0.0f) return;

    Tracker_TickChangeFlashes(self, dt);
    for (int i = 0; i < self->partCount; i++)
    {
        TrackerPart &part = self->parts[i];
        if (!part.collapseAnimating)
            continue;
        part.collapseAnimT = std::min(1.0f, part.collapseAnimT + dt / TRACKER_PART_COLLAPSE_ANIM_DURATION_S);
        if (part.collapseAnimT >= 1.0f)
        {
            part.collapsed = part.collapseAnimTo <= 0.001f;
            part.collapseAnimating = false;
            part.collapseAnimT = 1.0f;
            part.collapseAnimFrom = part.collapsed ? 0.0f : 1.0f;
            part.collapseAnimTo = part.collapseAnimFrom;
        }
    }
    (void)Tracker_TryArmCellMovePending(self, Tracker_NowMs());

    float maxScroll = Tracker_MaxScroll(self);
    float macroTarget = (float)std::max(0, std::min(TRACKER_MACRO_UI_STEPS - TRACKER_MACRO_VISIBLE_STEPS, self->macroViewFirst));
    self->macroViewAnimatedFirst += (macroTarget - self->macroViewAnimatedFirst) * std::min(1.0f, dt * 14.0f);
    if (self->macroRangeSelecting && self->macroRangeAutoScrollDir != 0)
    {
        self->macroRangeAutoScrollTimer += dt;
        while (self->macroRangeAutoScrollTimer >= 1.0f && self->macroRangeAutoScrollDir != 0)
        {
            self->macroRangeAutoScrollTimer -= 1.0f;
            int direction = self->macroRangeAutoScrollDir;
            Tracker_MacroRangeAutoScrollStep(self, direction);
            if (!Tracker_MacroCanScroll(self, direction))
                Tracker_SetMacroRangeAutoScroll(self, 0);
        }
    }
    else
    {
        self->macroRangeAutoScrollTimer = 0.0f;
        if (!self->macroRangeSelecting)
            self->macroRangeAutoScrollDir = 0;
    }
    if (self->editSelecting || self->editMoving)
    {
        float viewportH = self->editSelectViewportHeight > 1.0f ? self->editSelectViewportHeight : self->viewportHeight;
        float edge = std::max(36.0f, self->rowHeight * 1.75f);
        float direction = 0.0f;
        float closeness = 0.0f;
        if (self->editSelectLocalY < edge)
        {
            direction = -1.0f;
            closeness = (edge - self->editSelectLocalY) / edge;
        }
        else if (self->editSelectLocalY > viewportH - edge)
        {
            direction = 1.0f;
            closeness = (self->editSelectLocalY - (viewportH - edge)) / edge;
        }

        if (direction != 0.0f)
        {
            closeness = std::max(0.0f, std::min(1.6f, closeness));
            float speed = self->rowHeight * (3.0f + closeness * closeness * 12.0f);
            self->scrollY += direction * speed * dt;
            self->scrollY = std::max(0.0f, std::min(maxScroll, self->scrollY));
        }
        int row = Tracker_RowAtViewportY(self, self->editSelectLocalY);
        int channel = Tracker_ChannelAtGridX(self->editSelectLocalX, self->editSelectViewportWidth > 0.0f ? self->editSelectViewportWidth : 1.0f);
        if (self->editMoving)
            Tracker_MoveEditSelectionToGrabbedCell(self, row, channel);
        else
            Tracker_SetEditSelection(self, self->editSelectionAnchorRow, row, self->editSelectionAnchorChannel, self->editSelectionCurrentChannel);
    }
    if (self->loopSelecting || self->loopMoving)
    {
        float viewportH = self->loopSelectViewportHeight > 1.0f ? self->loopSelectViewportHeight : self->viewportHeight;
        float edge = std::max(36.0f, self->rowHeight * 1.75f);
        float direction = 0.0f;
        float closeness = 0.0f;
        if (self->loopSelectLocalY < edge)
        {
            direction = -1.0f;
            closeness = (edge - self->loopSelectLocalY) / edge;
        }
        else if (self->loopSelectLocalY > viewportH - edge)
        {
            direction = 1.0f;
            closeness = (self->loopSelectLocalY - (viewportH - edge)) / edge;
        }

        if (direction != 0.0f)
        {
            closeness = std::max(0.0f, std::min(1.6f, closeness));
            float speed = self->rowHeight * (3.0f + closeness * closeness * 12.0f);
            self->scrollY += direction * speed * dt;
            self->scrollY = std::max(0.0f, std::min(maxScroll, self->scrollY));
            int row = Tracker_RowAtViewportY(self, self->loopSelectLocalY);
            if (self->loopMoving)
                Tracker_MoveLoopRangeToGrabbedRow(self, row);
            else
                Tracker_SetLoopRange(self, self->loopAnchor, row);
        }
    }
    else if (!self->dragging && !self->scrollbarDragging)
    {
        if (std::fabs(self->scrollVelocity) > 0.1f)
        {
            self->scrollY += self->scrollVelocity * dt;
            self->scrollVelocity *= std::pow(0.0008f, dt);
        }
        float target = Tracker_SnappedScrollY(self, self->scrollY);
        self->scrollY += (target - self->scrollY) * std::min(1.0f, dt * 12.0f);
    }

    if (self->scrollY < -self->rowHeight * 1.5f) self->scrollY = -self->rowHeight * 1.5f;
    if (self->scrollY > maxScroll + self->rowHeight * 1.5f)
        self->scrollY = maxScroll + self->rowHeight * 1.5f;

    float instMaxScroll = Tracker_InstrumentsMaxScroll(self);
    const float instRowH = self->instrumentsRowHeight > 1.0f ? self->instrumentsRowHeight : 54.0f;
    if (!self->instrumentsDragging)
    {
        if (std::fabs(self->instrumentsScrollVelocity) > 0.1f)
        {
            self->instrumentsScrollY += self->instrumentsScrollVelocity * dt;
            self->instrumentsScrollVelocity *= std::pow(0.0008f, dt);
        }
        float target = Tracker_SnappedInstrumentsScrollY(self, self->instrumentsScrollY);
        self->instrumentsScrollY += (target - self->instrumentsScrollY) * std::min(1.0f, dt * 12.0f);
    }
    if (self->instrumentsScrollY < 0.0f) self->instrumentsScrollY = 0.0f;
    if (self->instrumentsScrollY > instMaxScroll) self->instrumentsScrollY = instMaxScroll;
    if (self->instrumentsScrollY <= 0.0f || self->instrumentsScrollY >= instMaxScroll)
        self->instrumentsScrollVelocity = 0.0f;

    if (self->playing)
    {
        static float s_playAccum = 0.0f;
        s_playAccum += dt;
        if (s_playAccum >= 0.12f)
        {
            s_playAccum = 0.0f;
            int tick = self->playTick + 1;
            int row = self->playRow;
            if (tick >= self->ticksPerRow)
            {
                tick = 0;
                row++;
                int loopStart = self->loopEnabled ? self->loopStart : 0;
                int loopEnd = self->loopEnabled ? self->loopEnd : self->rowCount - 1;
                if (row > loopEnd) row = loopStart;
            }
            setTrackerCursorState(self, row, tick, self->ticksPerRow);
        }
    }
    if (self->clipboardCutCooldown > 0.0f)
        self->clipboardCutCooldown = std::max(0.0f, self->clipboardCutCooldown - dt);
    if (self->clipboardBannerFlashTime > 0.0f)
    {
        self->clipboardBannerFlashTime = std::max(0.0f, self->clipboardBannerFlashTime - dt);
        if (self->clipboardBannerFlashTime <= 0.0f)
        {
            self->clipboardBannerText[0] = '\0';
            self->clipboardBannerKind = TRACKER_CLIPBOARD_BANNER_NONE;
        }
    }
}

inline void Tracker_AddRow(Tracker *self)
{
    Tracker_AddRowToPart(self, Tracker_CurrentPartIndex(self));
}

inline void Tracker_RemoveRow(Tracker *self)
{
    Tracker_RemoveRowFromPart(self, Tracker_CurrentPartIndex(self));
}
