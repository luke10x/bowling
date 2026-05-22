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

static constexpr float TRACKER_SIDE_UNIT = 1.0f / 14.0f;
static constexpr float TRACKER_CHANNEL_UNIT = 2.0f / 14.0f;
static constexpr float TRACKER_SCROLLABLE_UNIT = 13.0f / 14.0f;
static constexpr float TRACKER_LINE_IN_SCROLL = 1.0f / 13.0f;
static constexpr float TRACKER_CHANNEL_IN_SCROLL = 2.0f / 13.0f;

inline float Tracker_ScrollbarThumbHeight(const Tracker *self)
{
    if (!self || self->viewportHeight <= 1.0f) return 28.0f;
    float contentHeight = std::max(self->rowHeight, (float)self->rowCount * self->rowHeight);
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

inline void Tracker_BuildEditor(Tracker *self, Clayton *clayton)
{
    if (!self || !self->editorOpen || !clayton) return;

    Clay_TextElementConfig titleCfg = CLAY_THEME_TEXT_TITLE;
    Clay_TextElementConfig buttonCfg = CLAY_THEME_TEXT_BUTTON;
    Clay_TextElementConfig bodyCfg = CLAY_THEME_TEXT_BODY;
    ClayArena *arena = &clayton->clayArena;

    CLAY(
        CLAY_ID("TrackerEditorWindow"),
        {
            .layout = {
                .sizing = {CLAY_SIZING_PERCENT(0.96f), CLAY_SIZING_PERCENT(0.90f)},
                .padding = {10, 10, 10, 10},
                .childGap = 8,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
            .backgroundColor = CLAY_COLOR_PANEL_BG,
            .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
            .border = {.color = CLAY_COLOR_BORDER, .width = CLAY_BORDER_ALL(2)},
        }
    )
    {
            CLAY(
                CLAY_ID("TrackerEditorTitle"),
                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                            .childGap = 8,
                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                            .layoutDirection = CLAY_LEFT_TO_RIGHT}}
            )
            {
                Clay_String title = ClayArena_FormatString(
                    arena, "Edit R%03d CH%d", self->editRow, self->editChannel + 1
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
                            .childGap = 8,
                            .layoutDirection = CLAY_LEFT_TO_RIGHT}}
            )
            {
                Clay_ElementDeclaration tab = CLAY_THEME_BTN_PRIMARY;
                tab.layout.sizing.height = CLAY_SIZING_FIXED(42);
                tab.backgroundColor = self->editorTab == 0 ? CLAY_COLOR_BTN_ACTIVE : CLAY_COLOR_BTN_PRIMARY;
                CLAY(self->editorNoteTabButton.clayId, tab)
                {
                    CLAY_TEXT(CLAY_STRING("NOTE"), CLAY_TEXT_CONFIG(buttonCfg));
                }
                tab.backgroundColor = self->editorTab == 1 ? CLAY_COLOR_BTN_ACTIVE : CLAY_COLOR_BTN_PRIMARY;
                CLAY(self->editorInstrumentTabButton.clayId, tab)
                {
                    CLAY_TEXT(CLAY_STRING("INSTR"), CLAY_TEXT_CONFIG(buttonCfg));
                }
            }

            if (self->editorTab == 0)
            {
                const char *whiteKeys[7] = {"C", "D", "E", "F", "G", "A", "B"};
                for (int octave = 1; octave <= 7; octave++)
                {
                    CLAY(
                        CLAY_IDI("TrackerKeyboardRow", octave),
                        {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                    .childGap = 4,
                                    .layoutDirection = CLAY_LEFT_TO_RIGHT}}
                    )
                    {
                        Clay_String octLabel = ClayArena_FormatString(arena, "%d", octave);
                        CLAY(
                            CLAY_IDI("TrackerOctaveLabel", octave),
                            {.layout = {.sizing = {CLAY_SIZING_FIXED(30), CLAY_SIZING_GROW()},
                                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}},
                             .backgroundColor = {45, 45, 65, 255}}
                        )
                        {
                            CLAY_TEXT(octLabel, CLAY_TEXT_CONFIG(bodyCfg));
                        }
                        for (int k = 0; k < 7; k++)
                        {
                            CLAY(
                                CLAY_IDI("TrackerWhiteKey", octave * 10 + k),
                                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}},
                                 .backgroundColor = {220, 224, 235, 255},
                                 .cornerRadius = {3, 3, 3, 3},
                                 .border = {.color = {80, 80, 100, 255}, .width = CLAY_BORDER_ALL(1)}}
                            )
                            {
                                Clay_String label = ClayArena_FormatString(arena, "%s%d", whiteKeys[k], octave);
                                Clay_TextElementConfig dark = bodyCfg;
                                dark.textColor = {20, 20, 30, 255};
                                CLAY_TEXT(label, CLAY_TEXT_CONFIG(dark));
                            }
                        }
                    }
                }
            }
            else
            {
                CLAY(CLAY_ID("TrackerInstrumentSelector"), CLAY_THEME_SECTION)
                {
                    CLAY(
                        CLAY_ID("TrackerInstrumentSelectorRow"),
                        {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                                    .childGap = 8,
                                    .layoutDirection = CLAY_LEFT_TO_RIGHT}}
                    )
                    {
                        CLAY(self->instrumentPrevButton.clayId, CLAY_THEME_BTN_PRIMARY)
                        {
                            CLAY_TEXT(CLAY_STRING("<"), CLAY_TEXT_CONFIG(buttonCfg));
                        }
                        Clay_String name = ClayArena_FormatString(arena, "Instrument %02X", self->editInstrument);
                        CLAY(
                            CLAY_ID("TrackerInstrumentName"),
                            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(60)},
                                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}},
                             .backgroundColor = {35, 45, 65, 255},
                             .cornerRadius = {CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD}}
                        )
                        {
                            CLAY_TEXT(name, CLAY_TEXT_CONFIG(buttonCfg));
                        }
                        CLAY(self->instrumentNextButton.clayId, CLAY_THEME_BTN_PRIMARY)
                        {
                            CLAY_TEXT(CLAY_STRING(">"), CLAY_TEXT_CONFIG(buttonCfg));
                        }
                        CLAY(self->instrumentAddButton.clayId, CLAY_THEME_BTN_SUCCESS)
                        {
                            CLAY_TEXT(CLAY_STRING("+"), CLAY_TEXT_CONFIG(buttonCfg));
                        }
                        CLAY(self->instrumentRemoveButton.clayId, CLAY_THEME_BTN_DANGER)
                        {
                            CLAY_TEXT(CLAY_STRING("-"), CLAY_TEXT_CONFIG(buttonCfg));
                        }
                    }
                    CLAY_TEXT(CLAY_STRING("ALGO  BASE  FMS  AMS  FB"), CLAY_TEXT_CONFIG(bodyCfg));
                    CLAY_TEXT(CLAY_STRING("OP1 OP2 OP3 OP4 tabs will use sliders for TL, AR, DR, SR, SL, RR, MUL, DT, AM."), CLAY_TEXT_CONFIG(bodyCfg));
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
    monoCfg.fontSize = 12;

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
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(42)},
                        .childGap = 8,
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
        )
        {
            Clay_String title = ClayArena_FormatString(
                arena,
                "OPN Tracker :: %s  R%03d.%d",
                Tracker_SongName(self->songIndex),
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
            ) {}
            for (int ch = 0; ch < TRACKER_CHANNELS; ch++)
            {
                CLAY(
                    CLAY_IDI("TrackerHeaderChannel", ch),
                    {.layout = {.sizing = {CLAY_SIZING_PERCENT(TRACKER_CHANNEL_UNIT), CLAY_SIZING_GROW()},
                                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}},
                     .backgroundColor = {38, 48, 74, 255}}
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
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
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
                self->viewportHeight = bb.height > 1.0f ? bb.height : 360.0f;

                int firstRow = std::max(0, (int)std::floor(self->scrollY / self->rowHeight) - 1);
                int visibleRows = std::min(self->rowCount - firstRow, (int)(self->viewportHeight / self->rowHeight) + 3);
                int afterRows = std::max(0, self->rowCount - firstRow - visibleRows);

                CLAY(
                    CLAY_ID("TrackerGridBelt"),
                    {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                                .layoutDirection = CLAY_TOP_TO_BOTTOM}}
                )
                {
                    if (firstRow > 0)
                    {
                        CLAY(
                            CLAY_ID("TrackerGridTopSpacer"),
                            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(firstRow * self->rowHeight)}}}
                        ) {}
                    }
                    for (int vr = 0; vr < visibleRows; vr++)
                    {
                        int row = firstRow + vr;
                        bool activeRow = row == self->playRow;
                        CLAY(
                            CLAY_IDI("TrackerGridRow", row),
                            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(self->rowHeight)},
                                        .childGap = 0,
                                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
                        )
                        {
                            CLAY(
                                CLAY_IDI("TrackerLineCell", row),
                                {.layout = {.sizing = {CLAY_SIZING_PERCENT(TRACKER_LINE_IN_SCROLL), CLAY_SIZING_GROW()},
                                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}},
                                 .backgroundColor = Tracker_CellColor(activeRow, true),
                                 .border = {.color = {50, 56, 74, 255}, .width = CLAY_BORDER_ALL(1)}}
                            )
                            {
                                Clay_String rn = ClayArena_FormatString(arena, "%03d", row);
                                CLAY_TEXT(rn, CLAY_TEXT_CONFIG(monoCfg));
                            }
                            for (int ch = 0; ch < TRACKER_CHANNELS; ch++)
                            {
                                CLAY(
                                    CLAY_IDI("TrackerCell", row * 10 + ch),
                                    {.layout = {.sizing = {CLAY_SIZING_PERCENT(TRACKER_CHANNEL_IN_SCROLL), CLAY_SIZING_GROW()},
                                                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}},
                                     .backgroundColor = Tracker_CellColor(activeRow, false),
                                     .border = {.color = {50, 56, 74, 255}, .width = CLAY_BORDER_ALL(1)}}
                                )
                                {
                                    Clay_String txt = ClayArena_AllocString(arena, self->cells[row][ch].text);
                                    CLAY_TEXT(txt, CLAY_TEXT_CONFIG(monoCfg));
                                }
                            }
                        }
                    }
                    if (afterRows > 0)
                    {
                        CLAY(
                            CLAY_ID("TrackerGridBottomSpacer"),
                            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(afterRows * self->rowHeight)}}}
                        ) {}
                    }
                }
            }

            float thumbHeight = Tracker_ScrollbarThumbHeight(self);
            float thumbTop = Tracker_ScrollbarThumbTop(self, thumbHeight);
            float thumbBottom = std::max(0.0f, self->viewportHeight - thumbTop - thumbHeight);
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
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(82)},
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
                    CLAY_TEXT(CLAY_STRING("PLAY"), CLAY_TEXT_CONFIG(buttonCfg));
                }
                CLAY(self->stopButton.clayId, CLAY_THEME_BTN_PRIMARY)
                {
                    CLAY_TEXT(CLAY_STRING("STOP"), CLAY_TEXT_CONFIG(buttonCfg));
                }
                Clay_ElementDeclaration followBtn = CLAY_THEME_BTN_PRIMARY;
                if (self->followCursor) followBtn.backgroundColor = CLAY_COLOR_BTN_SUCCESS;
                CLAY(self->followButton.clayId, followBtn)
                {
                    CLAY_TEXT(CLAY_STRING("FOLLOW"), CLAY_TEXT_CONFIG(buttonCfg));
                }
                CLAY(self->addRowButton.clayId, CLAY_THEME_BTN_PRIMARY)
                {
                    CLAY_TEXT(CLAY_STRING("+ROW"), CLAY_TEXT_CONFIG(buttonCfg));
                }
                CLAY(self->removeRowButton.clayId, CLAY_THEME_BTN_PRIMARY)
                {
                    CLAY_TEXT(CLAY_STRING("-ROW"), CLAY_TEXT_CONFIG(buttonCfg));
                }
            }
            CLAY(
                CLAY_ID("TrackerSongRow"),
                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                            .childGap = 5,
                            .layoutDirection = CLAY_LEFT_TO_RIGHT}}
            )
            {
                for (int i = 0; i < 4; i++)
                {
                    Clay_ElementDeclaration btn = CLAY_THEME_BTN_PRIMARY;
                    btn.backgroundColor = self->songIndex == i + 1 ? CLAY_COLOR_BTN_ACTIVE : CLAY_COLOR_BTN_PRIMARY;
                    btn.layout.sizing.height = CLAY_SIZING_GROW();
                    CLAY(self->songButtons[i].clayId, btn)
                    {
                        Clay_String label = ClayArena_FormatString(arena, "S%d", i + 1);
                        CLAY_TEXT(label, CLAY_TEXT_CONFIG(buttonCfg));
                    }
                }
            }
        }
    }

}

inline void Tracker_OpenEditor(Tracker *self, int row, int channel)
{
    if (!self) return;
    self->editRow = std::max(0, std::min(row, self->rowCount - 1));
    self->editChannel = std::max(0, std::min(channel, TRACKER_CHANNELS - 1));
    self->editorOpen = true;
    self->editorWindowRequested = true;
    self->editorTab = 0;
}

inline bool Tracker_HandleEditorWindowEvent(Tracker *self, const SDL_Event &e)
{
    if (!self || !self->editorOpen) return false;

    if (isClaytonClicked(&self->editorCloseButton, e) ||
        isClaytonClicked(&self->editorCancelButton, e))
    {
        self->editorOpen = false;
        return true;
    }
    if (isClaytonClicked(&self->editorNoteTabButton, e))
    {
        self->editorTab = 0;
        return true;
    }
    if (isClaytonClicked(&self->editorInstrumentTabButton, e))
    {
        self->editorTab = 1;
        return true;
    }
    if (isClaytonClicked(&self->instrumentPrevButton, e))
    {
        self->editInstrument = (self->editInstrument + 255) & 0xFF;
        return true;
    }
    if (isClaytonClicked(&self->instrumentNextButton, e))
    {
        self->editInstrument = (self->editInstrument + 1) & 0xFF;
        return true;
    }

    const bool pointerEvent =
        e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP || e.type == SDL_MOUSEMOTION ||
        e.type == SDL_MOUSEWHEEL || e.type == SDL_FINGERDOWN || e.type == SDL_FINGERUP ||
        e.type == SDL_FINGERMOTION;
    if (pointerEvent && Clay_PointerOver(CLAY_ID("TrackerEditorWindow"))) return true;
    return pointerEvent;
}

inline bool Tracker_HandleEvent(Tracker *self, Clayton *clayton, const SDL_Event &e)
{
    if (!self || !self->active) return false;

    if (isClaytonClicked(&self->closeButton, e))
    {
        Tracker_Close(self);
        return true;
    }
    if (isClaytonClicked(&self->playButton, e))
    {
        self->playing = true;
        return true;
    }
    if (isClaytonClicked(&self->stopButton, e))
    {
        self->playing = false;
        return true;
    }
    if (isClaytonClicked(&self->followButton, e))
    {
        self->followCursor = !self->followCursor;
        if (self->followCursor)
            setTrackerCursorState(self, self->playRow, self->playTick, self->ticksPerRow);
        return true;
    }
    if (isClaytonClicked(&self->addRowButton, e))
    {
        Tracker_AddRow(self);
        return true;
    }
    if (isClaytonClicked(&self->removeRowButton, e))
    {
        Tracker_RemoveRow(self);
        return true;
    }
    for (int i = 0; i < 4; i++)
    {
        if (isClaytonClicked(&self->songButtons[i], e))
        {
            Tracker_LoadSong(self, i + 1);
            return true;
        }
    }

    bool pointerEvent =
        e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP || e.type == SDL_MOUSEMOTION ||
        e.type == SDL_MOUSEWHEEL;
    if (!pointerEvent) return false;

    Clay_BoundingBox grid = Clay_GetElementData(CLAY_ID("TrackerGridViewport")).boundingBox;
    bool overGrid = Clay_PointerOver(CLAY_ID("TrackerGridViewport"));
    Clay_BoundingBox scrollbar = Clay_GetElementData(CLAY_ID("TrackerScrollbarRail")).boundingBox;
    Clay_BoundingBox thumb = Clay_GetElementData(CLAY_ID("TrackerScrollbarThumb")).boundingBox;
    bool overScrollbar = Clay_PointerOver(CLAY_ID("TrackerScrollbarRail"));
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT && overScrollbar)
    {
        self->followCursor = false;
        self->dragging = false;
        self->scrollbarDragging = true;
        float localY = (float)e.button.y - scrollbar.y;
        if (Clay_PointerOver(CLAY_ID("TrackerScrollbarThumb")))
            self->scrollbarGrabOffsetY = (float)e.button.y - thumb.y;
        else
            self->scrollbarGrabOffsetY = Tracker_ScrollbarThumbHeight(self) * 0.5f;
        Tracker_SetScrollFromScrollbarY(self, localY);
        return true;
    }
    if (e.type == SDL_MOUSEMOTION && self->scrollbarDragging)
    {
        float localY = (float)e.motion.y - scrollbar.y;
        Tracker_SetScrollFromScrollbarY(self, localY);
        return true;
    }
    if (e.type == SDL_MOUSEBUTTONUP && self->scrollbarDragging)
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
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT && overGrid)
    {
        self->followCursor = false;
        self->dragging = true;
        self->dragMoved = false;
        self->dragStartY = (float)e.button.y;
        self->dragLastY = (float)e.button.y;
        self->dragStartScrollY = self->scrollY;
        self->scrollVelocity = 0.0f;
        return true;
    }
    if (e.type == SDL_MOUSEMOTION && self->dragging)
    {
        float y = (float)e.motion.y;
        float dy = y - self->dragLastY;
        if (std::fabs(y - self->dragStartY) > 4.0f) self->dragMoved = true;
        self->scrollY -= dy;
        self->scrollVelocity = -dy * 40.0f;
        self->dragLastY = y;
        return true;
    }
    if (e.type == SDL_MOUSEBUTTONUP && self->dragging)
    {
        self->dragging = false;
        if (!self->dragMoved && grid.width > 0.0f && grid.height > 0.0f)
        {
            float localX = (float)e.button.x - grid.x;
            float localY = (float)e.button.y - grid.y;
            float unit = grid.width / 13.0f;
            int channel = (int)std::floor((localX - unit) / (unit * 2.0f));
            int row = (int)std::floor((localY + self->scrollY) / self->rowHeight);
            if (channel >= 0 && channel < TRACKER_CHANNELS && row >= 0 && row < self->rowCount)
                Tracker_OpenEditor(self, row, channel);
        }
        Tracker_SnapToGrid(self);
        return true;
    }

    if (overGrid) return true;
    return false;
}
