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

inline Clay_Color Tracker_EditSelectionBorderColor(const char *cell, uint32_t displayColor)
{
    if (Tracker_CellIsEmpty(cell))
        return {244, 216, 70, 255};
    if (displayColor != 0 && Tracker_ColorIsBright(displayColor))
        return {110, 82, 0, 255};
    return {244, 216, 70, 255};
}

inline Clay_Color Tracker_ApplyZebraTint(Clay_Color color, bool darkBand)
{
    const float amount = darkBand ? -6.0f : 6.0f;
    color.r = std::max(0.0f, std::min(255.0f, color.r + amount));
    color.g = std::max(0.0f, std::min(255.0f, color.g + amount));
    color.b = std::max(0.0f, std::min(255.0f, color.b + amount));
    return color;
}

inline Clay_Color Tracker_ChangeFlashColor(uint8_t kind)
{
    if (kind == TRACKER_CHANGE_FLASH_ADD)
        return {90, 244, 176, 255};
    return {255, 192, 42, 255};
}

inline Clay_Color Tracker_ApplyChangeFlashTint(Clay_Color base, float flashAlpha, uint8_t kind, float timeLeft = -1.0f)
{
    if (flashAlpha <= 0.0f || kind == TRACKER_CHANGE_FLASH_NONE)
        return base;
    Clay_Color flash = Tracker_ChangeFlashColor(kind);
    if (timeLeft >= 0.0f)
    {
        const float remaining01 = std::max(0.0f, std::min(1.0f, timeLeft / TRACKER_CHANGE_FLASH_DURATION_S));
        const float elapsed01 = 1.0f - remaining01;
        const float startWhite = std::max(0.0f, std::min(1.0f, 1.0f - elapsed01 / 0.28f));
        const float endWhite = std::max(0.0f, std::min(1.0f, 1.0f - remaining01 / 0.32f));
        const float whiteMix = std::max(startWhite, endWhite);
        flash.r = flash.r + (255.0f - flash.r) * whiteMix;
        flash.g = flash.g + (255.0f - flash.g) * whiteMix;
        flash.b = flash.b + (255.0f - flash.b) * whiteMix;
    }
    const float mix = std::max(0.0f, std::min(0.86f, flashAlpha * 0.86f));
    base.r = base.r + (flash.r - base.r) * mix;
    base.g = base.g + (flash.g - base.g) * mix;
    base.b = base.b + (flash.b - base.b) * mix;
    base.a = std::max(base.a, std::min(255.0f, base.a + flashAlpha * 48.0f));
    return base;
}

inline Clay_Color Tracker_ButtonHoverColor(Clay_ElementId id, Clay_Color base, float rgbLift = 24.0f, float alphaLift = 0.0f)
{
    return Clay_PointerOver(id) ? CLAY_THEME_HOVER_COLOR(base, rgbLift, alphaLift) : base;
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
    float contentHeight = Tracker_ContentHeight(self);
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
    bool renderProgressFill,
    int16_t floatingLayerZ,
    bool floatTitleText,
    Clayton_Click *toggleButton,
    Clayton_Click *enableButton,
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
    toggleBtn.backgroundColor = Tracker_ButtonHoverColor(toggleButton->clayId, CLAY_COLOR_BTN_PRIMARY);
    CLAY(toggleButton->clayId, toggleBtn)
    {
        CLAY_TEXT(Tracker_PartCollapseIconShowsCollapsed(self, partIndex) ? CLAY_STRING("+") : CLAY_STRING("-"), CLAY_TEXT_CONFIG(buttonCfg));
    }

    Clay_ElementDeclaration enableBtn = CLAY_THEME_BTN_BOX;
    enableBtn.layout.sizing = {CLAY_SIZING_FIXED(42), CLAY_SIZING_FIXED(26)};
    enableBtn.backgroundColor = Tracker_ButtonHoverColor(
        enableButton->clayId,
        part.enabled ? CLAY_COLOR_BTN_SUCCESS : CLAY_COLOR_BTN_DISABLED,
        part.enabled ? 18.0f : 10.0f
    );
    CLAY(enableButton->clayId, enableBtn)
    {
        CLAY_TEXT(part.enabled ? CLAY_STRING("ON") : CLAY_STRING("OFF"), CLAY_TEXT_CONFIG(buttonCfg));
    }

    int titleKey = partIndex * 2 + (toggleButton == &self->stickyPartToggleButton ? 1 : 0);
    Clay_String partLabel = ClayArena_FormatString(arena, "%s (%d)", part.name, part.rowCount);
    CLAY(
        CLAY_IDI("TrackerPartProgressRail", titleKey),
        {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                    .padding = {6, 6, 3, 3},
                    .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER}},
         .backgroundColor = {7, 10, 16, 220},
         .cornerRadius = {4, 4, 4, 4}}
    )
    {
        if (renderProgressFill && progress > 0.0f)
        {
            CLAY(
                CLAY_IDI("TrackerPartProgressFill", titleKey),
                {.layout = {.sizing = {CLAY_SIZING_PERCENT(std::max(0.0f, std::min(1.0f, progress))), CLAY_SIZING_GROW()}},
                 .backgroundColor = {94, 196, 228, 180},
                 .cornerRadius = {4, 4, 4, 4},
                 .floating = {
                     .zIndex = floatingLayerZ,
                     .attachPoints = {CLAY_ATTACH_POINT_LEFT_CENTER, CLAY_ATTACH_POINT_LEFT_CENTER},
                     .pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH,
                     .attachTo = CLAY_ATTACH_TO_PARENT,
                     .clipTo = CLAY_CLIP_TO_ATTACHED_PARENT,
                 }}
            ) {}
        }
        Clay_ElementDeclaration titleTextDecl = {
            .layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                       .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER}}
        };
        if (floatTitleText)
        {
            titleTextDecl.floating = {
                .zIndex = (int16_t)(floatingLayerZ + 1),
                .attachPoints = {CLAY_ATTACH_POINT_CENTER_CENTER, CLAY_ATTACH_POINT_CENTER_CENTER},
                .pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH,
                .attachTo = CLAY_ATTACH_TO_PARENT,
                .clipTo = CLAY_CLIP_TO_ATTACHED_PARENT,
            };
        }
        CLAY(CLAY_IDI("TrackerPartTitleText", titleKey), titleTextDecl)
        {
            CLAY_TEXT(partLabel, CLAY_TEXT_CONFIG(bodyCfg));
        }
    }

    Clay_ElementDeclaration smallBtn = CLAY_THEME_BTN_PRIMARY;
    smallBtn.layout.sizing = {CLAY_SIZING_FIXED(34), CLAY_SIZING_FIXED(26)};
    smallBtn.backgroundColor = Tracker_ButtonHoverColor(upButton->clayId, CLAY_COLOR_BTN_PRIMARY);
    CLAY(upButton->clayId, smallBtn) { CLAY_TEXT(CLAY_STRING("▲"), CLAY_TEXT_CONFIG(buttonCfg)); }
    smallBtn.backgroundColor = Tracker_ButtonHoverColor(downButton->clayId, CLAY_COLOR_BTN_PRIMARY);
    CLAY(downButton->clayId, smallBtn) { CLAY_TEXT(CLAY_STRING("▼"), CLAY_TEXT_CONFIG(buttonCfg)); }
    Clay_ElementDeclaration partBtn = CLAY_THEME_BTN_PRIMARY;
    partBtn.layout.sizing = {CLAY_SIZING_FIXED(48), CLAY_SIZING_FIXED(26)};
    partBtn.backgroundColor = Tracker_ButtonHoverColor(partButton->clayId, CLAY_COLOR_BTN_PRIMARY);
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
                        .padding = {20, 20, 0, 0},
                        .childGap = 8,
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_BOTTOM},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
        )
        {
            Clay_ElementDeclaration tab = CLAY_THEME_BTN_PRIMARY;
            tab.backgroundColor = Tracker_ButtonHoverColor(
                self->editorNoteTabButton.clayId,
                self->editorTab == 0 ? CLAY_COLOR_PANEL_SECTION : CLAY_COLOR_BTN_PRIMARY,
                self->editorTab == 0 ? 16.0f : 24.0f
            );
            tab.cornerRadius.bottomLeft = 0;
            tab.cornerRadius.bottomRight = 0;

            CLAY(self->editorNoteTabButton.clayId, tab)
            {
                CLAY_TEXT(CLAY_STRING("NOTE"), CLAY_TEXT_CONFIG(buttonCfg));
            }
            tab.backgroundColor = Tracker_ButtonHoverColor(
                self->editorEffectsTabButton.clayId,
                self->editorTab == 1 ? CLAY_COLOR_PANEL_SECTION : CLAY_COLOR_BTN_PRIMARY,
                self->editorTab == 1 ? 16.0f : 24.0f
            );

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
                            shortBtn.backgroundColor = Tracker_ButtonHoverColor(self->instrumentPrevButton.clayId, CLAY_COLOR_BTN_PRIMARY);
                            CLAY(self->instrumentPrevButton.clayId, shortBtn)
                            {
                                CLAY_TEXT(CLAY_STRING("◀"), CLAY_TEXT_CONFIG(buttonCfg));
                            }
                            bool instrumentUsed = Tracker_InstrumentUsedInSong(self, self->editInstrument);
                            Clay_String name = ClayArena_FormatString(
                                arena,
                                "%02X: %s",
                                self->editInstrument,
                                Tracker_InstrumentName(self, self->editInstrument)
                            );
                            Clay_Color instrumentTextColor = instrumentUsed ? CLAY_COLOR_TEXT_PRIMARY : Clay_Color{145, 151, 164, 255};

                            uint32_t instColorRgb = Tracker_InstrumentColorU32(self, self->editInstrument);
                            Clay_ElementDeclaration colorBtn = CLAY_THEME_BTN_PRIMARY;
                            colorBtn.layout.sizing.width = CLAY_SIZING_GROW();
                            uint8_t selectorFlashKind = self->editorInstrumentSelectorFlashKind;
                            float selectorFlashAlpha = Tracker_ChangeFlashAlpha(self->editorInstrumentSelectorFlashTime);
                            colorBtn.backgroundColor = Tracker_ButtonHoverColor(
                                self->instrumentNameButton.clayId,
                                Tracker_ApplyChangeFlashTint(
                                    Tracker_ColorFromU32(instColorRgb, 255.0f),
                                    selectorFlashAlpha,
                                    selectorFlashKind,
                                    self->editorInstrumentSelectorFlashTime
                                ),
                                14.0f
                            );
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
                            shortBtn.backgroundColor = Tracker_ButtonHoverColor(self->instrumentNextButton.clayId, CLAY_COLOR_BTN_PRIMARY);
                            CLAY(self->instrumentNextButton.clayId, shortBtn)
                            {
                                CLAY_TEXT(CLAY_STRING("▶"), CLAY_TEXT_CONFIG(buttonCfg));
                            }
                            Clay_ElementDeclaration instCheck = CLAY_THEME_BTN_BOX;
                            instCheck.backgroundColor = Tracker_ButtonHoverColor(
                                self->instrumentExplicitButton.clayId,
                                self->editInstrumentExplicit ? CLAY_COLOR_BTN_SUCCESS : CLAY_COLOR_BTN_DISABLED,
                                self->editInstrumentExplicit ? 18.0f : 10.0f
                            );
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
                            CLAY(
                                CLAY_ID("TrackerVolumeLabel"),
                                {.layout = {.sizing = {CLAY_SIZING_FIXED(58), CLAY_SIZING_GROW()},
                                            .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER}}}
                            )
                            {
                                CLAY_TEXT(CLAY_STRING("VOL"), CLAY_TEXT_CONFIG(bodyCfg));
                            }
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
                            CLAY(
                                CLAY_ID("TrackerVolumeValue"),
                                {.layout = {.sizing = {CLAY_SIZING_FIXED(52), CLAY_SIZING_GROW()},
                                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}}}
                            )
                            {
                                Clay_String vol = ClayArena_FormatString(arena, "%02X", self->editVolume);
                                CLAY_TEXT(vol, CLAY_TEXT_CONFIG(bodyCfg));
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
                                        bool inScale = Tracker_SongScaleIncludesNote(self->songScaleMode, self->songScaleRoot, note);
                                        Clay_Color bg = selected ? (Clay_Color){78, 170, 126, 255}
                                            : black ? (inScale ? (Clay_Color){28, 30, 42, 255} : (Clay_Color){10, 12, 18, 255})
                                                    : (inScale ? (Clay_Color){220, 224, 235, 255} : (Clay_Color){182, 186, 196, 255});
                                        uint16_t keyBorderWidth = selected ? 2 : 1;
                                        Clay_TextElementConfig keyText = bodyCfg;
                                        keyText.textColor = selected
                                            ? (Clay_Color){245, 245, 250, 255}
                                            : black
                                                ? (inScale ? (Clay_Color){245, 245, 250, 255} : (Clay_Color){108, 114, 132, 255})
                                                : inScale ? (Clay_Color){20, 20, 30, 255} : (Clay_Color){72, 76, 90, 255};
                                        Clay_Color borderColor = selected ? (Clay_Color){235, 245, 255, 255}
                                            : black
                                                ? (inScale ? (Clay_Color){80, 80, 100, 255} : (Clay_Color){42, 46, 60, 255})
                                                : (Clay_Color){80, 80, 100, 255};

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
                                                .color = borderColor,
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
                                        bool inScale = Tracker_SongScaleIncludesNote(self->songScaleMode, self->songScaleRoot, note);
                                        Clay_Color bg = selected ? (Clay_Color){78, 170, 126, 255}
                                                                : inScale ? (Clay_Color){220, 224, 235, 255}
                                                                          : (Clay_Color){182, 186, 196, 255};
                                        uint16_t borderW = selected ? 2 : 1;
                                        Clay_TextElementConfig keyText = bodyCfg;
                                        keyText.textColor = selected ? (Clay_Color){245, 245, 250, 255}
                                                                    : inScale ? (Clay_Color){20, 20, 30, 255}
                                                                              : (Clay_Color){72, 76, 90, 255};

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
                            Clay_String paramLabel = ClayArena_AllocString(arena, label);
                            Clay_String paramValueText = ClayArena_FormatString(arena, "%02X", paramValue);
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
                                    {.layout = {.sizing = {CLAY_SIZING_FIXED(82), CLAY_SIZING_GROW()},
                                                .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER}}}
                                )
                                {
                                    CLAY_TEXT(paramLabel, CLAY_TEXT_CONFIG(bodyCfg));
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
                                CLAY(
                                    CLAY_IDI("TrackerEffectParamValue", barId.id),
                                    {.layout = {.sizing = {CLAY_SIZING_FIXED(52), CLAY_SIZING_GROW()},
                                                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}}}
                                )
                                {
                                    CLAY_TEXT(paramValueText, CLAY_TEXT_CONFIG(bodyCfg));
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
                                CLAY_TEXT(CLAY_STRING("◀"), CLAY_TEXT_CONFIG(buttonCfg));
                            }
                            CLAY(
                                CLAY_ID("TrackerEffectTypeValue"),
                                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}},
                                .backgroundColor = active ? Clay_Color{35, 45, 65, 255} : Clay_Color{42, 43, 50, 255},
                                .cornerRadius = {CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD}}
                            )
                            {
                                Clay_String label = ClayArena_FormatString(arena, "%02X: %s", def->code, def->name);
                                CLAY_TEXT(label, CLAY_TEXT_CONFIG(buttonCfg));
                            }
                            CLAY(self->effectNextButton.clayId, CLAY_THEME_BTN_BOX)
                            {
                                CLAY_TEXT(CLAY_STRING("▶"), CLAY_TEXT_CONFIG(buttonCfg));
                            }
                            Clay_ElementDeclaration activeBox = CLAY_THEME_BTN_BOX;
                            activeBox.backgroundColor = ClayTheme_HoverColor(
                                active ? CLAY_COLOR_BTN_SUCCESS :
                                    (limitReached ? Clay_Color{62, 62, 70, 255} : CLAY_COLOR_BTN_DISABLED),
                                active ? 18.0f : 10.0f
                            );
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
                "%02X: %s",
                self->editInstrument,
                Tracker_InstrumentName(self, self->editInstrument)
            );
            CLAY_TEXT(title, CLAY_TEXT_CONFIG(titleCfg));
            CLAY(CLAY_ID("TrackerInstrumentEditorGrow"), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()}}}) {}
            uint32_t instColorRgb = Tracker_InstrumentColorU32(self, self->editInstrument);
            Clay_ElementDeclaration colorBtn = CLAY_THEME_BTN_PRIMARY;
            colorBtn.layout.sizing.width = CLAY_SIZING_FIXED(60);
            colorBtn.backgroundColor = ClayTheme_HoverColor(
                Tracker_ColorFromU32(instColorRgb, 255.0f),
                14.0f
            );
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
                .padding = {20, 20, 0, 0},
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
            patchTab.backgroundColor = Tracker_ButtonHoverColor(
                self->instrumentPatchTabButton.clayId,
                self->instrumentEditorTab == 0 ? CLAY_COLOR_PANEL_SECTION : CLAY_COLOR_BTN_PRIMARY,
                self->instrumentEditorTab == 0 ? 16.0f : 24.0f
            );
            effectsTab.backgroundColor = Tracker_ButtonHoverColor(
                self->instrumentEffectsTabButton.clayId,
                self->instrumentEditorTab == 1 ? CLAY_COLOR_PANEL_SECTION : CLAY_COLOR_BTN_PRIMARY,
                self->instrumentEditorTab == 1 ? 16.0f : 24.0f
            );
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
                        CLAY_TEXT(CLAY_STRING("◀"), CLAY_TEXT_CONFIG(buttonCfg));
                    }
                    Clay_String algo = ClayArena_FormatString(arena, "ALGO %d:", patch.ALG);
                    CLAY(
                        CLAY_ID("TrackerInstrumentAlgoValue"),
                        {.layout =
                             {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                              .childGap = 10,
                              .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}
                            },
                         .backgroundColor = {35, 45, 65, 255},
                         .cornerRadius = {
                             CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD
                         }}
                    )
                    {
                        CLAY_TEXT(algo, CLAY_TEXT_CONFIG(buttonCfg));

                        Clay_ElementDeclaration algoPreview = {
                            .layout = {.sizing = {CLAY_SIZING_FIXED(96), CLAY_SIZING_FIXED(48)}},
                            .image = {.imageData = &clayton->trackerAlgoImages[patch.ALG & 7]},
                            .border = {.color = {146, 220, 132, 255}, 
                            .width = CLAY_BORDER_ALL(1)}
                        };
                        CLAY(CLAY_ID("TrackerSelectedAlgoDiagram"), algoPreview)
                        {
                        }
                    }
                    CLAY(self->instrumentAlgoNextButton.clayId, CLAY_THEME_BTN_BOX)
                    {
                        CLAY_TEXT(CLAY_STRING("▶"), CLAY_TEXT_CONFIG(buttonCfg));
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

                    // ---------- unique section box per operator ----------
                    auto sectionBox = [&](const char *baseId, int op, float height, auto &&content)
                    {
                        Clay_String id = ClayArena_FormatString(arena, "%s_OP%d", baseId, op);
                        CLAY(
                            CLAY_SID(id),
                            {.layout =
                                 {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                                  .padding = {6, 6, 6, 6},
                                  .childGap = 4,
                                  .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                                  .layoutDirection = CLAY_TOP_TO_BOTTOM},
                             .backgroundColor = {15, 18, 26, 255},
                             .cornerRadius = {6, 6, 6, 6},
                             .border = {.color = {44, 50, 68, 255}, .width = CLAY_BORDER_ALL(1)}}
                        )
                        {
                            content();
                        }
                    };

                    // ---------- unique grow spacer per operator ----------
                    auto growSpacer = [&](const char *baseId, int op)
                    {
                        Clay_String id = ClayArena_FormatString(arena, "%s_OP%d", baseId, op);
                        CLAY(
                            CLAY_SID(id),
                            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()}}}
                        )
                        {
                        }
                    };

                    // ---------- fixed statMeter (now includes opId) ----------
                    auto statMeter = [&](int rowIndex,
                                         const char *baseId, // e.g. "TrackerOperatorTlMeter"
                                         int opId,
                                         const char *label,
                                         int value,
                                         int minValue,
                                         int maxValue,
                                         Clay_Color fillColor,
                                         bool bipolar = false)
                    {
                        float t = maxValue > minValue
                            ? (float)(value - minValue) / (float)(maxValue - minValue)
                            : 0.0f;
                        t = std::max(0.0f, std::min(1.0f, t));
                        float negativeT = (value < 0 && minValue < 0) ? (float)value / (float)minValue : 0.0f;
                        float positiveT = (value > 0 && maxValue > 0) ? (float)value / (float)maxValue : 0.0f;
                        negativeT = std::max(0.0f, std::min(1.0f, negativeT));
                        positiveT = std::max(0.0f, std::min(1.0f, positiveT));
                        Clay_String rowId = ClayArena_FormatString(arena, "%s_OP%d", baseId, opId);
                        Clay_Color dimFillColor = {
                            (float)std::max(18, (int)(fillColor.r * 0.28f)),
                            (float)std::max(18, (int)(fillColor.g * 0.28f)),
                            (float)std::max(18, (int)(fillColor.b * 0.28f)),
                            255.0f
                        };
                        Clay_TextElementConfig statCfg = bodyCfg;
                        statCfg.textColor = {244, 246, 250, 255};

                        CLAY(
                            CLAY_SID(rowId),
                            {.layout = {
                                 .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(18)},
                             .padding = {0, 0, 0, 0},
                             .layoutDirection = CLAY_LEFT_TO_RIGHT
                             },
                             .backgroundColor = dimFillColor}
                        )
                        {
                            if (bipolar)
                            {
                                CLAY(
                                    CLAY_IDI("TrackerOperatorStatNegativeHalf", opId * 10 + rowIndex),
                                    {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                                .layoutDirection = CLAY_LEFT_TO_RIGHT}}
                                )
                                {
                                    CLAY(
                                        CLAY_IDI("TrackerOperatorStatNegativeSpacer", opId * 10 + rowIndex),
                                        {.layout = {.sizing = {CLAY_SIZING_PERCENT(1.0f - negativeT), CLAY_SIZING_GROW()}}}
                                    ) {}
                                    if (negativeT > 0.0f)
                                    {
                                        CLAY(
                                            CLAY_IDI("TrackerOperatorStatNegativeFill", opId * 10 + rowIndex),
                                            {.layout = {.sizing = {CLAY_SIZING_PERCENT(negativeT), CLAY_SIZING_GROW()}},
                                             .backgroundColor = fillColor}
                                        ) {}
                                    }
                                }
                                CLAY(
                                    CLAY_IDI("TrackerOperatorStatCenter", opId * 10 + rowIndex),
                                    {.layout = {.sizing = {CLAY_SIZING_FIXED(1), CLAY_SIZING_GROW()}},
                                     .backgroundColor = {232, 236, 244, 190}}
                                ) {}
                                CLAY(
                                    CLAY_IDI("TrackerOperatorStatPositiveHalf", opId * 10 + rowIndex),
                                    {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                                .layoutDirection = CLAY_LEFT_TO_RIGHT}}
                                )
                                {
                                    if (positiveT > 0.0f)
                                    {
                                        CLAY(
                                            CLAY_IDI("TrackerOperatorStatPositiveFill", opId * 10 + rowIndex),
                                            {.layout = {.sizing = {CLAY_SIZING_PERCENT(positiveT), CLAY_SIZING_GROW()}},
                                             .backgroundColor = fillColor}
                                        ) {}
                                    }
                                }
                            }
                            else
                            {
                                CLAY(
                                    CLAY_IDI("TrackerOperatorStatFill", opId * 10 + rowIndex),
                                    {.layout = {
                                         .sizing = {CLAY_SIZING_PERCENT(t), CLAY_SIZING_GROW()}
                                     },
                                     .backgroundColor = fillColor}
                                )
                                {
                                }
                            }
                            Clay_ElementDeclaration thingCfg = {
                                .layout =
                                    {
                                        .childAlignment =
                                            {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                    },

                                .floating = {
                                    .offset = {0, 0},
                                    .zIndex = 103, // BE CAREFUL : this has to be between Instrument and operator window
                                    .attachPoints =
                                        {CLAY_ATTACH_POINT_LEFT_TOP,
                                            CLAY_ATTACH_POINT_LEFT_TOP},
                                    .pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH,
                                    .attachTo = CLAY_ATTACH_TO_PARENT,
                                }
                            };

                            thingCfg.layout.sizing.width = CLAY_SIZING_GROW();
                            thingCfg.layout.sizing.height = CLAY_SIZING_FIXED(18);
                            CLAY(CLAY_IDI("somehanger", opId * 10 + rowIndex), thingCfg)
                            {
                                Clay_String statLabel = ClayArena_FormatString(arena, "%s", label);
                                Clay_String statValue = ClayArena_FormatString(arena, "%d", value);

                                CLAY_TEXT(statLabel, CLAY_TEXT_CONFIG(statCfg));
                                CLAY(
                                    CLAY_IDI("rowheaderspcr", opId * 10 + rowIndex),
                                    {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()}}}
                                )
                                {
                                }
                                CLAY_TEXT(statValue, CLAY_TEXT_CONFIG(buttonCfg));
                            }
                        }
                    };

                    // ---------- operator panel body ----------
                    CLAY(
                        self->operatorButtons[opId].clayId,
                        {.layout =
                             {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                              .padding = {8, 8, 8, 8},
                              .childGap = 0,
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
                        // header
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
                            CLAY(
                                CLAY_IDI("TrackerOperatorHeaderSpacer", opId),
                                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()}}}
                            )
                            {
                            }
                            Clay_TextElementConfig amCfg = bodyCfg;
                            amCfg.textColor = op.AM ? Clay_Color{168, 236, 170, 255}
                                                    : Clay_Color{108, 112, 124, 255};
                            CLAY_TEXT(CLAY_STRING("AM"), CLAY_TEXT_CONFIG(amCfg));
                        }

                        growSpacer("TrackerOperatorSpacerTop", opId);

                        sectionBox(
                            "TrackerOperatorTlSection",
                            opId,
                            24.0f,
                            [&]()
                            {
                                statMeter(
                                    0,
                                    "TrackerOperatorTlMeter",
                                    opId,
                                    "TL",
                                    (int)op.TL,
                                    0,
                                    127,
                                    {154, 152, 218, 255}
                                );
                            }
                        );

                        growSpacer("TrackerOperatorSpacerAfterTl", opId);

                        sectionBox(
                            "TrackerOperatorEnvelopeSection",
                            opId,
                            60.0f,
                            [&]()
                            {
                                CLAY(CLAY_IDI("OpEnvelopeOppacerss", opId), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(60)}}}) {
                                    renderSmallPreview(
                                        CLAY_IDI("TrackerOperatorEnvelopePreview", opId),
                                        &clayton->trackerEnvelopeImages[opId],
                                        true
                                    );
                                };
                            }
                        );

                        growSpacer("TrackerOperatorSpacerAfterEnv", opId);

                        sectionBox(
                            "TrackerOperatorSsgSection",
                            opId,
                            28.0f,
                            [&]()
                            {
                                CLAY(CLAY_IDI("OpSSGOppacerss", opId), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(30)}}}) {
                                    int ssg = op.SSG;
                                    renderSmallPreview(
                                        CLAY_IDI("TrackerOperatorSsgPreview", opId),
                                        ssg > 0
                                            ? &clayton
                                                ->trackerSsgImages[std::max(0, std::min(7, ssg - 1))]
                                            : nullptr,
                                        ssg > 0
                                    );
                                }
                            }
                        );

                        growSpacer("TrackerOperatorSpacerLow", opId);

                        sectionBox(
                            "TrackerOperatorStatsSection",
                            opId,
                            66.0f,
                            [&]()
                            {
                                CLAY(
                                    CLAY_IDI("TrackerOperatorStatsColA", opId),
                                    {.layout = {
                                         .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                         .childGap = 5,
                                         .layoutDirection = CLAY_TOP_TO_BOTTOM
                                     }}
                                )
                                {
                                    statMeter(
                                        1,
                                        "TrackerOperatorMulMeter",
                                        opId,
                                        "MUL",
                                        (int)op.MUL,
                                        0,
                                        15,
                                        {80, 176, 225, 255}
                                    );
                                    statMeter(
                                        2,
                                        "TrackerOperatorDtMeter",
                                        opId,
                                        "DT",
                                        (int)op.DT,
                                        -3,
                                        3,
                                        {210, 138, 75, 255},
                                        true
                                    );
                                    statMeter(
                                        3,
                                        "TrackerOperatorRsMeter",
                                        opId,
                                        "RS",
                                        (int)op.RS,
                                        0,
                                        3,
                                        {100, 192, 124, 255}
                                    );
                                }
                            }
                        );
                        growSpacer("TrackerOperatorSpacerCompleteBottom", opId);
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
                        Clay_String labelText = ClayArena_AllocString(arena, label);
                        Clay_String valueText = ClayArena_FormatString(arena, "%d", value);
                        CLAY(
                            CLAY_IDI("TrackerInstrumentSliderLabel", barId.id),
                            {.layout = {
                                 .sizing = {CLAY_SIZING_FIXED(58), CLAY_SIZING_GROW()},
                                 .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER}
                             }}
                        )
                        {
                            CLAY_TEXT(labelText, CLAY_TEXT_CONFIG(bodyCfg));
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
                        CLAY(
                            CLAY_IDI("TrackerInstrumentSliderValue", barId.id),
                            {.layout = {
                                 .sizing = {CLAY_SIZING_FIXED(44), CLAY_SIZING_GROW()},
                                 .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}
                             }}
                        )
                        {
                            CLAY_TEXT(valueText, CLAY_TEXT_CONFIG(bodyCfg));
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
                                 .sizing = {CLAY_SIZING_PERCENT(0.25f), CLAY_SIZING_GROW()},
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
                Tracker_EnsureMacroCapacity(&macro);
                int target = std::max((int)XFM_MACRO_TL1, std::min(Tracker_MacroMaxTarget(), self->editMacroTarget));
                bool enabled = self->editMacroEnabled[inst][target];
                int enabledCount = Tracker_MacroEnabledCount(self);

                CLAY(
                    CLAY_ID("TrackerMacroTargetRow"),
                    {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                                .childGap = 8,
                                .layoutDirection = CLAY_LEFT_TO_RIGHT}}
                )
                {
                    CLAY(self->macroTargetPrevButton.clayId, CLAY_THEME_BTN_BOX)
                    {
                        CLAY_TEXT(CLAY_STRING("◀"), CLAY_TEXT_CONFIG(buttonCfg));
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
                            "%02X: %s",
                            target,
                            Tracker_MacroTargetName(target)
                        );
                        CLAY_TEXT(label, CLAY_TEXT_CONFIG(buttonCfg));
                    }
                    CLAY(self->macroTargetNextButton.clayId, CLAY_THEME_BTN_BOX)
                    {
                        CLAY_TEXT(CLAY_STRING("▶"), CLAY_TEXT_CONFIG(buttonCfg));
                    }
                    Clay_ElementDeclaration enableCheck = CLAY_THEME_BTN_BOX;
                    // enableCheck.layout.sizing.width = CLAY_SIZING_FIXED(42);
                    enableCheck.backgroundColor = ClayTheme_HoverColor(
                        enabled ? CLAY_COLOR_BTN_SUCCESS : CLAY_COLOR_BTN_DISABLED,
                        enabled ? 18.0f : 10.0f
                    );
                    CLAY(self->macroEnableButton.clayId, enableCheck)
                    {
                        CLAY_TEXT(enabled ? CLAY_STRING("✓") : CLAY_STRING(" "), CLAY_TEXT_CONFIG(buttonCfg));
                    }
                }
                
                Clay_String label = ClayArena_FormatString(
                    arena,
                    "%d macros active",
                    enabledCount
                );
                CLAY_TEXT(label, CLAY_TEXT_CONFIG(buttonCfg));

                int valueMin = -64;
                int valueMax = 127;
                Tracker_MacroTargetValueRange(target, valueMin, valueMax);
                Tracker_EnsureMacroValueViewForRange(self, target, valueMin, valueMax);
                const int visibleSpan = Tracker_MacroVisibleValueSpan(target, valueMin, valueMax);
                const int viewMin = self->macroValueViewMin;
                const int viewMax = std::min(valueMax, viewMin + visibleSpan);
                bool signedMacro = valueMin < 0 && valueMax > 0;
                bool invertedVerticalMacro = Tracker_MacroTargetUsesInvertedVerticalValue(target);
                float zeroT = signedMacro ? (float)viewMax / (float)(viewMax - viewMin) : 1.0f;
                zeroT = std::max(0.0f, std::min(1.0f, zeroT));
                int baseValue = std::max(valueMin, std::min(valueMax, Tracker_MacroTargetBaseValue(self, target)));
                float baseValueT = viewMax > viewMin
                    ? (invertedVerticalMacro
                        ? ((float)baseValue - (float)viewMin) / (float)(viewMax - viewMin)
                        : ((float)viewMax - (float)baseValue) / (float)(viewMax - viewMin))
                    : 1.0f;
                baseValueT = std::max(0.0f, std::min(1.0f, baseValueT));
                Clay_Color graphBg = enabled ? (Clay_Color){18, 20, 30, 255} : (Clay_Color){42, 42, 46, 255};
                Clay_Color posColor = enabled ? (Clay_Color){96, 170, 236, 210} : (Clay_Color){92, 92, 96, 180};
                Clay_Color negColor = enabled ? (Clay_Color){232, 114, 118, 210} : (Clay_Color){82, 82, 86, 180};
                Clay_Color baseLineColor = enabled ? (Clay_Color){236, 236, 160, 210} : (Clay_Color){140, 140, 120, 180};
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

                .childGap = 2,
                .layoutDirection = CLAY_LEFT_TO_RIGHT}

            }
)
                        {
                            for (int i = 0; i < TRACKER_MACRO_UI_STEPS; i++)
                            {
                                bool columnEnabled = i < Tracker_MacroEnabledColumns(&macro);
                                Clay_TextElementConfig dimTinyCfg = tinyCfg;
                                dimTinyCfg.textColor = {118, 122, 132, 255};
                                if (reset)
                                {
                                    CLAY(
                                        CLAY_IDI("TrackerMacroReset", i),
                                        {.layout = {.sizing = {CLAY_SIZING_PERCENT(1.0f / (float)TRACKER_MACRO_UI_STEPS), CLAY_SIZING_GROW()},
                                                    .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}},
                                        .backgroundColor = enabled ? (columnEnabled ? (Clay_Color){46, 92, 66, 255} : (Clay_Color){62, 64, 72, 255}) : (Clay_Color){58, 58, 62, 255},
                                        .cornerRadius = {2, 2, 2, 2}}
                                    )
                                    {
                                        Clay_String text = ClayArena_FormatString(arena, "%d", i);
                                        CLAY_TEXT(text, CLAY_TEXT_CONFIG(columnEnabled ? tinyCfg : dimTinyCfg));
                                    }
                                }
                                else if (graph)
                                {
                                    int v = std::max(valueMin, std::min(valueMax, (int)macro.values[i]));
                                    macro.values[i] = (int16_t)v;
                                    const bool valueAboveView = invertedVerticalMacro ? v < viewMin : v > viewMax;
                                    const bool valueBelowView = invertedVerticalMacro ? v > viewMax : v < viewMin;
                                    const bool valueInView = !valueAboveView && !valueBelowView;
                                    float valueT = viewMax > viewMin
                                        ? (invertedVerticalMacro
                                            ? ((float)v - (float)viewMin) / (float)(viewMax - viewMin)
                                            : ((float)viewMax - (float)v) / (float)(viewMax - viewMin))
                                        : 1.0f;
                                    valueT = std::max(0.0f, std::min(1.0f, valueT));
                                    constexpr float MACRO_MARKER_HEIGHT = 12.0f;
                                    const Clay_Color macroMarkerColor = enabled ? (Clay_Color){255, 178, 24, 255} : (Clay_Color){132, 118, 82, 255};
                                    float posFill = signedMacro ? std::max(0.0f, zeroT - valueT) / std::max(0.001f, zeroT) : 1.0f - valueT;
                                    float negFill = signedMacro ? std::max(0.0f, valueT - zeroT) / std::max(0.001f, 1.0f - zeroT) : 0.0f;
                                    Clay_TextElementConfig arrowCfg = tinyCfg;
                                    arrowCfg.fontId = CLAY_FONT_NOTO;
                                    arrowCfg.fontSize = CLAY_FONT_SIZE_SM - 2;
                                    arrowCfg.textColor = macroMarkerColor;
                                    CLAY(
                                        CLAY_IDI("TrackerMacroBarColumn", i),
                                        {.layout = {.sizing = {CLAY_SIZING_PERCENT(1.0f / (float)TRACKER_MACRO_UI_STEPS), CLAY_SIZING_GROW()},
                                                    .childGap = 1,
                                                    .layoutDirection = CLAY_TOP_TO_BOTTOM}}
                                    )
                                    {
                                        CLAY(
                                            CLAY_IDI("TrackerMacroBarInset", i),
                                            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                                        .padding = {2, 2, 2, 2},
                                                        .childGap = 1,
                                                        .layoutDirection = CLAY_TOP_TO_BOTTOM},
                                            .backgroundColor = columnEnabled ? (Clay_Color){12, 14, 22, 150} : (Clay_Color){52, 54, 62, 210},
                                            .cornerRadius = {2, 2, 2, 2},
                                            .border = {.color = columnEnabled ? (Clay_Color){116, 124, 150, 255} : (Clay_Color){132, 136, 150, 255},
                                                       .width = CLAY_BORDER_ALL(2)}}
                                        )
                                        {
                                            if (columnEnabled && valueAboveView)
                                            {
                                                CLAY(CLAY_IDI("TrackerMacroValueAbove", i), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(MACRO_MARKER_HEIGHT)},
                                                                                                  .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}}})
                                                {
                                                    CLAY_TEXT(CLAY_STRING("▲"), CLAY_TEXT_CONFIG(arrowCfg));
                                                }
                                                if (signedMacro)
                                                {
                                                    if (v > 0 && zeroT > 0.0f)
                                                    {
                                                        if (zeroT < 1.0f)
                                                        {
                                                            CLAY(CLAY_IDI("TrackerMacroValueAbovePosFill", i), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_PERCENT(zeroT)}},
                                                                                                              .backgroundColor = posColor}) {}
                                                            CLAY(CLAY_IDI("TrackerMacroValueAboveSpace", i), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()}}}) {}
                                                        }
                                                        else
                                                        {
                                                            CLAY(CLAY_IDI("TrackerMacroValueAbovePosFill", i), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()}},
                                                                                                              .backgroundColor = posColor}) {}
                                                        }
                                                    }
                                                    else
                                                    {
                                                        CLAY(CLAY_IDI("TrackerMacroValueAboveSpace", i), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()}}}) {}
                                                    }
                                                }
                                                else
                                                {
                                                    CLAY(CLAY_IDI("TrackerMacroValueAbovePosFill", i), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()}},
                                                                                                      .backgroundColor = posColor}) {}
                                                }
                                            }
                                            else if (columnEnabled && valueBelowView)
                                            {
                                                if (signedMacro)
                                                {
                                                    if (v < 0 && zeroT < 1.0f)
                                                    {
                                                        if (zeroT > 0.0f)
                                                            CLAY(CLAY_IDI("TrackerMacroValueBelowSpace", i), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_PERCENT(zeroT)}}}) {}
                                                        CLAY(CLAY_IDI("TrackerMacroValueBelowNegFill", i), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()}},
                                                                                                          .backgroundColor = negColor}) {}
                                                    }
                                                    else
                                                    {
                                                        CLAY(CLAY_IDI("TrackerMacroValueBelowSpace", i), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()}}}) {}
                                                    }
                                                }
                                                else
                                                {
                                                    CLAY(CLAY_IDI("TrackerMacroValueBelowSpace", i), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()}}}) {}
                                                }
                                                CLAY(CLAY_IDI("TrackerMacroValueBelow", i), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(MACRO_MARKER_HEIGHT)},
                                                                                                  .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}}})
                                                {
                                                    CLAY_TEXT(CLAY_STRING("▼"), CLAY_TEXT_CONFIG(arrowCfg));
                                                }
                                            }
                                            else if (columnEnabled && valueInView)
                                            {
                                                if (signedMacro)
                                                {
                                                    if (valueT <= zeroT)
                                                    {
                                                        if (valueT > 0.0f)
                                                            CLAY(CLAY_IDI("TrackerMacroBarPosSpace", i), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_PERCENT(valueT)}}}) {}
                                                        CLAY(CLAY_IDI("TrackerMacroDash", i), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(MACRO_MARKER_HEIGHT)}},
                                                                                               .backgroundColor = macroMarkerColor,
                                                                                               .cornerRadius = {1, 1, 1, 1}}) {}
                                                        if (posFill > 0.0f)
                                                            CLAY(CLAY_IDI("TrackerMacroBarPos", i), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_PERCENT(zeroT - valueT)}},
                                                                                                     .backgroundColor = posColor}) {}
                                                        CLAY(CLAY_IDI("TrackerMacroBarPosBottomSpace", i), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()}}}) {}
                                                    }
                                                    else
                                                    {
                                                        if (zeroT > 0.0f)
                                                            CLAY(CLAY_IDI("TrackerMacroBarNegTopSpace", i), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_PERCENT(zeroT)}}}) {}
                                                        if (negFill > 0.0f)
                                                            CLAY(CLAY_IDI("TrackerMacroBarNeg", i), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_PERCENT(valueT - zeroT)}},
                                                                                                     .backgroundColor = negColor}) {}
                                                        CLAY(CLAY_IDI("TrackerMacroDash", i), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(MACRO_MARKER_HEIGHT)}},
                                                                                               .backgroundColor = macroMarkerColor,
                                                                                               .cornerRadius = {1, 1, 1, 1}}) {}
                                                        CLAY(CLAY_IDI("TrackerMacroBarNegBottomSpace", i), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()}}}) {}
                                                    }
                                                }
                                                else
                                                {
                                                    if (valueT > 0.0f)
                                                        CLAY(CLAY_IDI("TrackerMacroDashTopSpace", i), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_PERCENT(valueT)}}}) {}
                                                    CLAY(CLAY_IDI("TrackerMacroDash", i), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(MACRO_MARKER_HEIGHT)}},
                                                                                           .backgroundColor = macroMarkerColor,
                                                                                           .cornerRadius = {1, 1, 1, 1}}) {}
                                                    if (valueT < 1.0f)
                                                        CLAY(CLAY_IDI("TrackerMacroBarUnsigned", i), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()}},
                                                                                                      .backgroundColor = posColor}) {}
                                                }
                                            }
                                            else
                                            {
                                                CLAY(CLAY_IDI("TrackerMacroBarDisabledFill", i), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()}},
                                                                                                  .backgroundColor = {52, 54, 60, 170}}) {}
                                            }
                                        }
                                    }
                                }
                                else if (numbers)
                                {
                                    bool inLoopRange = columnEnabled && macro.has_loop &&
                                                    i >= (int)macro.loop_start &&
                                                    (macro.release_start == 0xFF || i < (int)macro.release_start);
                                    bool inReleaseRange = columnEnabled && macro.release_start != 0xFF && i >= (int)macro.release_start;
                                    Clay_Color numberBg = inLoopRange ? (Clay_Color){40, 90, 72, 255} :
                                                        inReleaseRange ? (Clay_Color){84, 54, 42, 255} :
                                                        (columnEnabled ? (Clay_Color){26, 28, 38, 255} : (Clay_Color){18, 20, 26, 255});
                                    CLAY(
                                        CLAY_IDI("TrackerMacroValueNumber", i),
                                        {.layout = {.sizing = {CLAY_SIZING_PERCENT(1.0f / (float)TRACKER_MACRO_UI_STEPS), CLAY_SIZING_GROW()},
                                                    .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_TOP}},
                                        .backgroundColor = numberBg,
                                        .cornerRadius = {2, 2, 2, 2}}
                                    )
                                    {
                                        Clay_String text = ClayArena_FormatString(arena, "%d", (int)macro.values[i]);
                                        CLAY_TEXT(text, CLAY_TEXT_CONFIG(columnEnabled ? tinyCfg : dimTinyCfg));
                                    }
                                }
                            }
                        }
                        if (graph)
                        {
                            if (baseValue >= viewMin && baseValue <= viewMax)
                            {
                                float markerY = baseValueT * std::max(0.0f, bb.height - 2.0f);
                                CLAY(
                                    CLAY_ID("TrackerMacroBaseValueLine"),
                                    {
                                        .layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(2)}},
                                        .backgroundColor = baseLineColor,
                                        .floating = {
                                            .offset = {0, markerY},
                                            .zIndex = 4,
                                            .attachPoints = {CLAY_ATTACH_POINT_LEFT_TOP, CLAY_ATTACH_POINT_LEFT_TOP},
                                            .pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH,
                                            .attachTo = CLAY_ATTACH_TO_PARENT,
                                        }
                                    }
                                ) {}
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
                        CLAY_TEXT(CLAY_STRING("◀"), CLAY_TEXT_CONFIG(buttonCfg));
                    }
                    CLAY(
                        CLAY_ID("TrackerMacroViewportStack"),
                        {.layout = {
                             .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                             .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_TOP},
                             .layoutDirection = CLAY_TOP_TO_BOTTOM,
                         }}
                    )
                    {

                        constexpr int MACRO_RESET_ROW_H = 32;
                        constexpr int MACRO_NUMBERS_ROW_H = 32;
                        constexpr int MACRO_STACK_RESERVED_GAP_H = 20;
                        constexpr int MACRO_VALUE_SCROLL_BUTTON_H = 42;
                        constexpr int MACRO_MIN_GRAPH_H = 42;

                        Clay_ElementData ed = Clay_GetElementData( CLAY_ID("TrackerMacroViewportStack"));

                        int beltHeight = 220;
                        if (ed.found) 
                        {
                            beltHeight = (int)ed.boundingBox.height;
                        }
                        const int graphHeight = std::max(
                            MACRO_MIN_GRAPH_H,
                            beltHeight -
                                MACRO_RESET_ROW_H -
                                MACRO_NUMBERS_ROW_H -
                                MACRO_VALUE_SCROLL_BUTTON_H * 2 -
                                MACRO_STACK_RESERVED_GAP_H
                        );

                        Clay_ElementDeclaration macroValueScrollBtn = CLAY_THEME_BTN_PRIMARY;
                        macroValueScrollBtn.layout.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(MACRO_VALUE_SCROLL_BUTTON_H)};
                        Clay_TextElementConfig macroValueScrollCfg = buttonCfg;
                        macroValueScrollCfg.fontSize = CLAY_FONT_SIZE_SM;

                        renderMacroBelt("TrackerMacroResetClip", "TrackerMacroResetBelt", MACRO_RESET_ROW_H, false, true, false);
                        CLAY(self->macroValueScrollUpButton.clayId, macroValueScrollBtn)
                        {
                            CLAY_TEXT(CLAY_STRING("▲"), CLAY_TEXT_CONFIG(macroValueScrollCfg));
                        }
                        renderMacroBelt("TrackerMacroGraphClip", "TrackerMacroGraphBelt", graphHeight, true, false, false);
                        CLAY(self->macroValueScrollDownButton.clayId, macroValueScrollBtn)
                        {
                            CLAY_TEXT(CLAY_STRING("▼"), CLAY_TEXT_CONFIG(macroValueScrollCfg));
                        }
                        renderMacroBelt("TrackerMacroNumbersClip", "TrackerMacroNumbersBelt", MACRO_NUMBERS_ROW_H, false, false, true);
                    }
                    CLAY(self->macroScrollNextButton.clayId, macroScrollNext)
                    {
                        CLAY_TEXT(CLAY_STRING("▶"), CLAY_TEXT_CONFIG(buttonCfg));
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
                    bool releaseSupported = Tracker_MacroTargetSupportsRelease(target);
                    loopBtn.backgroundColor = ClayTheme_HoverColor(
                        macro.has_loop ? (Clay_Color){172, 66, 66, 255} : CLAY_COLOR_BTN_SUCCESS,
                        18.0f
                    );
                    if (!macro.has_loop && self->macroSelectMode == TRACKER_MACRO_SELECT_LOOP)
                        loopBtn.border = {.color = {255, 255, 180, 255}, .width = CLAY_BORDER_ALL(2)};
                    releaseBtn.backgroundColor = ClayTheme_HoverColor(
                        !releaseSupported
                            ? CLAY_COLOR_BTN_DISABLED
                            : (macro.release_start != 0xFF ? (Clay_Color){172, 66, 66, 255}
                                                           : CLAY_COLOR_BTN_SUCCESS),
                        releaseSupported ? 18.0f : 10.0f
                    );
                    if (releaseSupported && macro.release_start == 0xFF && self->macroSelectMode == TRACKER_MACRO_SELECT_RELEASE)
                        releaseBtn.border = {.color = {255, 255, 180, 255}, .width = CLAY_BORDER_ALL(2)};
                    CLAY(self->macroLoopButton.clayId, loopBtn)
                    {
                        CLAY_TEXT(CLAY_STRING("LOOP"), CLAY_TEXT_CONFIG(buttonCfg));
                    }
                    CLAY(self->macroReleaseButton.clayId, releaseBtn)
                    {
                        CLAY_TEXT(CLAY_STRING("REL"), CLAY_TEXT_CONFIG(buttonCfg));
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
            Clay_String title = ClayArena_FormatString(arena, "Color %02X: %s", inst, Tracker_InstrumentName(self, inst));
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
                        Clay_Color swatchBg = Tracker_ColorFromU32(rgb, 255.0f);
                        if (Clay_PointerOver(CLAY_IDI("TrackerInstrumentColorSwatch", idx)))
                            swatchBg = CLAY_THEME_HOVER_COLOR(swatchBg, 18.0f, 0.0f);
                        Clay_ElementDeclaration swatch = {
                            .layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                       .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}},
                            .backgroundColor = swatchBg,
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
    opWin.layout.sizing =  {CLAY_SIZING_PERCENT(0.95f), CLAY_SIZING_FIT()};
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
            Clay_ElementDeclaration greyBox = CLAY_THEME_BTN_SUCCESS;
            greyBox.layout.padding = {6, 6, 4, 4};
            greyBox.layout.childGap = { 6 };

            greyBox.backgroundColor = {45, 45, 45, 255};

            CLAY(
                CLAY_ID("TrackerOperatorEditorSelector"),
                {.layout = {.sizing = {CLAY_SIZING_FIT(), CLAY_SIZING_FIT()},
                            .childGap = 4,
                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                            .layoutDirection = CLAY_LEFT_TO_RIGHT}}
            )
            {

                CLAY(self->operatorEditorPrevButton.clayId, CLAY_THEME_BTN_BOX)
                {
                    CLAY_TEXT(CLAY_STRING("◀"), CLAY_TEXT_CONFIG(buttonCfg));
                }
                // Clay_String opLabel = ClayArena_FormatString(arena, "OP%d", opIndex + 1);
                // CLAY(
                //     CLAY_ID("TrackerOperatorEditorOpLabel"),
                //     {.layout = {.sizing = {CLAY_SIZING_FIXED(52), CLAY_SIZING_FIXED(60)},
                //                 .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}},
                //      .backgroundColor = {35, 45, 65, 255},
                //      .cornerRadius = {CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD}}
                // )
                // {
                //     CLAY_TEXT(opLabel, CLAY_TEXT_CONFIG(buttonCfg));
                // }

                CLAY(CLAY_ID("OperatorEditorInstrumentMiniPrevTitle"), greyBox)
                {
                    Clay_ElementDeclaration algoPreview = {
                        .layout = {.sizing = {CLAY_SIZING_FIXED(72), CLAY_SIZING_FIXED(36)}},
                        .image = {.imageData = &clayton->trackerSelectedAlgoImages[patch.ALG & 7][opIndex]},
                        .border = {.color = {146, 220, 132, 255}, .width = CLAY_BORDER_ALL(0)}
                    };

                    Clay_String title = ClayArena_FormatString(
                        arena,
                        "%02X",
                        self->editInstrument
                    );

                    CLAY(CLAY_ID("TrackerOperatorEditorAlgoPreview"), algoPreview) {}

                    // CLAY(CLAY_ID("TrackerOperatorEditorGrow"), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()}}}) {}
                    // CLAY_TEXT(title, CLAY_TEXT_CONFIG(titleCfg));
                }
                CLAY(self->operatorEditorNextButton.clayId, CLAY_THEME_BTN_BOX)
                {
                    CLAY_TEXT(CLAY_STRING("▶"), CLAY_TEXT_CONFIG(buttonCfg));
                }
            }

            CLAY(CLAY_ID("OpTitleSpacer"), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()}}}) {};

            Clay_ElementDeclaration amBtn = CLAY_THEME_BTN_BOX;
            if (op.AM) amBtn.backgroundColor = ClayTheme_HoverColor(CLAY_COLOR_BTN_SUCCESS, 18.0f);
            CLAY(self->operatorAmButton.clayId, amBtn)
            {
                CLAY_TEXT(CLAY_STRING("AM"), CLAY_TEXT_CONFIG(buttonCfg));
            }
            CLAY(self->operatorEditorCloseButton.clayId, CLAY_THEME_BTN_DANGER)
            {
                CLAY_TEXT(CLAY_STRING("x"), CLAY_TEXT_CONFIG(buttonCfg));
            }
        }

        const float operatorParamLabelW = 58.0f;
        const float operatorParamValueW = 44.0f;

        auto slider = [&](const char *label, int value, int minValue, int maxValue, Clay_ElementId barId, Clay_ElementId fillId, bool invertValue = false) {
            float t = maxValue > minValue ? (float)(value - minValue) / (float)(maxValue - minValue) : 0.0f;
            if (invertValue)
                t = 1.0f - t;
            t = std::max(0.0f, std::min(1.0f, t));
            CLAY(
                CLAY_IDI("TrackerOperatorSliderRow", barId.id),
                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(35)},
                            .childGap = 8,
                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                            .layoutDirection = CLAY_LEFT_TO_RIGHT}}
            )
            {
                Clay_String labelText = ClayArena_AllocString(arena, label);
                Clay_String valueText = ClayArena_FormatString(arena, "%d", value);
                CLAY(
                    CLAY_IDI("TrackerOperatorSliderLabel", barId.id),
                    {.layout = {.sizing = {CLAY_SIZING_FIXED(operatorParamLabelW), CLAY_SIZING_GROW()},
                                .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER}}}
                )
                {
                    CLAY_TEXT(labelText, CLAY_TEXT_CONFIG(bodyCfg));
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
                        {.layout = {.sizing = {CLAY_SIZING_PERCENT(t), CLAY_SIZING_GROW()}},
                         .backgroundColor = {120, 146, 214, 255},
                         .cornerRadius = {4, 4, 4, 4}}
                    ) {}
                }
                CLAY(
                    CLAY_IDI("TrackerOperatorSliderValue", barId.id),
                    {.layout = {.sizing = {CLAY_SIZING_FIXED(operatorParamValueW), CLAY_SIZING_GROW()},
                                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}}}
                )
                {
                    CLAY_TEXT(valueText, CLAY_TEXT_CONFIG(bodyCfg));
                }
            }
        };

        auto bipolarSlider = [&](const char *label, int value, int minValue, int maxValue, Clay_ElementId barId) {
            float negativeT = (value < 0 && minValue < 0) ? (float)value / (float)minValue : 0.0f;
            float positiveT = (value > 0 && maxValue > 0) ? (float)value / (float)maxValue : 0.0f;
            negativeT = std::max(0.0f, std::min(1.0f, negativeT));
            positiveT = std::max(0.0f, std::min(1.0f, positiveT));
            CLAY(
                CLAY_IDI("TrackerOperatorBipolarSliderRow", barId.id),
                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(35)},
                            .childGap = 8,
                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                            .layoutDirection = CLAY_LEFT_TO_RIGHT}}
            )
            {
                Clay_String labelText = ClayArena_AllocString(arena, label);
                Clay_String valueText = ClayArena_FormatString(arena, "%d", value);
                CLAY(
                    CLAY_IDI("TrackerOperatorBipolarSliderLabel", barId.id),
                    {.layout = {.sizing = {CLAY_SIZING_FIXED(operatorParamLabelW), CLAY_SIZING_GROW()},
                                .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER}}}
                )
                {
                    CLAY_TEXT(labelText, CLAY_TEXT_CONFIG(bodyCfg));
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
                        CLAY_IDI("TrackerOperatorBipolarSliderNegativeHalf", barId.id),
                        {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                    .layoutDirection = CLAY_LEFT_TO_RIGHT}}
                    )
                    {
                        CLAY(
                            CLAY_IDI("TrackerOperatorBipolarSliderNegativeSpacer", barId.id),
                            {.layout = {.sizing = {CLAY_SIZING_PERCENT(1.0f - negativeT), CLAY_SIZING_GROW()}}}
                        ) {}
                        if (negativeT > 0.0f)
                        {
                            CLAY(
                                CLAY_IDI("TrackerOperatorBipolarSliderNegativeFill", barId.id),
                                {.layout = {.sizing = {CLAY_SIZING_PERCENT(negativeT), CLAY_SIZING_GROW()}},
                                 .backgroundColor = {120, 146, 214, 255},
                                 .cornerRadius = {4, 4, 4, 4}}
                            ) {}
                        }
                    }
                    CLAY(
                        CLAY_IDI("TrackerOperatorBipolarSliderCenter", barId.id),
                        {.layout = {.sizing = {CLAY_SIZING_FIXED(2), CLAY_SIZING_GROW()}},
                         .backgroundColor = {88, 92, 110, 255}}
                    ) {}
                    CLAY(
                        CLAY_IDI("TrackerOperatorBipolarSliderPositiveHalf", barId.id),
                        {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                    .layoutDirection = CLAY_LEFT_TO_RIGHT}}
                    )
                    {
                        if (positiveT > 0.0f)
                        {
                            CLAY(
                                CLAY_IDI("TrackerOperatorBipolarSliderPositiveFill", barId.id),
                                {.layout = {.sizing = {CLAY_SIZING_PERCENT(positiveT), CLAY_SIZING_GROW()}},
                                 .backgroundColor = {120, 146, 214, 255},
                                 .cornerRadius = {4, 4, 4, 4}}
                            ) {}
                        }
                    }
                }
                CLAY(
                    CLAY_IDI("TrackerOperatorBipolarSliderValue", barId.id),
                    {.layout = {.sizing = {CLAY_SIZING_FIXED(operatorParamValueW), CLAY_SIZING_GROW()},
                                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}}}
                )
                {
                    CLAY_TEXT(valueText, CLAY_TEXT_CONFIG(bodyCfg));
                }
            }
        };

        slider("TL", op.TL, 0, 127, CLAY_ID("TrackerOpTlBar"), CLAY_ID("TrackerOpTlFill"), true);

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
                     .sizing = {CLAY_SIZING_FIXED(operatorParamLabelW), CLAY_SIZING_GROW()},
                     .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER}
                 }}
            )
            {
                CLAY_TEXT(text, CLAY_TEXT_CONFIG(bodyCfg));
            }
            Clay_ElementDeclaration envelopePreviewHolder = {
                .layout = {
                    .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                     .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}
                },
                .border = {.color = {146, 220, 132, 255}, .width = CLAY_BORDER_ALL(1)}
            };
            Clay_ElementDeclaration envelopePreview = {
                .layout = {.sizing = {CLAY_SIZING_FIXED(240), CLAY_SIZING_FIXED(120)}},
                // .aspectRatio = {.aspectRatio = 1.0f * 16.0f / 9.0f},
                .image = {.imageData = &clayton->trackerOperatorEnvelopeImages[opIndex]},
            };
            CLAY(CLAY_ID("TrackerCurrentOperatorEnvelopeWrap"), envelopePreviewHolder)
            {
                CLAY(CLAY_ID("TrackerCurrentOperatorEnvelope"), envelopePreview)
                {
                }
            }
            CLAY(
                CLAY_ID("TrackerOperatorEnvelopeValueSpacer"),
                {.layout = {.sizing = {CLAY_SIZING_FIXED(operatorParamValueW), CLAY_SIZING_GROW()}}}
            ) {}
        }

        slider("AR", op.AR, 0, 31, CLAY_ID("TrackerOpArBar"), CLAY_ID("TrackerOpArFill"), true);
        slider("DR", op.DR, 0, 31, CLAY_ID("TrackerOpDrBar"), CLAY_ID("TrackerOpDrFill"), true);
        slider("SL", op.SL, 0, 15, CLAY_ID("TrackerOpSlBar"), CLAY_ID("TrackerOpSlFill"), true);
        slider("SR", op.SR, 0, 31, CLAY_ID("TrackerOpSrBar"), CLAY_ID("TrackerOpSrFill"), true);
        slider("RR", op.RR, 0, 15, CLAY_ID("TrackerOpRrBar"), CLAY_ID("TrackerOpRrFill"), true);

        
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
                    {.layout = {.sizing = {CLAY_SIZING_FIXED(operatorParamLabelW), CLAY_SIZING_GROW()},
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
                CLAY_TEXT(CLAY_STRING("◀"), CLAY_TEXT_CONFIG(buttonCfg));
            }
            CLAY(
                CLAY_ID("TrackerOperatorSsgValue"),
                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(60)},

                        .padding = {16, 16, 0, 0},
                        .childGap = 8,
                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}},
                 .backgroundColor = {35, 45, 65, 255},
                 .cornerRadius = {CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD}}
            )
            {
                Clay_String text = op.SSG == 0 ? CLAY_STRING("NO SSG-EG") : ClayArena_FormatString(arena, "%d:", op.SSG);
                CLAY_TEXT(text, CLAY_TEXT_CONFIG(buttonCfg));
                Clay_ElementDeclaration ssgPreview = {
                    .layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(48)}},
                    .backgroundColor = {11, 14, 20, 255},
                    .border = {.color = op.SSG > 0 ? (Clay_Color){146, 220, 132, 255} : (Clay_Color){54, 60, 78, 255},
                            .width = CLAY_BORDER_ALL(1)}
                };
                if (op.SSG > 0) {
                    ssgPreview.image.imageData = &clayton->trackerSsgImages[std::max(0, std::min(7, (int)op.SSG - 1))];
                    CLAY(CLAY_ID("TrackerCurrentOperatorSsg"), ssgPreview) {}
                }
            }
            CLAY(self->operatorSsgNextButton.clayId, CLAY_THEME_BTN_BOX)
            {
                CLAY_TEXT(CLAY_STRING("▶"), CLAY_TEXT_CONFIG(buttonCfg));
            }
        }
        CLAY(
            CLAY_ID("TrackerOperatorSsgValueSpacer"),
            {.layout = {.sizing = {CLAY_SIZING_FIXED(operatorParamValueW), CLAY_SIZING_GROW()}}}
        ) {}
        }
        slider("MUL", op.MUL, 0, 15, CLAY_ID("TrackerOpMulBar"), CLAY_ID("TrackerOpMulFill"));
        bipolarSlider("DT", op.DT, -3, 3, CLAY_ID("TrackerOpDtBar"));
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
                        disabled.backgroundColor = ClayTheme_HoverColor(CLAY_COLOR_BTN_DISABLED, 10.0f);
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
                            .backgroundColor = Tracker_ButtonHoverColor(
                                self->instrumentRowClicks[inst].clayId,
                                Tracker_ApplyChangeFlashTint(
                                    rowBg,
                                    Tracker_ChangeFlashAlpha(self->instrumentFlashTime[inst]),
                                    self->instrumentFlashKind[inst],
                                    self->instrumentFlashTime[inst]
                                ),
                                12.0f
                            ),
                            .cornerRadius = {4, 4, 4, 4},
                            .border = {.color = self->editInstrument == inst ? (Clay_Color){255, 255, 255, 255} : Tracker_ColorFromU32(instColorRgb, 255.0f),
                                       .width = CLAY_BORDER_ALL((uint16_t)(self->editInstrument == inst ? 2 : 1))}
                        };

                        CLAY(
                            self->instrumentRowClicks[inst].clayId,
                            rowDecl
                        )
                        {
                            Clay_String label = ClayArena_FormatString(arena, "%02X: %s", inst, Tracker_InstrumentName(self, inst));
                            CLAY(CLAY_IDI("TrackerInstrumentLabel", inst), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                                                                        .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER}}})
                            {
                                CLAY_TEXT(label, CLAY_TEXT_CONFIG(rowText));
                            }
                            CLAY(self->instrumentUpButtons[inst].clayId, inst > 0 ? smallBtn : disabledSmall)
                            {
                                CLAY_TEXT(CLAY_STRING("▲"), CLAY_TEXT_CONFIG(buttonCfg));
                            }
                            CLAY(self->instrumentDownButtons[inst].clayId, inst < 255 ? smallBtn : disabledSmall)
                            {
                                CLAY_TEXT(CLAY_STRING("▼"), CLAY_TEXT_CONFIG(buttonCfg));
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
                 .cornerRadius = {0, 6, 6, 0},
                 .border = {.color = {70, 76, 100, 255}, .width = CLAY_BORDER_ALL(1)}}
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
                Clay_String label = ClayArena_FormatString(arena, "Selected: %02X: %s", selected, Tracker_InstrumentName(self, selected));
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
            Clay_String labelText = ClayArena_AllocString(arena, label);
            Clay_String valueText = ClayArena_FormatString(arena, "%d", value);
            CLAY(CLAY_IDI("TrackerSongSettingsSliderLabel", barId.id),
                 {.layout = {.sizing = {CLAY_SIZING_FIXED(96), CLAY_SIZING_GROW()},
                             .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER}}})
            {
                CLAY_TEXT(labelText, CLAY_TEXT_CONFIG(bodyCfg));
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
            CLAY(CLAY_IDI("TrackerSongSettingsSliderValue", barId.id),
                 {.layout = {.sizing = {CLAY_SIZING_FIXED(58), CLAY_SIZING_GROW()},
                             .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}}})
            {
                CLAY_TEXT(valueText, CLAY_TEXT_CONFIG(bodyCfg));
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
            CLAY_ID("TrackerSongScaleRootRow"),
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(54)},
                        .childGap = 8,
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
        )
        {
            CLAY(CLAY_ID("TrackerSongScaleRootLabel"), {.layout = {.sizing = {CLAY_SIZING_FIXED(84), CLAY_SIZING_GROW()},
                                                                    .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER}}})
            {
                CLAY_TEXT(CLAY_STRING("Root"), CLAY_TEXT_CONFIG(bodyCfg));
            }
            Clay_ElementDeclaration shortBtn = CLAY_THEME_BTN_BOX;
            shortBtn.backgroundColor = Tracker_ButtonHoverColor(self->songScaleRootPrevButton.clayId, CLAY_COLOR_BTN_PRIMARY);
            CLAY(self->songScaleRootPrevButton.clayId, shortBtn)
            {
                CLAY_TEXT(CLAY_STRING("◀"), CLAY_TEXT_CONFIG(buttonCfg));
            }
            Clay_ElementDeclaration rootNameBtn = CLAY_THEME_BTN_PRIMARY;
            rootNameBtn.layout.sizing.width = CLAY_SIZING_GROW();
            CLAY(CLAY_ID("TrackerSongScaleRootValue"), rootNameBtn)
            {
                Clay_String rootName = ClayArena_FormatString(arena, "%s", Tracker_SongScaleRootName(self->songScaleRoot));
                CLAY_TEXT(rootName, CLAY_TEXT_CONFIG(buttonCfg));
            }
            shortBtn.backgroundColor = Tracker_ButtonHoverColor(self->songScaleRootNextButton.clayId, CLAY_COLOR_BTN_PRIMARY);
            CLAY(self->songScaleRootNextButton.clayId, shortBtn)
            {
                CLAY_TEXT(CLAY_STRING("▶"), CLAY_TEXT_CONFIG(buttonCfg));
            }
        }

        CLAY(
            CLAY_ID("TrackerSongScaleRow"),
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(54)},
                        .childGap = 8,
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
        )
        {
            CLAY(CLAY_ID("TrackerSongScaleLabel"), {.layout = {.sizing = {CLAY_SIZING_FIXED(84), CLAY_SIZING_GROW()},
                                                                .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER}}})
            {
                CLAY_TEXT(CLAY_STRING("Scale"), CLAY_TEXT_CONFIG(bodyCfg));
            }
            Clay_ElementDeclaration shortBtn = CLAY_THEME_BTN_BOX;
            shortBtn.backgroundColor = Tracker_ButtonHoverColor(self->songScalePrevButton.clayId, CLAY_COLOR_BTN_PRIMARY);
            CLAY(self->songScalePrevButton.clayId, shortBtn)
            {
                CLAY_TEXT(CLAY_STRING("◀"), CLAY_TEXT_CONFIG(buttonCfg));
            }
            Clay_ElementDeclaration scaleNameBtn = CLAY_THEME_BTN_PRIMARY;
            scaleNameBtn.layout.sizing.width = CLAY_SIZING_GROW();
            CLAY(CLAY_ID("TrackerSongScaleValue"), scaleNameBtn)
            {
                Clay_String scaleName = ClayArena_FormatString(
                    arena,
                    "%s %s",
                    Tracker_SongScaleRootName(self->songScaleRoot),
                    Tracker_SongScaleModeName(self->songScaleMode)
                );
                CLAY_TEXT(scaleName, CLAY_TEXT_CONFIG(buttonCfg));
            }
            shortBtn.backgroundColor = Tracker_ButtonHoverColor(self->songScaleNextButton.clayId, CLAY_COLOR_BTN_PRIMARY);
            CLAY(self->songScaleNextButton.clayId, shortBtn)
            {
                CLAY_TEXT(CLAY_STRING("▶"), CLAY_TEXT_CONFIG(buttonCfg));
            }
        }

        CLAY(
            CLAY_ID("TrackerSongScaleHintRow"),
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(34)},
                        .childGap = 8,
                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
        )
        {
            CLAY(CLAY_ID("TrackerSongScaleHintSpacer"), {.layout = {.sizing = {CLAY_SIZING_FIXED(84), CLAY_SIZING_GROW()}}}) {}
            CLAY(CLAY_ID("TrackerSongScaleHintText"), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                                                   .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER}}})
            {
                CLAY_TEXT(CLAY_STRING("Indicative only. Out-of-scale notes are dimmed."), CLAY_TEXT_CONFIG(mutedCfg));
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

            Clay_ElementDeclaration longDangerButtonCfg = CLAY_THEME_BTN_DANGER;
            longDangerButtonCfg.layout.sizing.width = CLAY_SIZING_GROW();
            CLAY(self->songLoadEmptyButton.clayId, longDangerButtonCfg)
            {
                CLAY_TEXT(CLAY_STRING("Load"), CLAY_TEXT_CONFIG(buttonCfg));
            }
        }

        Clay_ElementDeclaration lfoBtn = CLAY_THEME_BTN_BOX;
        if (self->songLfoEnabled) lfoBtn.backgroundColor = ClayTheme_HoverColor(CLAY_COLOR_BTN_SUCCESS, 18.0f);
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
            if (!canDec) minusBtn.backgroundColor = ClayTheme_HoverColor(CLAY_COLOR_BTN_DISABLED, 10.0f);
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
            if (!canInc) plusBtn.backgroundColor = ClayTheme_HoverColor(CLAY_COLOR_BTN_DISABLED, 10.0f);
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
                            .childGap = 8,
                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                            .layoutDirection = CLAY_LEFT_TO_RIGHT}}
            )
            {
                Clay_ElementDeclaration cloneBtn = CLAY_THEME_BTN_PRIMARY;
                cloneBtn.layout.sizing = {CLAY_SIZING_FIXED(118), CLAY_SIZING_FIXED(42)};
                if (self->partCount >= TRACKER_MAX_PARTS || self->rowCount + part.rowCount > TRACKER_MAX_ROWS)
                    cloneBtn.backgroundColor = ClayTheme_HoverColor(CLAY_COLOR_BTN_DISABLED, 10.0f);
                CLAY(self->partEditorCloneButton.clayId, cloneBtn)
                {
                    CLAY_TEXT(CLAY_STRING("CLONE"), CLAY_TEXT_CONFIG(buttonCfg));
                }
                CLAY(CLAY_ID("TrackerPartEditorActionSpacer"),
                     {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()}}})
                {
                }
                Clay_ElementDeclaration deleteBtn = CLAY_THEME_BTN_DANGER;
                deleteBtn.layout.sizing = {CLAY_SIZING_FIXED(96), CLAY_SIZING_FIXED(42)};
                if (self->partCount <= 1)
                    deleteBtn.backgroundColor = ClayTheme_HoverColor(CLAY_COLOR_BTN_DISABLED, 10.0f);
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

                            .padding = { 5, 5, 0, 0},
                        .childGap = 5,
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
        )
        {
            CLAY(
                CLAY_ID("TrackerTitleTextBlock"),
                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                            .childGap = 1,
                            .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                            .layoutDirection = CLAY_TOP_TO_BOTTOM}}
            )
            {
                Clay_String songTitle = {.length = (int32_t)std::strlen(self->songDisplayName), .chars = self->songDisplayName};
                CLAY_TEXT(songTitle, CLAY_TEXT_CONFIG(titleCfg));
                const float bpm =
                    (self->songTickRate > 0 && self->songSpeed > 0 && self->songRowsPerBeat > 0) ?
                        (self->songTickRate * 60.0f) / ((float)self->songSpeed * (float)self->songRowsPerBeat) :
                        0.0f;
                const float currentSeconds =
                    (self->songTickRate > 0 && self->songSpeed > 0) ?
                        ((float)(std::max(0, self->playRow) * std::max(1, self->songSpeed) + std::max(0, self->playTick)) / (float)std::max(1, self->songTickRate)) :
                        0.0f;
                const float totalSeconds =
                    (self->songTickRate > 0 && self->songSpeed > 0) ?
                        ((float)std::max(1, Tracker_PlaybackRowCount(self)) * (float)std::max(1, self->songSpeed) / (float)std::max(1, self->songTickRate)) :
                        0.0f;
                Clay_TextElementConfig metaCfg = bodyCfg;
                metaCfg.fontSize = CLAY_FONT_SIZE_SM;
                metaCfg.textColor = {150, 154, 170, 255};
                Clay_String meta = ClayArena_FormatString(
                    arena,
                    "%.1fs / %.1fs   R%03d.%d   BPM %.1f",
                    currentSeconds,
                    totalSeconds,
                    self->playRow,
                    self->playTick,
                    bpm
                );
                CLAY_TEXT(meta, CLAY_TEXT_CONFIG(metaCfg));
            }
            CLAY(CLAY_ID("TrackerTitleGrow"), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()}}}) {}
            Clay_ElementDeclaration saveLoadDecl = CLAY_THEME_BTN_PRIMARY; 
            saveLoadDecl.layout.padding.left = 10;
            saveLoadDecl.layout.padding.right = 10;
            CLAY(self->saveSongButton.clayId, saveLoadDecl)
            {
                CLAY_TEXT(CLAY_STRING("SAVE"), CLAY_TEXT_CONFIG(buttonCfg));
            }
            CLAY(self->loadSongButton.clayId, saveLoadDecl)
            {
                CLAY_TEXT(CLAY_STRING("LOAD"), CLAY_TEXT_CONFIG(buttonCfg));
            }
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
                    .backgroundColor = ClayTheme_HoverColor(
                        hasCustomLoop ? (Clay_Color){176, 68, 84, 255} : (Clay_Color){42, 46, 58, 255},
                        16.0f
                    ),
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
                    bool hideGridTextForPartAnimation = Tracker_AnyPartCollapseAnimating(self);
                    int visibleRows = Tracker_VisibleRowCount(self);
                    float contentCursorY = 0.0f;
                    for (int visualIndex = 0; visualIndex < visibleRows; visualIndex++)
                    {
                        TrackerVisualRow visual = Tracker_MapVisualIndex(self, visualIndex);
                        if (visual.kind == TRACKER_VISUAL_ROW_PART_TITLE)
                        {
                            int partIndex = visual.part;
                            TrackerPart &part = self->parts[partIndex];
                            bool titleCollapsed = Tracker_PartCollapseIconShowsCollapsed(self, partIndex);
                            Clay_Color titleBg = !part.enabled ? (Clay_Color){42, 34, 38, 255} :
                                (titleCollapsed ? (Clay_Color){34, 40, 58, 255} : (Clay_Color){28, 34, 48, 255});
                            uint8_t partFlashKind = TRACKER_CHANGE_FLASH_NONE;
                            float partFlashTimeLeft = 0.0f;
                            titleBg = Tracker_ApplyChangeFlashTint(
                                titleBg,
                                Tracker_PartFlashAlpha(self, partIndex, &partFlashKind, &partFlashTimeLeft),
                                partFlashKind,
                                partFlashTimeLeft
                            );
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
                                bool progressFillInViewport =
                                    contentCursorY >= self->scrollY &&
                                    contentCursorY + self->rowHeight <= self->scrollY + self->viewportHeight;
                                Tracker_BuildPartTitleContent(
                                    self,
                                    arena,
                                    partIndex,
                                    progressFillInViewport,
                                    0,
                                    false,
                                    &self->partToggleButtons[partIndex],
                                    &self->partEnableButtons[partIndex],
                                    &self->partUpButtons[partIndex],
                                    &self->partDownButtons[partIndex],
                                    &self->partSettingsButtons[partIndex],
                                    buttonCfg,
                                    bodyCfg
                                );
                            }
                            contentCursorY += self->rowHeight;
                            continue;
                        }
                        if (visual.kind != TRACKER_VISUAL_ROW_CELL)
                            continue;
                        int row = visual.row;
                        int displayRow = std::max(0, visual.localRow);
                        float partOpenFraction = Tracker_PartBodyOpenFraction(self, visual.part);
                        float animatedRowHeight = self->rowHeight * partOpenFraction;
                        bool activeRow = row == self->playRow;
                        bool zebraDarkBand = Tracker_RowIsDarkZebraBand(self, visual.part, displayRow);
                        CLAY(
                            CLAY_IDI("TrackerGridRow", visualIndex),
                            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(animatedRowHeight)},
                                        .childGap = 0,
                                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
                        )
                        {
                            CLAY(
                                CLAY_IDI("TrackerLineCell", row),
                                {.layout = {.sizing = {CLAY_SIZING_PERCENT(TRACKER_LINE_IN_SCROLL), CLAY_SIZING_GROW()},
                                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}},
                                 .backgroundColor = Tracker_ApplyZebraTint(Tracker_LoopLineColor(self, row, activeRow), zebraDarkBand),
                                 .border = {.color = {50, 56, 74, 255}, .width = CLAY_BORDER_ALL(1)}}
                            )
                            {
                                if (!hideGridTextForPartAnimation)
                                {
                                    Clay_String rn = ClayArena_FormatString(arena, "%03X", displayRow);
                                    CLAY_TEXT(rn, CLAY_TEXT_CONFIG(monoCfg));
                                }
                            }
                            for (int ch = 0; ch < TRACKER_CHANNELS; ch++)
                            {
                                bool selectedColumn = !self->channelSelectionEnabled || (ch >= self->channelStart && ch <= self->channelEnd);
                                Clay_Color cellBg = Tracker_LoopCellColor(self, row, activeRow);
                                if (self->loopEnabled && !selectedColumn)
                                    cellBg = Tracker_CellColor(activeRow, false);
                                const char *cell = self->cells[row][ch].text;
                                bool playable = Tracker_CellHasPlayableNote(cell);
                                int displayInst = Tracker_CellDisplayInstrument(self, cell, row, ch);
                                uint32_t displayColor = displayInst >= 0 ? Tracker_InstrumentColorU32(self, displayInst) : 0;
                                bool specialTerminator = Tracker_CellIsSpecialTerminator(cell);
                                bool noteLike = Tracker_CellHasNoteLikeValue(cell);
                                Clay_BorderElementConfig outerBorderConfig = {.color = {50, 56, 74, 255}, .width = CLAY_BORDER_ALL(1)};
                                if (specialTerminator || noteLike)
                                {
                                    outerBorderConfig = {
                                        .color = displayColor != 0 ? Tracker_ColorFromU32(displayColor, 255.0f) : (Clay_Color){245, 245, 250, 255},
                                        .width = CLAY_BORDER_ALL(2)
                                    };
                                }
                                bool brightCellBg = false;
                                if (playable && displayColor != 0)
                                {
                                    cellBg = Tracker_ColorFromU32(displayColor, activeRow ? 245.0f : 225.0f);
                                    brightCellBg = Tracker_ColorIsBright(displayColor);
                                }
                                bool movingSourceCell = self->cellMoving &&
                                    row == self->cellMoveSourceRow &&
                                    ch == self->cellMoveSourceChannel;
                                bool movingTargetCell = self->cellMoving &&
                                    self->cellMoveValidTarget &&
                                    row == self->cellMoveHoverRow &&
                                    ch == self->cellMoveHoverChannel;
                                if (movingSourceCell || movingTargetCell)
                                {
                                    if (movingTargetCell)
                                    {
                                        cell = self->cellMoveSource.text;
                                        cellBg = Tracker_CellMoveHighlightColor(self);
                                        brightCellBg = true;
                                        outerBorderConfig = {.color = {255, 255, 255, 200}, .width = CLAY_BORDER_ALL(2)};
                                        cellBg.a = 150.0f;
                                    }
                                    else
                                    {
                                        cellBg.a = 120.0f;
                                        outerBorderConfig.color.a = 140.0f;
                                    }
                                }
                                if (movingTargetCell)
                                {
                                    cell = self->cellMoveSource.text;
                                }
                                cellBg = Tracker_ApplyZebraTint(cellBg, zebraDarkBand);
                                uint8_t cellFlashKind = TRACKER_CHANGE_FLASH_NONE;
                                float cellFlashTimeLeft = 0.0f;
                                float cellFlashAlpha = Tracker_CellFlashAlpha(self, row, ch, &cellFlashKind, &cellFlashTimeLeft);
                                cellBg = Tracker_ApplyChangeFlashTint(cellBg, cellFlashAlpha, cellFlashKind, cellFlashTimeLeft);
                                if (cellFlashAlpha > 0.45f)
                                    brightCellBg = true;
                                Clay_Color innerBorderColor = Tracker_EditSelectionContains(self, row, ch) ?
                                    Tracker_EditSelectionBorderColor(cell, displayColor) : (Clay_Color){0, 0, 0, 255};
                                uint16_t innerBorderWidth = Tracker_EditSelectionContains(self, row, ch) ? 3 : 1;
                                CLAY(
                                    CLAY_IDI("TrackerCell", row * 10 + ch),
                                    {.layout = {.sizing = {CLAY_SIZING_PERCENT(TRACKER_CHANNEL_IN_SCROLL), CLAY_SIZING_GROW()},
                                                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}},
                                     .backgroundColor = cellBg,
                                     .border = outerBorderConfig}
                                )
                                {
                                    CLAY(
                                        CLAY_IDI("TrackerCellInner", row * 10 + ch),
                                        {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                                    .padding = {2, 2, 2, 2},
                                                    .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                                    .layoutDirection = CLAY_TOP_TO_BOTTOM},
                                         .backgroundColor = cellBg,
                                         .border = {.color = innerBorderColor, .width = CLAY_BORDER_ALL(innerBorderWidth)}}
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
                                            if (!hideGridTextForPartAnimation)
                                            {
                                                Clay_TextElementConfig cellMonoCfg = brightCellBg ? darkMonoCfg : monoCfg;
                                                Clay_TextElementConfig cellEffectCfg = brightCellBg ? darkEffectMonoCfg : effectMonoCfg;
                                                if (movingSourceCell || movingTargetCell)
                                                {
                                                    cellMonoCfg.textColor.a = 180.0f;
                                                    cellEffectCfg.textColor.a = 160.0f;
                                                }
                                                CLAY_TEXT(ClayArena_AllocString(arena, top), CLAY_TEXT_CONFIG(cellMonoCfg));
                                                if (bottom[0])
                                                    CLAY_TEXT(ClayArena_AllocString(arena, bottom), CLAY_TEXT_CONFIG(cellEffectCfg));
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        contentCursorY += animatedRowHeight;
                    }
                }

                int stickyPart = Tracker_StickyPartIndexAtScroll(self);
                if (stickyPart >= 0 && stickyPart < self->partCount)
                {
                    TrackerPart &part = self->parts[stickyPart];
                    float stickyTop = Tracker_StickyPartTitleTopY(self, stickyPart);
                    Clay_Color titleBg = !part.enabled ? (Clay_Color){42, 34, 38, 248} :
                        (Tracker_PartCollapseIconShowsCollapsed(self, stickyPart) ? (Clay_Color){34, 40, 58, 248} : (Clay_Color){28, 34, 48, 248});
                    uint8_t partFlashKind = TRACKER_CHANGE_FLASH_NONE;
                    float partFlashTimeLeft = 0.0f;
                    titleBg = Tracker_ApplyChangeFlashTint(
                        titleBg,
                        Tracker_PartFlashAlpha(self, stickyPart, &partFlashKind, &partFlashTimeLeft),
                        partFlashKind,
                        partFlashTimeLeft
                    );
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
                            true,
                            13,
                            true,
                            &self->stickyPartToggleButton,
                            &self->stickyPartEnableButton,
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
                for (int partIndex = 0; partIndex < self->partCount; partIndex++)
                {
                    const TrackerPart &part = self->parts[partIndex];
                    if (part.enabled || part.rowCount <= 0)
                        continue;
                    float skippedTop = self->viewportHeight *
                        ((float)Tracker_VisualIndexForRow(self, part.startRow) / rowCountForMap);
                    float skippedBottom = self->viewportHeight *
                        ((float)(Tracker_VisualIndexForRow(self, part.startRow + part.rowCount - 1) + 1) / rowCountForMap);
                    float skippedHeight = std::max(2.0f, skippedBottom - skippedTop);
                    CLAY(
                        CLAY_IDI("TrackerScrollbarSkippedRange", partIndex),
                        {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(skippedHeight)}},
                         .backgroundColor = {92, 96, 108, 92},
                         .floating = {
                             .offset = {0, skippedTop + skippedHeight * 0.5f - self->viewportHeight * 0.5f},
                             .zIndex = 0,
                             .attachPoints = {CLAY_ATTACH_POINT_CENTER_CENTER, CLAY_ATTACH_POINT_CENTER_CENTER},
                             .pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH,
                             .attachTo = CLAY_ATTACH_TO_PARENT,
                         }}
                    ) {}
                }
                if (self->loopEnabled)
                {
                    CLAY(
                        CLAY_ID("TrackerScrollbarLoopRange"),
                        {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(scrollbarRangeHeight)}},
                         .backgroundColor = {0, 0, 0, 0},
                         .floating = {
                             .offset = {0, scrollbarRangeTop + scrollbarRangeHeight * 0.5f - self->viewportHeight * 0.5f},
                             .zIndex = 0,
                             .attachPoints = {CLAY_ATTACH_POINT_CENTER_CENTER, CLAY_ATTACH_POINT_CENTER_CENTER},
                             .pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH,
                             .attachTo = CLAY_ATTACH_TO_PARENT,
                         },
                        .border = {.color = {112, 210, 132, 150}, .width = CLAY_BORDER_ALL(2)}}
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
                         .backgroundColor = {140, 164, 196, 120},
                         .floating = {
                             .offset = {0, boundaryTop - self->viewportHeight * 0.5f},
                             .zIndex = 0,
                             .attachPoints = {CLAY_ATTACH_POINT_CENTER_CENTER, CLAY_ATTACH_POINT_CENTER_CENTER},
                             .pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH,
                             .attachTo = CLAY_ATTACH_TO_PARENT,
                         }}
                    ) {}
                }
                CLAY(
                    CLAY_ID("TrackerScrollbarPlayhead"),
                    {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(2)}},
                     .backgroundColor = {255, 245, 160, 120},
                     .floating = {
                         .offset = {0, scrollbarPlayheadTop - self->viewportHeight * 0.5f},
                         .zIndex = 0,
                         .attachPoints = {CLAY_ATTACH_POINT_CENTER_CENTER, CLAY_ATTACH_POINT_CENTER_CENTER},
                         .pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH,
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
                Clay_ElementDeclaration playBtn = CLAY_THEME_BTN_PRIMARY;
                playBtn.backgroundColor = Tracker_ButtonHoverColor(self->playButton.clayId, CLAY_COLOR_BTN_PRIMARY);
                CLAY(self->playButton.clayId, playBtn)
                {
                    CLAY_TEXT(CLAY_STRING("START"), CLAY_TEXT_CONFIG(buttonCfg));
                }
                Clay_ElementDeclaration stopBtn = CLAY_THEME_BTN_PRIMARY;
                stopBtn.backgroundColor = Tracker_ButtonHoverColor(self->stopButton.clayId, CLAY_COLOR_BTN_PRIMARY);
                CLAY(self->stopButton.clayId, stopBtn)
                {
                    CLAY_TEXT(self->playing ? CLAY_STRING("STOP") : CLAY_STRING("CONT"), CLAY_TEXT_CONFIG(buttonCfg));
                }
                Clay_ElementDeclaration followBtn = CLAY_THEME_BTN_PRIMARY;
                followBtn.backgroundColor = Tracker_ButtonHoverColor(
                    self->followButton.clayId,
                    self->followCursor ? CLAY_COLOR_BTN_SUCCESS : CLAY_COLOR_BTN_PRIMARY,
                    self->followCursor ? 18.0f : 24.0f
                );
                CLAY(self->followButton.clayId, followBtn)
                {
                    CLAY_TEXT(CLAY_STRING("FOLLOW"), CLAY_TEXT_CONFIG(buttonCfg));
                }
                Clay_ElementDeclaration addPartBtn = CLAY_THEME_BTN_PRIMARY;
                addPartBtn.backgroundColor = Tracker_ButtonHoverColor(self->addPartButton.clayId, CLAY_COLOR_BTN_PRIMARY);
                CLAY(self->addPartButton.clayId, addPartBtn)
                {
                    CLAY_TEXT(CLAY_STRING("+PART"), CLAY_TEXT_CONFIG(buttonCfg));
                }
                Clay_ElementDeclaration songBtn = CLAY_THEME_BTN_PRIMARY;
                songBtn.backgroundColor = Tracker_ButtonHoverColor(self->songSettingsButton.clayId, CLAY_COLOR_BTN_PRIMARY);
                CLAY(self->songSettingsButton.clayId, songBtn)
                {
                    CLAY_TEXT(CLAY_STRING("SONG"), CLAY_TEXT_CONFIG(buttonCfg));
                }
                Clay_ElementDeclaration instrumentsBtn = CLAY_THEME_BTN_PRIMARY;
                instrumentsBtn.backgroundColor = Tracker_ButtonHoverColor(self->instrumentsButton.clayId, CLAY_COLOR_BTN_PRIMARY);
                CLAY(self->instrumentsButton.clayId, instrumentsBtn)
                {
                    CLAY_TEXT(CLAY_STRING("INSTR."), CLAY_TEXT_CONFIG(buttonCfg));
                }
                Clay_ElementDeclaration oscBtn = CLAY_THEME_BTN_PRIMARY;
                oscBtn.backgroundColor = Tracker_ButtonHoverColor(
                    self->oscilloscopeButton.clayId,
                    self->oscilloscopeVisible ? CLAY_COLOR_BTN_SUCCESS : CLAY_COLOR_BTN_PRIMARY,
                    self->oscilloscopeVisible ? 18.0f : 24.0f
                );
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
                bool usingEditSelection = Tracker_SelectionUsesEdit(self);
                bool canPaste = Tracker_CanPaste(self);

                Clay_TextElementConfig statusTitleCfg = bodyCfg;
                statusTitleCfg.fontSize = 11;
                statusTitleCfg.textColor = {162, 167, 180, 255};
                Clay_TextElementConfig statusValueCfg = bodyCfg;
                statusValueCfg.fontSize = 15;
                Clay_TextElementConfig statusMutedValueCfg = statusValueCfg;
                statusMutedValueCfg.textColor = {156, 161, 174, 255};
                Clay_TextElementConfig statusLightValueCfg = statusValueCfg;
                statusLightValueCfg.textColor = {246, 248, 251, 255};
                Clay_TextElementConfig statusDarkValueCfg = statusValueCfg;
                statusDarkValueCfg.textColor = {18, 20, 26, 255};
                Clay_TextElementConfig statusMismatchValueCfg = statusValueCfg;
                statusMismatchValueCfg.textColor = {255, 116, 116, 255};

                Clay_ElementDeclaration copyBtn = CLAY_THEME_BTN_PRIMARY;
                Clay_ElementDeclaration cutBtn = CLAY_THEME_BTN_PRIMARY;
                Clay_ElementDeclaration pasteBtn = CLAY_THEME_BTN_PRIMARY;
                Clay_ElementDeclaration editSelBtn = CLAY_THEME_BTN_PRIMARY;
                if (!hasSelection) copyBtn.backgroundColor = CLAY_COLOR_BTN_DISABLED;
                if (!hasSelection) cutBtn.backgroundColor = CLAY_COLOR_BTN_DISABLED;
                if (!canPaste) pasteBtn.backgroundColor = CLAY_COLOR_BTN_DISABLED;
                if (hasSelection)
                {
                    Clay_Color playSelectionBtn = {38, 92, 58, 255};
                    Clay_Color editSelectionBtn = {176, 156, 42, 255};
                    Clay_Color activeSelectionBtn = usingEditSelection ? editSelectionBtn : playSelectionBtn;
                    copyBtn.backgroundColor = activeSelectionBtn;
                    cutBtn.backgroundColor = activeSelectionBtn;
                    if (canPaste)
                        pasteBtn.backgroundColor = activeSelectionBtn;
                }
                copyBtn.backgroundColor = Tracker_ButtonHoverColor(
                    self->copyButton.clayId,
                    copyBtn.backgroundColor,
                    hasSelection ? 16.0f : 10.0f
                );
                cutBtn.backgroundColor = Tracker_ButtonHoverColor(
                    self->cutButton.clayId,
                    cutBtn.backgroundColor,
                    hasSelection ? 16.0f : 10.0f
                );
                pasteBtn.backgroundColor = Tracker_ButtonHoverColor(
                    self->pasteButton.clayId,
                    pasteBtn.backgroundColor,
                    canPaste ? 16.0f : 10.0f
                );
                editSelBtn.backgroundColor = Tracker_ButtonHoverColor(
                    self->editSelectionButton.clayId,
                    self->editSelectionEnabled ? (Clay_Color){176, 156, 42, 255} : (Clay_Color){50, 54, 68, 255},
                    16.0f
                );
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
                CLAY(self->editSelectionButton.clayId, editSelBtn)
                {
                    Clay_String editSelLabel = self->editSelectionEnabled ? CLAY_STRING("DES") : CLAY_STRING("SEL");
                    CLAY_TEXT(editSelLabel, CLAY_TEXT_CONFIG(buttonCfg));
                }
                CLAY(
                    CLAY_ID("TrackerStatusSpacer"),
                    {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()}}}
                )
                {
                }
                Clay_Color selectionBoxColor = hasSelection ?
                    (usingEditSelection ? (Clay_Color){176, 156, 42, 255} : (Clay_Color){38, 92, 58, 255}) :
                    (Clay_Color){76, 80, 92, 255};
                Clay_Color selectionValueColor = hasSelection ?
                    (usingEditSelection ? statusDarkValueCfg.textColor : statusLightValueCfg.textColor) :
                    statusMutedValueCfg.textColor;
                CLAY(
                    CLAY_ID("TrackerSelectionStatus"),
                    {.layout = {.sizing = {CLAY_SIZING_FIT(), CLAY_SIZING_FIXED(44)},
                                .childGap = 2,
                                .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_TOP},
                                .layoutDirection = CLAY_TOP_TO_BOTTOM}}
                )
                {
                    CLAY_TEXT(CLAY_STRING("selection"), CLAY_TEXT_CONFIG(statusTitleCfg));
                    CLAY(
                        CLAY_ID("TrackerSelectionValue"),
                        {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(24)},
                                    .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                    .layoutDirection = CLAY_LEFT_TO_RIGHT},
                         .backgroundColor = selectionBoxColor,
                         .cornerRadius = {4, 4, 4, 4}}
                    )
                    {
                        Clay_TextElementConfig selectionValueCfg = hasSelection ? statusValueCfg : statusMutedValueCfg;
                        selectionValueCfg.textColor = selectionValueColor;
                        Clay_String rowsText = ClayArena_FormatString(arena, "%d", hasSelection ? selectedRows : 0);
                        Clay_String colsText = ClayArena_FormatString(arena, "%d", hasSelection ? selectedChannels : 0);
                        CLAY(CLAY_ID("TrackerSelectionValueLine"), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                                                                .childGap = 0,
                                                                                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                                                                .layoutDirection = CLAY_LEFT_TO_RIGHT}})
                        {
                            CLAY_TEXT(CLAY_STRING("["), CLAY_TEXT_CONFIG(selectionValueCfg));
                            CLAY_TEXT(rowsText, CLAY_TEXT_CONFIG(selectionValueCfg));
                            CLAY_TEXT(CLAY_STRING(" x "), CLAY_TEXT_CONFIG(selectionValueCfg));
                            CLAY_TEXT(colsText, CLAY_TEXT_CONFIG(selectionValueCfg));
                            CLAY_TEXT(CLAY_STRING("]"), CLAY_TEXT_CONFIG(selectionValueCfg));
                        }
                    }
                }
                Clay_Color clipboardBoxColor = self->clipboard.valid ?
                    (canPaste ? (Clay_Color){98, 168, 110, 255} : (Clay_Color){164, 68, 68, 255}) :
                    (Clay_Color){76, 80, 92, 255};
                Clay_Color clipboardValueColor = self->clipboard.valid ?
                    (canPaste ? statusDarkValueCfg.textColor : statusLightValueCfg.textColor) :
                    statusMutedValueCfg.textColor;
                CLAY(
                    CLAY_ID("TrackerClipboardStatus"),
                    {.layout = {.sizing = {CLAY_SIZING_FIT(), CLAY_SIZING_FIXED(44)},
                                .childGap = 2,
                                .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_TOP},
                                .layoutDirection = CLAY_TOP_TO_BOTTOM}}
                )
                {
                    CLAY_TEXT(CLAY_STRING("clipboard"), CLAY_TEXT_CONFIG(statusTitleCfg));
                    CLAY(
                        CLAY_ID("TrackerClipboardValue"),
                        {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(24)},
                                    .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                    .layoutDirection = CLAY_LEFT_TO_RIGHT},
                         .backgroundColor = clipboardBoxColor,
                         .cornerRadius = {4, 4, 4, 4}}
                    )
                    {
                        Clay_TextElementConfig clipboardValueCfg = self->clipboard.valid ? statusValueCfg : statusMutedValueCfg;
                        clipboardValueCfg.textColor = clipboardValueColor;
                        Clay_String clipboardRowsText = ClayArena_FormatString(arena, "%d", self->clipboard.valid ? self->clipboard.rows : 0);
                        Clay_String clipboardColsText = ClayArena_FormatString(arena, "%d", self->clipboard.valid ? self->clipboard.channels : 0);
                        bool clipboardRowsMismatch = self->clipboard.valid && !canPaste && (!hasSelection || self->clipboard.rows != selectedRows);
                        bool clipboardColsMismatch = self->clipboard.valid && !canPaste && (!hasSelection || self->clipboard.channels != selectedChannels);
                        Clay_TextElementConfig clipboardRowsCfg = clipboardRowsMismatch ? statusMismatchValueCfg : clipboardValueCfg;
                        Clay_TextElementConfig clipboardColsCfg = clipboardColsMismatch ? statusMismatchValueCfg : clipboardValueCfg;
                        CLAY(CLAY_ID("TrackerClipboardValueLine"), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                                                                 .childGap = 0,
                                                                                 .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                                                                 .layoutDirection = CLAY_LEFT_TO_RIGHT}})
                        {
                            CLAY_TEXT(CLAY_STRING("["), CLAY_TEXT_CONFIG(clipboardValueCfg));
                            CLAY_TEXT(clipboardRowsText, CLAY_TEXT_CONFIG(clipboardRowsCfg));
                            CLAY_TEXT(CLAY_STRING(" x "), CLAY_TEXT_CONFIG(clipboardValueCfg));
                            CLAY_TEXT(clipboardColsText, CLAY_TEXT_CONFIG(clipboardColsCfg));
                            CLAY_TEXT(CLAY_STRING("]"), CLAY_TEXT_CONFIG(clipboardValueCfg));
                        }
                    }
                }
                CLAY(
                    CLAY_ID("TrackerStatusSpacerAfter"),
                    {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()}}}
                )
                {
                }
                if (self->clipboardBannerFlashTime > 0.0f && self->clipboardBannerText[0] != '\0')
                {
                    float pulse = 0.5f + 0.5f * sinf(self->clipboardBannerFlashTime * 10.0f);
                    float textA = glm::clamp(110.0f + 145.0f * pulse, 0.0f, 255.0f);
                    float bgA = glm::clamp(76.0f + 84.0f * pulse, 0.0f, 210.0f);
                    float outlineA = glm::clamp(90.0f + 90.0f * pulse, 0.0f, 255.0f);

                    Clay_Color bg;
                    Clay_Color outline;
                    Clay_Color textColor;
                    if (self->clipboardBannerKind == TRACKER_CLIPBOARD_BANNER_ERROR)
                    {
                        bg = {150.0f, 38.0f, 38.0f, bgA};
                        outline = {255.0f, 104.0f, 104.0f, outlineA};
                        textColor = {255.0f, 255.0f, 255.0f, textA};
                    }
                    else if (self->clipboardBannerUsesEditSelection)
                    {
                        bg = {176.0f, 156.0f, 42.0f, bgA};
                        outline = {255.0f, 240.0f, 150.0f, outlineA};
                        textColor = {28.0f, 24.0f, 12.0f, textA};
                    }
                    else
                    {
                        bg = {38.0f, 92.0f, 58.0f, bgA};
                        outline = {154.0f, 214.0f, 164.0f, outlineA};
                        textColor = {255.0f, 255.0f, 255.0f, textA};
                    }
                    Clay_TextElementConfig bannerCfg = bodyCfg;
                    bannerCfg.fontSize = 26;
                    bannerCfg.textColor = textColor;
                    Clay_String bannerStr = {
                        .length = (int32_t)std::strlen(self->clipboardBannerText),
                        .chars = self->clipboardBannerText
                    };
                    CLAY(
                        CLAY_ID("TrackerClipboardBanner"),
                        {
                            .layout = {
                                .sizing = {CLAY_SIZING_FIT(), CLAY_SIZING_FIT()},
                                .padding = {16, 24, 16, 24},
                                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                .layoutDirection = CLAY_LEFT_TO_RIGHT
                            },
                            .backgroundColor = bg,
                            .cornerRadius = {CLAY_RADIUS_XL, CLAY_RADIUS_XL, CLAY_RADIUS_XL, CLAY_RADIUS_XL},
                            .floating = {
                                .offset = {0, -trackerViewportHeight * 0.16f},
                                .zIndex = 80,
                                .attachPoints = {CLAY_ATTACH_POINT_CENTER_CENTER, CLAY_ATTACH_POINT_CENTER_CENTER},
                                .pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH,
                                .attachTo = CLAY_ATTACH_TO_PARENT,
                            },
                            .border = {.color = outline, .width = CLAY_BORDER_ALL(2)}
                        }
                    )
                    {
                        CLAY_TEXT(bannerStr, CLAY_TEXT_CONFIG(bannerCfg));
                    }
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
        Tracker_FlashCell(self, self->editRow, self->editChannel, TRACKER_CHANGE_FLASH_EDIT);
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
        self->instrumentEditorOpenedFromCellEditor = true;
        self->instrumentEditorOpenedFromInstrumentsWindow = false;
        return true;
    }
    if (isClaytonClicked(&self->instrumentExplicitButton, e))
    {
        Tracker_ToggleEditorInstrumentExplicit(self);
        Tracker_ApplyEditorToCell(self);
        Tracker_RequestEditorPreview(self);
        return true;
    }
    // Keep effect prev/next on the shared Clayton click path like instrument, macro,
    // and algo selectors. The old release-only hover path double-advanced on devices
    // that emit both native mouse and SDL_TOUCH_MOUSEID mouse-up for one tap.
    if (isClaytonClicked(&self->effectPrevButton, e))
    {
        self->editEffect = Tracker_NextEffectDefIndex(Tracker_SelectedEffectCode(self), -1);
        const TrackerEffectDef *def = &TRACKER_EFFECT_DEFS[Tracker_SelectedEffectDefIndex(self)];
        uint8_t clamped = Tracker_ClampEffectValueToDef(def, Tracker_SelectedEffectValue(self));
        Tracker_SetSelectedEffectValue(self, clamped);
        return true;
    }
    if (isClaytonClicked(&self->effectNextButton, e))
    {
        self->editEffect = Tracker_NextEffectDefIndex(Tracker_SelectedEffectCode(self), 1);
        const TrackerEffectDef *def = &TRACKER_EFFECT_DEFS[Tracker_SelectedEffectDefIndex(self)];
        uint8_t clamped = Tracker_ClampEffectValueToDef(def, Tracker_SelectedEffectValue(self));
        Tracker_SetSelectedEffectValue(self, clamped);
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
            Tracker_RequestEditorPreview(self, /*held=*/true);
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
        if (self->instrumentEditorOpenedFromCellEditor && self->editorOpen)
            Tracker_FlashEditorInstrumentSelector(self, TRACKER_CHANGE_FLASH_EDIT);
        else if (self->instrumentEditorOpenedFromInstrumentsWindow && self->instrumentsWindowOpen)
            Tracker_FlashInstrument(self, self->editInstrument, TRACKER_CHANGE_FLASH_EDIT);
        self->instrumentEditorOpenedFromCellEditor = false;
        self->instrumentEditorOpenedFromInstrumentsWindow = false;
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
        int valueMin = 0, valueMax = 0;
        Tracker_MacroTargetValueRange(self->editMacroTarget, valueMin, valueMax);
        Tracker_SetMacroValueViewMin(self, Tracker_MacroDefaultValueViewMin(self->editMacroTarget, valueMin, valueMax), self->editMacroTarget, valueMin, valueMax);
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
        int valueMin = 0, valueMax = 0;
        Tracker_MacroTargetValueRange(self->editMacroTarget, valueMin, valueMax);
        Tracker_SetMacroValueViewMin(self, Tracker_MacroDefaultValueViewMin(self->editMacroTarget, valueMin, valueMax), self->editMacroTarget, valueMin, valueMax);
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
    if (isClaytonClicked(&self->macroValueScrollUpButton, e))
    {
        int valueMin = 0, valueMax = 0;
        Tracker_MacroTargetValueRange(self->editMacroTarget, valueMin, valueMax);
        const int step = std::max(1, Tracker_MacroVisibleValueSpan(self->editMacroTarget, valueMin, valueMax) / 2);
        const int direction = Tracker_MacroTargetUsesInvertedVerticalValue(self->editMacroTarget) ? -1 : 1;
        Tracker_SetMacroValueViewMin(self, self->macroValueViewMin + direction * step, self->editMacroTarget, valueMin, valueMax);
        return true;
    }
    if (isClaytonClicked(&self->macroValueScrollDownButton, e))
    {
        int valueMin = 0, valueMax = 0;
        Tracker_MacroTargetValueRange(self->editMacroTarget, valueMin, valueMax);
        const int step = std::max(1, Tracker_MacroVisibleValueSpan(self->editMacroTarget, valueMin, valueMax) / 2);
        const int direction = Tracker_MacroTargetUsesInvertedVerticalValue(self->editMacroTarget) ? -1 : 1;
        Tracker_SetMacroValueViewMin(self, self->macroValueViewMin - direction * step, self->editMacroTarget, valueMin, valueMax);
        return true;
    }
    if (isClaytonClicked(&self->macroStepPrevButton, e))
    {
        XfmMacro &macro = Tracker_EditableMacro(self);
        int enabledColumns = Tracker_MacroEnabledColumns(&macro);
        if (enabledColumns > 0)
            self->editMacroValueIndex = (self->editMacroValueIndex + enabledColumns - 1) % enabledColumns;
        return true;
    }
    if (isClaytonClicked(&self->macroStepNextButton, e))
    {
        XfmMacro &macro = Tracker_EditableMacro(self);
        int enabledColumns = Tracker_MacroEnabledColumns(&macro);
        if (enabledColumns > 0)
            self->editMacroValueIndex = (self->editMacroValueIndex + 1) % enabledColumns;
        return true;
    }
    if (isClaytonClicked(&self->macroLoopButton, e))
    {
        XfmMacro &macro = Tracker_EditableMacro(self);
        if (macro.has_loop)
            Tracker_ClearMacroLoopRange(self);
        self->macroSelectMode = TRACKER_MACRO_SELECT_LOOP;
        return true;
    }
    if (isClaytonClicked(&self->macroReleaseButton, e))
    {
        XfmMacro &macro = Tracker_EditableMacro(self);
        if (!Tracker_MacroTargetSupportsRelease(macro.target))
        {
            self->macroSelectMode = TRACKER_MACRO_SELECT_LOOP;
            return true;
        }
        if (macro.release_start != 0xFF)
            Tracker_ClearMacroReleaseStart(self);
        self->macroSelectMode = TRACKER_MACRO_SELECT_RELEASE;
        return true;
    }
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
    {
        for (int i = self->macroViewFirst; i < self->macroViewFirst + TRACKER_MACRO_VISIBLE_STEPS && i < TRACKER_MACRO_UI_STEPS; i++)
        {
            if (Clay_PointerOver(CLAY_IDI("TrackerMacroReset", i)))
            {
                Tracker_DisableMacroFrom(self, i);
                return true;
            }
            if (Clay_PointerOver(CLAY_IDI("TrackerMacroValueNumber", i)))
            {
                self->macroRangeSelecting = true;
                self->macroRangeAnchor = i;
                if (self->macroSelectMode == TRACKER_MACRO_SELECT_LOOP)
                    Tracker_SetMacroLoopRange(self, i, i);
                else
                    Tracker_SetMacroReleaseStart(self, i);
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
    if (isClaytonClicked(&self->operatorEditorPrevButton, e))
    {
        Tracker_CycleEditOperator(self, -1);
        return true;
    }
    if (isClaytonClicked(&self->operatorEditorNextButton, e))
    {
        Tracker_CycleEditOperator(self, +1);
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
            if (self->macroSelectMode == TRACKER_MACRO_SELECT_LOOP)
                Tracker_SetMacroLoopRange(self, self->macroRangeAnchor, i);
            else
                Tracker_SetMacroReleaseStart(self, i);
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
        Tracker_EnsureMacroCapacity(&macro);
        int target = std::max((int)XFM_MACRO_TL1, std::min(Tracker_MacroMaxTarget(), self->editMacroTarget));
        int valueMin = -64;
        int valueMax = 127;
        Tracker_MacroTargetValueRange(target, valueMin, valueMax);
        Tracker_EnsureMacroValueViewForRange(self, target, valueMin, valueMax);
        const int visibleSpan = Tracker_MacroVisibleValueSpan(target, valueMin, valueMax);
        const int viewMin = self->macroValueViewMin;
        const int viewMax = std::min(valueMax, viewMin + visibleSpan);
        bool graphActive = self->macroDrawing || Clay_PointerOver(CLAY_ID("TrackerMacroGraphClip"));
        if (graphActive)
        {
            Clay_BoundingBox b = Clay_GetElementData(CLAY_ID("TrackerMacroGraphClip")).boundingBox;
            float pointerY = e.type == SDL_MOUSEMOTION ? (float)e.motion.y : (float)e.button.y;
            float yT = b.height > 0.0f ? (pointerY - b.y) / b.height : 0.0f;
            yT = std::max(0.0f, std::min(1.0f, yT));
            int idx = Tracker_MacroVisibleIndexAtX(self, pointerX);
            Tracker_EnableMacroThrough(self, idx);
            const bool invertedVerticalMacro = Tracker_MacroTargetUsesInvertedVerticalValue(target);
            int drawn = invertedVerticalMacro
                ? viewMin + (int)std::round(yT * (float)(viewMax - viewMin))
                : viewMax - (int)std::round(yT * (float)(viewMax - viewMin));
            drawn = std::max(valueMin, std::min(valueMax, drawn));
            macro.values[idx] = (int16_t)drawn;
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
    if (isClaytonClicked(&self->operatorEditorPrevButton, e))
    {
        Tracker_CycleEditOperator(self, -1);
        return true;
    }
    if (isClaytonClicked(&self->operatorEditorNextButton, e))
    {
        Tracker_CycleEditOperator(self, +1);
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
        auto sliderValue = [&](Clay_ElementId id, int minValue, int maxValue, int &out, bool invertValue = false) -> bool {
            if (!Tracker_CapturedSlider(self, id, e)) return false;
            out = Tracker_ValueFromSliderX(id, pointerX, minValue, maxValue);
            if (invertValue)
                out = minValue + maxValue - out;
            return true;
        };
        int value = 0;
        if (sliderValue(CLAY_ID("TrackerOpTlBar"), 0, 127, value, true)) op.TL = (uint8_t)value;
        else if (sliderValue(CLAY_ID("TrackerOpArBar"), 0, 31, value, true)) op.AR = (uint8_t)value;
        else if (sliderValue(CLAY_ID("TrackerOpDrBar"), 0, 31, value, true)) op.DR = (uint8_t)value;
        else if (sliderValue(CLAY_ID("TrackerOpSlBar"), 0, 15, value, true)) op.SL = (uint8_t)value;
        else if (sliderValue(CLAY_ID("TrackerOpSrBar"), 0, 31, value, true)) op.SR = (uint8_t)value;
        else if (sliderValue(CLAY_ID("TrackerOpRrBar"), 0, 15, value, true)) op.RR = (uint8_t)value;
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
            self->instrumentEditorOpenedFromCellEditor = false;
            self->instrumentEditorOpenedFromInstrumentsWindow = true;
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
    if (isClaytonClicked(&self->songScaleRootPrevButton, e))
    {
        self->songScaleRoot = Tracker_NextSongScaleRoot(self->songScaleRoot, -1);
        self->patternDirty = true;
        self->copyOnWriteRequested = true;
        return true;
    }
    if (isClaytonClicked(&self->songScaleRootNextButton, e))
    {
        self->songScaleRoot = Tracker_NextSongScaleRoot(self->songScaleRoot, 1);
        self->patternDirty = true;
        self->copyOnWriteRequested = true;
        return true;
    }
    if (isClaytonClicked(&self->songScalePrevButton, e))
    {
        self->songScaleMode = Tracker_NextSongScaleMode(self->songScaleMode, -1);
        self->patternDirty = true;
        self->copyOnWriteRequested = true;
        return true;
    }
    if (isClaytonClicked(&self->songScaleNextButton, e))
    {
        self->songScaleMode = Tracker_NextSongScaleMode(self->songScaleMode, 1);
        self->patternDirty = true;
        self->copyOnWriteRequested = true;
        return true;
    }
    if (isClaytonClicked(&self->songLoadEmptyButton, e))
    {
        Tracker_LoadEmptyPatternState(self);
        self->songSettingsWindowOpen = false;
        self->songSettingsWindowRequested = false;
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
        Tracker_FlashPart(self, partIndex, TRACKER_CHANGE_FLASH_EDIT);
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
    if (isClaytonClicked(&self->partEditorCloneButton, e))
    {
        if (Tracker_ClonePartAfter(self, partIndex))
        {
            self->partEditorPart = std::max(0, std::min(self->partCount - 1, partIndex + 1));
            Tracker_FlashPart(self, self->partEditorPart, TRACKER_CHANGE_FLASH_ADD);
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
        Tracker_AddPartToEnd(self);
        int newPartIndex = std::max(0, self->partCount - 1);
        for (int i = 1; i < 32 && self->rowCount < TRACKER_MAX_ROWS; i++)
            Tracker_AddRowToPart(self, newPartIndex);
        Tracker_FlashPart(self, newPartIndex, TRACKER_CHANGE_FLASH_ADD);
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
        if (isClaytonClicked(&self->stickyPartEnableButton, e))
        {
            self->parts[stickyPart].enabled = !self->parts[stickyPart].enabled;
            self->patternDirty = true;
            self->copyOnWriteRequested = true;
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
        if (isClaytonClicked(&self->partEnableButtons[i], e))
        {
            self->parts[i].enabled = !self->parts[i].enabled;
            self->patternDirty = true;
            self->copyOnWriteRequested = true;
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
    if (isClaytonClicked(&self->editSelectionButton, e))
    {
        if (self->gridNoteAuditionActive)
            Tracker_StopGridNoteAudition(self);
        self->editSelectionEnabled = !self->editSelectionEnabled;
        self->editSelecting = false;
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
    static constexpr uint64_t TRACKER_PART_PROGRESS_SHORT_CLICK_MS = 360;
    static constexpr float TRACKER_PART_PROGRESS_DRAG_Y_PX = 4.0f;

    auto pointerOverPartButton = [&]() -> bool {
        if (Clay_PointerOver(self->stickyPartToggleButton.clayId) ||
            Clay_PointerOver(self->stickyPartEnableButton.clayId) ||
            Clay_PointerOver(self->stickyPartUpButton.clayId) ||
            Clay_PointerOver(self->stickyPartDownButton.clayId) ||
            Clay_PointerOver(self->stickyPartSettingsButton.clayId))
            return true;
        for (int i = 0; i < self->partCount; i++)
        {
            if (Clay_PointerOver(self->partToggleButtons[i].clayId) ||
                Clay_PointerOver(self->partEnableButtons[i].clayId) ||
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
    auto partProgressRailAtPointer = [&](int *outPart, float *outRailX, float *outRailW) -> bool {
        auto checkRail = [&](int partIndex, int railKey) -> bool {
            if (partIndex < 0 || partIndex >= self->partCount)
                return false;
            Clay_ElementId id = CLAY_IDI("TrackerPartProgressRail", railKey);
            if (!Clay_PointerOver(id))
                return false;
            Clay_BoundingBox rail = Clay_GetElementData(id).boundingBox;
            if (rail.width <= 1.0f || rail.height <= 1.0f)
                return false;
            if (outPart) *outPart = partIndex;
            if (outRailX) *outRailX = rail.x;
            if (outRailW) *outRailW = rail.width;
            return true;
        };
        int stickyPart = Tracker_StickyPartIndexAtScroll(self);
        if (checkRail(stickyPart, stickyPart * 2 + 1))
            return true;
        for (int partIndex = 0; partIndex < self->partCount; partIndex++)
        {
            if (checkRail(partIndex, partIndex * 2))
                return true;
        }
        return false;
    };
    if (pointerDown)
    {
        int progressPart = -1;
        float railX = 0.0f;
        float railW = 0.0f;
        if (partProgressRailAtPointer(&progressPart, &railX, &railW))
        {
            self->followCursor = false;
            self->partProgressScrubPending = true;
            self->partProgressScrubMovedY = false;
            self->partProgressScrubPart = progressPart;
            self->partProgressScrubStartedAtMs = Tracker_NowMs();
            self->partProgressScrubStartX = pointerX();
            self->partProgressScrubStartY = pointerY();
            self->partProgressScrubRailX = railX;
            self->partProgressScrubRailW = railW;
            self->dragging = false;
            self->dragMoved = false;
            self->scrollVelocity = 0.0f;
            return true;
        }
    }
    if (pointerMove && self->partProgressScrubPending)
    {
        float y = pointerY();
        if (std::fabs(y - self->partProgressScrubStartY) > TRACKER_PART_PROGRESS_DRAG_Y_PX)
        {
            self->partProgressScrubMovedY = true;
            self->partProgressScrubPending = false;
            self->partProgressScrubPart = -1;
            Tracker_BeginScrollDragFromPendingCellMove(self, self->partProgressScrubStartY);
        }
        return true;
    }
    if (pointerUp && self->partProgressScrubPending)
    {
        uint64_t elapsed = Tracker_NowMs() - self->partProgressScrubStartedAtMs;
        if (!self->partProgressScrubMovedY && elapsed <= TRACKER_PART_PROGRESS_SHORT_CLICK_MS)
        {
            Tracker_SetPlayheadFromPartProgressX(
                self,
                self->partProgressScrubPart,
                pointerX(),
                self->partProgressScrubRailX,
                self->partProgressScrubRailW
            );
        }
        self->partProgressScrubPending = false;
        self->partProgressScrubMovedY = false;
        self->partProgressScrubPart = -1;
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
    auto selectionPointToRowChannel = [&](float x, float y, int *outRow, int *outChannel) -> bool {
        if (grid.width <= 0.0f || grid.height <= 0.0f) return false;
        float localX = x - grid.x;
        float localY = y - grid.y;
        float unit = grid.width / 13.0f;
        if (localX < unit) return false;
        int channel = (int)std::floor((localX - unit) / (unit * 2.0f));
        channel = std::max(0, std::min(TRACKER_CHANNELS - 1, channel));
        TrackerVisualRow visual = Tracker_MapVisualIndex(self, Tracker_VisualIndexAtViewportY(self, localY));
        if (visual.kind != TRACKER_VISUAL_ROW_CELL || visual.row < 0 || visual.row >= self->rowCount)
            return false;
        if (outRow) *outRow = visual.row;
        if (outChannel) *outChannel = channel;
        return true;
    };
    if (pointerMove && self->gridNoteAuditionActive && self->gridNoteAuditionSelectionMode)
    {
        int row = -1;
        int channel = -1;
        if (cellAtGridPoint(pointerX(), pointerY(), &row, &channel))
            Tracker_StartGridNoteAudition(self, row, channel, /*selectionMode=*/true);
        else
            Tracker_StopGridNoteAudition(self);
    }
    if (pointerUp && self->gridNoteAuditionActive)
        Tracker_StopGridNoteAudition(self);

    if (pointerDown && overGrid)
    {
        // Touch/devtools can emit the same gesture twice (e.g. synthetic mouse + touch).
        // Once an edit-selection drag is already active, a second press should not restart it.
        if (self->editSelecting || self->editMoving)
            return true;
        float px = pointerX();
        float py = pointerY();
        float localX = px - grid.x;
        float localY = py - grid.y;
        int row = Tracker_RowAtViewportY(self, localY);
        int channel = Tracker_ChannelAtGridX(localX, grid.width);
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
        bool hasCell = selectionPointToRowChannel(px, py, &row, &channel);
        if (hasCell)
            Tracker_StartGridNoteAudition(self, row, channel, self->editSelectionEnabled);
        if (self->editSelectionEnabled && Tracker_HasEditSelection(self) &&
            Tracker_EditSelectionContains(self, row, channel))
        {
            self->followCursor = false;
            self->loopSelecting = false;
            self->loopMoving = false;
            self->dragging = false;
            self->dragMoved = true;
            self->editSelecting = false;
            self->editMoving = true;
            self->editMoveGrabRowOffset = row - self->editSelectionStartRow;
            self->editMoveGrabChannelOffset = channel - self->editSelectionStartChannel;
            self->editMoveBaseStartRow = self->editSelectionStartRow;
            self->editMoveBaseStartChannel = self->editSelectionStartChannel;
            self->editMovePointerStartRow = row;
            self->editMovePointerStartChannel = channel;
            self->editSelectLocalY = localY;
            self->editSelectLocalX = localX;
            self->editSelectViewportWidth = grid.width;
            self->editSelectViewportHeight = grid.height;
            self->scrollVelocity = 0.0f;
            Tracker_CancelCellMove(self);
            return true;
        }
        if (self->editSelectionEnabled && hasCell)
        {
            self->followCursor = false;
            self->loopSelecting = false;
            self->loopMoving = false;
            self->dragging = false;
            self->dragMoved = true;
            self->editSelecting = true;
            self->editMoving = false;
            self->editSelectLocalY = localY;
            self->editSelectLocalX = localX;
            self->editSelectViewportWidth = grid.width;
            self->editSelectViewportHeight = grid.height;
            self->scrollVelocity = 0.0f;
            Tracker_CancelCellMove(self);
            self->editSelectionValid = false;
            self->editMoveGrabRowOffset = 0;
            self->editMoveGrabChannelOffset = 0;
            self->editMoveBaseStartRow = row;
            self->editMoveBaseStartChannel = channel;
            self->editMovePointerStartRow = row;
            self->editMovePointerStartChannel = channel;
            Tracker_SetEditSelection(self, row, row, channel, channel);
            return true;
        }
        if (hasCell && Tracker_CellMoveCanStart(self, row, channel))
        {
            Tracker_BeginCellMovePending(self, row, channel, px, py, Tracker_NowMs());
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
    if (pointerMove && (self->editSelecting || self->editMoving))
    {
        float px = pointerX();
        float py = pointerY();
        float localY = pointerY() - grid.y;
        float localX = pointerX() - grid.x;
        int row = Tracker_RowAtViewportY(self, localY);
        int channel = Tracker_ChannelAtGridX(localX, grid.width);
        self->editSelectLocalX = localX;
        self->editSelectLocalY = localY;
        self->editSelectViewportWidth = grid.width;
        self->editSelectViewportHeight = grid.height;
        if (self->editMoving)
        {
            self->editSelectionCurrentChannel = channel;
            Tracker_MoveEditSelectionByPointer(self, row, channel);
        }
        else
        {
            if (cellAtGridPoint(px, py, &row, &channel))
                self->editSelectionCurrentChannel = channel;
            Tracker_SetEditSelection(self, self->editSelectionAnchorRow, row, self->editSelectionAnchorChannel, self->editSelectionCurrentChannel);
        }
        return true;
    }
    if (pointerMove && self->cellMovePending && !self->cellMoving)
    {
        float px = pointerX();
        float py = pointerY();
        Tracker_UpdateCellMovePendingPointer(self, px, py);
        int row = -1;
        int channel = -1;
        bool insideCell = cellAtGridPoint(px, py, &row, &channel);
        bool leftSourceCell = !insideCell ||
            row != self->cellMovePendingRow ||
            channel != self->cellMovePendingChannel;
        if (leftSourceCell ||
            std::fabs(px - self->cellMovePendingStartX) > 8.0f ||
            std::fabs(py - self->cellMovePendingStartY) > 8.0f)
        {
            float startY = self->cellMovePendingStartY;
            Tracker_SuppressCellMovePending(self);
            Tracker_BeginScrollDragFromPendingCellMove(self, startY);
        }
        else
        {
            return true;
        }
    }
    if (pointerUp && (self->editSelecting || self->editMoving))
    {
        float px = pointerX();
        float py = pointerY();
        float localY = pointerY() - grid.y;
        float localX = pointerX() - grid.x;
        int row = Tracker_RowAtViewportY(self, localY);
        int channel = Tracker_ChannelAtGridX(localX, grid.width);
        self->editSelectLocalX = localX;
        self->editSelectLocalY = localY;
        self->editSelectViewportWidth = grid.width;
        self->editSelectViewportHeight = grid.height;
        if (self->editMoving)
        {
            Tracker_MoveEditSelectionByPointer(self, row, channel);
        }
        else
        {
            if (cellAtGridPoint(px, py, &row, &channel))
                self->editSelectionCurrentChannel = channel;
            Tracker_SetEditSelection(self, self->editSelectionAnchorRow, row, self->editSelectionAnchorChannel, self->editSelectionCurrentChannel);
        }
        self->editSelecting = false;
        self->editMoving = false;
        Tracker_SnapToGrid(self);
        return true;
    }
    if (pointerUp && self->cellMovePending && !self->cellMoving)
    {
        float px = pointerX();
        float py = pointerY();
        Tracker_UpdateCellMovePendingPointer(self, px, py);
        int row = -1;
        int channel = -1;
        bool releasedInsideSourceCell = cellAtGridPoint(px, py, &row, &channel) &&
            Tracker_IsTapReleaseForPendingCellMove(self, row, channel);
        bool armedMove = Tracker_TryArmCellMovePending(self, Tracker_NowMs());
        Tracker_CancelCellMovePending(self);
        if (!armedMove && releasedInsideSourceCell)
            Tracker_OpenEditor(self, row, channel);
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
            Tracker_CancelCellMovePending(self);
            if (tapSource)
                Tracker_OpenEditor(self, sourceRow, sourceChannel);
        }
        else
        {
            Tracker_CancelCellMovePending(self);
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
        Tracker_CancelCellMovePending(self);
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
