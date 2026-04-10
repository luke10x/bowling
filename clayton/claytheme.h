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
// Font IDs (matching clayton.h font loading order)
// -----------------------------------------------------------------------------
#define CLAY_FONT_NOTO      0   // NotoSansSC-Regular — primary UI font
#define CLAY_FONT_SUSEMONO  1   // SUSEMono-Medium — monospace (scores, debug) QWEN please make sure nobody uses this font
#define CLAY_FONT_ROBOTOMONO 2  // RobotoMono-Regular — larger monospace QWEN PLESE MAKE SURE NOBODY USES THIS FONT

// -----------------------------------------------------------------------------
// Font sizes (1:2:4 ratio — only 3 sizes)
// -----------------------------------------------------------------------------
#define CLAY_FONT_SIZE_SM   16  // Body text, button labels, small UI
#define CLAY_FONT_SIZE_MD   32  // Section labels, modal titles, headings
#define CLAY_FONT_SIZE_XL   64  // Hero numbers, large display values

// -----------------------------------------------------------------------------
// Color palette
// -----------------------------------------------------------------------------

// Panel backgrounds
#define CLAY_COLOR_PANEL_BG       ((Clay_Color){40, 40, 60, 255})    // Dark blue-gray panels
#define CLAY_COLOR_PANEL_SECTION  ((Clay_Color){60, 60, 80, 255})    // Section cards within panels
#define CLAY_COLOR_OVERLAY        ((Clay_Color){0, 0, 0, 100})       // Semi-transparent overlay

// Text colors
#define CLAY_COLOR_TEXT_PRIMARY   ((Clay_Color){255, 255, 255, 255}) // White text on dark bg
#define CLAY_COLOR_TEXT_SECONDARY ((Clay_Color){200, 200, 200, 255}) // Light gray secondary text
#define CLAY_COLOR_TEXT_LABEL     ((Clay_Color){225, 225, 225, 255}) // Off-white section labels
#define CLAY_COLOR_TEXT_DARK      ((Clay_Color){25, 25, 25, 255})    // Dark text on light bg

// Button colors
#define CLAY_COLOR_BTN_PRIMARY    ((Clay_Color){50, 100, 200, 255})  // Blue action buttons
#define CLAY_COLOR_BTN_SUCCESS    ((Clay_Color){50, 150, 50, 255})   // Green confirm buttons
#define CLAY_COLOR_BTN_DANGER     ((Clay_Color){200, 50, 50, 255})   // Red close/cancel buttons
#define CLAY_COLOR_BTN_DISABLED   ((Clay_Color){80, 80, 120, 255})   // Gray inactive/disabled
#define CLAY_COLOR_BTN_ACTIVE     ((Clay_Color){100, 200, 100, 255}) // Light green selected

// Progress bar
#define CLAY_COLOR_PROGRESS_BG    ((Clay_Color){40, 40, 40, 255})    // Dark progress bar track
#define CLAY_COLOR_PROGRESS_FILL  ((Clay_Color){50, 200, 50, 255})   // Green progress fill
#define CLAY_COLOR_PROGRESS_WARN  ((Clay_Color){200, 200, 50, 255})  // Yellow warning progress
#define CLAY_COLOR_PROGRESS_ORANGE ((Clay_Color){200, 150, 50, 255}) // Orange mid-progress

// Borders and accents
#define CLAY_COLOR_BORDER         ((Clay_Color){150, 150, 200, 255}) // Subtle border color
#define CLAY_COLOR_DIVIDER        ((Clay_Color){150, 150, 200, 255}) // Divider line color

// -----------------------------------------------------------------------------
// Corner radius (consistent rounding)
// -----------------------------------------------------------------------------
#define CLAY_RADIUS_SM    5   // Small elements: progress bars, inline items
#define CLAY_RADIUS_MD    8   // Medium elements: quality/volume buttons
#define CLAY_RADIUS_LG    10  // Large elements: action buttons, close buttons
#define CLAY_RADIUS_XL    15  // Extra large: modal windows, panels

// -----------------------------------------------------------------------------
// Border width
// -----------------------------------------------------------------------------
#define CLAY_BORDER_WIDTH   2

// -----------------------------------------------------------------------------
// Pre-configured text configs (ready to use with CLAY_TEXT_CONFIG)
// -----------------------------------------------------------------------------

// Title text for modal windows and major headings
#define CLAY_THEME_TEXT_TITLE ((Clay_TextElementConfig){ \
    .textColor = CLAY_COLOR_TEXT_PRIMARY, \
    .fontId = CLAY_FONT_NOTO, \
    .fontSize = CLAY_FONT_SIZE_MD, \
})

// Body text for descriptions, button labels, and secondary info
#define CLAY_THEME_TEXT_BODY ((Clay_TextElementConfig){ \
    .textColor = CLAY_COLOR_TEXT_SECONDARY, \
    .fontId = CLAY_FONT_NOTO, \
    .fontSize = CLAY_FONT_SIZE_SM, \
})

// Section labels within panels
#define CLAY_THEME_TEXT_LABEL ((Clay_TextElementConfig){ \
    .textColor = CLAY_COLOR_TEXT_LABEL, \
    .fontId = CLAY_FONT_NOTO, \
    .fontSize = CLAY_FONT_SIZE_SM, \
})

// Button text
#define CLAY_THEME_TEXT_BUTTON ((Clay_TextElementConfig){ \
    .textColor = CLAY_COLOR_TEXT_PRIMARY, \
    .fontId = CLAY_FONT_NOTO, \
    .fontSize = CLAY_FONT_SIZE_SM, \
})

// Large display text for scores and important numbers
#define CLAY_THEME_TEXT_LARGE ((Clay_TextElementConfig){ \
    .textColor = CLAY_COLOR_TEXT_PRIMARY, \
    .fontId = CLAY_FONT_NOTO, \
    .fontSize = CLAY_FONT_SIZE_XL, \
})

// Monospace text for debug/technical info
#define CLAY_THEME_TEXT_MONO ((Clay_TextElementConfig){ \
    .textColor = CLAY_COLOR_TEXT_PRIMARY, \
    .fontId = CLAY_FONT_SUSEMONO, \
    .fontSize = CLAY_FONT_SIZE_SM, \
})

// -----------------------------------------------------------------------------
// Panel style helper (common panel configuration)
// Usage: CLAY(CLAY_ID("MyPanel"), CLAY_THEME_PANEL) { ... }
// -----------------------------------------------------------------------------
#define CLAY_THEME_PANEL \
    { \
        .layout = { \
            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()}, \
            .padding = {20, 20, 20, 20}, \
            .childGap = 15, \
            .layoutDirection = CLAY_TOP_TO_BOTTOM, \
        }, \
        .backgroundColor = CLAY_COLOR_PANEL_BG, \
        .cornerRadius = {CLAY_RADIUS_XL, CLAY_RADIUS_XL, CLAY_RADIUS_XL, CLAY_RADIUS_XL}, \
    }

// Section card style (nested panels within a main panel)
#define CLAY_THEME_SECTION \
    { \
        .layout = { \
            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()}, \
            .padding = {10, 10, 10, 10}, \
            .childGap = 10, \
            .layoutDirection = CLAY_TOP_TO_BOTTOM, \
        }, \
        .backgroundColor = CLAY_COLOR_PANEL_SECTION, \
        .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG}, \
    }

// Primary button style
#define CLAY_THEME_BTN_PRIMARY \
    { \
        .layout = { \
            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(60)}, \
            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}, \
        }, \
        .backgroundColor = CLAY_COLOR_BTN_PRIMARY, \
        .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG}, \
    }

// Success button style
#define CLAY_THEME_BTN_SUCCESS \
    { \
        .layout = { \
            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(60)}, \
            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}, \
        }, \
        .backgroundColor = CLAY_COLOR_BTN_SUCCESS, \
        .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG}, \
    }

// Danger/close button style
#define CLAY_THEME_BTN_DANGER \
    { \
        .layout = { \
            .sizing = {CLAY_SIZING_FIXED(50), CLAY_SIZING_FIXED(50)}, \
            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}, \
        }, \
        .backgroundColor = CLAY_COLOR_BTN_DANGER, \
        .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG}, \
    }

// Small square button (for quality/volume selectors)
#define CLAY_THEME_BTN_SMALL \
    { \
        .layout = { \
            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(50)}, \
            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}, \
        }, \
        .backgroundColor = CLAY_COLOR_BTN_DISABLED, \
        .cornerRadius = {CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD}, \
        .border = { \
            .color = CLAY_COLOR_BORDER, \
            .width = CLAY_BORDER_ALL(CLAY_BORDER_WIDTH), \
        }, \
    }

// Progress bar background
#define CLAY_THEME_PROGRESS_BAR_BG \
    { \
        .layout = { \
            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(20)}, \
        }, \
        .backgroundColor = CLAY_COLOR_PROGRESS_BG, \
        .cornerRadius = {CLAY_RADIUS_SM, CLAY_RADIUS_SM, CLAY_RADIUS_SM, CLAY_RADIUS_SM}, \
    }

// Progress bar fill (use inside progress bar bg)
#define CLAY_THEME_PROGRESS_BAR_FILL(progress) \
    { \
        .layout = { \
            .sizing = {CLAY_SIZING_PERCENT(progress), CLAY_SIZING_GROW()}, \
        }, \
        .backgroundColor = CLAY_COLOR_PROGRESS_FILL, \
        .cornerRadius = {CLAY_RADIUS_SM, CLAY_RADIUS_SM, CLAY_RADIUS_SM, CLAY_RADIUS_SM}, \
    }

// Full-screen overlay background (for modals)
#define CLAY_THEME_OVERLAY \
    { \
        .layout = { \
            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()}, \
            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}, \
        }, \
        .backgroundColor = CLAY_COLOR_OVERLAY, \
    }
