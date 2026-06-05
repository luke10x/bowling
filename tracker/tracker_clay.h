#pragma once

#include <SDL.h>
#include <algorithm>
#include <cstdio>
#include <cstring>

#include "../clayton/claytheme.h"
#include "../clayton/clayton.h"
#include "tracker.h"

inline Clay_Color Tracker_CellColor(bool activeRow, bool channelHeader)
{
    if (channelHeader) return {38, 48, 74, 255};
    if (activeRow) return {80, 110, 120, 230};
    return {34, 31, 48, 235};
}

inline Clay_Color Tracker_ColorFromU32(uint32_t rgb, float alpha = 255.0f)
{
    return {
        (float)((rgb >> 16) & 0xFF),
        (float)((rgb >> 8) & 0xFF),
        (float)(rgb & 0xFF),
        alpha
    };
}

inline bool Tracker_ColorIsBright(uint32_t rgb)
{
    int r = (int)((rgb >> 16) & 0xFF);
    int g = (int)((rgb >> 8) & 0xFF);
    int b = (int)(rgb & 0xFF);
    return (r * 299 + g * 587 + b * 114) > 150000;
}

inline int Tracker_CellDisplayInstrument(const Tracker *self, const char *cell, int row, int channel)
{
    int inst = Tracker_ParseCellInstrument(cell);
    if (inst >= 0) return inst;
    return Tracker_FindInheritedInstrument(self, row, channel);
}

inline Clay_Color Tracker_CellMoveHighlightColor(const Tracker *self)
{
    if (!self || !self->cellMoving) return {255, 255, 255, 220};
    const char *cell = self->cellMoveSource.text;
    if (Tracker_CellHasPlayableNote(cell))
    {
        int inst = Tracker_CellDisplayInstrument(self, cell, self->cellMoveSourceRow, self->cellMoveSourceChannel);
        uint32_t rgb = inst >= 0 ? Tracker_InstrumentColorU32(self, inst) : 0;
        if (rgb != 0)
            return Tracker_ColorFromU32(rgb, 130.0f);
    }
    return {245, 245, 250, 130};
}

inline Clay_Color Tracker_LoopLineColor(const Tracker *self, int row, bool activeRow)
{
    if (!self || !self->loopEnabled) return Tracker_CellColor(activeRow, true);
    bool inLoop = self && row >= self->loopStart && row <= self->loopEnd;
    if (!inLoop) return Tracker_CellColor(activeRow, true);
    if (row == self->loopStart || row == self->loopEnd)
        return activeRow ? (Clay_Color){118, 154, 80, 255} : (Clay_Color){82, 112, 56, 255};
    return activeRow ? (Clay_Color){76, 112, 78, 255} : (Clay_Color){44, 74, 52, 255};
}

inline Clay_Color Tracker_LoopCellColor(const Tracker *self, int row, bool activeRow)
{
    if (!self || !self->loopEnabled) return Tracker_CellColor(activeRow, false);
    bool inLoop = self && row >= self->loopStart && row <= self->loopEnd;
    if (!inLoop) return Tracker_CellColor(activeRow, false);
    return activeRow ? (Clay_Color){50, 94, 82, 255} : (Clay_Color){25, 46, 42, 255};
}

static constexpr float TRACKER_SIDE_UNIT = 1.0f / 14.0f;
static constexpr float TRACKER_CHANNEL_UNIT = 2.0f / 14.0f;
static constexpr float TRACKER_SCROLLABLE_UNIT = 13.0f / 14.0f;
static constexpr float TRACKER_LINE_IN_SCROLL = 1.0f / 13.0f;
static constexpr float TRACKER_CHANNEL_IN_SCROLL = 2.0f / 13.0f;

inline float Tracker_ScrollbarThumbHeight(const Tracker *self)
{
    if (!self || self->viewportHeight <= 1.0f) return 28.0f;
    float contentHeight = std::max(self->rowHeight, (float)Tracker_VisibleRowCount(self) * self->rowHeight);
    return std::max(28.0f, self->viewportHeight * std::min(1.0f, self->viewportHeight / contentHeight));
}

inline float Tracker_ScrollbarThumbTop(const Tracker *self, float thumbHeight)
{
    if (!self) return 0.0f;
    float maxScroll = Tracker_MaxScroll(self);
    if (maxScroll <= 0.0f) return 0.0f;
    return (self->viewportHeight - thumbHeight) *
        (std::max(0.0f, std::min(maxScroll, self->scrollY)) / maxScroll);
}

inline void Tracker_SetScrollFromScrollbarY(Tracker *self, float localY)
{
    if (!self) return;
    float maxScroll = Tracker_MaxScroll(self);
    float thumbHeight = Tracker_ScrollbarThumbHeight(self);
    float trackRange = std::max(1.0f, self->viewportHeight - thumbHeight);
    float thumbTop = std::max(0.0f, std::min(trackRange, localY - self->scrollbarGrabOffsetY));
    self->scrollY = maxScroll > 0.0f ? (thumbTop / trackRange) * maxScroll : 0.0f;
    self->scrollVelocity = 0.0f;
}

inline float Tracker_InstrumentsScrollbarThumbHeight(const Tracker *self)
{
    if (!self || self->instrumentsViewportHeight <= 1.0f) return 28.0f;
    float rowH = self->instrumentsRowHeight > 1.0f ? self->instrumentsRowHeight : 54.0f;
    float contentHeight = std::max(rowH, (float)std::max(0, self->availableInstrumentCount) * rowH);
    if (contentHeight <= self->instrumentsViewportHeight)
        return self->instrumentsViewportHeight;
    return std::max(28.0f, self->instrumentsViewportHeight * std::min(1.0f, self->instrumentsViewportHeight / contentHeight));
}

inline float Tracker_InstrumentsScrollbarThumbTop(const Tracker *self, float thumbHeight)
{
    if (!self) return 0.0f;
    float maxScroll = Tracker_InstrumentsMaxScroll(self);
    if (maxScroll <= 0.0f) return 0.0f;
    return (self->instrumentsViewportHeight - thumbHeight) *
        (std::max(0.0f, std::min(maxScroll, self->instrumentsScrollY)) / maxScroll);
}

inline void Tracker_SetInstrumentsScrollFromScrollbarY(Tracker *self, float localY)
{
    if (!self) return;
    float maxScroll = Tracker_InstrumentsMaxScroll(self);
    float thumbHeight = Tracker_InstrumentsScrollbarThumbHeight(self);
    float trackRange = std::max(1.0f, self->instrumentsViewportHeight - thumbHeight);
    float thumbTop = std::max(0.0f, std::min(trackRange, localY - self->instrumentsScrollbarGrabOffsetY));
    self->instrumentsScrollY = maxScroll > 0.0f ? (thumbTop / trackRange) * maxScroll : 0.0f;
    self->instrumentsScrollVelocity = 0.0f;
}

inline bool Tracker_PointInBox(float x, float y, Clay_BoundingBox box)
{
    return x >= box.x && x <= box.x + box.width && y >= box.y && y <= box.y + box.height;
}

inline float Tracker_ClampFloat(float value, float lo, float hi)
{
    if (hi < lo) hi = lo;
    return std::max(lo, std::min(hi, value));
}

inline float Tracker_OscilloscopeNormalHeight(Clay_BoundingBox portraitBox)
{
    return std::max(24.0f, portraitBox.height / 6.0f);
}

inline Clay_BoundingBox Tracker_OscilloscopeBox(Tracker *self, Clay_BoundingBox portraitBox, Clay_BoundingBox rootBox)
{
    Clay_BoundingBox box = portraitBox;
    if (!self)
        return box;

    float normalW = std::max(1.0f, portraitBox.width);
    float normalH = Tracker_OscilloscopeNormalHeight(portraitBox);
    if (!self->oscilloscopeInitialized && portraitBox.width > 1.0f && portraitBox.height > 1.0f)
    {
        self->oscilloscopeX = portraitBox.x;
        self->oscilloscopeY = portraitBox.y;
        self->oscilloscopeSnappedToPortrait = true;
        self->oscilloscopeInitialized = true;
    }

    if (self->oscilloscopeMaximized)
        return portraitBox;

    if (self->oscilloscopeSnappedToPortrait)
    {
        box.x = portraitBox.x;
        box.y = Tracker_ClampFloat(self->oscilloscopeY, portraitBox.y, portraitBox.y + std::max(0.0f, portraitBox.height - normalH));
        box.width = normalW;
        box.height = normalH;
        self->oscilloscopeX = box.x;
        self->oscilloscopeY = box.y;
        return box;
    }

    Clay_BoundingBox bounds = rootBox.width > 1.0f && rootBox.height > 1.0f ? rootBox : portraitBox;
    box.width = normalW;
    box.height = normalH;
    box.x = Tracker_ClampFloat(self->oscilloscopeX, bounds.x, bounds.x + std::max(0.0f, bounds.width - normalW));
    box.y = Tracker_ClampFloat(self->oscilloscopeY, bounds.y, bounds.y + std::max(0.0f, bounds.height - normalH));
    self->oscilloscopeX = box.x;
    self->oscilloscopeY = box.y;
    return box;
}

inline int Tracker_OscilloscopeChannelAtPoint(Clay_BoundingBox box, bool maximized, float px, float py)
{
    if (!Tracker_PointInBox(px, py, box)) return -1;
    if (!maximized) return -1;
    float inset = 8.0f;
    float gap = 6.0f;
    float x = px - (box.x + inset);
    float y = py - (box.y + inset);
    float w = std::max(1.0f, box.width - inset * 2.0f);
    float h = std::max(1.0f, box.height - inset * 2.0f);
    if (x < 0.0f || y < 0.0f || x > w || y > h) return -1;

    if (box.width > box.height)
    {
        float cellW = (w - gap * 2.0f) / 3.0f;
        float cellH = (h - gap) / 2.0f;
        int col = (int)(x / (cellW + gap));
        int row = (int)(y / (cellH + gap));
        if (col < 0 || col > 2 || row < 0 || row > 1) return -1;
        if (x - col * (cellW + gap) > cellW || y - row * (cellH + gap) > cellH) return -1;
        return row * 3 + col;
    }

    float cellH = (h - gap * 5.0f) / 6.0f;
    int row = (int)(y / (cellH + gap));
    if (row < 0 || row >= 6) return -1;
    if (y - row * (cellH + gap) > cellH) return -1;
    return row;
}

inline void Tracker_OpenPartEditor(Tracker *self, int partIndex)
{
    if (!self) return;
    Tracker_NormalizeParts(self);
    if (partIndex < 0 || partIndex >= self->partCount) return;
    self->partEditorPart = partIndex;
    self->partEditorOpen = true;
    self->partEditorWindowRequested = true;
}

inline void Tracker_BuildPartTitleContent(
    Tracker *self,
    ClayArena *arena,
    int partIndex,
    Clayton_Click *toggleButton,
    Clayton_Click *upButton,
    Clayton_Click *downButton,
    Clayton_Click *partButton,
    Clay_TextElementConfig buttonCfg,
    Clay_TextElementConfig bodyCfg
)
{
    if (!self || !arena || partIndex < 0 || partIndex >= self->partCount) return;
    TrackerPart &part = self->parts[partIndex];
    float progress = Tracker_PartPlaybackProgress(self, partIndex);

    Clay_ElementDeclaration toggleBtn = CLAY_THEME_BTN_PRIMARY;
    toggleBtn.layout.sizing = {CLAY_SIZING_FIXED(34), CLAY_SIZING_FIXED(26)};
    CLAY(toggleButton->clayId, toggleBtn)
    {
        CLAY_TEXT(part.collapsed ? CLAY_STRING("+") : CLAY_STRING("-"), CLAY_TEXT_CONFIG(buttonCfg));
    }

    CLAY(
        CLAY_IDI("TrackerPartTitleText", partIndex * 2 + (toggleButton == &self->stickyPartToggleButton ? 1 : 0)),
        {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                    .padding = {6, 6, 3, 3},
                    .childGap = 3,
                    .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                    .layoutDirection = CLAY_TOP_TO_BOTTOM},
         .backgroundColor = {18, 20, 30, 255},
         .cornerRadius = {4, 4, 4, 4}}
        )
        {
            CLAY(
                CLAY_IDI("TrackerPartTitleLine", partIndex * 2 + (toggleButton == &self->stickyPartToggleButton ? 1 : 0)),
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                        .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER}}}
        )
        {
            CLAY_TEXT(ClayArena_FormatString(arena, "%s", part.name), CLAY_TEXT_CONFIG(bodyCfg));
        }
        CLAY(
            CLAY_IDI("TrackerPartProgressRail", partIndex * 2 + (toggleButton == &self->stickyPartToggleButton ? 1 : 0)),
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(5)}},
             .backgroundColor = {7, 10, 16, 255},
             .cornerRadius = {2, 2, 2, 2}}
        )
        {
            if (progress > 0.0f)
            {
                CLAY(
                    CLAY_IDI("TrackerPartProgressFill", partIndex * 2 + (toggleButton == &self->stickyPartToggleButton ? 1 : 0)),
                    {.layout = {.sizing = {CLAY_SIZING_PERCENT(std::max(0.0f, std::min(1.0f, progress))), CLAY_SIZING_GROW()}},
                     .backgroundColor = {94, 196, 228, 255},
                     .cornerRadius = {2, 2, 2, 2}}
                ) {}
            }
        }
    }

    Clay_ElementDeclaration smallBtn = CLAY_THEME_BTN_PRIMARY;
    smallBtn.layout.sizing = {CLAY_SIZING_FIXED(34), CLAY_SIZING_FIXED(26)};
    CLAY(upButton->clayId, smallBtn) { CLAY_TEXT(CLAY_STRING("UP"), CLAY_TEXT_CONFIG(buttonCfg)); }
    CLAY(downButton->clayId, smallBtn) { CLAY_TEXT(CLAY_STRING("DN"), CLAY_TEXT_CONFIG(buttonCfg)); }
    Clay_ElementDeclaration partBtn = CLAY_THEME_BTN_PRIMARY;
    partBtn.layout.sizing = {CLAY_SIZING_FIXED(48), CLAY_SIZING_FIXED(26)};
    CLAY(partButton->clayId, partBtn) { CLAY_TEXT(CLAY_STRING("PART"), CLAY_TEXT_CONFIG(buttonCfg)); }
}

inline void Tracker_BuildOscilloscopeOverlay(Tracker *self, Clayton *clayton)
{
    if (!self || !clayton || !self->oscilloscopeVisible) return;

    Clay_BoundingBox portraitBox = Clay_GetElementData(CLAY_ID("Portrait area")).boundingBox;
    Clay_BoundingBox rootBox = Clay_GetElementData(CLAY_ID("Root")).boundingBox;
    Clay_BoundingBox box = Tracker_OscilloscopeBox(self, portraitBox, rootBox);
    if (box.width <= 1.0f || box.height <= 1.0f || portraitBox.width <= 1.0f || portraitBox.height <= 1.0f)
        return;

    Clay_Vector2 offset = {
        box.x + box.width * 0.5f - (rootBox.x + rootBox.width * 0.5f),
        box.y + box.height * 0.5f - (rootBox.y + rootBox.height * 0.5f)
    };
    auto oscLabel = [](int ch) -> Clay_String {
        switch (ch)
        {
            case 0: return CLAY_STRING("CH1");
            case 1: return CLAY_STRING("CH2");
            case 2: return CLAY_STRING("CH3");
            case 3: return CLAY_STRING("CH4");
            case 4: return CLAY_STRING("CH5");
            default: return CLAY_STRING("CH6");
        }
    };
    Clay_TextElementConfig labelCfg = clayton->smallFontCfg;
    labelCfg.fontSize = 18;
    labelCfg.textColor = {230, 248, 255, 255};
    auto renderOscImage = [&](Clay_ElementId id, Gles3_ImageConfig *imageData, int ch, Clay_Color borderColor, uint16_t borderWidth) {
        Clay_ElementDeclaration image = {
            .layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                       .padding = {8, 8, 6, 6},
                       .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_TOP}},
            .backgroundColor = {0, 0, 0, 255},
            .image = {.imageData = imageData},
            .border = {.color = borderColor, .width = CLAY_BORDER_ALL(borderWidth)}
        };
        CLAY(id, image)
        {
            CLAY(
                CLAY_IDI("TrackerOscilloscopeChannelLabel", ch),
                {.layout = {.sizing = {CLAY_SIZING_FIXED(42), CLAY_SIZING_FIXED(22)},
                            .padding = {4, 4, 2, 2},
                            .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER}},
                 .backgroundColor = {0, 0, 0, 170}}
            )
            {
                CLAY_TEXT(oscLabel(ch), CLAY_TEXT_CONFIG(labelCfg));
            }
        }
    };
    CLAY(
        CLAY_ID("TrackerOscilloscopeOverlay"),
        {.layout = {.sizing = {CLAY_SIZING_FIXED(box.width), CLAY_SIZING_FIXED(box.height)},
                    .padding = {8, 8, 8, 8},
                    .childGap = 6,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM},
         .backgroundColor = {0, 0, 0, 255},
         .floating = {
             .offset = offset,
             .zIndex = 220,
             .attachPoints = {CLAY_ATTACH_POINT_CENTER_CENTER, CLAY_ATTACH_POINT_CENTER_CENTER},
             .attachTo = CLAY_ATTACH_TO_PARENT,
         },
         .border = {.color = {255, 255, 255, 255}, .width = CLAY_BORDER_ALL(5)}}
    )
    {
        int selected = std::max(0, std::min(5, self->oscilloscopeSelectedChannel));
        if (!self->oscilloscopeMaximized)
        {
            renderOscImage(
                CLAY_ID("TrackerOscilloscopeSelectedImage"),
                &clayton->trackerOscilloscopeImages[selected],
                selected,
                {0, 0, 0, 0},
                0
            );
        }
        else if (box.width > box.height)
        {
            for (int row = 0; row < 2; row++)
            {
                CLAY(
                    CLAY_IDI("TrackerOscilloscopeRow", row),
                    {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                .childGap = 6,
                                .layoutDirection = CLAY_LEFT_TO_RIGHT}}
                )
                {
                    for (int col = 0; col < 3; col++)
                    {
                        int ch = row * 3 + col;
                        renderOscImage(
                            CLAY_IDI("TrackerOscilloscopeChannelImage", ch),
                            &clayton->trackerOscilloscopeImages[ch],
                            ch,
                            ch == selected ? (Clay_Color){255, 255, 255, 255} : (Clay_Color){48, 58, 64, 255},
                            2
                        );
                    }
                }
            }
        }
        else
        {
            for (int ch = 0; ch < 6; ch++)
            {
                renderOscImage(
                    CLAY_IDI("TrackerOscilloscopeChannelImageTall", ch),
                    &clayton->trackerOscilloscopeImages[ch],
                    ch,
                    ch == selected ? (Clay_Color){255, 255, 255, 255} : (Clay_Color){48, 58, 64, 255},
                    2
                );
            }
        }
    }
}

inline void Tracker_BuildEditor(Tracker *self, Clayton *clayton)
{
    if (!self || !self->editorOpen || !clayton) return;

    Clay_TextElementConfig titleCfg = CLAY_THEME_TEXT_TITLE;
    Clay_TextElementConfig buttonCfg = CLAY_THEME_TEXT_BUTTON;
    Clay_TextElementConfig bodyCfg = CLAY_THEME_TEXT_BODY;
    Clay_TextElementConfig mutedCfg = bodyCfg;
    mutedCfg.textColor = {150, 154, 170, 255};
    ClayArena *arena = &clayton->clayArena;
    const char *noteNames[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    const char *specialNames[4] = {"OFF", "REL", "===", "..."};

    CLAY(
        CLAY_ID("TrackerEditorWindow"),
        {
            .layout = {
                .sizing = {CLAY_SIZING_PERCENT(0.96f), CLAY_SIZING_PERCENT(0.90f)},
                .padding = {0, 0, 10, 0},
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
            .backgroundColor = CLAY_COLOR_PANEL_BG,
            .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
            .border = {.color = CLAY_COLOR_BORDER, .width = CLAY_BORDER_OUTSIDE(2) },
        }
    )
    {
        CLAY(
            CLAY_ID("TrackerEditorTitle"),
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                        .padding = {10, 10, 10, 10},
                        .childGap = 8,
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
        )
        {
            Clay_String title = ClayArena_FormatString(
                arena,
                "CH%d:%03d %s",
                self->editChannel + 1,
                self->editRow,
                self->cells[self->editRow][self->editChannel].text
            );
            CLAY_TEXT(title, CLAY_TEXT_CONFIG(titleCfg));
            CLAY(CLAY_ID("TrackerEditorTitleGrow"), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()}}}) {}
            CLAY(self->editorCloseButton.clayId, CLAY_THEME_BTN_DANGER)
            {
                CLAY_TEXT(CLAY_STRING("x"), CLAY_TEXT_CONFIG(buttonCfg));
            }
        }

        CLAY(
            CLAY_ID("TrackerEditorTabs"),
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                        .padding = {10, 10, 0, 0},
                        .childGap = 8,
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_BOTTOM},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
        )
        {
            Clay_ElementDeclaration tab = CLAY_THEME_BTN_PRIMARY;
            tab.backgroundColor = self->editorTab == 0 ? CLAY_COLOR_PANEL_SECTION : CLAY_COLOR_BTN_PRIMARY;
            tab.cornerRadius.bottomLeft = 0;
            tab.cornerRadius.bottomRight = 0;

            CLAY(self->editorNoteTabButton.clayId, tab)
            {
                CLAY_TEXT(CLAY_STRING("NOTE"), CLAY_TEXT_CONFIG(buttonCfg));
            }
            tab.backgroundColor = self->editorTab == 1 ? CLAY_COLOR_PANEL_SECTION : CLAY_COLOR_BTN_PRIMARY;

            CLAY(self->editorEffectsTabButton.clayId, tab)
            {
                CLAY_TEXT(CLAY_STRING("EFFECTS"), CLAY_TEXT_CONFIG(buttonCfg));
            }
        }
        CLAY(
            CLAY_ID("TrackerEditorWindowWrap"), {
                .layout = {
                    .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                    .padding = {10, 10, 10, 10},
                    .childGap = 8,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
                .backgroundColor = CLAY_COLOR_PANEL_SECTION,
                .cornerRadius = {0, 0, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
            }
        )
        {

                if (self->editorTab == 0)
                {
                    CLAY(
                        CLAY_ID("TrackerNoteControls"),
                        {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                                    .childGap = 8,
                                    .layoutDirection = CLAY_TOP_TO_BOTTOM},
                                }
                    )
                    {
                        CLAY(
                            CLAY_ID("TrackerInstrumentSelectorRow"),
                            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                                        .childGap = 8,
                                        .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_BOTTOM},
                                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
                        )
                        {

                            Clay_ElementDeclaration shortBtn = CLAY_THEME_BTN_BOX;
                            CLAY(self->instrumentPrevButton.clayId, shortBtn)
                            {
                                CLAY_TEXT(CLAY_STRING("<"), CLAY_TEXT_CONFIG(buttonCfg));
                            }
                            bool instrumentUsed = Tracker_InstrumentUsedInSong(self, self->editInstrument);
                            Clay_String name = ClayArena_FormatString(
                                arena,
                                "%02X %s",
                                self->editInstrument,
                                Tracker_InstrumentName(self, self->editInstrument)
                            );
                            Clay_Color instrumentTextColor = instrumentUsed ? CLAY_COLOR_TEXT_PRIMARY : Clay_Color{145, 151, 164, 255};

                            uint32_t instColorRgb = Tracker_InstrumentColorU32(self, self->editInstrument);
                            Clay_ElementDeclaration colorBtn = CLAY_THEME_BTN_PRIMARY;
                            colorBtn.layout.sizing.width = CLAY_SIZING_GROW();
                            colorBtn.backgroundColor = Tracker_ColorFromU32(instColorRgb, 255.0f);
                            Clay_TextElementConfig colorTextCfg = buttonCfg;
                            if (Tracker_ColorIsBright(instColorRgb))
                                colorTextCfg.textColor = {14, 16, 22, 255};
                            colorBtn.border.width = CLAY_BORDER_ALL(1);
                            colorBtn.border.color = instrumentUsed
                                ? Clay_Color{78, 92, 124, 255} 
                                : Clay_Color{68, 70, 80, 255};
                            CLAY(
                                self->instrumentNameButton.clayId,
                                colorBtn
                            )
                            {
                                CLAY_TEXT(name, CLAY_TEXT_CONFIG(colorTextCfg));
                            }
                            CLAY(self->instrumentNextButton.clayId, shortBtn)
                            {
                                CLAY_TEXT(CLAY_STRING(">"), CLAY_TEXT_CONFIG(buttonCfg));
                            }
                            Clay_ElementDeclaration instCheck = CLAY_THEME_BTN_BOX;
                            instCheck.backgroundColor = self->editInstrumentExplicit ? CLAY_COLOR_BTN_SUCCESS : CLAY_COLOR_BTN_DISABLED;
                            CLAY(self->instrumentExplicitButton.clayId, instCheck)
                            {
                                CLAY_TEXT(self->editInstrumentExplicit ? CLAY_STRING("✓") : CLAY_STRING(" "), CLAY_TEXT_CONFIG(buttonCfg));
                            }
                        }

                        CLAY(
                            CLAY_ID("TrackerVolumeSlider"),
                            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                                        .childGap = 8,
                                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
                        )
                        {
                            Clay_String vol = ClayArena_FormatString(arena, "VOL %02X", self->editVolume);
                            CLAY_TEXT(vol, CLAY_TEXT_CONFIG(bodyCfg));
                            CLAY(
                                CLAY_ID("TrackerVolumeTrack"),
                                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(18)},
                                            .layoutDirection = CLAY_LEFT_TO_RIGHT},
                                .backgroundColor = {28, 30, 42, 255},
                                .cornerRadius = {4, 4, 4, 4}}
                            )
                            {
                                CLAY(
                                    CLAY_ID("TrackerVolumeFill"),
                                    {.layout = {.sizing = {CLAY_SIZING_PERCENT((float)self->editVolume / 127.0f), CLAY_SIZING_GROW()}},
                                    .backgroundColor = {88, 170, 126, 255},
                                    .cornerRadius = {4, 4, 4, 4}}
                                    ) {}
                            }
                            Clay_ElementDeclaration volCheck = CLAY_THEME_BTN_BOX;
                            volCheck.backgroundColor = self->editVolumeExplicit ? CLAY_COLOR_BTN_SUCCESS : CLAY_COLOR_BTN_DISABLED;
                            CLAY(self->volumeExplicitButton.clayId, volCheck)
                            {
                                CLAY_TEXT(self->editVolumeExplicit ? CLAY_STRING("✓") : CLAY_STRING(" "), CLAY_TEXT_CONFIG(buttonCfg));
                            }
                        }
                    }

                    Clay_ElementData ed = Clay_GetElementData(
                        CLAY_IDI("TrackerOctaveWrapper", 1) // All octaves are same
                    );

                    if (ed.found) 
                    {
                        self->keyHeight = ed.boundingBox.height;
                    }

                    for (int octave = 1; octave <= 7; octave++)
                    {
                        // Outer horizontal wrapper – octave number column + key rows column
                        CLAY(
                            CLAY_IDI("TrackerOctaveWrapper", octave),
                            {.layout = {
                                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                .childGap = 4,
                                .layoutDirection = CLAY_LEFT_TO_RIGHT
                            }}
                        )
                        {
                            // Left column – octave number, vertically centred across both key rows
                            CLAY(
                                CLAY_IDI("TrackerOctaveLabel", octave),
                                {.layout =
                                    {.sizing = {CLAY_SIZING_FIXED(30), CLAY_SIZING_GROW()},
                                    .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}},
                                .backgroundColor = {45, 45, 65, 255}}
                            )
                            {
                                Clay_String octLabel = ClayArena_FormatString(arena, "%d", octave);
                                CLAY_TEXT(octLabel, CLAY_TEXT_CONFIG(bodyCfg));
                            }

                            // Right column – stacks the two key rows without vertical gap
                            CLAY(
                                CLAY_IDI("TrackerKeyRowsColumn", octave),
                                {.layout = {
                                    .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                    .childGap = 0, // no vertical gap between the two rows
                                    .layoutDirection = CLAY_TOP_TO_BOTTOM
                                }}
                            )
                            {
                                // ---------- Upper row: 12 equal semitone keys, no gaps, no bottom
                                // border/rounding ----------
                                CLAY(
                                    CLAY_IDI("TrackerOctaveKeysRow", octave),
                                    {.layout = {
                                        .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                        .childGap = 0, // keys touch each other
                                        .layoutDirection = CLAY_LEFT_TO_RIGHT
                                    }}
                                )
                                {
                                    for (int note = 0; note < 12; note++)
                                    {
                                        bool black = note == 1 || note == 3 || note == 6 || note == 8 ||
                                            note == 10;
                                        bool selected = self->editSpecial == 0 &&
                                            self->editOctave == octave && self->editNote == note;
                                        Clay_Color bg = selected ? (Clay_Color){78, 170, 126, 255}
                                            : black              ? (Clay_Color){28, 30, 42, 255}
                                                                : (Clay_Color){220, 224, 235, 255};
                                        uint16_t keyBorderWidth = selected ? 2 : 1;
                                        Clay_TextElementConfig keyText = bodyCfg;
                                        keyText.textColor = black || selected
                                            ? (Clay_Color){245, 245, 250, 255}
                                            : (Clay_Color){20, 20, 30, 255};

                                        uint16_t keyBottomBorder = black ? keyBorderWidth : 0;
                                        float keyBottomRadius = black ? 3.0f : 0.0f;
                                        float upperKeyHeight = self->keyHeight - (10 + 2); // tiny 2px gap added for safety
                                        CLAY(
                                            CLAY_IDI("TrackerKey", octave * 100 + note),
                                            {.layout =
                                                {.sizing = { CLAY_SIZING_GROW() , CLAY_SIZING_FIXED(upperKeyHeight)},
                                                .childAlignment =
                                                    {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}},
                                            .backgroundColor = bg,
                                            .cornerRadius = {3, 3, keyBottomRadius, keyBottomRadius}, // top corners rounded,
                                                                        // bottom corners square
                                            .border = {
                                                .color = selected ? (Clay_Color){235, 245, 255, 255}
                                                                : (Clay_Color){80, 80, 100, 255},
                                                .width = {
                                                    .left = keyBorderWidth,
                                                    .right = keyBorderWidth,
                                                    .top = keyBorderWidth,
                                                    .bottom = keyBottomBorder // no bottom border
                                                }
                                            }}
                                        )
                                        {
                                            Clay_String label = ClayArena_FormatString(
                                                arena, "%s%d", noteNames[note], octave
                                            );
                                            CLAY_TEXT(label, CLAY_TEXT_CONFIG(keyText));
                                        }
                                    }
                                }

                                // ---------- Lower row: 7 white keys, no gaps, no top border/rounding
                                // ----------
                                CLAY(
                                    CLAY_IDI("TrackerWhiteKeysRow", octave),
                                    {.layout = {
                                        .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                        .childGap = 0, // keys touch each other
                                        .layoutDirection = CLAY_LEFT_TO_RIGHT
                                    }}
                                )
                                {
                                    const int whiteNoteIndexes[7] = {
                                        0, 2, 4, 5, 7, 9, 11
                                    }; // C D E F G A B
                                    const float whiteWidths[7] = {
                                        1.5f, 2.0f, 1.5f, 1.5f, 2.0f, 2.0f, 1.5f
                                    };

                                    for (int w = 0; w < 7; w++)
                                    {
                                        int note = whiteNoteIndexes[w];
                                        float widthFraction = whiteWidths[w] / 12.0f;

                                        bool selected = self->editSpecial == 0 &&
                                            self->editOctave == octave && self->editNote == note;
                                        Clay_Color bg = selected ? (Clay_Color){78, 170, 126, 255}
                                                                : (Clay_Color){220, 224, 235, 255};
                                        uint16_t borderW = selected ? 2 : 1;
                                        Clay_TextElementConfig keyText = bodyCfg;
                                        keyText.textColor = selected ? (Clay_Color){245, 245, 250, 255}
                                                                    : (Clay_Color){20, 20, 30, 255};

                                        CLAY(
                                            CLAY_IDI("TrackerWhiteKey", octave * 100 + note),
                                            {.layout =
                                                {.sizing =
                                                    {CLAY_SIZING_PERCENT(widthFraction),
                                                    CLAY_SIZING_FIXED(10)},
                                                .childAlignment =
                                                    {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}},
                                            .backgroundColor = bg,
                                            .cornerRadius = {0, 0, 3, 3}, // top corners square, bottom
                                                                        // corners rounded
                                            .border = {
                                                .color = selected ? (Clay_Color){235, 245, 255, 255}
                                                                : (Clay_Color){80, 80, 100, 255},
                                                .width = {
                                                    .left = borderW,
                                                    .right = borderW,
                                                    .top = 0, // no top border
                                                    .bottom = borderW
                                                }
                                            }}
                                        )
                                        {
                                            Clay_String label = ClayArena_FormatString(
                                                arena, "%s%d", noteNames[note], octave
                                            );
                                            // CLAY_TEXT(label, CLAY_TEXT_CONFIG(keyText));
                                        }
                                    }
                                }
                            }
                        }
                    }

                    CLAY(
                        CLAY_ID("TrackerSpecialValues"),
                        {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                                    .childGap = 6,
                                    .layoutDirection = CLAY_LEFT_TO_RIGHT}}
                    )
                    {
                        for (int i = 0; i < 4; i++)
                        {
                            Clay_ElementDeclaration special = CLAY_THEME_BTN_PRIMARY;
                            special.backgroundColor = self->editSpecial == i + 1 ? CLAY_COLOR_BTN_ACTIVE : CLAY_COLOR_BTN_PRIMARY;
                            CLAY(CLAY_IDI("TrackerSpecial", i), special)
                            {
                                CLAY_TEXT(ClayArena_AllocString(arena, specialNames[i]), CLAY_TEXT_CONFIG(buttonCfg));
                            }
                        }
                        CLAY(CLAY_ID("TrackerSpecialDelete"), CLAY_THEME_BTN_DANGER)
                        {
                            CLAY_TEXT(CLAY_STRING("DEL"), CLAY_TEXT_CONFIG(buttonCfg));
                        }
                    }
                }
                else
                {
                    CLAY(CLAY_ID("TrackerEffectEditor"), CLAY_THEME_SECTION)
                    {
                        auto paramSlider = [&](const char *label, int paramValue, int minValue, int maxValue, int hardMax, bool inRange, Clay_ElementId barId, Clay_ElementId fillId) {
                            int denom = std::max(1, maxValue - minValue);
                            float t = (float)(paramValue - minValue) / (float)denom;
                            t = std::max(0.0f, std::min(1.0f, t));
                            Clay_Color fillColor = inRange ? (Clay_Color){120, 146, 214, 255} : (Clay_Color){226, 72, 88, 255};
                            Clay_String param = ClayArena_FormatString(arena, "%s %02X", label, paramValue);
                            CLAY(
                                CLAY_IDI("TrackerEffectParamTrack", barId.id),
                                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(60)},
                                            .childGap = 8,
                                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                            .layoutDirection = CLAY_LEFT_TO_RIGHT}}
                            )
                            {
                                CLAY(
                                    CLAY_IDI("TrackerEffectParamLabel", barId.id),
                                    {.layout = {.sizing = {CLAY_SIZING_FIXED(88), CLAY_SIZING_GROW()},
                                                .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER}}}
                                )
                                {
                                    CLAY_TEXT(param, CLAY_TEXT_CONFIG(bodyCfg));
                                }
                                CLAY(
                                    barId,
                                    {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(18)},
                                                .layoutDirection = CLAY_LEFT_TO_RIGHT},
                                    .backgroundColor = {28, 30, 42, 255},
                                    .cornerRadius = {4, 4, 4, 4}}
                                )
                                {
                                    CLAY(
                                        fillId,
                                        {.layout = {.sizing = {CLAY_SIZING_PERCENT(t), CLAY_SIZING_GROW()}},
                                        .backgroundColor = fillColor,
                                        .cornerRadius = {4, 4, 4, 4}}
                                    ) {}
                                }
                            }
                        };

                        int effectIdx = Tracker_SelectedEffectDefIndex(self);
                        const TrackerEffectDef *def = &TRACKER_EFFECT_DEFS[effectIdx];
                        uint8_t value = Tracker_SelectedEffectValue(self);
                        bool active = Tracker_SelectedEffectActive(self);
                        bool limitReached = !active && Tracker_ActiveEffectCount(self) >= TRACKER_CELL_ACTIVE_EFFECT_LIMIT;

                        CLAY(
                            CLAY_ID("TrackerEffectSelectorRow"),
                            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(60)},
                                        .padding = {0, 0, 4, 4},
                                        .childGap = 8,
                                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
                        )
                        {
                            CLAY(self->effectPrevButton.clayId, CLAY_THEME_BTN_BOX)
                            {
                                CLAY_TEXT(CLAY_STRING("<"), CLAY_TEXT_CONFIG(buttonCfg));
                            }
                            CLAY(
                                CLAY_ID("TrackerEffectTypeValue"),
                                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}},
                                .backgroundColor = active ? Clay_Color{35, 45, 65, 255} : Clay_Color{42, 43, 50, 255},
                                .cornerRadius = {CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD}}
                            )
                            {
                                Clay_String label = ClayArena_FormatString(arena, "%02X %s", def->code, def->name);
                                CLAY_TEXT(label, CLAY_TEXT_CONFIG(buttonCfg));
                            }
                            CLAY(self->effectNextButton.clayId, CLAY_THEME_BTN_BOX)
                            {
                                CLAY_TEXT(CLAY_STRING(">"), CLAY_TEXT_CONFIG(buttonCfg));
                            }
                            Clay_ElementDeclaration activeBox = CLAY_THEME_BTN_BOX;
                            activeBox.backgroundColor = active ? CLAY_COLOR_BTN_SUCCESS :
                                (limitReached ? Clay_Color{62, 62, 70, 255} : CLAY_COLOR_BTN_DISABLED);
                            CLAY(CLAY_ID("TrackerEffectActive"), activeBox)
                            {
                                CLAY_TEXT(active ? CLAY_STRING("✓") : CLAY_STRING(" "), CLAY_TEXT_CONFIG(buttonCfg));
                            }
                        }

                        Clay_String activeStatus = ClayArena_FormatString(
                            arena,
                            "%d/%d effects active",
                            Tracker_ActiveEffectCount(self),
                            TRACKER_CELL_ACTIVE_EFFECT_LIMIT
                        );
                        CLAY_TEXT(activeStatus, CLAY_TEXT_CONFIG(bodyCfg));

                        const char *desc = Tracker_EffectDescription(def->code);
                        if (desc && desc[0])
                        {
                            Clay_TextElementConfig descCfg = bodyCfg;
                            descCfg.fontSize = CLAY_FONT_SIZE_SM;
                            descCfg.textColor = {150, 154, 170, 255};
                            CLAY_TEXT(ClayArena_AllocString(arena, desc), CLAY_TEXT_CONFIG(descCfg));
                        }

                        if (def->paramCount > 0)
                        {
                            int hardMax = Tracker_EffectUsesNibbles(def) ? 15 : 255;
                            paramSlider(
                                def->paramA,
                                Tracker_EffectDisplayA(def, value),
                                def->minA,
                                def->maxA,
                                hardMax,
                                Tracker_EffectAInRange(def, value),
                                CLAY_ID("TrackerEffectParamABar"),
                                CLAY_ID("TrackerEffectParamAFill")
                            );
                        }
                        if (def->paramCount > 1)
                        {
                            paramSlider(
                                def->paramB,
                                Tracker_EffectDisplayB(def, value),
                                def->minB,
                                def->maxB,
                                15,
                                Tracker_EffectBInRange(def, value),
                                CLAY_ID("TrackerEffectParamBBar"),
                                CLAY_ID("TrackerEffectParamBFill")
                            );
                        }
                    }
                }
        }
    }
}

inline void Tracker_BuildInstrumentEditor(Tracker *self, Clayton *clayton)
{
    if (!self || !self->instrumentEditorOpen || !clayton) return;
    Clay_TextElementConfig titleCfg = CLAY_THEME_TEXT_TITLE;
    Clay_TextElementConfig buttonCfg = CLAY_THEME_TEXT_BUTTON;
    Clay_TextElementConfig bodyCfg = CLAY_THEME_TEXT_BODY;
    Clay_TextElementConfig mutedCfg = bodyCfg;
    mutedCfg.textColor = {150, 154, 170, 255};
    ClayArena *arena = &clayton->clayArena;
    xfm_patch_opn &patch = Tracker_EditablePatch(self);

    CLAY(
        CLAY_ID("TrackerInstrumentEditorWindow"),
        {
            .layout = {
                .sizing = {CLAY_SIZING_PERCENT(0.96f), CLAY_SIZING_PERCENT(0.85f)},
                .padding = {0, 0, 10, 0},
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
            .backgroundColor = CLAY_COLOR_PANEL_BG,
            .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
            .border = {.color = CLAY_COLOR_BORDER, .width = CLAY_BORDER_OUTSIDE(2) },
        }
    )
    {
        CLAY(
            CLAY_ID("TrackerInstrumentEditorTitle"),
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                    .padding = {10, 10, 10, 10},
                        .childGap = 8,
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
        )
        {
            Clay_String title = ClayArena_FormatString(
                arena,
                "Instrument %02X %s",
                self->editInstrument,
                Tracker_InstrumentName(self, self->editInstrument)
            );
            CLAY_TEXT(title, CLAY_TEXT_CONFIG(titleCfg));
            CLAY(CLAY_ID("TrackerInstrumentEditorGrow"), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()}}}) {}
            uint32_t instColorRgb = Tracker_InstrumentColorU32(self, self->editInstrument);
            Clay_ElementDeclaration colorBtn = CLAY_THEME_BTN_PRIMARY;
            colorBtn.layout.sizing.width = CLAY_SIZING_FIXED(60);
            colorBtn.backgroundColor = Tracker_ColorFromU32(instColorRgb, 255.0f);
            Clay_TextElementConfig colorTextCfg = buttonCfg;
            if (Tracker_ColorIsBright(instColorRgb))
                colorTextCfg.textColor = {14, 16, 22, 255};
            CLAY(self->instrumentColorButton.clayId, colorBtn)
            {
                CLAY_TEXT(CLAY_STRING("Color"), CLAY_TEXT_CONFIG(colorTextCfg));
            }
            CLAY(self->instrumentEditorCloseButton.clayId, CLAY_THEME_BTN_DANGER)
            {
                CLAY_TEXT(CLAY_STRING("x"), CLAY_TEXT_CONFIG(buttonCfg));
            }
        }

        CLAY(
            CLAY_ID("TrackerInstrumentTabs"),
            {.layout = {
                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT() },
                .padding = {10, 10, 0, 0},
                .childGap = 8,
                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_BOTTOM},
                .layoutDirection = CLAY_LEFT_TO_RIGHT}}
        )
        {
            Clay_ElementDeclaration patchTab = CLAY_THEME_BTN_PRIMARY;
            Clay_ElementDeclaration effectsTab = CLAY_THEME_BTN_PRIMARY;
            patchTab.cornerRadius.bottomLeft = 0;
            patchTab.cornerRadius.bottomRight = 0;
            effectsTab.cornerRadius.bottomLeft = 0;
            effectsTab.cornerRadius.bottomRight = 0;
            if (self->instrumentEditorTab == 0) patchTab.backgroundColor = CLAY_COLOR_PANEL_SECTION;
            if (self->instrumentEditorTab == 1) effectsTab.backgroundColor = CLAY_COLOR_PANEL_SECTION;
            CLAY(self->instrumentPatchTabButton.clayId, patchTab)
            {
                CLAY_TEXT(CLAY_STRING("Patch"), CLAY_TEXT_CONFIG(buttonCfg));
            }
            CLAY(self->instrumentEffectsTabButton.clayId, effectsTab)
            {
                CLAY_TEXT(CLAY_STRING("Macros"), CLAY_TEXT_CONFIG(buttonCfg));
            }
        }
        CLAY(
            CLAY_ID("TrackerInstrumentWindowWrap"), {
                .layout = {
                    .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                    .padding = {10, 10, 10, 10},
                    .childGap = 8,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
                .backgroundColor = CLAY_COLOR_PANEL_SECTION,
                .cornerRadius = {0, 0, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
            }
        )
        {
            if (self->instrumentEditorTab == 0)
            {
                CLAY(
                    CLAY_ID("TrackerInstrumentAlgoRow"),
                    {.layout = {
                         .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(60)},
                         .childGap = 8,
                         .layoutDirection = CLAY_LEFT_TO_RIGHT
                     }}
                )
                {
                    CLAY(self->instrumentAlgoPrevButton.clayId, CLAY_THEME_BTN_BOX)
                    {
                        CLAY_TEXT(CLAY_STRING("<"), CLAY_TEXT_CONFIG(buttonCfg));
                    }
                    Clay_String algo = ClayArena_FormatString(arena, "ALGO %d", patch.ALG);
                    CLAY(
                        CLAY_ID("TrackerInstrumentAlgoValue"),
                        {.layout =
                             {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                              .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}},
                         .backgroundColor = {35, 45, 65, 255},
                         .cornerRadius = {
                             CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD
                         }}
                    )
                    {
                        CLAY_TEXT(algo, CLAY_TEXT_CONFIG(buttonCfg));

                        Clay_ElementDeclaration algoPreview = {
                            .layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(48)}},
                            .image = {.imageData = &clayton->trackerAlgoImages[patch.ALG & 7]},
                            .border = {.color = {146, 220, 132, 255}, .width = CLAY_BORDER_ALL(0)}
                        };
                        CLAY(CLAY_ID("TrackerSelectedAlgoDiagram"), algoPreview)
                        {
                        }
                    }
                    CLAY(self->instrumentAlgoNextButton.clayId, CLAY_THEME_BTN_BOX)
                    {
                        CLAY_TEXT(CLAY_STRING(">"), CLAY_TEXT_CONFIG(buttonCfg));
                    }
                }

                auto renderSmallPreview =
                    [&](Clay_ElementId id, Gles3_ImageConfig *image, bool enabled)
                {
                    Clay_ElementDeclaration preview = {
                        .layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()}},
                        .backgroundColor = {11, 14, 20, 255},
                        .border = {
                            .color = enabled ? (Clay_Color){88, 116, 92, 255}
                                             : (Clay_Color){54, 60, 78, 255},
                            .width = CLAY_BORDER_ALL(1)
                        }
                    };
                    if (enabled)
                        preview.image.imageData = image;
                    CLAY(id, preview)
                    {
                    }
                };

                auto renderOperatorButton = [&](int opId)
                {
                    const xfm_patch_opn_operator &op = patch.op[opId];

                    CLAY(
                        self->operatorButtons[opId].clayId,
                        {.layout =
                             {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                              .padding = {8, 8, 8, 8},
                              .childGap = 6,
                              .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_TOP},
                              .layoutDirection = CLAY_TOP_TO_BOTTOM},
                         .backgroundColor = {11, 14, 20, 255},
                         .cornerRadius =
                             {CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD},
                         .border = {
                             .color = (Clay_Color){40, 46, 62, 255}, .width = CLAY_BORDER_ALL(1)
                         }}
                    )
                    {
                        CLAY(
                            CLAY_IDI("TrackerOperatorHeader", opId),
                            {.layout = {
                                 .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                                 .childGap = 6,
                                 .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                                 .layoutDirection = CLAY_LEFT_TO_RIGHT
                             }}
                        )
                        {
                            Clay_String text = ClayArena_FormatString(arena, "OP%d", opId + 1);
                            CLAY_TEXT(text, CLAY_TEXT_CONFIG(buttonCfg));
                        }

                        CLAY(
                            CLAY_IDI("TrackerOperatorHeaderEnve", opId),
                            {.layout = {
                                 .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                 .childGap = 6,
                                 .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                                 .layoutDirection = CLAY_LEFT_TO_RIGHT
                             }}
                        )
                        {
                            renderSmallPreview(
                                CLAY_IDI("TrackerOperatorEnvelopePreview", opId),
                                &clayton->trackerEnvelopeImages[opId],
                                true
                            );
                        }
                        CLAY(
                            CLAY_IDI("TrackerOperatorHeaderSsg", opId),
                            {.layout = {
                                 .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(30)},
                                 .childGap = 6,
                                 .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                                 .layoutDirection = CLAY_LEFT_TO_RIGHT
                             }}
                        )
                        {
                            int ssg = op.SSG;
                            renderSmallPreview(
                                CLAY_IDI("TrackerOperatorSsgPreview", opId),
                                ssg > 0
                                    ? &clayton->trackerSsgImages[std::max(0, std::min(7, ssg - 1))]
                                    : nullptr,
                                ssg > 0
                            );
                        }
                        auto stat = [&](const char *label, int value)
                        {
                            Clay_String line = ClayArena_FormatString(arena, "%s %d", label, value);
                            CLAY_TEXT(line, CLAY_TEXT_CONFIG(bodyCfg));
                        };

                        CLAY(
                            CLAY_IDI("TrackerOperatorStats", opId),
                            {.layout = {
                                 .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                 .childGap = 6,
                                 .layoutDirection = CLAY_LEFT_TO_RIGHT
                             }}
                        )
                        {
                            CLAY(
                                CLAY_IDI("TrackerOperatorStatsColA", opId),
                                {.layout = {
                                     .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                     .childGap = 2,
                                     .layoutDirection = CLAY_TOP_TO_BOTTOM
                                 }}
                            )
                            {
                                stat("TL", (int)op.TL);
                                stat("MUL", (int)op.MUL);
                                stat("DT", (int)op.DT);
                                stat("RS", (int)op.RS);
                                stat("AM", (int)op.AM);
                            }
                        }
                    }
                };

                auto slider = [&](const char *label,
                                  int value,
                                  int maxValue,
                                  Clay_ElementId barId,
                                  Clay_ElementId fillId)
                {
                    CLAY(
                        CLAY_IDI("TrackerInstrumentSliderRow", barId.id),
                        {.layout = {
                             .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(42)},
                             .childGap = 8,
                             .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                             .layoutDirection = CLAY_LEFT_TO_RIGHT
                         }}
                    )
                    {
                        Clay_String text = ClayArena_FormatString(arena, "%s %d", label, value);
                        CLAY(
                            CLAY_IDI("TrackerInstrumentSliderLabel", barId.id),
                            {.layout = {
                                 .sizing = {CLAY_SIZING_FIXED(70), CLAY_SIZING_GROW()},
                                 .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER}
                             }}
                        )
                        {
                            CLAY_TEXT(text, CLAY_TEXT_CONFIG(bodyCfg));
                        }
                        CLAY(
                            barId,
                            {.layout =
                                 {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(18)},
                                  .layoutDirection = CLAY_LEFT_TO_RIGHT},
                             .backgroundColor = {28, 30, 42, 255},
                             .cornerRadius = {4, 4, 4, 4}}
                        )
                        {
                            CLAY(
                                fillId,
                                {.layout =
                                     {.sizing =
                                          {CLAY_SIZING_PERCENT(
                                               maxValue > 0 ? (float)value / (float)maxValue : 0.0f
                                           ),
                                           CLAY_SIZING_GROW()}},
                                 .backgroundColor = {120, 146, 214, 255},
                                 .cornerRadius = {4, 4, 4, 4}}
                            )
                            {
                            }
                        }
                    }
                };
                slider(
                    "FB", patch.FB, 7, CLAY_ID("TrackerPatchFbBar"), CLAY_ID("TrackerPatchFbFill")
                );
                slider(
                    "AMS",
                    patch.AMS,
                    3,
                    CLAY_ID("TrackerPatchAmsBar"),
                    CLAY_ID("TrackerPatchAmsFill")
                );
                slider(
                    "FMS",
                    patch.FMS,
                    7,
                    CLAY_ID("TrackerPatchFmsBar"),
                    CLAY_ID("TrackerPatchFmsFill")
                );

                CLAY(
                    CLAY_ID("TrackerOperatorButtonGrid"),
                    {.layout = {
                         .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                         .childGap = 8,
                         .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_TOP},
                         .layoutDirection = CLAY_LEFT_TO_RIGHT
                     }}
                )
                {
                    for (int op = 0; op < 4; op++)
                    {
                        CLAY(
                            CLAY_IDI("TrackerOperatorButtonCol", op),
                            {.layout = {
                                 .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                 .layoutDirection = CLAY_TOP_TO_BOTTOM
                             }}
                        )
                        {
                            renderOperatorButton(op);
                        }
                    }
                }
            }
            else
            {
                int inst = std::max(0, std::min(255, self->editInstrument));
                XfmMacro &macro = Tracker_EditableMacro(self);
                Tracker_EnsureMacroUiLength(&macro);
                int target = std::max((int)XFM_MACRO_TL1, std::min(Tracker_MacroMaxTarget(), self->editMacroTarget));
                bool enabled = self->editMacroEnabled[inst][target];
                int enabledCount = Tracker_MacroEnabledCount(self);

                Clay_String label = ClayArena_FormatString(
                    arena,
                    "%d macros on",
                    enabledCount
                );
                CLAY_TEXT(label, CLAY_TEXT_CONFIG(buttonCfg));

                CLAY(
                    CLAY_ID("TrackerMacroTargetRow"),
                    {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                                .childGap = 8,
                                .layoutDirection = CLAY_LEFT_TO_RIGHT}}
                )
                {
                    CLAY(self->macroTargetPrevButton.clayId, CLAY_THEME_BTN_BOX)
                    {
                        CLAY_TEXT(CLAY_STRING("<"), CLAY_TEXT_CONFIG(buttonCfg));
                    }
                    CLAY(
                        CLAY_ID("TrackerMacroTargetValue"),
                        {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(60)},
                                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}},
                        .backgroundColor = {35, 45, 65, 255},
                        .cornerRadius = {CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD}}
                    )
                    {
                        Clay_String label = ClayArena_FormatString(
                            arena,
                            "%02X %s",
                            target,
                            Tracker_MacroTargetName(target)
                        );
                        CLAY_TEXT(label, CLAY_TEXT_CONFIG(buttonCfg));
                    }
                    CLAY(self->macroTargetNextButton.clayId, CLAY_THEME_BTN_BOX)
                    {
                        CLAY_TEXT(CLAY_STRING(">"), CLAY_TEXT_CONFIG(buttonCfg));
                    }
                    Clay_ElementDeclaration enableCheck = CLAY_THEME_BTN_BOX;
                    // enableCheck.layout.sizing.width = CLAY_SIZING_FIXED(42);
                    enableCheck.backgroundColor = enabled ? CLAY_COLOR_BTN_SUCCESS : CLAY_COLOR_BTN_DISABLED;
                    CLAY(self->macroEnableButton.clayId, enableCheck)
                    {
                        CLAY_TEXT(enabled ? CLAY_STRING("✓") : CLAY_STRING(" "), CLAY_TEXT_CONFIG(buttonCfg));
                    }
                }

                int valueMin = -64;
                int valueMax = 127;
                if (target >= XFM_MACRO_TL1 && target <= XFM_MACRO_TL4) valueMin = 0, valueMax = 127;
                else if (target >= XFM_MACRO_MUL1 && target <= XFM_MACRO_MUL4) valueMin = 0, valueMax = 15;
                else if (target >= XFM_MACRO_DT1 && target <= XFM_MACRO_DT4) valueMin = -3, valueMax = 3;
                else if (target >= XFM_MACRO_AR1 && target <= XFM_MACRO_SR4) valueMin = 0, valueMax = 31;
                else if (target >= XFM_MACRO_SL1 && target <= XFM_MACRO_RR4) valueMin = 0, valueMax = 15;
                else if (target >= XFM_MACRO_SSG1 && target <= XFM_MACRO_SSG4) valueMin = 0, valueMax = 8;
                else if (target == XFM_MACRO_FB) valueMin = 0, valueMax = 7;
                else if (target == XFM_MACRO_ARP) valueMin = -12, valueMax = 12;
                bool signedMacro = valueMin < 0 && valueMax > 0;
                float zeroT = signedMacro ? (float)valueMax / (float)(valueMax - valueMin) : 1.0f;
                zeroT = std::max(0.0f, std::min(1.0f, zeroT));
                Clay_Color graphBg = enabled ? (Clay_Color){18, 20, 30, 255} : (Clay_Color){42, 42, 46, 255};
                Clay_Color posColor = enabled ? (Clay_Color){96, 170, 236, 255} : (Clay_Color){92, 92, 96, 255};
                Clay_Color negColor = enabled ? (Clay_Color){232, 114, 118, 255} : (Clay_Color){82, 82, 86, 255};
                Clay_TextElementConfig tinyCfg = bodyCfg;
                tinyCfg.fontSize = CLAY_FONT_SIZE_SM;
                tinyCfg.fontId = CLAY_FONT_MONO;

                float macroColumnWidth = self->macroViewportWidth > 1.0f ? self->macroViewportWidth / (float)TRACKER_MACRO_VISIBLE_STEPS : 0.0f;
                float macroOffsetX = -self->macroViewAnimatedFirst * macroColumnWidth;
                auto renderMacroBelt = [&](const char *clipId, const char *beltId, float height, bool graph, bool reset, bool numbers) {
                    CLAY(
                        CLAY_SID(ClayArena_AllocString(arena, clipId)),
                        {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(height)}},
                        .backgroundColor = graph ? graphBg : (Clay_Color){0, 0, 0, 0},
                        .clip = {.horizontal = true, .vertical = false, .childOffset = {macroOffsetX, 0}},
                        .border = graph ? (Clay_BorderElementConfig){.color = {78, 84, 106, 255}, .width = CLAY_BORDER_ALL(1)} : (Clay_BorderElementConfig){}}
                    )
                    {
                        Clay_BoundingBox bb = Clay_GetElementData(CLAY_ID("TrackerMacroGraphClip")).boundingBox;
                        if (graph && bb.width > 1.0f) self->macroViewportWidth = bb.width;

float colWidth = macroColumnWidth > 0.0f ? macroColumnWidth : 10.0f; // fallback for first frame
float beltWidth = colWidth * (float)TRACKER_MACRO_UI_STEPS;
                        CLAY(
    CLAY_SID(ClayArena_AllocString(arena, beltId)),
    {.layout = {.sizing = {CLAY_SIZING_FIXED(beltWidth), CLAY_SIZING_GROW()},
                .padding = graph ? (Clay_Padding){3, 3, 3, 3} : (Clay_Padding){0, 0, 0, 0},

                .childGap = 1,
                .layoutDirection = CLAY_LEFT_TO_RIGHT}

            }
)
                        {
                            for (int i = 0; i < TRACKER_MACRO_UI_STEPS; i++)
                            {
                                if (reset)
                                {
                                    bool isRestorable = false;
                                    {
                                        int v = std::max(valueMin, std::min(valueMax, (int)macro.values[i]));
                                        macro.values[i] = (int16_t)v;
                                        float valueT = valueMax > valueMin ? ((float)valueMax - (float)v) / (float)(valueMax - valueMin) : 1.0f;
                                        valueT = std::max(0.0f, std::min(1.0f, valueT));
                                        float posFill = signedMacro ? std::max(0.0f, zeroT - valueT) / std::max(0.001f, zeroT) : 1.0f - valueT;
                                        float negFill = signedMacro ? std::max(0.0f, valueT - zeroT) / std::max(0.001f, 1.0f - zeroT) : 0.0f;
                                        if (posFill > 0.001f) {
                                            isRestorable = true;
                                        }
                                        if (negFill > 0.001f) {
                                            isRestorable = true;
                                        }
                                    }
                                    CLAY(
                                        CLAY_IDI("TrackerMacroReset", i),
                                        {.layout = {.sizing = {CLAY_SIZING_PERCENT(1.0f / (float)TRACKER_MACRO_UI_STEPS), CLAY_SIZING_GROW()},
                                                    .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}},
                                        .backgroundColor = enabled ? (isRestorable ?  (Clay_Color){236, 40, 52, 255} : (Clay_Color){36, 40, 52, 255}) : (Clay_Color){48, 48, 52, 255},
                                        .cornerRadius = {2, 2, 2, 2}}
                                    )
                                    {
                                        Clay_String text = ClayArena_FormatString(arena, "%d", i);
                                        CLAY_TEXT(text, CLAY_TEXT_CONFIG(tinyCfg));
                                    }
                                }
                                else if (graph)
                                {
                                    int v = std::max(valueMin, std::min(valueMax, (int)macro.values[i]));
                                    macro.values[i] = (int16_t)v;
                                    float valueT = valueMax > valueMin ? ((float)valueMax - (float)v) / (float)(valueMax - valueMin) : 1.0f;
                                    valueT = std::max(0.0f, std::min(1.0f, valueT));
                                    float posFill = signedMacro ? std::max(0.0f, zeroT - valueT) / std::max(0.001f, zeroT) : 1.0f - valueT;
                                    float negFill = signedMacro ? std::max(0.0f, valueT - zeroT) / std::max(0.001f, 1.0f - zeroT) : 0.0f;
                                    CLAY(
                                        CLAY_IDI("TrackerMacroBarColumn", i),
                                        {.layout = {.sizing = {CLAY_SIZING_PERCENT(1.0f / (float)TRACKER_MACRO_UI_STEPS), CLAY_SIZING_GROW()},
                                                    .childGap = 1,
                                                    .layoutDirection = CLAY_TOP_TO_BOTTOM}}
                                    )
                                    {
                                        CLAY(
                                            CLAY_IDI("TrackerMacroBarTop", i),
                                            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_PERCENT(zeroT)},
                                                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_BOTTOM},
                                                        .layoutDirection = CLAY_TOP_TO_BOTTOM},
                                            .backgroundColor = signedMacro ? (Clay_Color){28, 31, 43, 255} : graphBg}
                                        )
                                        {
                                            if (posFill < 1.0f)
                                                CLAY(CLAY_IDI("TrackerMacroBarPosSpace", i), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_PERCENT(1.0f - posFill)}}}) {}
                                            if (posFill > 0.0f)
                                                CLAY(CLAY_IDI("TrackerMacroBarPos", i), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_PERCENT(posFill)}}, .backgroundColor = posColor}) {}
                                        }
                                        CLAY(
                                            CLAY_IDI("TrackerMacroBarBottom", i),
                                            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_PERCENT(1.0f - zeroT)},
                                                        .layoutDirection = CLAY_TOP_TO_BOTTOM},
                                            .backgroundColor = signedMacro ? (Clay_Color){32, 28, 34, 255} : graphBg}
                                        )
                                        {
                                            if (negFill > 0.0f)
                                                CLAY(CLAY_IDI("TrackerMacroBarNeg", i), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_PERCENT(negFill)}}, .backgroundColor = negColor}) {}
                                            if (negFill < 1.0f)
                                                CLAY(CLAY_IDI("TrackerMacroBarNegSpace", i), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_PERCENT(1.0f - negFill)}}}) {}
                                        }
                                    }
                                }
                                else if (numbers)
                                {
                                    bool inLoopRange = macro.has_loop &&
                                                    i >= (int)macro.loop_start &&
                                                    (macro.release_start == 0xFF || i < (int)macro.release_start);
                                    bool inReleaseRange = macro.release_start != 0xFF && i >= (int)macro.release_start;
                                    Clay_Color numberBg = inLoopRange ? (Clay_Color){40, 90, 72, 255} :
                                                        inReleaseRange ? (Clay_Color){84, 54, 42, 255} :
                                                        (Clay_Color){26, 28, 38, 255};
                                    CLAY(
                                        CLAY_IDI("TrackerMacroValueNumber", i),
                                        {.layout = {.sizing = {CLAY_SIZING_PERCENT(1.0f / (float)TRACKER_MACRO_UI_STEPS), CLAY_SIZING_GROW()},
                                                    .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_TOP}},
                                        .backgroundColor = numberBg,
                                        .cornerRadius = {2, 2, 2, 2}}
                                    )
                                    {
                                        Clay_String text = ClayArena_FormatString(arena, "%d", (int)macro.values[i]);
                                        CLAY_TEXT(text, CLAY_TEXT_CONFIG(tinyCfg));
                                    }
                                }
                            }
                        }
                    }
                };

                CLAY(
                    CLAY_ID("TrackerMacroViewportRow"),
                    {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                .childGap = 4,

                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                .layoutDirection = CLAY_LEFT_TO_RIGHT}}
                )
                {
                    Clay_ElementDeclaration macroScrollPrev = CLAY_THEME_BTN_PRIMARY;
                    Clay_ElementDeclaration macroScrollNext = CLAY_THEME_BTN_PRIMARY;
                    macroScrollPrev.layout.sizing.width = CLAY_SIZING_PERCENT(0.1f);
                    macroScrollNext.layout.sizing.width = CLAY_SIZING_PERCENT(0.1f);
                    CLAY(self->macroScrollPrevButton.clayId, macroScrollPrev)
                    {
                        CLAY_TEXT(CLAY_STRING("<"), CLAY_TEXT_CONFIG(buttonCfg));
                    }
                    // CLAY(
                    //     CLAY_ID("TrackerMacroViewportStack"),
                    //     {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(250)},
                    //                 .layoutDirection = CLAY_TOP_TO_BOTTOM}}
                    // )
CLAY(
    CLAY_ID("TrackerMacroViewportStack"),
    {.layout = {
        .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
        .layoutDirection = CLAY_TOP_TO_BOTTOM,
    },
    

    },
)
                    {

                        uint16_t beltHeight;
                        Clay_ElementData ed = Clay_GetElementData( CLAY_ID("TrackerMacroViewportStack"));

                        if (ed.found) 
                        {
                            beltHeight = ed.boundingBox.height;
                        }

                        renderMacroBelt("TrackerMacroResetClip", "TrackerMacroResetBelt", 32, false, true, false);
                        renderMacroBelt("TrackerMacroGraphClip", "TrackerMacroGraphBelt", beltHeight - 32 - 32 - 20 , true, false, false);
                        renderMacroBelt("TrackerMacroNumbersClip", "TrackerMacroNumbersBelt", 32, false, false, true);
                    }
                    CLAY(self->macroScrollNextButton.clayId, macroScrollNext)
                    {
                        CLAY_TEXT(CLAY_STRING(">"), CLAY_TEXT_CONFIG(buttonCfg));
                    }
                }

                CLAY(
                    CLAY_ID("TrackerMacroFlagsRow"),
                    {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                                .childGap = 8,
                                .layoutDirection = CLAY_LEFT_TO_RIGHT}}
                )
                {
                    Clay_ElementDeclaration loopBtn = CLAY_THEME_BTN_PRIMARY;
                    Clay_ElementDeclaration releaseBtn = CLAY_THEME_BTN_PRIMARY;
                    if (macro.has_loop) loopBtn.backgroundColor = CLAY_COLOR_BTN_SUCCESS;
                    if (macro.release_start != 0xFF) releaseBtn.backgroundColor = CLAY_COLOR_BTN_SUCCESS;
                    CLAY(self->macroLoopButton.clayId, loopBtn)
                    {
                        Clay_String text = macro.has_loop ? ClayArena_FormatString(arena, "Loop %02d", macro.loop_start + 1) : CLAY_STRING("Loop off");
                        CLAY_TEXT(text, CLAY_TEXT_CONFIG(buttonCfg));
                    }
                    CLAY(self->macroReleaseButton.clayId, releaseBtn)
                    {
                        Clay_String text = macro.release_start != 0xFF ? ClayArena_FormatString(arena, "Rel %02d", macro.release_start + 1) : CLAY_STRING("Rel off");
                        CLAY_TEXT(text, CLAY_TEXT_CONFIG(buttonCfg));
                    }
                }
            }
        }
    }
}

inline void Tracker_BuildInstrumentColorWindow(Tracker *self, Clayton *clayton)
{
    if (!self || !self->instrumentColorWindowOpen || !clayton) return;
    ClayArena *arena = &clayton->clayArena;
    Clay_TextElementConfig titleCfg = CLAY_THEME_TEXT_TITLE;
    Clay_TextElementConfig buttonCfg = CLAY_THEME_TEXT_BUTTON;
    int inst = std::max(0, std::min(255, self->editInstrument));
    uint32_t current = Tracker_InstrumentColorU32(self, inst);
    Clay_BoundingBox portraitBox = Clay_GetElementData(CLAY_ID("Portrait area")).boundingBox;
    float colorGridSize = portraitBox.width > 1.0f ? portraitBox.width * 0.72f : 300.0f;
    colorGridSize = std::max(240.0f, std::min(360.0f, colorGridSize));
    const uint16_t colorGap = 6;
    const float colorRowHeight = (colorGridSize - (float)colorGap * 7.0f) / 8.0f;

    CLAY(CLAY_ID("TrackerInstrumentColorWindow"), CLAY_THEME_WINDOW_PANEL)
    {
        CLAY(
            CLAY_ID("TrackerInstrumentColorTitleRow"),
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                        .childGap = 8,
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
        )
        {
            Clay_String title = ClayArena_FormatString(arena, "Color %02X %s", inst, Tracker_InstrumentName(self, inst));
            CLAY_TEXT(title, CLAY_TEXT_CONFIG(titleCfg));
            CLAY(CLAY_ID("TrackerInstrumentColorGrow"), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()}}}) {}
            CLAY(self->instrumentColorCloseButton.clayId, CLAY_THEME_BTN_DANGER)
            {
                CLAY_TEXT(CLAY_STRING("x"), CLAY_TEXT_CONFIG(buttonCfg));
            }
        }
        CLAY(
            CLAY_ID("TrackerInstrumentColorGrid"),
            {.layout = {.sizing = {CLAY_SIZING_FIXED(colorGridSize), CLAY_SIZING_FIXED(colorGridSize)},
                        .childGap = colorGap,
                        .layoutDirection = CLAY_TOP_TO_BOTTOM}}
        )
        {
            for (int row = 0; row < 8; row++)
            {
                CLAY(
                    CLAY_IDI("TrackerInstrumentColorRow", row),
                    {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(colorRowHeight)},
                                .childGap = colorGap,
                                .layoutDirection = CLAY_LEFT_TO_RIGHT}}
                )
                {
                    for (int col = 0; col < 8; col++)
                    {
                        int idx = row * 8 + col;
                        uint32_t rgb = TRACKER_INSTRUMENT_COLOR_PALETTE[idx];
                        bool selected = (rgb & 0xFFFFFFu) == (current & 0xFFFFFFu);
                        Clay_BorderElementConfig border = selected ?
                            (Clay_BorderElementConfig){.color = {255, 255, 255, 255}, .width = CLAY_BORDER_ALL(3)} :
                            (Clay_BorderElementConfig){.color = {34, 36, 46, 255}, .width = CLAY_BORDER_ALL(1)};
                        Clay_ElementDeclaration swatch = {
                            .layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                       .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}},
                            .backgroundColor = Tracker_ColorFromU32(rgb, 255.0f),
                            .cornerRadius = {5, 5, 5, 5},
                            .border = border
                        };
                        CLAY(CLAY_IDI("TrackerInstrumentColorSwatch", idx), swatch)
                        {
                            if (selected)
                            {
                                Clay_TextElementConfig selectedCfg = buttonCfg;
                                selectedCfg.textColor = Tracker_ColorIsBright(rgb) ? (Clay_Color){14, 16, 22, 255} : (Clay_Color){255, 255, 255, 255};
                                CLAY_TEXT(CLAY_STRING("✓"), CLAY_TEXT_CONFIG(selectedCfg));
                            }
                        }
                    }
                }
            }
        }
    }
}

inline void Tracker_BuildOperatorEditor(Tracker *self, Clayton *clayton)
{
    if (!self || !self->operatorEditorOpen || !clayton) return;
    Clay_TextElementConfig titleCfg = CLAY_THEME_TEXT_TITLE;
    Clay_TextElementConfig buttonCfg = CLAY_THEME_TEXT_BUTTON;
    Clay_TextElementConfig bodyCfg = CLAY_THEME_TEXT_BODY;
    Clay_TextElementConfig mutedCfg = bodyCfg;
    mutedCfg.textColor = {150, 154, 170, 255};
    ClayArena *arena = &clayton->clayArena;
    xfm_patch_opn &patch = Tracker_EditablePatch(self);
    int opIndex = std::max(0, std::min(3, self->editOperator));
    xfm_patch_opn_operator &op = patch.op[opIndex];

    Clay_ElementDeclaration opWin = CLAY_THEME_WINDOW_PANEL;
    opWin.layout.padding = {10, 10, 10, 10};
    opWin.layout.sizing =  {CLAY_SIZING_PERCENT(0.9f), CLAY_SIZING_FIT()};
    CLAY(CLAY_ID("TrackerOperatorEditorWindow"), opWin)
    {
        CLAY(
            CLAY_ID("TrackerOperatorEditorTitle"),
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},

                        .childGap = 8,
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
        )
        {
            Clay_String title = ClayArena_FormatString(
                arena,
                "Inst %02X %s OP%d",
                self->editInstrument,
                Tracker_InstrumentName(self, self->editInstrument),
                opIndex + 1
            );
            CLAY_TEXT(title, CLAY_TEXT_CONFIG(titleCfg));
            CLAY(CLAY_ID("TrackerOperatorEditorGrow"), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()}}}) {}

            Clay_ElementDeclaration amBtn = CLAY_THEME_BTN_BOX;
            if (op.AM) amBtn.backgroundColor = CLAY_COLOR_BTN_SUCCESS;
            CLAY(self->operatorAmButton.clayId, amBtn)
            {
                CLAY_TEXT(CLAY_STRING("AM"), CLAY_TEXT_CONFIG(buttonCfg));
            }
            CLAY(self->operatorEditorCloseButton.clayId, CLAY_THEME_BTN_DANGER)
            {
                CLAY_TEXT(CLAY_STRING("x"), CLAY_TEXT_CONFIG(buttonCfg));
            }
        }

        auto slider = [&](const char *label, int value, int minValue, int maxValue, Clay_ElementId barId, Clay_ElementId fillId) {
            float t = maxValue > minValue ? (float)(value - minValue) / (float)(maxValue - minValue) : 0.0f;
            CLAY(
                CLAY_IDI("TrackerOperatorSliderRow", barId.id),
                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(35)},
                            .childGap = 8,
                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                            .layoutDirection = CLAY_LEFT_TO_RIGHT}}
            )
            {
                Clay_String text = ClayArena_FormatString(arena, "%s %d", label, value);
                CLAY(
                    CLAY_IDI("TrackerOperatorSliderLabel", barId.id),
                    {.layout = {.sizing = {CLAY_SIZING_FIXED(78), CLAY_SIZING_GROW()},
                                .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER}}}
                )
                {
                    CLAY_TEXT(text, CLAY_TEXT_CONFIG(bodyCfg));
                }
                CLAY(
                    barId,
                    {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(16)},
                                .layoutDirection = CLAY_LEFT_TO_RIGHT},
                     .backgroundColor = {28, 30, 42, 255},
                     .cornerRadius = {4, 4, 4, 4}}
                )
                {
                    CLAY(
                        fillId,
                        {.layout = {.sizing = {CLAY_SIZING_PERCENT(std::max(0.0f, std::min(1.0f, t))), CLAY_SIZING_GROW()}},
                         .backgroundColor = {120, 146, 214, 255},
                         .cornerRadius = {4, 4, 4, 4}}
                    ) {}
                }
            }
        };

        slider("TL", op.TL, 0, 127, CLAY_ID("TrackerOpTlBar"), CLAY_ID("TrackerOpTlFill"));

        CLAY(
            CLAY_ID("TrackerOperatorENVEL"),
            {.layout = {
                 .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(120)},
                 .childGap = 8,
                 .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                 .layoutDirection = CLAY_LEFT_TO_RIGHT
             }}
        )
        {
            Clay_String text = ClayArena_FormatString(arena, "ENV.");
            CLAY(
                CLAY_ID("TrackerOperatorEnvelopeLabel"),
                {.layout = {
                     .sizing = {CLAY_SIZING_FIXED(78), CLAY_SIZING_GROW()},
                     .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER}
                 }}
            )
            {
                CLAY_TEXT(text, CLAY_TEXT_CONFIG(bodyCfg));
            }
            Clay_ElementDeclaration envelopePreview = {
                .layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()}},
                .aspectRatio = {.aspectRatio = 1.5f * 16.0f / 9.0f},
                .image = {.imageData = &clayton->trackerEnvelopeImages[opIndex]},
                .border = {.color = {146, 220, 132, 255}, .width = CLAY_BORDER_ALL(1)}
            };
            CLAY(CLAY_ID("TrackerCurrentOperatorEnvelope"), envelopePreview)
            {
            }
        }

        slider("AR", op.AR, 0, 31, CLAY_ID("TrackerOpArBar"), CLAY_ID("TrackerOpArFill"));
        slider("DR", op.DR, 0, 31, CLAY_ID("TrackerOpDrBar"), CLAY_ID("TrackerOpDrFill"));
        slider("SL", op.SL, 0, 15, CLAY_ID("TrackerOpSlBar"), CLAY_ID("TrackerOpSlFill"));
        slider("SR", op.SR, 0, 31, CLAY_ID("TrackerOpSrBar"), CLAY_ID("TrackerOpSrFill"));
        slider("RR", op.RR, 0, 15, CLAY_ID("TrackerOpRrBar"), CLAY_ID("TrackerOpRrFill"));

        
            // float t = maxValue > minValue ? (float)(value - minValue) / (float)(maxValue - minValue) : 0.0f;
            CLAY(
                CLAY_ID("TrackerOperatorSSGROWWW"),
                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(60)},
                            .childGap = 8,
                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                            .layoutDirection = CLAY_LEFT_TO_RIGHT}}
            )
            {
                Clay_String text = ClayArena_FormatString(arena, "SSG-EG");
                CLAY(
                    CLAY_ID("TrackerOperatorSsgEgLabel"),
                    {.layout = {.sizing = {CLAY_SIZING_FIXED(78), CLAY_SIZING_GROW()},
                                .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER}}}
                )
                {
                    CLAY_TEXT(text, CLAY_TEXT_CONFIG(bodyCfg));
                }



        CLAY(
            CLAY_ID("TrackerOperatorSSDEGToggleRow"),
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                        .childGap = 8,
                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
        )
        {
            CLAY(self->operatorSsgPrevButton.clayId, CLAY_THEME_BTN_BOX)
            {
                CLAY_TEXT(CLAY_STRING("<"), CLAY_TEXT_CONFIG(buttonCfg));
            }
            CLAY(
                CLAY_ID("TrackerOperatorSsgValue"),
                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(60)},
                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}},
                 .backgroundColor = {35, 45, 65, 255},
                 .cornerRadius = {CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD}}
            )
            {
                Clay_String text = op.SSG == 0 ? CLAY_STRING("NO SSG-EG") : ClayArena_FormatString(arena, "SSG-EG %d", op.SSG);
                CLAY_TEXT(text, CLAY_TEXT_CONFIG(buttonCfg));
                Clay_ElementDeclaration ssgPreview = {
                    .layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(48)}},
                    .backgroundColor = {11, 14, 20, 255},
                    .border = {.color = op.SSG > 0 ? (Clay_Color){146, 220, 132, 255} : (Clay_Color){54, 60, 78, 255},
                            .width = CLAY_BORDER_ALL(1)}
                };
                if (op.SSG > 0) ssgPreview.image.imageData = &clayton->trackerSsgImages[std::max(0, std::min(7, (int)op.SSG - 1))];
                CLAY(CLAY_ID("TrackerCurrentOperatorSsg"), ssgPreview) {}
            }
            CLAY(self->operatorSsgNextButton.clayId, CLAY_THEME_BTN_BOX)
            {
                CLAY_TEXT(CLAY_STRING(">"), CLAY_TEXT_CONFIG(buttonCfg));
            }
        }
    }
        slider("MUL", op.MUL, 0, 15, CLAY_ID("TrackerOpMulBar"), CLAY_ID("TrackerOpMulFill"));
        slider("DT", op.DT, -3, 3, CLAY_ID("TrackerOpDtBar"), CLAY_ID("TrackerOpDtFill"));
        slider("RS", op.RS, 0, 3, CLAY_ID("TrackerOpRsBar"), CLAY_ID("TrackerOpRsFill"));

    }
}

inline void Tracker_BuildInstrumentsWindow(Tracker *self, Clayton *clayton)
{
    if (!self || !self->instrumentsWindowOpen || !clayton) return;

    Clay_TextElementConfig titleCfg = CLAY_THEME_TEXT_TITLE;
    Clay_TextElementConfig buttonCfg = CLAY_THEME_TEXT_BUTTON;
    Clay_TextElementConfig bodyCfg = CLAY_THEME_TEXT_BODY;
    Clay_TextElementConfig mutedCfg = bodyCfg;
    mutedCfg.textColor = {150, 154, 170, 255};
    ClayArena *arena = &clayton->clayArena;

    const float headerH = 58.0f;
    const float footerH = 72.0f;
    Clay_BoundingBox stackBox = Clay_GetElementData(CLAY_ID("WindowStackViewport")).boundingBox;
    const float windowH = stackBox.height > 0.0f ? stackBox.height * 0.88f : 620.0f;
    const float viewportH = std::max(120.0f, windowH - headerH - footerH - 36.0f);
    float maxScroll = Tracker_InstrumentsMaxScroll(self);
    self->instrumentsViewportHeight = viewportH;

    CLAY(CLAY_ID("TrackerInstrumentsWindow"), CLAY_THEME_WINDOW_PANEL)
    {
        CLAY(
            CLAY_ID("TrackerInstrumentsTitleRow"),
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(headerH)},
                        .childGap = 8,
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
        )
        {
            CLAY(CLAY_ID("TrackerInstrumentsTitle"), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                                                  .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER}}})
            {
                CLAY_TEXT(CLAY_STRING("Instruments"), CLAY_TEXT_CONFIG(titleCfg));
            }
            CLAY(self->instrumentsCloseButton.clayId, CLAY_THEME_BTN_DANGER)
            {
                CLAY_TEXT(CLAY_STRING("x"), CLAY_TEXT_CONFIG(buttonCfg));
            }
        }

        CLAY(
            CLAY_ID("TrackerInstrumentsContentRow"),
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(viewportH)},
                        .childGap = 0,
                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
        )
        {
            CLAY(
                CLAY_ID("TrackerInstrumentsViewport"),
                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                            .layoutDirection = CLAY_TOP_TO_BOTTOM},
                .backgroundColor = {18, 20, 30, 255},
                 .cornerRadius = {6, 0, 0, 6},
                 .clip = {.vertical = true, .childOffset = {0, -self->instrumentsScrollY}},
                 .border = {.color = {70, 76, 100, 255}, .width = CLAY_BORDER_ALL(1)}}
            )
            {
                Clay_BoundingBox bb = Clay_GetElementData(CLAY_ID("TrackerInstrumentsViewport")).boundingBox;
                self->instrumentsViewportHeight = bb.height > 1.0f ? bb.height : viewportH;
                CLAY(
                    CLAY_ID("TrackerInstrumentsList"),
                    {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                                .layoutDirection = CLAY_TOP_TO_BOTTOM}}
                )
                {
                    for (int inst = 0; inst < 256; inst++)
                    {
                        if (!self->availableInstruments[inst])
                            continue;

                        bool used = Tracker_InstrumentUsedInSong(self, inst);
                        uint32_t instColorRgb = Tracker_InstrumentColorU32(self, inst);
                        Clay_Color rowBg = Tracker_ColorFromU32(instColorRgb, 255.0f);
                        Clay_TextElementConfig rowText = used ? bodyCfg : mutedCfg;
                        if (Tracker_ColorIsBright(instColorRgb))
                            rowText.textColor = {14, 16, 22, 255};
                        else
                            rowText.textColor = {255, 255, 255, 255};
                        Clay_ElementDeclaration disabled = CLAY_THEME_BTN_PRIMARY;
                        disabled.backgroundColor = CLAY_COLOR_BTN_DISABLED;
                        Clay_ElementDeclaration smallBtn = CLAY_THEME_BTN_PRIMARY;
                        smallBtn.layout.sizing.width = CLAY_SIZING_FIXED(48);
                        Clay_ElementDeclaration disabledSmall = disabled;
                        disabledSmall.layout.sizing.width = CLAY_SIZING_FIXED(48);
                        Clay_ElementDeclaration rowDecl = {
                            .layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                                       .padding = {6, 6, 4, 4},
                                       .childGap = 4,
                                       .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                       .layoutDirection = CLAY_LEFT_TO_RIGHT},
                            .backgroundColor = rowBg,
                            .border = {.color = self->editInstrument == inst ? (Clay_Color){255, 255, 255, 255} : Tracker_ColorFromU32(instColorRgb, 255.0f),
                                       .width = CLAY_BORDER_ALL((uint16_t)(self->editInstrument == inst ? 2 : 1))},
                            .cornerRadius = {4, 4, 4, 4}
                        };

                        CLAY(
                            self->instrumentRowClicks[inst].clayId,
                            rowDecl
                        )
                        {
                            Clay_String label = ClayArena_FormatString(arena, "%02X %s", inst, Tracker_InstrumentName(self, inst));
                            CLAY(CLAY_IDI("TrackerInstrumentLabel", inst), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                                                                        .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER}}})
                            {
                                CLAY_TEXT(label, CLAY_TEXT_CONFIG(rowText));
                            }
                            CLAY(self->instrumentUpButtons[inst].clayId, inst > 0 ? smallBtn : disabledSmall)
                            {
                                CLAY_TEXT(CLAY_STRING("UP"), CLAY_TEXT_CONFIG(buttonCfg));
                            }
                            CLAY(self->instrumentDownButtons[inst].clayId, inst < 255 ? smallBtn : disabledSmall)
                            {
                                CLAY_TEXT(CLAY_STRING("DN"), CLAY_TEXT_CONFIG(buttonCfg));
                            }
                        }

                        Clay_BoundingBox rowBox = Clay_GetElementData(CLAY_IDI("TrackerInstrumentRow", inst)).boundingBox;
                        if (rowBox.height > 1.0f)
                            self->instrumentsRowHeight = rowBox.height;
                    }
                }
            }
            float thumbHeight = Tracker_InstrumentsScrollbarThumbHeight(self);
            float thumbTop = Tracker_InstrumentsScrollbarThumbTop(self, thumbHeight);
            float thumbBottom = std::max(0.0f, self->instrumentsViewportHeight - thumbTop - thumbHeight);
            CLAY(
                CLAY_ID("TrackerInstrumentsScrollbarRail"),
                {.layout = {.sizing = {CLAY_SIZING_FIXED(35), CLAY_SIZING_GROW()},
                            .layoutDirection = CLAY_TOP_TO_BOTTOM},
                 .backgroundColor = {24, 26, 36, 255},
                 .border = {.color = {70, 76, 100, 255}, .width = CLAY_BORDER_ALL(1)},
                 .cornerRadius = {0, 6, 6, 0}}
            )
            {
                if (thumbTop > 0.0f)
                {
                    CLAY(CLAY_ID("TrackerInstrumentsScrollbarTopSpace"), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(thumbTop)}}}) {}
                }
                CLAY(
                    CLAY_ID("TrackerInstrumentsScrollbarThumb"),
                    {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(thumbHeight)}},
                     .backgroundColor = {92, 118, 144, 255},
                     .cornerRadius = {4, 4, 4, 4}}
                ) {}
                if (thumbBottom > 0.0f)
                {
                    CLAY(CLAY_ID("TrackerInstrumentsScrollbarBottomSpace"), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(thumbBottom)}}}) {}
                }
            }
        }

            CLAY(CLAY_ID("TrackerInstrumentsSelectedLabel"), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                                                         .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER}}})
            {
                int selected = std::max(0, std::min(255, self->editInstrument));
                Clay_String label = ClayArena_FormatString(arena, "Selected: %02X %s", selected, Tracker_InstrumentName(self, selected));
                CLAY_TEXT(label, CLAY_TEXT_CONFIG(mutedCfg));
            }
        CLAY(
            CLAY_ID("TrackerInstrumentsActions"),
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                        .childGap = 8,
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
        )
        {
            CLAY(self->instrumentManagementNewButton.clayId, CLAY_THEME_BTN_PRIMARY)
            {
                CLAY_TEXT(CLAY_STRING("NEW"), CLAY_TEXT_CONFIG(buttonCfg));
            }
            CLAY(self->instrumentManagementCloneButton.clayId, CLAY_THEME_BTN_PRIMARY)
            {
                CLAY_TEXT(CLAY_STRING("CLONE"), CLAY_TEXT_CONFIG(buttonCfg));
            }
            CLAY(self->instrumentManagementRenameButton.clayId, CLAY_THEME_BTN_PRIMARY)
            {
                CLAY_TEXT(CLAY_STRING("RENAME"), CLAY_TEXT_CONFIG(buttonCfg));
            }
            CLAY(self->instrumentManagementEditButton.clayId, CLAY_THEME_BTN_PRIMARY)
            {
                CLAY_TEXT(CLAY_STRING("EDIT"), CLAY_TEXT_CONFIG(buttonCfg));
            }
            CLAY(self->instrumentManagementDeleteButton.clayId, CLAY_THEME_BTN_DANGER)
            {
                CLAY_TEXT(CLAY_STRING("DEL"), CLAY_TEXT_CONFIG(buttonCfg));
                        }
                    }
                }
                Clay_BoundingBox listBox = Clay_GetElementData(CLAY_ID("TrackerInstrumentsList")).boundingBox;
                if (listBox.height > 1.0f)
                {
                    self->instrumentsContentHeight = listBox.height;
                    self->instrumentsRowHeight = listBox.height / (float)std::max(1, self->availableInstrumentCount);
                }
            }

inline void Tracker_BuildSongSettingsWindow(Tracker *self, Clayton *clayton)
{
    if (!self || !self->songSettingsWindowOpen || !clayton) return;

    Clay_TextElementConfig titleCfg = CLAY_THEME_TEXT_TITLE;
    Clay_TextElementConfig buttonCfg = CLAY_THEME_TEXT_BUTTON;
    Clay_TextElementConfig bodyCfg = CLAY_THEME_TEXT_BODY;
    Clay_TextElementConfig mutedCfg = bodyCfg;
    mutedCfg.textColor = {150, 154, 170, 255};
    ClayArena *arena = &clayton->clayArena;

    auto slider = [&](const char *label, int value, int minValue, int maxValue,
                      Clay_ElementId barId, Clay_ElementId fillId) {
        float denom = (float)std::max(1, maxValue - minValue);
        float pct = std::max(0.0f, std::min(1.0f, (float)(value - minValue) / denom));
        CLAY(
            CLAY_IDI("TrackerSongSettingsSliderRow", barId.id),
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(52)},
                        .childGap = 8,
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
        )
        {
            Clay_String text = ClayArena_FormatString(arena, "%s %d", label, value);
            CLAY(CLAY_IDI("TrackerSongSettingsSliderLabel", barId.id),
                 {.layout = {.sizing = {CLAY_SIZING_FIXED(116), CLAY_SIZING_GROW()},
                             .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER}}})
            {
                CLAY_TEXT(text, CLAY_TEXT_CONFIG(bodyCfg));
            }
            CLAY(barId,
                 {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(18)}},
                  .backgroundColor = {18, 22, 32, 255},
                  .cornerRadius = {4, 4, 4, 4}})
            {
                CLAY(fillId,
                     {.layout = {.sizing = {CLAY_SIZING_PERCENT(pct), CLAY_SIZING_GROW()}},
                      .backgroundColor = {126, 154, 214, 255},
                      .cornerRadius = {4, 4, 4, 4}})
                {}
            }
        }
    };

    CLAY(CLAY_ID("TrackerSongSettingsWindow"), CLAY_THEME_WINDOW_PANEL)
    {
        CLAY(
            CLAY_ID("TrackerSongSettingsTitleRow"),
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(58)},
                        .childGap = 8,
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
        )
        {
            CLAY(CLAY_ID("TrackerSongSettingsTitle"), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                                                   .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER}}})
            {
                CLAY_TEXT(CLAY_STRING("Song Settings"), CLAY_TEXT_CONFIG(titleCfg));
            }
            CLAY(self->songSettingsCloseButton.clayId, CLAY_THEME_BTN_DANGER)
            {
                CLAY_TEXT(CLAY_STRING("x"), CLAY_TEXT_CONFIG(buttonCfg));
            }
        }

        CLAY(
            CLAY_ID("TrackerSongNameRow"),
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(54)},
                        .childGap = 8,
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
        )
        {
            CLAY(CLAY_ID("TrackerSongNameLabel"), {.layout = {.sizing = {CLAY_SIZING_FIXED(84), CLAY_SIZING_GROW()},
                                                               .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER}}})
            {
                CLAY_TEXT(CLAY_STRING("Name"), CLAY_TEXT_CONFIG(bodyCfg));
            }
            CLAY(self->songNameButton.clayId, CLAY_THEME_BTN_PRIMARY)
            {
                Clay_String name = ClayArena_FormatString(arena, "%s", self->songDisplayName);
                CLAY_TEXT(name, CLAY_TEXT_CONFIG(buttonCfg));
            }
        }

        CLAY(
            CLAY_ID("TrackerSongLoadEmptyRow"),
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(54)},
                        .childGap = 8,
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
        )
        {
            CLAY(CLAY_ID("TrackerSongLoadEmptyLabel"), {.layout = {.sizing = {CLAY_SIZING_FIXED(84), CLAY_SIZING_GROW()},
                                                                    .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER}}})
            {
                CLAY_TEXT(CLAY_STRING("Empty"), CLAY_TEXT_CONFIG(bodyCfg));
            }
            CLAY(self->songLoadEmptyButton.clayId, CLAY_THEME_BTN_PRIMARY)
            {
                CLAY_TEXT(CLAY_STRING("Load"), CLAY_TEXT_CONFIG(buttonCfg));
            }
        }

        Clay_ElementDeclaration lfoBtn = CLAY_THEME_BTN_BOX;
        if (self->songLfoEnabled) lfoBtn.backgroundColor = CLAY_COLOR_BTN_SUCCESS;
        CLAY(
            CLAY_ID("TrackerSongLfoRow"),
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(54)},
                        .childGap = 8,
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
        )
        {
            CLAY(CLAY_ID("TrackerSongLfoLabel"), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                                              .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER}}})
            {
                CLAY_TEXT(CLAY_STRING("LFO"), CLAY_TEXT_CONFIG(bodyCfg));
            }
            CLAY(self->songLfoButton.clayId, lfoBtn)
            {
                CLAY_TEXT(self->songLfoEnabled ? CLAY_STRING("✓") : CLAY_STRING(""), CLAY_TEXT_CONFIG(buttonCfg));
            }
        }

        slider("LFO Freq", self->songLfoFrequency, 0, 7, CLAY_ID("TrackerSongLfoFreqBar"), CLAY_ID("TrackerSongLfoFreqFill"));
        CLAY(
            CLAY_ID("TrackerSongLfoHzRow"),
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(30)},
                        .childGap = 8,
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
        )
        {
            CLAY(CLAY_ID("TrackerSongLfoHzSpacer"), {.layout = {.sizing = {CLAY_SIZING_FIXED(116), CLAY_SIZING_GROW()}}}) {}
            CLAY(CLAY_ID("TrackerSongLfoHzText"), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                                               .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER}}})
            {
                Clay_String hz = ClayArena_FormatString(
                    arena,
                    "YM2612 LFO %d = %.2f Hz",
                    std::max(0, std::min(7, self->songLfoFrequency)),
                    Tracker_OpnLfoFrequencyHz(self->songLfoFrequency)
                );
                CLAY_TEXT(hz, CLAY_TEXT_CONFIG(mutedCfg));
            }
        }
        slider("Tick Rate", self->songTickRate, 30, 300, CLAY_ID("TrackerSongTickRateBar"), CLAY_ID("TrackerSongTickRateFill"));
        slider("Ticks/Row", self->songSpeed, 1, 32, CLAY_ID("TrackerSongSpeedBar"), CLAY_ID("TrackerSongSpeedFill"));
        slider("Rows/Beat", self->songRowsPerBeat, 1, 16, CLAY_ID("TrackerSongRowsPerBeatBar"), CLAY_ID("TrackerSongRowsPerBeatFill"));

        const float bpm =
            (self->songTickRate > 0 && self->songSpeed > 0 && self->songRowsPerBeat > 0) ?
                (self->songTickRate * 60.0f) / ((float)self->songSpeed * (float)self->songRowsPerBeat) :
                0.0f;
        CLAY(
            CLAY_ID("TrackerSongBpmRow"),
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(40)},
                        .childGap = 8,
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
        )
        {
            CLAY(CLAY_ID("TrackerSongBpmLabel"), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                                              .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER}}})
            {
                Clay_String text = ClayArena_FormatString(arena, "Est. BPM %.1f", bpm);
                CLAY_TEXT(text, CLAY_TEXT_CONFIG(mutedCfg));
            }
        }
    }
}

inline void Tracker_BuildPartEditorWindow(Tracker *self, Clayton *clayton)
{
    if (!self || !self->partEditorOpen || !clayton) return;

    Tracker_NormalizeParts(self);
    if (self->partCount <= 0)
        return;
    self->partEditorPart = std::max(0, std::min(self->partCount - 1, self->partEditorPart));
    TrackerPart &part = self->parts[self->partEditorPart];

    ClayArena *arena = &clayton->clayArena;
    Clay_TextElementConfig titleCfg = CLAY_THEME_TEXT_TITLE;
    Clay_TextElementConfig bodyCfg = CLAY_THEME_TEXT_BODY;
    Clay_TextElementConfig buttonCfg = CLAY_THEME_TEXT_BUTTON;
    Clay_TextElementConfig mutedCfg = bodyCfg;
    mutedCfg.textColor = {150, 154, 170, 255};

    auto renderRowSelector = [&](const char *label, int value, bool canDec, bool canInc, Clay_ElementId decId, Clay_ElementId incId)
    {
        CLAY(
            CLAY_IDI("TrackerPartEditorRowsRow", decId.id),
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(54)},
                        .childGap = 8,
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
        )
        {
            CLAY(CLAY_IDI("TrackerPartEditorRowsLabel", decId.id),
                 {.layout = {.sizing = {CLAY_SIZING_FIXED(88), CLAY_SIZING_GROW()},
                             .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER}}})
            {
                CLAY_TEXT(ClayArena_AllocString(arena, label), CLAY_TEXT_CONFIG(bodyCfg));
            }
            Clay_ElementDeclaration minusBtn = CLAY_THEME_BTN_BOX;
            minusBtn.layout.sizing = {CLAY_SIZING_FIXED(42), CLAY_SIZING_FIXED(38)};
            if (!canDec) minusBtn.backgroundColor = CLAY_COLOR_BTN_DISABLED;
            CLAY(decId, minusBtn)
            {
                CLAY_TEXT(CLAY_STRING("-"), CLAY_TEXT_CONFIG(buttonCfg));
            }
            CLAY(
                CLAY_IDI("TrackerPartEditorRowsValue", decId.id),
                {.layout = {.sizing = {CLAY_SIZING_FIXED(74), CLAY_SIZING_GROW()},
                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}},
                 .backgroundColor = {18, 22, 32, 255},
                 .cornerRadius = {4, 4, 4, 4}}
            )
            {
                Clay_String text = ClayArena_FormatString(arena, "%d", value);
                CLAY_TEXT(text, CLAY_TEXT_CONFIG(buttonCfg));
            }
            Clay_ElementDeclaration plusBtn = CLAY_THEME_BTN_BOX;
            plusBtn.layout.sizing = {CLAY_SIZING_FIXED(42), CLAY_SIZING_FIXED(38)};
            if (!canInc) plusBtn.backgroundColor = CLAY_COLOR_BTN_DISABLED;
            CLAY(incId, plusBtn)
            {
                CLAY_TEXT(CLAY_STRING("+"), CLAY_TEXT_CONFIG(buttonCfg));
            }
        }
    };

    CLAY(CLAY_ID("TrackerPartEditorWindow"), CLAY_THEME_WINDOW_PANEL)
    {
        CLAY(
            CLAY_ID("TrackerPartEditorTitleRow"),
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(58)},
                        .childGap = 8,
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
        )
        {
            Clay_String title = ClayArena_FormatString(arena, "PART %02d", self->partEditorPart + 1);
            CLAY(CLAY_ID("TrackerPartEditorTitle"), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                                                 .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER}}})
            {
                CLAY_TEXT(title, CLAY_TEXT_CONFIG(titleCfg));
            }
            CLAY(self->partEditorCloseButton.clayId, CLAY_THEME_BTN_DANGER)
            {
                CLAY_TEXT(CLAY_STRING("x"), CLAY_TEXT_CONFIG(buttonCfg));
            }
        }

        CLAY(
            CLAY_ID("TrackerPartEditorBody"),
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                        .childGap = 10,
                        .layoutDirection = CLAY_TOP_TO_BOTTOM}}
        )
        {
            CLAY(
                CLAY_ID("TrackerPartEditorNameRow"),
                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(54)},
                            .childGap = 8,
                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                            .layoutDirection = CLAY_LEFT_TO_RIGHT}}
            )
            {
                CLAY(CLAY_ID("TrackerPartEditorNameLabel"), {.layout = {.sizing = {CLAY_SIZING_FIXED(88), CLAY_SIZING_GROW()},
                                                                        .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER}}})
                {
                    CLAY_TEXT(CLAY_STRING("Name"), CLAY_TEXT_CONFIG(bodyCfg));
                }
                CLAY(self->partEditorNameButton.clayId, CLAY_THEME_BTN_PRIMARY)
                {
                    Clay_String name = ClayArena_FormatString(arena, "%s", part.name);
                    CLAY_TEXT(name, CLAY_TEXT_CONFIG(buttonCfg));
                }
            }

            CLAY(
                CLAY_ID("TrackerPartEditorEnableRow"),
                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(54)},
                            .childGap = 8,
                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                            .layoutDirection = CLAY_LEFT_TO_RIGHT}}
            )
            {
                CLAY(CLAY_ID("TrackerPartEditorEnableLabel"), {.layout = {.sizing = {CLAY_SIZING_FIXED(88), CLAY_SIZING_GROW()},
                                                                          .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER}}})
                {
                    CLAY_TEXT(CLAY_STRING("On"), CLAY_TEXT_CONFIG(bodyCfg));
                }
                Clay_ElementDeclaration enableBtn = CLAY_THEME_BTN_BOX;
                enableBtn.layout.sizing = {CLAY_SIZING_FIXED(42), CLAY_SIZING_FIXED(38)};
                enableBtn.backgroundColor = part.enabled ? CLAY_COLOR_BTN_SUCCESS : CLAY_COLOR_BTN_DISABLED;
                CLAY(self->partEditorEnableButton.clayId, enableBtn)
                {
                    CLAY_TEXT(part.enabled ? CLAY_STRING("✓") : CLAY_STRING(""), CLAY_TEXT_CONFIG(buttonCfg));
                }
            }

            renderRowSelector(
                "Rows",
                part.rowCount,
                self->rowCount > 1,
                self->rowCount < TRACKER_MAX_ROWS,
                self->partEditorRowsMinusButton.clayId,
                self->partEditorRowsPlusButton.clayId
            );

            CLAY(
                CLAY_ID("TrackerPartEditorNote"),
                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                            .padding = {2, 0, 0, 0}}}
            )
            {
                CLAY_TEXT(CLAY_STRING("Deleting this part closes the window."), CLAY_TEXT_CONFIG(mutedCfg));
            }

            CLAY(
                CLAY_ID("TrackerPartEditorDeleteRow"),
                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(48)},
                            .childAlignment = {CLAY_ALIGN_X_RIGHT, CLAY_ALIGN_Y_CENTER},
                            .layoutDirection = CLAY_LEFT_TO_RIGHT}}
            )
            {
                Clay_ElementDeclaration deleteBtn = CLAY_THEME_BTN_DANGER;
                deleteBtn.layout.sizing = {CLAY_SIZING_FIXED(96), CLAY_SIZING_FIXED(42)};
                if (self->partCount <= 1)
                    deleteBtn.backgroundColor = CLAY_COLOR_BTN_DISABLED;
                CLAY(self->partEditorDeleteButton.clayId, deleteBtn)
                {
                    CLAY_TEXT(CLAY_STRING("DEL"), CLAY_TEXT_CONFIG(buttonCfg));
                }
            }
        }
    }
}

inline void Tracker_BuildSaveConfirmWindow(Tracker *self, Clayton *clayton)
{
    if (!self || !self->songSaveConfirmWindowOpen || !clayton) return;

    ClayArena *arena = &clayton->clayArena;
    Clay_TextElementConfig titleCfg = CLAY_THEME_TEXT_TITLE;
    Clay_TextElementConfig bodyCfg = CLAY_THEME_TEXT_BODY;
    Clay_TextElementConfig buttonCfg = CLAY_THEME_TEXT_BUTTON;
    Clay_TextElementConfig fileCfg = CLAY_THEME_TEXT_BODY;
    fileCfg.fontId = CLAY_FONT_MONO;
    fileCfg.fontSize = CLAY_FONT_SIZE_SM;

    std::string displayName = self->songDisplayName;
    if (displayName.empty())
        displayName = "User Song";
    std::string filename = TrackerSongIO_SaveFilenameForDisplay(displayName);
    if (filename.size() <= 2 || filename == ".h")
        filename = TrackerSongIO_SaveFilenameForDisplay("User Song");

    CLAY(CLAY_ID("TrackerSaveConfirmWindow"), CLAY_THEME_WINDOW_PANEL)
    {
        CLAY(
            CLAY_ID("TrackerSaveConfirmBody"),
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                        .childGap = 12,
                        .layoutDirection = CLAY_TOP_TO_BOTTOM}}
        )
        {
            CLAY_TEXT(CLAY_STRING("Save Song"), CLAY_TEXT_CONFIG(titleCfg));
            CLAY_TEXT(CLAY_STRING("Your song will be saved as this file name:"), CLAY_TEXT_CONFIG(bodyCfg));

            CLAY(
                CLAY_ID("TrackerSaveConfirmFilename"),
                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(44)},
                            .padding = {10, 10, 0, 0},
                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}},
                 .backgroundColor = {18, 22, 32, 255},
                 .cornerRadius = {4, 4, 4, 4}}
            )
            {
                CLAY_TEXT(ClayArena_AllocString(arena, filename.c_str()), CLAY_TEXT_CONFIG(fileCfg));
            }

            CLAY(
                CLAY_ID("TrackerSaveConfirmButtons"),
                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(48)},
                            .childGap = 8,
                            .layoutDirection = CLAY_LEFT_TO_RIGHT}}
            )
            {
                CLAY(self->saveConfirmSaveButton.clayId, CLAY_THEME_BTN_PRIMARY)
                {
                    CLAY_TEXT(CLAY_STRING("SAVE"), CLAY_TEXT_CONFIG(buttonCfg));
                }
                CLAY(self->saveConfirmChangeNameButton.clayId, CLAY_THEME_BTN_PRIMARY)
                {
                    CLAY_TEXT(CLAY_STRING("CHANGE NAME"), CLAY_TEXT_CONFIG(buttonCfg));
                }
                CLAY(self->saveConfirmCancelButton.clayId, CLAY_THEME_BTN_DANGER)
                {
                    CLAY_TEXT(CLAY_STRING("CANCEL"), CLAY_TEXT_CONFIG(buttonCfg));
                }
            }
        }
    }
}

inline void Tracker_BuildLoadErrorWindow(Tracker *self, Clayton *clayton)
{
    if (!self || !self->songLoadErrorWindowOpen || !clayton) return;

    ClayArena *arena = &clayton->clayArena;
    Clay_TextElementConfig titleCfg = CLAY_THEME_TEXT_TITLE;
    Clay_TextElementConfig bodyCfg = CLAY_THEME_TEXT_BODY;
    Clay_TextElementConfig monoCfg = CLAY_THEME_TEXT_BODY;
    monoCfg.fontId = CLAY_FONT_MONO;
    monoCfg.fontSize = CLAY_FONT_SIZE_SM;
    Clay_TextElementConfig buttonCfg = CLAY_THEME_TEXT_BUTTON;

    CLAY(CLAY_ID("TrackerLoadErrorWindow"), CLAY_THEME_WINDOW_PANEL)
    {
        CLAY(
            CLAY_ID("TrackerLoadErrorBody"),
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                        .childGap = 12,
                        .layoutDirection = CLAY_TOP_TO_BOTTOM}}
        )
        {
            CLAY_TEXT(CLAY_STRING("Load Failed"), CLAY_TEXT_CONFIG(titleCfg));
            CLAY_TEXT(CLAY_STRING("The song file has parser errors:"), CLAY_TEXT_CONFIG(bodyCfg));

            CLAY(
                CLAY_ID("TrackerLoadErrorMessages"),
                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                            .padding = {10, 10, 10, 10},
                            .childGap = 6,
                            .layoutDirection = CLAY_TOP_TO_BOTTOM},
                 .backgroundColor = {18, 22, 32, 255},
                 .cornerRadius = {4, 4, 4, 4}}
            )
            {
                const char *p = self->songLoadErrorText[0] ? self->songLoadErrorText : "invalid tracker file";
                int lineCount = 0;
                while (*p && lineCount < 16)
                {
                    const char *lineStart = p;
                    const char *lineEnd = p;
                    while (*lineEnd && *lineEnd != '\n' && *lineEnd != '\r') lineEnd++;
                    while (lineStart < lineEnd && (*lineStart == ' ' || *lineStart == '\t')) lineStart++;
                    while (lineEnd > lineStart && (lineEnd[-1] == ' ' || lineEnd[-1] == '\t')) lineEnd--;
                    if (lineEnd > lineStart)
                    {
                        Clay_String line = ClayArena_AllocString(arena, std::string(lineStart, lineEnd - lineStart).c_str());
                        CLAY_TEXT(line, CLAY_TEXT_CONFIG(monoCfg));
                        lineCount++;
                    }
                    p = lineEnd;
                    while (*p == '\r') p++;
                    if (*p == '\n') p++;
                }
                if (*p)
                    CLAY_TEXT(CLAY_STRING("..."), CLAY_TEXT_CONFIG(monoCfg));
            }

            CLAY(
                CLAY_ID("TrackerLoadErrorButtons"),
                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(48)},
                            .childAlignment = {CLAY_ALIGN_X_RIGHT, CLAY_ALIGN_Y_CENTER},
                            .layoutDirection = CLAY_LEFT_TO_RIGHT}}
            )
            {
                CLAY(self->loadErrorOkButton.clayId, CLAY_THEME_BTN_PRIMARY)
                {
                    CLAY_TEXT(CLAY_STRING("OK"), CLAY_TEXT_CONFIG(buttonCfg));
                }
            }
        }
    }
}

inline void Tracker_BuildHud(Tracker *self, Clayton *clayton)
{
    if (!self || !self->active || !clayton) return;

    ClayArena *arena = &clayton->clayArena;
    Clay_TextElementConfig titleCfg = CLAY_THEME_TEXT_TITLE;
    Clay_TextElementConfig bodyCfg = CLAY_THEME_TEXT_BODY;
    Clay_TextElementConfig buttonCfg = CLAY_THEME_TEXT_BUTTON;
    Clay_TextElementConfig monoCfg = CLAY_THEME_TEXT_BODY;
    monoCfg.fontId = CLAY_FONT_MONO;
    monoCfg.fontSize = CLAY_FONT_SIZE_SM;
    Clay_TextElementConfig effectMonoCfg = monoCfg;
    effectMonoCfg.fontSize = CLAY_FONT_SIZE_SM ;
    Clay_TextElementConfig darkMonoCfg = monoCfg;
    darkMonoCfg.textColor = {14, 16, 22, 255};
    Clay_TextElementConfig darkEffectMonoCfg = effectMonoCfg;
    darkEffectMonoCfg.textColor = {14, 16, 22, 255};
    const float trackerFooterHeight = 144.0f;
    float trackerViewportHeight = self->viewportHeight > 1.0f ? self->viewportHeight : 360.0f;
    Clay_BoundingBox portraitBox = Clay_GetElementData(CLAY_ID("Portrait area")).boundingBox;
    if (portraitBox.height > 1.0f)
    {
        const float trackerChromeHeight = 6.0f + 6.0f + 42.0f + 6.0f + 28.0f + 6.0f + trackerFooterHeight;
        trackerViewportHeight = std::max(80.0f, portraitBox.height - trackerChromeHeight);
    }

    CLAY(
        CLAY_ID("TrackerHud"),
        {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                    .padding = {6, 6, 6, 6},
                    .childGap = 6,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM},
         .backgroundColor = {18, 18, 28, 235}}
    )
    {
        CLAY(
            CLAY_ID("TrackerTitleRow"),
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                        .childGap = 8,
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
        )
        {
            Clay_String title = self->loopEnabled ?
                ClayArena_FormatString(
                    arena,
                    "OPN Tracker :: %s  R%03d.%d  LOOP %03d-%03d",
                    self->songDisplayName,
                    self->playRow,
                    self->playTick,
                    self->loopStart,
                    self->loopEnd
                ) :
                ClayArena_FormatString(
                    arena,
                    "OPN Tracker :: %s  R%03d.%d  LOOP off",
                    self->songDisplayName,
                    self->playRow,
                    self->playTick
                );
            CLAY_TEXT(title, CLAY_TEXT_CONFIG(titleCfg));
            CLAY(CLAY_ID("TrackerTitleGrow"), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()}}}) {}
            CLAY(self->closeButton.clayId, CLAY_THEME_BTN_DANGER)
            {
                CLAY_TEXT(CLAY_STRING("x"), CLAY_TEXT_CONFIG(buttonCfg));
            }
        }

        CLAY(
            CLAY_ID("TrackerFixedHeader"),
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(28)},
                        .childGap = 0,
                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
        )
        {
            CLAY(
                CLAY_ID("TrackerHeaderLine"),
                {.layout = {.sizing = {CLAY_SIZING_PERCENT(TRACKER_SIDE_UNIT), CLAY_SIZING_GROW()},
                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}},
                 .backgroundColor = {22, 24, 36, 255}}
            )
            {
                bool hasCustomLoop = self->loopEnabled;
                Clay_TextElementConfig clearCfg = CLAY_THEME_TEXT_BUTTON;
                clearCfg.fontSize = CLAY_FONT_SIZE_SM;
                Clay_ElementDeclaration clearBtn = {
                    .layout = {.sizing = {CLAY_SIZING_FIXED(20), CLAY_SIZING_FIXED(20)},
                               .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}},
                    .backgroundColor = hasCustomLoop ? (Clay_Color){176, 68, 84, 255} : (Clay_Color){42, 46, 58, 255},
                    .cornerRadius = {4, 4, 4, 4},
                    .border = {.color = hasCustomLoop ? (Clay_Color){230, 120, 132, 255} : (Clay_Color){66, 70, 84, 255},
                               .width = CLAY_BORDER_ALL(1)}
                };
                CLAY(self->clearLoopButton.clayId, clearBtn)
                {
                    CLAY_TEXT(CLAY_STRING("x"), CLAY_TEXT_CONFIG(clearCfg));
                }
            }
            for (int ch = 0; ch < TRACKER_CHANNELS; ch++)
            {
                bool channelSelected = !self->channelSelectionEnabled || (ch >= self->channelStart && ch <= self->channelEnd);
                CLAY(
                    CLAY_IDI("TrackerHeaderChannel", ch),
                    {.layout = {.sizing = {CLAY_SIZING_PERCENT(TRACKER_CHANNEL_UNIT), CLAY_SIZING_GROW()},
                                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}},
                     .backgroundColor = channelSelected ? (Clay_Color){54, 78, 104, 255} : (Clay_Color){38, 48, 74, 255},
                     .border = {.color = channelSelected ? (Clay_Color){112, 210, 132, 230} : (Clay_Color){48, 54, 72, 255},
                                .width = CLAY_BORDER_ALL(1)}}
                )
                {
                    Clay_String label = ClayArena_FormatString(arena, "CH%d", ch + 1);
                    CLAY_TEXT(label, CLAY_TEXT_CONFIG(bodyCfg));
                }
            }
            CLAY(
                CLAY_ID("TrackerHeaderScrollbar"),
                {.layout = {.sizing = {CLAY_SIZING_PERCENT(TRACKER_SIDE_UNIT), CLAY_SIZING_GROW()},
                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}},
                 .backgroundColor = {22, 24, 36, 255}}
            ) {}
        }

        self->viewportHeight = 0.0f;
        CLAY(
            CLAY_ID("TrackerGridArea"),
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(trackerViewportHeight)},
                        .childGap = 0,
                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
        )
        {
            CLAY(
                CLAY_ID("TrackerGridViewport"),
                {.layout = {.sizing = {CLAY_SIZING_PERCENT(TRACKER_SCROLLABLE_UNIT), CLAY_SIZING_GROW()},
                            .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_TOP}},
                 .backgroundColor = {12, 12, 20, 255},
                 .clip = {.horizontal = false, .vertical = true, .childOffset = {0, -self->scrollY}},
                 .border = {.color = {70, 76, 100, 255}, .width = CLAY_BORDER_ALL(1)}}
            )
            {
                Clay_BoundingBox bb = Clay_GetElementData(CLAY_ID("TrackerGridViewport")).boundingBox;
                self->viewportHeight = bb.height > 1.0f ? bb.height : trackerViewportHeight;

                CLAY(
                    CLAY_ID("TrackerGridBelt"),
                    {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                                .layoutDirection = CLAY_TOP_TO_BOTTOM}}
                )
                {
                    int visibleRows = Tracker_VisibleRowCount(self);
                    for (int visualIndex = 0; visualIndex < visibleRows; visualIndex++)
                    {
                        TrackerVisualRow visual = Tracker_MapVisualIndex(self, visualIndex);
                        if (visual.kind == TRACKER_VISUAL_ROW_PART_TITLE)
                        {
                            int partIndex = visual.part;
                            TrackerPart &part = self->parts[partIndex];
                            Clay_Color titleBg = !part.enabled ? (Clay_Color){42, 34, 38, 255} :
                                (part.collapsed ? (Clay_Color){34, 40, 58, 255} : (Clay_Color){28, 34, 48, 255});
                            CLAY(
                                CLAY_IDI("TrackerPartTitleRow", partIndex),
                                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(self->rowHeight)},
                                            .padding = {6, 4, 6, 4},
                                            .childGap = 4,
                                            .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                                            .layoutDirection = CLAY_LEFT_TO_RIGHT},
                                 .backgroundColor = titleBg,
                                 .border = {.color = {86, 98, 126, 255}, .width = CLAY_BORDER_OUTSIDE(1)}}
                            )
                            {
                                Tracker_BuildPartTitleContent(
                                    self,
                                    arena,
                                    partIndex,
                                    &self->partToggleButtons[partIndex],
                                    &self->partUpButtons[partIndex],
                                    &self->partDownButtons[partIndex],
                                    &self->partSettingsButtons[partIndex],
                                    buttonCfg,
                                    bodyCfg
                                );
                            }
                            continue;
                        }
                        if (visual.kind != TRACKER_VISUAL_ROW_CELL)
                            continue;
                        int row = visual.row;
                        int displayRow = std::max(0, visual.localRow);
                        bool activeRow = row == self->playRow;
                        CLAY(
                            CLAY_IDI("TrackerGridRow", visualIndex),
                            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(self->rowHeight)},
                                        .childGap = 0,
                                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
                        )
                        {
                            CLAY(
                                CLAY_IDI("TrackerLineCell", row),
                                {.layout = {.sizing = {CLAY_SIZING_PERCENT(TRACKER_LINE_IN_SCROLL), CLAY_SIZING_GROW()},
                                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}},
                                 .backgroundColor = Tracker_LoopLineColor(self, row, activeRow),
                                 .border = {.color = {50, 56, 74, 255}, .width = CLAY_BORDER_ALL(1)}}
                            )
                            {
                                Clay_String rn = ClayArena_FormatString(arena, "%03X", displayRow);
                                CLAY_TEXT(rn, CLAY_TEXT_CONFIG(monoCfg));
                            }
                            for (int ch = 0; ch < TRACKER_CHANNELS; ch++)
                            {
                                bool selectedColumn = !self->channelSelectionEnabled || (ch >= self->channelStart && ch <= self->channelEnd);
                                Clay_Color cellBg = Tracker_LoopCellColor(self, row, activeRow);
                                if (self->loopEnabled && !selectedColumn)
                                    cellBg = Tracker_CellColor(activeRow, false);
                                const char *cell = self->cells[row][ch].text;
                                bool playable = Tracker_CellHasPlayableNote(cell);
                                bool specialTerminator = Tracker_CellIsSpecialTerminator(cell);
                                int displayInst = Tracker_CellDisplayInstrument(self, cell, row, ch);
                                uint32_t displayColor = displayInst >= 0 ? Tracker_InstrumentColorU32(self, displayInst) : 0;
                                Clay_Color cellBorder = {50, 56, 74, 255};
                                Clay_BorderElementConfig cellBorderConfig = {.color = cellBorder, .width = CLAY_BORDER_ALL(1)};
                                bool brightCellBg = false;
                                if (playable && displayColor != 0)
                                {
                                    cellBg = Tracker_ColorFromU32(displayColor, activeRow ? 245.0f : 225.0f);
                                    brightCellBg = Tracker_ColorIsBright(displayColor);
                                }
                                else if (specialTerminator || Tracker_CellHasNoteLikeValue(cell))
                                {
                                    cellBorder = displayColor != 0 ?
                                        Tracker_ColorFromU32(displayColor, 255.0f) : (Clay_Color){245, 245, 250, 255};
                                    cellBorderConfig = {.color = cellBorder, .width = CLAY_BORDER_ALL(2)};
                                }
                                if (self->cellMoving &&
                                    self->cellMoveValidTarget &&
                                    row == self->cellMoveHoverRow &&
                                    ch == self->cellMoveHoverChannel)
                                {
                                    cell = self->cellMoveSource.text;
                                    cellBg = Tracker_CellMoveHighlightColor(self);
                                    brightCellBg = true;
                                    cellBorderConfig = {.color = {255, 255, 255, 255}, .width = CLAY_BORDER_ALL(2)};
                                }
                                CLAY(
                                    CLAY_IDI("TrackerCell", row * 10 + ch),
                                    {.layout = {.sizing = {CLAY_SIZING_PERCENT(TRACKER_CHANNEL_IN_SCROLL), CLAY_SIZING_GROW()},
                                                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}},
                                     .backgroundColor = cellBg,
                                     .border = cellBorderConfig}
                                )
                                {
                                    char top[8] = ".......";
                                    char bottom[18] = "";
                                    for (int i = 0; i < 7 && cell[i]; i++)
                                        top[i] = cell[i];
                                    top[7] = '\0';
                                    int effectCount = 0;
                                    int out = 0;
                                    for (int pos = 7; effectCount < 2 && pos + 3 < TRACKER_CELL_CHARS && cell[pos]; pos += 4)
                                    {
                                        if (!Tracker_IsHex(cell[pos]) || !Tracker_IsHex(cell[pos + 1]) ||
                                            !Tracker_IsHex(cell[pos + 2]) || !Tracker_IsHex(cell[pos + 3]))
                                            break;
                                        if (out > 0 && out < (int)sizeof(bottom) - 1)
                                            bottom[out++] = ' ';
                                        for (int k = 0; k < 4 && out < (int)sizeof(bottom) - 1; k++)
                                            bottom[out++] = cell[pos + k];
                                        effectCount++;
                                    }
                                    bottom[out] = '\0';
                                    CLAY(
                                        CLAY_IDI("TrackerCellTextStack", row * 10 + ch),
                                        {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                                                    .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                                    .layoutDirection = CLAY_TOP_TO_BOTTOM}}
                                    )
                                    {
                                        CLAY_TEXT(ClayArena_AllocString(arena, top), CLAY_TEXT_CONFIG(brightCellBg ? darkMonoCfg : monoCfg));
                                        if (bottom[0])
                                            CLAY_TEXT(ClayArena_AllocString(arena, bottom), CLAY_TEXT_CONFIG(brightCellBg ? darkEffectMonoCfg : effectMonoCfg));
                                    }
                                }
                            }
                        }
                    }
                }

                int stickyPart = Tracker_StickyPartIndexAtScroll(self);
                if (stickyPart >= 0 && stickyPart < self->partCount)
                {
                    TrackerPart &part = self->parts[stickyPart];
                    float stickyTop = Tracker_StickyPartTitleTopY(self, stickyPart);
                    Clay_Color titleBg = !part.enabled ? (Clay_Color){42, 34, 38, 248} :
                        (part.collapsed ? (Clay_Color){34, 40, 58, 248} : (Clay_Color){28, 34, 48, 248});
                    CLAY(
                        CLAY_ID("TrackerStickyPartTitleRow"),
                        {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(self->rowHeight)},
                                    .padding = {6, 4, 6, 4},
                                    .childGap = 4,
                                    .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                                    .layoutDirection = CLAY_LEFT_TO_RIGHT},
                         .backgroundColor = titleBg,
                         .floating = {
                             .offset = {0, stickyTop + self->rowHeight * 0.5f - self->viewportHeight * 0.5f},
                             .zIndex = 12,
                             .attachPoints = {CLAY_ATTACH_POINT_CENTER_CENTER, CLAY_ATTACH_POINT_CENTER_CENTER},
                             .attachTo = CLAY_ATTACH_TO_PARENT,
                         },
                         .border = {.color = {118, 132, 164, 255}, .width = CLAY_BORDER_OUTSIDE(1)}}
                    )
                    {
                        Tracker_BuildPartTitleContent(
                            self,
                            arena,
                            stickyPart,
                            &self->stickyPartToggleButton,
                            &self->stickyPartUpButton,
                            &self->stickyPartDownButton,
                            &self->stickyPartSettingsButton,
                            buttonCfg,
                            bodyCfg
                        );
                    }
                }
            }

            float thumbHeight = Tracker_ScrollbarThumbHeight(self);
            float thumbTop = Tracker_ScrollbarThumbTop(self, thumbHeight);
            float thumbBottom = std::max(0.0f, self->viewportHeight - thumbTop - thumbHeight);
            float rowCountForMap = std::max(1.0f, (float)Tracker_VisibleRowCount(self));
            float loopVisualStart = self->loopEnabled ? (float)Tracker_VisualIndexForRow(self, self->loopStart) : 0.0f;
            float loopVisualEnd = self->loopEnabled ? (float)(Tracker_VisualIndexForRow(self, self->loopEnd) + 1) : 0.0f;
            float scrollbarRangeTop = self->loopEnabled ? self->viewportHeight * (loopVisualStart / rowCountForMap) : 0.0f;
            float scrollbarRangeBottom = self->loopEnabled ?
                self->viewportHeight * (loopVisualEnd / rowCountForMap) : 0.0f;
            float scrollbarRangeHeight = self->loopEnabled ? std::max(3.0f, scrollbarRangeBottom - scrollbarRangeTop) : 0.0f;
            float scrollbarPlayheadTop =
                self->viewportHeight * ((float)Tracker_VisualIndexForRow(self, self->playRow) / rowCountForMap);
            Clay_Color railColor = self->followCursor ? (Clay_Color){16, 18, 28, 255} : (Clay_Color){46, 34, 20, 255};
            Clay_Color thumbColor = self->followCursor ? (Clay_Color){92, 118, 144, 255} : (Clay_Color){214, 132, 54, 255};
            CLAY(
                CLAY_ID("TrackerScrollbarRail"),
                {.layout = {.sizing = {CLAY_SIZING_PERCENT(TRACKER_SIDE_UNIT), CLAY_SIZING_GROW()},
                            .layoutDirection = CLAY_TOP_TO_BOTTOM},
                 .backgroundColor = railColor,
                 .border = {.color = {70, 76, 100, 255}, .width = CLAY_BORDER_ALL(1)}}
            )
            {
                if (self->loopEnabled)
                {
                    CLAY(
                        CLAY_ID("TrackerScrollbarLoopRange"),
                        {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(scrollbarRangeHeight)}},
                         .backgroundColor = {0, 0, 0, 0},
                         .floating = {
                             .offset = {0, scrollbarRangeTop + scrollbarRangeHeight * 0.5f - self->viewportHeight * 0.5f},
                             .zIndex = 1,
                             .attachPoints = {CLAY_ATTACH_POINT_CENTER_CENTER, CLAY_ATTACH_POINT_CENTER_CENTER},
                             .attachTo = CLAY_ATTACH_TO_PARENT,
                         },
                        .border = {.color = {112, 210, 132, 230}, .width = CLAY_BORDER_ALL(2)}}
                    ) {}
                }
                for (int partIndex = 0; partIndex + 1 < self->partCount; partIndex++)
                {
                    int boundaryVisual = Tracker_VisualIndexForPartBoundary(self, partIndex);
                    if (boundaryVisual < 0)
                        continue;
                    float boundaryTop = self->viewportHeight * ((float)boundaryVisual / rowCountForMap);
                    CLAY(
                        CLAY_IDI("TrackerScrollbarPartBoundary", partIndex),
                        {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(3)}},
                         .backgroundColor = {140, 164, 196, 230},
                         .floating = {
                             .offset = {0, boundaryTop - self->viewportHeight * 0.5f},
                             .zIndex = 2,
                             .attachPoints = {CLAY_ATTACH_POINT_CENTER_CENTER, CLAY_ATTACH_POINT_CENTER_CENTER},
                             .attachTo = CLAY_ATTACH_TO_PARENT,
                         }}
                    ) {}
                }
                CLAY(
                    CLAY_ID("TrackerScrollbarPlayhead"),
                    {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(2)}},
                     .backgroundColor = {255, 245, 160, 255},
                     .floating = {
                         .offset = {0, scrollbarPlayheadTop - self->viewportHeight * 0.5f},
                         .zIndex = 3,
                         .attachPoints = {CLAY_ATTACH_POINT_CENTER_CENTER, CLAY_ATTACH_POINT_CENTER_CENTER},
                         .attachTo = CLAY_ATTACH_TO_PARENT,
                     }}
                ) {}
                if (thumbTop > 0.0f)
                {
                    CLAY(CLAY_ID("TrackerScrollbarTopSpace"), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(thumbTop)}}}) {}
                }
                CLAY(
                    CLAY_ID("TrackerScrollbarThumb"),
                    {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(thumbHeight)}},
                     .backgroundColor = thumbColor}
                ) {}
                if (thumbBottom > 0.0f)
                {
                    CLAY(CLAY_ID("TrackerScrollbarBottomSpace"), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(thumbBottom)}}}) {}
                }
            }
        }

        CLAY(
            CLAY_ID("TrackerBottomControls"),
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(trackerFooterHeight)},
                        .padding = {4, 4, 4, 4},
                        .childGap = 5,
                        .layoutDirection = CLAY_TOP_TO_BOTTOM},
             .backgroundColor = {28, 28, 42, 255}}
        )
        {
            CLAY(
                CLAY_ID("TrackerTransportRow"),
                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                            .childGap = 5,
                            .layoutDirection = CLAY_LEFT_TO_RIGHT}}
            )
            {
                CLAY(self->playButton.clayId, CLAY_THEME_BTN_PRIMARY)
                {
                    CLAY_TEXT(CLAY_STRING("START"), CLAY_TEXT_CONFIG(buttonCfg));
                }
                CLAY(self->stopButton.clayId, CLAY_THEME_BTN_PRIMARY)
                {
                    CLAY_TEXT(self->playing ? CLAY_STRING("STOP") : CLAY_STRING("CONT"), CLAY_TEXT_CONFIG(buttonCfg));
                }
                Clay_ElementDeclaration followBtn = CLAY_THEME_BTN_PRIMARY;
                if (self->followCursor) followBtn.backgroundColor = CLAY_COLOR_BTN_SUCCESS;
                CLAY(self->followButton.clayId, followBtn)
                {
                    CLAY_TEXT(CLAY_STRING("FOLLOW"), CLAY_TEXT_CONFIG(buttonCfg));
                }
                CLAY(self->addPartButton.clayId, CLAY_THEME_BTN_PRIMARY)
                {
                    CLAY_TEXT(CLAY_STRING("+PART"), CLAY_TEXT_CONFIG(buttonCfg));
                }
                CLAY(self->saveSongButton.clayId, CLAY_THEME_BTN_PRIMARY)
                {
                    CLAY_TEXT(CLAY_STRING("SAVE"), CLAY_TEXT_CONFIG(buttonCfg));
                }
                CLAY(self->loadSongButton.clayId, CLAY_THEME_BTN_PRIMARY)
                {
                    CLAY_TEXT(CLAY_STRING("LOAD"), CLAY_TEXT_CONFIG(buttonCfg));
                }
                CLAY(self->songSettingsButton.clayId, CLAY_THEME_BTN_PRIMARY)
                {
                    CLAY_TEXT(CLAY_STRING("SONG"), CLAY_TEXT_CONFIG(buttonCfg));
                }
                CLAY(self->instrumentsButton.clayId, CLAY_THEME_BTN_PRIMARY)
                {
                    CLAY_TEXT(CLAY_STRING("INSTR."), CLAY_TEXT_CONFIG(buttonCfg));
                }
                Clay_ElementDeclaration oscBtn = CLAY_THEME_BTN_PRIMARY;
                if (self->oscilloscopeVisible) oscBtn.backgroundColor = CLAY_COLOR_BTN_SUCCESS;
                CLAY(self->oscilloscopeButton.clayId, oscBtn)
                {
                    CLAY_TEXT(CLAY_STRING("OSC"), CLAY_TEXT_CONFIG(buttonCfg));
                }
            }
            CLAY(
                CLAY_ID("TrackerStatusRow"),
                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                            .childGap = 5,
                            .layoutDirection = CLAY_LEFT_TO_RIGHT}}
            )
            {
                int selectedRows = Tracker_SelectedRowCount(self);
                int selectedChannels = Tracker_SelectedChannelCount(self);
                bool hasSelection = Tracker_HasSelection(self);

                Clay_ElementDeclaration copyBtn = CLAY_THEME_BTN_PRIMARY;
                Clay_ElementDeclaration cutBtn = CLAY_THEME_BTN_PRIMARY;
                Clay_ElementDeclaration pasteBtn = CLAY_THEME_BTN_PRIMARY;
                if (!hasSelection) copyBtn.backgroundColor = CLAY_COLOR_BTN_DISABLED;
                if (!hasSelection) cutBtn.backgroundColor = CLAY_COLOR_BTN_DISABLED;
                if (!Tracker_CanPaste(self)) pasteBtn.backgroundColor = CLAY_COLOR_BTN_DISABLED;
                CLAY(self->copyButton.clayId, copyBtn)
                {
                    CLAY_TEXT(CLAY_STRING("COPY"), CLAY_TEXT_CONFIG(buttonCfg));
                }
                CLAY(self->cutButton.clayId, cutBtn)
                {
                    CLAY_TEXT(CLAY_STRING("CUT"), CLAY_TEXT_CONFIG(buttonCfg));
                }
                CLAY(self->pasteButton.clayId, pasteBtn)
                {
                    CLAY_TEXT(CLAY_STRING("PASTE"), CLAY_TEXT_CONFIG(buttonCfg));
                }
                Clay_String selectionText = hasSelection ?
                    ClayArena_FormatString(arena, "%d rows selected (%d channels)", selectedRows, selectedChannels) :
                    CLAY_STRING("nothing selected");
                Clay_String clipboardText = self->clipboard.valid ?
                    ClayArena_FormatString(arena, "%d rows copied (%d channels copied)", self->clipboard.rows, self->clipboard.channels) :
                    CLAY_STRING("clipboard empty");
                Clay_String rightStatusText = self->songLoadStatus[0] ?
                    ClayArena_AllocString(arena, self->songLoadStatus) : clipboardText;
                CLAY(
                    CLAY_ID("TrackerSelectionStatus"),
                    {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER}},
                     .backgroundColor = {20, 22, 32, 255},
                     .cornerRadius = {4, 4, 4, 4}}
                )
                {
                    CLAY_TEXT(selectionText, CLAY_TEXT_CONFIG(bodyCfg));
                }
                CLAY(
                    CLAY_ID("TrackerClipboardStatus"),
                    {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER}},
                     .backgroundColor = {20, 22, 32, 255},
                     .cornerRadius = {4, 4, 4, 4}}
                )
                {
                    CLAY_TEXT(rightStatusText, CLAY_TEXT_CONFIG(bodyCfg));
                }
        }
    }
}

}

inline void Tracker_OpenEditor(Tracker *self, int row, int channel)
{
    if (!self) return;
    Tracker_RebuildUsedInstruments(self);
    self->editRow = std::max(0, std::min(row, self->rowCount - 1));
    self->editChannel = std::max(0, std::min(channel, TRACKER_CHANNELS - 1));
    Tracker_ParseCellForEditor(self);
    self->editorOpen = true;
    self->editorWindowRequested = true;
    self->editorTab = 0;
}

inline bool Tracker_SliderIdEquals(Clay_ElementId a, Clay_ElementId b)
{
    return a.id == b.id && a.offset == b.offset && a.baseId == b.baseId;
}

inline bool Tracker_SliderPointerEvent(const SDL_Event &e)
{
    return (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) ||
           (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) ||
           e.type == SDL_MOUSEMOTION;
}

inline float Tracker_SliderPointerX(const SDL_Event &e)
{
    return e.type == SDL_MOUSEMOTION ? (float)e.motion.x : (float)e.button.x;
}

inline bool Tracker_CapturedSlider(Tracker *self, Clay_ElementId id, const SDL_Event &e)
{
    if (!self || !Tracker_SliderPointerEvent(e)) return false;
    if (e.type == SDL_MOUSEBUTTONDOWN)
    {
        if (!Clay_PointerOver(id)) return false;
        self->sliderDragging = true;
        self->sliderActiveId = id;
        return true;
    }
    if (!self->sliderDragging || !Tracker_SliderIdEquals(self->sliderActiveId, id)) return false;
    return e.type == SDL_MOUSEMOTION || e.type == SDL_MOUSEBUTTONUP;
}

inline void Tracker_ClearSliderCaptureOnUp(Tracker *self, const SDL_Event &e)
{
    if (self && e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT)
    {
        self->sliderDragging = false;
        self->sliderActiveId = {};
    }
}

inline int Tracker_ValueFromSliderX(Clay_ElementId id, float pointerX, int minValue, int maxValue)
{
    Clay_BoundingBox b = Clay_GetElementData(id).boundingBox;
    float t = b.width > 0.0f ? (pointerX - b.x) / b.width : 0.0f;
    t = std::max(0.0f, std::min(1.0f, t));
    return minValue + (int)std::round(t * (float)(maxValue - minValue));
}

inline void Tracker_RequestEditorPreview(Tracker *self)
{
    if (self && self->editSpecial == 0)
        self->previewNoteRequested = true;
}

inline bool Tracker_EditorVirtualKeyAtPointer(Tracker *self, int *outOctave, int *outNote)
{
    if (!self) return false;
    for (int octave = 1; octave <= 7; octave++)
    {
        for (int note = 0; note < 12; note++)
        {
            if (
                Clay_PointerOver(CLAY_IDI("TrackerKey", octave * 100 + note))
                ||
                Clay_PointerOver(CLAY_IDI("TrackerWhiteKey", octave * 100 + note))
            )
            {
                if (outOctave) *outOctave = octave;
                if (outNote) *outNote = note;
                return true;
            }
        }
    }
    return false;
}

inline bool Tracker_HandleEditorWindowEvent(Tracker *self, const SDL_Event &e)
{
    if (!self || !self->editorOpen) return false;

    if (isClaytonClicked(&self->editorCloseButton, e) ||
        isClaytonClicked(&self->editorCancelButton, e))
    {
        if (self->virtualKeyPointerDown)
            self->previewHeldNoteStopRequested = true;
        self->virtualKeyPointerDown = false;
        self->editorOpen = false;
        return true;
    }
    if (isClaytonClicked(&self->editorNoteTabButton, e))
    {
        self->editorTab = 0;
        return true;
    }
    if (isClaytonClicked(&self->editorEffectsTabButton, e))
    {
        self->editorTab = 1;
        return true;
    }
    if (isClaytonClicked(&self->instrumentPrevButton, e))
    {
        self->editInstrument = Tracker_NextAvailableInstrument(self, self->editInstrument, -1);
        Tracker_NormalizeExplicitFields(self);
        Tracker_ApplyEditorToCell(self);
        Tracker_RequestEditorPreview(self);
        return true;
    }
    if (isClaytonClicked(&self->instrumentNextButton, e))
    {
        self->editInstrument = Tracker_NextAvailableInstrument(self, self->editInstrument, 1);
        Tracker_NormalizeExplicitFields(self);
        Tracker_ApplyEditorToCell(self);
        Tracker_RequestEditorPreview(self);
        return true;
    }
    if (isClaytonClicked(&self->instrumentNameButton, e))
    {
        self->instrumentEditorOpen = true;
        self->instrumentEditorWindowRequested = true;
        return true;
    }
    if (isClaytonClicked(&self->instrumentExplicitButton, e))
    {
        Tracker_ToggleEditorInstrumentExplicit(self);
        Tracker_ApplyEditorToCell(self);
        Tracker_RequestEditorPreview(self);
        return true;
    }
    if (isClaytonClicked(&self->volumeExplicitButton, e))
    {
        Tracker_ToggleEditorVolumeExplicit(self);
        Tracker_ApplyEditorToCell(self);
        Tracker_RequestEditorPreview(self);
        return true;
    }

    const bool pointerEvent =
        e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP || e.type == SDL_MOUSEMOTION ||
        e.type == SDL_MOUSEWHEEL || e.type == SDL_FINGERDOWN || e.type == SDL_FINGERUP ||
        e.type == SDL_FINGERMOTION;
    if (Tracker_SliderPointerEvent(e))
    {
        float pointerX = Tracker_SliderPointerX(e);
        if (Tracker_CapturedSlider(self, CLAY_ID("TrackerVolumeTrack"), e))
        {
            self->editVolume = Tracker_ValueFromSliderX(CLAY_ID("TrackerVolumeTrack"), pointerX, 0, 127);
            Tracker_NormalizeExplicitFields(self);
            Tracker_ApplyEditorToCell(self);
            if (e.type == SDL_MOUSEBUTTONUP)
                Tracker_RequestEditorPreview(self);
            Tracker_ClearSliderCaptureOnUp(self, e);
            return true;
        }
        if (Tracker_CapturedSlider(self, CLAY_ID("TrackerEffectParamABar"), e))
        {
            const TrackerEffectDef *def = &TRACKER_EFFECT_DEFS[Tracker_SelectedEffectDefIndex(self)];
            if (def->paramCount <= 0)
            {
                Tracker_ClearSliderCaptureOnUp(self, e);
                return true;
            }
            int value = Tracker_ValueFromSliderX(CLAY_ID("TrackerEffectParamABar"), pointerX, def->minA, def->maxA);
            Tracker_SetSelectedEffectValue(self, Tracker_EffectSetA(def, Tracker_SelectedEffectValue(self), value));
            Tracker_ApplyEditorToCell(self);
            if (e.type == SDL_MOUSEBUTTONUP)
                Tracker_RequestEditorPreview(self);
            Tracker_ClearSliderCaptureOnUp(self, e);
            return true;
        }
        if (Tracker_CapturedSlider(self, CLAY_ID("TrackerEffectParamBBar"), e))
        {
            const TrackerEffectDef *def = &TRACKER_EFFECT_DEFS[Tracker_SelectedEffectDefIndex(self)];
            if (def->paramCount <= 1)
            {
                Tracker_ClearSliderCaptureOnUp(self, e);
                return true;
            }
            int value = Tracker_ValueFromSliderX(CLAY_ID("TrackerEffectParamBBar"), pointerX, def->minB, def->maxB);
            Tracker_SetSelectedEffectValue(self, Tracker_EffectSetB(def, Tracker_SelectedEffectValue(self), value));
            Tracker_ApplyEditorToCell(self);
            if (e.type == SDL_MOUSEBUTTONUP)
                Tracker_RequestEditorPreview(self);
            Tracker_ClearSliderCaptureOnUp(self, e);
            return true;
        }
    }
    if (self->sliderDragging && e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT)
    {
        Tracker_ClearSliderCaptureOnUp(self, e);
        return true;
    }
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
    {
        int octave = 0;
        int note = 0;
        if (Tracker_EditorVirtualKeyAtPointer(self, &octave, &note))
        {
            self->editOctave = octave;
            self->editNote = note;
            self->editSpecial = 0;
            self->virtualKeyPointerDown = true;
            self->previewHeldNoteStartRequested = true;
            Tracker_ApplyEditorToCell(self);
            return true;
        }
        if (Clay_PointerOver(CLAY_ID("TrackerEffectActive")))
        {
            self->effectActivePointerDown = true;
            return true;
        }
    }
    if (self->effectActivePointerDown && e.type == SDL_MOUSEMOTION && !Clay_PointerOver(CLAY_ID("TrackerEffectActive")))
    {
        self->effectActivePointerDown = false;
        return true;
    }
    if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT)
    {
        if (self->virtualKeyPointerDown)
        {
            self->virtualKeyPointerDown = false;
            self->previewHeldNoteStopRequested = true;
            return true;
        }
        if (self->effectActivePointerDown)
        {
            self->effectActivePointerDown = false;
            if (Clay_PointerOver(CLAY_ID("TrackerEffectActive")))
            {
                Tracker_ToggleSelectedEffectActive(self);
                Tracker_ApplyEditorToCell(self);
            }
            return true;
        }
        if (Clay_PointerOver(CLAY_ID("TrackerSpecialDelete")))
        {
            Tracker_DeleteEditorCell(self);
            return true;
        }
        for (int i = 0; i < 4; i++)
        {
            if (Clay_PointerOver(CLAY_IDI("TrackerSpecial", i)))
            {
                self->editSpecial = i + 1;
                Tracker_ApplyEditorToCell(self);
                return true;
            }
        }
        bool effectPrevClicked = isClaytonClicked(&self->effectPrevButton, e) ||
            Clay_PointerOver(self->effectPrevButton.clayId);
        bool effectNextClicked = isClaytonClicked(&self->effectNextButton, e) ||
            Clay_PointerOver(self->effectNextButton.clayId);
        if (effectPrevClicked || effectNextClicked)
        {
            int dir = effectPrevClicked ? -1 : 1;
            self->editEffect = Tracker_NextEffectDefIndex(Tracker_SelectedEffectCode(self), dir);
            const TrackerEffectDef *def = &TRACKER_EFFECT_DEFS[Tracker_SelectedEffectDefIndex(self)];
            uint8_t clamped = Tracker_ClampEffectValueToDef(def, Tracker_SelectedEffectValue(self));
            Tracker_SetSelectedEffectValue(self, clamped);
            return true;
        }
    }
    if (pointerEvent && Clay_PointerOver(CLAY_ID("TrackerEditorWindow"))) return true;
    return pointerEvent;
}

inline bool Tracker_HandleInstrumentEditorWindowEvent(Tracker *self, const SDL_Event &e)
{
    if (!self || !self->instrumentEditorOpen) return false;
    xfm_patch_opn &patch = Tracker_EditablePatch(self);
    if (isClaytonClicked(&self->instrumentEditorCloseButton, e))
    {
        self->instrumentEditorOpen = false;
        return true;
    }
    if (isClaytonClicked(&self->instrumentPatchTabButton, e))
    {
        self->instrumentEditorTab = 0;
        return true;
    }
    if (isClaytonClicked(&self->instrumentEffectsTabButton, e))
    {
        self->instrumentEditorTab = 1;
        return true;
    }
    if (isClaytonClicked(&self->instrumentColorButton, e))
    {
        self->instrumentColorWindowOpen = true;
        self->instrumentColorWindowRequested = true;
        return true;
    }
    if (isClaytonClicked(&self->macroTargetPrevButton, e))
    {
        self->editMacroTarget--;
        if (self->editMacroTarget < XFM_MACRO_TL1) self->editMacroTarget = Tracker_MacroMaxTarget();
        self->editMacroValueIndex = 0;
        Tracker_SetMacroViewFirst(self, 0);
        self->macroViewAnimatedFirst = 0.0f;
        (void)Tracker_EditableMacro(self);
        return true;
    }
    if (isClaytonClicked(&self->macroTargetNextButton, e))
    {
        self->editMacroTarget++;
        if (self->editMacroTarget > Tracker_MacroMaxTarget()) self->editMacroTarget = XFM_MACRO_TL1;
        self->editMacroValueIndex = 0;
        Tracker_SetMacroViewFirst(self, 0);
        self->macroViewAnimatedFirst = 0.0f;
        (void)Tracker_EditableMacro(self);
        return true;
    }
    if (isClaytonClicked(&self->macroEnableButton, e))
    {
        int inst = std::max(0, std::min(255, self->editInstrument));
        int target = std::max((int)XFM_MACRO_TL1, std::min(Tracker_MacroMaxTarget(), self->editMacroTarget));
        (void)Tracker_EditableMacro(self);
        self->editMacroEnabled[inst][target] = !self->editMacroEnabled[inst][target];
        Tracker_MarkMacroDirty(self);
        return true;
    }
    if (isClaytonClicked(&self->macroScrollPrevButton, e))
    {
        Tracker_SetMacroViewFirst(self, self->macroViewFirst - TRACKER_MACRO_SCROLL_STEP);
        return true;
    }
    if (isClaytonClicked(&self->macroScrollNextButton, e))
    {
        Tracker_SetMacroViewFirst(self, self->macroViewFirst + TRACKER_MACRO_SCROLL_STEP);
        return true;
    }
    if (isClaytonClicked(&self->macroStepPrevButton, e))
    {
        XfmMacro &macro = Tracker_EditableMacro(self);
        self->editMacroValueIndex = (self->editMacroValueIndex + macro.length - 1) % macro.length;
        return true;
    }
    if (isClaytonClicked(&self->macroStepNextButton, e))
    {
        XfmMacro &macro = Tracker_EditableMacro(self);
        self->editMacroValueIndex = (self->editMacroValueIndex + 1) % macro.length;
        return true;
    }
    if (isClaytonClicked(&self->macroLoopButton, e))
    {
        XfmMacro &macro = Tracker_EditableMacro(self);
        if (!macro.has_loop)
        {
            macro.has_loop = true;
            macro.loop_start = (uint8_t)self->editMacroValueIndex;
        }
        else if (macro.loop_start != self->editMacroValueIndex)
        {
            macro.loop_start = (uint8_t)self->editMacroValueIndex;
        }
        else
        {
            macro.has_loop = false;
            macro.loop_start = 0;
        }
        Tracker_MarkMacroDirty(self);
        return true;
    }
    if (isClaytonClicked(&self->macroReleaseButton, e))
    {
        XfmMacro &macro = Tracker_EditableMacro(self);
        if (macro.release_start != self->editMacroValueIndex)
            macro.release_start = (uint8_t)self->editMacroValueIndex;
        else
            macro.release_start = 0xFF;
        Tracker_MarkMacroDirty(self);
        return true;
    }
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
    {
        for (int i = self->macroViewFirst; i < self->macroViewFirst + TRACKER_MACRO_VISIBLE_STEPS && i < TRACKER_MACRO_UI_STEPS; i++)
        {
            if (Clay_PointerOver(CLAY_IDI("TrackerMacroReset", i)))
            {
                XfmMacro &macro = Tracker_EditableMacro(self);
                Tracker_EnsureMacroUiLength(&macro);
                macro.values[i] = Tracker_MacroDefaultValue(self->editMacroTarget);
                self->editMacroValueIndex = i;
                Tracker_MarkMacroDirty(self);
                return true;
            }
            if (Clay_PointerOver(CLAY_IDI("TrackerMacroValueNumber", i)))
            {
                self->macroRangeSelecting = true;
                self->macroRangeAnchor = i;
                Tracker_SetMacroLoopRange(self, i, i);
                return true;
            }
        }
    }
    if (isClaytonClicked(&self->instrumentAlgoPrevButton, e))
    {
        patch.ALG = (uint8_t)((patch.ALG + 7) & 7);
        Tracker_MarkPatchDirty(self);
        return true;
    }
    if (isClaytonClicked(&self->instrumentAlgoNextButton, e))
    {
        patch.ALG = (uint8_t)((patch.ALG + 1) & 7);
        Tracker_MarkPatchDirty(self);
        return true;
    }
    for (int op = 0; op < 4; op++)
    {
        if (isClaytonClicked(&self->operatorButtons[op], e))
        {
            self->editOperator = op;
            self->operatorEditorOpen = true;
            self->operatorEditorWindowRequested = true;
            return true;
        }
    }
    const bool pointerEvent =
        e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP || e.type == SDL_MOUSEMOTION ||
        e.type == SDL_MOUSEWHEEL || e.type == SDL_FINGERDOWN || e.type == SDL_FINGERUP ||
        e.type == SDL_FINGERMOTION;
    const bool mouseSliderEvent =
        (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) ||
        (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) ||
        (e.type == SDL_MOUSEMOTION && (e.motion.state & SDL_BUTTON_LMASK));
    if (self->macroRangeSelecting &&
        (e.type == SDL_MOUSEMOTION || e.type == SDL_MOUSEBUTTONUP))
    {
        if (Clay_PointerOver(CLAY_ID("TrackerMacroNumbersClip")))
        {
            float pointerX = e.type == SDL_MOUSEMOTION ? (float)e.motion.x : (float)e.button.x;
            int i = Tracker_MacroVisibleIndexAtX(self, pointerX);
            Tracker_SetMacroLoopRange(self, self->macroRangeAnchor, i);
            if (e.type == SDL_MOUSEBUTTONUP)
                self->macroRangeSelecting = false;
            return true;
        }
        if (e.type == SDL_MOUSEBUTTONUP)
        {
            self->macroRangeSelecting = false;
            return true;
        }
    }
    if (Tracker_SliderPointerEvent(e))
    {
        float pointerX = Tracker_SliderPointerX(e);
        auto sliderValue = [&](Clay_ElementId id, int maxValue, int &out) -> bool {
            if (!Tracker_CapturedSlider(self, id, e)) return false;
            out = Tracker_ValueFromSliderX(id, pointerX, 0, maxValue);
            return true;
        };
        int value = 0;
        if (sliderValue(CLAY_ID("TrackerPatchFbBar"), 7, value))
        {
            patch.FB = (uint8_t)value;
            Tracker_MarkPatchDirty(self);
            Tracker_ClearSliderCaptureOnUp(self, e);
            return true;
        }
        if (sliderValue(CLAY_ID("TrackerPatchAmsBar"), 3, value))
        {
            patch.AMS = (uint8_t)value;
            Tracker_MarkPatchDirty(self);
            Tracker_ClearSliderCaptureOnUp(self, e);
            return true;
        }
        if (sliderValue(CLAY_ID("TrackerPatchFmsBar"), 7, value))
        {
            patch.FMS = (uint8_t)value;
            Tracker_MarkPatchDirty(self);
            Tracker_ClearSliderCaptureOnUp(self, e);
            return true;
        }
    }
    if (mouseSliderEvent)
    {
        float pointerX = e.type == SDL_MOUSEMOTION ? (float)e.motion.x : (float)e.button.x;
        XfmMacro &macro = Tracker_EditableMacro(self);
        Tracker_EnsureMacroUiLength(&macro);
        int target = std::max((int)XFM_MACRO_TL1, std::min(Tracker_MacroMaxTarget(), self->editMacroTarget));
        int valueMin = -64;
        int valueMax = 127;
        if (target >= XFM_MACRO_TL1 && target <= XFM_MACRO_TL4) valueMin = 0, valueMax = 127;
        else if (target >= XFM_MACRO_MUL1 && target <= XFM_MACRO_MUL4) valueMin = 0, valueMax = 15;
        else if (target >= XFM_MACRO_DT1 && target <= XFM_MACRO_DT4) valueMin = -3, valueMax = 3;
        else if (target >= XFM_MACRO_AR1 && target <= XFM_MACRO_SR4) valueMin = 0, valueMax = 31;
        else if (target >= XFM_MACRO_SL1 && target <= XFM_MACRO_RR4) valueMin = 0, valueMax = 15;
        else if (target >= XFM_MACRO_SSG1 && target <= XFM_MACRO_SSG4) valueMin = 0, valueMax = 8;
        else if (target == XFM_MACRO_FB) valueMin = 0, valueMax = 7;
        else if (target == XFM_MACRO_ARP) valueMin = -12, valueMax = 12;
        bool graphActive = self->macroDrawing || Clay_PointerOver(CLAY_ID("TrackerMacroGraphClip"));
        if (graphActive)
        {
            Clay_BoundingBox b = Clay_GetElementData(CLAY_ID("TrackerMacroGraphClip")).boundingBox;
            float pointerY = e.type == SDL_MOUSEMOTION ? (float)e.motion.y : (float)e.button.y;
            float yT = b.height > 0.0f ? (pointerY - b.y) / b.height : 0.0f;
            yT = std::max(0.0f, std::min(1.0f, yT));
            int idx = Tracker_MacroVisibleIndexAtX(self, pointerX);
            int drawn = valueMax - (int)std::round(yT * (float)(valueMax - valueMin));
            drawn = std::max(valueMin, std::min(valueMax, drawn));
            macro.values[idx] = (int16_t)drawn;
            macro.length = TRACKER_MACRO_UI_STEPS;
            self->editMacroValueIndex = idx;
            self->macroDrawing = e.type != SDL_MOUSEBUTTONUP;
            Tracker_MarkMacroDirty(self);
            return true;
        }
    }
    if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT)
    {
        Tracker_ClearSliderCaptureOnUp(self, e);
        self->macroDrawing = false;
        self->macroRangeSelecting = false;
    }
    if (pointerEvent && Clay_PointerOver(CLAY_ID("TrackerInstrumentEditorWindow"))) return true;
    return pointerEvent;
}

inline bool Tracker_HandleInstrumentColorWindowEvent(Tracker *self, const SDL_Event &e)
{
    if (!self || !self->instrumentColorWindowOpen) return false;
    if (isClaytonClicked(&self->instrumentColorCloseButton, e))
    {
        self->instrumentColorWindowOpen = false;
        return true;
    }
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
    {
        for (int idx = 0; idx < 64; idx++)
        {
            if (Clay_PointerOver(CLAY_IDI("TrackerInstrumentColorSwatch", idx)))
            {
                Tracker_SetInstrumentColor(self, self->editInstrument, TRACKER_INSTRUMENT_COLOR_PALETTE[idx]);
                self->instrumentColorWindowOpen = false;
                return true;
            }
        }
    }
    const bool pointerEvent =
        e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP || e.type == SDL_MOUSEMOTION ||
        e.type == SDL_MOUSEWHEEL || e.type == SDL_FINGERDOWN || e.type == SDL_FINGERUP ||
        e.type == SDL_FINGERMOTION;
    if (pointerEvent && Clay_PointerOver(CLAY_ID("TrackerInstrumentColorWindow"))) return true;
    return pointerEvent;
}

inline bool Tracker_HandleOperatorEditorWindowEvent(Tracker *self, const SDL_Event &e)
{
    if (!self || !self->operatorEditorOpen) return false;
    xfm_patch_opn &patch = Tracker_EditablePatch(self);
    int opIndex = std::max(0, std::min(3, self->editOperator));
    xfm_patch_opn_operator &op = patch.op[opIndex];

    if (isClaytonClicked(&self->operatorEditorCloseButton, e))
    {
        self->operatorEditorOpen = false;
        return true;
    }
    if (isClaytonClicked(&self->operatorSsgPrevButton, e))
    {
        op.SSG = (uint8_t)((op.SSG + 8) % 9);
        Tracker_MarkPatchDirty(self);
        return true;
    }
    if (isClaytonClicked(&self->operatorSsgNextButton, e))
    {
        op.SSG = (uint8_t)((op.SSG + 1) % 9);
        Tracker_MarkPatchDirty(self);
        return true;
    }
    if (isClaytonClicked(&self->operatorAmButton, e))
    {
        op.AM = op.AM ? 0 : 1;
        Tracker_MarkPatchDirty(self);
        return true;
    }

    const bool pointerEvent =
        e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP || e.type == SDL_MOUSEMOTION ||
        e.type == SDL_MOUSEWHEEL || e.type == SDL_FINGERDOWN || e.type == SDL_FINGERUP ||
        e.type == SDL_FINGERMOTION;
    if (Tracker_SliderPointerEvent(e))
    {
        float pointerX = Tracker_SliderPointerX(e);
        auto sliderValue = [&](Clay_ElementId id, int minValue, int maxValue, int &out) -> bool {
            if (!Tracker_CapturedSlider(self, id, e)) return false;
            out = Tracker_ValueFromSliderX(id, pointerX, minValue, maxValue);
            return true;
        };
        int value = 0;
        if (sliderValue(CLAY_ID("TrackerOpTlBar"), 0, 127, value)) op.TL = (uint8_t)value;
        else if (sliderValue(CLAY_ID("TrackerOpArBar"), 0, 31, value)) op.AR = (uint8_t)value;
        else if (sliderValue(CLAY_ID("TrackerOpDrBar"), 0, 31, value)) op.DR = (uint8_t)value;
        else if (sliderValue(CLAY_ID("TrackerOpSlBar"), 0, 15, value)) op.SL = (uint8_t)value;
        else if (sliderValue(CLAY_ID("TrackerOpSrBar"), 0, 31, value)) op.SR = (uint8_t)value;
        else if (sliderValue(CLAY_ID("TrackerOpRrBar"), 0, 15, value)) op.RR = (uint8_t)value;
        else if (sliderValue(CLAY_ID("TrackerOpMulBar"), 0, 15, value)) op.MUL = (uint8_t)value;
        else if (sliderValue(CLAY_ID("TrackerOpDtBar"), -3, 3, value)) op.DT = (int8_t)value;
        else if (sliderValue(CLAY_ID("TrackerOpRsBar"), 0, 3, value)) op.RS = (uint8_t)value;
        else value = -1000;
        if (value != -1000)
        {
            Tracker_MarkPatchDirty(self);
            Tracker_ClearSliderCaptureOnUp(self, e);
            return true;
        }
    }
    if (self->sliderDragging && e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT)
    {
        Tracker_ClearSliderCaptureOnUp(self, e);
        return true;
    }
    if (pointerEvent && Clay_PointerOver(CLAY_ID("TrackerOperatorEditorWindow"))) return true;
    return pointerEvent;
}

inline bool Tracker_HandleInstrumentsWindowEvent(Tracker *self, const SDL_Event &e)
{
    if (!self || !self->instrumentsWindowOpen) return false;

    bool pointerEvent =
        e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP || e.type == SDL_MOUSEMOTION ||
        e.type == SDL_MOUSEWHEEL || e.type == SDL_FINGERDOWN || e.type == SDL_FINGERUP ||
        e.type == SDL_FINGERMOTION;
    auto pointerY = [&]() -> float {
        if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP) return (float)e.button.y;
        if (e.type == SDL_MOUSEMOTION) return (float)e.motion.y;
        if (e.type == SDL_FINGERDOWN || e.type == SDL_FINGERUP || e.type == SDL_FINGERMOTION)
        {
            Clay_BoundingBox root = Clay_GetElementData(CLAY_ID("Root")).boundingBox;
            return root.y + e.tfinger.y * root.height;
        }
        return 0.0f;
    };

    if ((e.type == SDL_MOUSEMOTION || e.type == SDL_FINGERMOTION) && self->instrumentsDragging)
    {
        float y = pointerY();
        float dy = y - self->instrumentsDragLastY;
        if (std::fabs(y - self->instrumentsDragStartY) > 4.0f) self->instrumentsDragMoved = true;
        self->instrumentsScrollY -= dy;
        self->instrumentsScrollVelocity = -dy * 40.0f;
        self->instrumentsDragLastY = y;
        return true;
    }
    if ((e.type == SDL_MOUSEBUTTONUP || e.type == SDL_FINGERUP) && self->instrumentsDragging)
    {
        bool moved = self->instrumentsDragMoved;
        self->instrumentsDragging = false;
        if (moved) return true;
    }

    if (isClaytonClicked(&self->instrumentsCloseButton, e))
    {
        self->instrumentsWindowOpen = false;
        return true;
    }

    Clay_BoundingBox instrumentsRail = Clay_GetElementData(CLAY_ID("TrackerInstrumentsScrollbarRail")).boundingBox;
    Clay_BoundingBox instrumentsThumb = Clay_GetElementData(CLAY_ID("TrackerInstrumentsScrollbarThumb")).boundingBox;
    bool overInstrumentsRail = Clay_PointerOver(CLAY_ID("TrackerInstrumentsScrollbarRail"));
    if ((e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_FINGERDOWN) && overInstrumentsRail)
    {
        self->followCursor = false;
        self->instrumentsDragging = false;
        self->instrumentsDragMoved = false;
        self->instrumentsScrollbarDragging = true;
        float localY = pointerY() - instrumentsRail.y;
        if (Clay_PointerOver(CLAY_ID("TrackerInstrumentsScrollbarThumb")))
            self->instrumentsScrollbarGrabOffsetY = pointerY() - instrumentsThumb.y;
        else
            self->instrumentsScrollbarGrabOffsetY = Tracker_InstrumentsScrollbarThumbHeight(self) * 0.5f;
        Tracker_SetInstrumentsScrollFromScrollbarY(self, localY);
        return true;
    }
    if ((e.type == SDL_MOUSEMOTION || e.type == SDL_FINGERMOTION) && self->instrumentsScrollbarDragging)
    {
        float localY = pointerY() - instrumentsRail.y;
        Tracker_SetInstrumentsScrollFromScrollbarY(self, localY);
        return true;
    }
    if ((e.type == SDL_MOUSEBUTTONUP || e.type == SDL_FINGERUP) && self->instrumentsScrollbarDragging)
    {
        self->instrumentsScrollbarDragging = false;
        return true;
    }

    for (int inst = 0; inst < 256; inst++)
    {
        if (!self->availableInstruments[inst])
            continue;
        if (isClaytonClicked(&self->instrumentUpButtons[inst], e))
        {
            Tracker_MoveInstrument(self, inst, -1);
            return true;
        }
        if (isClaytonClicked(&self->instrumentDownButtons[inst], e))
        {
            Tracker_MoveInstrument(self, inst, 1);
            return true;
        }
        if (isClaytonClicked(&self->instrumentRowClicks[inst], e))
        {
            self->editInstrument = inst;
            return true;
        }
    }

    auto startInstrumentNameEdit = [&](int action, int source, int target, const char *name) -> bool {
        if (target < 0 || target > 255)
            return false;
        self->pendingInstrumentAction = action;
        self->pendingInstrument = source;
        self->pendingInstrumentTarget = target;
        std::snprintf(self->pendingInstrumentName, sizeof(self->pendingInstrumentName), "%s", name ? name : "");
        self->pendingInstrumentNameLen = (int32_t)std::strlen(self->pendingInstrumentName);
        return true;
    };

    int selectedInstrument = std::max(0, std::min(255, self->editInstrument));
    if (isClaytonClicked(&self->instrumentManagementCloneButton, e))
    {
        int target = Tracker_FirstFreeInstrumentSlot(self);
        if (target >= 0 && self->availableInstruments[selectedInstrument])
        {
            return startInstrumentNameEdit(1, selectedInstrument, target, Tracker_InstrumentName(self, selectedInstrument));
        }
        return true;
    }
    if (isClaytonClicked(&self->instrumentManagementEditButton, e))
    {
        if (self->availableInstruments[selectedInstrument])
        {
            self->editInstrument = selectedInstrument;
            self->instrumentEditorOpen = true;
            self->instrumentEditorWindowRequested = true;
        }
        return true;
    }
    if (isClaytonClicked(&self->instrumentManagementRenameButton, e))
    {
        if (self->availableInstruments[selectedInstrument])
        {
            return startInstrumentNameEdit(2, selectedInstrument, selectedInstrument, Tracker_InstrumentName(self, selectedInstrument));
        }
        return true;
    }
    if (isClaytonClicked(&self->instrumentManagementDeleteButton, e))
    {
        if (self->availableInstruments[selectedInstrument])
        {
            Tracker_DeleteInstrument(self, selectedInstrument);
            if (!self->availableInstruments[self->editInstrument] && self->availableInstrumentCount > 0)
                self->editInstrument = Tracker_NextAvailableInstrument(self, selectedInstrument, 1);
        }
        return true;
    }
    if (isClaytonClicked(&self->instrumentManagementNewButton, e))
    {
        int target = Tracker_FirstFreeInstrumentSlot(self);
        if (target >= 0)
            return startInstrumentNameEdit(3, selectedInstrument, target, Tracker_DefaultInstrumentName(target));
        return true;
    }

    if (e.type == SDL_MOUSEWHEEL && Clay_PointerOver(CLAY_ID("TrackerInstrumentsViewport")))
    {
        self->instrumentsScrollY = std::max(0.0f, self->instrumentsScrollY - e.wheel.y * 42.0f);
        self->instrumentsScrollVelocity = -e.wheel.y * 42.0f * 20.0f;
        return true;
    }
    if (e.type == SDL_MOUSEWHEEL && overInstrumentsRail)
    {
        self->instrumentsScrollY = std::max(0.0f, self->instrumentsScrollY - e.wheel.y * 42.0f);
        self->instrumentsScrollVelocity = -e.wheel.y * 42.0f * 20.0f;
        return true;
    }
    if ((e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_FINGERDOWN) &&
        Clay_PointerOver(CLAY_ID("TrackerInstrumentsViewport")))
    {
        float y = pointerY();
        self->instrumentsDragging = true;
        self->instrumentsDragMoved = false;
        self->instrumentsDragStartY = y;
        self->instrumentsDragLastY = y;
        self->instrumentsScrollVelocity = 0.0f;
        return true;
    }
    if (pointerEvent && Clay_PointerOver(CLAY_ID("TrackerInstrumentsWindow"))) return true;
    return pointerEvent;
}

inline bool Tracker_HandleSongSettingsWindowEvent(Tracker *self, const SDL_Event &e)
{
    if (!self || !self->songSettingsWindowOpen) return false;

    if (isClaytonClicked(&self->songSettingsCloseButton, e))
    {
        self->songSettingsWindowOpen = false;
        return true;
    }
    if (isClaytonClicked(&self->songNameButton, e))
    {
        std::snprintf(self->pendingSongName, sizeof(self->pendingSongName), "%s", self->songDisplayName);
        self->pendingSongNameLen = (int32_t)std::strlen(self->pendingSongName);
        self->pendingSongNameKeypadOpen = true;
        self->pendingSongNameKeypadActive = false;
        return true;
    }
    if (isClaytonClicked(&self->songLoadEmptyButton, e))
    {
        self->songLoadEmptyRequested = true;
        return true;
    }
    if (isClaytonClicked(&self->songLfoButton, e))
    {
        self->songLfoEnabled = !self->songLfoEnabled;
        self->patternDirty = true;
        self->copyOnWriteRequested = true;
        return true;
    }

    const bool pointerEvent =
        e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP || e.type == SDL_MOUSEMOTION ||
        e.type == SDL_MOUSEWHEEL || e.type == SDL_FINGERDOWN || e.type == SDL_FINGERUP ||
        e.type == SDL_FINGERMOTION;
    if (Tracker_SliderPointerEvent(e))
    {
        float pointerX = Tracker_SliderPointerX(e);
        auto sliderValue = [&](Clay_ElementId id, int minValue, int maxValue, int &out) -> bool {
            if (!Tracker_CapturedSlider(self, id, e)) return false;
            out = Tracker_ValueFromSliderX(id, pointerX, minValue, maxValue);
            return true;
        };
        int value = 0;
        bool tempoChanged = false;
        bool lfoChanged = false;
        bool cosmeticChanged = false;
        if (sliderValue(CLAY_ID("TrackerSongLfoFreqBar"), 0, 7, value))
        {
            self->songLfoFrequency = value;
            lfoChanged = true;
        }
        else if (sliderValue(CLAY_ID("TrackerSongTickRateBar"), 30, 300, value))
        {
            self->songTickRate = value;
            tempoChanged = true;
        }
        else if (sliderValue(CLAY_ID("TrackerSongSpeedBar"), 1, 32, value))
        {
            self->songSpeed = value;
            self->ticksPerRow = value;
            tempoChanged = true;
        }
        else if (sliderValue(CLAY_ID("TrackerSongRowsPerBeatBar"), 1, 16, value))
        {
            self->songRowsPerBeat = value;
            cosmeticChanged = true;
        }
        if (tempoChanged)
        {
            self->patternDirty = true;
            self->copyOnWriteRequested = true;
            Tracker_ClearSliderCaptureOnUp(self, e);
            return true;
        }
        if (lfoChanged)
        {
            self->patternDirty = true;
            self->copyOnWriteRequested = true;
            Tracker_ClearSliderCaptureOnUp(self, e);
            return true;
        }
        if (cosmeticChanged)
        {
            Tracker_ClearSliderCaptureOnUp(self, e);
            return true;
        }
    }
    if (self->sliderDragging && e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT)
    {
        Tracker_ClearSliderCaptureOnUp(self, e);
        return true;
    }
    if (pointerEvent && Clay_PointerOver(CLAY_ID("TrackerSongSettingsWindow"))) return true;
    return pointerEvent;
}

inline bool Tracker_HandlePartEditorWindowEvent(Tracker *self, const SDL_Event &e)
{
    if (!self || !self->partEditorOpen) return false;

    Tracker_NormalizeParts(self);
    if (self->partCount <= 0)
        return false;
    self->partEditorPart = std::max(0, std::min(self->partCount - 1, self->partEditorPart));
    int partIndex = self->partEditorPart;

    if (isClaytonClicked(&self->partEditorCloseButton, e))
    {
        self->partEditorOpen = false;
        self->partEditorPart = -1;
        return true;
    }
    if (isClaytonClicked(&self->partEditorNameButton, e))
    {
        std::snprintf(self->pendingPartName, sizeof(self->pendingPartName), "%s", self->parts[partIndex].name);
        self->pendingPartNameLen = (int32_t)std::strlen(self->pendingPartName);
        self->pendingPart = partIndex;
        self->pendingPartAction = 1;
        self->pendingPartNameKeypadOpen = true;
        self->pendingPartNameKeypadActive = false;
        return true;
    }
    if (isClaytonClicked(&self->partEditorEnableButton, e))
    {
        self->parts[partIndex].enabled = !self->parts[partIndex].enabled;
        self->patternDirty = true;
        self->copyOnWriteRequested = true;
        return true;
    }
    if (isClaytonClicked(&self->partEditorRowsMinusButton, e))
    {
        if (self->rowCount > 1)
        {
            Tracker_RemoveRowFromPart(self, partIndex);
            self->partEditorPart = std::max(0, std::min(self->partCount - 1, self->partEditorPart));
        }
        return true;
    }
    if (isClaytonClicked(&self->partEditorRowsPlusButton, e))
    {
        if (self->rowCount < TRACKER_MAX_ROWS)
        {
            Tracker_AddRowToPart(self, partIndex);
            self->partEditorPart = std::max(0, std::min(self->partCount - 1, self->partEditorPart));
        }
        return true;
    }
    if (isClaytonClicked(&self->partEditorDeleteButton, e))
    {
        if (self->partCount > 1)
        {
            Tracker_DeletePart(self, partIndex);
            self->partEditorOpen = false;
            self->partEditorPart = -1;
        }
        return true;
    }

    const bool pointerEvent =
        e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP || e.type == SDL_MOUSEMOTION ||
        e.type == SDL_MOUSEWHEEL || e.type == SDL_FINGERDOWN || e.type == SDL_FINGERUP ||
        e.type == SDL_FINGERMOTION;
    if (pointerEvent && Clay_PointerOver(CLAY_ID("TrackerPartEditorWindow"))) return true;
    return pointerEvent;
}

inline bool Tracker_HandleSaveConfirmWindowEvent(Tracker *self, const SDL_Event &e)
{
    if (!self || !self->songSaveConfirmWindowOpen) return false;

    if (isClaytonClicked(&self->saveConfirmSaveButton, e))
    {
        self->songSaveRequested = true;
        self->songSaveConfirmWindowOpen = false;
        return true;
    }
    if (isClaytonClicked(&self->saveConfirmChangeNameButton, e))
    {
        std::snprintf(self->pendingSongName, sizeof(self->pendingSongName), "%s", self->songDisplayName);
        self->pendingSongNameLen = (int32_t)std::strlen(self->pendingSongName);
        self->pendingSongNameKeypadOpen = true;
        self->pendingSongNameKeypadActive = false;
        return true;
    }
    if (isClaytonClicked(&self->saveConfirmCancelButton, e))
    {
        self->songSaveConfirmWindowOpen = false;
        return true;
    }

    const bool pointerEvent =
        e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP || e.type == SDL_MOUSEMOTION ||
        e.type == SDL_MOUSEWHEEL || e.type == SDL_FINGERDOWN || e.type == SDL_FINGERUP ||
        e.type == SDL_FINGERMOTION;
    if (pointerEvent && Clay_PointerOver(CLAY_ID("TrackerSaveConfirmWindow"))) return true;
    return pointerEvent;
}

inline bool Tracker_HandleLoadErrorWindowEvent(Tracker *self, const SDL_Event &e)
{
    if (!self || !self->songLoadErrorWindowOpen) return false;

    if (isClaytonClicked(&self->loadErrorOkButton, e))
    {
        self->songLoadErrorWindowOpen = false;
        return true;
    }

    const bool pointerEvent =
        e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP || e.type == SDL_MOUSEMOTION ||
        e.type == SDL_MOUSEWHEEL || e.type == SDL_FINGERDOWN || e.type == SDL_FINGERUP ||
        e.type == SDL_FINGERMOTION;
    if (pointerEvent && Clay_PointerOver(CLAY_ID("TrackerLoadErrorWindow"))) return true;
    return pointerEvent;
}

inline bool Tracker_HandleOscilloscopeEvent(Tracker *self, Clayton *clayton, const SDL_Event &e)
{
    if (!self || !self->active || !clayton || !self->oscilloscopeVisible)
        return false;

    bool pointerEvent =
        e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP || e.type == SDL_MOUSEMOTION ||
        e.type == SDL_FINGERDOWN || e.type == SDL_FINGERUP || e.type == SDL_FINGERMOTION;
    if (!pointerEvent)
        return false;

    Clay_BoundingBox root = Clay_GetElementData(CLAY_ID("Root")).boundingBox;
    Clay_BoundingBox portrait = Clay_GetElementData(CLAY_ID("Portrait area")).boundingBox;
    Clay_BoundingBox box = Tracker_OscilloscopeBox(self, portrait, root);
    if (box.width <= 1.0f || box.height <= 1.0f)
        return false;

    auto pointerX = [&]() -> float {
        if (e.type == SDL_FINGERDOWN || e.type == SDL_FINGERUP || e.type == SDL_FINGERMOTION)
            return root.x + e.tfinger.x * root.width;
        return e.type == SDL_MOUSEMOTION ? (float)e.motion.x : (float)e.button.x;
    };
    auto pointerY = [&]() -> float {
        if (e.type == SDL_FINGERDOWN || e.type == SDL_FINGERUP || e.type == SDL_FINGERMOTION)
            return root.y + e.tfinger.y * root.height;
        return e.type == SDL_MOUSEMOTION ? (float)e.motion.y : (float)e.button.y;
    };

    bool pointerDown = (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) || e.type == SDL_FINGERDOWN;
    bool pointerUp = (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) || e.type == SDL_FINGERUP;
    bool pointerMove = e.type == SDL_MOUSEMOTION || e.type == SDL_FINGERMOTION;
    float px = pointerX();
    float py = pointerY();
    uint64_t now = SDL_GetTicks64();
    if (now < self->oscilloscopeInputCooldownUntil && Tracker_PointInBox(px, py, box))
        return true;

    if (pointerDown && Tracker_PointInBox(px, py, box))
    {
        self->oscilloscopeDragging = !self->oscilloscopeMaximized;
        self->oscilloscopeDragMoved = false;
        self->oscilloscopeDragStartX = px;
        self->oscilloscopeDragStartY = py;
        self->oscilloscopeDragOffsetX = px - box.x;
        self->oscilloscopeDragOffsetY = py - box.y;
        return true;
    }

    if (pointerMove && self->oscilloscopeDragging)
    {
        float dx = px - self->oscilloscopeDragStartX;
        float dy = py - self->oscilloscopeDragStartY;
        if (std::fabs(dx) > 3.0f || std::fabs(dy) > 3.0f)
            self->oscilloscopeDragMoved = true;

        float normalW = std::max(1.0f, portrait.width);
        float normalH = Tracker_OscilloscopeNormalHeight(portrait);
        if (Tracker_PointInBox(px, py, portrait))
        {
            self->oscilloscopeSnappedToPortrait = true;
            self->oscilloscopeX = portrait.x;
            self->oscilloscopeY = Tracker_ClampFloat(py - self->oscilloscopeDragOffsetY, portrait.y, portrait.y + std::max(0.0f, portrait.height - normalH));
        }
        else
        {
            Clay_BoundingBox bounds = root.width > 1.0f && root.height > 1.0f ? root : portrait;
            self->oscilloscopeSnappedToPortrait = false;
            self->oscilloscopeX = Tracker_ClampFloat(px - self->oscilloscopeDragOffsetX, bounds.x, bounds.x + std::max(0.0f, bounds.width - normalW));
            self->oscilloscopeY = Tracker_ClampFloat(py - self->oscilloscopeDragOffsetY, bounds.y, bounds.y + std::max(0.0f, bounds.height - normalH));
        }
        return true;
    }

    if (pointerUp)
    {
        if (self->oscilloscopeDragging)
        {
            self->oscilloscopeDragging = false;
            if (!self->oscilloscopeDragMoved)
            {
                self->oscilloscopeMaximized = true;
                self->oscilloscopeInputCooldownUntil = now + 220;
            }
            return true;
        }
        if (self->oscilloscopeMaximized && Tracker_PointInBox(px, py, box))
        {
            int channel = Tracker_OscilloscopeChannelAtPoint(box, true, px, py);
            if (channel >= 0)
                self->oscilloscopeSelectedChannel = channel;
            self->oscilloscopeMaximized = false;
            self->oscilloscopeInputCooldownUntil = now + 220;
            return true;
        }
        return Tracker_PointInBox(px, py, box);
    }

    return Tracker_PointInBox(px, py, box);
}

inline bool Tracker_HandleEvent(Tracker *self, Clayton *clayton, const SDL_Event &e)
{
    if (!self || !self->active) return false;

    if (Tracker_HandleOscilloscopeEvent(self, clayton, e))
        return true;

    if (isClaytonClicked(&self->closeButton, e))
    {
        Tracker_Close(self);
        return true;
    }
    if (isClaytonClicked(&self->playButton, e))
    {
        int startRow = self->loopEnabled ? self->loopStart : 0;
        setTrackerCursorState(self, startRow, 0, self->ticksPerRow);
        self->playing = true;
        self->musicStartRequested = true;
        return true;
    }
    if (isClaytonClicked(&self->stopButton, e))
    {
        if (self->playing)
        {
            self->playing = false;
            self->musicStopRequested = true;
        }
        else
        {
            self->playing = true;
            self->musicPlayRequested = true;
        }
        return true;
    }
    if (isClaytonClicked(&self->followButton, e))
    {
        self->followCursor = !self->followCursor;
        if (self->followCursor)
            setTrackerCursorState(self, self->playRow, self->playTick, self->ticksPerRow);
        return true;
    }
    if (isClaytonClicked(&self->clearLoopButton, e))
    {
        Tracker_ClearLoopRange(self);
        return true;
    }
    if (isClaytonClicked(&self->addPartButton, e))
    {
        int insertAfter = Tracker_CurrentPartIndex(self);
        Tracker_AddPartAfter(self, insertAfter);
        int newPartIndex = std::max(0, std::min(self->partCount - 1, insertAfter + 1));
        for (int i = 1; i < 32 && self->rowCount < TRACKER_MAX_ROWS; i++)
            Tracker_AddRowToPart(self, newPartIndex);
        return true;
    }
    if (isClaytonClicked(&self->oscilloscopeButton, e))
    {
        self->oscilloscopeVisible = !self->oscilloscopeVisible;
        self->oscilloscopeDragging = false;
        self->oscilloscopeDragMoved = false;
        if (self->oscilloscopeVisible && !self->oscilloscopeInitialized)
            self->oscilloscopeSnappedToPortrait = true;
        return true;
    }
    if (isClaytonClicked(&self->saveSongButton, e))
    {
        self->songSaveConfirmWindowOpen = true;
        self->songSaveConfirmWindowRequested = true;
        return true;
    }
    if (isClaytonClicked(&self->loadSongButton, e))
    {
        std::snprintf(self->songLoadStatus, sizeof(self->songLoadStatus), "Opening file...");
        self->songLoadErrorText[0] = '\0';
        self->songLoadErrorWindowOpen = false;
        self->songLoadRequested = true;
        return true;
    }
    if (isClaytonClicked(&self->songSettingsButton, e))
    {
        self->songSettingsWindowOpen = true;
        self->songSettingsWindowRequested = true;
        return true;
    }
    if (isClaytonClicked(&self->instrumentsButton, e))
    {
        self->instrumentsWindowOpen = true;
        self->instrumentsWindowRequested = true;
        return true;
    }
    int stickyPart = Tracker_StickyPartIndexAtScroll(self);
    if (stickyPart >= 0 && stickyPart < self->partCount)
    {
        if (isClaytonClicked(&self->stickyPartToggleButton, e))
        {
            Tracker_TogglePartCollapsed(self, stickyPart);
            return true;
        }
        if (isClaytonClicked(&self->stickyPartUpButton, e))
        {
            Tracker_MovePart(self, stickyPart, -1);
            return true;
        }
        if (isClaytonClicked(&self->stickyPartDownButton, e))
        {
            Tracker_MovePart(self, stickyPart, 1);
            return true;
        }
        if (isClaytonClicked(&self->stickyPartSettingsButton, e))
        {
            Tracker_OpenPartEditor(self, stickyPart);
            return true;
        }
    }
    for (int i = 0; i < self->partCount; i++)
    {
        if (isClaytonClicked(&self->partToggleButtons[i], e))
        {
            Tracker_TogglePartCollapsed(self, i);
            return true;
        }
        if (isClaytonClicked(&self->partUpButtons[i], e))
        {
            Tracker_MovePart(self, i, -1);
            return true;
        }
        if (isClaytonClicked(&self->partDownButtons[i], e))
        {
            Tracker_MovePart(self, i, 1);
            return true;
        }
        if (isClaytonClicked(&self->partSettingsButtons[i], e))
        {
            Tracker_OpenPartEditor(self, i);
            return true;
        }
    }
    if (isClaytonClicked(&self->copyButton, e))
    {
        Tracker_CopySelection(self);
        return true;
    }
    if (isClaytonClicked(&self->cutButton, e))
    {
        Tracker_CutSelection(self);
        return true;
    }
    if (isClaytonClicked(&self->pasteButton, e))
    {
        Tracker_PasteSelection(self);
        return true;
    }
    for (int i = 0; i < TRACKER_MAX_SONG_COUNT; i++)
    {
        if (isClaytonClicked(&self->songButtons[i], e))
        {
            Tracker_LoadSong(self, i + 1);
            return true;
        }
    }

    bool pointerEvent =
        e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP || e.type == SDL_MOUSEMOTION ||
        e.type == SDL_MOUSEWHEEL || e.type == SDL_FINGERDOWN || e.type == SDL_FINGERUP ||
        e.type == SDL_FINGERMOTION;
    if (!pointerEvent) return false;
    Clay_BoundingBox root = Clay_GetElementData(CLAY_ID("Portrait area")).boundingBox;
    auto pointerX = [&]() -> float {
        if (e.type == SDL_FINGERDOWN || e.type == SDL_FINGERUP || e.type == SDL_FINGERMOTION)
            return root.x + e.tfinger.x * root.width;
        return e.type == SDL_MOUSEMOTION ? (float)e.motion.x : (float)e.button.x;
    };
    auto pointerY = [&]() -> float {
        if (e.type == SDL_FINGERDOWN || e.type == SDL_FINGERUP || e.type == SDL_FINGERMOTION)
            return root.y + e.tfinger.y * root.height;
        return e.type == SDL_MOUSEMOTION ? (float)e.motion.y : (float)e.button.y;
    };
    bool pointerDown = (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) || e.type == SDL_FINGERDOWN;
    bool pointerUp = (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) || e.type == SDL_FINGERUP;
    bool pointerMove = e.type == SDL_MOUSEMOTION || e.type == SDL_FINGERMOTION;

    auto pointerOverPartButton = [&]() -> bool {
        if (Clay_PointerOver(self->stickyPartToggleButton.clayId) ||
            Clay_PointerOver(self->stickyPartUpButton.clayId) ||
            Clay_PointerOver(self->stickyPartDownButton.clayId) ||
            Clay_PointerOver(self->stickyPartSettingsButton.clayId))
            return true;
        for (int i = 0; i < self->partCount; i++)
        {
            if (Clay_PointerOver(self->partToggleButtons[i].clayId) ||
                Clay_PointerOver(self->partUpButtons[i].clayId) ||
                Clay_PointerOver(self->partDownButtons[i].clayId) ||
                Clay_PointerOver(self->partSettingsButtons[i].clayId))
                return true;
        }
        return false;
    };
    if ((pointerDown || pointerMove || pointerUp) && pointerOverPartButton())
        return true;

    Clay_BoundingBox header = Clay_GetElementData(CLAY_ID("TrackerFixedHeader")).boundingBox;
    auto channelAtHeaderX = [&](float x) -> int {
        if (header.width <= 0.0f) return -1;
        float unit = header.width / 13.0f;
        float localX = x - header.x;
        int channel = (int)std::floor((localX - unit) / (unit * 2.0f));
        return channel >= 0 && channel < TRACKER_CHANNELS ? channel : -1;
    };
    bool overHeader = Clay_PointerOver(CLAY_ID("TrackerFixedHeader"));
    if (pointerDown && overHeader)
    {
        int channel = channelAtHeaderX(pointerX());
        if (channel >= 0)
        {
            self->channelSelecting = true;
            self->channelAnchor = channel;
            Tracker_SetChannelSelection(self, channel, channel);
            return true;
        }
    }
    if (pointerMove && self->channelSelecting)
    {
        int channel = channelAtHeaderX(pointerX());
        if (channel >= 0)
            Tracker_SetChannelSelection(self, self->channelAnchor, channel);
        return true;
    }
    if (pointerUp && self->channelSelecting)
    {
        int channel = channelAtHeaderX(pointerX());
        if (channel >= 0)
            Tracker_SetChannelSelection(self, self->channelAnchor, channel);
        self->channelSelecting = false;
        return true;
    }

    Clay_BoundingBox grid = Clay_GetElementData(CLAY_ID("TrackerGridViewport")).boundingBox;
    bool overGrid = Clay_PointerOver(CLAY_ID("TrackerGridViewport"));
    Clay_BoundingBox scrollbar = Clay_GetElementData(CLAY_ID("TrackerScrollbarRail")).boundingBox;
    Clay_BoundingBox thumb = Clay_GetElementData(CLAY_ID("TrackerScrollbarThumb")).boundingBox;
    bool overScrollbar = Clay_PointerOver(CLAY_ID("TrackerScrollbarRail"));
    if (pointerDown && overScrollbar)
    {
        self->followCursor = false;
        self->dragging = false;
        self->scrollbarDragging = true;
        float localY = pointerY() - scrollbar.y;
        if (Clay_PointerOver(CLAY_ID("TrackerScrollbarThumb")))
            self->scrollbarGrabOffsetY = pointerY() - thumb.y;
        else
            self->scrollbarGrabOffsetY = Tracker_ScrollbarThumbHeight(self) * 0.5f;
        Tracker_SetScrollFromScrollbarY(self, localY);
        return true;
    }
    if (pointerMove && self->scrollbarDragging)
    {
        float localY = pointerY() - scrollbar.y;
        Tracker_SetScrollFromScrollbarY(self, localY);
        return true;
    }
    if (pointerUp && self->scrollbarDragging)
    {
        self->scrollbarDragging = false;
        Tracker_SnapToGrid(self);
        return true;
    }
    if (e.type == SDL_MOUSEWHEEL && overScrollbar)
    {
        self->followCursor = false;
        self->scrollY -= (float)e.wheel.y * self->rowHeight * 2.0f;
        Tracker_SnapToGrid(self);
        return true;
    }
    if (e.type == SDL_MOUSEWHEEL && overGrid)
    {
        self->followCursor = false;
        self->scrollY -= (float)e.wheel.y * self->rowHeight * 2.0f;
        Tracker_SnapToGrid(self);
        return true;
    }
    auto cellAtGridPoint = [&](float x, float y, int *outRow, int *outChannel) -> bool {
        if (grid.width <= 0.0f || grid.height <= 0.0f) return false;
        float localX = x - grid.x;
        float localY = y - grid.y;
        float unit = grid.width / 13.0f;
        int channel = (int)std::floor((localX - unit) / (unit * 2.0f));
        TrackerVisualRow visual = Tracker_MapVisualIndex(self, Tracker_VisualIndexAtViewportY(self, localY));
        int row = visual.row;
        if (outRow) *outRow = row;
        if (outChannel) *outChannel = channel;
        return visual.kind == TRACKER_VISUAL_ROW_CELL && channel >= 0 && channel < TRACKER_CHANNELS && row >= 0 && row < self->rowCount;
    };
    if (pointerDown && overGrid)
    {
        float px = pointerX();
        float py = pointerY();
        float localX = px - grid.x;
        float localY = py - grid.y;
        float unit = grid.width / 13.0f;
        if (localX >= 0.0f && localX < unit)
        {
            TrackerVisualRow visual = Tracker_MapVisualIndex(self, Tracker_VisualIndexAtViewportY(self, localY));
            if (visual.kind != TRACKER_VISUAL_ROW_CELL)
                return true;
            int row = visual.row;
            self->followCursor = false;
            self->loopSelecting = false;
            self->loopMoving = false;
            self->dragging = false;
            self->dragMoved = true;
            self->loopSelectLocalY = localY;
            self->loopSelectViewportHeight = grid.height;
            if (self->loopEnabled && row > self->loopStart && row < self->loopEnd)
            {
                self->loopMoving = true;
                self->loopMoveGrabOffset = row - self->loopStart;
                self->loopMoveLength = std::max(1, self->loopEnd - self->loopStart + 1);
                Tracker_MoveLoopRangeToGrabbedRow(self, row);
            }
            else
            {
                self->loopSelecting = true;
                if (self->loopEnabled && row == self->loopStart && self->loopEnd > self->loopStart)
                    self->loopAnchor = self->loopEnd;
                else if (self->loopEnabled && row == self->loopEnd && self->loopEnd > self->loopStart)
                    self->loopAnchor = self->loopStart;
                else
                    self->loopAnchor = row;
                Tracker_SetLoopRange(self, self->loopAnchor, row);
            }
            return true;
        }
        int row = -1;
        int channel = -1;
        if (cellAtGridPoint(px, py, &row, &channel) && Tracker_CellMoveCanStart(self, row, channel))
        {
            self->followCursor = false;
            self->dragging = false;
            self->dragMoved = false;
            self->dragStartY = py;
            self->dragLastY = py;
            self->scrollVelocity = 0.0f;
            Tracker_BeginCellMove(self, row, channel);
            return true;
        }
        self->followCursor = false;
        self->dragging = true;
        self->dragMoved = false;
        self->dragStartY = py;
        self->dragLastY = py;
        self->dragStartScrollY = self->scrollY;
        self->scrollVelocity = 0.0f;
        return true;
    }
    if (pointerMove && self->loopSelecting)
    {
        float localY = pointerY() - grid.y;
        self->loopSelectLocalY = localY;
        self->loopSelectViewportHeight = grid.height;
        int row = Tracker_RowAtViewportY(self, localY);
        Tracker_SetLoopRange(self, self->loopAnchor, row);
        return true;
    }
    if (pointerMove && self->loopMoving)
    {
        float localY = pointerY() - grid.y;
        self->loopSelectLocalY = localY;
        self->loopSelectViewportHeight = grid.height;
        int row = Tracker_RowAtViewportY(self, localY);
        Tracker_MoveLoopRangeToGrabbedRow(self, row);
        return true;
    }
    if (pointerUp && self->loopSelecting)
    {
        float localY = pointerY() - grid.y;
        self->loopSelectLocalY = localY;
        self->loopSelectViewportHeight = grid.height;
        int row = Tracker_RowAtViewportY(self, localY);
        Tracker_SetLoopRange(self, self->loopAnchor, row);
        self->loopSelecting = false;
        Tracker_SnapToGrid(self);
        return true;
    }
    if (pointerUp && self->loopMoving)
    {
        float localY = pointerY() - grid.y;
        self->loopSelectLocalY = localY;
        self->loopSelectViewportHeight = grid.height;
        int row = Tracker_RowAtViewportY(self, localY);
        Tracker_MoveLoopRangeToGrabbedRow(self, row);
        self->loopMoving = false;
        Tracker_SnapToGrid(self);
        return true;
    }
    if (pointerMove && self->cellMoving)
    {
        float px = pointerX();
        float py = pointerY();
        int row = -1;
        int channel = -1;
        if (!cellAtGridPoint(px, py, &row, &channel))
        {
            row = -1;
            channel = -1;
        }
        Tracker_UpdateCellMoveHover(self, row, channel);
        if (std::fabs(py - self->dragStartY) > 4.0f) self->dragMoved = true;
        return true;
    }
    if (pointerUp && self->cellMoving)
    {
        float px = pointerX();
        float py = pointerY();
        int row = -1;
        int channel = -1;
        if (!cellAtGridPoint(px, py, &row, &channel))
        {
            row = -1;
            channel = -1;
        }
        Tracker_UpdateCellMoveHover(self, row, channel);
        bool moved = Tracker_CommitCellMove(self);
        if (!moved)
        {
            bool tapSource = !self->dragMoved &&
                row == self->cellMoveSourceRow &&
                channel == self->cellMoveSourceChannel;
            int sourceRow = self->cellMoveSourceRow;
            int sourceChannel = self->cellMoveSourceChannel;
            Tracker_CancelCellMove(self);
            if (tapSource)
                Tracker_OpenEditor(self, sourceRow, sourceChannel);
        }
        Tracker_SnapToGrid(self);
        return true;
    }
    if (pointerMove && self->dragging)
    {
        float y = pointerY();
        float dy = y - self->dragLastY;
        if (std::fabs(y - self->dragStartY) > 4.0f) self->dragMoved = true;
        self->scrollY -= dy;
        self->scrollVelocity = -dy * 40.0f;
        self->dragLastY = y;
        return true;
    }
    if (pointerUp && self->dragging)
    {
        self->dragging = false;
        if (!self->dragMoved && grid.width > 0.0f && grid.height > 0.0f)
        {
            int row = -1;
            int channel = -1;
            if (cellAtGridPoint(pointerX(), pointerY(), &row, &channel))
                Tracker_OpenEditor(self, row, channel);
        }
        Tracker_SnapToGrid(self);
        return true;
    }

    if (overGrid) return true;
    return false;
}
