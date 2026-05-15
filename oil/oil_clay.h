#pragma once

#include "../clayton/clayton.h"
#include "oil_status.h"
#include <glm/glm.hpp>

inline void buildOilStatusWindowClay(Clayton *clayton, float bank, const OilStatusUI *oilStatus)
{
    if (!clayton || !clayton->shouldShowOilStatus)
        return;

    Clay_TextElementConfig titleCfg = CLAY_THEME_TEXT_TITLE;
    Clay_TextElementConfig buttonCfg = CLAY_THEME_TEXT_BUTTON;
    Clay_TextElementConfig labelCfg = CLAY_THEME_TEXT_LABEL;
    Clay_TextElementConfig bodyCfg = CLAY_THEME_TEXT_BODY;

    const float REOIL_COST = oilStatus ? oilStatus->reoilCost : 10.0f;
    const bool isFree = REOIL_COST <= 0.001f;
    const bool canAfford = isFree || (bank >= REOIL_COST);
    const bool reoilEnabled = oilStatus ? oilStatus->reoilEnabled : true;
    Clay_TextElementConfig disabledCfg = {
        .textColor = CLAY_COLOR_TEXT_SECONDARY,
        .fontId = CLAY_FONT_NOTO,
        .fontSize = CLAY_FONT_SIZE_SM,
    };

    CLAY(
        CLAY_ID("OilStatusContainer"),
        {
            .layout =
                {
                    .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                    .padding = {0, 0, 0, 0},
                    .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                },
        }
    )
    {
        CLAY(CLAY_ID("OilStatusWindow"), CLAY_THEME_WINDOW_PANEL)
        {
            // Title row
            CLAY(
                CLAY_ID("OilStatusTitle"),
                {
                    .layout =
                        {
                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                            .padding = {0, 0, 5, 0},
                            .childGap = 10,
                            .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        },
                }
            )
            {
                CLAY_TEXT(CLAY_STRING("Oil Status"), CLAY_TEXT_CONFIG(titleCfg));
                CLAY(
                    CLAY_ID("OilStatusTitleDivider"),
                    {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1)}}}
                )
                {
                }
                CLAY(clayton->oilStatusCloseClick.clayId, CLAY_THEME_BTN_DANGER)
                {
                    CLAY_TEXT(CLAY_STRING("x"), CLAY_TEXT_CONFIG(buttonCfg));
                }
            }

            // Body: left tall map column + right info
            CLAY(
                CLAY_ID("OilStatusBody"),
                {
                    .layout =
                        {
                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                            .padding = {10, 10, 10, 10},
                            .childGap = 12,
                            .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_TOP},
                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        },
                }
            )
            {
                // Left: map preview
                CLAY(
                    CLAY_ID("OilStatusLeft"),
                    {
                        .layout =
                            {
                                .sizing = {CLAY_SIZING_FIXED(130), CLAY_SIZING_GROW()},
                                .padding = {10, 10, 10, 10},
                                .childGap = 10,
                                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                            },
                        .backgroundColor = CLAY_COLOR_PANEL_SECTION,
                        .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
                    }
                )
                {
                    CLAY_TEXT(CLAY_STRING("Lane Oil Map"), CLAY_TEXT_CONFIG(labelCfg));
                    CLAY(
                        CLAY_ID("OilStatusPreviewImage"),
                        {
                            .layout =
                                {.sizing =
                                     {// 9:32 aspect (narrow + tall)
                                      .width = CLAY_SIZING_FIXED(90),
                                      .height = CLAY_SIZING_FIXED(320)}},
                            .image = {.imageData = &clayton->oilImage},
                        }
                    )
                    {
                    }
                }

                // Right: data + actions
                CLAY(
                    CLAY_ID("OilStatusRight"),
                    {
                        .layout =
                            {
                                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                .childGap = 12,
                                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                            },
                    }
                )
                {
                    CLAY(CLAY_ID("OilStatusTrack"), CLAY_THEME_SECTION)
                    {
                        CLAY_TEXT(CLAY_STRING("Track Info"), CLAY_TEXT_CONFIG(labelCfg));

                        const float houseT01 = oilStatus ? oilStatus->houseOilThickness : 0.0f;
                        const float curT01 = oilStatus ? oilStatus->currentOilThickness : 0.0f;
                        const float laneFriction = oilStatus ? oilStatus->laneFriction : 0.0f;
                        const float carryPerM = oilStatus ? oilStatus->oilCarrydownPerBallTravelM : 0.0f;
                        const float decayPerM = oilStatus ? oilStatus->oilThicknessDecayPerBallTravel : 0.0f;

                        // Interpret oil thickness 0..1 as 0..MAX_OIL_MM. House thickness scales max for this track.
                        const float MAX_OIL_MM = 3.0f;
                        const float maxOilMm = MAX_OIL_MM * glm::clamp(houseT01, 0.0f, 1.0f);
                        const float curOilMm = MAX_OIL_MM * glm::clamp(curT01, 0.0f, 1.0f);
                        const float oilFill01 = (houseT01 > 1e-6f) ? glm::clamp(curT01 / houseT01, 0.0f, 1.0f) : 0.0f;

                        // Slipperiness is the inverse of lane friction (0..0.15 assumed as the tunable range).
                        const float FRICTION_MAX = 0.15f;
                        const float slip01 = 1.0f - glm::clamp(laneFriction / glm::max(1e-6f, FRICTION_MAX), 0.0f, 1.0f);

                        CLAY_TEXT(
                            ClayArena_FormatString(
                                &clayton->clayArena,
                                "Max oil level: %.1fmm\nCurrent: %.1fmm\nCarrydown: %.3fm/m\nDecay: %.4f/m",
                                maxOilMm,
                                curOilMm,
                                carryPerM,
                                decayPerM
                            ),
                            CLAY_TEXT_CONFIG(bodyCfg)
                        );

                        // Bar 1: Oil remaining vs track max
                        CLAY(CLAY_ID("OilBarRow"), CLAY_THEME_STAT_ROW)
                        {
                            CLAY_TEXT(CLAY_STRING("Oil"), CLAY_TEXT_CONFIG(labelCfg));
                            CLAY(CLAY_ID("OilBarBg"), CLAY_THEME_STAT_BAR_BG)
                            {
                                CLAY(CLAY_ID("OilBarFill"), CLAY_THEME_STAT_BAR_FILL(oilFill01)) {}
                            }
                        }

                        // Bar 2: Surface slipperiness (inverse friction)
                        CLAY(CLAY_ID("SlipBarRow"), CLAY_THEME_STAT_ROW)
                        {
                            CLAY_TEXT(CLAY_STRING("Slippery"), CLAY_TEXT_CONFIG(labelCfg));
                            CLAY(CLAY_ID("SlipBarBg"), CLAY_THEME_STAT_BAR_BG)
                            {
                                CLAY(CLAY_ID("SlipBarFill"), CLAY_THEME_STAT_BAR_FILL(slip01)) {}
                            }
                        }
                    }

                    if (isFree)
                    {
                        CLAY_TEXT(CLAY_STRING("Re-oil: FREE"), CLAY_TEXT_CONFIG(bodyCfg));
                    }
                    else
                    {
                        CLAY_TEXT(
                            ClayArena_FormatString(
                                &clayton->clayArena,
                                "Re-oil cost: $%.0f  (you have: $%.0f)",
                                REOIL_COST,
                                bank
                            ),
                            CLAY_TEXT_CONFIG(bodyCfg)
                        );
                    }

                    if (oilStatus && oilStatus->lessonReoilNeeded > 0)
                    {
                        CLAY_TEXT(
                            ClayArena_FormatString(
                                &clayton->clayArena,
                                "Re-oils: %d/%d",
                                oilStatus->lessonReoilCount,
                                oilStatus->lessonReoilNeeded
                            ),
                            CLAY_TEXT_CONFIG(bodyCfg)
                        );
                    }

                    if (oilStatus && !oilStatus->reoilEnabled && oilStatus->reoilDisabledLabel)
                    {
                        Clay_String msg = ClayArena_AllocString(&clayton->clayArena, oilStatus->reoilDisabledLabel);
                        CLAY_TEXT(msg, CLAY_TEXT_CONFIG(bodyCfg));
                    }

                    CLAY(
                        CLAY_ID("OilStatusActions"),
                        {
                            .layout =
                                {
                                    .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                                    .padding = {0, 0, 0, 0},
                                    .childGap = 10,
                                    .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                                },
                        }
                    )
                    {
                        if (canAfford && reoilEnabled)
                        {
                            CLAY(clayton->oilReoilClick.clayId, CLAY_THEME_BTN_BUY)
                            {
                                CLAY_TEXT(CLAY_STRING("RE-OIL"), CLAY_TEXT_CONFIG(buttonCfg));
                            }
                        }
                        else
                        {
                            CLAY(CLAY_ID("OilReoilDisabled"), CLAY_THEME_BTN_BUY_DISABLED)
                            {
                                const char *label = "CAN'T AFFORD";
                                if (!reoilEnabled)
                                    label = "RE-OIL LOCKED";
                                Clay_String btnMsg = ClayArena_AllocString(&clayton->clayArena, label);
                                CLAY_TEXT(btnMsg, CLAY_TEXT_CONFIG(disabledCfg));
                            }
                        }
                    }
                }
            }
        }
    }
}
