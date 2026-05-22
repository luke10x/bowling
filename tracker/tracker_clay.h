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

inline Clay_Color Tracker_LoopLineColor(const Tracker *self, int row, bool activeRow)
{
    bool inLoop = self && row >= self->loopStart && row <= self->loopEnd;
    if (!inLoop) return Tracker_CellColor(activeRow, true);
    if (row == self->loopStart || row == self->loopEnd)
        return activeRow ? (Clay_Color){118, 154, 80, 255} : (Clay_Color){82, 112, 56, 255};
    return activeRow ? (Clay_Color){76, 112, 78, 255} : (Clay_Color){44, 74, 52, 255};
}

inline Clay_Color Tracker_LoopCellColor(const Tracker *self, int row, bool activeRow)
{
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
    const char *noteNames[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    const char *specialNames[4] = {"OFF", "REL", "===", "..."};
    const char *effectNames[5] = {"None", "Pitch Slide", "Vibrato", "Tremolo", "Portamento"};
    const char *effectParamA[5] = {"", "Amount", "Speed", "Speed", "Time"};
    const char *effectParamB[5] = {"", "", "Depth", "Depth", "Target"};

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
                    arena, "CH%d  Line %03d", self->editChannel + 1, self->editRow
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
                CLAY(self->editorEffectsTabButton.clayId, tab)
                {
                    CLAY_TEXT(CLAY_STRING("EFFECTS"), CLAY_TEXT_CONFIG(buttonCfg));
                }
            }

            if (self->editorTab == 0)
            {
                CLAY(
                    CLAY_ID("TrackerNoteControls"),
                    {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                                .childGap = 8,
                                .layoutDirection = CLAY_TOP_TO_BOTTOM}}
                )
                {
                    CLAY(
                        CLAY_ID("TrackerInstrumentSelectorRow"),
                        {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(48)},
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
                            self->instrumentNameButton.clayId,
                            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}},
                             .backgroundColor = {35, 45, 65, 255},
                             .cornerRadius = {CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD},
                             .border = {.color = {78, 92, 124, 255}, .width = CLAY_BORDER_ALL(1)}}
                        )
                        {
                            CLAY_TEXT(name, CLAY_TEXT_CONFIG(buttonCfg));
                        }
                        CLAY(self->instrumentNextButton.clayId, CLAY_THEME_BTN_PRIMARY)
                        {
                            CLAY_TEXT(CLAY_STRING(">"), CLAY_TEXT_CONFIG(buttonCfg));
                        }
                    }

                    CLAY(
                        CLAY_ID("TrackerVolumeSlider"),
                        {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(44)},
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
                    }

                    CLAY(
                        CLAY_ID("TrackerSpecialValues"),
                        {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(42)},
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
                    }
                }

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
                        for (int note = 0; note < 12; note++)
                        {
                            bool black = note == 1 || note == 3 || note == 6 || note == 8 || note == 10;
                            bool selected = self->editSpecial == 0 && self->editOctave == octave && self->editNote == note;
                            Clay_Color bg = selected ? (Clay_Color){78, 170, 126, 255}
                                : black ? (Clay_Color){28, 30, 42, 255}
                                        : (Clay_Color){220, 224, 235, 255};
                            uint16_t keyBorderWidth = selected ? 2 : 1;
                            Clay_TextElementConfig keyText = bodyCfg;
                            keyText.textColor = black || selected ? (Clay_Color){245, 245, 250, 255}
                                                                  : (Clay_Color){20, 20, 30, 255};
                            CLAY(
                                CLAY_IDI("TrackerKey", octave * 100 + note),
                                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}},
                                 .backgroundColor = bg,
                                 .cornerRadius = {3, 3, 3, 3},
                                 .border = {.color = selected ? (Clay_Color){235, 245, 255, 255} : (Clay_Color){80, 80, 100, 255},
                                            .width = CLAY_BORDER_ALL(keyBorderWidth)}}
                            )
                            {
                                Clay_String label = ClayArena_FormatString(arena, "%s%d", noteNames[note], octave);
                                CLAY_TEXT(label, CLAY_TEXT_CONFIG(keyText));
                            }
                        }
                    }
                }
            }
            else
            {
                CLAY(CLAY_ID("TrackerEffectEditor"), CLAY_THEME_SECTION)
                {
                    CLAY(
                        CLAY_ID("TrackerEffectSelector"),
                        {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                                    .childGap = 8,
                                    .layoutDirection = CLAY_TOP_TO_BOTTOM}}
                    )
                    {
                        CLAY_TEXT(CLAY_STRING("Effect"), CLAY_TEXT_CONFIG(bodyCfg));
                        for (int i = 0; i < 5; i++)
                        {
                            Clay_ElementDeclaration effectBtn = CLAY_THEME_BTN_PRIMARY;
                            effectBtn.backgroundColor = self->editEffect == i ? CLAY_COLOR_BTN_ACTIVE : CLAY_COLOR_BTN_PRIMARY;
                            effectBtn.layout.sizing.height = CLAY_SIZING_FIXED(36);
                            CLAY(CLAY_IDI("TrackerEffect", i), effectBtn)
                            {
                                CLAY_TEXT(ClayArena_AllocString(arena, effectNames[i]), CLAY_TEXT_CONFIG(buttonCfg));
                            }
                        }
                    }
                    if (self->editEffect > 0)
                    {
                        Clay_String paramA = ClayArena_FormatString(arena, "%s %d", effectParamA[self->editEffect], self->editEffectParamA);
                        CLAY(
                            CLAY_ID("TrackerEffectParamATrack"),
                            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(44)},
                                        .childGap = 8,
                                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
                        )
                        {
                            CLAY_TEXT(paramA, CLAY_TEXT_CONFIG(bodyCfg));
                            CLAY(
                                CLAY_ID("TrackerEffectParamABar"),
                                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(18)},
                                            .layoutDirection = CLAY_LEFT_TO_RIGHT},
                                 .backgroundColor = {28, 30, 42, 255},
                                 .cornerRadius = {4, 4, 4, 4}}
                            )
                            {
                                CLAY(
                                    CLAY_ID("TrackerEffectParamAFill"),
                                    {.layout = {.sizing = {CLAY_SIZING_PERCENT((float)self->editEffectParamA / 127.0f), CLAY_SIZING_GROW()}},
                                     .backgroundColor = {120, 146, 214, 255},
                                     .cornerRadius = {4, 4, 4, 4}}
                                ) {}
                            }
                        }
                    }
                    if (self->editEffect == 2 || self->editEffect == 3 || self->editEffect == 4)
                    {
                        Clay_String paramB = ClayArena_FormatString(arena, "%s %d", effectParamB[self->editEffect], self->editEffectParamB);
                        CLAY(
                            CLAY_ID("TrackerEffectParamBTrack"),
                            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(44)},
                                        .childGap = 8,
                                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
                        )
                        {
                            CLAY_TEXT(paramB, CLAY_TEXT_CONFIG(bodyCfg));
                            CLAY(
                                CLAY_ID("TrackerEffectParamBBar"),
                                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(18)},
                                            .layoutDirection = CLAY_LEFT_TO_RIGHT},
                                 .backgroundColor = {28, 30, 42, 255},
                                 .cornerRadius = {4, 4, 4, 4}}
                            )
                            {
                                CLAY(
                                    CLAY_ID("TrackerEffectParamBFill"),
                                    {.layout = {.sizing = {CLAY_SIZING_PERCENT((float)self->editEffectParamB / 127.0f), CLAY_SIZING_GROW()}},
                                     .backgroundColor = {120, 146, 214, 255},
                                     .cornerRadius = {4, 4, 4, 4}}
                                ) {}
                            }
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
    ClayArena *arena = &clayton->clayArena;
    CLAY(CLAY_ID("TrackerInstrumentEditorWindow"), CLAY_THEME_WINDOW_PANEL)
    {
        CLAY(
            CLAY_ID("TrackerInstrumentEditorTitle"),
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                        .childGap = 8,
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
        )
        {
            Clay_String title = ClayArena_FormatString(arena, "Instrument %02X", self->editInstrument);
            CLAY_TEXT(title, CLAY_TEXT_CONFIG(titleCfg));
            CLAY(CLAY_ID("TrackerInstrumentEditorGrow"), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()}}}) {}
            CLAY(self->instrumentEditorCloseButton.clayId, CLAY_THEME_BTN_DANGER)
            {
                CLAY_TEXT(CLAY_STRING("x"), CLAY_TEXT_CONFIG(buttonCfg));
            }
        }
        CLAY_TEXT(CLAY_STRING("Instrument editor stub"), CLAY_TEXT_CONFIG(bodyCfg));
        CLAY_TEXT(CLAY_STRING("OPN params will live here: ALGO, FB, FMS, AMS, operators, macros."), CLAY_TEXT_CONFIG(bodyCfg));
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
    float trackerViewportHeight = self->viewportHeight > 1.0f ? self->viewportHeight : 360.0f;
    Clay_BoundingBox portraitBox = Clay_GetElementData(CLAY_ID("Portrait area")).boundingBox;
    if (portraitBox.height > 1.0f)
    {
        const float trackerChromeHeight = 6.0f + 6.0f + 42.0f + 6.0f + 28.0f + 6.0f + 82.0f;
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
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(42)},
                        .childGap = 8,
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
        )
        {
            Clay_String title = ClayArena_FormatString(
                arena,
                "OPN Tracker :: %s  R%03d.%d  LOOP %03d-%03d",
                Tracker_SongName(self->songIndex),
                self->playRow,
                self->playTick,
                self->loopStart,
                self->loopEnd
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
                    for (int row = 0; row < self->rowCount; row++)
                    {
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
                                 .backgroundColor = Tracker_LoopLineColor(self, row, activeRow),
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
                                     .backgroundColor = Tracker_LoopCellColor(self, row, activeRow),
                                     .border = {.color = {50, 56, 74, 255}, .width = CLAY_BORDER_ALL(1)}}
                                )
                                {
                                    Clay_String txt = ClayArena_AllocString(arena, self->cells[row][ch].text);
                                    CLAY_TEXT(txt, CLAY_TEXT_CONFIG(monoCfg));
                                }
                            }
                        }
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
    if (isClaytonClicked(&self->editorEffectsTabButton, e))
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
    if (isClaytonClicked(&self->instrumentNameButton, e))
    {
        self->instrumentEditorOpen = true;
        self->instrumentEditorWindowRequested = true;
        return true;
    }

    const bool pointerEvent =
        e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP || e.type == SDL_MOUSEMOTION ||
        e.type == SDL_MOUSEWHEEL || e.type == SDL_FINGERDOWN || e.type == SDL_FINGERUP ||
        e.type == SDL_FINGERMOTION;
    const bool mouseSliderEvent =
        (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) ||
        (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) ||
        (e.type == SDL_MOUSEMOTION && (e.motion.state & SDL_BUTTON_LMASK));
    if (mouseSliderEvent)
    {
        float pointerX = e.type == SDL_MOUSEMOTION ? (float)e.motion.x : (float)e.button.x;
        if (Clay_PointerOver(CLAY_ID("TrackerVolumeTrack")))
        {
            Clay_BoundingBox b = Clay_GetElementData(CLAY_ID("TrackerVolumeTrack")).boundingBox;
            float t = b.width > 0.0f ? (pointerX - b.x) / b.width : 0.0f;
            self->editVolume = std::max(0, std::min(127, (int)std::round(t * 127.0f)));
            return true;
        }
        if (Clay_PointerOver(CLAY_ID("TrackerEffectParamABar")))
        {
            Clay_BoundingBox b = Clay_GetElementData(CLAY_ID("TrackerEffectParamABar")).boundingBox;
            float t = b.width > 0.0f ? (pointerX - b.x) / b.width : 0.0f;
            self->editEffectParamA = std::max(0, std::min(127, (int)std::round(t * 127.0f)));
            return true;
        }
        if (Clay_PointerOver(CLAY_ID("TrackerEffectParamBBar")))
        {
            Clay_BoundingBox b = Clay_GetElementData(CLAY_ID("TrackerEffectParamBBar")).boundingBox;
            float t = b.width > 0.0f ? (pointerX - b.x) / b.width : 0.0f;
            self->editEffectParamB = std::max(0, std::min(127, (int)std::round(t * 127.0f)));
            return true;
        }
    }
    if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT)
    {
        for (int i = 0; i < 4; i++)
        {
            if (Clay_PointerOver(CLAY_IDI("TrackerSpecial", i)))
            {
                self->editSpecial = i + 1;
                return true;
            }
        }
        for (int octave = 1; octave <= 7; octave++)
        {
            for (int note = 0; note < 12; note++)
            {
                if (Clay_PointerOver(CLAY_IDI("TrackerKey", octave * 100 + note)))
                {
                    self->editOctave = octave;
                    self->editNote = note;
                    self->editSpecial = 0;
                    return true;
                }
            }
        }
        for (int i = 0; i < 5; i++)
        {
            if (Clay_PointerOver(CLAY_IDI("TrackerEffect", i)))
            {
                self->editEffect = i;
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
    if (isClaytonClicked(&self->instrumentEditorCloseButton, e))
    {
        self->instrumentEditorOpen = false;
        return true;
    }
    const bool pointerEvent =
        e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP || e.type == SDL_MOUSEMOTION ||
        e.type == SDL_MOUSEWHEEL || e.type == SDL_FINGERDOWN || e.type == SDL_FINGERUP ||
        e.type == SDL_FINGERMOTION;
    if (pointerEvent && Clay_PointerOver(CLAY_ID("TrackerInstrumentEditorWindow"))) return true;
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
        float localX = (float)e.button.x - grid.x;
        float localY = (float)e.button.y - grid.y;
        float unit = grid.width / 13.0f;
        if (localX >= 0.0f && localX < unit)
        {
            int row = Tracker_RowAtViewportY(self, localY);
            self->followCursor = false;
            self->loopSelecting = true;
            self->dragging = false;
            self->dragMoved = true;
            self->loopAnchor = row;
            Tracker_SetLoopRange(self, row, row);
            return true;
        }
        self->followCursor = false;
        self->dragging = true;
        self->dragMoved = false;
        self->dragStartY = (float)e.button.y;
        self->dragLastY = (float)e.button.y;
        self->dragStartScrollY = self->scrollY;
        self->scrollVelocity = 0.0f;
        return true;
    }
    if (e.type == SDL_MOUSEMOTION && self->loopSelecting)
    {
        float localY = (float)e.motion.y - grid.y;
        int row = Tracker_RowAtViewportY(self, localY);
        Tracker_SetLoopRange(self, self->loopAnchor, row);
        return true;
    }
    if (e.type == SDL_MOUSEBUTTONUP && self->loopSelecting)
    {
        float localY = (float)e.button.y - grid.y;
        int row = Tracker_RowAtViewportY(self, localY);
        Tracker_SetLoopRange(self, self->loopAnchor, row);
        self->loopSelecting = false;
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
