#pragma once

#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <utility>

#include "../clayton/clayton_click.h"
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
static constexpr int TRACKER_CELL_CHARS = 7 + TRACKER_MAX_EFFECT_SLOTS * 4 + 1;
static constexpr int TRACKER_MAX_USED_INSTRUMENTS = 64;
static constexpr int TRACKER_INSTRUMENT_NAME_CAPACITY = 24;
static constexpr int TRACKER_MAX_PARTS = 32;
static constexpr int TRACKER_PART_NAME_CAPACITY = 32;

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
};

struct TrackerPart
{
    int startRow = 0;
    int rowCount = 0;
    bool collapsed = false;
    bool enabled = true;
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

static constexpr TrackerEffectDef TRACKER_EFFECT_DEFS[] = {
    {0x00, "None", "", "", 0, 0, 0, 0, 0},
    {0x01, "Pitch up", "speed", "", 0, 255, 0, 0, 1},
    {0x02, "Pitch down", "speed", "", 0, 255, 0, 0, 1},
    {0x03, "Portamento", "speed", "", 0, 255, 0, 0, 1},
    {0x04, "Vibrato", "speed", "depth", 0, 15, 0, 15, 2},
    {0x07, "Tremolo", "speed", "depth", 0, 15, 0, 15, 2},
    {0x0A, "Volume slide", "up", "down", 0, 15, 0, 15, 2},
    {0xE1, "Note slide up", "speed", "semi", 0, 15, 0, 15, 2},
    {0xE2, "Note slide down", "speed", "semi", 0, 15, 0, 15, 2},
    {0xE5, "Fine pitch", "offset", "", 0, 255, 0, 0, 1},
    {0xEA, "Legato", "on", "", 0, 1, 0, 0, 1},
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
    bool cellMoving = false;
    bool cellMoveValidTarget = false;
    int cellMoveSourceRow = -1;
    int cellMoveSourceChannel = -1;
    int cellMoveHoverRow = -1;
    int cellMoveHoverChannel = -1;
    TrackerCell cellMoveSource = {};
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
    bool copyOnWriteRequested = false;
    bool songSaveRequested = false;
    bool songSaveConfirmWindowOpen = false;
    bool songSaveConfirmWindowRequested = false;
    bool songLoadRequested = false;
    char songLoadStatus[128] = {};
    bool musicStartRequested = false;
    bool musicPlayRequested = false;
    bool musicStopRequested = false;
    bool previewNoteRequested = false;
    bool previewHeldNoteStartRequested = false;
    bool previewHeldNoteStopRequested = false;
    bool virtualKeyPointerDown = false;
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
    TrackerClipboard clipboard = {};

    bool playing = false;
    bool followCursor = true;
    int playRow = 0;
    int playTick = 0;
    int ticksPerRow = 6;
    int songTickRate = 60;
    int songSpeed = 6;
    int songRowsPerBeat = 4;
    bool songLfoEnabled = false;
    int songLfoFrequency = 0;
    int loopStart = 0;
    int loopEnd = 31;

    bool editorOpen = false;
    bool editorWindowRequested = false;
    bool instrumentEditorOpen = false;
    bool instrumentEditorWindowRequested = false;
    bool instrumentColorWindowOpen = false;
    bool instrumentColorWindowRequested = false;
    bool instrumentsWindowOpen = false;
    bool instrumentsWindowRequested = false;
    bool songSettingsWindowOpen = false;
    bool songSettingsWindowRequested = false;
    bool operatorEditorOpen = false;
    bool operatorEditorWindowRequested = false;
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
    int macroViewFirst = 0;
    float macroViewAnimatedFirst = 0.0f;
    float macroViewportWidth = 0.0f;
    bool macroDrawing = false;
    bool macroRangeSelecting = false;
    int macroRangeAnchor = 0;
    bool sliderDragging = false;
    Clay_ElementId sliderActiveId = {};
    int usedInstruments[TRACKER_MAX_USED_INSTRUMENTS] = {};
    int usedInstrumentCount = 0;
    bool availableInstruments[256] = {};
    int availableInstrumentCount = 0;
    bool builtinInstruments[256] = {};
    char instrumentNames[256][TRACKER_INSTRUMENT_NAME_CAPACITY] = {};
    int32_t instrumentNameLengths[256] = {};
    uint32_t instrumentColors[256] = {};
    float instrumentsScrollY = 0.0f;
    float instrumentsScrollVelocity = 0.0f;
    float instrumentsViewportHeight = 360.0f;
    bool instrumentsDragging = false;
    bool instrumentsDragMoved = false;
    float instrumentsDragStartY = 0.0f;
    float instrumentsDragLastY = 0.0f;
    int pendingInstrumentAction = 0; // 1 clone, 2 rename
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
    Clayton_Click songButtons[TRACKER_MAX_SONG_COUNT];
    Clayton_Click saveSongButton;
    Clayton_Click loadSongButton;
    Clayton_Click saveConfirmSaveButton;
    Clayton_Click saveConfirmChangeNameButton;
    Clayton_Click saveConfirmCancelButton;
    Clayton_Click copyButton;
    Clayton_Click cutButton;
    Clayton_Click pasteButton;
    Clayton_Click instrumentsButton;
    Clayton_Click songSettingsButton;
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
    Clayton_Click instrumentColorButton;
    Clayton_Click instrumentColorCloseButton;
    Clayton_Click instrumentsCloseButton;
    Clayton_Click songSettingsCloseButton;
    Clayton_Click songNameButton;
    Clayton_Click songLfoButton;
    Clayton_Click instrumentUpButtons[256];
    Clayton_Click instrumentDownButtons[256];
    Clayton_Click instrumentDeleteButtons[256];
    Clayton_Click instrumentCloneButtons[256];
    Clayton_Click instrumentRenameButtons[256];
    Clayton_Click instrumentPatchTabButton;
    Clayton_Click instrumentEffectsTabButton;
    Clayton_Click instrumentAlgoPrevButton;
    Clayton_Click instrumentAlgoNextButton;
    Clayton_Click macroTargetPrevButton;
    Clayton_Click macroTargetNextButton;
    Clayton_Click macroEnableButton;
    Clayton_Click macroScrollPrevButton;
    Clayton_Click macroScrollNextButton;
    Clayton_Click macroStepPrevButton;
    Clayton_Click macroStepNextButton;
    Clayton_Click macroLoopButton;
    Clayton_Click macroReleaseButton;
    Clayton_Click operatorButtons[4];
    Clayton_Click operatorEditorCloseButton;
    Clayton_Click operatorSsgPrevButton;
    Clayton_Click operatorSsgNextButton;
    Clayton_Click operatorAmButton;

    uint16_t keyHeight;
};

inline const char *Tracker_SongPattern(int songIndex)
{
    switch (songIndex)
    {
    case 1: return SONG_01;
    case 2: return SONG_02;
    case 3: return SONG_03;
    case 4: return SONG_04;
    default: return SONG_01;
    }
}

inline const char *Tracker_SongName(int songIndex)
{
    switch (songIndex)
    {
    case 1: return "Bowling Strike";
    case 2: return "Gutter Groove";
    case 3: return "Pin Crusher";
    case 4: return "Alley Cat";
    default: return "Bowling Strike";
    }
}

inline int Tracker_DefaultSongSpeed(int songIndex)
{
    return songIndex == 2 ? 8 : 6;
}

inline const char *Tracker_DefaultInstrumentName(int instrument)
{
    auto legacyIndexFromBuiltin = [](int inst) -> int {
        // Built-in music instruments live at the end of the 0..255 instrument bank.
        // Legacy ids 0x00..0x13 map to 0xFF..0xEC (0xFF - legacy).
        inst = std::max(0, std::min(255, inst));
        return (inst >= 0xEC && inst <= 0xFF) ? (0xFF - inst) : -1;
    };

    int inst = std::max(0, std::min(255, instrument));
    int legacy = legacyIndexFromBuiltin(inst);
    if (legacy < 0)
        return "Custom";

    switch (legacy)
    {
    case 0x00: return "Rubber Bass";
    case 0x01: return "Hollow Electric";
    case 0x02: return "Angry Hihat";
    case 0x03: return "Guitar";
    case 0x04: return "Saw";
    case 0x05: return "Flute";
    case 0x06: return "Football Kick";
    case 0x07: return "Snare";
    case 0x08: return "Hihat";
    case 0x09: return "Wah";
    case 0x0A: return "Guitar2";
    case 0x0B: return "Bass Kick";
    case 0x0C: return "Tsh";
    case 0x0D: return "Tick";
    case 0x0E: return "Lead";
    case 0x0F: return "Kick";
    case 0x10: return "Hardbass";
    case 0x11: return "Lowbass";
    case 0x12: return "Axe";
    case 0x13: return "Roll";
    default: return "Custom";
    }
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

inline bool Tracker_IsBuiltinInstrument(const Tracker *self, int instrument)
{
    if (!self) return false;
    int inst = std::max(0, std::min(255, instrument));
    return self->builtinInstruments[inst];
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
    if (!self || code == 0) return -1;
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
        if (self->editEffectActive[i] && self->editEffectCodes[i] != 0)
            count++;
    return count;
}

inline int Tracker_FirstFreeEffectSlot(const Tracker *self)
{
    if (!self) return -1;
    for (int i = 0; i < TRACKER_MAX_EFFECT_SLOTS; i++)
        if (!self->editEffectActive[i] || self->editEffectCodes[i] == 0)
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
    if (!self || code == 0) return;
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
    if (code == 0) return;
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
    case 0x00: return "No effect.";
    case 0x01: return "Pitch slide up (continuous).";
    case 0x02: return "Pitch slide down (continuous).";
    case 0x03: return "Portamento towards next note.";
    case 0x04: return "Vibrato: A=speed, B=depth (nibbles).";
    case 0x07: return "Tremolo: A=speed, B=depth (nibbles).";
    case 0x0A: return "Volume slide: A=up, B=down (nibbles).";
    case 0xE1: return "Note slide up: A=speed, B=semitones (nibbles).";
    case 0xE2: return "Note slide down: A=speed, B=semitones (nibbles).";
    case 0xE5: return "Fine pitch offset (signed/centered by instrument implementation).";
    case 0xEA: return "Legato on/off (keeps envelope).";
    case 0xF5: return "Disable a macro target for this note.";
    case 0xF6: return "Enable a macro target for this note.";
    case 0x10: return "Chip LFO: A=on/off, B=freq (nibbles).";
    case 0x11: return "Feedback (patch FB).";
    case 0x12: return "Operator 1 TL (volume).";
    case 0x13: return "Operator 2 TL (volume).";
    case 0x14: return "Operator 3 TL (volume).";
    case 0x15: return "Operator 4 TL (volume).";
    case 0x16: return "Operator MUL: A=op, B=mul (nibbles).";
    case 0x19: return "All operators AR (attack rate).";
    case 0x1A: return "Operator 1 AR (attack rate).";
    case 0x1B: return "Operator 2 AR (attack rate).";
    case 0x1C: return "Operator 3 AR (attack rate).";
    case 0x1D: return "Operator 4 AR (attack rate).";
    case 0x30: return "Hard reset (debug/experimental).";
    case 0x50: return "Operator AM: A=op, B=on/off (nibbles).";
    case 0x51: return "Operator SL: A=op, B=sl (nibbles).";
    case 0x52: return "Operator RR: A=op, B=rr (nibbles).";
    case 0x53: return "Operator DT: A=op, B=dt (nibbles).";
    case 0x54: return "Operator RS: A=op, B=rs (nibbles).";
    case 0x55: return "Operator SSG: A=op, B=ssg (nibbles).";
    case 0x56: return "All operators DR (decay rate).";
    case 0x57: return "Operator 1 DR (decay rate).";
    case 0x58: return "Operator 2 DR (decay rate).";
    case 0x59: return "Operator 3 DR (decay rate).";
    case 0x5A: return "Operator 4 DR (decay rate).";
    case 0x5B: return "All operators SR (sustain rate).";
    case 0x5C: return "Operator 1 SR (sustain rate).";
    case 0x5D: return "Operator 2 SR (sustain rate).";
    case 0x5E: return "Operator 3 SR (sustain rate).";
    case 0x5F: return "Operator 4 SR (sustain rate).";
    case 0x60: return "Operator mask: A=mode, B=mask (nibbles).";
    case 0x61: return "Algorithm (patch ALG).";
    case 0x62: return "FMS (frequency modulation sensitivity).";
    case 0x63: return "AMS (amplitude modulation sensitivity).";
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
        self->builtinInstruments[i] = false;
    }
    self->availableInstrumentCount = 0;
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

inline void Tracker_SetBuiltinInstrument(Tracker *self, int instrument)
{
    if (!self) return;
    int inst = std::max(0, std::min(255, instrument));
    Tracker_SetInstrumentAvailable(self, inst);
    self->builtinInstruments[inst] = true;
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

inline int Tracker_FirstFreeInstrumentSlot(const Tracker *self)
{
    if (!self) return -1;
    for (int inst = 0; inst < 256; inst++)
        if (!self->availableInstruments[inst])
            return inst;
    return -1;
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
    std::swap(self->builtinInstruments[a], self->builtinInstruments[b]);
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
}

inline void Tracker_DeleteInstrument(Tracker *self, int instrument)
{
    if (!self) return;
    int inst = std::max(0, std::min(255, instrument));
    if (!self->availableInstruments[inst] || self->builtinInstruments[inst])
        return;
    self->availableInstruments[inst] = false;
    self->availableInstrumentCount = std::max(0, self->availableInstrumentCount - 1);
    self->builtinInstruments[inst] = false;
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
    self->builtinInstruments[target] = false;
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
        self->editEffectCodes[i] = TRACKER_EFFECT_DEFS[1].code;
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
        self->editEffectActive[slot] = self->editEffectCodes[slot] != 0;
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
        self->editEffect = 1;
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
        if (!self->editEffectActive[i] || self->editEffectCodes[i] == 0) continue;
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
        self->editEffectCodes[i] = TRACKER_EFFECT_DEFS[1].code;
        self->editEffectValues[i] = 0;
    }
    for (int i = 0; i < TRACKER_EFFECT_DEF_COUNT; i++)
        self->editEffectValuesByDef[i] = 0;
    self->editEffect = 1;
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

inline bool Tracker_IsBuiltinSongInstrument(int inst)
{
    return inst >= 0xEC && inst <= 0xFF;
}

inline const xfm_patch_opn *Tracker_BuiltinInstrumentPatch(int instrument)
{
    int inst = std::max(0, std::min(255, instrument));
    if (!Tracker_IsBuiltinSongInstrument(inst))
        return nullptr;
    int legacy = 0xFF - inst;
    switch (legacy)
    {
    case 0x00: return &PATCH_00_RUBBER_BASS;
    case 0x01: return &PATCH_01_HOLLOW_ELECTRIC;
    case 0x02: return &PATCH_02_ANGRY_HIHAT;
    case 0x03: return &PATCH_03_GUITAR;
    case 0x04: return &PATCH_04_SAW;
    case 0x05: return &PATCH_05_FLUTE;
    case 0x06: return &PATCH_06_FOOTBALL_KICK;
    case 0x07: return &PATCH_07_SNARE;
    case 0x08: return &PATCH_08_HIHAT;
    case 0x09: return &PATCH_09_WAH;
    case 0x0A: return &PATCH_0A_GUITAR2;
    case 0x0B: return &PATCH_0B_BASS_KICK;
    case 0x0C: return &PATCH_0C_TSH;
    case 0x0D: return &PATCH_0D_TICK;
    case 0x0E: return &PATCH_0E_LEAD;
    case 0x0F: return &PATCH_0F_KICK;
    case 0x10: return &PATCH_10_HARDBASS;
    case 0x11: return &PATCH_11_LOWBASS;
    case 0x12: return &PATCH_12_AXE;
    case 0x13: return &PATCH_13_ROLL;
    default: return nullptr;
    }
}

inline void Tracker_LoadBuiltinInstrumentCatalog(Tracker *self)
{
    if (!self) return;
    Tracker_ClearAvailableInstruments(self);
    for (int legacy = 0; legacy <= 0x13; legacy++)
    {
        const int inst = 0xFF - legacy;
        const xfm_patch_opn *patch = Tracker_BuiltinInstrumentPatch(inst);
        if (!patch) continue;
        Tracker_SetBuiltinInstrument(self, inst);
        const char *name = Tracker_DefaultInstrumentName(inst);
        Tracker_SetInstrumentName(self, inst, name, (int32_t)std::strlen(name));
        self->instrumentColors[inst] = Tracker_DefaultInstrumentColor(inst);
        self->editPatches[inst] = *patch;
        self->editPatchValid[inst] = true;
        self->editPatchDirty[inst] = false;
        for (int target = 0; target < XFM_MACRO_TARGET_COUNT; target++)
        {
            self->editMacroEnabled[inst][target] = false;
            self->editMacroValid[inst][target] = false;
            self->editMacroDirty[inst][target] = false;
        }
    }
}

inline void Tracker_LoadBuiltinInstrumentCatalogPreserveCustom(Tracker *self)
{
    if (!self) return;
    bool prevAvailable[256] = {};
    for (int i = 0; i < 256; i++)
        prevAvailable[i] = self->availableInstruments[i] && !Tracker_IsBuiltinSongInstrument(i);
    Tracker_LoadBuiltinInstrumentCatalog(self);
    for (int i = 0; i < 256; i++)
        if (prevAvailable[i])
            Tracker_SetInstrumentAvailable(self, i);
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
    // Custom instruments start at 0x00 and fill upwards. Built-in song instruments
    // occupy the end of the bank (0xEC..0xFF).
    for (int inst = 0x00; inst < 0xEC; inst++)
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

inline void Tracker_CopyBuiltinInstrumentForEdit(Tracker *self)
{
    if (!self || self->songIndex == TRACKER_USER_SONG_SLOT) return;
    int oldInst = std::max(0, std::min(255, self->editInstrument));
    if (!Tracker_IsBuiltinSongInstrument(oldInst)) return;

    Tracker_RebuildUsedInstruments(self);
    int newInst = Tracker_FirstUnusedCustomInstrument(self);
    if (newInst == oldInst) return;

    self->editPatches[newInst] = self->editPatchValid[oldInst] ? self->editPatches[oldInst] : Tracker_DefaultPatch();
    self->editPatchValid[newInst] = true;
    self->editPatchDirty[newInst] = true;
    for (int target = XFM_MACRO_TL1; target < XFM_MACRO_TARGET_COUNT; target++)
    {
        self->editMacros[newInst][target] = self->editMacros[oldInst][target];
        self->editMacroEnabled[newInst][target] = self->editMacroEnabled[oldInst][target];
        self->editMacroValid[newInst][target] = self->editMacroValid[oldInst][target];
        self->editMacroDirty[newInst][target] =
            self->editMacroEnabled[newInst][target] || self->editMacroDirty[oldInst][target];
    }
    self->editInstrument = newInst;
    self->editInstrumentExplicit = true;
    Tracker_RemapInstrumentInCurrentSong(self, oldInst, newInst);
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

inline void Tracker_MarkPatchDirty(Tracker *self)
{
    if (!self) return;
    Tracker_CopyBuiltinInstrumentForEdit(self);
    int inst = std::max(0, std::min(255, self->editInstrument));
    self->editPatchValid[inst] = true;
    self->editPatchDirty[inst] = true;
    self->copyOnWriteRequested = true;
}

inline const char *Tracker_MacroTargetName(int target)
{
    switch (target)
    {
    case XFM_MACRO_TL1: return "TL1";
    case XFM_MACRO_TL2: return "TL2";
    case XFM_MACRO_TL3: return "TL3";
    case XFM_MACRO_TL4: return "TL4";
    case XFM_MACRO_MUL1: return "MUL1";
    case XFM_MACRO_MUL2: return "MUL2";
    case XFM_MACRO_MUL3: return "MUL3";
    case XFM_MACRO_MUL4: return "MUL4";
    case XFM_MACRO_DT1: return "DT1";
    case XFM_MACRO_DT2: return "DT2";
    case XFM_MACRO_DT3: return "DT3";
    case XFM_MACRO_DT4: return "DT4";
    case XFM_MACRO_FB: return "FB";
    case XFM_MACRO_ARP: return "ARP";
    case XFM_MACRO_AR1: return "AR1";
    case XFM_MACRO_AR2: return "AR2";
    case XFM_MACRO_AR3: return "AR3";
    case XFM_MACRO_AR4: return "AR4";
    case XFM_MACRO_DR1: return "DR1";
    case XFM_MACRO_DR2: return "DR2";
    case XFM_MACRO_DR3: return "DR3";
    case XFM_MACRO_DR4: return "DR4";
    case XFM_MACRO_SR1: return "SR1";
    case XFM_MACRO_SR2: return "SR2";
    case XFM_MACRO_SR3: return "SR3";
    case XFM_MACRO_SR4: return "SR4";
    case XFM_MACRO_SL1: return "SL1";
    case XFM_MACRO_SL2: return "SL2";
    case XFM_MACRO_SL3: return "SL3";
    case XFM_MACRO_SL4: return "SL4";
    case XFM_MACRO_RR1: return "RR1";
    case XFM_MACRO_RR2: return "RR2";
    case XFM_MACRO_RR3: return "RR3";
    case XFM_MACRO_RR4: return "RR4";
    case XFM_MACRO_SSG1: return "SSG1";
    case XFM_MACRO_SSG2: return "SSG2";
    case XFM_MACRO_SSG3: return "SSG3";
    case XFM_MACRO_SSG4: return "SSG4";
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
    macro->length = TRACKER_MACRO_UI_STEPS;
    macro->loop_start = 0;
    macro->release_start = 0xFF;
    macro->has_loop = false;
    int16_t value = 0;
    if (macro->target >= XFM_MACRO_MUL1 && macro->target <= XFM_MACRO_MUL4)
        value = 1;
    for (int i = 0; i < XFM_MAX_MACRO_VALUES; i++)
        macro->values[i] = value;
}

inline int16_t Tracker_MacroDefaultValue(int target)
{
    return (target >= XFM_MACRO_MUL1 && target <= XFM_MACRO_MUL4) ? 1 : 0;
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
    if (macro.length == 0) macro.length = 1;
    if (macro.length > XFM_MAX_MACRO_VALUES) macro.length = XFM_MAX_MACRO_VALUES;
    self->editMacroValueIndex = std::max(0, std::min((int)macro.length - 1, self->editMacroValueIndex));
    return macro;
}

inline void Tracker_MarkMacroDirty(Tracker *self)
{
    if (!self) return;
    Tracker_CopyBuiltinInstrumentForEdit(self);
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

inline void Tracker_EnsureMacroUiLength(XfmMacro *macro)
{
    if (!macro) return;
    if (macro->length == 0)
        macro->length = 1;
    uint8_t oldLength = macro->length;
    if (macro->length < TRACKER_MACRO_UI_STEPS)
    {
        int16_t fill = macro->values[oldLength - 1];
        for (int i = oldLength; i < TRACKER_MACRO_UI_STEPS; i++)
            macro->values[i] = fill;
        macro->length = TRACKER_MACRO_UI_STEPS;
    }
    if (macro->length > TRACKER_MACRO_UI_STEPS)
        macro->length = TRACKER_MACRO_UI_STEPS;
    if (macro->has_loop && macro->loop_start >= macro->length)
        macro->loop_start = macro->length - 1;
    if (macro->release_start != 0xFF && macro->release_start >= macro->length)
        macro->release_start = macro->length - 1;
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
                hasMacros = true;
        bool hasName = tracker->instrumentNameLengths[inst] > 0;
        bool shouldSave = tracker->availableInstruments[inst] || usedByPattern[inst] ||
                          tracker->editPatchValid[inst] || hasMacros || hasName;
        if (!shouldSave)
            continue;

        xfm_patch_opn patch = tracker->editPatchValid[inst] ? tracker->editPatches[inst] : Tracker_DefaultPatch();
        char line[512];
        std::snprintf(line, sizeof(line), "INST %02X\nPATCH %u %u %u %u\n", inst, patch.ALG, patch.FB, patch.AMS, patch.FMS);
        out += line;
        if (tracker->instrumentNameLengths[inst] > 0)
        {
            std::snprintf(line, sizeof(line), "NAME %s\n", tracker->instrumentNames[inst]);
            out += line;
        }
        std::snprintf(line, sizeof(line), "COLOR %06X\n", (unsigned int)Tracker_InstrumentColorU32(tracker, inst));
        out += line;
        for (int op = 0; op < 4; op++)
        {
            const xfm_patch_opn_operator &o = patch.op[op];
            std::snprintf(
                line,
                sizeof(line),
                "OP %d %d %u %u %u %u %u %u %u %u %u %u\n",
                op + 1,
                (int)o.DT,
                o.MUL,
                o.TL,
                o.RS,
                o.AR,
                o.AM,
                o.DR,
                o.SR,
                o.SL,
                o.RR,
                o.SSG
            );
            out += line;
        }
        for (int target = XFM_MACRO_TL1; target < XFM_MACRO_TARGET_COUNT; target++)
        {
            if (!tracker->editMacroEnabled[inst][target] || !tracker->editMacroValid[inst][target])
                continue;
            XfmMacro macro = tracker->editMacros[inst][target];
            Tracker_EnsureMacroUiLength(&macro);
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
    Tracker_EnsureMacroUiLength(&macro);
    int start = std::max(0, std::min(a, b));
    int end = std::min(TRACKER_MACRO_UI_STEPS - 1, std::max(a, b));
    macro.has_loop = true;
    macro.loop_start = (uint8_t)start;
    macro.release_start = end < TRACKER_MACRO_UI_STEPS - 1 ? (uint8_t)(end + 1) : 0xFF;
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

inline int Tracker_VisibleRowCount(const Tracker *self)
{
    if (!self) return 0;
    if (self->partCount == 1 && self->parts[0].rowCount <= 0)
        return std::max(1, self->rowCount + 1);
    int visualRows = 0;
    for (int i = 0; i < self->partCount; i++)
        visualRows += 1 + (self->parts[i].collapsed ? 0 : self->parts[i].rowCount);
    return std::max(1, visualRows);
}

inline int Tracker_VisualIndexForPartTitle(const Tracker *self, int partIndex)
{
    if (!self) return 0;
    int visual = 0;
    partIndex = std::max(0, std::min(std::max(0, self->partCount - 1), partIndex));
    for (int i = 0; i < partIndex; i++)
        visual += 1 + (self->parts[i].collapsed ? 0 : self->parts[i].rowCount);
    return visual;
}

inline int Tracker_VisualIndexForRow(const Tracker *self, int row)
{
    if (!self) return 0;
    int partIndex = Tracker_PartIndexForRow(self, row);
    const TrackerPart &part = self->parts[partIndex];
    if (self->partCount == 1 && part.rowCount <= 0)
        return std::max(0, std::min(std::max(0, self->rowCount - 1), row)) + 1;
    int visual = Tracker_VisualIndexForPartTitle(self, partIndex);
    if (part.collapsed)
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
        if (!part.collapsed)
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
}

inline void Tracker_TogglePartCollapsed(Tracker *self, int partIndex)
{
    if (!self || partIndex < 0 || partIndex >= self->partCount) return;
    self->parts[partIndex].collapsed = !self->parts[partIndex].collapsed;
    self->scrollY = std::max(0.0f, std::min(Tracker_MaxScroll(self), self->scrollY));
}

inline bool Tracker_RowEnabledForPlayback(const Tracker *tracker, int row)
{
    if (!tracker) return true;
    int partIndex = Tracker_PartIndexForRow(tracker, row);
    return partIndex >= 0 && partIndex < tracker->partCount ? tracker->parts[partIndex].enabled : true;
}

inline int Tracker_PlaybackRowCount(const Tracker *tracker)
{
    if (!tracker) return 1;
    int rows = 0;
    for (int partIndex = 0; partIndex < tracker->partCount; partIndex++)
        if (tracker->parts[partIndex].enabled)
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
        if (!part.enabled)
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
        if (!part.enabled)
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
        if (!part.enabled)
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
        if (!part.enabled)
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

inline void setTrackerPatternState(Tracker *self, int songIndex, const char *pattern, const char *displayName)
{
    if (!self) return;
    if (!pattern) pattern = Tracker_SongPattern(songIndex);
    self->songIndex = std::max(1, std::min(TRACKER_MAX_SONG_COUNT, songIndex));
    const char *name = displayName && displayName[0] ? displayName : Tracker_SongName(self->songIndex);
    std::snprintf(self->songDisplayName, sizeof(self->songDisplayName), "%s", name);
    self->rowCount = Tracker_ParseLeadingRowCount(pattern);
    self->songTickRate = 60;
    self->songSpeed = Tracker_DefaultSongSpeed(self->songIndex);
    self->ticksPerRow = self->songSpeed;
    self->songRowsPerBeat = 4;
    self->songLfoEnabled = false;
    self->songLfoFrequency = 0;
    self->loopStart = 0;
    self->loopEnd = std::max(0, self->rowCount - 1);
    self->loopAnchor = 0;
    self->loopSelecting = false;
    self->loopMoving = false;
    self->loopEnabled = false;
    self->channelSelectionEnabled = false;
    self->channelSelecting = false;
    self->loopRangeDirty = true;
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
    setTrackerPatternState(self, songIndex, Tracker_SongPattern(songIndex), Tracker_SongName(songIndex));
}

inline void Tracker_LoadSong(Tracker *self, int songIndex)
{
    setTrackerSongState(self, songIndex);
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
    }
    initClaytonClick(&self->saveSongButton, "TrackerSaveSong");
    initClaytonClick(&self->loadSongButton, "TrackerLoadSong");
    initClaytonClick(&self->saveConfirmSaveButton, "TrackerSaveConfirmSave");
    initClaytonClick(&self->saveConfirmChangeNameButton, "TrackerSaveConfirmChangeName");
    initClaytonClick(&self->saveConfirmCancelButton, "TrackerSaveConfirmCancel");
    initClaytonClick(&self->copyButton, "TrackerCopy");
    initClaytonClick(&self->cutButton, "TrackerCut");
    initClaytonClick(&self->pasteButton, "TrackerPaste");
    initClaytonClick(&self->instrumentsButton, "TrackerInstruments");
    initClaytonClick(&self->songSettingsButton, "TrackerSongSettings");
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
    initClaytonClick(&self->instrumentColorButton, "TrackerInstrumentColorButton");
    initClaytonClick(&self->instrumentColorCloseButton, "TrackerInstrumentColorClose");
    initClaytonClick(&self->instrumentsCloseButton, "TrackerInstrumentsClose");
    initClaytonClick(&self->songSettingsCloseButton, "TrackerSongSettingsClose");
    initClaytonClick(&self->songNameButton, "TrackerSongNameButton");
    initClaytonClick(&self->songLfoButton, "TrackerSongLfoButton");
    for (int i = 0; i < 256; i++)
    {
        char id[40];
        (void)std::snprintf(id, sizeof(id), "TrackerInstrumentUp%02X", i);
        initClaytonClick(&self->instrumentUpButtons[i], id);
        (void)std::snprintf(id, sizeof(id), "TrackerInstrumentDown%02X", i);
        initClaytonClick(&self->instrumentDownButtons[i], id);
        (void)std::snprintf(id, sizeof(id), "TrackerInstrumentDel%02X", i);
        initClaytonClick(&self->instrumentDeleteButtons[i], id);
        (void)std::snprintf(id, sizeof(id), "TrackerInstrumentClone%02X", i);
        initClaytonClick(&self->instrumentCloneButtons[i], id);
        (void)std::snprintf(id, sizeof(id), "TrackerInstrumentName%02X", i);
        initClaytonClick(&self->instrumentRenameButtons[i], id);
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
    initClaytonClick(&self->macroStepPrevButton, "TrackerMacroStepPrev");
    initClaytonClick(&self->macroStepNextButton, "TrackerMacroStepNext");
    initClaytonClick(&self->macroLoopButton, "TrackerMacroLoop");
    initClaytonClick(&self->macroReleaseButton, "TrackerMacroRelease");
    for (int i = 0; i < 4; i++)
    {
        char id[32];
        (void)std::snprintf(id, sizeof(id), "TrackerOperator%d", i + 1);
        initClaytonClick(&self->operatorButtons[i], id);
    }
    initClaytonClick(&self->operatorEditorCloseButton, "TrackerOperatorEditorClose");
    initClaytonClick(&self->operatorSsgPrevButton, "TrackerOperatorSsgPrev");
    initClaytonClick(&self->operatorSsgNextButton, "TrackerOperatorSsgNext");
    initClaytonClick(&self->operatorAmButton, "TrackerOperatorAm");
    Tracker_LoadBuiltinInstrumentCatalog(self);
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
    self->editorOpen = false;
    self->instrumentEditorOpen = false;
    self->instrumentColorWindowOpen = false;
    self->instrumentsWindowOpen = false;
    self->songSettingsWindowOpen = false;
    self->songSaveConfirmWindowOpen = false;
    self->pendingPartNameKeypadOpen = false;
    self->pendingPartNameKeypadActive = false;
    self->operatorEditorOpen = false;
    self->instrumentEditorTab = 0;
    self->dragging = false;
    self->cellMoving = false;
    self->scrollbarDragging = false;
    self->loopSelecting = false;
    self->loopMoving = false;
    self->channelSelecting = false;
    self->macroDrawing = false;
    self->macroRangeSelecting = false;
    self->instrumentsDragging = false;
}

inline void Tracker_Close(Tracker *self)
{
    if (!self) return;
    self->active = false;
    self->playing = false;
    self->editorOpen = false;
    self->editorWindowRequested = false;
    self->instrumentEditorOpen = false;
    self->instrumentEditorWindowRequested = false;
    self->instrumentColorWindowOpen = false;
    self->instrumentColorWindowRequested = false;
    self->instrumentsWindowOpen = false;
    self->instrumentsWindowRequested = false;
    self->songSettingsWindowOpen = false;
    self->songSettingsWindowRequested = false;
    self->songSaveConfirmWindowOpen = false;
    self->songSaveConfirmWindowRequested = false;
    self->pendingPartAction = 0;
    self->pendingPartNameKeypadOpen = false;
    self->pendingPartNameKeypadActive = false;
    self->operatorEditorOpen = false;
    self->operatorEditorWindowRequested = false;
    self->dragging = false;
    self->scrollbarDragging = false;
    self->loopSelecting = false;
    self->loopMoving = false;
    self->channelSelecting = false;
    self->macroDrawing = false;
    self->macroRangeSelecting = false;
    self->instrumentsDragging = false;
}

inline float Tracker_MaxScroll(const Tracker *self)
{
    if (!self) return 0.0f;
    return std::max(0.0f, (float)Tracker_VisibleRowCount(self) * self->rowHeight - self->viewportHeight);
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
    const float rowH = 54.0f;
    return std::max(0.0f, (float)std::max(0, self->availableInstrumentCount) * rowH - self->instrumentsViewportHeight);
}

inline void Tracker_SnapToGrid(Tracker *self)
{
    if (!self || self->rowHeight <= 0.0f) return;
    self->scrollY = Tracker_SnappedScrollY(self, self->scrollY);
}

inline void Tracker_SnapInstruments(Tracker *self)
{
    if (!self) return;
    const float rowH = 54.0f;
    float snapped = std::round(self->instrumentsScrollY / rowH) * rowH;
    self->instrumentsScrollY = std::max(0.0f, std::min(Tracker_InstrumentsMaxScroll(self), snapped));
}

inline int Tracker_RowAtViewportY(const Tracker *self, float localY)
{
    if (!self || self->rowHeight <= 0.0f) return 0;
    return Tracker_FirstEditableRowForVisualY(self, localY);
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
    return self && self->loopEnabled ? std::max(0, self->loopEnd - self->loopStart + 1) : 0;
}

inline int Tracker_SelectedChannelStart(const Tracker *self)
{
    return self && self->channelSelectionEnabled ? self->channelStart : 0;
}

inline int Tracker_SelectedChannelCount(const Tracker *self)
{
    if (!self) return 0;
    if (!self->channelSelectionEnabled) return TRACKER_CHANNELS;
    return std::max(0, self->channelEnd - self->channelStart + 1);
}

inline bool Tracker_HasSelection(const Tracker *self)
{
    return self && self->loopEnabled && Tracker_SelectedRowCount(self) > 0 && Tracker_SelectedChannelCount(self) > 0;
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
           self->clipboard.rows == Tracker_SelectedRowCount(self) &&
           self->clipboard.channels == Tracker_SelectedChannelCount(self);
}

inline void Tracker_CopySelection(Tracker *self)
{
    if (!Tracker_HasSelection(self)) return;
    int rows = Tracker_SelectedRowCount(self);
    int channels = Tracker_SelectedChannelCount(self);
    int chStart = Tracker_SelectedChannelStart(self);
    self->clipboard.valid = true;
    self->clipboard.rows = rows;
    self->clipboard.channels = channels;
    for (int r = 0; r < rows; r++)
        for (int ch = 0; ch < channels; ch++)
            self->clipboard.cells[r][ch] = self->cells[self->loopStart + r][chStart + ch];
}

inline void Tracker_PasteSelection(Tracker *self)
{
    if (!Tracker_CanPaste(self)) return;
    int rows = Tracker_SelectedRowCount(self);
    int channels = Tracker_SelectedChannelCount(self);
    int chStart = Tracker_SelectedChannelStart(self);
    for (int r = 0; r < rows; r++)
        for (int ch = 0; ch < channels; ch++)
            self->cells[self->loopStart + r][chStart + ch] = self->clipboard.cells[r][ch];
    self->patternDirty = true;
    self->copyOnWriteRequested = true;
}

inline void Tracker_CutSelection(Tracker *self)
{
    if (!Tracker_HasSelection(self)) return;
    Tracker_CopySelection(self);
    int rows = Tracker_SelectedRowCount(self);
    int channels = Tracker_SelectedChannelCount(self);
    int chStart = Tracker_SelectedChannelStart(self);
    for (int r = 0; r < rows; r++)
        for (int ch = 0; ch < channels; ch++)
            Tracker_ClearCell(&self->cells[self->loopStart + r][chStart + ch]);
    self->patternDirty = true;
    self->copyOnWriteRequested = true;
    Tracker_RebuildUsedInstruments(self);
}

inline bool Tracker_CellMoveCanStart(const Tracker *self, int row, int channel)
{
    if (!self || row < 0 || row >= self->rowCount || channel < 0 || channel >= TRACKER_CHANNELS)
        return false;
    return !Tracker_CellIsEmpty(self->cells[row][channel].text);
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

    float maxScroll = Tracker_MaxScroll(self);
    float macroTarget = (float)std::max(0, std::min(TRACKER_MACRO_UI_STEPS - TRACKER_MACRO_VISIBLE_STEPS, self->macroViewFirst));
    self->macroViewAnimatedFirst += (macroTarget - self->macroViewAnimatedFirst) * std::min(1.0f, dt * 14.0f);
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
    const float instRowH = 54.0f;
    if (!self->instrumentsDragging)
    {
        if (std::fabs(self->instrumentsScrollVelocity) > 0.1f)
        {
            self->instrumentsScrollY += self->instrumentsScrollVelocity * dt;
            self->instrumentsScrollVelocity *= std::pow(0.0008f, dt);
        }
        float target = std::round(self->instrumentsScrollY / instRowH) * instRowH;
        target = std::max(0.0f, std::min(instMaxScroll, target));
        self->instrumentsScrollY += (target - self->instrumentsScrollY) * std::min(1.0f, dt * 12.0f);
    }
    if (self->instrumentsScrollY < -instRowH * 1.5f) self->instrumentsScrollY = -instRowH * 1.5f;
    if (self->instrumentsScrollY > instMaxScroll + instRowH * 1.5f)
        self->instrumentsScrollY = instMaxScroll + instRowH * 1.5f;

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
}

inline void Tracker_AddRow(Tracker *self)
{
    Tracker_AddRowToPart(self, Tracker_CurrentPartIndex(self));
}

inline void Tracker_RemoveRow(Tracker *self)
{
    Tracker_RemoveRowFromPart(self, Tracker_CurrentPartIndex(self));
}
