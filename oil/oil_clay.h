#pragma once

#include "../clayton/clayton.h"
#include "oil_status.h"

inline void buildOilStatusWindowClay(Clayton *clayton, float bank, const OilStatusUI *oilStatus)
{
    if (!clayton || !clayton->shouldShowOilStatus)
        return;

    Clay_TextElementConfig titleCfg = CLAY_THEME_TEXT_TITLE;
    Clay_TextElementConfig buttonCfg = CLAY_THEME_TEXT_BUTTON;
    Clay_TextElementConfig labelCfg = CLAY_THEME_TEXT_LABEL;
    Clay_TextElementConfig bodyCfg = CLAY_THEME_TEXT_BODY;

    const float REOIL_COST = 10.0f;
    const bool canAfford = bank >= REOIL_COST;
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
                CLAY_TEXT(CLAY_STRING("🛢 Oil Status"), CLAY_TEXT_CONFIG(titleCfg));
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

            // Body: left map + right info
            CLAY(
                CLAY_ID("OilStatusBody"),
                {
                    .layout =
                        {
                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(340)},
                            .padding = {10, 10, 10, 10},
                            .childGap = 12,
                            .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        },
                }
            )
            {
                // Left: map preview
                CLAY(CLAY_ID("OilStatusLeft"), CLAY_THEME_SECTION)
                {
                    CLAY_TEXT(CLAY_STRING("Lane Oil Map"), CLAY_TEXT_CONFIG(labelCfg));
                    CLAY(
                        CLAY_ID("OilStatusPreviewImage"),
                        {
                            .layout =
                                {.sizing =
                                     {.width = CLAY_SIZING_FIXED(220),
                                      .height = CLAY_SIZING_FIXED(300)}},
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
                    CLAY(CLAY_ID("OilStatusInfo"), CLAY_THEME_SECTION)
                    {
                        CLAY_TEXT(CLAY_STRING("Current Lane"), CLAY_TEXT_CONFIG(labelCfg));

                        const float laneFriction = oilStatus ? oilStatus->laneFriction : 0.0f;
                        const float lanePush = oilStatus ? oilStatus->lanePushbackStrength : 0.0f;
                        const float houseT = oilStatus ? oilStatus->houseOilThickness : 0.0f;
                        const float curT = oilStatus ? oilStatus->currentOilThickness : 0.0f;

                        CLAY_TEXT(
                            ClayArena_FormatString(
                                &clayton->clayArena,
                                "Lane friction: %.3f\nPushback strength: %.3f\nOil thickness (house/current): %.2f / %.2f",
                                laneFriction,
                                lanePush,
                                houseT,
                                curT
                            ),
                            CLAY_TEXT_CONFIG(bodyCfg)
                        );
                    }

                    CLAY(CLAY_ID("OilStatusWear"), CLAY_THEME_SECTION)
                    {
                        CLAY_TEXT(CLAY_STRING("Wear / Carrydown"), CLAY_TEXT_CONFIG(labelCfg));

                        const float wl = oilStatus ? oilStatus->oilWearLeftM : 0.0f;
                        const float wr = oilStatus ? oilStatus->oilWearRightM : 0.0f;
                        const float wt = oilStatus ? oilStatus->oilWearTotalM : 0.0f;
                        const float cd = oilStatus ? oilStatus->oilCarrydownPerBallTravelM : 0.0f;
                        const float dec = oilStatus ? oilStatus->oilThicknessDecayPerBallTravel : 0.0f;

                        const float estCL = oilStatus ? oilStatus->estCarryStartLeftM : 0.0f;
                        const float estCR = oilStatus ? oilStatus->estCarryStartRightM : 0.0f;
                        const float estDrop = oilStatus ? oilStatus->estThicknessDrop : 0.0f;

                        CLAY_TEXT(
                            ClayArena_FormatString(
                                &clayton->clayArena,
                                "Wear (L/R/Total): %.2f / %.2f / %.2f m\nCarrydown / m: %.3f\nThickness decay / m: %.4f\nEst. carry start (L/R): %.2f / %.2f\nEst. thickness drop: %.3f",
                                wl,
                                wr,
                                wt,
                                cd,
                                dec,
                                estCL,
                                estCR,
                                estDrop
                            ),
                            CLAY_TEXT_CONFIG(bodyCfg)
                        );
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
                        CLAY_TEXT(
                            ClayArena_FormatString(
                                &clayton->clayArena,
                                "Re-oil cost: $%.0f  (you have: $%.0f)",
                                REOIL_COST,
                                bank
                            ),
                            CLAY_TEXT_CONFIG(bodyCfg)
                        );

                        if (canAfford)
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
                                CLAY_TEXT(CLAY_STRING("CAN'T AFFORD"), CLAY_TEXT_CONFIG(disabledCfg));
                            }
                        }
                    }
                }
            }
        }
    }
}

