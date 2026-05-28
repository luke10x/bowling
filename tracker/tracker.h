#pragma once

#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "../clayton/clayton_click.h"
#include "../sounds/songs_data.h"
#include "tracker_song_io.h"

struct Clayton;

static constexpr int TRACKER_CHANNELS = 6;
static constexpr int TRACKER_MAX_ROWS = TRACKER_USER_SONG_MAX_ROWS;
static constexpr int TRACKER_MAX_EFFECT_SLOTS = 4;
static constexpr int TRACKER_MACRO_UI_STEPS = 32;
static constexpr int TRACKER_MACRO_VISIBLE_STEPS = 8;
static constexpr int TRACKER_MACRO_SCROLL_STEP = 4;
static constexpr int TRACKER_CELL_CHARS = 7 + TRACKER_MAX_EFFECT_SLOTS * 4 + 1;
static constexpr int TRACKER_MAX_USED_INSTRUMENTS = 64;

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
    {0xF5, "Macro off", "target", "", 0, XFM_MACRO_ARP, 0, 0, 1},
    {0xF6, "Macro on", "target", "", 0, XFM_MACRO_ARP, 0, 0, 1},
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

    float scrollY = 0.0f;
    float scrollVelocity = 0.0f;
    float rowHeight = 44.0f;
    float viewportHeight = 360.0f;
    bool dragging = false;
    bool dragMoved = false;
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
    bool copyOnWriteRequested = false;
    bool songSaveRequested = false;
    bool songLoadRequested = false;
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
    TrackerClipboard clipboard = {};

    bool playing = false;
    bool followCursor = true;
    int playRow = 0;
    int playTick = 0;
    int ticksPerRow = 6;
    int loopStart = 0;
    int loopEnd = 31;

    bool editorOpen = false;
    bool editorWindowRequested = false;
    bool instrumentEditorOpen = false;
    bool instrumentEditorWindowRequested = false;
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
    int editEffectSlot = 0;
    uint8_t editEffectCodes[TRACKER_MAX_EFFECT_SLOTS] = {};
    uint8_t editEffectValues[TRACKER_MAX_EFFECT_SLOTS] = {};
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
    Clayton_Click songButtons[TRACKER_MAX_SONG_COUNT];
    Clayton_Click saveSongButton;
    Clayton_Click loadSongButton;
    Clayton_Click copyButton;
    Clayton_Click pasteButton;
    Clayton_Click editorCloseButton;
    Clayton_Click editorNoteTabButton;
    Clayton_Click editorEffectsTabButton;
    Clayton_Click editorCancelButton;
    Clayton_Click instrumentExplicitButton;
    Clayton_Click volumeExplicitButton;
    Clayton_Click effectSlotPrevButton;
    Clayton_Click effectSlotNextButton;
    Clayton_Click effectPrevButton;
    Clayton_Click effectNextButton;
    Clayton_Click instrumentPrevButton;
    Clayton_Click instrumentNextButton;
    Clayton_Click instrumentNameButton;
    Clayton_Click instrumentEditorCloseButton;
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
        self->availableInstruments[i] = false;
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

inline void Tracker_ParseCellForEditor(Tracker *self)
{
    if (!self) return;
    char *cell = self->cells[self->editRow][self->editChannel].text;
    self->editSpecial = 0;
    if (std::strncmp(cell, "OFF", 3) == 0) self->editSpecial = 1;
    else if (std::strncmp(cell, "REL", 3) == 0) self->editSpecial = 2;
    else if (std::strncmp(cell, "===", 3) == 0) self->editSpecial = 3;
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
        self->editEffectCodes[i] = 0;
        self->editEffectValues[i] = 0;
    }
    int slot = 0;
    int pos = 7;
    while (slot < TRACKER_MAX_EFFECT_SLOTS && pos + 3 < TRACKER_CELL_CHARS && cell[pos] && cell[pos] != '.')
    {
        if (!Tracker_IsHex(cell[pos]) || !Tracker_IsHex(cell[pos + 1]) ||
            !Tracker_IsHex(cell[pos + 2]) || !Tracker_IsHex(cell[pos + 3]))
            break;
        self->editEffectCodes[slot] = (uint8_t)Tracker_ParseHexByte(cell + pos);
        self->editEffectValues[slot] = (uint8_t)Tracker_ParseHexByte(cell + pos + 2);
        slot++;
        pos += 4;
    }
    self->editEffectSlot = std::max(0, std::min(TRACKER_MAX_EFFECT_SLOTS - 1, self->editEffectSlot));
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
    for (int i = 0; i < TRACKER_MAX_EFFECT_SLOTS && pos + 3 < TRACKER_CELL_CHARS - 1; i++)
    {
        if (self->editEffectCodes[i] == 0) continue;
        Tracker_WriteHexByte(cell + pos, self->editEffectCodes[i]);
        Tracker_WriteHexByte(cell + pos + 2, self->editEffectValues[i]);
        pos += 4;
    }
    for (; pos < TRACKER_CELL_CHARS - 1; pos++)
        cell[pos] = '\0';
    cell[TRACKER_CELL_CHARS - 1] = '\0';
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
    return inst >= 0x00 && inst <= 0x11;
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
    for (int inst = 0x12; inst < 256; inst++)
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
    default: return "MAC";
    }
}

inline void Tracker_DefaultMacro(XfmMacro *macro, int target)
{
    if (!macro) return;
    *macro = {};
    macro->target = (uint8_t)std::max((int)XFM_MACRO_TL1, std::min((int)XFM_MACRO_ARP, target));
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
    int target = std::max((int)XFM_MACRO_TL1, std::min((int)XFM_MACRO_ARP, self->editMacroTarget));
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
    int target = std::max((int)XFM_MACRO_TL1, std::min((int)XFM_MACRO_ARP, self->editMacroTarget));
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

inline void setTrackerCursorState(Tracker *self, int row, int tick, int ticksPerRow)
{
    if (!self) return;
    self->ticksPerRow = std::max(1, ticksPerRow);
    self->playRow = std::max(0, std::min(row, std::max(0, self->rowCount - 1)));
    self->playTick = std::max(0, std::min(tick, self->ticksPerRow - 1));
    if (self->followCursor && self->rowHeight > 0.0f)
    {
        const float target = (float)self->playRow * self->rowHeight;
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

    const char *p = Tracker_FindPatternRows(pattern);
    if (!p) return;

    for (int row = 0; row < self->rowCount && *p; row++)
    {
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
    }
    Tracker_RebuildUsedInstruments(self);
    self->patternDirty = false;
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
    initClaytonClick(&self->saveSongButton, "TrackerSaveSong");
    initClaytonClick(&self->loadSongButton, "TrackerLoadSong");
    initClaytonClick(&self->copyButton, "TrackerCopy");
    initClaytonClick(&self->pasteButton, "TrackerPaste");
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
    initClaytonClick(&self->effectSlotPrevButton, "TrackerEffectSlotPrev");
    initClaytonClick(&self->effectSlotNextButton, "TrackerEffectSlotNext");
    initClaytonClick(&self->effectPrevButton, "TrackerEffectPrev");
    initClaytonClick(&self->effectNextButton, "TrackerEffectNext");
    initClaytonClick(&self->instrumentPrevButton, "TrackerInstrumentPrev");
    initClaytonClick(&self->instrumentNextButton, "TrackerInstrumentNext");
    initClaytonClick(&self->instrumentNameButton, "TrackerInstrumentNameClick");
    initClaytonClick(&self->instrumentEditorCloseButton, "TrackerInstrumentEditorClose");
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
    self->operatorEditorOpen = false;
    self->instrumentEditorTab = 0;
    self->dragging = false;
    self->scrollbarDragging = false;
    self->loopSelecting = false;
    self->loopMoving = false;
    self->channelSelecting = false;
    self->macroDrawing = false;
    self->macroRangeSelecting = false;
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
    self->operatorEditorOpen = false;
    self->operatorEditorWindowRequested = false;
    self->dragging = false;
    self->scrollbarDragging = false;
    self->loopSelecting = false;
    self->loopMoving = false;
    self->channelSelecting = false;
    self->macroDrawing = false;
    self->macroRangeSelecting = false;
}

inline float Tracker_MaxScroll(const Tracker *self)
{
    if (!self) return 0.0f;
    return std::max(0.0f, (float)self->rowCount * self->rowHeight - self->viewportHeight);
}

inline void Tracker_SnapToGrid(Tracker *self)
{
    if (!self || self->rowHeight <= 0.0f) return;
    float maxScroll = Tracker_MaxScroll(self);
    float snapped = std::round(self->scrollY / self->rowHeight) * self->rowHeight;
    self->scrollY = std::max(0.0f, std::min(maxScroll, snapped));
}

inline int Tracker_RowAtViewportY(const Tracker *self, float localY)
{
    if (!self || self->rowHeight <= 0.0f) return 0;
    return std::max(0, std::min(self->rowCount - 1, (int)std::floor((localY + self->scrollY) / self->rowHeight)));
}

inline void Tracker_SetLoopRange(Tracker *self, int a, int b)
{
    if (!self || self->rowCount <= 0) return;
    int start = std::max(0, std::min(a, b));
    int end = std::min(self->rowCount - 1, std::max(a, b));
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

inline void Tracker_MoveLoopRangeToGrabbedRow(Tracker *self, int grabbedRow)
{
    if (!self || self->rowCount <= 0) return;
    int length = std::max(1, std::min(self->loopMoveLength, self->rowCount));
    int offset = std::max(0, std::min(self->loopMoveGrabOffset, length - 1));
    int start = grabbedRow - offset;
    start = std::max(0, std::min(self->rowCount - length, start));
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
        float target = std::round(self->scrollY / self->rowHeight) * self->rowHeight;
        target = std::max(0.0f, std::min(maxScroll, target));
        self->scrollY += (target - self->scrollY) * std::min(1.0f, dt * 12.0f);
    }

    if (self->scrollY < -self->rowHeight * 1.5f) self->scrollY = -self->rowHeight * 1.5f;
    if (self->scrollY > maxScroll + self->rowHeight * 1.5f)
        self->scrollY = maxScroll + self->rowHeight * 1.5f;

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
    if (!self || self->rowCount >= TRACKER_MAX_ROWS) return;
    for (int ch = 0; ch < TRACKER_CHANNELS; ch++)
        std::strncpy(self->cells[self->rowCount][ch].text, ".......", TRACKER_CELL_CHARS);
    self->rowCount++;
    if (self->loopEnabled)
    {
        self->loopEnd = self->rowCount - 1;
        self->loopRangeDirty = true;
    }
    self->patternDirty = true;
    self->copyOnWriteRequested = true;
}

inline void Tracker_RemoveRow(Tracker *self)
{
    if (!self || self->rowCount <= 1) return;
    self->rowCount--;
    self->playRow = std::min(self->playRow, self->rowCount - 1);
    self->loopEnd = std::min(self->loopEnd, self->rowCount - 1);
    self->loopStart = std::min(self->loopStart, self->loopEnd);
    if (self->loopEnabled)
        self->loopRangeDirty = true;
    self->patternDirty = true;
    self->copyOnWriteRequested = true;
}
