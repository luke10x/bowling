#pragma once
// -----------------------------------------------------------------------------
// Bowling Game - Sound Data
// 
// This file contains all sound definitions:
// - YM2612/OPN2 patches
// - Song patterns (4 songs)
// - SFX patterns (6 sound effects)
//
// This file can be included from:
// - sounds.h (for the game) - xfm_api.h must be included first
// - game-wav-exporter.cpp (for native WAV export) - xfm_api.h must be included first
// -----------------------------------------------------------------------------

// Note: xfm_api.h must be included before this file

// -----------------------------------------------------------------------------
// YM2612/OPN2 Patches
// -----------------------------------------------------------------------------

constexpr xfm_patch_opn PATCH_00_RUBBER_BASS =
{
    .ALG = 2,
    .FB  = 5,
    .AMS = 0,
    .FMS = 0,

    .op =
    {
        { .DT = 1, .MUL = 3, .TL = 38, .RS = 0, .AR = 12, .AM = 0, .DR = 7, .SR = 11, .SL = 4, .RR = 6, .SSG = 0 },
        { .DT = -1, .MUL = 1, .TL = 38, .RS = 0, .AR = 17, .AM = 0, .DR = 5, .SR = 2, .SL = 2, .RR = 1, .SSG = 0 },
        { .DT = 1, .MUL = 2, .TL = 5,  .RS = 0, .AR = 11, .AM = 0, .DR = 13, .SR = 11, .SL = 5, .RR = 13, .SSG = 0 },
        { .DT = -1, .MUL = 1, .TL = 0,  .RS = 0, .AR = 31, .AM = 0, .DR = 9,  .SR = 15, .SL = 5, .RR = 8,  .SSG = 3 }
    }
};

constexpr xfm_patch_opn PATCH_01_HOLLOW_ELECTRIC =
{
    .ALG = 4,
    .FB  = 6,
    .AMS = 0,
    .FMS = 0,
    .op =
    {
        { .DT = 0, .MUL = 3, .TL = 35, .RS = 0, .AR = 13, .AM = 0, .DR = 1,  .SR = 25, .SL = 2, .RR = 0, .SSG = 0 },
        { .DT = 0, .MUL = 1, .TL = 20, .RS = 0, .AR = 17, .AM = 0, .DR = 10, .SR = 8,  .SL = 2, .RR = 7, .SSG = 0 },
        { .DT = 0, .MUL = 1, .TL = 11, .RS = 0, .AR = 8,  .AM = 0, .DR = 4,  .SR = 23, .SL = 7, .RR = 1, .SSG = 0 },
        { .DT = 0, .MUL = 1, .TL = 14, .RS = 0, .AR = 25, .AM = 0, .DR = 0,  .SR = 10, .SL = 0, .RR = 9, .SSG = 0 }
    }
};

// Hi-hat patch (for channel 2)
constexpr xfm_patch_opn PATCH_02_ANGRY_HIHAT =
{
    .ALG = 7,
    .FB  = 7,
    .AMS = 0,
    .FMS = 0,
    .op =
    {
        { .DT = 3, .MUL = 13, .TL =  8, .RS = 3, .AR = 31, .AM = 0, .DR = 31, .SR =  0, .SL = 15, .RR = 15, .SSG = 0 },
        { .DT = 2, .MUL = 11, .TL = 12, .RS = 3, .AR = 31, .AM = 0, .DR = 31, .SR =  0, .SL = 15, .RR = 15, .SSG = 0 },
        { .DT = 1, .MUL =  7, .TL = 16, .RS = 3, .AR = 31, .AM = 0, .DR = 30, .SR =  0, .SL = 15, .RR = 14, .SSG = 0 },
        { .DT = 0, .MUL = 15, .TL = 20, .RS = 3, .AR = 31, .AM = 0, .DR = 29, .SR =  0, .SL = 15, .RR = 13, .SSG = 0 }
    }
};

constexpr xfm_patch_opn PATCH_14_GLASS_CRACK =
{
    .ALG = 7,
    .FB  = 7,
    .AMS = 0,
    .FMS = 7,
    .op =
    {
        { .DT = -3, .MUL = 15, .TL =  0, .RS = 3, .AR = 31, .AM = 1, .DR = 31, .SR = 0, .SL = 15, .RR = 15, .SSG = 8 },
        { .DT = -1, .MUL = 11, .TL = 10, .RS = 3, .AR = 31, .AM = 1, .DR = 30, .SR = 0, .SL = 15, .RR = 14, .SSG = 7 },
        { .DT =  2, .MUL = 13, .TL =  4, .RS = 3, .AR = 31, .AM = 1, .DR = 31, .SR = 0, .SL = 15, .RR = 15, .SSG = 6 },
        { .DT =  3, .MUL =  9, .TL =  0, .RS = 3, .AR = 31, .AM = 1, .DR = 31, .SR = 0, .SL = 15, .RR = 15, .SSG = 5 }
    }
};

constexpr xfm_patch_opn PATCH_15_GLASS_SCRAPE =
{
    .ALG = 5,
    .FB  = 7,
    .AMS = 0,
    .FMS = 5,
    .op =
    {
        { .DT = -3, .MUL = 14, .TL = 18, .RS = 3, .AR = 31, .AM = 1, .DR = 12, .SR = 18, .SL = 4, .RR = 11, .SSG = 3 },
        { .DT =  3, .MUL = 10, .TL =  8, .RS = 3, .AR = 31, .AM = 1, .DR = 16, .SR = 20, .SL = 5, .RR = 12, .SSG = 4 },
        { .DT = -1, .MUL =  7, .TL = 12, .RS = 3, .AR = 31, .AM = 1, .DR = 18, .SR = 22, .SL = 6, .RR = 12, .SSG = 5 },
        { .DT =  2, .MUL = 15, .TL =  0, .RS = 3, .AR = 31, .AM = 1, .DR = 10, .SR = 16, .SL = 5, .RR = 13, .SSG = 6 }
    }
};

constexpr xfm_patch_opn PATCH_16_GLASS_SHARD =
{
    .ALG = 7,
    .FB  = 6,
    .AMS = 0,
    .FMS = 6,
    .op =
    {
        { .DT = -2, .MUL =  8, .TL = 16, .RS = 3, .AR = 31, .AM = 1, .DR = 22, .SR = 0, .SL = 15, .RR = 12, .SSG = 0 },
        { .DT =  1, .MUL = 12, .TL = 12, .RS = 3, .AR = 31, .AM = 1, .DR = 26, .SR = 0, .SL = 15, .RR = 14, .SSG = 0 },
        { .DT =  3, .MUL = 15, .TL =  8, .RS = 3, .AR = 31, .AM = 1, .DR = 24, .SR = 0, .SL = 15, .RR = 13, .SSG = 0 },
        { .DT = -3, .MUL = 11, .TL =  2, .RS = 3, .AR = 31, .AM = 1, .DR = 20, .SR = 0, .SL = 15, .RR = 12, .SSG = 0 }
    }
};

constexpr xfm_patch_opn PATCH_17_NOS_PAD =
{
    .ALG = 7,
    .FB  = 6,
    .AMS = 1,
    .FMS = 5,
    .op =
    {
        { .DT = -3, .MUL = 1, .TL = 12, .RS = 1, .AR = 27, .AM = 1, .DR = 5, .SR = 0, .SL = 14, .RR = 4, .SSG = 0 },
        { .DT =  2, .MUL = 3, .TL = 22, .RS = 1, .AR = 24, .AM = 1, .DR = 9, .SR = 0, .SL = 15, .RR = 5, .SSG = 8 },
        { .DT = -1, .MUL = 7, .TL = 30, .RS = 1, .AR = 28, .AM = 1, .DR = 7, .SR = 0, .SL = 14, .RR = 4, .SSG = 0 },
        { .DT =  1, .MUL = 1, .TL =  0, .RS = 1, .AR = 31, .AM = 1, .DR = 3, .SR = 0, .SL = 14, .RR = 3, .SSG = 0 }
    }
};

constexpr xfm_patch_opn PATCH_03_GUITAR =
{
    .ALG = 3,
    .FB  = 4,
    .AMS = 0,
    .FMS = 0,

    .op =
    {
        { .DT = 3, .MUL = 15, .TL = 61, .RS = 0, .AR = 11, .AM = 0, .DR = 0, .SR = 0, .SL = 10, .RR = 0, .SSG = 0 },
        { .DT = 3, .MUL = 1, .TL = 0, .RS = 0, .AR = 21, .AM = 0, .DR = 18, .SR = 0, .SL = 2, .RR = 4, .SSG = 0 },
        { .DT = -2, .MUL = 7, .TL = 19, .RS = 0, .AR = 31, .AM = 0, .DR = 31, .SR = 0, .SL = 15, .RR = 9, .SSG = 1 },
        { .DT = 0, .MUL = 2, .TL = 6, .RS = 0, .AR = 21, .AM = 0, .DR = 5, .SR = 0, .SL = 1, .RR = 5, .SSG = 0 }
    }
};

constexpr xfm_patch_opn PATCH_04_SAW =
{
    .ALG = 7,
    .FB  = 5,
    .AMS = 0,
    .FMS = 0,

    .op =
    {
        { .DT = -2, .MUL = 1, .TL = 8, .RS = 0, .AR = 31, .AM = 0, .DR = 10, .SR = 0, .SL = 0, .RR = 6, .SSG = 0 },
        { .DT = 0, .MUL = 1, .TL = 10, .RS = 0, .AR = 31, .AM = 0, .DR = 10, .SR = 0, .SL = 15, .RR = 6, .SSG = 0 },
        { .DT = 2, .MUL = 1, .TL = 8, .RS = 0, .AR = 31, .AM = 0, .DR = 10, .SR = 0, .SL = 0, .RR = 6, .SSG = 0 },
        { .DT = 0, .MUL = 2, .TL = 18, .RS = 0, .AR = 31, .AM = 0, .DR = 12, .SR = 0, .SL = 0, .RR = 6, .SSG = 0 }
    }
};

constexpr xfm_patch_opn PATCH_05_FLUTE =
{
    .ALG = 4,
    .FB  = 5,
    .AMS = 0,
    .FMS = 0,

    .op =
    {
        { .DT = 0, .MUL = 1, .TL = 63, .RS = 0, .AR = 31, .AM = 0, .DR = 5, .SR = 0, .SL = 1, .RR = 10, .SSG = 0 },
        { .DT = 3, .MUL = 1, .TL = 0, .RS = 0, .AR = 31, .AM = 0, .DR = 16, .SR = 0, .SL = 1, .RR = 10, .SSG = 0 },
        { .DT = 0, .MUL = 1, .TL = 63, .RS = 0, .AR = 31, .AM = 0, .DR = 5, .SR = 0, .SL = 1, .RR = 10, .SSG = 0 },
        { .DT = 0, .MUL = 1, .TL = 0, .RS = 0, .AR = 0, .AM = 0, .DR = 5, .SR = 0, .SL = 1, .RR = 10, .SSG = 0 }
    }
};

constexpr xfm_patch_opn PATCH_06_FOOTBALL_KICK =
{
    .ALG = 2,
    .FB  = 5,
    .AMS = 0,
    .FMS = 0,

    .op =
    {
        { .DT = 0, .MUL = 0, .TL = 15, .RS = 2, .AR = 31, .AM = 0, .DR = 7, .SR = 16, .SL = 5, .RR = 4, .SSG = 0 },
        { .DT = 1, .MUL = 2, .TL = 0, .RS = 1, .AR = 31, .AM = 0, .DR = 30, .SR = 31, .SL = 14, .RR = 15, .SSG = 0 },
        { .DT = 0, .MUL = 1, .TL = 29, .RS = 1, .AR = 13, .AM = 0, .DR = 15, .SR = 31, .SL = 1, .RR = 10, .SSG = 0 },
        { .DT = 0, .MUL = 1, .TL = 0, .RS = 1, .AR = 31, .AM = 0, .DR = 23, .SR = 31, .SL = 9, .RR = 10, .SSG = 0 }
    }
};

constexpr xfm_patch_opn PATCH_07_SNARE =
{
    .ALG = 0,
    .FB  = 6,
    .AMS = 0,
    .FMS = 0,

    .op =
    {
        { .DT = 3, .MUL = 12, .TL = 11, .RS = 3, .AR = 31, .AM = 0, .DR = 15, .SR = 19, .SL = 4, .RR = 10, .SSG = 0 },
        { .DT = -3, .MUL = 9, .TL = 6, .RS = 3, .AR = 25, .AM = 0, .DR = 7, .SR = 5, .SL = 5, .RR = 9, .SSG = 0 },
        { .DT = 1, .MUL = 3, .TL = 19, .RS = 2, .AR = 29, .AM = 0, .DR = 13, .SR = 22, .SL = 13, .RR = 8, .SSG = 0 },
        { .DT = 0, .MUL = 1, .TL = 0, .RS = 1, .AR = 30, .AM = 0, .DR = 20, .SR = 21, .SL = 10, .RR = 11, .SSG = 0 }
    }
};

constexpr xfm_patch_opn PATCH_08_HIHAT =
{
    .ALG = 2,
    .FB  = 4,
    .AMS = 0,
    .FMS = 0,

    .op =
    {
        { .DT = 0, .MUL = 12, .TL = 0, .RS = 2, .AR = 28, .AM = 0, .DR = 7, .SR = 23, .SL = 5, .RR = 12, .SSG = 0 },
        { .DT = 1, .MUL = 12, .TL = 0, .RS = 1, .AR = 20, .AM = 0, .DR = 31, .SR = 31, .SL = 8, .RR = 10, .SSG = 0 },
        { .DT = 0, .MUL = 3, .TL = 10, .RS = 1, .AR = 8, .AM = 0, .DR = 15, .SR = 31, .SL = 1, .RR = 10, .SSG = 0 },
        { .DT = 0, .MUL = 5, .TL = 0, .RS = 1, .AR = 23, .AM = 0, .DR = 23, .SR = 31, .SL = 9, .RR = 10, .SSG = 0 }
    }
};

constexpr xfm_patch_opn PATCH_09_WAH = 
{
    .ALG = 4,
    .FB  = 1,
    .AMS = 0,
    .FMS = 0,

    .op =
    {
        { .DT = 0, .MUL = 0, .TL = 13, .RS = 0, .AR = 9, .AM = 0, .DR = 13, .SR = 0, .SL = 0, .RR = 15, .SSG = 0 },
        { .DT = 0, .MUL = 1, .TL = 21, .RS = 0, .AR = 21, .AM = 0, .DR = 9, .SR = 0, .SL = 2, .RR = 15, .SSG = 0 },
        { .DT = 0, .MUL = 1, .TL = 19, .RS = 0, .AR = 31, .AM = 0, .DR = 4, .SR = 0, .SL = 11, .RR = 4, .SSG = 0 },
        { .DT = -3, .MUL = 1, .TL = 58, .RS = 0, .AR = 31, .AM = 0, .DR = 9, .SR = 0, .SL = 15, .RR = 9, .SSG = 0 }
    }
};

constexpr xfm_patch_opn PATCH_0A_GUITAR2 = {
    .ALG = 0,
    .FB  = 7,
    .AMS = 3,
    .FMS = 3,

    .op =
    {
        { .DT = 0, .MUL = 5, .TL = 8, .RS = 0, .AR = 25, .AM = 1, .DR = 15, .SR = 13, .SL = 5, .RR = 10, .SSG = 0 },
        { .DT = 0, .MUL = 0, .TL = 24, .RS = 0, .AR = 25, .AM = 0, .DR = 10, .SR = 10, .SL = 5, .RR = 13, .SSG = 0 },
        { .DT = -3, .MUL = 0, .TL = 12, .RS = 0, .AR = 26, .AM = 0, .DR = 9, .SR = 25, .SL = 5, .RR = 14, .SSG = 0 },
        { .DT = 0, .MUL = 0, .TL = 28, .RS = 0, .AR = 19, .AM = 0, .DR = 13, .SR = 20, .SL = 7, .RR = 15, .SSG = 0 }
    }
};

constexpr xfm_patch_opn PATCH_0B_BASS_KICK = {
    .ALG = 4,
    .FB  = 4,
    .AMS = 0,
    .FMS = 0,

    .op =
    {
        { .DT = -2, .MUL = 0, .TL = 49, .RS = 1, .AR = 31, .AM = 0, .DR = 0, .SR = 31, .SL = 15, .RR = 15, .SSG = 0 },
        { .DT = 0, .MUL = 0, .TL = 0, .RS = 1, .AR = 29, .AM = 0, .DR = 15, .SR = 31, .SL = 9, .RR = 15, .SSG = 0 },
        { .DT = -3, .MUL = 0, .TL = 15, .RS = 1, .AR = 13, .AM = 0, .DR = 0, .SR = 0, .SL = 1, .RR = 7, .SSG = 0 },
        { .DT = 3, .MUL = 1, .TL = 71, .RS = 1, .AR = 28, .AM = 0, .DR = 18, .SR = 27, .SL = 5, .RR = 15, .SSG = 0 }
    }
};

constexpr xfm_patch_opn PATCH_0C_TSH = {
    .ALG = 0,
    .FB  = 0,
    .AMS = 0,
    .FMS = 0,

    .op =
    {
        { .DT = 0, .MUL = 15, .TL = 7, .RS = 0, .AR = 31, .AM = 0, .DR = 0, .SR = 3, .SL = 0, .RR = 15, .SSG = 0 },
        { .DT = 0, .MUL = 15, .TL = 4, .RS = 0, .AR = 31, .AM = 0, .DR = 0, .SR = 1, .SL = 0, .RR = 3, .SSG = 0 },
        { .DT = 0, .MUL = 15, .TL = 8, .RS = 0, .AR = 24, .AM = 0, .DR = 3, .SR = 31, .SL = 5, .RR = 15, .SSG = 0 },
        { .DT = 0, .MUL = 15, .TL = 20, .RS = 0, .AR = 31, .AM = 0, .DR = 14, .SR = 20, .SL = 8, .RR = 13, .SSG = 0 }
    }
};

constexpr xfm_patch_opn PATCH_0D_TICK = 
{
    .ALG = 2,
    .FB  = 0,
    .AMS = 0,
    .FMS = 0,

    .op =
    {
        { .DT = 0, .MUL = 11, .TL = 37, .RS = 0, .AR = 24, .AM = 0, .DR = 25, .SR = 31, .SL = 12, .RR = 15, .SSG = 0 },
        { .DT = 0, .MUL = 12, .TL = 9, .RS = 0, .AR = 24, .AM = 0, .DR = 25, .SR = 31, .SL = 12, .RR = 15, .SSG = 0 },
        { .DT = 0, .MUL = 11, .TL = 23, .RS = 0, .AR = 24, .AM = 0, .DR = 25, .SR = 31, .SL = 12, .RR = 15, .SSG = 0 },
        { .DT = 0, .MUL = 15, .TL = 34, .RS = 0, .AR = 24, .AM = 0, .DR = 25, .SR = 31, .SL = 10, .RR = 15, .SSG = 0 }
    }
};

constexpr xfm_patch_opn PATCH_0E_LEAD = { 
    .ALG = 7,
    .FB  = 5,
    .AMS = 0,
    .FMS = 0,

    .op =
    {
        { .DT = -2, .MUL = 1, .TL = 8, .RS = 0, .AR = 31, .AM = 0, .DR = 10, .SR = 0, .SL = 0, .RR = 6, .SSG = 0 },
        { .DT = 0, .MUL = 1, .TL = 10, .RS = 0, .AR = 31, .AM = 0, .DR = 10, .SR = 0, .SL = 15, .RR = 6, .SSG = 0 },
        { .DT = 2, .MUL = 1, .TL = 8, .RS = 0, .AR = 31, .AM = 0, .DR = 10, .SR = 0, .SL = 0, .RR = 6, .SSG = 0 },
        { .DT = 0, .MUL = 2, .TL = 18, .RS = 0, .AR = 31, .AM = 0, .DR = 12, .SR = 0, .SL = 0, .RR = 6, .SSG = 0 }
    }
};

constexpr xfm_patch_opn PATCH_0F_KICK = 
{
    .ALG = 4,
    .FB  = 1,
    .AMS = 0,
    .FMS = 0,

    .op =
    {
        { .DT = -2, .MUL = 4, .TL = 0, .RS = 0, .AR = 31, .AM = 0, .DR = 27, .SR = 6, .SL = 12, .RR = 10, .SSG = 0 },
        { .DT = 0, .MUL = 0, .TL = 0, .RS = 1, .AR = 29, .AM = 0, .DR = 15, .SR = 29, .SL = 9, .RR = 15, .SSG = 0 },
        { .DT = -3, .MUL = 0, .TL = 10, .RS = 1, .AR = 23, .AM = 0, .DR = 25, .SR = 23, .SL = 10, .RR = 4, .SSG = 0 },
        { .DT = 3, .MUL = 1, .TL = 0, .RS = 0, .AR = 31, .AM = 0, .DR = 30, .SR = 23, .SL = 1, .RR = 0, .SSG = 0 }
    }
};

constexpr xfm_patch_opn PATCH_10_HARDBASS =
{
    .ALG = 3,
    .FB  = 4,
    .AMS = 1,
    .FMS = 2,

    .op =
    {
        { .DT = 0, .MUL = 5, .TL = 10, .RS = 1, .AR = 5, .AM = 1, .DR = 27, .SR = 20, .SL = 1, .RR = 11, .SSG = 0 },
        { .DT = 0, .MUL = 4, .TL = 37, .RS = 0, .AR = 31, .AM = 0, .DR = 8, .SR = 23, .SL = 2, .RR = 0, .SSG = 0 },
        { .DT = -1, .MUL = 3, .TL = 13, .RS = 0, .AR = 13, .AM = 0, .DR = 12, .SR = 31, .SL = 3, .RR = 15, .SSG = 0 },
        { .DT = 1, .MUL = 0, .TL = 0, .RS = 0, .AR = 21, .AM = 0, .DR = 3, .SR = 0, .SL = 4, .RR = 15, .SSG = 0 }
    }
};

constexpr xfm_patch_opn PATCH_11_LOWBASS = {
    .ALG = 4,
    .FB  = 7,
    .AMS = 0,
    .FMS = 0,

    .op =
    {
        { .DT = 0, .MUL = 5, .TL = 0, .RS = 0, .AR = 0, .AM = 0, .DR = 8, .SR = 0, .SL = 15, .RR = 3, .SSG = 0 },
        { .DT = 0, .MUL = 0, .TL = 20, .RS = 0, .AR = 13, .AM = 0, .DR = 10, .SR = 0, .SL = 15, .RR = 4, .SSG = 0 },
        { .DT = 0, .MUL = 0, .TL = 0, .RS = 0, .AR = 31, .AM = 0, .DR = 24, .SR = 0, .SL = 3, .RR = 1, .SSG = 1 },
        { .DT = 0, .MUL = 0, .TL = 16, .RS = 0, .AR = 31, .AM = 0, .DR = 9, .SR = 0, .SL = 2, .RR = 10, .SSG = 0 }
    }
};
constexpr xfm_patch_opn PATCH_12_AXE =
{
    .ALG = 4,
    .FB  = 2,
    .AMS = 3,
    .FMS = 0,

    .op =
    {
        { .DT = -3, .MUL = 1, .TL = 0, .RS = 0, .AR = 28, .AM = 1, .DR = 20, .SR = 24, .SL = 12, .RR = 15, .SSG = 0 },
        { .DT = 2, .MUL = 4, .TL = 37, .RS = 0, .AR = 28, .AM = 0, .DR = 12, .SR = 27, .SL = 14, .RR = 15, .SSG = 0 },
        { .DT = -2, .MUL = 2, .TL = 8, .RS = 1, .AR = 25, .AM = 1, .DR = 23, .SR = 5, .SL = 9, .RR = 15, .SSG = 0 },
        { .DT = 3, .MUL = 1, .TL = 6, .RS = 0, .AR = 27, .AM = 0, .DR = 25, .SR = 10, .SL = 2, .RR = 15, .SSG = 0 }
    }
};

constexpr xfm_patch_opn PATCH_13_ROLL =
{
    .ALG = 2,
    .FB  = 0,
    .AMS = 0,
    .FMS = 0,

    .op =
    {
        { .DT = 1, .MUL = 15, .TL = 127, .RS = 0, .AR = 17, .AM = 0, .DR = 10, .SR = 11, .SL = 4, .RR = 6, .SSG = 0 },
        { .DT = -1, .MUL = 1, .TL = 10, .RS = 0, .AR = 26, .AM = 0, .DR = 0, .SR = 2, .SL = 2, .RR = 1, .SSG = 0 },
        { .DT = 1, .MUL = 0, .TL = 127, .RS = 0, .AR = 0, .AM = 0, .DR = 13, .SR = 11, .SL = 5, .RR = 13, .SSG = 0 },
        { .DT = -1, .MUL = 1, .TL = 0, .RS = 0, .AR = 31, .AM = 0, .DR = 9, .SR = 16, .SL = 5, .RR = 8, .SSG = 3 }
    }
};

// Rewures big AMS to work
// -----------------------------------------------------------------------------
// SFX Patterns
// -----------------------------------------------------------------------------

// Ball hitting the lane
// (Heavier "thud" — used for actual ball<->lane contacts, including rebounds.)
constexpr const char* SFX_PAT_BALL_HIT_LANE = "6\n"
                                               "A-2007F\n"
                                               ".......\n"
                                               "OFF....\n"
                                               ".......\n"
                                               ".......\n"
                                               ".......\n";

constexpr const char* SFX_PAT_BALL_HIT_PINS = "6\n"
                                               "A-2122F\n"
                                               ".......\n"
                                               ".......\n"
                                               ".......\n"
                                               "OFF....\n"
                                               ".......\n";

// Pin hitting another pin
constexpr const char* SFX_PAT_PIN_HIT_PIN = "4\n"
                                             "A-3007F\n"
                                             "OFF....\n"
                                             ".......\n"
                                             ".......\n";

// Score display fanfare
constexpr const char* SFX_PAT_SCORE_DISPLAY = "8\n"
                                               "C-3007F\n"
                                               "E-3007F\n"
                                               "G-3007F\n"
                                               "C-4007F\n"
                                               "OFF....\n"
                                               ".......\n"
                                               ".......\n"
                                               ".......\n";

// Typewriter tick (audible but still subtle; used for dialog text typing).
// A short 2-step blip with a higher pitch so it cuts through music.
constexpr const char* SFX_PAT_TYPEWRITER = "4\n"
                                            "G-6007F\n"
                                            ".......\n"
                                            "OFF....\n"
                                            ".......\n";

// Ball in gutter
constexpr const char* SFX_PAT_GUTTER = "8\n"
                                        "A-2007F\n"
                                        "F-2007F\n"
                                        "D-2007F\n"
                                        "OFF....\n"
                                        ".......\n"
                                        ".......\n"
                                        ".......\n"
                                        ".......\n";

// Timeout warning
constexpr const char* SFX_PAT_TIMEOUT = "6\n"
                                         "D-4007F\n"
                                         "OFF....\n"
                                         "A-3007F\n"
                                         "OFF....\n"
                                         ".......\n"
                                         ".......\n";

// Coin pickup - bright ascending blip
constexpr const char* SFX_PAT_COIN_PICKUP = "4\n"
                                             "E-5007F\n"
                                             "G-5007F\n"
                                             "OFF....\n"
                                             ".......\n";

// Spare - uses the old strike fanfare (short rising confirmation)
constexpr const char* SFX_PAT_SPARE = "10\n"
                                       "C-4007F\n"
                                       "E-4007F\n"
                                       "G-4007F\n"
                                       "C-5007F\n"
                                       "E-5007F\n"
                                       "G-5007F\n"
                                       "C-6007F\n"
                                       "OFF....\n"
                                       ".......\n"
                                       ".......\n";

// Strike - bigger "ta-ra-ra ta-daaa" style fanfare
constexpr const char* SFX_PAT_STRIKE = "14\n"
                                        "C-4007F\n"
                                        ".......\n"
                                        "E-4007F\n"
                                        ".......\n"
                                        "G-4007F\n"
                                        ".......\n"
                                        "C-5007F\n"
                                        "E-5007F\n"
                                        "G-5007F\n"
                                        "C-6007F\n"
                                        ".......\n"
                                        "OFF....\n"
                                        ".......\n"
                                        ".......\n";

// Neutral roll - subtle confirmation blip (used for normal scoring rolls)
constexpr const char* SFX_PAT_NEUTRAL_ROLL = "5\n"
                                              "C-3007F\n"
                                              "E-3007F\n"
                                              "OFF....\n"
                                              ".......\n"
                                              ".......\n";

constexpr const char* SFX_PAT_GLASS_CRACK = "8\n"
                                            "C-8147F\n"
                                            "F#7147F\n"
                                            "A#7147F\n"
                                            "D-8147F\n"
                                            "===....\n"
                                            ".......\n"
                                            ".......\n"
                                            ".......\n";

constexpr const char* SFX_PAT_GLASS_SCRAPE = "18\n"
                                             ".......\n"
                                             "B-7157F\n"
                                             "A#7157F\n"
                                             "G-7157F\n"
                                             "F#7157F\n"
                                             "E-7157F\n"
                                             "D#7157F\n"
                                             "C#7157F\n"
                                             "B-6157F\n"
                                             "A-6157F\n"
                                             "G#6157F\n"
                                             "F-6157F\n"
                                             "E-6157F\n"
                                             "REL....\n"
                                             ".......\n"
                                             ".......\n"
                                             ".......\n"
                                             ".......\n";

constexpr const char* SFX_PAT_GLASS_SHARDS = "22\n"
                                             ".......\n"
                                             ".......\n"
                                             "D-8167F\n"
                                             ".......\n"
                                             "A#7167F\n"
                                             ".......\n"
                                             "F#7167F\n"
                                             ".......\n"
                                             "C#7167F\n"
                                             ".......\n"
                                             "G-6167F\n"
                                             ".......\n"
                                             "D#6167F\n"
                                             ".......\n"
                                             "A-5167F\n"
                                             ".......\n"
                                             "F-5167F\n"
                                             ".......\n"
                                             "C#5167F\n"
                                             "===....\n"
                                             ".......\n"
                                             ".......\n";

#define SFX_ROLL_REST_10 ".......\n.......\n.......\n.......\n.......\n.......\n.......\n.......\n.......\n.......\n"

// Ball rolling on the lane. This is intentionally long and cancellable:
// gameplay starts it on the first lane impact and stops it on pin hit, gutter,
// timeout, or any non-throw phase. The long tail exists for cached WAV mode.
constexpr const char* SFX_PAT_BALL_ROLLING = "240\n"
                                              //"F-1007F\n"
                                              //"C-2007F\n"
                                              "E-2135F\n"
                                              ".......\n"
                                              ".......\n"
                                              ".......\n"
                                              SFX_ROLL_REST_10
                                              SFX_ROLL_REST_10
                                              SFX_ROLL_REST_10
                                              SFX_ROLL_REST_10
                                              SFX_ROLL_REST_10
                                              SFX_ROLL_REST_10
                                              SFX_ROLL_REST_10
                                              SFX_ROLL_REST_10
                                              SFX_ROLL_REST_10
                                              SFX_ROLL_REST_10
                                              SFX_ROLL_REST_10
                                              SFX_ROLL_REST_10
                                              SFX_ROLL_REST_10
                                              SFX_ROLL_REST_10
                                              SFX_ROLL_REST_10
                                              SFX_ROLL_REST_10
                                              SFX_ROLL_REST_10
                                              SFX_ROLL_REST_10
                                              SFX_ROLL_REST_10
                                              SFX_ROLL_REST_10
                                              SFX_ROLL_REST_10
                                              SFX_ROLL_REST_10
                                              SFX_ROLL_REST_10
                                              ".......\n.......\n.......\n.......\n.......\n.......\n.......\n.......\n"
                                              "OFF....\n";

#undef SFX_ROLL_REST_10

#define SFX_NOS_REST_8 ".......\n.......\n.......\n.......\n.......\n.......\n.......\n.......\n"
#define SFX_NOS_REST_64 SFX_NOS_REST_8 SFX_NOS_REST_8 SFX_NOS_REST_8 SFX_NOS_REST_8 SFX_NOS_REST_8 SFX_NOS_REST_8 SFX_NOS_REST_8 SFX_NOS_REST_8
#define SFX_NOS_REST_512 SFX_NOS_REST_64 SFX_NOS_REST_64 SFX_NOS_REST_64 SFX_NOS_REST_64 SFX_NOS_REST_64 SFX_NOS_REST_64 SFX_NOS_REST_64 SFX_NOS_REST_64

// NOS boost loop. This must stay keyed on for the entire boost and only
// release when gameplay stops it, so the pattern uses one note plus empty rows.
constexpr const char* SFX_PAT_NOS_LOOP = "4097\n"
                                         "E-3177F\n"
                                         SFX_NOS_REST_512
                                         SFX_NOS_REST_512
                                         SFX_NOS_REST_512
                                         SFX_NOS_REST_512
                                         SFX_NOS_REST_512
                                         SFX_NOS_REST_512
                                         SFX_NOS_REST_512
                                         SFX_NOS_REST_512;

#undef SFX_NOS_REST_512
#undef SFX_NOS_REST_64
#undef SFX_NOS_REST_8

// Win/Lose fanfares (played once when final score is known).
// Keep them short and readable under the mix.
constexpr const char* SFX_PAT_WIN = "12\n"
                                    "C-4007F\n"
                                    "E-4007F\n"
                                    "G-4007F\n"
                                    "C-5007F\n"
                                    "E-5007F\n"
                                    "G-5007F\n"
                                    "C-6007F\n"
                                    ".......\n"
                                    "OFF....\n"
                                    ".......\n"
                                    ".......\n"
                                    ".......\n";

constexpr const char* SFX_PAT_LOSE = "12\n"
                                     "E-4007F\n"
                                     "D-4007F\n"
                                     "C-4007F\n"
                                     "A-3007F\n"
                                     "F-3007F\n"
                                     "D-3007F\n"
                                     "C-3007F\n"
                                     ".......\n"
                                     "OFF....\n"
                                     ".......\n"
                                     ".......\n"
                                     ".......\n";

// BUY/RE-OIL confirmation (UI purchase) - triumphant ascending arpeggio.
constexpr const char* SFX_PAT_BUY = "12\n"
                                    "C-4017F\n"
                                    "E-4017F\n"
                                    "G-4017F\n"
                                    "C-5017F\n"
                                    "E-5017F\n"
                                    "G-5017F\n"
                                    "C-6017F\n"
                                    ".......\n"
                                    ".......\n"
                                    ".......\n"
                                    "OFF....\n"
                                    ".......\n";

// -----------------------------------------------------------------------------
#include "builtin_songs_compiled.h"
