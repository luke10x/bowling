#pragma once

#include <clay.h>

// =============================================================================
// Clay UI Theme — Centralized styling for all Clay UI components
// =============================================================================
// All UI elements (sound settings, adaptive audio modal, keypad, scoreboard,
// menu buttons, etc.) should use these definitions for consistent look & feel.
//
// Font: NotoSansSC-Regular (fontId 0) is the primary font.
// Font sizes follow powers of 2: 12, 16, 24, 32, 48, 64
// =============================================================================

// -----------------------------------------------------------------------------
// Font IDs — NotoSansSC for all UI, RobotoMono for keypad input
// -----------------------------------------------------------------------------
#define CLAY_FONT_NOTO 0 // NotoSansSC-Regular — primary UI font
#define CLAY_FONT_MONO 2 // RobotoMono-Regular — keypad input display

// -----------------------------------------------------------------------------
// Font sizes (1:2:4 ratio — only 3 sizes)
// -----------------------------------------------------------------------------
#define CLAY_FONT_SIZE_SM 18 // Body text, button labels, small UI
#define CLAY_FONT_SIZE_MD 24 // Section labels, modal titles, headings
#define CLAY_FONT_SIZE_XL 48 // Hero numbers, large display values

// -----------------------------------------------------------------------------
// Color palette — vibrant
// -----------------------------------------------------------------------------

// Panel backgrounds
#define CLAY_COLOR_PANEL_BG ((Clay_Color){45, 25, 65, 255})      // Deep purple panel
#define CLAY_COLOR_PANEL_SECTION ((Clay_Color){65, 35, 85, 255}) // Lighter purple section cards
#define CLAY_COLOR_OVERLAY ((Clay_Color){20, 10, 40, 120}) // Deep purple semi-transparent overlay
#define CLAY_COLOR_WINDOW_STACK_OVERLAY ((Clay_Color){255, 255, 255, 100}) // Match side spacers

// Text colors
#define CLAY_COLOR_TEXT_PRIMARY ((Clay_Color){255, 255, 255, 255})   // White text on dark bg
#define CLAY_COLOR_TEXT_SECONDARY ((Clay_Color){220, 190, 255, 255}) // Soft lavender secondary text
#define CLAY_COLOR_TEXT_LABEL ((Clay_Color){230, 210, 255, 255}) // Light lavender section labels
#define CLAY_COLOR_TEXT_DARK ((Clay_Color){40, 20, 60, 255}) // Deep purple dark text on light bg

// Button colors
#define CLAY_COLOR_BTN_PRIMARY ((Clay_Color){80, 60, 220, 255})  // Vibrant indigo
#define CLAY_COLOR_BTN_HUD ((Clay_Color){80, 60, 220, 180})      // Vibrant indigo
#define CLAY_COLOR_BTN_SUCCESS ((Clay_Color){40, 200, 120, 255}) // Mint green
#define CLAY_COLOR_BTN_DANGER ((Clay_Color){230, 60, 100, 255})  // Hot pink/coral
#define CLAY_COLOR_BTN_DISABLED ((Clay_Color){90, 70, 130, 255}) // Muted purple
#define CLAY_COLOR_BTN_ACTIVE ((Clay_Color){100, 230, 160, 255}) // Bright mint selected

// Progress bar — rainbow gradient
#define CLAY_COLOR_PROGRESS_BG ((Clay_Color){55, 35, 80, 255})       // Dark purple track
#define CLAY_COLOR_PROGRESS_FILL ((Clay_Color){180, 80, 255, 255})   // Vibrant purple fill
#define CLAY_COLOR_PROGRESS_WARN ((Clay_Color){255, 200, 60, 255})   // Golden yellow warning
#define CLAY_COLOR_PROGRESS_ORANGE ((Clay_Color){255, 140, 50, 255}) // Vibrant orange mid-progress

// Borders and accents
#define CLAY_COLOR_BORDER ((Clay_Color){160, 120, 220, 255})  // Soft purple border
#define CLAY_COLOR_DIVIDER ((Clay_Color){160, 120, 220, 255}) // Purple divider line

// -----------------------------------------------------------------------------
// Corner radius (consistent rounding)
// -----------------------------------------------------------------------------
#define CLAY_RADIUS_SM 5  // Small elements: progress bars, inline items
#define CLAY_RADIUS_MD 8  // Medium elements: quality/volume buttons
#define CLAY_RADIUS_LG 10 // Large elements: action buttons, close buttons
#define CLAY_RADIUS_XL 15 // Extra large: modal windows, panels

// -----------------------------------------------------------------------------
// Border width
// -----------------------------------------------------------------------------
#define CLAY_BORDER_WIDTH 2

// -----------------------------------------------------------------------------
// Pre-configured text configs (ready to use with CLAY_TEXT_CONFIG)
// -----------------------------------------------------------------------------

// Title text for modal windows and major headings
#define CLAY_THEME_TEXT_TITLE                                                                      \
    ((Clay_TextElementConfig){                                                                     \
        .textColor = CLAY_COLOR_TEXT_PRIMARY,                                                      \
        .fontId = CLAY_FONT_NOTO,                                                                  \
        .fontSize = CLAY_FONT_SIZE_MD,                                                             \
    })

// Body text for descriptions, button labels, and secondary info
#define CLAY_THEME_TEXT_BODY                                                                       \
    ((Clay_TextElementConfig){                                                                     \
        .textColor = CLAY_COLOR_TEXT_SECONDARY,                                                    \
        .fontId = CLAY_FONT_NOTO,                                                                  \
        .fontSize = CLAY_FONT_SIZE_SM,                                                             \
    })

// Section labels within panels
#define CLAY_THEME_TEXT_LABEL                                                                      \
    ((Clay_TextElementConfig){                                                                     \
        .textColor = CLAY_COLOR_TEXT_LABEL,                                                        \
        .fontId = CLAY_FONT_NOTO,                                                                  \
        .fontSize = CLAY_FONT_SIZE_SM,                                                             \
    })

// Button text
#define CLAY_THEME_TEXT_BUTTON                                                                     \
    ((Clay_TextElementConfig){                                                                     \
        .textColor = CLAY_COLOR_TEXT_PRIMARY,                                                      \
        .fontId = CLAY_FONT_NOTO,                                                                  \
        .fontSize = CLAY_FONT_SIZE_SM,                                                             \
    })

// Large display text for scores and important numbers
#define CLAY_THEME_TEXT_LARGE                                                                      \
    ((Clay_TextElementConfig){                                                                     \
        .textColor = CLAY_COLOR_TEXT_PRIMARY,                                                      \
        .fontId = CLAY_FONT_NOTO,                                                                  \
        .fontSize = CLAY_FONT_SIZE_XL,                                                             \
    })

// Monospace text for keypad input display
#define CLAY_THEME_TEXT_INPUT                                                                      \
    ((Clay_TextElementConfig){                                                                     \
        .textColor = {255, 25, 25, 255},                                                           \
        .fontId = CLAY_FONT_MONO,                                                                  \
        .fontSize = CLAY_FONT_SIZE_XL,                                                             \
    })

// -----------------------------------------------------------------------------
// Panel style helper (common panel configuration)
// Usage: CLAY(CLAY_ID("MyPanel"), CLAY_THEME_PANEL) { ... }
// -----------------------------------------------------------------------------
#define CLAY_THEME_WINDOW_BORDER                                                                   \
    .border = {                                                                                    \
        .color = CLAY_COLOR_BORDER,                                                                \
        .width = CLAY_BORDER_OUTSIDE(CLAY_BORDER_WIDTH + 1),                                           \
    },

// Small outline border for buttons/controls (used e.g. by keypad keys).
#define CLAY_THEME_BTN_BORDER_SMALL                                                                \
    .border = {                                                                                    \
        .color = CLAY_COLOR_BORDER,                                                                \
        .width = CLAY_BORDER_ALL(1),                                                               \
    },

#define CLAY_THEME_PANEL                                                                           \
    {                                                                                              \
        .layout =                                                                                  \
            {                                                                                      \
                .sizing = {CLAY_SIZING_PERCENT(0.8f), CLAY_SIZING_FIT()},                          \
                .padding = {20, 20, 20, 20},                                                       \
                .childGap = 15,                                                                    \
                .layoutDirection = CLAY_TOP_TO_BOTTOM,                                             \
            },                                                                                     \
        .backgroundColor = CLAY_COLOR_PANEL_BG,                                                    \
        .cornerRadius = {CLAY_RADIUS_XL, CLAY_RADIUS_XL, CLAY_RADIUS_XL, CLAY_RADIUS_XL},          \
    }

// Panel style for top-level windows (same as panel + outer border like the shop container).
#define CLAY_THEME_WINDOW_PANEL                                                                    \
    {                                                                                              \
        .layout =                                                                                  \
            {                                                                                      \
                .sizing = {CLAY_SIZING_PERCENT(0.8f), CLAY_SIZING_FIT()},                          \
                .padding = {20, 20, 20, 20},                                                       \
                .childGap = 15,                                                                    \
                .layoutDirection = CLAY_TOP_TO_BOTTOM,                                             \
            },                                                                                     \
        .backgroundColor = CLAY_COLOR_PANEL_BG,                                                    \
        .cornerRadius = {CLAY_RADIUS_XL, CLAY_RADIUS_XL, CLAY_RADIUS_XL, CLAY_RADIUS_XL},          \
        CLAY_THEME_WINDOW_BORDER                                                                   \
    }

// Section card style (nested panels within a main panel)
#define CLAY_THEME_SECTION                                                                         \
    {                                                                                              \
        .layout =                                                                                  \
            {                                                                                      \
                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},                                 \
                .padding = {10, 10, 10, 10},                                                       \
                .childGap = 10,                                                                    \
                .layoutDirection = CLAY_TOP_TO_BOTTOM,                                             \
            },                                                                                     \
        .backgroundColor = CLAY_COLOR_PANEL_SECTION,                                               \
        .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},          \
    }

// Primary button style
#define CLAY_THEME_BTN_PRIMARY                                                                     \
    {                                                                                              \
        .layout =                                                                                  \
            {                                                                                      \
                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(60)},                             \
                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},                      \
            },                                                                                     \
        .backgroundColor = CLAY_COLOR_BTN_PRIMARY,                                                 \
        .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},          \
    }

    #define CLAY_THEME_BTN_BOX                                                                     \
    {                                                                                              \
        .layout =                                                                                  \
            {                                                                                      \
                .sizing = {CLAY_SIZING_FIXED(60), CLAY_SIZING_FIXED(60)},                             \
                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},                      \
            },                                                                                     \
        .backgroundColor = CLAY_COLOR_BTN_PRIMARY,                                                 \
        .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},          \
    }

#define CLAY_THEME_BTN_HUD                                                                         \
    {                                                                                              \
        .layout =                                                                                  \
            {                                                                                      \
                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(60)},                             \
                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},                      \
            },                                                                                     \
        .backgroundColor = CLAY_COLOR_BTN_HUD,                                                     \
        .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},          \
    }

// Success button style
#define CLAY_THEME_BTN_SUCCESS                                                                     \
    {                                                                                              \
        .layout =                                                                                  \
            {                                                                                      \
                .sizing = {CLAY_SIZING_FIT(), CLAY_SIZING_FIXED(60)},                              \
                .padding = {30, 30, 60, 60},                                                       \
                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},                      \
            },                                                                                     \
        .backgroundColor = CLAY_COLOR_BTN_SUCCESS,                                                 \
        .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},          \
    }

// Danger/close button style
#define CLAY_THEME_BTN_DANGER                                                                      \
    {                                                                                              \
        .layout =                                                                                  \
            {                                                                                      \
                .sizing = {CLAY_SIZING_FIXED(50), CLAY_SIZING_FIXED(50)},                          \
                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},                      \
            },                                                                                     \
        .backgroundColor = CLAY_COLOR_BTN_DANGER,                                                  \
        .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},          \
    }

// Small square button (for quality/volume selectors)
#define CLAY_THEME_BTN_SMALL                                                                       \
    {                                                                                              \
        .layout =                                                                                  \
            {                                                                                      \
                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(50)},                             \
                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},                      \
            },                                                                                     \
        .backgroundColor = CLAY_COLOR_BTN_DISABLED,                                                \
        .cornerRadius = {CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD},          \
        .border = {                                                                                \
            .color = CLAY_COLOR_BORDER,                                                            \
            .width = CLAY_BORDER_ALL(CLAY_BORDER_WIDTH),                                           \
        },                                                                                         \
    }

// Progress bar background
#define CLAY_THEME_PROGRESS_BAR_BG                                                                 \
    {                                                                                              \
        .layout =                                                                                  \
            {                                                                                      \
                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(20)},                             \
            },                                                                                     \
        .backgroundColor = CLAY_COLOR_PROGRESS_BG,                                                 \
        .cornerRadius = {CLAY_RADIUS_SM, CLAY_RADIUS_SM, CLAY_RADIUS_SM, CLAY_RADIUS_SM},          \
    }

// Progress bar fill (use inside progress bar bg)
#define CLAY_THEME_PROGRESS_BAR_FILL(progress)                                                     \
    {                                                                                              \
        .layout =                                                                                  \
            {                                                                                      \
                .sizing = {CLAY_SIZING_PERCENT(progress), CLAY_SIZING_GROW()},                     \
            },                                                                                     \
        .backgroundColor = CLAY_COLOR_PROGRESS_FILL,                                               \
        .cornerRadius = {CLAY_RADIUS_SM, CLAY_RADIUS_SM, CLAY_RADIUS_SM, CLAY_RADIUS_SM},          \
    }

// Full-screen overlay background (for modals)
#define CLAY_THEME_OVERLAY                                                                         \
    {                                                                                              \
        .layout =                                                                                  \
            {                                                                                      \
                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},                                \
                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},                      \
            },                                                                                     \
        .backgroundColor = CLAY_COLOR_OVERLAY,                                                     \
    }

// Used by WindowStack to dim everything below the active (topmost) window, matching the side spacers.
#define CLAY_THEME_WINDOW_STACK_OVERLAY                                                            \
    {                                                                                              \
        .layout =                                                                                  \
            {                                                                                      \
                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},                                \
            },                                                                                     \
        .backgroundColor = CLAY_COLOR_WINDOW_STACK_OVERLAY,                                        \
    }

// Top bar container (for username, money display, etc.)
#define CLAY_THEME_TOP_BAR                                                                         \
    {                                                                                              \
        .layout = {                                                                                \
            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},                                     \
            .padding = {.top = 5, .bottom = 5},                        \
            .childGap = 10,                                                                        \
            .layoutDirection = CLAY_LEFT_TO_RIGHT,                                                 \
        },                                                                                         \
    }
// =============================================================================
// SHOP UI THEME — Catalog items, shop grid, and purchase components
// =============================================================================

// -----------------------------------------------------------------------------
// Shop-specific colors
// -----------------------------------------------------------------------------
#define CLAY_COLOR_SHOP_ITEM_BG ((Clay_Color){55, 35, 75, 255})       // Slightly lighter card bg
#define CLAY_COLOR_RARITY_COMMON ((Clay_Color){120, 120, 120, 255})   // Gray for Common
#define CLAY_COLOR_RARITY_RARE ((Clay_Color){60, 120, 255, 255})      // Blue for Rare
#define CLAY_COLOR_RARITY_EPIC ((Clay_Color){180, 60, 255, 255})      // Purple for Epic
#define CLAY_COLOR_RARITY_LEGENDARY ((Clay_Color){255, 200, 40, 255}) // Gold for Legendary
#define CLAY_COLOR_PRICE_TAG ((Clay_Color){255, 215, 0, 255})         // Golden coins
#define CLAY_COLOR_STAT_EMPTY ((Clay_Color){70, 50, 90, 255})         // Dimmed stat bar bg
#define CLAY_COLOR_STAT_FILL ((Clay_Color){100, 230, 160, 255}) // Bright mint for active stats

// -----------------------------------------------------------------------------
// Shop layout configs
// -----------------------------------------------------------------------------

// Catalog item card — holds ball preview, stats, price, buy button
#define CLAY_THEME_CATALOG_ITEM                                                                    \
    {                                                                                              \
        .layout =                                                                                  \
            {                                                                                      \
                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},                                 \
                .padding = {12, 12, 12, 12},                                                       \
                .childGap = 8,                                                                     \
                .layoutDirection = CLAY_TOP_TO_BOTTOM,                                             \
            },                                                                                     \
        .backgroundColor = CLAY_COLOR_SHOP_ITEM_BG,                                                \
        .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},          \
        .border = {                                                                                \
            .color = CLAY_COLOR_BORDER,                                                            \
            .width = CLAY_BORDER_ALL(CLAY_BORDER_WIDTH),                                           \
        },                                                                                         \
    }

// Rarity badge — small pill in top-right of item card
#define CLAY_THEME_RARITY_BADGE                                                                    \
    {                                                                                              \
        .layout =                                                                                  \
            {                                                                                      \
                .sizing = {CLAY_SIZING_FIT(), CLAY_SIZING_FIXED(22)},                              \
                .padding = {8, 8, 4, 4},                                                           \
                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},                      \
            },                                                                                     \
        .cornerRadius = {CLAY_RADIUS_SM, CLAY_RADIUS_SM, CLAY_RADIUS_SM, CLAY_RADIUS_SM},          \
    }

// Ball image placeholder area
#define CLAY_THEME_BALL_PREVIEW                                                                    \
    {                                                                                              \
        .layout =                                                                                  \
            {                                                                                      \
                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(120)},                            \
                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},                      \
            },                                                                                     \
        .backgroundColor = CLAY_COLOR_PANEL_SECTION,                                               \
        .cornerRadius = {CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD},          \
    }

// Price display row
#define CLAY_THEME_PRICE_ROW                                                                       \
    {                                                                                              \
        .layout = {                                                                                \
            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},                                     \
            .padding = {4, 4, 4, 4},                                                               \
            .childAlignment = {CLAY_ALIGN_X_RIGHT, CLAY_ALIGN_Y_CENTER},                           \
            .layoutDirection = CLAY_LEFT_TO_RIGHT,                                                 \
        },                                                                                         \
    }

// Stat row — label + bar visualization
#define CLAY_THEME_STAT_ROW                                                                        \
    {                                                                                              \
        .layout = {                                                                                \
            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(18)},                                 \
            .padding = {0, 0, 0, 0},                                                               \
            .childGap = 6,                                                                         \
            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},                          \
            .layoutDirection = CLAY_LEFT_TO_RIGHT,                                                 \
        },                                                                                         \
    }

// Stat bar background track
#define CLAY_THEME_STAT_BAR_BG                                                                     \
    {                                                                                              \
        .layout =                                                                                  \
            {                                                                                      \
                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(10)},                             \
            },                                                                                     \
        .backgroundColor = CLAY_COLOR_STAT_EMPTY,                                                  \
        .cornerRadius = {CLAY_RADIUS_SM, CLAY_RADIUS_SM, CLAY_RADIUS_SM, CLAY_RADIUS_SM},          \
    }

// Stat bar fill (dynamic width based on value 0.0–1.0)
#define CLAY_THEME_STAT_BAR_FILL(value)                                                            \
    {                                                                                              \
        .layout =                                                                                  \
            {                                                                                      \
                .sizing = {CLAY_SIZING_PERCENT(value), CLAY_SIZING_GROW()},                        \
            },                                                                                     \
        .backgroundColor = CLAY_COLOR_STAT_FILL,                                                   \
        .cornerRadius = {CLAY_RADIUS_SM, CLAY_RADIUS_SM, CLAY_RADIUS_SM, CLAY_RADIUS_SM},          \
    }

// Buy button — fits inside catalog item
#define CLAY_THEME_BTN_BUY                                                                         \
    {                                                                                              \
        .layout =                                                                                  \
            {                                                                                      \
                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(40)},                             \
                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},                      \
            },                                                                                     \
        .backgroundColor = CLAY_COLOR_BTN_SUCCESS,                                                 \
        .cornerRadius = {CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD},          \
    }

#define CLAY_THEME_BTN_BUY_DISABLED                                                                \
    {                                                                                              \
        .layout =                                                                                  \
            {                                                                                      \
                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(40)},                             \
                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},                      \
            },                                                                                     \
        .backgroundColor = CLAY_COLOR_BTN_DISABLED,                                                \
        .cornerRadius = {CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD},          \
    }

// Shop container — main panel holding grid + header/footer
#define CLAY_THEME_SHOP_CONTAINER                                                                  \
    {                                                                                              \
        .layout =                                                                                  \
            {                                                                                      \
                .sizing = {CLAY_SIZING_PERCENT(0.9f), CLAY_SIZING_FIT()},                           \
                .childGap = 12,                                                                    \
                .layoutDirection = CLAY_TOP_TO_BOTTOM,                                             \
            },                                                                                     \
        .backgroundColor = CLAY_COLOR_PANEL_BG,                                                    \
        .cornerRadius = {CLAY_RADIUS_XL, CLAY_RADIUS_XL, CLAY_RADIUS_XL, CLAY_RADIUS_XL},          \
        .border = {                                                                                \
            .color = CLAY_COLOR_BORDER,                                                            \
            .width = CLAY_BORDER_ALL(CLAY_BORDER_WIDTH + 1),                                       \
        },                                                                                         \
    }
#define CLAY_THEME_SHOP_CONTAINER_PADDING                                                          \
    {                                                                                              \
        .layout =                                                                                  \
            {                                                                                      \
                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},                                  \
                .padding = {16, 16, 16, 16},                                                       \
                .childGap = 12,                                                                    \
                .layoutDirection = CLAY_TOP_TO_BOTTOM,                                             \
            },                                                                                     \
        .backgroundColor = CLAY_COLOR_PANEL_BG,                                                    \
        .cornerRadius = {CLAY_RADIUS_XL, CLAY_RADIUS_XL, CLAY_RADIUS_XL, CLAY_RADIUS_XL},          \
    }

// Shop header — title + currency
#define CLAY_THEME_SHOP_HEADER                                                                     \
    {                                                                                              \
        .layout =                                                                                  \
            {                                                                                      \
                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},                                 \
                .padding = {12, 16, 12, 16},                                                       \
                .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},                        \
                .layoutDirection = CLAY_LEFT_TO_RIGHT,                                             \
            },                                                                                     \
        .border = {                                                                                \
            .color = CLAY_COLOR_DIVIDER,                                                           \
            .width = CLAY_BORDER_OUTSIDE(CLAY_BORDER_WIDTH),                                           \
        },                                                                                         \
    }

#define CLAY_THEME_SHOP_GRID                                                                       \
    {                                                                                              \
        .layout =                                                                                  \
            {                                                                                      \
                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT},                                   \
                .padding = {8, 8, 8, 8},                                                           \
                .childGap = 10,                                                                    \
                .layoutDirection = CLAY_LEFT_TO_RIGHT,                                             \
            },                                                                                     \
        .clip = {.vertical = false, .horizontal = true, .childOffset = Clay_GetScrollOffset()},    \
    }

#define CLAY_THEME_SHOP_GRID_INNER                                                                 \
    {                                                                                              \
        .layout = {                                                                                \
            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},                                    \
            .padding = {8, 8, 8, 8},                                                               \
            .childGap = 10,                                                                        \
            .layoutDirection = CLAY_LEFT_TO_RIGHT,                                                 \
        },                                                                                         \
    }

// Grid cell wrapper — ensures equal sizing for 2x2
#define CLAY_THEME_SHOP_CELL                                                                       \
    {                                                                                              \
        .layout = {                                                                                \
            .sizing = {CLAY_SIZING_PERCENT(0.48f), CLAY_SIZING_GROW()},                            \
        },                                                                                         \
    }

// Shop footer — reset countdown
#define CLAY_THEME_SHOP_FOOTER                                                                     \
    {                                                                                              \
        .layout =                                                                                  \
            {                                                                                      \
                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},                                 \
                .padding = {12, 12, 12, 12},                                                       \
                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},                      \
            },                                                                                     \
        .backgroundColor = CLAY_COLOR_PANEL_SECTION,                                               \
        .cornerRadius = {CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD},          \
        .border = {                                                                                \
            .color = CLAY_COLOR_BORDER,                                                            \
            .width = CLAY_BORDER_ALL(CLAY_BORDER_WIDTH),                                           \
        },                                                                                         \
    }

// Text config for rarity badge (small, bold, white)
#define CLAY_THEME_TEXT_RARITY                                                                     \
    ((Clay_TextElementConfig){                                                                     \
        .textColor = CLAY_COLOR_TEXT_PRIMARY,                                                      \
        .fontId = CLAY_FONT_NOTO,                                                                  \
        .fontSize = CLAY_FONT_SIZE_SM,                                                             \
    })

// Text config for price (golden, slightly larger)
#define CLAY_THEME_TEXT_PRICE                                                                      \
    ((Clay_TextElementConfig){                                                                     \
        .textColor = CLAY_COLOR_PRICE_TAG,                                                         \
        .fontId = CLAY_FONT_NOTO,                                                                  \
        .fontSize = CLAY_FONT_SIZE_XL,                                                             \
    })

// Text config for stat labels (compact)
#define CLAY_THEME_TEXT_STAT                                                                       \
    ((Clay_TextElementConfig){                                                                     \
        .textColor = CLAY_COLOR_TEXT_LABEL,                                                        \
        .fontId = CLAY_FONT_NOTO,                                                                  \
        .fontSize = CLAY_FONT_SIZE_SM - 2,                                                         \
    })

// Text config for countdown footer
#define CLAY_THEME_TEXT_COUNTDOWN                                                                  \
    ((Clay_TextElementConfig){                                                                     \
        .textColor = CLAY_COLOR_TEXT_SECONDARY,                                                    \
        .fontId = CLAY_FONT_NOTO,                                                                  \
        .fontSize = CLAY_FONT_SIZE_SM,                                                             \
    })
