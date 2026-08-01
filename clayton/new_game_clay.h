#pragma once

#include <cstdio>

#include "./clayton.h"
#include "./claytheme.h"

static inline void NewGame_RenderBigScore(
    Clayton *clayton,
    int id,
    int score,
    const char *label,
    Clay_Color bg,
    Clay_Color scoreColor)
{
    Clay_TextElementConfig scoreCfg = {
        .textColor = scoreColor,
        .fontId = CLAY_FONT_NOTO,
        .fontSize = 64,
    };
    Clay_TextElementConfig labelCfg = {
        .textColor = CLAY_COLOR_TEXT_SECONDARY,
        .fontId = CLAY_FONT_NOTO,
        .fontSize = 16,
    };
    Clay_String scoreText = ClayArena_FormatString(&clayton->clayArena, "%d", score);
    Clay_String labelText = ClayArena_FormatString(&clayton->clayArena, "%s", label ? label : "");
    CLAY(
        CLAY_IDI("NewGameBigScore", id),
        {
            .layout = {
                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                .padding = {14, 14, 10, 10},
                .childGap = 8,
                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
            .backgroundColor = bg,
            .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
            .border = {.color = {160, 120, 220, 160}, .width = CLAY_BORDER_ALL(2)},
        }
    )
    {
        CLAY_TEXT(scoreText, CLAY_TEXT_CONFIG(scoreCfg));
        CLAY(
            CLAY_IDI("NewGameBigScoreLabelBox", id),
            {
                .layout = {
                    .sizing = {CLAY_SIZING_FIT(), CLAY_SIZING_GROW()},
                    .padding = {0, 0, 5, 5},
                    .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_BOTTOM},
                },
            }
        )
        {
            CLAY_TEXT(labelText, CLAY_TEXT_CONFIG(labelCfg));
        }
    }
}

static inline void NewGame_RenderScoreDuel(Clayton *clayton)
{
    if (!clayton || !clayton->newGameShowScores)
        return;

    const Clay_Color playerBg = {38, 66, 110, 225};
    const Clay_Color opponentBg = {95, 42, 52, 225};

    CLAY(
        CLAY_ID("NewGameScoreDuel"),
        {
            .layout = {
                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                .childGap = 12,
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
            },
        }
    )
    {
        NewGame_RenderBigScore(
            clayton,
            0,
            clayton->newGamePlayerTotal,
            "you",
            playerBg,
            {205, 230, 255, 255}
        );
        if (clayton->newGameShowOpponent)
        {
            NewGame_RenderBigScore(
                clayton,
                1,
                clayton->newGameOpponentTotal,
                clayton->newGameOpponentLabel,
                opponentBg,
                {255, 210, 220, 255}
            );
        }
    }
}

static inline void NewGame_RenderMoneyRow(
    Clayton *clayton,
    int id,
    const char *name,
    const char *formula,
    int amount,
    bool totalRow = false)
{
    Clay_TextElementConfig labelCfg = CLAY_THEME_TEXT_BODY;
    Clay_TextElementConfig formulaCfg = CLAY_THEME_TEXT_LABEL;
    Clay_TextElementConfig amountCfg = CLAY_THEME_TEXT_BODY;
    if (totalRow)
    {
        labelCfg.fontSize = 22;
        amountCfg.fontSize = 28;
        amountCfg.textColor = {255, 222, 45, 255};
    }
    Clay_String nameText = ClayArena_FormatString(&clayton->clayArena, "%s", name ? name : "");
    Clay_String formulaText = ClayArena_FormatString(&clayton->clayArena, "%s", formula ? formula : "");
    Clay_String amountText = ClayArena_FormatString(&clayton->clayArena, "%d", amount);
    CLAY(
        CLAY_IDI("NewGameMoneyRow", id),
        {
            .layout = {
                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(totalRow ? 38.0f : 30.0f)},
                .childGap = 8,
                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
            },
        }
    )
    {
        CLAY(
            CLAY_IDI("NewGameMoneyName", id),
            {
                .layout = {
                    .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                    .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                },
            }
        )
        {
            CLAY_TEXT(nameText, CLAY_TEXT_CONFIG(labelCfg));
        }
        CLAY(
            CLAY_IDI("NewGameMoneyFormula", id),
            {
                .layout = {
                    .sizing = {CLAY_SIZING_FIXED(90), CLAY_SIZING_GROW()},
                    .childAlignment = {CLAY_ALIGN_X_RIGHT, CLAY_ALIGN_Y_CENTER},
                },
            }
        )
        {
            CLAY_TEXT(formulaText, CLAY_TEXT_CONFIG(formulaCfg));
        }
        CLAY(
            CLAY_IDI("NewGameMoneyAmount", id),
            {
                .layout = {
                    .sizing = {CLAY_SIZING_FIXED(76), CLAY_SIZING_GROW()},
                    .childAlignment = {CLAY_ALIGN_X_RIGHT, CLAY_ALIGN_Y_CENTER},
                },
            }
        )
        {
            CLAY_TEXT(amountText, CLAY_TEXT_CONFIG(amountCfg));
        }
    }
}

static inline void NewGame_RenderMoneyBreakdown(Clayton *clayton)
{
    if (!clayton || !clayton->newGameShowMoneyBreakdown)
        return;

    char strikesFormula[24];
    char sparesFormula[24];
    snprintf(strikesFormula, sizeof(strikesFormula), "%d x 10", clayton->newGameRoundStrikeCount);
    snprintf(sparesFormula, sizeof(sparesFormula), "%d x 5", clayton->newGameRoundSpareCount);

    CLAY(
        CLAY_ID("NewGameMoneyBreakdown"),
        {
            .layout = {
                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                .padding = {14, 14, 12, 12},
                .childGap = 4,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
            .backgroundColor = {18, 12, 30, 190},
            .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
            .border = {.color = {160, 120, 220, 135}, .width = CLAY_BORDER_ALL(1)},
        }
    )
    {
        NewGame_RenderMoneyRow(
            clayton,
            0,
            "Strikes",
            strikesFormula,
            clayton->newGameRoundStrikeCount * 10
        );
        NewGame_RenderMoneyRow(
            clayton,
            1,
            "Spares",
            sparesFormula,
            clayton->newGameRoundSpareCount * 5
        );
        NewGame_RenderMoneyRow(clayton, 2, "Coins", "", clayton->newGameRoundCoins);
        if (clayton->newGameRoundWinByPoints > 0)
            NewGame_RenderMoneyRow(clayton, 3, "Won by points", "", clayton->newGameRoundWinByPoints);

        CLAY(
            CLAY_ID("NewGameMoneyDivider"),
            {
                .layout = {
                    .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(0)},
                    .padding = {0, 0, 0, 0},
                },
                .backgroundColor = {160, 120, 220, 120},
            }
        )
        {
        }
        NewGame_RenderMoneyRow(clayton, 4, "Total", "", clayton->newGameRoundMoneyTotal, true);
    }
}

inline void renderNewGameWindow(Clayton *clayton)
{
    Clay_TextElementConfig titleCfg = CLAY_THEME_TEXT_TITLE;
    Clay_TextElementConfig detailCfg = CLAY_THEME_TEXT_BODY;
    Clay_TextElementConfig buttonCfg = CLAY_THEME_TEXT_BUTTON;
    ClayArena *arena = &clayton->clayArena;

    const bool isResult = clayton->newGameIsResult;
    const Clay_Color outcomeColor = clayton->newGameVictory
        ? (Clay_Color){90, 240, 160, 255}
        : (Clay_Color){255, 90, 120, 255};
    if (isResult)
    {
        titleCfg.fontSize = 40;
        titleCfg.textColor = outcomeColor;
    }

    Clay_String title = ClayArena_FormatString(arena, "%s", clayton->newGameTitle ? clayton->newGameTitle : "TRY AGAIN");
    Clay_String detail = ClayArena_FormatString(arena, "%s", clayton->newGameDetail ? clayton->newGameDetail : "");
    Clay_String button = ClayArena_FormatString(
        arena,
        "%s",
        clayton->newGameButtonLabel ? clayton->newGameButtonLabel : "TRY AGAIN"
    );
    Clay_String coinsLabel = ClayArena_FormatString(arena, "COINS OBTAINED");
    Clay_String coinsAmount = ClayArena_FormatString(arena, "$ %d", clayton->newGameCoinsAnimated);
    Clay_String shopLabel = ClayArena_FormatString(arena, "SHOP");
    Clay_String reloadLabel = ClayArena_FormatString(
        arena,
        "Reloads in %s",
        clayton->newGameShopReloadText[0] ? clayton->newGameShopReloadText : "RESTOCK SOON"
    );

    // Container exists for pointer-hit testing in WindowStack.
    CLAY(
        CLAY_ID("NewGameWindowContainer"),
        {
            .layout = {
                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                .padding = {0, 0, 0, 0},
                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
        }
    )
    {
        CLAY(
            CLAY_ID("NewGameWindow"),
            {
                .layout = {
                    .sizing = {CLAY_SIZING_PERCENT(0.88f), CLAY_SIZING_FIT()},
                    .padding = {20, 20, 20, 20},
                    .childGap = 16,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
                .backgroundColor = CLAY_COLOR_PANEL_BG,
                .cornerRadius = {CLAY_RADIUS_XL, CLAY_RADIUS_XL, CLAY_RADIUS_XL, CLAY_RADIUS_XL},
                CLAY_THEME_WINDOW_BORDER
            }
        )
        {
            CLAY(
                CLAY_ID("NewGameHeader"),
                {
                    .layout = {
                        .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                        .padding = {10, 10, 8, 8},
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                    },
                    .backgroundColor = isResult ? (Clay_Color){20, 12, 32, 120} : CLAY_COLOR_PANEL_BG,
                    .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
                }
            )
            {
                CLAY_TEXT(title, CLAY_TEXT_CONFIG(titleCfg));
            }

            if (detail.length > 0)
            {
                CLAY(
                    CLAY_ID("NewGameDetail"),
                    {
                        .layout = {
                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                            .padding = {10, 10, 8, 8},
                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                        },
                    }
                )
                {
                    CLAY_TEXT(detail, CLAY_TEXT_CONFIG(detailCfg));
                }
            }

            NewGame_RenderScoreDuel(clayton);
            NewGame_RenderMoneyBreakdown(clayton);

            if (isResult)
            {
                CLAY(
                    CLAY_ID("NewGameCoinShopRow"),
                    {
                        .layout = {
                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                            .childGap = 12,
                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        },
                    }
                )
                {
                    CLAY(
                        CLAY_ID("NewGameCoinsPanel"),
                        {
                            .layout = {
                                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(82)},
                                .padding = {14, 14, 8, 8},
                                .childGap = 4,
                                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                            },
                            .backgroundColor = {35, 22, 52, 220},
                            .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
                            .border = {.color = outcomeColor, .width = CLAY_BORDER_OUTSIDE(2)},
                        }
                    )
                    {
                        Clay_TextElementConfig small = CLAY_THEME_TEXT_LABEL;
                        small.fontSize = 14;
                        CLAY_TEXT(coinsLabel, CLAY_TEXT_CONFIG(small));
                        Clay_TextElementConfig coinsCfg = CLAY_THEME_TEXT_LARGE;
                        coinsCfg.textColor = {255, 222, 45, 255};
                        coinsCfg.fontSize = 36;
                        CLAY_TEXT(coinsAmount, CLAY_TEXT_CONFIG(coinsCfg));
                    }

                    CLAY(
                        CLAY_ID("NewGameShopPanel"),
                        {
                            .layout = {
                                .sizing = {CLAY_SIZING_PERCENT(0.34f), CLAY_SIZING_FIXED(82)},
                                .childGap = 6,
                                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                            },
                        }
                    )
                    {
                        CLAY(
                            clayton->newGameShopClick.clayId,
                            {
                                .layout = {
                                    .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(46)},
                                    .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                },
                                .backgroundColor = CLAY_THEME_HOVER_COLOR(CLAY_COLOR_BTN_PRIMARY, 24.0f, 0.0f),
                                .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
                            }
                        )
                        {
                            CLAY_TEXT(shopLabel, CLAY_TEXT_CONFIG(buttonCfg));
                        }
                        CLAY(
                            CLAY_ID("NewGameShopReload"),
                            {
                                .layout = {
                                    .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(30)},
                                    .padding = {6, 6, 0, 0},
                                    .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                },
                                .backgroundColor = {25, 14, 38, 180},
                                .cornerRadius = {CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD},
                                .border = {.color = {160, 120, 220, 170}, .width = CLAY_BORDER_ALL(1)},
                            }
                        )
                        {
                            Clay_TextElementConfig reloadCfg = CLAY_THEME_TEXT_BODY;
                            reloadCfg.fontSize = 14;
                            CLAY_TEXT(reloadLabel, CLAY_TEXT_CONFIG(reloadCfg));
                        }
                    }
                }
            }

            CLAY(
                clayton->playAgainClick.clayId,
                {
                    .layout = {
                        .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(64)},
                        .padding = {20, 20, 0, 0},
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                    },
                    .backgroundColor = CLAY_THEME_HOVER_COLOR(
                        (!isResult || clayton->newGameVictory) ? CLAY_COLOR_BTN_SUCCESS : CLAY_COLOR_BTN_DANGER,
                        22.0f,
                        0.0f
                    ),
                    .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
                }
            )
            {
                CLAY_TEXT(button, CLAY_TEXT_CONFIG(buttonCfg));
            }
        }
    }
}
