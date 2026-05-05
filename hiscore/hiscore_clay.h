#pragma once

#include "../clayton/clayton.h"
#include "./localhi.h"

inline void buildHiScoreWindowClay(Clayton *clayton, LocalHighscore *self)
{
    if (!clayton || !self)
        return;
    ClayArena *arena = &clayton->clayArena; // ← Embedded arena

    // Theme font configs
    Clay_TextElementConfig labelCfg = CLAY_THEME_TEXT_LABEL;
    Clay_TextElementConfig buttonCfg = CLAY_THEME_TEXT_BUTTON;
    Clay_TextElementConfig titleCfg = CLAY_THEME_TEXT_TITLE;
    Clay_TextElementConfig scoreCfg = CLAY_THEME_TEXT_LARGE;

    // Root container exists for pointer-hit testing.
    CLAY(
        CLAY_ID("HiScoreContainer"),
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
        CLAY(CLAY_ID("HiScoreWindow"), CLAY_THEME_PANEL)
        {

            // Title bar
            CLAY(
                CLAY_ID("HiScoreTitle"),
                {.layout = {
                     .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                     .padding = {0, 0, 5, 0},
                     .childGap = 10,
                     .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                     .layoutDirection = CLAY_LEFT_TO_RIGHT
                 }}
            )
            {
                CLAY_TEXT(CLAY_STRING("🏆 Top Scores"), CLAY_TEXT_CONFIG(titleCfg));
                CLAY(
                    CLAY_ID("TitleDivider"),
                    {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1)}}}
                ){};
                CLAY(clayton->hiScoreCloseClick.clayId, CLAY_THEME_BTN_DANGER)
                {
                    CLAY_TEXT(CLAY_STRING("x"), CLAY_TEXT_CONFIG(buttonCfg));
                }
            }

            if (clayton->shouldShowHiScoreWithLatest == true)
            {

                // Feedback section — simplified text-only percentile
                if (self->lastSubmitResult != LOCALHI_SUBMIT_NONE)
                {
                    Clay_Color feedbackBg = (self->lastSubmitResult == LOCALHI_SUBMIT_NEW_RECORD)
                        ? CLAY_COLOR_BTN_SUCCESS
                        : CLAY_COLOR_BTN_DISABLED;
                    Clay_String feedbackTitle;
                    char feedbackBuf[64];
                    if (self->lastSubmitResult == LOCALHI_SUBMIT_NEW_RECORD)
                    {
                        int len = snprintf(
                            feedbackBuf,
                            sizeof(feedbackBuf),
                            "Your score is in top %d",
                            self->lastSubmittedRank
                        );
                        feedbackTitle = ClayArena_AllocString(arena, feedbackBuf);
                    }
                    else
                    {
                        feedbackTitle = CLAY_STRING("Good Run!");
                    }

                    CLAY(
                        CLAY_ID("Feedback"),
                        {.layout =
                             {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                              .padding = {12, 12, 12, 12},
                              .childGap = 8,
                              .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                              .layoutDirection = CLAY_TOP_TO_BOTTOM},
                         .backgroundColor = feedbackBg,
                         .cornerRadius = {
                             CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG
                         }}
                    )
                    {
                        CLAY_TEXT(feedbackTitle, CLAY_TEXT_CONFIG(buttonCfg));

                        // Score display
                        Clay_String scoreStr =
                            ClayArena_FormatString(arena, "%d points", self->lastSubmittedScore);
                        CLAY_TEXT(scoreStr, CLAY_TEXT_CONFIG(scoreCfg));

                        // Simple percentile label: "Your score is higher than X% of all recent
                        // runs"
                        Clay_String pctLabel = ClayArena_FormatString(
                            arena,
                            "Your score is higher than %.0f%% of all recent runs",
                            self->lastSubmittedPercentile
                        );
                        CLAY_TEXT(pctLabel, CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_BODY));
                    }
                }
            }

            // Leaderboard
            CLAY(CLAY_ID("LBSection"), CLAY_THEME_SECTION)
            {
                CLAY_TEXT(CLAY_STRING("Leaderboard (Last Hour)"), CLAY_TEXT_CONFIG(labelCfg));

                // Header
                CLAY(
                    CLAY_ID("LBHeader"),
                    {.layout =
                         {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                          .padding = {5, 5, 5, 5},
                          .childGap = 10,
                          .layoutDirection = CLAY_LEFT_TO_RIGHT},
                     .border = {.color = CLAY_COLOR_DIVIDER, .width = {.top = 1, .bottom = 1}}}
                )
                {
                    CLAY(
                        CLAY_ID("HRank"),
                        {.layout = {.sizing = {CLAY_SIZING_FIXED(40), CLAY_SIZING_FIT()}}}
                    )
                    {
                        CLAY_TEXT(CLAY_STRING("#"), CLAY_TEXT_CONFIG(labelCfg));
                    }
                    CLAY(
                        CLAY_ID("HName"),
                        {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()}}}
                    )
                    {
                        CLAY_TEXT(CLAY_STRING("Player"), CLAY_TEXT_CONFIG(labelCfg));
                    }
                    CLAY(
                        CLAY_ID("HScore"),
                        {.layout = {
                             .sizing = {CLAY_SIZING_FIXED(80), CLAY_SIZING_FIT()},
                             .childAlignment = {CLAY_ALIGN_X_RIGHT, CLAY_ALIGN_Y_CENTER}
                         }}
                    )
                    {
                        CLAY_TEXT(CLAY_STRING("Score"), CLAY_TEXT_CONFIG(labelCfg));
                    }
                    CLAY(
                        CLAY_ID("HTime"),
                        {.layout = {
                             .sizing = {CLAY_SIZING_FIXED(60), CLAY_SIZING_FIT()},
                             .childAlignment = {CLAY_ALIGN_X_RIGHT, CLAY_ALIGN_Y_CENTER}
                         }}
                    )
                    {
                        CLAY_TEXT(CLAY_STRING("Age"), CLAY_TEXT_CONFIG(labelCfg));
                    }
                }

                // Entries
                LocalHi_CleanExpired(self);
                for (int32_t i = 0; i < self->count; i++)
                {
                    LocalHiEntry *e = &self->entries[i];
                    bool isUser =
                        (self->lastSubmitResult == LOCALHI_SUBMIT_NEW_RECORD &&
                         self->lastSubmittedRank == i + 1);
                    Clay_Color rowBg =
                        isUser ? (Clay_Color){90, 70, 140, 255} : CLAY_COLOR_PANEL_SECTION;

                    CLAY(
                        CLAY_IDI("LBRow", i),
                        {.layout =
                             {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                              .padding = {8, 8, 8, 8},
                              .childGap = 10,
                              .layoutDirection = CLAY_LEFT_TO_RIGHT},
                         .backgroundColor = rowBg,
                         .cornerRadius = {
                             CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD
                         }}
                    )
                    {
                        // Rank
                        CLAY(
                            CLAY_IDI("RRank", i),
                            {.layout = {
                                 .sizing = {CLAY_SIZING_FIXED(40), CLAY_SIZING_FIT()},
                                 .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}
                             }}
                        )
                        {
                            Clay_String rs = ClayArena_FormatString(arena, "%d", i + 1);
                            Clay_Color rc = (i == 0) ? (Clay_Color){255, 215, 0, 255}
                                : (i == 1)           ? (Clay_Color){192, 192, 192, 255}
                                : (i == 2)           ? (Clay_Color){205, 127, 50, 255}
                                                     : CLAY_COLOR_TEXT_SECONDARY;
                            Clay_TextElementConfig rcf = {
                                .textColor = rc,
                                .fontId = CLAY_FONT_NOTO,
                                .fontSize = CLAY_FONT_SIZE_SM
                            };
                            CLAY_TEXT(rs, CLAY_TEXT_CONFIG(rcf));
                        }
                        // Username
                        CLAY(
                            CLAY_IDI("RName", i),
                            {.layout = {
                                 .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                                 .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER}
                             }}
                        )
                        {
                            Clay_String ns = ClayArena_AllocString(arena, e->username);
                            Clay_Color nc =
                                isUser ? CLAY_COLOR_BTN_ACTIVE : CLAY_COLOR_TEXT_PRIMARY;
                            Clay_TextElementConfig ncf = {
                                .textColor = nc,
                                .fontId = CLAY_FONT_NOTO,
                                .fontSize = CLAY_FONT_SIZE_SM
                            };
                            CLAY_TEXT(ns, CLAY_TEXT_CONFIG(ncf));
                        }
                        // Score
                        CLAY(
                            CLAY_IDI("RScore", i),
                            {.layout = {
                                 .sizing = {CLAY_SIZING_FIXED(80), CLAY_SIZING_FIT()},
                                 .childAlignment = {CLAY_ALIGN_X_RIGHT, CLAY_ALIGN_Y_CENTER}
                             }}
                        )
                        {
                            Clay_String ss = ClayArena_FormatString(arena, "%d", e->score);
                            Clay_Color sc =
                                isUser ? CLAY_COLOR_BTN_SUCCESS : CLAY_COLOR_TEXT_PRIMARY;
                            Clay_TextElementConfig scf = {
                                .textColor = sc,
                                .fontId = CLAY_FONT_NOTO,
                                .fontSize = CLAY_FONT_SIZE_SM
                            };
                            CLAY_TEXT(ss, CLAY_TEXT_CONFIG(scf));
                        }
                        // Time
                        CLAY(
                            CLAY_IDI("RTime", i),
                            {.layout = {
                                 .sizing = {CLAY_SIZING_FIXED(60), CLAY_SIZING_FIT()},
                                 .childAlignment = {CLAY_ALIGN_X_RIGHT, CLAY_ALIGN_Y_CENTER}
                             }}
                        )
                        {
                            int32_t m = LocalHi_GetMinutesAgo(e->timestamp);
                            Clay_String ts = ClayArena_FormatString(arena, "%dm", m);
                            CLAY_TEXT(ts, CLAY_TEXT_CONFIG(labelCfg));
                        }
                    }
                }

                // Empty state
                if (self->count == 0)
                {
                    CLAY(
                        CLAY_ID("LBEmpty"),
                        {.layout = {
                             .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(60)},
                             .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}
                         }}
                    )
                    {
                        CLAY_TEXT(
                            CLAY_STRING("No scores yet — be the first! 🎮"),
                            CLAY_TEXT_CONFIG(labelCfg)
                        );
                    }
                }
            }

            // Stats footer
            CLAY(
                CLAY_ID("LBStats"),
                {.layout = {
                     .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                     .padding = {10, 10, 10, 10},
                     .childGap = 15,
                     .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                     .layoutDirection = CLAY_LEFT_TO_RIGHT
                 }}
            )
            {
                Clay_String att = ClayArena_FormatString(
                    arena, "Attempts: %d", self->percentileTracker.totalAttempts
                );
                CLAY_TEXT(att, CLAY_TEXT_CONFIG(labelCfg));
                if (self->percentileTracker.totalAttempts > 0)
                {
                    Clay_String rng = ClayArena_FormatString(
                        arena,
                        "Range: %d–%d",
                        self->percentileTracker.minScore,
                        self->percentileTracker.maxScore
                    );
                    CLAY_TEXT(rng, CLAY_TEXT_CONFIG(labelCfg));
                }
            }
        }
    }
}

// Legacy wrapper: preserves the old overlay behavior for call sites that expect it.
inline void buildHiScoreClay(Clayton *clayton, LocalHighscore *self)
{
    if (!clayton || !self)
        return;
    CLAY(CLAY_ID("HiScoreContainerOverlay"), CLAY_THEME_OVERLAY)
    {
        buildHiScoreWindowClay(clayton, self);
    }
}
