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
    bodyCfg.wrapMode = CLAY_TEXT_WRAP_WORDS;
    Clay_TextElementConfig priceCfg = {
        .textColor = {255, 215, 70, 255},
        .fontId = CLAY_FONT_NOTO,
        .fontSize = CLAY_FONT_SIZE_SM,
    };

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
                CLAY_TEXT(clayton->txl(TXL_OIL_STATUS), CLAY_TEXT_CONFIG(titleCfg));
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

            // Shared column headers keep their baseline, font, and column starts aligned.
            CLAY(
                CLAY_ID("OilStatusBody"),
                {
                    .layout =
                        {
                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                            .padding = {0, 0, 0, 0},
                            .childGap = 8,
                            .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_TOP},
                            .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        },
                }
            )
            {
                CLAY(
                    CLAY_ID("OilStatusColumnHeaders"),
                    {.layout = {
                        .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                        .childGap = 12,
                        .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT,
                    }}
                )
                {
                    CLAY(
                        CLAY_ID("OilStatusMapHeader"),
                        {.layout = {
                            .sizing = {CLAY_SIZING_FIXED(130), CLAY_SIZING_FIT()},
                            .padding = {.left = 10, .right = 0, .top = 0, .bottom = 0},
                        }}
                    )
                    {
                        CLAY_TEXT(clayton->txl(TXL_LANE_OIL_MAP), CLAY_TEXT_CONFIG(labelCfg));
                    }
                    CLAY(
                        CLAY_ID("OilStatusTrackHeader"),
                        {.layout = {
                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                            .padding = {.left = 10, .right = 0, .top = 0, .bottom = 0},
                        }}
                    )
                    {
                        CLAY_TEXT(clayton->txl(TXL_TRACK_INFO), CLAY_TEXT_CONFIG(labelCfg));
                    }
                }

                // Map and track columns sit directly below the shared headers.
                CLAY(
                    CLAY_ID("OilStatusColumns"),
                    {.layout = {
                        .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                        .childGap = 12,
                        .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_TOP},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT,
                    }}
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
                        const float houseT01 = oilStatus ? oilStatus->houseOilThickness : 0.0f;
                        const float curT01 = oilStatus ? oilStatus->currentOilThickness : 0.0f;
                        const float laneFriction = oilStatus ? oilStatus->laneFriction : 0.0f;
                        const float lanePushback = oilStatus ? oilStatus->lanePushbackStrength : 0.0f;
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
                        const float pushback01 = glm::clamp(
                            lanePushback * glm::clamp(curT01, 0.0f, 1.0f) / 50.0f,
                            0.0f,
                            1.0f
                        );

                        // Keep the values in one fixed-width, right-aligned column.
                        // 112 px accommodates the longest supported formatted value
                        // (for example, "0.0000m/m") while leaving the label column
                        // flexible for every locale.
                        constexpr float OIL_VALUE_COLUMN_WIDTH = 112.0f;
                        Clay_TextElementConfig metricLabelCfg = CLAY_THEME_TEXT_BODY;
                        auto metricRow = [&](Clay_ElementId id, Clay_String label, Clay_String value)
                        {
                            CLAY(
                                id,
                                {.layout = {
                                    .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                                    .childGap = 8,
                                    .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                                }}
                            )
                            {
                                CLAY(
                                    CLAY_IDI("OilMetricLabel", id.id),
                                    {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()}}}
                                )
                                {
                                    CLAY_TEXT(label, CLAY_TEXT_CONFIG(metricLabelCfg));
                                }
                                CLAY(
                                    CLAY_IDI("OilMetricValue", id.id),
                                    {.layout = {
                                        .sizing = {CLAY_SIZING_FIXED(OIL_VALUE_COLUMN_WIDTH), CLAY_SIZING_FIT()},
                                        .childAlignment = {CLAY_ALIGN_X_RIGHT, CLAY_ALIGN_Y_CENTER},
                                    }}
                                )
                                {
                                    CLAY_TEXT(value, CLAY_TEXT_CONFIG(bodyCfg));
                                }
                            }
                        };
                        metricRow(
                            CLAY_ID("OilMetricMax"),
                            clayton->txl(TXL_MAX_OIL_LEVEL),
                            ClayArena_FormatString(&clayton->clayArena, "%.1fmm", maxOilMm)
                        );
                        metricRow(
                            CLAY_ID("OilMetricCurrent"),
                            clayton->txl(TXL_CURRENT_OIL_LEVEL),
                            ClayArena_FormatString(&clayton->clayArena, "%.1fmm", curOilMm)
                        );
                        metricRow(
                            CLAY_ID("OilMetricCarrydown"),
                            clayton->txl(TXL_CARRYDOWN),
                            ClayArena_FormatString(&clayton->clayArena, "%.3fm/m", carryPerM)
                        );
                        metricRow(
                            CLAY_ID("OilMetricDecay"),
                            clayton->txl(TXL_OIL_DECAY),
                            ClayArena_FormatString(&clayton->clayArena, "%.4f/m", decayPerM)
                        );

                        // A shared label column keeps every indicator bar aligned and equal-width.
                        constexpr float OIL_INDICATOR_LABEL_WIDTH = 112.0f;

                        // Bar 1: Oil remaining vs track max
                        CLAY(CLAY_ID("OilBarRow"), CLAY_THEME_STAT_ROW)
                        {
                            CLAY(
                                CLAY_ID("OilBarLabel"),
                                {.layout = {.sizing = {CLAY_SIZING_FIXED(OIL_INDICATOR_LABEL_WIDTH), CLAY_SIZING_FIT()}}}
                            )
                            {
                                CLAY_TEXT(clayton->txl(TXL_OIL), CLAY_TEXT_CONFIG(labelCfg));
                            }
                            CLAY(CLAY_ID("OilBarBg"), CLAY_THEME_STAT_BAR_BG)
                            {
                                CLAY(CLAY_ID("OilBarFill"), CLAY_THEME_STAT_BAR_FILL(oilFill01)) {}
                            }
                        }

                        // Bar 2: Surface slipperiness (inverse friction)
                        CLAY(CLAY_ID("SlipBarRow"), CLAY_THEME_STAT_ROW)
                        {
                            CLAY(
                                CLAY_ID("SlipBarLabel"),
                                {.layout = {.sizing = {CLAY_SIZING_FIXED(OIL_INDICATOR_LABEL_WIDTH), CLAY_SIZING_FIT()}}}
                            )
                            {
                                CLAY_TEXT(clayton->txl(TXL_SLIPPERY), CLAY_TEXT_CONFIG(labelCfg));
                            }
                            CLAY(CLAY_ID("SlipBarBg"), CLAY_THEME_STAT_BAR_BG)
                            {
                                Clay_ElementDeclaration slipFill = CLAY_THEME_STAT_BAR_FILL(slip01);
                                slipFill.backgroundColor = {62, 218, 238, 255};
                                CLAY(CLAY_ID("SlipBarFill"), slipFill) {}
                            }
                        }

                        // Bar 3: the current oil-powered correction toward lane center.
                        CLAY(CLAY_ID("PushbackBarRow"), CLAY_THEME_STAT_ROW)
                        {
                            CLAY(
                                CLAY_ID("PushbackBarLabel"),
                                {.layout = {.sizing = {CLAY_SIZING_FIXED(OIL_INDICATOR_LABEL_WIDTH), CLAY_SIZING_FIT()}}}
                            )
                            {
                                CLAY_TEXT(clayton->txl(TXL_PUSHBACK), CLAY_TEXT_CONFIG(labelCfg));
                            }
                            CLAY(CLAY_ID("PushbackBarBg"), CLAY_THEME_STAT_BAR_BG)
                            {
                                Clay_ElementDeclaration pushbackFill = CLAY_THEME_STAT_BAR_FILL(pushback01);
                                pushbackFill.backgroundColor = {158, 92, 232, 255};
                                CLAY(CLAY_ID("PushbackBarFill"), pushbackFill) {}
                            }
                        }
                    }

                    if (oilStatus && oilStatus->lessonReoilNeeded > 0)
                    {
                        CLAY_TEXT(
                            ClayArena_FormatString(
                                &clayton->clayArena,
                                Txl_Get(clayton->uiLanguage, TXL_REOILS_FMT),
                                oilStatus->lessonReoilCount,
                                oilStatus->lessonReoilNeeded
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
                        if (canAfford && reoilEnabled)
                        {
                            // Keep the shop-standard 40 px button height, but use a
                            // full-width row so the action is left-aligned and its cost
                            // stays visibly paired with it on the right.
                            Clay_ElementDeclaration reoilButton = CLAY_THEME_BTN_BUY;
                            reoilButton.layout.sizing.height = CLAY_SIZING_FIXED(60);
                            reoilButton.layout.childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER};
                            reoilButton.layout.padding.left = 12;
                            reoilButton.layout.padding.right = 12;
                            CLAY(clayton->oilReoilClick.clayId, reoilButton)
                            {
                                CLAY(
                                    CLAY_ID("OilReoilButtonContents"),
                                    {.layout = {
                                        .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                        .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                                        .layoutDirection = CLAY_LEFT_TO_RIGHT,
                                    }}
                                )
                                {
                                    CLAY_TEXT(clayton->txl(TXL_REOIL), CLAY_TEXT_CONFIG(buttonCfg));
                                    CLAY(CLAY_ID("OilReoilPriceSpacer"), {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1)}}}) {}
                                    if (!isFree)
                                    {
                                        CLAY_TEXT(
                                            ClayArena_FormatString(&clayton->clayArena, "$%.0f", REOIL_COST),
                                            CLAY_TEXT_CONFIG(priceCfg)
                                        );
                                    }
                                }
                            }
                        }
                        else
                        {
                            // Keep unavailable re-oil controls gray and price-free, while
                            // matching the active action and close-button height.
                            Clay_ElementDeclaration disabledReoilButton = CLAY_THEME_BTN_BUY_DISABLED;
                            disabledReoilButton.layout.sizing.height = CLAY_SIZING_FIXED(60);
                            CLAY(CLAY_ID("OilReoilDisabled"), disabledReoilButton)
                            {
                                const char *label = Txl_Get(clayton->uiLanguage, TXL_CANT_AFFORD);
                                if (!reoilEnabled)
                                {
                                    label = (oilStatus && oilStatus->reoilDisabledLabel)
                                        ? oilStatus->reoilDisabledLabel
                                        : Txl_Get(clayton->uiLanguage, TXL_REOIL_LOCKED);
                                }
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
}
