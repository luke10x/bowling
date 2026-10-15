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
            Txl_Get(clayton->uiLanguage, TXL_YOU),
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

static inline void NewGame_RenderScoreboardRow(Clayton *clayton, int role)
{
    const BowlingScoreboard &board = role == 0
        ? clayton->newGamePlayerScoreboard : clayton->newGameOpponentScoreboard;
    const int *scores = role == 0
        ? clayton->newGamePlayerFrameScores : clayton->newGameOpponentFrameScores;
    Clay_TextElementConfig textCfg = CLAY_THEME_TEXT_BODY;
    textCfg.fontSize = 14;
    textCfg.wrapMode = CLAY_TEXT_WRAP_NONE;
    Clay_TextElementConfig rollCfg = textCfg;
    rollCfg.fontSize = 12;
    CLAY(CLAY_IDI("ResultScoreboardRow", role),
         {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                     .childGap = 1, .layoutDirection = CLAY_LEFT_TO_RIGHT}})
    {
        for (int i = 0; i < 10; ++i)
        {
            const Frame &frame = board.frames[i];
            char first[16], second[16], third[16];
            Clay_String r1 = Clayton::rollSymbol2(first, frame.roll1, 0, frame.isStrike, frame.isSpare, false);
            Clay_String r2 = i < 9 && frame.isStrike ? CLAY_STRING("")
                : Clayton::rollSymbol2(second, frame.roll2, frame.roll1, frame.isStrike, frame.isSpare, true);
            Clay_String r3 = i == 9
                ? Clayton::rollSymbol2(third, frame.roll3, frame.roll2, frame.roll3 == 10,
                    frame.roll1 == 10 && frame.roll2 >= 0 && frame.roll2 < 10 && frame.roll2 + frame.roll3 == 10, true)
                : CLAY_STRING("");
            Clay_String score = scores[i] < 0 ? CLAY_STRING("")
                : ClayArena_FormatString(&clayton->clayArena, "%d", scores[i]);
            CLAY(CLAY_IDI("ResultScoreboardFrame", role * 10 + i),
                 {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIT()},
                             .padding = {1, 1, 4, 4}, .childGap = 2,
                             .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                             .layoutDirection = CLAY_TOP_TO_BOTTOM},
                  .backgroundColor = role == 0 ? Clay_Color{38, 66, 110, 225} : Clay_Color{95, 42, 52, 225},
                  .cornerRadius = {
                      .topLeft = role == 0 && i == 0 ? float(CLAY_RADIUS_SM) : 0.0f,
                      .topRight = role == 0 && i == 9 ? float(CLAY_RADIUS_SM) : 0.0f,
                      .bottomLeft = (role == 1 || !clayton->newGameShowOpponent) && i == 0 ? float(CLAY_RADIUS_SM) : 0.0f,
                      .bottomRight = (role == 1 || !clayton->newGameShowOpponent) && i == 9 ? float(CLAY_RADIUS_SM) : 0.0f,
                  }})
            {
                CLAY(CLAY_IDI("ResultFrameThrows", role * 10 + i),
                     {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIT()},
                                 .layoutDirection = CLAY_LEFT_TO_RIGHT}})
                {
                    const Clay_String symbols[] = {r1, r2, r3};
                    for (int roll = 0; roll < (i == 9 ? 3 : 2); ++roll)
                    {
                        CLAY(CLAY_IDI("ResultThrowSlot", role * 30 + i * 3 + roll),
                             {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIT()},
                                         .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}}})
                        {
                            CLAY_TEXT(ClayArena_FormatString(&clayton->clayArena, "%.*s",
                                symbols[roll].length, symbols[roll].chars), CLAY_TEXT_CONFIG(rollCfg));
                        }
                    }
                }
                CLAY_TEXT(score, CLAY_TEXT_CONFIG(textCfg));
            }
        }
    }
}

static inline void NewGame_RenderScoreboards(Clayton *clayton)
{
    if (!clayton->newGameShowScores) return;
    CLAY(CLAY_ID("ResultScoreboards"),
         {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()}, .childGap = 3,
                     .layoutDirection = CLAY_TOP_TO_BOTTOM}})
    {
        NewGame_RenderScoreboardRow(clayton, 0);
        if (clayton->newGameShowOpponent)
        {
            NewGame_RenderScoreboardRow(clayton, 1);
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
    amountCfg.wrapMode = CLAY_TEXT_WRAP_NONE;
    Clay_String amountText = ClayArena_FormatString(&clayton->clayArena, totalRow ? "$ %d" : "%d", amount);
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
        if (formula && formula[0])
        {
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
        }
        CLAY(
            CLAY_IDI("NewGameMoneyAmount", id),
            {
                .layout = {
                    .sizing = {totalRow ? CLAY_SIZING_FIT(76) : CLAY_SIZING_FIXED(76), CLAY_SIZING_GROW()},
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
        if (clayton->newGameMoneyRowCount > 0)
        {
            for (int i = 0; i < clayton->newGameMoneyRowCount && i < Clayton::NEW_GAME_MONEY_ROW_COUNT; ++i)
            {
                NewGame_RenderMoneyRow(
                    clayton,
                    i,
                    clayton->newGameMoneyRowLabels[i],
                    clayton->newGameMoneyRowFormulas[i],
                    clayton->newGameMoneyRowAmounts[i]
                );
            }
        }
        else
        {
            if (clayton->newGameRoundStrikeCount > 0)
                NewGame_RenderMoneyRow(
                    clayton,
                    0,
                    Txl_Get(clayton->uiLanguage, TXL_STRIKES),
                    strikesFormula,
                    clayton->newGameRoundStrikeCount * 10
                );
            if (clayton->newGameRoundSpareCount > 0)
                NewGame_RenderMoneyRow(
                    clayton,
                    1,
                    Txl_Get(clayton->uiLanguage, TXL_SPARES),
                    sparesFormula,
                    clayton->newGameRoundSpareCount * 5
                );
            if (clayton->newGameRoundCoins > 0 ||
                (clayton->newGameRoundStrikeCount <= 0 &&
                 clayton->newGameRoundSpareCount <= 0 &&
                 clayton->newGameRoundWinByPoints <= 0))
            {
                NewGame_RenderMoneyRow(clayton, 2, Txl_Get(clayton->uiLanguage, TXL_COINS), "", clayton->newGameRoundCoins);
            }
            if (clayton->newGameRoundWinByPoints > 0)
                NewGame_RenderMoneyRow(clayton, 3, Txl_Get(clayton->uiLanguage, TXL_WON_BY_POINTS), "", clayton->newGameRoundWinByPoints);
        }

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
        NewGame_RenderMoneyRow(clayton, 4, Txl_Get(clayton->uiLanguage, TXL_TOTAL), "", clayton->newGameRoundMoneyTotal, true);
    }
}

inline void renderNewGameWindow(Clayton *clayton)
{
    Clay_TextElementConfig titleCfg = CLAY_THEME_TEXT_TITLE;
    Clay_TextElementConfig detailCfg = CLAY_THEME_TEXT_BODY;
    Clay_TextElementConfig buttonCfg = CLAY_THEME_TEXT_BUTTON;
    titleCfg.textAlignment = CLAY_TEXT_ALIGN_CENTER;
    detailCfg.textAlignment = CLAY_TEXT_ALIGN_CENTER;
    detailCfg.wrapMode = CLAY_TEXT_WRAP_WORDS;
    buttonCfg.textAlignment = CLAY_TEXT_ALIGN_CENTER;
    buttonCfg.wrapMode = CLAY_TEXT_WRAP_NONE;
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

    Clay_String title = ClayArena_FormatString(arena, "%s", clayton->newGameTitle ? clayton->newGameTitle : Txl_Get(clayton->uiLanguage, TXL_TRY_AGAIN));
    Clay_String detail = ClayArena_FormatString(arena, "%s", clayton->newGameDetail ? clayton->newGameDetail : "");
    Clay_String button = ClayArena_FormatString(
        arena,
        "%s",
        clayton->newGameButtonLabel ? clayton->newGameButtonLabel : Txl_Get(clayton->uiLanguage, TXL_TRY_AGAIN)
    );
    Clay_String repeatButton = ClayArena_FormatString(
        arena,
        "%s",
        Txl_Get(clayton->uiLanguage, TXL_REPEAT)
    );
    Clay_String shopLabel = ClayArena_FormatString(
        arena,
        "%s",
        clayton->newGameShopButtonLabel ? clayton->newGameShopButtonLabel : Txl_Get(clayton->uiLanguage, TXL_SHOP)
    );
    Clay_String reloadLabel = ClayArena_FormatString(
        arena,
        Txl_Get(clayton->uiLanguage, TXL_RELOADS_IN_FMT),
        clayton->newGameShopReloadText[0] ? clayton->newGameShopReloadText : Txl_Get(clayton->uiLanguage, TXL_RESTOCK_SOON)
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

            if (clayton->newGameContextLabel && clayton->newGameContextLabel[0])
            {
                Clay_TextElementConfig contextCfg = CLAY_THEME_TEXT_LABEL;
                contextCfg.textAlignment = CLAY_TEXT_ALIGN_CENTER;
                CLAY(CLAY_ID("ResultContext"),
                     {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                                 .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                 .layoutDirection = CLAY_TOP_TO_BOTTOM, .childGap = 4}})
                {
                    CLAY_TEXT(ClayArena_FormatString(arena, "%s", clayton->newGameContextLabel), CLAY_TEXT_CONFIG(contextCfg));
                    if (clayton->newGamePostCampaign)
                    {
                        CLAY(CLAY_ID("ResultPostgameTools"),
                             {.layout = {.sizing = {CLAY_SIZING_FIT(), CLAY_SIZING_FIT()},
                                         .childGap = 12, .layoutDirection = CLAY_LEFT_TO_RIGHT}})
                        {
                            const TxlKey keys[] = {TXL_NOS_SHORT, TXL_BLOCKS_SHORT, TXL_RUNES_SHORT};
                            const bool enabled[] = {clayton->newGameNosEnabled, clayton->newGameBlocksEnabled, clayton->newGameRunesEnabled};
                            for (int i = 0; i < 3; ++i)
                            {
                                Clay_TextElementConfig toolCfg = CLAY_THEME_TEXT_BODY;
                                toolCfg.textColor = enabled[i] ? Clay_Color{120, 235, 150, 255} : Clay_Color{130, 120, 140, 255};
                                CLAY_TEXT(ClayArena_FormatString(arena, "%s %s", Txl_Get(clayton->uiLanguage, keys[i]), enabled[i] ? "+" : "-"), CLAY_TEXT_CONFIG(toolCfg));
                            }
                        }
                    }
                }
            }
            NewGame_RenderScoreDuel(clayton);
            NewGame_RenderScoreboards(clayton);
            NewGame_RenderMoneyBreakdown(clayton);

            if (isResult)
            {
                CLAY(
                    CLAY_ID("NewGameShopRow"),
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
                        CLAY_ID("NewGameShopPanel"),
                        {
                            .layout = {
                                .sizing = {
                                    CLAY_SIZING_GROW(),
                                    CLAY_SIZING_FIXED(76)
                                },
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

            CLAY(CLAY_ID("ResultActionsRow"),
                 {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                             .childGap = 8, .layoutDirection = CLAY_LEFT_TO_RIGHT}})
            {
                if (isResult && clayton->newGameRepeatAvailable && clayton->newGameNextAvailable)
                {
                    CLAY(clayton->resultRepeatClick.clayId,
                         {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(64)},
                                     .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}},
                          .backgroundColor = CLAY_COLOR_BTN_DANGER,
                          .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG}})
                    { CLAY_TEXT(repeatButton, CLAY_TEXT_CONFIG(buttonCfg)); }
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
}
