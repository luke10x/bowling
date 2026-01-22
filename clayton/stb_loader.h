#pragma once

#include <clay.h>
#include <stdbool.h>

#include <stb_image.h>
#include <stb_truetype.h>

#include "renderers/GLES3/clay_renderer_gles3.h"

typedef struct LoadedImage
{
    unsigned char *data;
    int width;
    int height;
    int channels;
} LoadedImage;

typedef struct LoadedImageInternal
{
    LoadedImage pub;
} LoadedImageInternal;

static LoadedImageInternal g_imageSlot;

const LoadedImage *loadImage(const char *path, bool flip)
{
    if (!path)
        return NULL;

    stbi_set_flip_vertically_on_load(flip ? 1 : 0);

    int w = 0;
    int h = 0;
    int c = 0;

    // unsigned char *data = stbi_load(path, &w, &h, &c, 0);
    SDL_RWops *rw = SDL_RWFromFile(path, "rb");

    Sint64 size = SDL_RWsize(rw);
    unsigned char *buffer = new unsigned char[size];
    SDL_RWread(rw, buffer, 1, size);
    SDL_RWclose(rw);

    // int w,h,c;
    unsigned char *data = stbi_load_from_memory(buffer, size, &w, &h, &c, 4);

    if (!data)
    {
        // Failed
        g_imageSlot.pub.data = NULL;
        g_imageSlot.pub.width = 0;
        g_imageSlot.pub.height = 0;
        g_imageSlot.pub.channels = 0;
        return NULL;
    }

    g_imageSlot.pub.data = data;
    g_imageSlot.pub.width = w;
    g_imageSlot.pub.height = h;
    g_imageSlot.pub.channels = c;

    return &g_imageSlot.pub;
}

void freeImage(const LoadedImage *img)
{
    if (!img || !img->data)
        return;

    // cast back to internal container
    stbi_image_free((void *)img->data);

    // reset slot
    g_imageSlot.pub.data = NULL;
    g_imageSlot.pub.width = 0;
    g_imageSlot.pub.height = 0;
    g_imageSlot.pub.channels = 0;
}

typedef struct Stb_FontData
{
    float bakePxH;   // font baking height (e.g. 48.0f)
    float ascentPx;  // in baked pixels (at bake_px size)
    float descentPx; // usually negative (at bake_px size)
    int firstChar;   // e.g. 32
    int charCount;   // e.g. 96
    stbtt_bakedchar *cdata;
    int atlasW;
    int atlasH;
} Stb_FontData;

bool Stb_LoadFont(
    GLuint *textureOut,
    Stb_FontData *fontOut,
    const char *ttfPath,
    float bakePxH, // Height of a char in pixels
    int atlasW,    // Width of atlas in pixels
    int atlasH     // Height of atlas in pixels
)
{
    fontOut->firstChar = 32; // ASCII space
    fontOut->charCount = 96; // 32..127
    fontOut->bakePxH = bakePxH;
    fontOut->atlasW = atlasW;
    fontOut->atlasH = atlasH;

    // allocate baked-char array
    fontOut->cdata = (stbtt_bakedchar *)malloc(
        sizeof(stbtt_bakedchar) // Store baked info
        * fontOut->charCount    // For each char
    );
    if (!fontOut->cdata)
    {
        fprintf(stderr, "Cannot allocate cdata\n");
        return false;
    }

    SDL_RWops *rw = SDL_RWFromFile(ttfPath, "rb");

    Sint64 size = SDL_RWsize(rw);
    unsigned char *ttf_buf = new unsigned char[size];
    SDL_RWread(rw, ttf_buf, 1, size);
    SDL_RWclose(rw);

    // temporary atlas memory
    unsigned char *atlas = (unsigned char *)malloc(atlasW * atlasH);
    memset(atlas, 0, atlasW * atlasH);

    // bake
    int res = stbtt_BakeFontBitmap(
        ttf_buf, // raw TTF file
        0,       // font index inside TTF (0 = first font)
        bakePxH, // pixel height of glyphs to generate
        atlas,   // OUT: bitmap buffer (unsigned char*)
        atlasW,
        atlasH,             // size of bitmap buffer
        fontOut->firstChar, // first character to bake (e.g., 32 = space)
        fontOut->charCount, // how many sequential chars to bake
        fontOut->cdata      // OUT: array of stbtt_bakedchar
    );

    stbtt_fontinfo fi;
    if (!stbtt_InitFont(&fi, ttf_buf, stbtt_GetFontOffsetForIndex(ttf_buf, 0)))
    {
        return false;
    }

    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&fi, &ascent, &descent, &lineGap);

    // Convert the font's "font units" to pixels proportional to bakePxH size:
    float scaleForBake = stbtt_ScaleForPixelHeight(&fi, bakePxH);

    fontOut->ascentPx = ascent * scaleForBake;
    fontOut->descentPx = descent * scaleForBake; // this is typically negative

    free(ttf_buf);

    if (res <= 0)
    {
        fprintf(stderr, "Font baking failed\n");
        free(atlas);
        free(fontOut->cdata);
        fontOut->cdata = NULL;
        return false;
    }

    // Creating glyphVtxArray atlas texture
    glGenTextures(1, textureOut);
    glBindTexture(GL_TEXTURE_2D, *textureOut);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, atlasW, atlasH, 0, GL_RED, GL_UNSIGNED_BYTE, atlas);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);

    free(atlas);

    return true;
}

static inline Clay_Dimensions
Stb_MeasureText(Clay_StringSlice glyphVtxArray, Clay_TextElementConfig *config, void *userData)
{
    // Use fontData of specified font
    Stb_FontData *allFontData = (Stb_FontData *)userData;
    Stb_FontData *fontData = &allFontData[config->fontId];

    if (!fontData->cdata)
    {
        fprintf(
            stderr,
            "MeasureText cannot do anything when cdata is not baked: '%.*s' → %d x %d px\n",
            (int)glyphVtxArray.length,
            glyphVtxArray.chars,
            0,
            0
        );
        return (Clay_Dimensions){.width = 0, .height = 0};
    }

    float x = 0.0f;
    float y = 0.0f;

    const char *str = glyphVtxArray.chars;
    int len = glyphVtxArray.length;

    float scale = config->fontSize / fontData->bakePxH;

    float letterSpacing = (float)config->letterSpacing;
    float lineHeight = (config->lineHeight > 0) ? (float)config->lineHeight : fontData->bakePxH;

    for (int i = 0; i < len; i++)
    {
        unsigned char c = str[i];

        if (
            c < fontData->firstChar                           // before range
            || c >= fontData->firstChar + fontData->charCount // after range
        )
        {
            // Unsupported char: treat as space
            fprintf(stderr, "illegal char %d\n", (int)c);
            x += fontData->bakePxH * 0.25f;
            continue;
        }

        stbtt_bakedchar *b = &fontData->cdata[c - fontData->firstChar];

        // horizontal advance while moving along word characters
        x += b->xadvance * scale + letterSpacing;
    }

    float ascent = fontData->ascentPx * scale;
    float descent = fontData->descentPx * scale; // negative
    float lineH = (ascent - descent); // total line height in pixels (at requested fontSize)

    return (Clay_Dimensions){
        .width = x,
        .height = y + lineH,
    };
}

static inline void Stb_RenderText(
    Clay_RenderCommand *cmd,        // Layout command
    Gles3_QuadInstanceArray *quads, // quads to be appended here
    void *userData
)
{
    const Clay_TextRenderData *tr = &cmd->renderData.text;

    float cr = tr->textColor.r / 255.0f;
    float cg = tr->textColor.g / 255.0f;
    float cb = tr->textColor.b / 255.0f;
    float ca = tr->textColor.a / 255.0f;
    float fontToUse = (float)tr->fontId;

    Stb_FontData *fontArray = (Stb_FontData *)userData;
    Stb_FontData *stbFontData = &fontArray[tr->fontId];
    if (!stbFontData->cdata)
        return;

    Clay_StringSlice ss = tr->stringContents;
    const char *txt = ss.chars;
    int len = (int)ss.length;

    float scale = tr->fontSize / stbFontData->bakePxH;
    float ascent = stbFontData->ascentPx * scale; // pixels above baseline
    float x = cmd->boundingBox.x;
    float y = cmd->boundingBox.y + ascent; // baseline (note: no descent)

    for (int i = 0; i < len; i++)
    {
        char ch = txt[i];

        int cidx = ch - stbFontData->firstChar;
        if (cidx < 0 || cidx >= stbFontData->charCount)
        {
            continue;
        }

        stbtt_bakedchar *bc = &stbFontData->cdata[cidx];

        float gw = (float)(bc->x1 - bc->x0); // glyph width in atlas pixels
        float gh = (float)(bc->y1 - bc->y0); // glyph height

        float sw = gw * scale; // scaled width on screen
        float sh = gh * scale; // scaled height

        float ox = bc->xoff * scale; // baseline offset
        float oy = bc->yoff * scale;

        // top-left corner on screen (pixel coords)
        float x0 = x + ox;
        float y0 = y + oy;
        float x1 = x0 + sw;
        float y1 = y0 + sh;

        // atlas size (you can make it configurable later)
        float atlasW = stbFontData->atlasW;
        float atlasH = stbFontData->atlasH;

        float u0 = bc->x0 / atlasW;
        float v0 = bc->y0 / atlasH;
        float u1 = bc->x1 / atlasW;
        float v1 = bc->y1 / atlasH;

        // Ensure we don't overflow the capacity
        if (quads->count >= quads->capacity)
        {
            printf("Clay renderer: instance overflow!\n");
            break;
        }

        int idx = quads->count;
        RectInstance *dst = &quads->instData[idx];

        dst->x = roundf(x0);
        dst->y = roundf(y0);
        dst->w = sw;
        dst->h = sh;

        dst->u0 = u0;
        dst->v0 = v0;
        dst->u1 = u1;
        dst->v1 = v1;

        // Texture slots 4 - 7 are reserved for fonts
        dst->texToUse = fontToUse + 4.0f;

        dst->r = cr;
        dst->g = cg;
        dst->b = cb;
        dst->a = ca;

        dst->radiusTL = 0.0f;
        dst->radiusTR = 0.0f;
        dst->radiusBL = 0.0f;
        dst->radiusBR = 0.0f;

        dst->borderT = 0.0f;
        dst->borderR = 0.0f;
        dst->borderB = 0.0f;
        dst->borderL = 0.0f;

        x += (bc->xadvance * scale) + tr->letterSpacing;

        quads->count += 1;
    }
}

bool Stb_LoadImage(GLuint *textureOut, const char *path)
{
    const LoadedImage *li = loadImage(path, false);
    if (!li || !li->data)
    {
        fprintf(stderr, "Failed to load texture at: %s\n", path);
        return false;
    }

    glGenTextures(1, textureOut);
    glBindTexture(GL_TEXTURE_2D, *textureOut);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    GLenum format = (li->channels == 4) ? GL_RGBA : GL_RGB;

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(
        GL_TEXTURE_2D, // target
        0,             // level
        format,        // internal format int
        li->width,
        li->height,
        0,                // border
        format,           // format, GLEnum
        GL_UNSIGNED_BYTE, // Type
        li->data          // pixels
    );

    glGenerateMipmap(GL_TEXTURE_2D);

    freeImage(li);
    return true;
}
