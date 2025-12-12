#pragma once

#include "../framework/gl_header.h"

#define MAX_IMAGES 4
#define MAX_FONTS 4

/*
 * Instanced rendering for Rects/Images/Borders
 * will use this data
 * Note, it needs to be padded to 4 floats
 * Draws:
 * - One rectangular with possibly rounded corner
 * - And possibly with a hole inside (with rounded edges too, if corners are rounded)
 * - It could also draw a picture with alsoe rounded corner
 */
typedef struct RectInstance
{
    float x, y, w, h;         // 4 Draw where on screen
    float u0, v0, u1, v1;     // 4 Atlas region
    float r, g, b, a;         // 4 Color
    float radiusTL, radiusTR; // 2 Corner rounding
    float radiusBL, radiusBR; // 2
    float borderL, borderR;   // 2 Border widths
    float borderT, borderB;   // 2
    float texToUse;           // 1 Texture atlas to take an image from (1-4)
    float pad[3];             // 3
} RectInstance;

/*
 * Struct for glyph instanced rendering
 * Each glyph consists of 6 vertexes (to make 2 triangle of a quad)
 */
typedef struct GlyphVtx
{
    float x, y;         // To draw Where
    float u, v;         // To draw What
    float r, g, b, a;   // Text color
    float atlasTexUnit; // Shader will have all samples loaded but this will point which to use
    float pad[3];       // 3
} GlyphVtx;

typedef struct Gles3_GlyphVtxArray
{
    GlyphVtx *instData;
    int capacity;
    int count;
} Gles3_GlyphVtxArray;

typedef struct Gles3_QuadInstanceArray
{
    RectInstance *instData; // packed per-instance floats
    int capacity;           // how many instances it can hold
    int count;              // how many instances does it actually hold
} Gles3_QuadInstanceArray;
