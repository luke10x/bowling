#pragma once

#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "../clayton/clayton_click.h"
#include "../sounds/songs_data.h"

struct Clayton;

static constexpr int TRACKER_CHANNELS = 6;
static constexpr int TRACKER_MAX_ROWS = 348;
static constexpr int TRACKER_CELL_CHARS = 12;
static constexpr int TRACKER_MAX_USED_INSTRUMENTS = 64;

struct TrackerCell
{
    char text[TRACKER_CELL_CHARS] = ".......";
};

struct Tracker
{
    bool active = false;

    int songIndex = 1;
    int rowCount = 32;
    TrackerCell cells[TRACKER_MAX_ROWS][TRACKER_CHANNELS] = {};

    float scrollY = 0.0f;
    float scrollVelocity = 0.0f;
    float rowHeight = 36.0f;
    float viewportHeight = 360.0f;
    bool dragging = false;
    bool dragMoved = false;
    float dragStartY = 0.0f;
    float dragLastY = 0.0f;
    float dragStartScrollY = 0.0f;
    bool scrollbarDragging = false;
    float scrollbarGrabOffsetY = 0.0f;
    bool loopSelecting = false;
    bool loopRangeDirty = false;
    bool patternDirty = false;
    int loopAnchor = 0;
    float loopSelectLocalY = 0.0f;
    float loopSelectViewportHeight = 0.0f;

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
    int editSpecial = 0; // 0 note, 1 OFF, 2 REL, 3 ===, 4 ...
    int editEffect = 0;
    int editEffectParamA = 0;
    int editEffectParamB = 0;
    int editOperator = 0;
    int usedInstruments[TRACKER_MAX_USED_INSTRUMENTS] = {};
    int usedInstrumentCount = 0;
    xfm_patch_opn editPatches[256] = {};
    bool editPatchValid[256] = {};
    bool editPatchDirty[256] = {};

    Clayton_Click closeButton;
    Clayton_Click playButton;
    Clayton_Click stopButton;
    Clayton_Click followButton;
    Clayton_Click addRowButton;
    Clayton_Click removeRowButton;
    Clayton_Click songButtons[4];
    Clayton_Click editorCloseButton;
    Clayton_Click editorNoteTabButton;
    Clayton_Click editorEffectsTabButton;
    Clayton_Click editorCancelButton;
    Clayton_Click instrumentPrevButton;
    Clayton_Click instrumentNextButton;
    Clayton_Click instrumentNameButton;
    Clayton_Click instrumentEditorCloseButton;
    Clayton_Click instrumentAlgoPrevButton;
    Clayton_Click instrumentAlgoNextButton;
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

inline int Tracker_NextUsedInstrument(const Tracker *self, int current, int direction)
{
    if (!self || self->usedInstrumentCount <= 0) return current;
    int idx = 0;
    for (int i = 0; i < self->usedInstrumentCount; i++)
    {
        if (self->usedInstruments[i] == current)
        {
            idx = i;
            break;
        }
    }
    idx = (idx + direction + self->usedInstrumentCount) % self->usedInstrumentCount;
    return self->usedInstruments[idx];
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
    if (inst < 0) inst = Tracker_FindPreviousInstrument(self, self->editRow, self->editChannel);
    if (inst < 0) inst = self->usedInstruments[0];
    self->editInstrument = inst;

    int vol = Tracker_ParseCellVolume(cell);
    if (vol < 0) vol = Tracker_FindPreviousVolume(self, self->editRow, self->editChannel);
    self->editVolume = vol >= 0 ? std::max(0, std::min(127, vol)) : 0x7F;
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
    Tracker_WriteHexByte(cell + 3, self->editInstrument);
    Tracker_WriteHexByte(cell + 5, self->editVolume);
    cell[TRACKER_CELL_CHARS - 1] = '\0';
    self->patternDirty = true;
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
    int inst = std::max(0, std::min(255, self->editInstrument));
    self->editPatchValid[inst] = true;
    self->editPatchDirty[inst] = true;
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

inline void setTrackerSongState(Tracker *self, int songIndex)
{
    if (!self) return;
    const char *pattern = Tracker_SongPattern(songIndex);
    self->songIndex = std::max(1, std::min(4, songIndex));
    self->rowCount = Tracker_ParseLeadingRowCount(pattern);
    self->loopStart = 0;
    self->loopEnd = std::max(0, self->rowCount - 1);
    self->loopAnchor = 0;
    self->loopSelecting = false;
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
    initClaytonClick(&self->addRowButton, "TrackerAddRow");
    initClaytonClick(&self->removeRowButton, "TrackerRemoveRow");
    for (int i = 0; i < 4; i++)
    {
        char id[32];
        (void)std::snprintf(id, sizeof(id), "TrackerSong%d", i + 1);
        initClaytonClick(&self->songButtons[i], id);
    }
    initClaytonClick(&self->editorCloseButton, "TrackerEditorClose");
    initClaytonClick(&self->editorNoteTabButton, "TrackerEditorNoteTab");
    initClaytonClick(&self->editorEffectsTabButton, "TrackerEditorEffectsTab");
    initClaytonClick(&self->editorCancelButton, "TrackerEditorCancel");
    initClaytonClick(&self->instrumentPrevButton, "TrackerInstrumentPrev");
    initClaytonClick(&self->instrumentNextButton, "TrackerInstrumentNext");
    initClaytonClick(&self->instrumentNameButton, "TrackerInstrumentNameClick");
    initClaytonClick(&self->instrumentEditorCloseButton, "TrackerInstrumentEditorClose");
    initClaytonClick(&self->instrumentAlgoPrevButton, "TrackerInstrumentAlgoPrev");
    initClaytonClick(&self->instrumentAlgoNextButton, "TrackerInstrumentAlgoNext");
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
    self->dragging = false;
    self->scrollbarDragging = false;
    self->loopSelecting = false;
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
    if (start != self->loopStart || end != self->loopEnd)
    {
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
    if (self->loopSelecting)
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
                if (row > self->loopEnd) row = self->loopStart;
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
    self->loopEnd = self->rowCount - 1;
    self->loopRangeDirty = true;
}

inline void Tracker_RemoveRow(Tracker *self)
{
    if (!self || self->rowCount <= 1) return;
    self->rowCount--;
    self->playRow = std::min(self->playRow, self->rowCount - 1);
    self->loopEnd = std::min(self->loopEnd, self->rowCount - 1);
    self->loopStart = std::min(self->loopStart, self->loopEnd);
    self->loopRangeDirty = true;
}
