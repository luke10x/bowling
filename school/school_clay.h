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

    // Lesson 1: mass slider
    if (self->selectedLesson == 1)
    {
        if (ClaytonSlider_ProcessEvent(&self->massSlider, e))
        {
            float v = glm::clamp(
                self->massSlider.value, SchoolMassTuning::MASS_MIN_KG, SchoolMassTuning::MASS_MAX_KG
            );
            ClaytonSlider_SetValue(&self->massSlider, v);
            if (massChangedOut)
                *massChangedOut = true;
            if (newMassKgOut)
                *newMassKgOut = v;
            return true;
        }
    }

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

inline void School_ClayBuildPanel(School *self, Clayton *clayton)
{
    if (!self || !clayton)
        return;

    Clay_ElementDeclaration schoolPanel = CLAY_THEME_SECTION;
    schoolPanel.backgroundColor = (Clay_Color){60, 60, 80, 180};

    CLAY(CLAY_ID("SchoolPanel"), schoolPanel)
    {
        Clay_TextElementConfig titleCfg = CLAY_THEME_TEXT_TITLE;
        Clay_TextElementConfig bodyCfg = CLAY_THEME_TEXT_BODY;
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
                "Ball Mass", "Spin ball", "Lesson 3", "Lesson 4", "Lesson 5",
            };
            int li = self->selectedLesson - 1;
            if (li < 0)
                li = 0;
            if (li > 4)
                li = 4;
            Clay_String title = ClayArena_FormatString(
                arena, "School :: Lesson %d. %s", self->selectedLesson, lessonNames[li]
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
                    bg = CLAY_COLOR_BTN_ACTIVE;
                else if (enabled)
                    bg = CLAY_COLOR_BTN_PRIMARY;

                Clay_ElementDeclaration btn = {
                    .layout = {
                        .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(50)},
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                    },
                    .backgroundColor = bg,
                    .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
                    .border = {
                        .color = CLAY_COLOR_BORDER,
                        .width = CLAY_BORDER_ALL(1),
                    },
                };

                CLAY(self->lessonButtons[i].clayId, btn)
                {
                    Clay_String label = ClayArena_FormatString(arena, "%d", lessonNum);
                    CLAY_TEXT(label, CLAY_TEXT_CONFIG(buttonCfg));
                }
            }
        }

        // Per-lesson hint line (text only; actual slider is built by caller where appropriate)
        if (self->selectedLesson == 1)
        {
            CLAY(CLAY_ID("SchoolMassHintRow"), CLAY_THEME_SECTION)
            {
                ClaytonSlider_Render(&self->massSlider, clayton, "Mass", "kg");
                Clay_TextElementConfig hintCfg = bodyCfg;
                hintCfg.fontSize = CLAY_FONT_SIZE_MD;
                hintCfg.textColor = {220, 220, 240, 220};
                CLAY_TEXT(
                    CLAY_STRING("Try hit pins with a LIGHT ball (left) and a HEAVY ball (right)."),
                    CLAY_TEXT_CONFIG(hintCfg)
                );
            }
        }
        else if (self->selectedLesson == 2)
        {
            CLAY(CLAY_ID("SchoolSpinHintRow"), CLAY_THEME_SECTION)
            {
                Clay_TextElementConfig hintCfg = bodyCfg;
                hintCfg.fontSize = CLAY_FONT_SIZE_MD;
                hintCfg.textColor = {220, 220, 240, 220};
                CLAY_TEXT(
                    ClayArena_AllocString(arena, "Collect all coins to pass. Miss 1 coin in a level and you drop back."),
                    CLAY_TEXT_CONFIG(hintCfg)
                );
            }
        }

        // Progress bar (based on unlocked lessons)
        float frac = (float)(self->unlockedLessons - 1) / 4.0f;
        frac = glm::clamp(frac, 0.0f, 1.0f);
        CLAY(
            CLAY_ID("SchoolProgressOuter"),
            {
                .layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(18)}},
                .backgroundColor = {0.12f, 0.12f, 0.12f, 0.9f},
                .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
            }
        )
        {
            CLAY(
                CLAY_ID("SchoolProgressInner"),
                {
                    .layout = {.sizing = {CLAY_SIZING_PERCENT(frac), CLAY_SIZING_GROW()}},
                    .backgroundColor = {0.2f, 0.7f, 0.3f, 0.9f},
                    .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
                }
            )
            {
            }
        }
    }
}

inline void School_ClayBuildHud(School *self, Clayton *clayton)
{
    if (!self || !clayton)
        return;

    // Lesson 1 HUD (progress bars)
    if (self->selectedLesson == 1 && !self->massTestCompleted)
    {
        const int need = SchoolMassTuning::REQUIRED_HITS_EACH;
        float lightFrac = (need > 0) ? (float)self->massLightHits / (float)need : 0.0f;
        float heavyFrac = (need > 0) ? (float)self->massHeavyHits / (float)need : 0.0f;
        lightFrac = glm::clamp(lightFrac, 0.0f, 1.0f);
        heavyFrac = glm::clamp(heavyFrac, 0.0f, 1.0f);

        CLAY(
            CLAY_ID("SchoolMassTestHud"),
            {
                .layout = {
                    .sizing = {CLAY_SIZING_FIXED(230), CLAY_SIZING_FIT()},
                    .padding = {12, 12, 12, 12},
                    .childGap = 8,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
                .backgroundColor = {30, 30, 45, 160},
                .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
                .floating = {
                    .offset = {0.0f, -10.0f},
                    .zIndex = 2,
                    .attachPoints = {CLAY_ATTACH_POINT_LEFT_BOTTOM, CLAY_ATTACH_POINT_LEFT_BOTTOM},
                    .attachTo = CLAY_ATTACH_TO_PARENT,
                },
                .border = {
                    .color = CLAY_COLOR_BORDER,
                    .width = CLAY_BORDER_ALL(1),
                },
            }
        )
        {
            ClayArena *arena = &clayton->clayArena;
            Clay_TextElementConfig hudLabelCfg = CLAY_THEME_TEXT_BODY;
            hudLabelCfg.fontSize = CLAY_FONT_SIZE_SM;
            hudLabelCfg.textColor = {235, 235, 245, 230};
            Clay_TextElementConfig passedCfg = hudLabelCfg;
            passedCfg.textColor = {80, 220, 120, 235};

            const bool lightPassed = self->massLightHits >= need;
            const bool heavyPassed = self->massHeavyHits >= need;

            CLAY_TEXT(ClayArena_AllocString(arena, "Light ball test"), CLAY_TEXT_CONFIG(hudLabelCfg));
            if (lightPassed)
                CLAY_TEXT(ClayArena_AllocString(arena, "Passed"), CLAY_TEXT_CONFIG(passedCfg));

            CLAY(
                CLAY_ID("SchoolMassTestLightOuter"),
                {
                    .layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(14)}},
                    .backgroundColor = {0, 0, 0, 120},
                    .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
                }
            )
            {
                CLAY(
                    CLAY_ID("SchoolMassTestLightInner"),
                    {
                        .layout = {.sizing = {CLAY_SIZING_PERCENT(lightFrac), CLAY_SIZING_GROW()}},
                        .backgroundColor = {80, 190, 255, 200},
                        .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
                    }
                )
                {
                }
            }

            CLAY_TEXT(ClayArena_AllocString(arena, "Heavy ball test"), CLAY_TEXT_CONFIG(hudLabelCfg));
            if (heavyPassed)
                CLAY_TEXT(ClayArena_AllocString(arena, "Passed"), CLAY_TEXT_CONFIG(passedCfg));

            CLAY(
                CLAY_ID("SchoolMassTestHeavyOuter"),
                {
                    .layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(14)}},
                    .backgroundColor = {0, 0, 0, 120},
                    .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
                }
            )
            {
                CLAY(
                    CLAY_ID("SchoolMassTestHeavyInner"),
                    {
                        .layout = {.sizing = {CLAY_SIZING_PERCENT(heavyFrac), CLAY_SIZING_GROW()}},
                        .backgroundColor = {255, 120, 80, 200},
                        .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
                    }
                )
                {
                }
            }
        }
    }

    // Lesson 2 HUD (coins progress)
    if (self->selectedLesson == 2 && !self->spinTestCompleted)
    {
        const int totalNeeded = SchoolSpinTuning::TOTAL_REQUIRED;
        int total = self->spinSafeCoins + self->spinCollectedInLevel;
        total = glm::clamp(total, 0, totalNeeded);
        float frac = (totalNeeded > 0) ? (float)total / (float)totalNeeded : 0.0f;
        frac = glm::clamp(frac, 0.0f, 1.0f);

        CLAY(
            CLAY_ID("SchoolSpinTestHud"),
            {
                .layout = {
                    .sizing = {CLAY_SIZING_FIXED(260), CLAY_SIZING_FIT()},
                    .padding = {12, 12, 12, 12},
                    .childGap = 8,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
                .backgroundColor = {30, 30, 45, 160},
                .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
                .floating = {
                    .offset = {0.0f, -10.0f},
                    .zIndex = 2,
                    .attachPoints = {CLAY_ATTACH_POINT_LEFT_BOTTOM, CLAY_ATTACH_POINT_LEFT_BOTTOM},
                    .attachTo = CLAY_ATTACH_TO_PARENT,
                },
                .border = {
                    .color = CLAY_COLOR_BORDER,
                    .width = CLAY_BORDER_ALL(1),
                },
            }
        )
        {
            ClayArena *arena = &clayton->clayArena;
            Clay_TextElementConfig hudLabelCfg = CLAY_THEME_TEXT_BODY;
            hudLabelCfg.fontSize = CLAY_FONT_SIZE_SM;
            hudLabelCfg.textColor = {235, 235, 245, 230};

            Clay_String title = ClayArena_FormatString(
                arena,
                "Spin test: %d/%d coins (Level %d/%d)",
                total,
                totalNeeded,
                self->spinLevel,
                SchoolSpinTuning::LEVELS
            );
            CLAY_TEXT(title, CLAY_TEXT_CONFIG(hudLabelCfg));

            CLAY(
                CLAY_ID("SchoolSpinCoinsOuter"),
                {
                    .layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(14)}},
                    .backgroundColor = {0, 0, 0, 120},
                    .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
                }
            )
            {
                CLAY(
                    CLAY_ID("SchoolSpinCoinsInner"),
                    {
                        .layout = {.sizing = {CLAY_SIZING_PERCENT(frac), CLAY_SIZING_GROW()}},
                        .backgroundColor = {130, 210, 255, 200},
                        .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
                    }
                )
                {
                }
            }
        }
    }
}
