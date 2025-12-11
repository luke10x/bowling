#pragma once

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

// C++ libs
#include <stdlib.h>

// C libs
#define CLAY_IMPLEMENTATION
#include <clay.h>

void Gles3_ErrorHandler(Clay_ErrorData errorData)
{
    printf("[ClaY ErroR] %s", errorData.errorText.chars);
}

enum
{
    ATTR_POS = 0,
    ATTR_RECT = 1,
    ATTR_COLOR = 2,
    ATTR_UV = 3,
    ATTR_RAD1 = 4,
    ATTR_BORDER1 = 5,
    ATTR_TEX = 6,
};

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

// Todo rename Gles_GlyphInstanceArray
typedef struct Gles3_GlyphVtxArray
{
    GlyphVtx *glyphVerticeData;
    int glyphCapacity;
    int glyphCount;
} Gles3_GlyphVtxArray;

#define MAX_IMAGES 4
#define MAX_FONTS 4

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
    Stb_FontData *stbFont,
    const char *ttfPath,
    float bakePxH, // Height of a char in pixels
    int atlasW,    // Width of atlas in pixels
    int atlasH     // Height of atlas in pixels
)
{
    stbFont->firstChar = 32; // ASCII space
    stbFont->charCount = 96; // 32..127
    stbFont->bakePxH = bakePxH;
    stbFont->atlasW = atlasW;
    stbFont->atlasH = atlasH;

    // allocate baked-char array
    stbFont->cdata = (stbtt_bakedchar *)malloc(
        sizeof(stbtt_bakedchar) // Store baked info
        * stbFont->charCount    // For each char
    );
    if (!stbFont->cdata)
    {
        fprintf(stderr, "Cannot allocate cdata\n");
        return false;
    }

    // load font file
    FILE *f = fopen(ttfPath, "rb");
    if (!f)
    {
        fprintf(stderr, "Could not open font: %s\n", ttfPath);
        return false;
    }

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    unsigned char *ttf_buf = (unsigned char *)malloc(sz);
    fread(ttf_buf, 1, sz, f);
    fclose(f);

    // temporary atlas memory
    unsigned char *atlas = (unsigned char *)malloc(atlasW * atlasH);
    memset(atlas, 0, atlasW * atlasH);

    // bake
    int res = stbtt_BakeFontBitmap(
        ttf_buf,            // raw TTF file
        0,                  // font index inside TTF (0 = first font)
        bakePxH,            // pixel height of glyphs to generate
        atlas,              // OUT: bitmap buffer (unsigned char*)
        atlasW, atlasH,     // size of bitmap buffer
        stbFont->firstChar, // first character to bake (e.g., 32 = space)
        stbFont->charCount, // how many sequential chars to bake
        stbFont->cdata      // OUT: array of stbtt_bakedchar
    );

    stbtt_fontinfo fi;
    if (!stbtt_InitFont(&fi, ttf_buf, stbtt_GetFontOffsetForIndex(ttf_buf, 0)))
    {
        // TODO handle error
    }

    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&fi, &ascent, &descent, &lineGap);

    // Convert the font's "font units" to pixels proportional to bakePxH size:
    float scaleForBake = stbtt_ScaleForPixelHeight(&fi, bakePxH);

    stbFont->ascentPx = ascent * scaleForBake;
    stbFont->descentPx = descent * scaleForBake; // this is typically negative

    free(ttf_buf);

    if (res <= 0)
    {
        fprintf(stderr, "Font baking failed\n");
        free(atlas);
        free(stbFont->cdata);
        stbFont->cdata = NULL;
        return false;
    }

    // Creating glyphVtxArray atlas texture
    glGenTextures(1, textureOut);
    glBindTexture(GL_TEXTURE_2D, *textureOut);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8,
                 atlasW, atlasH,
                 0, GL_RED, GL_UNSIGNED_BYTE, atlas);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);

    free(atlas);

    return true;
}

static inline Clay_Dimensions Stb_MeasureText(
    Clay_StringSlice glyphVtxArray,
    Clay_TextElementConfig *config,
    void *userData)
{
    Stb_FontData *fontData = (Stb_FontData *)userData;

    if (!fontData->cdata)
    {
        fprintf(
            stderr,
            "MeasureText cannot do anything when cdata is not baked: '%.*s' → %d x %d px\n",
            (int)glyphVtxArray.length, glyphVtxArray.chars, 0, 0);
        return (Clay_Dimensions){.width = 0, .height = 0};
    }

    float x = 0.0f;
    float y = 0.0f;

    const char *str = glyphVtxArray.chars;
    int len = glyphVtxArray.length;

    float scale = config->fontSize / fontData->bakePxH;

    float letterSpacing = (float)config->letterSpacing;
    float lineHeight = (config->lineHeight > 0)
                           ? (float)config->lineHeight
                           : fontData->bakePxH;

    for (int i = 0; i < len; i++)
    {
        unsigned char c = str[i];

        if (c < fontData->firstChar                           // before range
            || c >= fontData->firstChar + fontData->charCount // after range
        )
        {
            // Unsupported char: treat as space
            std::cerr << "Illegal char: " << c
                      << " as int: " << (int)c
                      << " first char is: " << fontData->firstChar
                      << " char count is: " << fontData->charCount
                      << std::endl;
            x += fontData->bakePxH * 0.25f;
            continue;
        }

        stbtt_bakedchar *b = &fontData->cdata[c - fontData->firstChar];

        // horizontal advance while moving along word characters
        x += b->xadvance * scale + letterSpacing;
    }

    float ascent = fontData->ascentPx * scale;
    float descent = fontData->descentPx * scale; // negative
    float lineH = (ascent - descent);            // total line height in pixels (at requested fontSize)

    return (Clay_Dimensions){
        .width = x,
        .height = y + lineH,
    };
}

static inline void Stb_RenderText(
    Clay_RenderCommand *cmd,
    Gles3_GlyphVtxArray *glyphVtxArray,
    void *userData)
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

        int idx = ch - stbFontData->firstChar;
        if (idx < 0 || idx >= stbFontData->charCount)
        {
            continue;
        }

        stbtt_bakedchar *bc = &stbFontData->cdata[idx];

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

        // append 6 vertices (two triangles) to your buffer
        GlyphVtx *v = &glyphVtxArray->glyphVerticeData[glyphVtxArray->glyphCount * 6];

        v[0] = (GlyphVtx){x0, y0, u0, v0, cr, cg, cb, ca, fontToUse};
        v[1] = (GlyphVtx){x1, y0, u1, v0, cr, cg, cb, ca, fontToUse};
        v[2] = (GlyphVtx){x0, y1, u0, v1, cr, cg, cb, ca, fontToUse};

        v[3] = (GlyphVtx){x0, y1, u0, v1, cr, cg, cb, ca, fontToUse};
        v[4] = (GlyphVtx){x1, y0, u1, v0, cr, cg, cb, ca, fontToUse};
        v[5] = (GlyphVtx){x1, y1, u1, v1, cr, cg, cb, ca, fontToUse};

        // advance pen by baked xadvance + letter spacing
        x += (bc->xadvance * scale) + tr->letterSpacing;

        // prevent buffer overrun
        if (glyphVtxArray->glyphCount >= glyphVtxArray->glyphCapacity)
        {
            break;
        }
        glyphVtxArray->glyphCount++;
    }
}

/* Image loading in STBI */
int Stb_LoadImage(GLuint *textureOut, const char *path)
{
    const acl::LoadedImage *li = acl::loadImage(path, false);
    if (!li || !li->data)
    {
        fprintf(stderr, "Failed to load texture at: %s\n", path);
        return 0;
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

    acl::freeImage(li);
    return 1;
}

/*
 * rendering
 */

const char *GLES3_QUAD_VERTEX_SHADER =
    GLSL_VERSION
    R"(
    precision mediump float;
    layout(location = 0) in vec2 aPos;        // unit quad (0..1)
    layout(location = 1) in vec4 aRect;       // x,y,w,h (pixels)
    layout(location = 3) in vec4 aUV;         // u0,v0,u1,v1
    layout(location = 2) in vec4 aColor;      // rgba
    layout(location = 4) in vec4 aCornerRadii;
    layout(location = 5) in vec4 aBorderWidths;
    layout(location = 6) in float aTexSlot;

    uniform vec2 uScreen;                     // screen size in pixels
    out vec2 vPos;
    out vec4 vRect;
    out vec4 vColor;
    out vec2 vUV;
    out vec4 vCornerRadii;
    out vec4 vBorderWidths;
    out float vTexSlot;

    void main() {
        vec2 pos = vec2(aPos.x * aRect.z + aRect.x, aPos.y * aRect.w + aRect.y);
        vec2 ndc = pos / uScreen * 2.0 - 1.0; // ndc.y increases up; pos y increases down (we will inve
        ndc.y = -ndc.y;
        gl_Position = vec4(ndc, 0.0, 1.0);
        vPos = aPos;
        vRect = aRect;
        vColor = aColor;
        vUV = mix(aUV.xy, aUV.zw, aPos);
        vCornerRadii = aCornerRadii;
        vBorderWidths = aBorderWidths;
        vTexSlot = aTexSlot;
    }
    )";

const char *GLES3_QUAD_FRAGMENT_SHADER =
    GLSL_VERSION
    R"(
    precision mediump float;

    in vec2 vPos;
    in vec4 vRect;
    in vec4 vColor;
    in vec2 vUV;
    in vec4 vCornerRadii;
    in vec4 vBorderWidths;
    in float vTexSlot;

    uniform sampler2D uTex0;
    uniform sampler2D uTex1;
    uniform sampler2D uTex2;
    uniform sampler2D uTex3;

    out vec4 frag;

    void main() {
        // Pixel coordinates in pixel space
        vec2 pix = vRect.xy + vPos * vRect.zw;

        float x0 = vRect.x;
        float y0 = vRect.y;
        float w  = vRect.z;
        float h  = vRect.w;

        // Local position inside the rectangle (0..w, 0..h)
        vec2 local = pix - vec2(x0, y0);

        // Original corner radii
        float tl = vCornerRadii.x;
        float tr = vCornerRadii.y;
        float bl = vCornerRadii.z;
        float br = vCornerRadii.w;

        // Border thicknesses
        float L = vBorderWidths.x;
        float R = vBorderWidths.y;
        float T = vBorderWidths.z;
        float B = vBorderWidths.w;

        bool isBorder = (L > 0.0 || R > 0.0 || T > 0.0 || B > 0.0);

        // -------- Compute inner and outer radii --------
        float tl_i = tl;
        float tr_i = tr;
        float bl_i = bl;
        float br_i = br;

        float tl_o = tl;
        float tr_o = tr;
        float bl_o = bl;
        float br_o = br;

        if (isBorder) {
            // Inner radius = like a normal rectangle (matches borderless)
            tl_i = max(tl - T, 0.0);
            tr_i = max(tr - T, 0.0);
            bl_i = max(bl - B, 0.0);
            br_i = max(br - B, 0.0);

            // Outer radius = inner + border thickness
            tl_o = tl_i + T;
            tr_o = tr_i + T;
            bl_o = bl_i + B;
            br_o = br_i + B;
        }

        // -------- Outer rounded-rectangle clip --------
        float outerAlpha = 1.0;

        if (tl_o > 0.0 && local.x < tl_o && local.y < tl_o)
            outerAlpha = step(length(local - vec2(tl_o, tl_o)), tl_o);
        if (tr_o > 0.0 && local.x > w - tr_o && local.y < tr_o)
            outerAlpha *= step(length(local - vec2(w - tr_o, tr_o)), tr_o);
        if (bl_o > 0.0 && local.x < bl_o && local.y > h - bl_o)
            outerAlpha *= step(length(local - vec2(bl_o, h - bl_o)), bl_o);
        if (br_o > 0.0 && local.x > w - br_o && local.y > h - br_o)
            outerAlpha *= step(length(local - vec2(w - br_o, h - br_o)), br_o);

        if (outerAlpha < 0.5)
            discard;

        // -------- Border logic --------
        if (isBorder) {
            float iw = w - L - R;
            float ih = h - T - B;

            vec2 innerLocal = local - vec2(L, T);

            // Check if pixel is inside inner rounded rect
            bool insideInner = true;

            if (tl_o > 0.0 && innerLocal.x < tl_o && innerLocal.y < tl_i)
                insideInner = (length(innerLocal - vec2(tl_o, tl_o)) <= tl_o);
            if (tr_o > 0.0 && innerLocal.x > iw - tr_o && innerLocal.y < tr_o)
                insideInner = insideInner && (length(innerLocal - vec2(iw - tr_o, tr_o)) <= tr_o);
            // Bottom-left
            if (bl_o > 0.0 && innerLocal.x < bl_o && innerLocal.y > ih - bl_o) 
                insideInner = insideInner && (length(innerLocal - vec2(bl_o, ih - bl_o)) <= bl_o);
            // Bottom-right
            if (br_o > 0.0 && innerLocal.x > iw - br_o && innerLocal.y > ih - br_o)
                insideInner = insideInner && (length(innerLocal - vec2(iw - br_o, ih - br_o)) <= br_o);
        
            // Discard pixels inside inner rounded rect
            if (insideInner && innerLocal.x >= 0.0 && innerLocal.x <= iw && innerLocal.y >= 0.0 && innerLocal.y <= ih)
                discard;

            frag = vColor;
            return;
        }

        // -------- Non-border rectangle or image --------
        if (vTexSlot < 0.0) {
            frag = vColor;
        } else {
            int slot = int(vTexSlot + 0.5);
            if (slot == 0) frag = texture(uTex0, vUV);
            if (slot == 1) frag = texture(uTex1, vUV);
            if (slot == 2) frag = texture(uTex2, vUV);
            if (slot == 3) frag = texture(uTex3, vUV);
        }
    }
    )";

const char *GLES3_TEXT_VERTEX_SHADER =
    GLSL_VERSION
    R"(
        precision mediump float;

        layout(location = 0) in vec2 aPos;
        layout(location = 1) in vec2 aUV;
        layout(location = 2) in vec4 aColor;
        layout(location = 3) in float aTexSlot;

        uniform vec2 uScreen;

        out vec2 vUV;
        out vec4 vColor;
        out float vTexSlot;

        void main() {
            vec2 ndc = (aPos / uScreen) * 2.0 - 1.0;
            gl_Position = vec4(ndc * vec2(1.0, -1.0), 0.0, 1.0);

            vUV = aUV;
            vColor = aColor;
            vTexSlot = aTexSlot;
        }
    )";

const char *GLES3_TEXT_FRAGMENT_SHADER =
    GLSL_VERSION
    R"(
    precision mediump float;

    in vec2 vUV;
    in vec4 vColor;
    in float vTexSlot;

    uniform sampler2D uTex0;
    uniform sampler2D uTex1;
    uniform sampler2D uTex2;
    uniform sampler2D uTex3;

    out vec4 fragColor;

    void main() {

        int slot = int(vTexSlot + 0.5);
        float coverage;
        if (slot == 0) coverage = texture(uTex0, vUV).r;
        if (slot == 1) coverage = texture(uTex1, vUV).r;
        if (slot == 2) coverage = texture(uTex2, vUV).r;
        if (slot == 3) coverage = texture(uTex3, vUV).r;
        fragColor = vec4(vColor.rgb, vColor.a * coverage);
    } 
    )";

typedef struct Gles3_ImageConfig
{
    int textureToUse;
    float u0, v0;
    float u1, v1;
} Gles3_ImageConfig;

typedef struct Gles3_Renderer
{
    Clay_Arena clayMemory;

    float screenWidth;
    float screenHeight;

    /* Quads rendering */
    GLuint quadVAO;
    GLuint quadVBO;
    GLuint quadInstanceVBO;
    GLuint quadShaderId;
    GLuint imageTextures[MAX_IMAGES];

    RectInstance *quadInstanceData; // packed per-instance floats
    int quadInstanceCapacity;       // how many instances it can hold
    int instanceCount;              // how many instances does it actually hold

    /* Fonts rendering */
    GLuint textVAO;
    GLuint textVBO;
    GLuint textShader;
    GLuint fontTextures[MAX_FONTS];

    Gles3_GlyphVtxArray glyphVtxArray; // Instance data: every vertex is an element,
                                       // 6 elements per each instance

    void (*renderTextFunction)(
        Clay_RenderCommand *cmd,
        Gles3_GlyphVtxArray *accum,
        void *userData //
    );
} Gles3_Renderer;

void Gles3_Initialize(Gles3_Renderer *renderer, int maxInstances)
{
    // compile shader
    renderer->quadShaderId = vtx::createShaderProgram(
        GLES3_QUAD_VERTEX_SHADER, GLES3_QUAD_FRAGMENT_SHADER);

    glUseProgram(renderer->quadShaderId);
    glUniform1i(glGetUniformLocation(renderer->quadShaderId, "uTex0"), 0);
    glUniform1i(glGetUniformLocation(renderer->quadShaderId, "uTex1"), 1);
    glUniform1i(glGetUniformLocation(renderer->quadShaderId, "uTex2"), 2);
    glUniform1i(glGetUniformLocation(renderer->quadShaderId, "uTex3"), 3);
    // create unit quad VBO (0..1)
    const float quadVerts[8] = {0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f};
    glGenVertexArrays(1, &renderer->quadVAO);
    glBindVertexArray(renderer->quadVAO);

    glGenBuffers(1, &renderer->quadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);

    // attribute 0: aPos (vec2), per-vertex
    glEnableVertexAttribArray(ATTR_POS);
    glVertexAttribPointer(ATTR_POS, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    glVertexAttribDivisor(ATTR_POS, 0);

    // create instance buffer big enough
    renderer->quadInstanceCapacity = maxInstances;
    renderer->quadInstanceData =
        (RectInstance *)malloc(sizeof(RectInstance) * renderer->quadInstanceCapacity);
    renderer->instanceCount = 0;

    glGenBuffers(1, &renderer->quadInstanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->quadInstanceVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(RectInstance) * renderer->quadInstanceCapacity,
                 NULL,
                 GL_DYNAMIC_DRAW);

    // set up instance attributes
    GLsizei stride = sizeof(RectInstance);

    glEnableVertexAttribArray(ATTR_RECT);
    glVertexAttribPointer(ATTR_RECT, 4, GL_FLOAT, GL_FALSE,
                          stride, (void *)offsetof(RectInstance, x));
    glVertexAttribDivisor(ATTR_RECT, 1);

    glEnableVertexAttribArray(ATTR_UV);
    glVertexAttribPointer(ATTR_UV, 4, GL_FLOAT, GL_FALSE,
                          stride, (void *)offsetof(RectInstance, u0));
    glVertexAttribDivisor(ATTR_UV, 1);

    glEnableVertexAttribArray(ATTR_COLOR);
    glVertexAttribPointer(ATTR_COLOR, 4, GL_FLOAT, GL_FALSE,
                          stride, (void *)offsetof(RectInstance, r));
    glVertexAttribDivisor(ATTR_COLOR, 1);

    glEnableVertexAttribArray(ATTR_RAD1);
    glVertexAttribPointer(ATTR_RAD1, 4, GL_FLOAT, GL_FALSE,
                          stride, (void *)offsetof(RectInstance, radiusTL));
    glVertexAttribDivisor(ATTR_RAD1, 1);

    glEnableVertexAttribArray(ATTR_BORDER1);
    glVertexAttribPointer(ATTR_BORDER1, 4, GL_FLOAT, GL_FALSE,
                          stride, (void *)offsetof(RectInstance, borderL));
    glVertexAttribDivisor(ATTR_BORDER1, 1);

    glEnableVertexAttribArray(ATTR_TEX);
    glVertexAttribPointer(ATTR_TEX, 1, GL_FLOAT, GL_FALSE,
                          stride, (void *)offsetof(RectInstance, texToUse));
    glVertexAttribDivisor(ATTR_TEX, 1);

    glBindVertexArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Ok now we will initialize text!
    Gles3_GlyphVtxArray *t = &renderer->glyphVtxArray;

    // configure capacity
    t->glyphCapacity = 4096; // adjust as needed
    t->glyphCount = 0;

    // allocate CPU-side vertex buffer: 6 vertices per glyph
    t->glyphVerticeData = (GlyphVtx *)malloc(sizeof(GlyphVtx) * 6 * t->glyphCapacity);
    if (!t->glyphVerticeData)
    {
        fprintf(stderr, "Failed to allocate glyph_vertices\n");
        t->glyphCapacity = 0;
    }

    // create VAO/VBO for text rendering
    glGenVertexArrays(1, &renderer->textVAO);
    glBindVertexArray(renderer->textVAO);

    glGenBuffers(1, &renderer->textVBO);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->textVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(GlyphVtx) * 6 * t->glyphCapacity,
                 NULL,
                 GL_DYNAMIC_DRAW);

    GLsizei gv_stride = sizeof(GlyphVtx);

    // attrib 0: position vec2
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, gv_stride, (void *)(offsetof(GlyphVtx, x)));

    // attrib 1: uv vec2
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, gv_stride, (void *)(offsetof(GlyphVtx, u)));

    // attrib 2: color vec4
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, gv_stride, (void *)(offsetof(GlyphVtx, r)));

    // attrib 3: fontTexSlot
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, gv_stride, (void *)(offsetof(GlyphVtx, atlasTexUnit)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    renderer->textShader = vtx::createShaderProgram(
        GLES3_TEXT_VERTEX_SHADER, GLES3_TEXT_FRAGMENT_SHADER);
    glUseProgram(renderer->textShader);

    // Link sampler uniforms in the text shader to the correct texture units.
    // Each uniform tells the shader which unit to read from.
    glUniform1i(glGetUniformLocation(renderer->textShader, "uTex0"), 0);
    glUniform1i(glGetUniformLocation(renderer->textShader, "uTex1"), 1);
    glUniform1i(glGetUniformLocation(renderer->textShader, "uTex2"), 2);
    glUniform1i(glGetUniformLocation(renderer->textShader, "uTex3"), 3);
}

void Gles3_SetRenderTextFunction(
    Gles3_Renderer *renderer,
    void (*renderTextFunction)(
        Clay_RenderCommand *cmd, Gles3_GlyphVtxArray *accum, void *userData),
    void *userData)
{
    renderer->renderTextFunction = renderTextFunction;
}

void Gles3_Render(
    Gles3_Renderer *renderer,
    Clay_RenderCommandArray cmds,
    void *userData // eg. fonts
)
{
    renderer->glyphVtxArray.glyphCount = 0;
    for (int i = 0; i < cmds.length; i++)
    {
        Clay_RenderCommand *cmd = Clay_RenderCommandArray_Get(&cmds, i);
        Clay_BoundingBox boundingBox = (Clay_BoundingBox){
            .x = roundf(cmd->boundingBox.x),
            .y = roundf(cmd->boundingBox.y),
            .width = roundf(cmd->boundingBox.width),
            .height = roundf(cmd->boundingBox.height),
        };

        bool scissorChanged = false;
        switch (cmd->commandType)
        {
        case CLAY_RENDER_COMMAND_TYPE_TEXT:
        {
            renderer->renderTextFunction(
                cmd,
                &renderer->glyphVtxArray,
                userData);
            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_RECTANGLE:
        case CLAY_RENDER_COMMAND_TYPE_IMAGE:
        {
            Clay_RectangleRenderData *config = &cmd->renderData.rectangle;
            Clay_Color c = config->backgroundColor;

            // Convert to float 0..1
            float rf = c.r / 255.0f;
            float gf = c.g / 255.0f;
            float bf = c.b / 255.0f;
            float af = c.a / 255.0f;

            bool isImage = cmd->commandType == CLAY_RENDER_COMMAND_TYPE_IMAGE;

            // Ensure we don't overflow the capacity
            if (
                renderer->instanceCount >= renderer->quadInstanceCapacity)
            {
                printf("Clay renderer: instance overflow!\n");
                break;
            }

            int idx = renderer->instanceCount;
            RectInstance *dst = &renderer->quadInstanceData[idx];
            dst->x = boundingBox.x;
            dst->y = boundingBox.y;
            dst->w = boundingBox.width;
            dst->h = boundingBox.height;

            if (isImage)
            {
                Gles3_ImageConfig *imgConf = (Gles3_ImageConfig *)cmd->renderData.image.imageData;
                dst->u0 = imgConf->u0;
                dst->v0 = imgConf->v0;
                dst->u1 = imgConf->u1;
                dst->v1 = imgConf->v1;
                dst->texToUse = (float)imgConf->textureToUse;
            }
            else
            {
                dst->u0 = dst->v0 = 0.0f;
                dst->u1 = dst->v1 = 1.0f;
                dst->texToUse = -1.0f; // This means no image, use albedo color
            }

            // colour
            dst->r = rf;
            dst->g = gf;
            dst->b = bf;
            dst->a = af;

            // corner radii
            Clay_CornerRadius r = config->cornerRadius;
            dst->radiusTL = r.topLeft;
            dst->radiusTR = r.topRight;
            dst->radiusBL = r.bottomLeft;
            dst->radiusBR = r.bottomRight;

            dst->borderT = 0.0f;
            dst->borderR = 0.0f;
            dst->borderB = 0.0f;
            dst->borderL = 0.0f;

            renderer->instanceCount++;
            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START:
        {
            scissorChanged = true;
            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END:
        {
            scissorChanged = true;
            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_BORDER:
        {
            Clay_BorderRenderData *br = &cmd->renderData.border;

            float rf = br->color.r / 255.0f;
            float gf = br->color.g / 255.0f;
            float bf = br->color.b / 255.0f;
            float af = br->color.a / 255.0f;

            float x = boundingBox.x;
            float y = boundingBox.y;
            float w = boundingBox.width;
            float h = boundingBox.height;

            float top = br->width.top;
            float bottom = br->width.bottom;
            float left = br->width.left;
            float right = br->width.right;

            int idx = renderer->instanceCount;
            RectInstance *dst = &renderer->quadInstanceData[idx];

            dst->x = x - left;
            dst->y = y - top;
            dst->w = w + right;
            dst->h = h + bottom;

            dst->borderB = bottom;
            dst->borderL = left;
            dst->borderT = top;
            dst->borderR = right;

            dst->x = x - left;
            dst->y = y - top;
            dst->w = w + left + right;
            dst->h = h + top + bottom;

            dst->u0 = 0.0f;
            dst->v0 = 0.0f;
            dst->u1 = 1.0f;
            dst->v1 = 1.0f;

            dst->r = rf;
            dst->g = gf;
            dst->b = bf;
            dst->a = af;

            dst->radiusTL = br->cornerRadius.topLeft;
            dst->radiusTR = br->cornerRadius.topRight;
            dst->radiusBR = br->cornerRadius.bottomRight;
            dst->radiusBL = br->cornerRadius.bottomLeft;

            dst->texToUse = -1.0f;

            renderer->instanceCount++;
            break;
        }

        case CLAY_RENDER_COMMAND_TYPE_CUSTOM:
        {
            // printf("Unhandled clay cmd: custom\n");
            break;
        }
        default:
        {
            printf("Error: unhandled render command\n");
            exit(1);
        }
        }

        // Flush draw calls if scissors about to change in this iteration
        if (i == cmds.length - 1 || scissorChanged)
        {
            scissorChanged = false;
            // Render Recatangles and Images
            if (renderer->instanceCount > 0)
            {
                glUseProgram(renderer->quadShaderId);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, renderer->imageTextures[0]);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, renderer->imageTextures[1]);
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, renderer->imageTextures[2]);
                glActiveTexture(GL_TEXTURE3);
                glBindTexture(GL_TEXTURE_2D, renderer->imageTextures[3]);

                // set uniforms
                GLint locScreen = glGetUniformLocation(renderer->quadShaderId, "uScreen");
                glUniform2f(locScreen,
                            (float)renderer->screenWidth,
                            (float)renderer->screenHeight);

                glBindVertexArray(renderer->quadVAO);

                // upload all instances at once
                glBindBuffer(GL_ARRAY_BUFFER, renderer->quadInstanceVBO);

                // rectangles are solid colour — disable atlas use
                glBufferSubData(GL_ARRAY_BUFFER,
                                0,
                                renderer->instanceCount * sizeof(RectInstance),
                                renderer->quadInstanceData);
                // draw unit quad (4 verts) instanced
                glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, renderer->instanceCount);

                glBindVertexArray(0);
                glUseProgram(0);
            }
            // Clrear instance arrays, as they were flushed to their render calls
            renderer->instanceCount = 0;

            // Text rendering
            if (renderer->glyphVtxArray.glyphCount > 0)
            {
                glUseProgram(renderer->textShader);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, renderer->fontTextures[0]);

                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, renderer->fontTextures[1]);

                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, renderer->fontTextures[2]);

                glActiveTexture(GL_TEXTURE3);
                glBindTexture(GL_TEXTURE_2D, renderer->fontTextures[3]);

                GLint uScreenLoc = glGetUniformLocation(renderer->textShader, "uScreen");
                glUniform2f(uScreenLoc, renderer->screenWidth, renderer->screenHeight);

                glBindVertexArray(renderer->textVAO);
                glBindBuffer(GL_ARRAY_BUFFER, renderer->textVBO);

                glBufferSubData(GL_ARRAY_BUFFER,
                                0,
                                sizeof(struct GlyphVtx) * 6 * renderer->glyphVtxArray.glyphCount,
                                renderer->glyphVtxArray.glyphVerticeData);
                glDrawArrays(GL_TRIANGLES, 0, renderer->glyphVtxArray.glyphCount * 6);

                glBindVertexArray(0);
                glBindTexture(GL_TEXTURE_2D, 0);
            }
            renderer->glyphVtxArray.glyphCount = 0;

            if (cmd->commandType == CLAY_RENDER_COMMAND_TYPE_SCISSOR_START)
            {
                Clay_BoundingBox bb = cmd->boundingBox;
                GLint x = (GLint)bb.x;
                GLint y = (GLint)(renderer->screenHeight - (bb.y + bb.height));
                GLsizei w = (GLsizei)bb.width;
                GLsizei h = (GLsizei)bb.height;

                glEnable(GL_SCISSOR_TEST);
                glScissor(x, y, w, h);
            }
            else
            {
                glDisable(GL_SCISSOR_TEST);
            }
        }
    }
}

struct Clayton
{
    Gles3_Renderer renderer;
    Stb_FontData stbFonts[MAX_FONTS];

    Gles3_ImageConfig pinImage;
    Gles3_ImageConfig parkImage;

    void initClayton(float screenWidth, float screenHeight)
    {
        size_t clayRequiredMemory = Clay_MinMemorySize();
        this->renderer.clayMemory = (Clay_Arena){
            .capacity = clayRequiredMemory,
            .memory = (char *)malloc(clayRequiredMemory),
        };
        Clay_Context *clayCtx = Clay_Initialize(
            this->renderer.clayMemory,
            (Clay_Dimensions){
                .width = screenWidth,
                .height = screenHeight,
            },
            (Clay_ErrorHandler){
                .errorHandlerFunction = Gles3_ErrorHandler,
            });

        // Note that MeasureText has to be set after the Context is set!
        Clay_SetCurrentContext(clayCtx);
        Clay_SetMeasureTextFunction(Stb_MeasureText, &this->stbFonts);
        Gles3_SetRenderTextFunction(&this->renderer, Stb_RenderText, &this->stbFonts);

        Gles3_Initialize(&this->renderer, 4096);

        if (!Stb_LoadImage(
                &this->renderer.imageTextures[0],
                "assets/files/everything_tex.png"))
            abort();

        if (!Stb_LoadImage(
                &this->renderer.imageTextures[1],
                "assets/files/park.jpg"))
            abort();

        this->pinImage = Gles3_ImageConfig{
            .textureToUse = 0,
            .u0 = 0.0f,
            .v0 = 0.75f,
            .u1 = 0.125f,
            .v1 = 1.0f,
        };
        this->parkImage = Gles3_ImageConfig{
            .textureToUse = 1,
            .u0 = 0.0f,
            .v0 = 0.0f,
            .u1 = 1.0f,
            .v1 = 1.0f,
        };

        int atlasW = 512;
        int atlasH = 512;
        if (!Stb_LoadFont(
                &this->renderer.fontTextures[0],
                &this->stbFonts[0],
                "assets/files/Roboto-Regular.ttf",
                48.0f, // bake pixel height
                atlasW,
                atlasH))
            abort();

        if (!Stb_LoadFont(
                &this->renderer.fontTextures[1],
                &this->stbFonts[1],
                "assets/files/SUSEMono-Medium.ttf",
                48.0f, // bake pixel height
                atlasW,
                atlasH))
            abort();
    }

    void renderClayton(Clay_RenderCommandArray cmds)
    {
        Gles3_Render(&this->renderer, cmds, this->stbFonts);
    }
};
