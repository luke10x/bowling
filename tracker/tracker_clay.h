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
                        bool instrumentUsed = Tracker_InstrumentUsedInSong(self, self->editInstrument);
                        Clay_String name = ClayArena_FormatString(
                            arena,
                            "%02X %s",
                            self->editInstrument,
                            Tracker_InstrumentName(self, self->editInstrument)
                        );
                        Clay_Color instrumentTextColor = instrumentUsed ? CLAY_COLOR_TEXT_PRIMARY : Clay_Color{145, 151, 164, 255};
                        CLAY(
                            self->instrumentNameButton.clayId,
                            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}},
                             .backgroundColor = instrumentUsed ? Clay_Color{35, 45, 65, 255} : Clay_Color{41, 43, 51, 255},
                             .cornerRadius = {CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD},
                             .border = {.color = instrumentUsed ? Clay_Color{78, 92, 124, 255} : Clay_Color{68, 70, 80, 255}, .width = CLAY_BORDER_ALL(1)}}
                        )
                        {
                            Clay_TextElementConfig mutedButtonCfg = buttonCfg;
                            mutedButtonCfg.textColor = instrumentTextColor;
                            CLAY_TEXT(name, CLAY_TEXT_CONFIG(mutedButtonCfg));
                        }
                        CLAY(self->instrumentNextButton.clayId, CLAY_THEME_BTN_PRIMARY)
                        {
                            CLAY_TEXT(CLAY_STRING(">"), CLAY_TEXT_CONFIG(buttonCfg));
                        }
                        bool canInheritInst = Tracker_CanInheritInstrument(self);
                        Clay_ElementDeclaration instCheck = CLAY_THEME_BTN_PRIMARY;
                        instCheck.layout.sizing.width = CLAY_SIZING_FIXED(42);
                        instCheck.backgroundColor = self->editInstrumentExplicit ? CLAY_COLOR_BTN_SUCCESS : CLAY_COLOR_BTN_DISABLED;
                        if (!canInheritInst) instCheck.backgroundColor = {74, 74, 88, 255};
                        CLAY(self->instrumentExplicitButton.clayId, instCheck)
                        {
                            CLAY_TEXT(self->editInstrumentExplicit ? CLAY_STRING("x") : CLAY_STRING(" "), CLAY_TEXT_CONFIG(buttonCfg));
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
                        bool canInheritVol = Tracker_CanInheritVolume(self);
                        Clay_ElementDeclaration volCheck = CLAY_THEME_BTN_PRIMARY;
                        volCheck.layout.sizing.width = CLAY_SIZING_FIXED(42);
                        volCheck.backgroundColor = self->editVolumeExplicit ? CLAY_COLOR_BTN_SUCCESS : CLAY_COLOR_BTN_DISABLED;
                        if (!canInheritVol) volCheck.backgroundColor = {74, 74, 88, 255};
                        CLAY(self->volumeExplicitButton.clayId, volCheck)
                        {
                            CLAY_TEXT(self->editVolumeExplicit ? CLAY_STRING("x") : CLAY_STRING(" "), CLAY_TEXT_CONFIG(buttonCfg));
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

                Clay_ElementData ed = Clay_GetElementData(
                    CLAY_IDI("TrackerOctaveWrapper", 1) // All octaves are same
                );

                if (ed.found) 
                {
                    // self->keyHeight = ed.
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
                                             {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(upperKeyHeight)},
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
            }
            else
            {
                CLAY(CLAY_ID("TrackerEffectEditor"), CLAY_THEME_SECTION)
                {
                    int slot = std::max(0, std::min(TRACKER_MAX_EFFECT_SLOTS - 1, self->editEffectSlot));
                    uint8_t code = self->editEffectCodes[slot];
                    uint8_t value = self->editEffectValues[slot];
                    const TrackerEffectDef *def = Tracker_EffectDefByCode(code);

                    CLAY(
                        CLAY_ID("TrackerEffectSlotRow"),
                        {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(46)},
                                    .childGap = 8,
                                    .layoutDirection = CLAY_LEFT_TO_RIGHT}}
                    )
                    {
                        CLAY(self->effectSlotPrevButton.clayId, CLAY_THEME_BTN_PRIMARY)
                        {
                            CLAY_TEXT(CLAY_STRING("<"), CLAY_TEXT_CONFIG(buttonCfg));
                        }
                        CLAY(
                            CLAY_ID("TrackerEffectSlotValue"),
                            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}},
                             .backgroundColor = {35, 45, 65, 255},
                             .cornerRadius = {CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD}}
                        )
                        {
                            Clay_String label = ClayArena_FormatString(arena, "Slot %d", slot);
                            CLAY_TEXT(label, CLAY_TEXT_CONFIG(buttonCfg));
                        }
                        CLAY(self->effectSlotNextButton.clayId, CLAY_THEME_BTN_PRIMARY)
                        {
                            CLAY_TEXT(CLAY_STRING(">"), CLAY_TEXT_CONFIG(buttonCfg));
                        }
                    }

                    CLAY(
                        CLAY_ID("TrackerEffectTypeRow"),
                        {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(52)},
                                    .childGap = 8,
                                    .layoutDirection = CLAY_LEFT_TO_RIGHT}}
                    )
                    {
                        CLAY(self->effectPrevButton.clayId, CLAY_THEME_BTN_PRIMARY)
                        {
                            CLAY_TEXT(CLAY_STRING("<"), CLAY_TEXT_CONFIG(buttonCfg));
                        }
                        CLAY(
                            CLAY_ID("TrackerEffectTypeValue"),
                            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}},
                             .backgroundColor = {35, 45, 65, 255},
                             .cornerRadius = {CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD}}
                        )
                        {
                            Clay_String label = def->code == 0
                                ? CLAY_STRING("None")
                                : ClayArena_FormatString(arena, "%02X %s", def->code, def->name);
                            CLAY_TEXT(label, CLAY_TEXT_CONFIG(buttonCfg));
                        }
                        CLAY(self->effectNextButton.clayId, CLAY_THEME_BTN_PRIMARY)
                        {
                            CLAY_TEXT(CLAY_STRING(">"), CLAY_TEXT_CONFIG(buttonCfg));
                        }
                    }

                    auto paramSlider = [&](const char *label, int paramValue, int minValue, int maxValue, int hardMax, bool inRange, Clay_ElementId barId, Clay_ElementId fillId) {
                        float t = hardMax > 0 ? (float)paramValue / (float)hardMax : 0.0f;
                        t = std::max(0.0f, std::min(1.0f, t));
                        Clay_Color fillColor = inRange ? (Clay_Color){120, 146, 214, 255} : (Clay_Color){226, 72, 88, 255};
                        Clay_String param = ClayArena_FormatString(arena, "%s %02X", label, paramValue);
                        CLAY(
                            CLAY_IDI("TrackerEffectParamTrack", barId.id),
                            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(46)},
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

inline void Tracker_BuildInstrumentEditor(Tracker *self, Clayton *clayton)
{
    if (!self || !self->instrumentEditorOpen || !clayton) return;
    Clay_TextElementConfig titleCfg = CLAY_THEME_TEXT_TITLE;
    Clay_TextElementConfig buttonCfg = CLAY_THEME_TEXT_BUTTON;
    Clay_TextElementConfig bodyCfg = CLAY_THEME_TEXT_BODY;
    ClayArena *arena = &clayton->clayArena;
    xfm_patch_opn &patch = Tracker_EditablePatch(self);
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
            colorBtn.layout.sizing.width = CLAY_SIZING_FIXED(92);
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
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(42)},
                        .childGap = 8,
                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
        )
        {
            Clay_ElementDeclaration patchTab = CLAY_THEME_BTN_PRIMARY;
            Clay_ElementDeclaration effectsTab = CLAY_THEME_BTN_PRIMARY;
            if (self->instrumentEditorTab == 0) patchTab.backgroundColor = CLAY_COLOR_BTN_SUCCESS;
            if (self->instrumentEditorTab == 1) effectsTab.backgroundColor = CLAY_COLOR_BTN_SUCCESS;
            CLAY(self->instrumentPatchTabButton.clayId, patchTab)
            {
                CLAY_TEXT(CLAY_STRING("Patch"), CLAY_TEXT_CONFIG(buttonCfg));
            }
            CLAY(self->instrumentEffectsTabButton.clayId, effectsTab)
            {
                CLAY_TEXT(CLAY_STRING("Macros"), CLAY_TEXT_CONFIG(buttonCfg));
            }
        }
        if (self->instrumentEditorTab == 0)
        {
        CLAY(
            CLAY_ID("TrackerInstrumentAlgoRow"),
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(46)},
                        .childGap = 8,
                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
        )
        {
            CLAY(self->instrumentAlgoPrevButton.clayId, CLAY_THEME_BTN_PRIMARY)
            {
                CLAY_TEXT(CLAY_STRING("<"), CLAY_TEXT_CONFIG(buttonCfg));
            }
            Clay_String algo = ClayArena_FormatString(arena, "ALGO %d", patch.ALG);
            CLAY(
                CLAY_ID("TrackerInstrumentAlgoValue"),
                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}},
                 .backgroundColor = {35, 45, 65, 255},
                 .cornerRadius = {CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD}}
            )
            {
                CLAY_TEXT(algo, CLAY_TEXT_CONFIG(buttonCfg));
            }
            CLAY(self->instrumentAlgoNextButton.clayId, CLAY_THEME_BTN_PRIMARY)
            {
                CLAY_TEXT(CLAY_STRING(">"), CLAY_TEXT_CONFIG(buttonCfg));
            }
            Clay_ElementDeclaration algoPreview = {
                .layout = {.sizing = {CLAY_SIZING_FIXED(92), CLAY_SIZING_GROW()}},
                .image = {.imageData = &clayton->trackerAlgoImages[patch.ALG & 7]},
                .border = {.color = {146, 220, 132, 255}, .width = CLAY_BORDER_ALL(1)}
            };
            CLAY(CLAY_ID("TrackerSelectedAlgoDiagram"), algoPreview) {}
        }

        auto renderSmallPreview = [&](Clay_ElementId id, Gles3_ImageConfig *image, bool enabled) {
            Clay_ElementDeclaration preview = {
                .layout = {.sizing = {CLAY_SIZING_FIXED(58), CLAY_SIZING_GROW()}},
                .backgroundColor = {11, 14, 20, 255},
                .border = {.color = enabled ? (Clay_Color){88, 116, 92, 255} : (Clay_Color){54, 60, 78, 255},
                           .width = CLAY_BORDER_ALL(1)}
            };
            if (enabled) preview.image.imageData = image;
            CLAY(id, preview) {}
        };

        auto renderOperatorButton = [&](int opId) {
            CLAY(
                CLAY_IDI("TrackerOperatorButtonCell", opId),
                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                            .childGap = 4,
                            .layoutDirection = CLAY_LEFT_TO_RIGHT}}
            )
            {
                CLAY(self->operatorButtons[opId].clayId, CLAY_THEME_BTN_PRIMARY)
                {
                    Clay_String text = ClayArena_FormatString(arena, "OP%d", opId + 1);
                    CLAY_TEXT(text, CLAY_TEXT_CONFIG(buttonCfg));
                }
                renderSmallPreview(CLAY_IDI("TrackerOperatorEnvelopePreview", opId), &clayton->trackerEnvelopeImages[opId], true);
                int ssg = patch.op[opId].SSG;
                renderSmallPreview(CLAY_IDI("TrackerOperatorSsgPreview", opId), ssg > 0 ? &clayton->trackerSsgImages[std::max(0, std::min(7, ssg - 1))] : nullptr, ssg > 0);
            }
        };

        auto slider = [&](const char *label, int value, int maxValue, Clay_ElementId barId, Clay_ElementId fillId) {
            CLAY(
                CLAY_IDI("TrackerInstrumentSliderRow", barId.id),
                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(42)},
                            .childGap = 8,
                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                            .layoutDirection = CLAY_LEFT_TO_RIGHT}}
            )
            {
                Clay_String text = ClayArena_FormatString(arena, "%s %d", label, value);
                CLAY(
                    CLAY_IDI("TrackerInstrumentSliderLabel", barId.id),
                    {.layout = {.sizing = {CLAY_SIZING_FIXED(70), CLAY_SIZING_GROW()},
                                .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER}}}
                )
                {
                    CLAY_TEXT(text, CLAY_TEXT_CONFIG(bodyCfg));
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
                        {.layout = {.sizing = {CLAY_SIZING_PERCENT(maxValue > 0 ? (float)value / (float)maxValue : 0.0f), CLAY_SIZING_GROW()}},
                         .backgroundColor = {120, 146, 214, 255},
                         .cornerRadius = {4, 4, 4, 4}}
                    ) {}
                }
            }
        };
        slider("FB", patch.FB, 7, CLAY_ID("TrackerPatchFbBar"), CLAY_ID("TrackerPatchFbFill"));
        slider("AMS", patch.AMS, 3, CLAY_ID("TrackerPatchAmsBar"), CLAY_ID("TrackerPatchAmsFill"));
        slider("FMS", patch.FMS, 7, CLAY_ID("TrackerPatchFmsBar"), CLAY_ID("TrackerPatchFmsFill"));

        CLAY(
            CLAY_ID("TrackerOperatorButtonGrid"),
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                        .childGap = 8,
                        .layoutDirection = CLAY_TOP_TO_BOTTOM}}
        )
        {
            for (int row = 0; row < 2; row++)
            {
                CLAY(
                    CLAY_IDI("TrackerOperatorButtonRow", row),
                    {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(48)},
                                .childGap = 8,
                                .layoutDirection = CLAY_LEFT_TO_RIGHT}}
                )
                {
                    for (int col = 0; col < 2; col++)
                    {
                        int op = row * 2 + col;
                        renderOperatorButton(op);
                    }
                }
            }
        }
        }
        else
        {
            int inst = std::max(0, std::min(255, self->editInstrument));
            XfmMacro &macro = Tracker_EditableMacro(self);
            Tracker_EnsureMacroUiLength(&macro);
            int target = std::max((int)XFM_MACRO_TL1, std::min((int)XFM_MACRO_ARP, self->editMacroTarget));
            bool enabled = self->editMacroEnabled[inst][target];
            int enabledCount = Tracker_MacroEnabledCount(self);

            CLAY(
                CLAY_ID("TrackerMacroTargetRow"),
                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(44)},
                            .childGap = 8,
                            .layoutDirection = CLAY_LEFT_TO_RIGHT}}
            )
            {
                CLAY(self->macroTargetPrevButton.clayId, CLAY_THEME_BTN_PRIMARY)
                {
                    CLAY_TEXT(CLAY_STRING("<"), CLAY_TEXT_CONFIG(buttonCfg));
                }
                CLAY(
                    CLAY_ID("TrackerMacroTargetValue"),
                    {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}},
                     .backgroundColor = {35, 45, 65, 255},
                     .cornerRadius = {CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD}}
                )
                {
                    Clay_String label = ClayArena_FormatString(
                        arena,
                        "%s macro  %d on",
                        Tracker_MacroTargetName(target),
                        enabledCount
                    );
                    CLAY_TEXT(label, CLAY_TEXT_CONFIG(buttonCfg));
                }
                CLAY(self->macroTargetNextButton.clayId, CLAY_THEME_BTN_PRIMARY)
                {
                    CLAY_TEXT(CLAY_STRING(">"), CLAY_TEXT_CONFIG(buttonCfg));
                }
                Clay_ElementDeclaration enableCheck = CLAY_THEME_BTN_PRIMARY;
                enableCheck.layout.sizing.width = CLAY_SIZING_FIXED(42);
                enableCheck.backgroundColor = enabled ? CLAY_COLOR_BTN_SUCCESS : CLAY_COLOR_BTN_DISABLED;
                CLAY(self->macroEnableButton.clayId, enableCheck)
                {
                    CLAY_TEXT(enabled ? CLAY_STRING("x") : CLAY_STRING(" "), CLAY_TEXT_CONFIG(buttonCfg));
                }
            }

            int valueMin = -64;
            int valueMax = 127;
            if (target >= XFM_MACRO_TL1 && target <= XFM_MACRO_TL4) valueMin = 0, valueMax = 127;
            else if (target >= XFM_MACRO_MUL1 && target <= XFM_MACRO_MUL4) valueMin = 0, valueMax = 15;
            else if (target >= XFM_MACRO_DT1 && target <= XFM_MACRO_DT4) valueMin = -3, valueMax = 3;
            else if (target == XFM_MACRO_FB) valueMin = 0, valueMax = 7;
            else if (target == XFM_MACRO_ARP) valueMin = -12, valueMax = 12;
            bool signedMacro = valueMin < 0 && valueMax > 0;
            float zeroT = signedMacro ? (float)valueMax / (float)(valueMax - valueMin) : 1.0f;
            zeroT = std::max(0.0f, std::min(1.0f, zeroT));
            Clay_Color graphBg = enabled ? (Clay_Color){18, 20, 30, 255} : (Clay_Color){42, 42, 46, 255};
            Clay_Color posColor = enabled ? (Clay_Color){96, 170, 236, 255} : (Clay_Color){92, 92, 96, 255};
            Clay_Color negColor = enabled ? (Clay_Color){232, 114, 118, 255} : (Clay_Color){82, 82, 86, 255};
            Clay_TextElementConfig tinyCfg = bodyCfg;
            tinyCfg.fontSize = 8;
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
                    CLAY(
                        CLAY_SID(ClayArena_AllocString(arena, beltId)),
                        {.layout = {.sizing = {CLAY_SIZING_PERCENT((float)TRACKER_MACRO_UI_STEPS / (float)TRACKER_MACRO_VISIBLE_STEPS), CLAY_SIZING_GROW()},
                                    .padding = graph ? (Clay_Padding){3, 3, 3, 3} : (Clay_Padding){0, 0, 0, 0},
                                    .childGap = 1,
                                    .layoutDirection = CLAY_LEFT_TO_RIGHT}}
                    )
                    {
                        for (int i = 0; i < TRACKER_MACRO_UI_STEPS; i++)
                        {
                            if (reset)
                            {
                                CLAY(
                                    CLAY_IDI("TrackerMacroReset", i),
                                    {.layout = {.sizing = {CLAY_SIZING_PERCENT(1.0f / (float)TRACKER_MACRO_UI_STEPS), CLAY_SIZING_GROW()},
                                                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}},
                                     .backgroundColor = enabled ? (Clay_Color){36, 40, 52, 255} : (Clay_Color){48, 48, 52, 255},
                                     .cornerRadius = {2, 2, 2, 2}}
                                )
                                {
                                    CLAY_TEXT(CLAY_STRING("x"), CLAY_TEXT_CONFIG(tinyCfg));
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
                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(168)},
                            .childGap = 4,
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
                CLAY(
                    CLAY_ID("TrackerMacroViewportStack"),
                    {.layout = {.sizing = {CLAY_SIZING_PERCENT(0.8f), CLAY_SIZING_GROW()},
                                .layoutDirection = CLAY_TOP_TO_BOTTOM}}
                )
                {
                    renderMacroBelt("TrackerMacroResetClip", "TrackerMacroResetBelt", 18, false, true, false);
                    renderMacroBelt("TrackerMacroGraphClip", "TrackerMacroGraphBelt", 122, true, false, false);
                    renderMacroBelt("TrackerMacroNumbersClip", "TrackerMacroNumbersBelt", 24, false, false, true);
                }
                CLAY(self->macroScrollNextButton.clayId, macroScrollNext)
                {
                    CLAY_TEXT(CLAY_STRING(">"), CLAY_TEXT_CONFIG(buttonCfg));
                }
            }

            CLAY(
                CLAY_ID("TrackerMacroFlagsRow"),
                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(42)},
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
                                CLAY_TEXT(CLAY_STRING("x"), CLAY_TEXT_CONFIG(selectedCfg));
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
    ClayArena *arena = &clayton->clayArena;
    xfm_patch_opn &patch = Tracker_EditablePatch(self);
    int opIndex = std::max(0, std::min(3, self->editOperator));
    xfm_patch_opn_operator &op = patch.op[opIndex];

    CLAY(CLAY_ID("TrackerOperatorEditorWindow"), CLAY_THEME_WINDOW_PANEL)
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
            CLAY_ID("TrackerOperatorEnvelopeParamRow"),
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(178)},
                        .childGap = 8,
                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
        )
        {
            CLAY(
                CLAY_ID("TrackerOperatorEnvelopeSliders"),
                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                            .layoutDirection = CLAY_TOP_TO_BOTTOM}}
            )
            {
                slider("AR", op.AR, 0, 31, CLAY_ID("TrackerOpArBar"), CLAY_ID("TrackerOpArFill"));
                slider("DR", op.DR, 0, 31, CLAY_ID("TrackerOpDrBar"), CLAY_ID("TrackerOpDrFill"));
                slider("SL", op.SL, 0, 15, CLAY_ID("TrackerOpSlBar"), CLAY_ID("TrackerOpSlFill"));
                slider("SR", op.SR, 0, 31, CLAY_ID("TrackerOpSrBar"), CLAY_ID("TrackerOpSrFill"));
                slider("RR", op.RR, 0, 15, CLAY_ID("TrackerOpRrBar"), CLAY_ID("TrackerOpRrFill"));
            }
            Clay_ElementDeclaration envelopePreview = {
                .layout = {.sizing = {CLAY_SIZING_FIXED(164), CLAY_SIZING_GROW()}},
                .image = {.imageData = &clayton->trackerEnvelopeImages[opIndex]},
                .border = {.color = {146, 220, 132, 255}, .width = CLAY_BORDER_ALL(1)}
            };
            CLAY(CLAY_ID("TrackerCurrentOperatorEnvelope"), envelopePreview) {}
        }

        slider("MUL", op.MUL, 0, 15, CLAY_ID("TrackerOpMulBar"), CLAY_ID("TrackerOpMulFill"));
        slider("DT", op.DT, -3, 3, CLAY_ID("TrackerOpDtBar"), CLAY_ID("TrackerOpDtFill"));
        slider("RS", op.RS, 0, 3, CLAY_ID("TrackerOpRsBar"), CLAY_ID("TrackerOpRsFill"));

        CLAY(
            CLAY_ID("TrackerOperatorToggleRow"),
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(44)},
                        .childGap = 8,
                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
        )
        {
            CLAY(self->operatorSsgPrevButton.clayId, CLAY_THEME_BTN_PRIMARY)
            {
                CLAY_TEXT(CLAY_STRING("<"), CLAY_TEXT_CONFIG(buttonCfg));
            }
            CLAY(
                CLAY_ID("TrackerOperatorSsgValue"),
                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}},
                 .backgroundColor = {35, 45, 65, 255},
                 .cornerRadius = {CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD}}
            )
            {
                Clay_String text = op.SSG == 0 ? CLAY_STRING("SSG-EG none") : ClayArena_FormatString(arena, "SSG-EG %d", op.SSG);
                CLAY_TEXT(text, CLAY_TEXT_CONFIG(buttonCfg));
            }
            CLAY(self->operatorSsgNextButton.clayId, CLAY_THEME_BTN_PRIMARY)
            {
                CLAY_TEXT(CLAY_STRING(">"), CLAY_TEXT_CONFIG(buttonCfg));
            }
            Clay_ElementDeclaration ssgPreview = {
                .layout = {.sizing = {CLAY_SIZING_FIXED(76), CLAY_SIZING_GROW()}},
                .backgroundColor = {11, 14, 20, 255},
                .border = {.color = op.SSG > 0 ? (Clay_Color){146, 220, 132, 255} : (Clay_Color){54, 60, 78, 255},
                           .width = CLAY_BORDER_ALL(1)}
            };
            if (op.SSG > 0) ssgPreview.image.imageData = &clayton->trackerSsgImages[std::max(0, std::min(7, (int)op.SSG - 1))];
            CLAY(CLAY_ID("TrackerCurrentOperatorSsg"), ssgPreview) {}
            Clay_ElementDeclaration amBtn = CLAY_THEME_BTN_PRIMARY;
            if (op.AM) amBtn.backgroundColor = CLAY_COLOR_BTN_SUCCESS;
            CLAY(self->operatorAmButton.clayId, amBtn)
            {
                CLAY_TEXT(CLAY_STRING("AM"), CLAY_TEXT_CONFIG(buttonCfg));
            }
        }
    }
}

inline void Tracker_BuildInstrumentsWindow(Tracker *self, Clayton *clayton)
{
    if (!self || !self->instrumentsWindowOpen || !clayton) return;
    if (self->availableInstrumentCount <= 0)
        Tracker_LoadBuiltinInstrumentCatalog(self);

    Clay_TextElementConfig titleCfg = CLAY_THEME_TEXT_TITLE;
    Clay_TextElementConfig buttonCfg = CLAY_THEME_TEXT_BUTTON;
    Clay_TextElementConfig bodyCfg = CLAY_THEME_TEXT_BODY;
    Clay_TextElementConfig mutedCfg = bodyCfg;
    mutedCfg.textColor = {150, 154, 170, 255};
    ClayArena *arena = &clayton->clayArena;

    const float rowH = 54.0f;
    const float headerH = 58.0f;
    const float footerH = 12.0f;
    Clay_BoundingBox stackBox = Clay_GetElementData(CLAY_ID("WindowStackViewport")).boundingBox;
    const float windowH = stackBox.height > 0.0f ? stackBox.height * 0.88f : 620.0f;
    const float viewportH = std::max(120.0f, windowH - headerH - footerH - 36.0f);
    int availableCount = std::max(1, self->availableInstrumentCount);
    float maxScroll = std::max(0.0f, availableCount * rowH - viewportH);
    self->instrumentsScrollY = std::max(0.0f, std::min(maxScroll, self->instrumentsScrollY));
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
            CLAY_ID("TrackerInstrumentsViewport"),
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(viewportH)},
                        .layoutDirection = CLAY_TOP_TO_BOTTOM},
             .backgroundColor = {18, 20, 30, 255},
             .cornerRadius = {6, 6, 6, 6},
             .clip = {.vertical = true}}
        )
        {
            int startIndex = rowH > 0.0f ? (int)std::floor(self->instrumentsScrollY / rowH) : 0;
            int visibleRows = (int)std::ceil(viewportH / rowH) + 1;
            int ordinal = 0;
            int rendered = 0;
            for (int inst = 0; inst < 256; inst++)
            {
                if (!self->availableInstruments[inst])
                    continue;
                if (ordinal++ < startIndex)
                    continue;
                if (rendered++ >= visibleRows)
                {
                    break;
                }

                bool builtin = Tracker_IsBuiltinInstrument(self, inst);
                bool used = Tracker_InstrumentUsedInSong(self, inst);
                Clay_Color rowBg = builtin ? (Clay_Color){32, 34, 48, 255} : (Clay_Color){28, 42, 42, 255};
                if (!used) rowBg = {34, 34, 40, 255};
                Clay_TextElementConfig rowText = used ? bodyCfg : mutedCfg;
                Clay_ElementDeclaration disabled = CLAY_THEME_BTN_PRIMARY;
                disabled.backgroundColor = CLAY_COLOR_BTN_DISABLED;
                Clay_ElementDeclaration smallBtn = CLAY_THEME_BTN_PRIMARY;
                smallBtn.layout.sizing.width = CLAY_SIZING_FIXED(48);
                Clay_ElementDeclaration disabledSmall = disabled;
                disabledSmall.layout.sizing.width = CLAY_SIZING_FIXED(48);

                CLAY(
                    CLAY_IDI("TrackerInstrumentRow", inst),
                    {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(rowH - 4.0f)},
                                .padding = {6, 6, 4, 4},
                                .childGap = 4,
                                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                .layoutDirection = CLAY_LEFT_TO_RIGHT},
                     .backgroundColor = rowBg,
                     .cornerRadius = {4, 4, 4, 4}}
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
                    CLAY(self->instrumentCloneButtons[inst].clayId, smallBtn)
                    {
                        CLAY_TEXT(CLAY_STRING("CL"), CLAY_TEXT_CONFIG(buttonCfg));
                    }
                    if (!builtin)
                    {
                        CLAY(self->instrumentRenameButtons[inst].clayId, smallBtn)
                        {
                            CLAY_TEXT(CLAY_STRING("NM"), CLAY_TEXT_CONFIG(buttonCfg));
                        }
                        CLAY(self->instrumentDeleteButtons[inst].clayId, smallBtn)
                        {
                            CLAY_TEXT(CLAY_STRING("DEL"), CLAY_TEXT_CONFIG(buttonCfg));
                        }
                    }
                }
            }
        }
    }
}

inline void Tracker_BuildSongSettingsWindow(Tracker *self, Clayton *clayton)
{
    if (!self || !self->songSettingsWindowOpen || !clayton) return;

    Clay_TextElementConfig titleCfg = CLAY_THEME_TEXT_TITLE;
    Clay_TextElementConfig buttonCfg = CLAY_THEME_TEXT_BUTTON;
    Clay_TextElementConfig bodyCfg = CLAY_THEME_TEXT_BODY;
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

        Clay_ElementDeclaration lfoBtn = CLAY_THEME_BTN_PRIMARY;
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
                CLAY_TEXT(self->songLfoEnabled ? CLAY_STRING("ON") : CLAY_STRING("OFF"), CLAY_TEXT_CONFIG(buttonCfg));
            }
        }

        slider("LFO Freq", self->songLfoFrequency, 0, 7, CLAY_ID("TrackerSongLfoFreqBar"), CLAY_ID("TrackerSongLfoFreqFill"));
        slider("Tick Rate", self->songTickRate, 30, 240, CLAY_ID("TrackerSongTickRateBar"), CLAY_ID("TrackerSongTickRateFill"));
        slider("Speed", self->songSpeed, 1, 16, CLAY_ID("TrackerSongSpeedBar"), CLAY_ID("TrackerSongSpeedFill"));
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
    monoCfg.fontSize = 11;
    Clay_TextElementConfig effectMonoCfg = monoCfg;
    effectMonoCfg.fontSize = 9;
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
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(42)},
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
                clearCfg.fontSize = 12;
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
                                bool selectedColumn = !self->channelSelectionEnabled || (ch >= self->channelStart && ch <= self->channelEnd);
                                Clay_Color cellBg = Tracker_LoopCellColor(self, row, activeRow);
                                if (self->loopEnabled && !selectedColumn)
                                    cellBg = Tracker_CellColor(activeRow, false);
                                const char *cell = self->cells[row][ch].text;
                                int explicitInst = Tracker_ParseCellInstrument(cell);
                                int inheritedInst = explicitInst < 0 && Tracker_CellHasNoteLikeValue(cell) ?
                                    Tracker_FindInheritedInstrument(self, row, ch) : -1;
                                uint32_t explicitColor = explicitInst >= 0 ? Tracker_InstrumentColorU32(self, explicitInst) : 0;
                                uint32_t borderColorRgb = inheritedInst >= 0 ? Tracker_InstrumentColorU32(self, inheritedInst) : 0;
                                Clay_Color cellBorder = borderColorRgb != 0 ?
                                    Tracker_ColorFromU32(borderColorRgb, 255.0f) : (Clay_Color){50, 56, 74, 255};
                                Clay_BorderElementConfig cellBorderConfig = borderColorRgb != 0 ?
                                    (Clay_BorderElementConfig){.color = cellBorder, .width = CLAY_BORDER_ALL(2)} :
                                    (Clay_BorderElementConfig){.color = cellBorder, .width = CLAY_BORDER_ALL(1)};
                                bool brightCellBg = false;
                                if (explicitColor != 0)
                                {
                                    cellBg = Tracker_ColorFromU32(explicitColor, activeRow ? 245.0f : 225.0f);
                                    brightCellBg = Tracker_ColorIsBright(explicitColor);
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
            }

            float thumbHeight = Tracker_ScrollbarThumbHeight(self);
            float thumbTop = Tracker_ScrollbarThumbTop(self, thumbHeight);
            float thumbBottom = std::max(0.0f, self->viewportHeight - thumbTop - thumbHeight);
            float rowCountForMap = std::max(1.0f, (float)self->rowCount);
            float scrollbarRangeTop = self->loopEnabled ? self->viewportHeight * ((float)std::max(0, self->loopStart) / rowCountForMap) : 0.0f;
            float scrollbarRangeBottom = self->loopEnabled ?
                self->viewportHeight * ((float)std::min(self->rowCount, self->loopEnd + 1) / rowCountForMap) : 0.0f;
            float scrollbarRangeHeight = self->loopEnabled ? std::max(3.0f, scrollbarRangeBottom - scrollbarRangeTop) : 0.0f;
            float scrollbarPlayheadTop =
                self->viewportHeight * ((float)std::max(0, std::min(self->rowCount - 1, self->playRow)) / rowCountForMap);
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
                CLAY(self->addRowButton.clayId, CLAY_THEME_BTN_PRIMARY)
                {
                    CLAY_TEXT(CLAY_STRING("+ROW"), CLAY_TEXT_CONFIG(buttonCfg));
                }
                CLAY(self->removeRowButton.clayId, CLAY_THEME_BTN_PRIMARY)
                {
                    CLAY_TEXT(CLAY_STRING("-ROW"), CLAY_TEXT_CONFIG(buttonCfg));
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
                Clay_ElementDeclaration copyBtn = CLAY_THEME_BTN_PRIMARY;
                Clay_ElementDeclaration pasteBtn = CLAY_THEME_BTN_PRIMARY;
                if (!hasSelection) copyBtn.backgroundColor = CLAY_COLOR_BTN_DISABLED;
                if (!Tracker_CanPaste(self)) pasteBtn.backgroundColor = CLAY_COLOR_BTN_DISABLED;
                CLAY(self->copyButton.clayId, copyBtn)
                {
                    CLAY_TEXT(CLAY_STRING("COPY"), CLAY_TEXT_CONFIG(buttonCfg));
                }
                CLAY(self->pasteButton.clayId, pasteBtn)
                {
                    CLAY_TEXT(CLAY_STRING("PASTE"), CLAY_TEXT_CONFIG(buttonCfg));
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
    if (isClaytonClicked(&self->effectSlotPrevButton, e))
    {
        self->editEffectSlot = (self->editEffectSlot + TRACKER_MAX_EFFECT_SLOTS - 1) % TRACKER_MAX_EFFECT_SLOTS;
        return true;
    }
    if (isClaytonClicked(&self->effectSlotNextButton, e))
    {
        self->editEffectSlot = (self->editEffectSlot + 1) % TRACKER_MAX_EFFECT_SLOTS;
        return true;
    }
    bool effectPrevClicked = isClaytonClicked(&self->effectPrevButton, e);
    bool effectNextClicked = isClaytonClicked(&self->effectNextButton, e);
    if (effectPrevClicked || effectNextClicked)
    {
        int slot = std::max(0, std::min(TRACKER_MAX_EFFECT_SLOTS - 1, self->editEffectSlot));
        int idx = Tracker_EffectDefIndexByCode(self->editEffectCodes[slot]);
        int dir = effectPrevClicked ? -1 : 1;
        idx = (idx + dir + TRACKER_EFFECT_DEF_COUNT) % TRACKER_EFFECT_DEF_COUNT;
        self->editEffectCodes[slot] = TRACKER_EFFECT_DEFS[idx].code;
        Tracker_ApplyEditorToCell(self);
        return true;
    }
    if (isClaytonClicked(&self->instrumentPrevButton, e))
    {
        self->editInstrument = Tracker_NextAvailableInstrument(self, self->editInstrument, -1);
        Tracker_NormalizeExplicitFields(self);
        Tracker_ApplyEditorToCell(self);
        return true;
    }
    if (isClaytonClicked(&self->instrumentNextButton, e))
    {
        self->editInstrument = Tracker_NextAvailableInstrument(self, self->editInstrument, 1);
        Tracker_NormalizeExplicitFields(self);
        Tracker_ApplyEditorToCell(self);
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
        if (Tracker_CanInheritInstrument(self))
        {
            self->editInstrumentExplicit = !self->editInstrumentExplicit;
            Tracker_ApplyEditorToCell(self);
        }
        return true;
    }
    if (isClaytonClicked(&self->volumeExplicitButton, e))
    {
        if (Tracker_CanInheritVolume(self))
        {
            self->editVolumeExplicit = !self->editVolumeExplicit;
            Tracker_ApplyEditorToCell(self);
        }
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
            Tracker_ClearSliderCaptureOnUp(self, e);
            return true;
        }
        if (Tracker_CapturedSlider(self, CLAY_ID("TrackerEffectParamABar"), e))
        {
            int slot = std::max(0, std::min(TRACKER_MAX_EFFECT_SLOTS - 1, self->editEffectSlot));
            const TrackerEffectDef *def = Tracker_EffectDefByCode(self->editEffectCodes[slot]);
            if (def->paramCount <= 0)
            {
                Tracker_ClearSliderCaptureOnUp(self, e);
                return true;
            }
            int value = Tracker_ValueFromSliderX(CLAY_ID("TrackerEffectParamABar"), pointerX, def->minA, def->maxA);
            self->editEffectValues[slot] = Tracker_EffectSetA(def, self->editEffectValues[slot], value);
            Tracker_ApplyEditorToCell(self);
            Tracker_ClearSliderCaptureOnUp(self, e);
            return true;
        }
        if (Tracker_CapturedSlider(self, CLAY_ID("TrackerEffectParamBBar"), e))
        {
            int slot = std::max(0, std::min(TRACKER_MAX_EFFECT_SLOTS - 1, self->editEffectSlot));
            const TrackerEffectDef *def = Tracker_EffectDefByCode(self->editEffectCodes[slot]);
            if (def->paramCount <= 1)
            {
                Tracker_ClearSliderCaptureOnUp(self, e);
                return true;
            }
            int value = Tracker_ValueFromSliderX(CLAY_ID("TrackerEffectParamBBar"), pointerX, def->minB, def->maxB);
            self->editEffectValues[slot] = Tracker_EffectSetB(def, self->editEffectValues[slot], value);
            Tracker_ApplyEditorToCell(self);
            Tracker_ClearSliderCaptureOnUp(self, e);
            return true;
        }
    }
    if (self->sliderDragging && e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT)
    {
        Tracker_ClearSliderCaptureOnUp(self, e);
        return true;
    }
    if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT)
    {
        for (int i = 0; i < 4; i++)
        {
            if (Clay_PointerOver(CLAY_IDI("TrackerSpecial", i)))
            {
                self->editSpecial = i + 1;
                Tracker_ApplyEditorToCell(self);
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
                    Tracker_ApplyEditorToCell(self);
                    return true;
                }
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
        if (self->editMacroTarget < XFM_MACRO_TL1) self->editMacroTarget = XFM_MACRO_ARP;
        self->editMacroValueIndex = 0;
        Tracker_SetMacroViewFirst(self, 0);
        self->macroViewAnimatedFirst = 0.0f;
        (void)Tracker_EditableMacro(self);
        return true;
    }
    if (isClaytonClicked(&self->macroTargetNextButton, e))
    {
        self->editMacroTarget++;
        if (self->editMacroTarget > XFM_MACRO_ARP) self->editMacroTarget = XFM_MACRO_TL1;
        self->editMacroValueIndex = 0;
        Tracker_SetMacroViewFirst(self, 0);
        self->macroViewAnimatedFirst = 0.0f;
        (void)Tracker_EditableMacro(self);
        return true;
    }
    if (isClaytonClicked(&self->macroEnableButton, e))
    {
        int inst = std::max(0, std::min(255, self->editInstrument));
        int target = std::max((int)XFM_MACRO_TL1, std::min((int)XFM_MACRO_ARP, self->editMacroTarget));
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
        int target = std::max((int)XFM_MACRO_TL1, std::min((int)XFM_MACRO_ARP, self->editMacroTarget));
        int valueMin = -64;
        int valueMax = 127;
        if (target >= XFM_MACRO_TL1 && target <= XFM_MACRO_TL4) valueMin = 0, valueMax = 127;
        else if (target >= XFM_MACRO_MUL1 && target <= XFM_MACRO_MUL4) valueMin = 0, valueMax = 15;
        else if (target >= XFM_MACRO_DT1 && target <= XFM_MACRO_DT4) valueMin = -3, valueMax = 3;
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
        Tracker_SnapInstruments(self);
        if (moved) return true;
    }

    if (isClaytonClicked(&self->instrumentsCloseButton, e))
    {
        self->instrumentsWindowOpen = false;
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
        if (isClaytonClicked(&self->instrumentCloneButtons[inst], e))
        {
            int target = Tracker_FirstFreeInstrumentSlot(self);
            if (target >= 0)
            {
                self->pendingInstrumentAction = 1;
                self->pendingInstrument = inst;
                self->pendingInstrumentTarget = target;
                std::snprintf(
                    self->pendingInstrumentName,
                    sizeof(self->pendingInstrumentName),
                    "%s",
                    Tracker_InstrumentName(self, inst)
                );
                self->pendingInstrumentNameLen = (int32_t)std::strlen(self->pendingInstrumentName);
            }
            return true;
        }
        if (!self->builtinInstruments[inst])
        {
            if (isClaytonClicked(&self->instrumentRenameButtons[inst], e))
            {
                self->pendingInstrumentAction = 2;
                self->pendingInstrument = inst;
                self->pendingInstrumentTarget = inst;
                std::snprintf(
                    self->pendingInstrumentName,
                    sizeof(self->pendingInstrumentName),
                    "%s",
                    Tracker_InstrumentName(self, inst)
                );
                self->pendingInstrumentNameLen = (int32_t)std::strlen(self->pendingInstrumentName);
                return true;
            }
            if (isClaytonClicked(&self->instrumentDeleteButtons[inst], e))
            {
                Tracker_DeleteInstrument(self, inst);
                return true;
            }
        }
    }

    if (e.type == SDL_MOUSEWHEEL && Clay_PointerOver(CLAY_ID("TrackerInstrumentsViewport")))
    {
        self->instrumentsScrollY = std::max(0.0f, self->instrumentsScrollY - e.wheel.y * 42.0f);
        Tracker_SnapInstruments(self);
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
        bool changed = false;
        if (sliderValue(CLAY_ID("TrackerSongLfoFreqBar"), 0, 7, value))
        {
            self->songLfoFrequency = value;
            changed = true;
        }
        else if (sliderValue(CLAY_ID("TrackerSongTickRateBar"), 30, 240, value))
        {
            self->songTickRate = value;
            changed = true;
        }
        else if (sliderValue(CLAY_ID("TrackerSongSpeedBar"), 1, 16, value))
        {
            self->songSpeed = value;
            self->ticksPerRow = value;
            changed = true;
        }
        if (changed)
        {
            self->patternDirty = true;
            self->copyOnWriteRequested = true;
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
    if (isClaytonClicked(&self->saveSongButton, e))
    {
        self->songSaveRequested = true;
        return true;
    }
    if (isClaytonClicked(&self->loadSongButton, e))
    {
        std::snprintf(self->songLoadStatus, sizeof(self->songLoadStatus), "Opening file...");
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
    if (isClaytonClicked(&self->copyButton, e))
    {
        Tracker_CopySelection(self);
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
        e.type == SDL_MOUSEWHEEL;
    if (!pointerEvent) return false;

    Clay_BoundingBox header = Clay_GetElementData(CLAY_ID("TrackerFixedHeader")).boundingBox;
    auto channelAtHeaderX = [&](float x) -> int {
        if (header.width <= 0.0f) return -1;
        float unit = header.width / 13.0f;
        float localX = x - header.x;
        int channel = (int)std::floor((localX - unit) / (unit * 2.0f));
        return channel >= 0 && channel < TRACKER_CHANNELS ? channel : -1;
    };
    bool overHeader = Clay_PointerOver(CLAY_ID("TrackerFixedHeader"));
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT && overHeader)
    {
        int channel = channelAtHeaderX((float)e.button.x);
        if (channel >= 0)
        {
            self->channelSelecting = true;
            self->channelAnchor = channel;
            Tracker_SetChannelSelection(self, channel, channel);
            return true;
        }
    }
    if (e.type == SDL_MOUSEMOTION && self->channelSelecting)
    {
        int channel = channelAtHeaderX((float)e.motion.x);
        if (channel >= 0)
            Tracker_SetChannelSelection(self, self->channelAnchor, channel);
        return true;
    }
    if (e.type == SDL_MOUSEBUTTONUP && self->channelSelecting)
    {
        int channel = channelAtHeaderX((float)e.button.x);
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
        self->loopSelectLocalY = localY;
        self->loopSelectViewportHeight = grid.height;
        int row = Tracker_RowAtViewportY(self, localY);
        Tracker_SetLoopRange(self, self->loopAnchor, row);
        return true;
    }
    if (e.type == SDL_MOUSEMOTION && self->loopMoving)
    {
        float localY = (float)e.motion.y - grid.y;
        self->loopSelectLocalY = localY;
        self->loopSelectViewportHeight = grid.height;
        int row = Tracker_RowAtViewportY(self, localY);
        Tracker_MoveLoopRangeToGrabbedRow(self, row);
        return true;
    }
    if (e.type == SDL_MOUSEBUTTONUP && self->loopSelecting)
    {
        float localY = (float)e.button.y - grid.y;
        self->loopSelectLocalY = localY;
        self->loopSelectViewportHeight = grid.height;
        int row = Tracker_RowAtViewportY(self, localY);
        Tracker_SetLoopRange(self, self->loopAnchor, row);
        self->loopSelecting = false;
        Tracker_SnapToGrid(self);
        return true;
    }
    if (e.type == SDL_MOUSEBUTTONUP && self->loopMoving)
    {
        float localY = (float)e.button.y - grid.y;
        self->loopSelectLocalY = localY;
        self->loopSelectViewportHeight = grid.height;
        int row = Tracker_RowAtViewportY(self, localY);
        Tracker_MoveLoopRangeToGrabbedRow(self, row);
        self->loopMoving = false;
        Tracker_SnapToGrid(self);
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
