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
    int firstChar;   // e.g. 32 (legacy, for range-based baking)
    int charCount;   // e.g. 96 (legacy, for range-based baking)
    stbtt_bakedchar *cdata;  // for legacy range-based baking
    stbtt_packedchar *pdata; // for custom character packing
    int atlasW;
    int atlasH;
    
    // Custom character set support
    bool useCustomChars;           // true if using custom char string
    int customCharCount;           // number of custom characters
    uint32_t *customCodepoints;    // array of Unicode codepoints
    int *codepointToIndex;         // hash table: codepoint -> atlas index (sparse)
    int codepointTableSize;        // size of the hash table
} Stb_FontData;

// Forward declaration
static inline bool Stb_LoadFontWithChars(
    GLuint *textureOut,
    Stb_FontData *fontOut,
    const char *ttfPath,
    float bakePxH,
    int atlasW,
    int atlasH,
    const char *customChars
);

// Helper: decode one UTF-8 character, return codepoint and advance pointer
static inline uint32_t Stb_DecodeUTF8(const char **p, const char *end)
{
    if (*p >= end) return 0;
    
    unsigned char c = (unsigned char)(**p);
    
    if (c < 0x80) {
        // 1-byte: 0xxxxxxx
        (*p)++;
        return c;
    } else if (c < 0xC0) {
        // Invalid continuation byte
        (*p)++;
        return '?';
    } else if (c < 0xE0) {
        // 2-byte: 110xxxxx 10xxxxxx
        if (*p + 1 >= end) { (*p)++; return '?'; }
        uint32_t cp = ((uint32_t)(c & 0x1F) << 6) | ((uint32_t)(unsigned char)(*p)[1] & 0x3F);
        *p += 2;
        return cp;
    } else if (c < 0xF0) {
        // 3-byte: 1110xxxx 10xxxxxx 10xxxxxx
        if (*p + 2 >= end) { (*p)++; return '?'; }
        uint32_t cp = ((uint32_t)(c & 0x0F) << 12) | ((uint32_t)(unsigned char)(*p)[1] & 0x3F) << 6 | ((uint32_t)(unsigned char)(*p)[2] & 0x3F);
        *p += 3;
        return cp;
    } else if (c < 0xF8) {
        // 4-byte: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
        if (*p + 3 >= end) { (*p)++; return '?'; }
        uint32_t cp = ((uint32_t)(c & 0x07) << 18) | ((uint32_t)(unsigned char)(*p)[1] & 0x3F) << 12 | ((uint32_t)(unsigned char)(*p)[2] & 0x3F) << 6 | ((uint32_t)(unsigned char)(*p)[3] & 0x3F);
        *p += 4;
        return cp;
    } else {
        // Invalid
        (*p)++;
        return '?';
    }
}

// Helper: find index of a codepoint in custom codepoints array
static inline int Stb_FindCodepointIndex(const Stb_FontData *font, uint32_t codepoint)
{
    if (!font->useCustomChars || !font->codepointToIndex) {
        return -1;
    }
    
    // Use hash table for fast lookup
    int slot = codepoint % font->codepointTableSize;
    for (int i = 0; i < font->codepointTableSize; i++) {
        int probe = (slot + i) % font->codepointTableSize;
        int idx = font->codepointToIndex[probe];
        if (idx == -1) {
            return -1; // Not found (empty slot)
        }
        if (font->customCodepoints[idx] == codepoint) {
            return idx; // Found!
        }
    }
    return -1; // Not found
}

// Helper: free custom character data
static inline void Stb_FreeFontData(Stb_FontData *font)
{
    if (font->cdata) {
        free(font->cdata);
        font->cdata = NULL;
    }
    if (font->pdata) {
        free(font->pdata);
        font->pdata = NULL;
    }
    if (font->customCodepoints) {
        free(font->customCodepoints);
        font->customCodepoints = NULL;
    }
    if (font->codepointToIndex) {
        free(font->codepointToIndex);
        font->codepointToIndex = NULL;
    }
}

static inline bool Stb_LoadFont(
    GLuint *textureOut,
    Stb_FontData *fontOut,
    const char *ttfPath,
    float bakePxH, // Height of a char in pixels
    int atlasW,    // Width of atlas in pixels
    int atlasH     // Height of atlas in pixels
)
{
    return Stb_LoadFontWithChars(textureOut, fontOut, ttfPath, bakePxH, atlasW, atlasH, NULL);
}

static inline bool Stb_LoadFontWithChars(
    GLuint *textureOut,
    Stb_FontData *fontOut,
    const char *ttfPath,
    float bakePxH, // Height of a char in pixels
    int atlasW,    // Width of atlas in pixels
    int atlasH,    // Height of atlas in pixels
    const char *customChars // UTF-8 string of desired characters, or NULL for default range
)
{
    fontOut->bakePxH = bakePxH;
    fontOut->atlasW = atlasW;
    fontOut->atlasH = atlasH;
    
    // Initialize custom char support
    fontOut->useCustomChars = false;
    fontOut->customCodepoints = NULL;
    fontOut->codepointToIndex = NULL;
    fontOut->codepointTableSize = 0;

    // Count characters if custom string provided
    int numChars = 0;
    if (customChars) {
        const char *p = customChars;
        const char *end = customChars + strlen(customChars);
        while (p < end) {
            uint32_t cp = Stb_DecodeUTF8(&p, end);
            if (cp > 0) numChars++;
        }
        fontOut->useCustomChars = true;
        fontOut->customCharCount = numChars;
        fontOut->firstChar = 0;
        fontOut->charCount = numChars;
    } else {
        // Default: ASCII range 32..127
        fontOut->firstChar = 32;
        fontOut->charCount = 96;
    }

    // allocate baked-char array
    fontOut->cdata = NULL;
    fontOut->pdata = NULL;
    
    if (fontOut->useCustomChars) {
        fontOut->pdata = (stbtt_packedchar *)malloc(
            sizeof(stbtt_packedchar) * fontOut->charCount
        );
    } else {
        fontOut->cdata = (stbtt_bakedchar *)malloc(
            sizeof(stbtt_bakedchar) * fontOut->charCount
        );
    }
    
    if (fontOut->useCustomChars && !fontOut->pdata) {
        fprintf(stderr, "Cannot allocate pdata\n");
        return false;
    }
    if (!fontOut->useCustomChars && !fontOut->cdata) {
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

    int res;
    
    if (fontOut->useCustomChars) {
        /* Use the more flexible stbtt_Pack API for custom characters */
        int validCharCount = 0;
        const char *p;
        const char *end;
        stbtt_pack_context spc;
        stbtt_pack_range range;
        int i;

        /* Decode all codepoints first */
        fontOut->customCodepoints = (uint32_t *)malloc(sizeof(uint32_t) * numChars);
        p = customChars;
        end = customChars + strlen(customChars);
        while (p < end && validCharCount < numChars) {
            uint32_t cp = Stb_DecodeUTF8(&p, end);
            if (cp == 0) continue;
            fontOut->customCodepoints[validCharCount] = cp;
            validCharCount++;
        }

        fontOut->charCount = validCharCount;
        fontOut->customCharCount = validCharCount;

        /* Pack all characters at once using stbtt_PackFontRanges */
        stbtt_PackBegin(&spc, atlas, atlasW, atlasH, 0, 1, NULL);
        stbtt_PackSetOversampling(&spc, 1, 1);
        stbtt_PackSetSkipMissingCodepoints(&spc, true);

        /* Create a range structure for all our custom characters */
        range.font_size = bakePxH;
        range.first_unicode_codepoint_in_range = -1;  /* Signal we're using array */
        range.array_of_unicode_codepoints = (int*)fontOut->customCodepoints;  /* Array of codepoints */
        range.num_chars = validCharCount;
        range.chardata_for_range = fontOut->pdata;

        stbtt_PackFontRanges(&spc, ttf_buf, 0, &range, 1);

        stbtt_PackEnd(&spc);

        /* Build hash table for fast lookup */
        fontOut->codepointTableSize = fontOut->customCharCount * 2; /* 50% load factor */
        if (fontOut->codepointTableSize < 16) fontOut->codepointTableSize = 16;
        fontOut->codepointToIndex = (int *)malloc(sizeof(int) * fontOut->codepointTableSize);
        memset(fontOut->codepointToIndex, -1, sizeof(int) * fontOut->codepointTableSize);

        for (i = 0; i < fontOut->customCharCount; i++) {
            int slot = fontOut->customCodepoints[i] % fontOut->codepointTableSize;
            int j;
            for (j = 0; j < fontOut->codepointTableSize; j++) {
                int probe = (slot + j) % fontOut->codepointTableSize;
                if (fontOut->codepointToIndex[probe] == -1) {
                    fontOut->codepointToIndex[probe] = i;
                    break;
                }
            }
        }

    } else {
        // Bake sequential range (legacy)
        res = stbtt_BakeFontBitmap(
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
    }

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

    if (!fontOut->useCustomChars && res <= 0)
    {
        fprintf(stderr, "Font baking failed\n");
        free(atlas);
        if (fontOut->cdata) free(fontOut->cdata);
        fontOut->cdata = NULL;
        return false;
    }
    
    if (fontOut->useCustomChars && fontOut->customCharCount == 0) {
        fprintf(stderr, "Font baking failed: no valid characters\n");
        free(atlas);
        if (fontOut->pdata) free(fontOut->pdata);
        fontOut->pdata = NULL;
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

    if (!fontData->cdata && !fontData->pdata)
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

    const char *p = str;
    const char *end = str + len;

    while (p < end)
    {
        uint32_t cp = Stb_DecodeUTF8(&p, end);
        int charIndex = -1;
        
        if (cp == 0) break;

        if (fontData->useCustomChars) {
            /* Use hash table for fast lookup */
            if (fontData->codepointToIndex) {
                int slot = cp % fontData->codepointTableSize;
                int i;
                for (i = 0; i < fontData->codepointTableSize; i++) {
                    int probe = (slot + i) % fontData->codepointTableSize;
                    int idx = fontData->codepointToIndex[probe];
                    if (idx == -1) {
                        break; /* Not in table */
                    }
                    if (fontData->customCodepoints[idx] == cp) {
                        charIndex = idx;
                        break;
                    }
                }
            }
        } else {
            /* Legacy range-based lookup */
            if (cp >= fontData->firstChar && cp < fontData->firstChar + fontData->charCount) {
                charIndex = (int)(cp - fontData->firstChar);
            }
        }

        if (charIndex < 0) {
            /* Unsupported char: treat as space */
            x += fontData->bakePxH * 0.25f;
            continue;
        }

        if (fontData->useCustomChars) {
            /* Use packed char data - stbtt_GetPackedQuad advances xpos automatically */
            stbtt_aligned_quad q;
            float xpos = 0;
            float ypos = 0;
            stbtt_GetPackedQuad(&fontData->pdata[charIndex], fontData->atlasW, fontData->atlasH, 0, &xpos, &ypos, &q, 0);
            x += xpos * scale + letterSpacing;
        } else {
            /* Legacy range-based data */
            stbtt_bakedchar *b = &fontData->cdata[charIndex];
            x += b->xadvance * scale + letterSpacing;
        }
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
    if (!stbFontData->cdata && !stbFontData->pdata)
        return;

    Clay_StringSlice ss = tr->stringContents;
    const char *txt = ss.chars;
    int len = (int)ss.length;

    float scale = tr->fontSize / stbFontData->bakePxH;
    float ascent = stbFontData->ascentPx * scale; /* pixels above baseline */
    float x = cmd->boundingBox.x;
    float y = cmd->boundingBox.y + ascent; /* baseline (note: no descent) */

    const char *p = txt;
    const char *end = txt + len;

    while (p < end)
    {
        uint32_t cp = Stb_DecodeUTF8(&p, end);
        int charIndex = -1;
        
        if (cp == 0) break;

        if (stbFontData->useCustomChars) {
            /* Use hash table for fast lookup */
            if (stbFontData->codepointToIndex) {
                int slot = cp % stbFontData->codepointTableSize;
                int i;
                for (i = 0; i < stbFontData->codepointTableSize; i++) {
                    int probe = (slot + i) % stbFontData->codepointTableSize;
                    int idx = stbFontData->codepointToIndex[probe];
                    if (idx == -1) {
                        break; /* Not in table */
                    }
                    if (stbFontData->customCodepoints[idx] == cp) {
                        charIndex = idx;
                        break;
                    }
                }
            }
            if (charIndex < 0 && cp > 32) {
                /* Character not found - skip silently */
            }
        } else {
            /* Legacy range-based lookup */
            if (cp >= stbFontData->firstChar && cp < stbFontData->firstChar + stbFontData->charCount) {
                charIndex = (int)(cp - stbFontData->firstChar);
            }
        }

        if (charIndex < 0) {
            continue; /* Skip unsupported characters */
        }

        {
            float x0, y0, x1, y1, u0, v0, u1, v1, sw, sh;

            if (stbFontData->useCustomChars) {
                /* Use packed char data - stbtt_GetPackedQuad advances position automatically */
                stbtt_aligned_quad q;
                float xpos = 0;
                float ypos = 0;
                stbtt_GetPackedQuad(&stbFontData->pdata[charIndex], stbFontData->atlasW, stbFontData->atlasH, 0, &xpos, &ypos, &q, 0);

                /* xpos now contains the advance width (next position) */
                float advance = xpos;

                /* Scale from bake size to display size and position relative to current x */
                x0 = x + q.x0 * scale;
                y0 = y + q.y0 * scale;
                x1 = x + q.x1 * scale;
                y1 = y + q.y1 * scale;
                u0 = q.s0;
                v0 = q.t0;
                u1 = q.s1;
                v1 = q.t1;
                sw = x1 - x0;
                sh = y1 - y0;

                x += advance * scale + tr->letterSpacing;
            } else {
                /* Legacy range-based data */
                stbtt_bakedchar *bc = &stbFontData->cdata[charIndex];
                float ox = bc->xoff * scale;
                float oy = bc->yoff * scale;

                sw = (bc->x1 - bc->x0) * scale;
                sh = (bc->y1 - bc->y0) * scale;

            x0 = x + ox;
            y0 = y + oy;
            x1 = x0 + sw;
            y1 = y0 + sh;

            u0 = bc->x0 / stbFontData->atlasW;
            v0 = bc->y0 / stbFontData->atlasH;
            u1 = bc->x1 / stbFontData->atlasW;
            v1 = bc->y1 / stbFontData->atlasH;

            x += (bc->xadvance * scale) + tr->letterSpacing;
        }

        /* Ensure we don't overflow the capacity */
        if (quads->count >= quads->capacity)
        {
            printf("Clay renderer: instance overflow!\n");
            break;
        }

        {
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

            /* Texture slots 4 - 7 are reserved for fonts */
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
        }

        quads->count += 1;
        }
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
