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

inline void School_ClayBuildPanel(School *self, Clayton *clayton, uint16_t portraitPadding)
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
                    bg = CLAY_COLOR_BTN_ACTIVE;
                else if (enabled)
                    bg = CLAY_COLOR_BTN_PRIMARY;

                Clay_ElementDeclaration btn = {
                    .layout = {
                        .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(50)},
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                    },
                    .backgroundColor = ClayTheme_HoverColor(bg, enabled ? 18.0f : 10.0f),
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

        // Per-lesson hint line (text only; sliders live in dedicated editors to save screen space)
        if (self->selectedLesson == 2)
        {

//            CLAY(CLAY_ID("SchoolMassHintRow"), CLAY_THEME_SECTION)
//            {
//                Clay_TextElementConfig hintCfg = bodyCfg;
//                hintCfg.fontSize = CLAY_FONT_SIZE_MD;
//                hintCfg.textColor = {220, 220, 240, 220};
//                CLAY_TEXT(
//                    CLAY_STRING("Try hit pins with a LIGHT ball (left) and a HEAVY ball (right)."),
//                    CLAY_TEXT_CONFIG(hintCfg)
//                );
//            }

            // Actually i want the following to be displayed outside school panel just like in the game is outside of th scoreboard
            //
            // Mass editor opener (HUD style), only visible in Mass lesson.
//            CLAY(CLAY_ID("SchoolMassEditorOpen"), CLAY_THEME_BTN_HUD)
//            {
//                Clay_TextElementConfig buttonCfg2 = CLAY_THEME_TEXT_BUTTON;
//                ClayArena *arena = &clayton->clayArena;
//                Clay_String label = ClayArena_FormatString(
//                    arena, "MASS (%.1fKG)", (double)self->massSlider.value
//                );
//                CLAY_TEXT(label, CLAY_TEXT_CONFIG(buttonCfg2));
//            }
        }
        else if (self->selectedLesson == 3)
        {
            // CLAY(CLAY_ID("SchoolSpinHintRow"), CLAY_THEME_SECTION)
            // {
            //     Clay_TextElementConfig hintCfg = bodyCfg;
            //     hintCfg.fontSize = CLAY_FONT_SIZE_MD;
            //     hintCfg.textColor = {220, 220, 240, 220};
            //     CLAY_TEXT(
            //         ClayArena_AllocString(arena, "Collect all coins to pass. Miss 1 coin in a level and you drop back."),
            //         CLAY_TEXT_CONFIG(hintCfg)
            //     );
            // }
        }
        else if (self->selectedLesson == 4)
        {
            // CLAY(CLAY_ID("SchoolOilHintRow"), CLAY_THEME_SECTION)
            // {
            //     Clay_TextElementConfig hintCfg = bodyCfg;
            //     hintCfg.fontSize = CLAY_FONT_SIZE_MD;
            //     hintCfg.textColor = {220, 220, 240, 220};
            //     CLAY_TEXT(
            //         CLAY_STRING("Lesson 4: the lane was just oiled. It will wear out quickly. Try a few shots."),
            //         CLAY_TEXT_CONFIG(hintCfg)
            //     );
            // }
        }
        else if (self->selectedLesson == 5)
        {
            // CLAY(CLAY_ID("SchoolStrikeHintRow"), CLAY_THEME_SECTION)
            // {
            //     Clay_TextElementConfig hintCfg = bodyCfg;
            //     hintCfg.fontSize = CLAY_FONT_SIZE_MD;
            //     hintCfg.textColor = {220, 220, 240, 220};
            //     CLAY_TEXT(
            //         CLAY_STRING("Lesson 5: follow the coin line and score a STRIKE."),
            //         CLAY_TEXT_CONFIG(hintCfg)
            //     );
            // }
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


    if (self->selectedLesson == 2)
    {
        CLAY(
            CLAY_ID("MenuRowMassOnlyTbh"),
            {.layout =
                 {
                     .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                     .padding = {.top = portraitPadding, .bottom = portraitPadding},
                     .childGap = portraitPadding,
                     .childAlignment =
                         {
                             .x = CLAY_ALIGN_X_CENTER,
                             .y = CLAY_ALIGN_Y_CENTER,
                         },
                     .layoutDirection = CLAY_LEFT_TO_RIGHT,
                 }}
        )
        { // BEGIN menu
            // Mass editor opener (HUD style), only visible in Mass lesson.
            CLAY(CLAY_ID("SchoolMassEditorOpen"), CLAY_THEME_BTN_HUD)
            {
                Clay_TextElementConfig buttonCfg2 = CLAY_THEME_TEXT_BUTTON;
                ClayArena *arena = &clayton->clayArena;
                Clay_String label = ClayArena_FormatString(
                    arena, Txl_Get(clayton->uiLanguage, TXL_MASS_FMT), (double)self->massSlider.value
                );
                CLAY_TEXT(label, CLAY_TEXT_CONFIG(buttonCfg2));
            }
            CLAY( CLAY_ID("Right spacer 1/3"), { .layout = { .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()}, }, }){};
            CLAY( CLAY_ID("Right spacer 2/3"), { .layout = { .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()}, }, }){};
        } // END Menu
    }
    if (self->selectedLesson == 4)
    {
        CLAY(
            CLAY_ID("MenuRowOilOnly"),
            {.layout =
                 {
                     .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                     .padding = {.top = portraitPadding, .bottom = portraitPadding},
                     .childGap = portraitPadding,
                     .childAlignment =
                         {
                             .x = CLAY_ALIGN_X_CENTER,
                             .y = CLAY_ALIGN_Y_CENTER,
                         },
                     .layoutDirection = CLAY_LEFT_TO_RIGHT,
                 }}
        )
        {
            // Oil window opener (HUD style), only visible in Oil lesson.
            CLAY(CLAY_ID("SchoolOilWindowOpen"), CLAY_THEME_BTN_HUD)
            {
                CLAY_TEXT(clayton->txl(TXL_OIL), CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_BUTTON));
            }
            CLAY(CLAY_ID("OilSpacer1"), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},},}){};
            CLAY(CLAY_ID("OilSpacer2"), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},},}){};
        }
    }
    if (self->selectedLesson == 5)
    {
        CLAY(CLAY_ID("MenuRowStrikeOnly"), {.layout =
             {
                 .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                 .padding = {.top = portraitPadding, .bottom = portraitPadding},
                 .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
                 .layoutDirection = CLAY_LEFT_TO_RIGHT,
             }})
        {
            CLAY(CLAY_ID("StrikeSpacer1"), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},},}){};
            CLAY(CLAY_ID("StrikeSpacer2"), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},},}){};
        }
    }
}

inline void School_ClayBuildHud(
    School *self,
    Clayton *clayton,
    float aimNdcX,
    bool showAimIndicator,
    float oilRemaining01,
    int oilReoilCount,
    int oilReoilNeeded,
    bool oilCanReoilNow
)
{
    if (!self || !clayton)
        return;

    // Lesson 2 HUD (progress bars)
    if (self->selectedLesson == 2 && !self->massTestCompleted)
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

            CLAY_TEXT(clayton->txl(TXL_LIGHT_BALL_TEST), CLAY_TEXT_CONFIG(hudLabelCfg));
            if (lightPassed)
                CLAY_TEXT(clayton->txl(TXL_PASSED), CLAY_TEXT_CONFIG(passedCfg));

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

            CLAY_TEXT(clayton->txl(TXL_HEAVY_BALL_TEST), CLAY_TEXT_CONFIG(hudLabelCfg));
            if (heavyPassed)
                CLAY_TEXT(clayton->txl(TXL_PASSED), CLAY_TEXT_CONFIG(passedCfg));

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

    // Lesson 3 HUD (coins progress)
    if (self->selectedLesson == 3 && !self->spinTestCompleted)
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
                Txl_Get(clayton->uiLanguage, TXL_SPIN_TEST_FMT),
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

    // Lesson 3 HUD (aim/pullback)
    if (self->selectedLesson == 1 && !self->aimLessonCompleted)
    {
        const int need = SchoolAimTuning::REQUIRED_POINTS;
        int pts = glm::clamp(self->aimLessonPoints, 0, need);
        float ptsFrac = (need > 0) ? (float)pts / (float)need : 0.0f;
        ptsFrac = glm::clamp(ptsFrac, 0.0f, 1.0f);

        // Bottom-left lesson progress
        CLAY(
            CLAY_ID("SchoolAimTestHud"),
            {
                .layout = {
                    .sizing = {CLAY_SIZING_FIXED(240), CLAY_SIZING_FIT()},
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

            Clay_String title = ClayArena_FormatString(arena, Txl_Get(clayton->uiLanguage, TXL_AIM_LESSON_POINTS_FMT), pts, need);
            CLAY_TEXT(title, CLAY_TEXT_CONFIG(hudLabelCfg));

            CLAY(
                CLAY_ID("SchoolAimPtsOuter"),
                {
                    .layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(14)}},
                    .backgroundColor = {0, 0, 0, 120},
                    .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
                }
            )
            {
                CLAY(
                    CLAY_ID("SchoolAimPtsInner"),
                    {
                        .layout = {.sizing = {CLAY_SIZING_PERCENT(ptsFrac), CLAY_SIZING_GROW()}},
                        .backgroundColor = {130, 210, 255, 200},
                        .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
                    }
                )
                {
                }
            }
        }

        if (showAimIndicator)
        {
            // Instruction banner (25% below top of screen)
            float yOff = clayton->renderer.screenHeight * 0.25f;
            const char *msg = Txl_Get(clayton->uiLanguage, TXL_PULL_BALL_BACK);
            Clay_Color bannerBg = {120, 40, 40, 190}; // red (not enough pull)
            if (self->aimPullEnough)
            {
                if (self->aimCenteredEnough)
                {
                    msg = Txl_Get(clayton->uiLanguage, TXL_LET_IT_GO_NOW);
                    bannerBg = (Clay_Color){40, 120, 60, 190}; // green
                }
                else
                {
                    msg = (aimNdcX < 0.0f) ? Txl_Get(clayton->uiLanguage, TXL_MOVE_RIGHT) : Txl_Get(clayton->uiLanguage, TXL_MOVE_LEFT);
                    bannerBg = (Clay_Color){160, 120, 35, 190}; // yellow
                }
            }

            CLAY(
                CLAY_ID("SchoolAimBanner"),
                {
                    .layout = {
                        .sizing = {CLAY_SIZING_FIXED(360), CLAY_SIZING_FIT()},
                        .padding = {12, 14, 12, 14},
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT,
                    },
                    .backgroundColor = bannerBg,
                    .cornerRadius = {CLAY_RADIUS_XL, CLAY_RADIUS_XL, CLAY_RADIUS_XL, CLAY_RADIUS_XL},
                    .floating = {
                        .offset = {0.0f, yOff},
                        .zIndex = 3,
                        .attachPoints = {CLAY_ATTACH_POINT_CENTER_TOP, CLAY_ATTACH_POINT_CENTER_TOP},
                        .attachTo = CLAY_ATTACH_TO_PARENT,
                    },
                    .border = {
                        .color = CLAY_COLOR_BORDER,
                        .width = CLAY_BORDER_ALL(1),
                    },
                }
            )
            {
                Clay_TextElementConfig bannerCfg = CLAY_THEME_TEXT_BODY;
                bannerCfg.fontSize = CLAY_FONT_SIZE_MD;
                bannerCfg.textColor = {255, 255, 255, 235};
                CLAY_TEXT(ClayArena_AllocString(&clayton->clayArena, msg), CLAY_TEXT_CONFIG(bannerCfg));
            }

            // Pullback bar directly under the banner (red -> green).
            {
                float pull = glm::clamp(self->aimPull01, 0.0f, 1.0f);
                Clay_Color fill = {200, 60, 60, 210};
                if (pull >= SchoolAimTuning::PULL_ENOUGH_THRESHOLD)
                    fill = (Clay_Color){60, 200, 120, 220};

                CLAY(
                    CLAY_ID("SchoolAimPullOuter"),
                    {
                        .layout = {.sizing = {CLAY_SIZING_FIXED(360), CLAY_SIZING_FIXED(12)}},
                        .backgroundColor = {0, 0, 0, 110},
                        .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
                        .floating = {
                            .offset = {0.0f, yOff + 42.0f},
                            .zIndex = 3,
                            .attachPoints = {CLAY_ATTACH_POINT_CENTER_TOP, CLAY_ATTACH_POINT_CENTER_TOP},
                            .attachTo = CLAY_ATTACH_TO_PARENT,
                        },
                    }
                )
                {
                    CLAY(
                        CLAY_ID("SchoolAimPullInner"),
                        {
                            .layout = {.sizing = {CLAY_SIZING_PERCENT(pull), CLAY_SIZING_GROW()}},
                            .backgroundColor = fill,
                            .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
                        }
                    )
                    {
                    }
                }
            }
        }
    }

    // Lesson 4 HUD (oil wear + re-oil progress)
    // Keep showing even after passing so the player can still see oil state while practicing.
    if (self->selectedLesson == 4)
    {
        oilRemaining01 = glm::clamp(oilRemaining01, 0.0f, 1.0f);
        if (oilReoilNeeded < 1)
            oilReoilNeeded = 3;
        oilReoilCount = glm::clamp(oilReoilCount, 0, oilReoilNeeded);
        float reoilFrac = (oilReoilNeeded > 0) ? (float)oilReoilCount / (float)oilReoilNeeded : 0.0f;
        reoilFrac = glm::clamp(reoilFrac, 0.0f, 1.0f);
        // Treat "passed" as session completion (re-oils this run), not the persistent unlock flag.
        // This lets the test HUD start fresh even if the player completed the lesson earlier.
        const bool passed = (oilReoilCount >= oilReoilNeeded);

        CLAY(
            CLAY_ID("SchoolOilTestHud"),
            {
                .layout = {
                    .sizing = {CLAY_SIZING_FIXED(255), CLAY_SIZING_FIT()},
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

            Clay_String title = ClayArena_FormatString(
                arena, Txl_Get(clayton->uiLanguage, TXL_OIL_TEST_REOILS_FMT), oilReoilCount, oilReoilNeeded
            );
            CLAY_TEXT(title, CLAY_TEXT_CONFIG(hudLabelCfg));
            if (passed)
                CLAY_TEXT(clayton->txl(TXL_PASSED), CLAY_TEXT_CONFIG(passedCfg));

            // Bar 1: remaining oil level (must wear down to re-oil)
            CLAY_TEXT(clayton->txl(TXL_REMAINING_OIL), CLAY_TEXT_CONFIG(hudLabelCfg));
            CLAY(
                CLAY_ID("SchoolOilRemainingOuter"),
                {
                    .layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(14)}},
                    .backgroundColor = {0, 0, 0, 120},
                    .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
                }
            )
            {
                CLAY(
                    CLAY_ID("SchoolOilRemainingInner"),
                    {
                        .layout = {.sizing = {CLAY_SIZING_PERCENT(oilRemaining01), CLAY_SIZING_GROW()}},
                        .backgroundColor = {255, 80, 80, 190},
                        .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
                    }
                )
                {
                }
            }

            // Bar 2: re-oil progress
            CLAY_TEXT(clayton->txl(TXL_REOIL_PROGRESS), CLAY_TEXT_CONFIG(hudLabelCfg));
            CLAY(
                CLAY_ID("SchoolOilReoilOuter"),
                {
                    .layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(14)}},
                    .backgroundColor = {0, 0, 0, 120},
                    .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
                }
            )
            {
                CLAY(
                    CLAY_ID("SchoolOilReoilInner"),
                    {
                        .layout = {.sizing = {CLAY_SIZING_PERCENT(reoilFrac), CLAY_SIZING_GROW()}},
                        .backgroundColor = {120, 200, 255, 200},
                        .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
                    }
                )
                {
                }
            }

            if (!passed && !oilCanReoilNow)
            {
                Clay_TextElementConfig warnCfg = hudLabelCfg;
                warnCfg.textColor = {255, 220, 120, 235};
                CLAY_TEXT(
                    clayton->txl(TXL_WEAR_IT_DOWN),
                    CLAY_TEXT_CONFIG(warnCfg)
                );
            }
        }
    }

    // Lesson 5 HUD (strike objective)
    // Keep showing even after passing so the player can still practice the line.
    if (self->selectedLesson == 5)
    {
        const bool passed = self->lessonDone[4];
        CLAY(
            CLAY_ID("SchoolStrikeTestHud"),
            {
                .layout = {
                    .sizing = {CLAY_SIZING_FIXED(250), CLAY_SIZING_FIT()},
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
            CLAY_TEXT(clayton->txl(TXL_STRIKE_TEST_SCORE_STRIKE), CLAY_TEXT_CONFIG(hudLabelCfg));
            if (passed)
                CLAY_TEXT(clayton->txl(TXL_PASSED), CLAY_TEXT_CONFIG(passedCfg));
        }
    }
}
