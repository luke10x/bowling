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
            Clay_String title = ClayArena_FormatString(arena, "Instrument %02X", self->editInstrument);
            CLAY_TEXT(title, CLAY_TEXT_CONFIG(titleCfg));
            CLAY(CLAY_ID("TrackerInstrumentEditorGrow"), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()}}}) {}
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
        }

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
                        CLAY(self->operatorButtons[op].clayId, CLAY_THEME_BTN_PRIMARY)
                        {
                            Clay_String text = ClayArena_FormatString(arena, "OP%d", op + 1);
                            CLAY_TEXT(text, CLAY_TEXT_CONFIG(buttonCfg));
                        }
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
            else if (target == XFM_MACRO_ARP) valueMin = -48, valueMax = 48;
            bool signedMacro = valueMin < 0 && valueMax > 0;
            float zeroT = signedMacro ? (float)valueMax / (float)(valueMax - valueMin) : 1.0f;
            zeroT = std::max(0.0f, std::min(1.0f, zeroT));
            Clay_Color graphBg = enabled ? (Clay_Color){18, 20, 30, 255} : (Clay_Color){42, 42, 46, 255};
            Clay_Color posColor = enabled ? (Clay_Color){96, 170, 236, 255} : (Clay_Color){92, 92, 96, 255};
            Clay_Color negColor = enabled ? (Clay_Color){232, 114, 118, 255} : (Clay_Color){82, 82, 86, 255};
            Clay_TextElementConfig tinyCfg = bodyCfg;
            tinyCfg.fontSize = 8;
            tinyCfg.fontId = CLAY_FONT_MONO;

            auto renderMacroLane = [&](int lane) {
                int first = lane * TRACKER_MACRO_LANE_STEPS;
                CLAY(
                    CLAY_IDI("TrackerMacroResetRow", lane),
                    {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(18)},
                                .childGap = 1,
                                .layoutDirection = CLAY_LEFT_TO_RIGHT}}
                )
                {
                    for (int local = 0; local < TRACKER_MACRO_LANE_STEPS; local++)
                    {
                        int i = first + local;
                        CLAY(
                            CLAY_IDI("TrackerMacroReset", i),
                            {.layout = {.sizing = {CLAY_SIZING_PERCENT(1.0f / (float)TRACKER_MACRO_LANE_STEPS), CLAY_SIZING_GROW()},
                                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}},
                             .backgroundColor = enabled ? (Clay_Color){36, 40, 52, 255} : (Clay_Color){48, 48, 52, 255},
                             .cornerRadius = {2, 2, 2, 2}}
                        )
                        {
                            CLAY_TEXT(CLAY_STRING("x"), CLAY_TEXT_CONFIG(tinyCfg));
                        }
                    }
                }

                CLAY(
                    CLAY_IDI("TrackerMacroGraph", lane),
                    {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(78)},
                                .padding = {3, 3, 3, 3},
                                .childGap = 1,
                                .layoutDirection = CLAY_LEFT_TO_RIGHT},
                     .backgroundColor = graphBg,
                     .border = {.color = {78, 84, 106, 255}, .width = CLAY_BORDER_ALL(1)}}
                )
                {
                    for (int local = 0; local < TRACKER_MACRO_LANE_STEPS; local++)
                    {
                        int i = first + local;
                        int v = std::max(valueMin, std::min(valueMax, (int)macro.values[i]));
                        macro.values[i] = (int16_t)v;
                        float valueT = valueMax > valueMin ? ((float)valueMax - (float)v) / (float)(valueMax - valueMin) : 1.0f;
                        valueT = std::max(0.0f, std::min(1.0f, valueT));
                        float posFill = signedMacro ? std::max(0.0f, zeroT - valueT) / std::max(0.001f, zeroT) : 1.0f - valueT;
                        float negFill = signedMacro ? std::max(0.0f, valueT - zeroT) / std::max(0.001f, 1.0f - zeroT) : 0.0f;
                        CLAY(
                            CLAY_IDI("TrackerMacroBarColumn", i),
                            {.layout = {.sizing = {CLAY_SIZING_PERCENT(1.0f / (float)TRACKER_MACRO_LANE_STEPS), CLAY_SIZING_GROW()},
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
                                    CLAY(
                                        CLAY_IDI("TrackerMacroBarPos", i),
                                        {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_PERCENT(posFill)}},
                                         .backgroundColor = posColor}
                                    ) {}
                            }
                            CLAY(
                                CLAY_IDI("TrackerMacroBarBottom", i),
                                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_PERCENT(1.0f - zeroT)},
                                            .layoutDirection = CLAY_TOP_TO_BOTTOM},
                                 .backgroundColor = signedMacro ? (Clay_Color){32, 28, 34, 255} : graphBg}
                            )
                            {
                                if (negFill > 0.0f)
                                    CLAY(
                                        CLAY_IDI("TrackerMacroBarNeg", i),
                                        {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_PERCENT(negFill)}},
                                         .backgroundColor = negColor}
                                    ) {}
                                if (negFill < 1.0f)
                                    CLAY(CLAY_IDI("TrackerMacroBarNegSpace", i), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_PERCENT(1.0f - negFill)}}}) {}
                            }
                        }
                    }
                }

                CLAY(
                    CLAY_IDI("TrackerMacroValueNumbers", lane),
                    {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(24)},
                                .childGap = 1,
                                .layoutDirection = CLAY_LEFT_TO_RIGHT}}
                )
                {
                    for (int local = 0; local < TRACKER_MACRO_LANE_STEPS; local++)
                    {
                        int i = first + local;
                        bool inLoopRange = macro.has_loop &&
                                           i >= (int)macro.loop_start &&
                                           (macro.release_start == 0xFF || i < (int)macro.release_start);
                        bool inReleaseRange = macro.release_start != 0xFF && i >= (int)macro.release_start;
                        Clay_Color numberBg = inLoopRange ? (Clay_Color){40, 90, 72, 255} :
                                              inReleaseRange ? (Clay_Color){84, 54, 42, 255} :
                                              (Clay_Color){26, 28, 38, 255};
                        CLAY(
                            CLAY_IDI("TrackerMacroValueNumber", i),
                            {.layout = {.sizing = {CLAY_SIZING_PERCENT(1.0f / (float)TRACKER_MACRO_LANE_STEPS), CLAY_SIZING_GROW()},
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
            };

            renderMacroLane(0);
            renderMacroLane(1);

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
            Clay_String title = ClayArena_FormatString(arena, "Instrument %02X  OP%d", self->editInstrument, opIndex + 1);
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
        slider("AR", op.AR, 0, 31, CLAY_ID("TrackerOpArBar"), CLAY_ID("TrackerOpArFill"));
        slider("DR", op.DR, 0, 31, CLAY_ID("TrackerOpDrBar"), CLAY_ID("TrackerOpDrFill"));
        slider("SL", op.SL, 0, 15, CLAY_ID("TrackerOpSlBar"), CLAY_ID("TrackerOpSlFill"));
        slider("SR", op.SR, 0, 31, CLAY_ID("TrackerOpSrBar"), CLAY_ID("TrackerOpSrFill"));
        slider("RR", op.RR, 0, 15, CLAY_ID("TrackerOpRrBar"), CLAY_ID("TrackerOpRrFill"));
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
            Clay_ElementDeclaration amBtn = CLAY_THEME_BTN_PRIMARY;
            if (op.AM) amBtn.backgroundColor = CLAY_COLOR_BTN_SUCCESS;
            CLAY(self->operatorAmButton.clayId, amBtn)
            {
                CLAY_TEXT(CLAY_STRING("AM"), CLAY_TEXT_CONFIG(buttonCfg));
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
    monoCfg.fontSize = 11;
    Clay_TextElementConfig effectMonoCfg = monoCfg;
    effectMonoCfg.fontSize = 9;
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
                                    char top[8] = ".......";
                                    char bottom[18] = "";
                                    const char *cell = self->cells[row][ch].text;
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
                                        CLAY_TEXT(ClayArena_AllocString(arena, top), CLAY_TEXT_CONFIG(monoCfg));
                                        if (bottom[0])
                                            CLAY_TEXT(ClayArena_AllocString(arena, bottom), CLAY_TEXT_CONFIG(effectMonoCfg));
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
            float scrollbarRangeTop = self->viewportHeight * ((float)std::max(0, self->loopStart) / rowCountForMap);
            float scrollbarRangeBottom =
                self->viewportHeight * ((float)std::min(self->rowCount, self->loopEnd + 1) / rowCountForMap);
            float scrollbarRangeHeight = std::max(3.0f, scrollbarRangeBottom - scrollbarRangeTop);
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
    Tracker_RebuildUsedInstruments(self);
    self->editRow = std::max(0, std::min(row, self->rowCount - 1));
    self->editChannel = std::max(0, std::min(channel, TRACKER_CHANNELS - 1));
    Tracker_ParseCellForEditor(self);
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
        self->editInstrument = Tracker_NextUsedInstrument(self, self->editInstrument, -1);
        Tracker_NormalizeExplicitFields(self);
        Tracker_ApplyEditorToCell(self);
        return true;
    }
    if (isClaytonClicked(&self->instrumentNextButton, e))
    {
        self->editInstrument = Tracker_NextUsedInstrument(self, self->editInstrument, 1);
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
            Tracker_NormalizeExplicitFields(self);
            Tracker_ApplyEditorToCell(self);
            return true;
        }
        if (Clay_PointerOver(CLAY_ID("TrackerEffectParamABar")))
        {
            int slot = std::max(0, std::min(TRACKER_MAX_EFFECT_SLOTS - 1, self->editEffectSlot));
            const TrackerEffectDef *def = Tracker_EffectDefByCode(self->editEffectCodes[slot]);
            if (def->paramCount <= 0) return true;
            Clay_BoundingBox b = Clay_GetElementData(CLAY_ID("TrackerEffectParamABar")).boundingBox;
            float t = b.width > 0.0f ? (pointerX - b.x) / b.width : 0.0f;
            t = std::max(0.0f, std::min(1.0f, t));
            int value = def->minA + (int)std::round(t * (float)(def->maxA - def->minA));
            self->editEffectValues[slot] = Tracker_EffectSetA(def, self->editEffectValues[slot], value);
            Tracker_ApplyEditorToCell(self);
            return true;
        }
        if (Clay_PointerOver(CLAY_ID("TrackerEffectParamBBar")))
        {
            int slot = std::max(0, std::min(TRACKER_MAX_EFFECT_SLOTS - 1, self->editEffectSlot));
            const TrackerEffectDef *def = Tracker_EffectDefByCode(self->editEffectCodes[slot]);
            if (def->paramCount <= 1) return true;
            Clay_BoundingBox b = Clay_GetElementData(CLAY_ID("TrackerEffectParamBBar")).boundingBox;
            float t = b.width > 0.0f ? (pointerX - b.x) / b.width : 0.0f;
            t = std::max(0.0f, std::min(1.0f, t));
            int value = def->minB + (int)std::round(t * (float)(def->maxB - def->minB));
            self->editEffectValues[slot] = Tracker_EffectSetB(def, self->editEffectValues[slot], value);
            Tracker_ApplyEditorToCell(self);
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
    if (isClaytonClicked(&self->macroTargetPrevButton, e))
    {
        self->editMacroTarget--;
        if (self->editMacroTarget < XFM_MACRO_TL1) self->editMacroTarget = XFM_MACRO_ARP;
        self->editMacroValueIndex = 0;
        (void)Tracker_EditableMacro(self);
        return true;
    }
    if (isClaytonClicked(&self->macroTargetNextButton, e))
    {
        self->editMacroTarget++;
        if (self->editMacroTarget > XFM_MACRO_ARP) self->editMacroTarget = XFM_MACRO_TL1;
        self->editMacroValueIndex = 0;
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
        for (int i = 0; i < TRACKER_MACRO_UI_STEPS; i++)
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
        for (int i = 0; i < TRACKER_MACRO_UI_STEPS; i++)
        {
            if (Clay_PointerOver(CLAY_IDI("TrackerMacroValueNumber", i)))
            {
                Tracker_SetMacroLoopRange(self, self->macroRangeAnchor, i);
                if (e.type == SDL_MOUSEBUTTONUP)
                    self->macroRangeSelecting = false;
                return true;
            }
        }
        if (e.type == SDL_MOUSEBUTTONUP)
        {
            self->macroRangeSelecting = false;
            return true;
        }
    }
    if (mouseSliderEvent)
    {
        float pointerX = e.type == SDL_MOUSEMOTION ? (float)e.motion.x : (float)e.button.x;
        auto sliderValue = [&](Clay_ElementId id, int maxValue, int &out) -> bool {
            if (!Clay_PointerOver(id)) return false;
            Clay_BoundingBox b = Clay_GetElementData(id).boundingBox;
            float t = b.width > 0.0f ? (pointerX - b.x) / b.width : 0.0f;
            out = std::max(0, std::min(maxValue, (int)std::round(t * (float)maxValue)));
            return true;
        };
        int value = 0;
        if (sliderValue(CLAY_ID("TrackerPatchFbBar"), 7, value))
        {
            patch.FB = (uint8_t)value;
            Tracker_MarkPatchDirty(self);
            return true;
        }
        if (sliderValue(CLAY_ID("TrackerPatchAmsBar"), 3, value))
        {
            patch.AMS = (uint8_t)value;
            Tracker_MarkPatchDirty(self);
            return true;
        }
        if (sliderValue(CLAY_ID("TrackerPatchFmsBar"), 7, value))
        {
            patch.FMS = (uint8_t)value;
            Tracker_MarkPatchDirty(self);
            return true;
        }
        XfmMacro &macro = Tracker_EditableMacro(self);
        Tracker_EnsureMacroUiLength(&macro);
        int target = std::max((int)XFM_MACRO_TL1, std::min((int)XFM_MACRO_ARP, self->editMacroTarget));
        int valueMin = -64;
        int valueMax = 127;
        if (target >= XFM_MACRO_TL1 && target <= XFM_MACRO_TL4) valueMin = 0, valueMax = 127;
        else if (target >= XFM_MACRO_MUL1 && target <= XFM_MACRO_MUL4) valueMin = 0, valueMax = 15;
        else if (target >= XFM_MACRO_DT1 && target <= XFM_MACRO_DT4) valueMin = -3, valueMax = 3;
        else if (target == XFM_MACRO_FB) valueMin = 0, valueMax = 7;
        else if (target == XFM_MACRO_ARP) valueMin = -48, valueMax = 48;
        int graphLane = -1;
        for (int lane = 0; lane < 2; lane++)
        {
            if (Clay_PointerOver(CLAY_IDI("TrackerMacroGraph", lane)))
            {
                graphLane = lane;
                break;
            }
        }
        bool graphActive = self->macroDrawing || graphLane >= 0;
        if (graphActive)
        {
            if (graphLane < 0)
                graphLane = std::max(0, std::min(1, self->editMacroValueIndex / TRACKER_MACRO_LANE_STEPS));
            Clay_BoundingBox b = Clay_GetElementData(CLAY_IDI("TrackerMacroGraph", graphLane)).boundingBox;
            float pointerY = e.type == SDL_MOUSEMOTION ? (float)e.motion.y : (float)e.button.y;
            float xT = b.width > 0.0f ? (pointerX - b.x) / b.width : 0.0f;
            float yT = b.height > 0.0f ? (pointerY - b.y) / b.height : 0.0f;
            xT = std::max(0.0f, std::min(0.9999f, xT));
            yT = std::max(0.0f, std::min(1.0f, yT));
            int local = std::max(0, std::min(TRACKER_MACRO_LANE_STEPS - 1, (int)std::floor(xT * (float)TRACKER_MACRO_LANE_STEPS)));
            int idx = std::max(0, std::min(TRACKER_MACRO_UI_STEPS - 1, graphLane * TRACKER_MACRO_LANE_STEPS + local));
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
        self->macroDrawing = false;
        self->macroRangeSelecting = false;
    }
    if (pointerEvent && Clay_PointerOver(CLAY_ID("TrackerInstrumentEditorWindow"))) return true;
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
    const bool mouseSliderEvent =
        (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) ||
        (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) ||
        (e.type == SDL_MOUSEMOTION && (e.motion.state & SDL_BUTTON_LMASK));
    if (mouseSliderEvent)
    {
        float pointerX = e.type == SDL_MOUSEMOTION ? (float)e.motion.x : (float)e.button.x;
        auto sliderValue = [&](Clay_ElementId id, int minValue, int maxValue, int &out) -> bool {
            if (!Clay_PointerOver(id)) return false;
            Clay_BoundingBox b = Clay_GetElementData(id).boundingBox;
            float t = b.width > 0.0f ? (pointerX - b.x) / b.width : 0.0f;
            t = std::max(0.0f, std::min(1.0f, t));
            out = minValue + (int)std::round(t * (float)(maxValue - minValue));
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
            return true;
        }
    }
    if (pointerEvent && Clay_PointerOver(CLAY_ID("TrackerOperatorEditorWindow"))) return true;
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
            self->loopSelectLocalY = localY;
            self->loopSelectViewportHeight = grid.height;
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
        self->loopSelectLocalY = localY;
        self->loopSelectViewportHeight = grid.height;
        int row = Tracker_RowAtViewportY(self, localY);
        Tracker_SetLoopRange(self, self->loopAnchor, row);
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
