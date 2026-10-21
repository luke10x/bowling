#pragma once

// school/school_clay.h
// Clay UI builders + event handling for School mode.
//
// Takes `School*` and `Clayton*` only (no UserContext dependency).

#include <SDL.h>
#include <glm/glm.hpp>
#include "../clayton/claytheme.h"
#include "../clayton/slider.h"
#include "school.h"

struct Clayton;
struct Physics;

inline void School_ClayInit(School *self, Clayton *clayton, float initialMassKg)
{
    (void)clayton;
    if (!self)
        return;
    initClaytonClick(&self->exitButton, "SchoolExitButton");
    for (int i = 0; i < 5; i++)
    {
        char id[32];
        (void)snprintf(id, sizeof(id), "SchoolLesson%d", i + 1);
        initClaytonClick(&self->lessonButtons[i], id);
    }

    ClaytonSlider_Init(
        &self->massSlider,
        "SchoolMass",
        SchoolMassTuning::MASS_MIN_KG,
        SchoolMassTuning::MASS_MAX_KG,
        glm::clamp(initialMassKg, SchoolMassTuning::MASS_MIN_KG, SchoolMassTuning::MASS_MAX_KG)
    );
}

// Returns true if event was consumed by the school UI.
inline bool School_ClayHandleEvent(
    School *self,
    const SDL_Event &e,
    int currentGameModeIsSchool,
    int *desiredLessonOut, // set when user clicks a lesson button
    bool *exitRequestedOut, // set when user clicks exit
    bool *massChangedOut,    // set when slider changes
    float *newMassKgOut      // new mass value
)
{
    if (!self || !currentGameModeIsSchool)
        return false;

    if (exitRequestedOut)
        *exitRequestedOut = false;
    if (desiredLessonOut)
        *desiredLessonOut = 0;
    if (massChangedOut)
        *massChangedOut = false;
    if (newMassKgOut)
        *newMassKgOut = 0.0f;

    if (isClaytonClicked(&self->exitButton, e))
    {
        if (exitRequestedOut)
            *exitRequestedOut = true;
        return true;
    }

    // Mass slider lives in a dedicated window now (see WindowKind_MassEditor),
    // so we do not process slider events here.

    for (int i = 0; i < 5; i++)
    {
        const int lessonNum = i + 1;
        const bool enabled = lessonNum <= self->unlockedLessons;
        if (isClaytonClicked(&self->lessonButtons[i], e))
        {
            if (!enabled)
                return true; // consume but ignore (disabled button)
            if (desiredLessonOut)
                *desiredLessonOut = lessonNum;
            return true;
        }
    }

    return false;
}

enum class SchoolProgressMark { Waiting, Active, Partial, Ready, Complete, DoneGreen };

inline void School_ClayProgressBox(int id, SchoolProgressMark mark, bool blink)
{
    const bool dim = blink && ((SDL_GetTicks() / 320) % 2 == 0);
    Clay_Color color = {95, 92, 111, 220};
    if (mark == SchoolProgressMark::Active) color = {225, 65, 80, 230};
    if (mark == SchoolProgressMark::Partial) color = {235, 190, 55, 235};
    if (mark == SchoolProgressMark::Ready) color = {60, 205, 110, 235};
    if (mark == SchoolProgressMark::Complete) color = {65, 150, 235, 240};
    if (mark == SchoolProgressMark::DoneGreen) color = {50, 200, 105, 240};
    if (dim) color.a = 105;
    CLAY(CLAY_IDI("SchoolProgressBox", id),
         {.layout = {.sizing = {CLAY_SIZING_FIXED(27), CLAY_SIZING_FIXED(27)},
                     .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}},
          .backgroundColor = color,
          .cornerRadius = {5, 5, 5, 5},
          .border = {.color = {235, 225, 245, 220}, .width = CLAY_BORDER_ALL(2)}})
    {
        if (mark == SchoolProgressMark::Complete || mark == SchoolProgressMark::DoneGreen)
        {
            Clay_TextElementConfig checkCfg = CLAY_THEME_TEXT_BUTTON;
            checkCfg.fontSize = CLAY_FONT_SIZE_SM;
            CLAY_TEXT(CLAY_STRING("x"), CLAY_TEXT_CONFIG(checkCfg));
        }
    }
}

inline void School_ClayBuildProgress(School *self, Clayton *clayton,
                                     bool aimThrowInFlight, bool oilCanReoilNow)
{
    if (!self || !clayton)
        return;

    // A single progress row stays in the same place for every lesson.
    CLAY(CLAY_ID("SchoolLessonProgressRow"),
         {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(45)},
                     .padding = {8, 8, 4, 4}, .childGap = 8,
                     .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                     .layoutDirection = CLAY_LEFT_TO_RIGHT},
          .backgroundColor = {35, 30, 52, 190},
          .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG}})
    {
        Clay_TextElementConfig progressText = CLAY_THEME_TEXT_BUTTON;
        progressText.fontSize = CLAY_FONT_SIZE_SM;
        progressText.textColor = {245, 240, 250, 240};
        if (self->selectedLesson == 2)
        {
            const float mass = self->massSlider.value;
            const bool lightSelected = mass <= SchoolMassTuning::LIGHT_TEST_MAX_KG;
            const bool heavySelected = mass >= SchoolMassTuning::HEAVY_TEST_MIN_KG;
            const auto massMark = [](int hits, bool selected) -> SchoolProgressMark
            {
                if (hits >= SchoolMassTuning::REQUIRED_HITS_EACH) return SchoolProgressMark::DoneGreen;
                if (hits > 0) return SchoolProgressMark::Partial;
                return selected ? SchoolProgressMark::Active : SchoolProgressMark::Waiting;
            };
            Clay_TextElementConfig lightText = progressText;
            Clay_TextElementConfig heavyText = progressText;
            if (!lightSelected && self->massLightHits == 0)
                lightText.textColor = {155, 150, 170, 225};
            if (!heavySelected && self->massHeavyHits == 0)
                heavyText.textColor = {155, 150, 170, 225};
            CLAY_TEXT(clayton->txl(TXL_SCHOOL_PROGRESS_LIGHT), CLAY_TEXT_CONFIG(lightText));
            School_ClayProgressBox(10, massMark(self->massLightHits, lightSelected), false);
            CLAY(CLAY_ID("SchoolMassProgressSpace"),
                 {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(1)}}}) {};
            CLAY_TEXT(clayton->txl(TXL_SCHOOL_PROGRESS_HEAVY), CLAY_TEXT_CONFIG(heavyText));
            School_ClayProgressBox(11, massMark(self->massHeavyHits, heavySelected), false);
        }
        else
        {
            Clay_String label = clayton->txl(TXL_SCHOOL_PROGRESS_GOOD_SHOTS);
            if (self->selectedLesson == 3) label = clayton->txl(TXL_SCHOOL_PROGRESS_HIT_ALL);
            if (self->selectedLesson == 4) label = clayton->txl(TXL_SCHOOL_PROGRESS_REOILS);
            if (self->selectedLesson == 5) label = clayton->txl(TXL_STRIKE);
            CLAY_TEXT(label, CLAY_TEXT_CONFIG(progressText));
            CLAY(CLAY_ID("SchoolProgressSpace"),
                 {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(1)}}}) {};

            const int boxes = self->selectedLesson == 3 ? SchoolSpinTuning::LEVELS
                            : self->selectedLesson == 5 ? 1 : 3;
            for (int i = 0; i < boxes; ++i)
            {
                SchoolProgressMark mark = SchoolProgressMark::Waiting;
                bool blink = false;
                if (self->selectedLesson == 1)
                {
                    if (i < self->aimLessonPoints) mark = SchoolProgressMark::Complete;
                    else if (i == self->aimLessonPoints)
                    {
                        mark = self->aimPullEnough ? SchoolProgressMark::Ready : SchoolProgressMark::Active;
                        blink = !self->aimPullEnough || (aimThrowInFlight && self->aimQualifiedThisThrow);
                    }
                }
                else if (self->selectedLesson == 3)
                {
                    const int finished = self->spinSafeCoins / SchoolSpinTuning::COINS_PER_LEVEL;
                    if (i < finished || self->spinTestCompleted) mark = SchoolProgressMark::DoneGreen;
                    else if (i == finished)
                    {
                        mark = self->spinCollectedInLevel >= SchoolSpinTuning::COINS_PER_LEVEL
                            ? SchoolProgressMark::DoneGreen
                            : self->spinCollectedInLevel > 0
                            ? SchoolProgressMark::Partial : SchoolProgressMark::Active;
                    }
                }
                else if (self->selectedLesson == 4)
                {
                    if (i < self->spinSafeCoins) mark = SchoolProgressMark::DoneGreen;
                    else if (i == self->spinSafeCoins && oilCanReoilNow)
                    { mark = SchoolProgressMark::Active; blink = true; }
                }
                else if (self->selectedLesson == 5)
                    mark = self->lessonDone[4] ? SchoolProgressMark::DoneGreen : SchoolProgressMark::Active;
                School_ClayProgressBox(self->selectedLesson * 10 + i, mark, blink);
            }
        }
    }
}

inline void School_ClayBuildPanel(School *self, Clayton *clayton)
{
    if (!self || !clayton)
        return;

    Clay_ElementDeclaration schoolPanel = CLAY_THEME_SECTION;
    schoolPanel.backgroundColor = (Clay_Color){60, 60, 80, 180};

    CLAY(CLAY_ID("SchoolPanel"), schoolPanel)
    {
        Clay_TextElementConfig titleCfg = CLAY_THEME_TEXT_TITLE;
        Clay_TextElementConfig buttonCfg = CLAY_THEME_TEXT_BUTTON;
        ClayArena *arena = &clayton->clayArena;

        // Title row
        CLAY(
            CLAY_ID("SchoolTitleRow"),
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                        .padding = {0, 0, 5, 0},
                        .childGap = 10,
                        .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
        )
        {
            const char *lessonNames[5] = {
                Txl_Get(clayton->uiLanguage, TXL_AIM_LESSON),
                Txl_Get(clayton->uiLanguage, TXL_BALL_MASS),
                Txl_Get(clayton->uiLanguage, TXL_SPIN_BALL),
                Txl_Get(clayton->uiLanguage, TXL_OIL_AND_SKID),
                Txl_Get(clayton->uiLanguage, TXL_STRIKE_LINE),
            };
            int li = self->selectedLesson - 1;
            if (li < 0)
                li = 0;
            if (li > 4)
                li = 4;
            Clay_String title = ClayArena_FormatString(
                arena, Txl_Get(clayton->uiLanguage, TXL_SCHOOL_TITLE_FMT), self->selectedLesson, lessonNames[li]
            );
            CLAY_TEXT(title, CLAY_TEXT_CONFIG(titleCfg));
            CLAY(CLAY_ID("SchoolTitleDivider"), {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1)}}})
            {
            }
            CLAY(self->exitButton.clayId, CLAY_THEME_BTN_DANGER)
            {
                CLAY_TEXT(CLAY_STRING("x"), CLAY_TEXT_CONFIG(buttonCfg));
            }
        }

        // Lessons row (1..5)
        CLAY(
            CLAY_ID("SchoolLessonsRow"),
            {
                .layout = {
                    .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                    .childGap = 10,
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                },
            }
        )
        {
            for (int i = 0; i < 5; i++)
            {
                const int lessonNum = i + 1;
                const bool enabled = lessonNum <= self->unlockedLessons;
                const bool selected = lessonNum == self->selectedLesson;

                Clay_Color bg = CLAY_COLOR_BTN_DISABLED;
                if (enabled && selected)
                    bg = CLAY_COLOR_BTN_SUCCESS;
                else if (enabled)
                    bg = CLAY_COLOR_BTN_PRIMARY;
                const Clay_Color borderColor = selected
                    ? (Clay_Color){255, 255, 255, 245}
                    : CLAY_COLOR_BORDER;
                const uint16_t borderWidth = selected ? 3 : 1;

                Clay_ElementDeclaration btn = {
                    .layout = {
                        .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(50)},
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                    },
                    .backgroundColor = ClayTheme_HoverColor(bg, enabled ? 18.0f : 10.0f),
                    .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
                    .border = {
                        .color = borderColor,
                        .width = CLAY_BORDER_ALL(borderWidth),
                    },
                };

                CLAY(self->lessonButtons[i].clayId, btn)
                {
                    Clay_String label = ClayArena_FormatString(arena, "%d", lessonNum);
                    CLAY_TEXT(label, CLAY_TEXT_CONFIG(buttonCfg));
                }
            }
        }
    }
}
