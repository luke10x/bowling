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

struct RectInstance
{
    float x, y, w, h;         // 4
    float u0, v0, u1, v1;     // 4
    float r, g, b, a;         // 4
    float radiusTL, radiusTR; // 2
    float radiusBL, radiusBR; // 2
    float borderL, borderR;   // 2
    float borderT, borderB;   // 2
    float tex_slot;           // 1
    float pad[3];             // 3
};

struct GlyphVtx
{
    float x, y;
    float u, v;
    float r, g, b, a;
    GLuint tex;
};

typedef struct
{
    GLuint atlas_tex; // baked R8 glyph atlas
    stbtt_bakedchar *cdata;

    // baked font info
    int first_char; // e.g. 32
    int char_count; // e.g. 96
    float bake_px;  // font baking height (e.g. 48.0f)

    // batching buffer
    int glyph_capacity;
    int glyph_count;

    // CPU-side temporary buffer of vertices
    GlyphVtx *glyph_vertices;

    // GPU VBO/VAO
    GLuint textVAO;
    GLuint textVBO;

    // shader
    GLuint textShader;
    float ascent_px;  // in baked pixels (at bake_px size)
    float descent_px; // usually negative (at bake_px size)
} Gles3_Text;

typedef struct Gles3_Image
{
    GLuint textureSlot;
    float u0, v0;
    float u1, v1;
} Gles3_Image;

typedef struct
{
    Clay_Arena clayMemory;

    GLuint quadVAO;
    GLuint quadVBO;

    // pre-allocated CPU-side instance arrays
    RectInstance *instance_data; // packed per-instance floats
    int instance_capacity;       // how many instances it can hold
    int instance_count;          // how many instances does it actually hold

    GLuint vbo_instance; // dynamic instance buffer

    GLuint quadShaderId;
    // GLuint textShaderId;
    float screenWidth;
    float screenHeight;

    GLuint img_atlas_tex;

    // Text related details
    Gles3_Text text;
} Gles3_Renderer;

static inline Clay_Dimensions Gles3_MeasureText(
    Clay_StringSlice text,
    Clay_TextElementConfig *config,
    void *userData)
{
    Gles3_Text *fontData = (Gles3_Text *)userData;

    // If no font baked, fail gracefully
    if (!fontData->cdata)
    {
        fprintf(stderr, "MeasureText early exit: '%.*s' → %d x %d px\n",
                (int)text.length, text.chars, 0, 0);
        return (Clay_Dimensions){.width = 0, .height = 0};
    }

    float x = 0.0f;
    float y = 0.0f;

    const char *str = text.chars;
    int len = text.length;

    float scale = config->fontSize / fontData->bake_px;

    float letterSpacing = (float)config->letterSpacing;
    float lineHeight = (config->lineHeight > 0)
                           ? (float)config->lineHeight
                           : fontData->bake_px;

    for (int i = 0; i < len; i++)
    {
        unsigned char c = str[i];

        if (c < fontData->first_char                            // before range
            || c >= fontData->first_char + fontData->char_count // after range
        )
        {
            // Unsupported char: treat as space
            std::cerr << "Illegal char: " << c
                      << " as int: " << (int)c
                      << " first char is: " << fontData->first_char
                      << " char count is: " << fontData->char_count
                      << std::endl;
            x += fontData->bake_px * 0.25f;
            continue;
        }

        stbtt_bakedchar *b = &fontData->cdata[c - fontData->first_char];

        // horizontal advance while moving along word characters
        x += b->xadvance * scale + letterSpacing;
    }

    float ascent = fontData->ascent_px * scale;
    float descent = fontData->descent_px * scale; // negative
    float lineH = (ascent - descent);             // total line height in pixels (at requested fontSize)

    return (Clay_Dimensions){
        .width = x,
        .height = y + lineH,
    };
}

void Gles3_Render(Gles3_Renderer *self, Clay_RenderCommandArray cmds)
{
    self->text.glyph_count = 0;
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
            const Clay_TextRenderData *tr = &cmd->renderData.text;

            if (!self->text.cdata)
                break;

            Clay_StringSlice ss = tr->stringContents;
            const char *txt = ss.chars;
            int len = (int)ss.length;

            float scale = tr->fontSize / self->text.bake_px;
            float ascent = self->text.ascent_px * scale; // pixels above baseline
            // (descent is not needed here unless you want to validate box size)
            float x = cmd->boundingBox.x;
            float y = cmd->boundingBox.y + ascent; // <-- baseline, not “fontSize”

            float cr = tr->textColor.r / 255.0f;
            float cg = tr->textColor.g / 255.0f;
            float cb = tr->textColor.b / 255.0f;
            float ca = tr->textColor.a / 255.0f;

            for (int i = 0; i < len; i++)
            {
                char ch = txt[i];

                int idx = ch - self->text.first_char;
                if (idx < 0 || idx >= self->text.char_count)
                {
                    continue;
                }

                stbtt_bakedchar *bc = &self->text.cdata[idx];

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
                float atlasW = 1024.0f;
                float atlasH = 1024.0f;

                float u0 = bc->x0 / atlasW;
                float v0 = bc->y0 / atlasH;
                float u1 = bc->x1 / atlasW;
                float v1 = bc->y1 / atlasH;

                // append 6 vertices (two triangles) to your buffer
                GlyphVtx *v = &self->text.glyph_vertices[self->text.glyph_count * 6];

                v[0] = (GlyphVtx){x0, y0, u0, v0, cr, cg, cb, ca};
                v[1] = (GlyphVtx){x1, y0, u1, v0, cr, cg, cb, ca};
                v[2] = (GlyphVtx){x0, y1, u0, v1, cr, cg, cb, ca};

                v[3] = (GlyphVtx){x0, y1, u0, v1, cr, cg, cb, ca};
                v[4] = (GlyphVtx){x1, y0, u1, v0, cr, cg, cb, ca};
                v[5] = (GlyphVtx){x1, y1, u1, v1, cr, cg, cb, ca};

                self->text.glyph_count++;

                // advance pen by baked xadvance + letter spacing
                x += (bc->xadvance * scale) + tr->letterSpacing;

                // prevent buffer overrun
                if (self->text.glyph_count >= self->text.glyph_capacity)
                {
                    break;
                }
            }

            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_RECTANGLE:
        case CLAY_RENDER_COMMAND_TYPE_IMAGE:
        {
            Clay_RectangleRenderData *config = &cmd->renderData.rectangle;
            // Acquire colour (RGBA * u8)
            Clay_Color c = config->backgroundColor;

            // Convert to float 0..1
            float rf = c.r / 255.0f;
            float gf = c.g / 255.0f;
            float bf = c.b / 255.0f;
            float af = c.a / 255.0f;

            bool isImage = cmd->commandType == CLAY_RENDER_COMMAND_TYPE_IMAGE;

            // Ensure we don't overflow the capacity
            if (
                self->instance_count >= self->instance_capacity)
            {
                printf("Clay renderer: instance overflow!\n");
                break;
            }

            int idx = self->instance_count;
            RectInstance *dst = &self->instance_data[idx];
            dst->x = boundingBox.x;
            dst->y = boundingBox.y;
            dst->w = boundingBox.width;
            dst->h = boundingBox.height;

            // UVs
            if (isImage)
            {
                Gles3_Image *id = (Gles3_Image *)cmd->renderData.image.imageData;
                dst->u0 = id->u0;
                dst->v0 = id->v0;
                dst->u1 = id->u1;
                dst->v1 = id->v1;
                dst->tex_slot = 0; // TODO map from index to slot
            }
            else
            {
                dst->u0 = dst->v0 = 0.0f;
                dst->u1 = dst->v1 = 1.0f;
                dst->tex_slot = -1.0f;
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

            self->instance_count++;
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

            int idx = self->instance_count;
            RectInstance *dst = &self->instance_data[idx];

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

            dst->tex_slot = -1.0f;

            self->instance_count++;
        }

        case CLAY_RENDER_COMMAND_TYPE_CUSTOM:
        {
            printf("Unhandled clay cmd: custom\n");
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
            if (self->instance_count > 0)
            {
                glUseProgram(self->quadShaderId);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, self->img_atlas_tex);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, self->img_atlas_tex);
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, self->img_atlas_tex);
                glActiveTexture(GL_TEXTURE3);
                glBindTexture(GL_TEXTURE_2D, self->img_atlas_tex);

                glUniform1i(glGetUniformLocation(self->quadShaderId, "tex0"), 0);
                glUniform1i(glGetUniformLocation(self->quadShaderId, "tex1"), 1);
                glUniform1i(glGetUniformLocation(self->quadShaderId, "tex3"), 3);
                glUniform1i(glGetUniformLocation(self->quadShaderId, "tex4"), 4);

                // set uniforms
                GLint locScreen = glGetUniformLocation(self->quadShaderId, "uScreen");
                glUniform2f(locScreen,
                            (float)self->screenWidth,
                            (float)self->screenHeight);

                glBindVertexArray(self->quadVAO);

                // upload all instances at once
                glBindBuffer(GL_ARRAY_BUFFER, self->vbo_instance);

                // rectangles are solid colour — disable atlas use
                glBufferSubData(GL_ARRAY_BUFFER,
                                0,
                                self->instance_count * sizeof(RectInstance),
                                self->instance_data);
                // draw unit quad (4 verts) instanced
                glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, self->instance_count);

                glBindVertexArray(0);
                glUseProgram(0);
            }
            // Clrear instance arrays, as they were flushed to their render calls
            self->instance_count = 0;

            // Text rendering
            if (self->text.glyph_count > 0)
            {
                glUseProgram(self->text.textShader);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, self->text.atlas_tex);

                GLint uScreenLoc = glGetUniformLocation(self->text.textShader, "uScreen");
                glUniform2f(uScreenLoc, self->screenWidth, self->screenHeight);

                GLint loc = glGetUniformLocation(self->text.textShader, "uAtlas");
                glUniform1i(loc, 0);

                glBindVertexArray(self->text.textVAO);
                glBindBuffer(GL_ARRAY_BUFFER, self->text.textVBO);

                glBufferSubData(GL_ARRAY_BUFFER,
                                0,
                                sizeof(struct GlyphVtx) * 6 * self->text.glyph_count,
                                self->text.glyph_vertices);
                glDrawArrays(GL_TRIANGLES, 0, self->text.glyph_count * 6);

                glBindVertexArray(0);
                glBindTexture(GL_TEXTURE_2D, 0);
            }
            self->text.glyph_count = 0;

            if (cmd->commandType == CLAY_RENDER_COMMAND_TYPE_SCISSOR_START)
            {
                Clay_BoundingBox bb = cmd->boundingBox;
                GLint x = (GLint)bb.x;
                GLint y = (GLint)(self->screenHeight - (bb.y + bb.height));
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

bool Text_LoadFont(
    Gles3_Text *self,
    const char *ttf_path,
    float bake_pixel_height, // Height of a char in pixels
    int atlas_w,             // Width of atlas in pixels
    int atlas_h              // Height of atlas in pixels
)
{
    self->first_char = 32; // ASCII space
    self->char_count = 96; // 32..127
    self->bake_px = bake_pixel_height;

    // allocate baked-char array
    self->cdata = (stbtt_bakedchar *)malloc(sizeof(stbtt_bakedchar) * self->char_count);
    if (!self->cdata)
    {
        fprintf(stderr, "Cannot allocate cdata\n");
        return false;
    }

    // load font file
    FILE *f = fopen(ttf_path, "rb");
    if (!f)
    {
        fprintf(stderr, "Could not open font: %s\n", ttf_path);
        return false;
    }

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    unsigned char *ttf_buf = (unsigned char *)malloc(sz);
    fread(ttf_buf, 1, sz, f);
    fclose(f);

    // temporary atlas memory
    unsigned char *atlas = (unsigned char *)malloc(atlas_w * atlas_h);
    memset(atlas, 0, atlas_w * atlas_h);

    // bake
    int res = stbtt_BakeFontBitmap(
        ttf_buf,           // raw TTF file
        0,                 // font index inside TTF (0 = first font)
        bake_pixel_height, // pixel height of glyphs to generate
        atlas,             // OUT: bitmap buffer (unsigned char*)
        atlas_w, atlas_h,  // size of bitmap buffer
        self->first_char,  // first character to bake (e.g., 32 = space)
        self->char_count,  // how many sequential chars to bake
        self->cdata        // OUT: array of stbtt_bakedchar
    );

    stbtt_fontinfo fi;
    if (!stbtt_InitFont(&fi, ttf_buf, stbtt_GetFontOffsetForIndex(ttf_buf, 0)))
    {
        // handle error...
    }

    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&fi, &ascent, &descent, &lineGap);

    // Convert the font's "font units" to pixels at your bake_px size:
    float scale_for_bake = stbtt_ScaleForPixelHeight(&fi, bake_pixel_height);

    self->ascent_px = ascent * scale_for_bake;
    self->descent_px = descent * scale_for_bake; // this is typically negative

    free(ttf_buf);

    if (res <= 0)
    {
        fprintf(stderr, "Font baking failed\n");
        free(atlas);
        free(self->cdata);
        self->cdata = NULL;
        return false;
    }
    else
    {
        printf("This many chars baked: %d\n", res);
    }

    // upload atlas to OpenGL
    glGenTextures(1, &self->atlas_tex);
    glBindTexture(GL_TEXTURE_2D, self->atlas_tex);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8,
                 atlas_w, atlas_h,
                 0, GL_RED, GL_UNSIGNED_BYTE, atlas);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);

    free(atlas);

    return true;
}

struct Clayton
{
    static const char *CLAYTON_QUAD_VERTEX_SHADER;
    static const char *CLAYTON_QUAD_FRAGMENT_SHADER;
    static const char *CLAYTON_TEXT_VERTEX_SHADER;
    static const char *CLAYTON_TEXT_FRAGMENT_SHADER;

    Gles3_Renderer renderer;

    Gles3_Image pinPicture;

    void initClayton(float screenWidth, float screenHeight, int max_instances)
    {
        // Atlas will be same size
        int atlas_w = 1024;
        int atlas_h = 1024;
        if (!Text_LoadFont(&this->renderer.text,
                           "assets/files/Roboto-Regular.ttf",
                           48.0f, // bake pixel height
                           atlas_w,
                           atlas_h))
        {
            fprintf(stderr, "Font load failed\n");
        }
        Gles3_Text *t_ = &this->renderer.text;
        std::cerr << "loaded text  from " << t_->first_char
                  << std::endl;
        fprintf(stderr, "Text address after loading is %p", t_);

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
        Clay_SetMeasureTextFunction(Gles3_MeasureText, &this->renderer.text);

        this->renderer.screenWidth = screenWidth;
        this->renderer.screenHeight = screenHeight;

        // compile shader
        this->renderer.quadShaderId = vtx::createShaderProgram(
            CLAYTON_QUAD_VERTEX_SHADER, CLAYTON_QUAD_FRAGMENT_SHADER);

        // create unit quad VBO (0..1)
        const float quad_verts[8] = {0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f};
        glGenVertexArrays(1, &this->renderer.quadVAO);
        glBindVertexArray(this->renderer.quadVAO);

        glGenBuffers(1, &this->renderer.quadVBO);
        glBindBuffer(GL_ARRAY_BUFFER, this->renderer.quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad_verts), quad_verts, GL_STATIC_DRAW);

        // attribute 0: aPos (vec2), per-vertex
        glEnableVertexAttribArray(ATTR_POS);
        glVertexAttribPointer(ATTR_POS, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
        glVertexAttribDivisor(ATTR_POS, 0);

        // create instance buffer big enough
        this->renderer.instance_capacity = (max_instances > 0 ? max_instances : 4096);
        this->renderer.instance_data =
            (RectInstance *)malloc(sizeof(RectInstance) * this->renderer.instance_capacity);
        this->renderer.instance_count = 0;

        glGenBuffers(1, &this->renderer.vbo_instance);
        glBindBuffer(GL_ARRAY_BUFFER, this->renderer.vbo_instance);
        glBufferData(GL_ARRAY_BUFFER,
                     sizeof(RectInstance) * this->renderer.instance_capacity,
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
                              stride, (void *)offsetof(RectInstance, tex_slot));
        glVertexAttribDivisor(ATTR_TEX, 1);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Ok now we will initialize text!
        Gles3_Text *t = &this->renderer.text;

        // configure capacity
        t->glyph_capacity = 4096; // adjust as needed
        t->glyph_count = 0;

        // allocate CPU-side vertex buffer: 6 vertices per glyph
        t->glyph_vertices = (GlyphVtx *)malloc(sizeof(GlyphVtx) * 6 * t->glyph_capacity);
        if (!t->glyph_vertices)
        {
            fprintf(stderr, "Failed to allocate glyph_vertices\n");
            t->glyph_capacity = 0;
        }

        // create VAO/VBO for text rendering
        glGenVertexArrays(1, &t->textVAO);
        glBindVertexArray(t->textVAO);

        glGenBuffers(1, &t->textVBO);
        glBindBuffer(GL_ARRAY_BUFFER, t->textVBO);
        glBufferData(GL_ARRAY_BUFFER,
                     sizeof(GlyphVtx) * 6 * t->glyph_capacity,
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

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        this->renderer.text.textShader = vtx::createShaderProgram(
            CLAYTON_TEXT_VERTEX_SHADER, CLAYTON_TEXT_FRAGMENT_SHADER);
        glUseProgram(this->renderer.text.textShader);

        // Bind the texture to unit 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, this->renderer.text.atlas_tex);

        // Tell the shader that uAtlas = texture unit 0
        GLint loc = glGetUniformLocation(this->renderer.text.textShader, "uAtlas");
        glUniform1i(loc, 0);

        GLuint textureId;
        {
            const char *texturePath = "assets/files/everything_tex.png";
            std::cerr << "Start of loading image " << std::endl;
            const acl::LoadedImage *li = acl::loadImage(texturePath, false);
            unsigned char *data = li->data;
            int width = li->width;
            int height = li->height;
            int nrChannels = li->channels;

            glGenTextures(1, &textureId);
            glBindTexture(GL_TEXTURE_2D, textureId);

            // Set filtering to nearest (closest UV sampling)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            // Set wrap mode (optional, for UV coordinates outside [0,1])
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

            // Upload texture data
            if (data)
            {
                GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
                glTexImage2D(
                    GL_TEXTURE_2D, // target
                    0,             // level
                    format,        // internal format int
                    width,
                    height,
                    0,                // border
                    format,           // format, GLEnum
                    GL_UNSIGNED_BYTE, // Type
                    data              // pixels
                );
                glGenerateMipmap(GL_TEXTURE_2D);
                acl::freeImage(li);
            }
            else
            {
                std::cerr << "Failed to load texture at: " << texturePath << std::endl;
                abort();
            }
        }

        this->renderer.img_atlas_tex = textureId;

        this->pinPicture = Gles3_Image{
            .textureSlot = 0, // ok will be more
            .u0 = 0.0f,
            .v0 = 0.75f,
            .u1 = 0.125f,
            .v1 = 1.0,
        };
    }

    void renderClayton(Clay_RenderCommandArray cmds)
    {
        Gles3_Render(&this->renderer, cmds);
    }
};

const char *Clayton::CLAYTON_QUAD_VERTEX_SHADER =
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

const char *Clayton::CLAYTON_QUAD_FRAGMENT_SHADER =
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

    uniform sampler2D tex0;
    uniform sampler2D tex1;
    uniform sampler2D tex2;
    uniform sampler2D tex3;

    out vec4 frag;
    void main(){

        // Pixel coordinates inside the rectangle
        vec2 pix = vRect.xy + vPos * vRect.zw;

        float x0 = vRect.x;
        float y0 = vRect.y;
        float w  = vRect.z;
        float h  = vRect.w;

        // Convert pix to local pixel-space inside rect (0..w, 0..h)
        vec2 local = pix - vec2(x0, y0);

        float tl = vCornerRadii.x;
        float tr = vCornerRadii.y;
        float bl = vCornerRadii.z;
        float br = vCornerRadii.w;

        // ------ Outer rounded-rectangle clip ------
        float outerAlpha = 1.0;

        if (tl > 0.0 && local.x < tl && local.y < tl) {
            float d = length(local - vec2(tl, tl));
            outerAlpha = step(d, tl);
        }

        if (tr > 0.0 && local.x > w - tr && local.y < tr) {
            float d = length(local - vec2(w - tr, tr));
            outerAlpha *= step(d, tr);
        }

        if (bl > 0.0 && local.x < bl && local.y > h - bl) {
            float d = length(local - vec2(bl, h - bl));
            outerAlpha *= step(d, bl);
        }

        if (br > 0.0 && local.x > w - br && local.y > h - br) {
            float d = length(local - vec2(w - br, h - br));
            outerAlpha *= step(d, br);
        }

        if (outerAlpha < 0.5)
            discard;

        bool isBorder =
            vBorderWidths.x > 0.0 ||
            vBorderWidths.y > 0.0 ||
            vBorderWidths.z > 0.0 ||
            vBorderWidths.w > 0.0;

        if (isBorder) {
            // ------ Inner rounded rectangle ------
            float leftB   = vBorderWidths.x;
            float rightB  = vBorderWidths.y;
            float topB    = vBorderWidths.z;
            float bottomB = vBorderWidths.w;

            // Inner rect dims
            float iw = w - leftB - rightB;
            float ih = h - topB - bottomB;

            // Local coords relative to inner-rect origin
            vec2 innerLocal = local - vec2(leftB, topB);

            // Compute inner corner radii
            float tl_i = max(tl - topB, 0.0);
            float tr_i = max(tr - topB, 0.0);
            float bl_i = max(bl - bottomB, 0.0);
            float br_i = max(br - bottomB, 0.0);

            // Check if pixel is inside INNER rounded-rect
            bool insideInnerRounded = true;

            // Top-left inner corner
            if (tl_i > 0.0 && innerLocal.x < tl_i && innerLocal.y < tl_i) {
                float d = length(innerLocal - vec2(tl_i, tl_i));
                insideInnerRounded = (d <= tl_i);
            }

            // Top-right
            if (tr_i > 0.0 && innerLocal.x > iw - tr_i && innerLocal.y < tr_i) {
                float d = length(innerLocal - vec2(iw - tr_i, tr_i));
                insideInnerRounded = insideInnerRounded && (d <= tr_i);
            }

            // Bottom-left
            if (bl_i > 0.0 && innerLocal.x < bl_i && innerLocal.y > ih - bl_i) {
                float d = length(innerLocal - vec2(bl_i, ih - bl_i));
                insideInnerRounded = insideInnerRounded && (d <= bl_i);
            }

            // Bottom-right
            if (br_i > 0.0 && innerLocal.x > iw - br_i && innerLocal.y > ih - br_i) {
                float d = length(innerLocal - vec2(iw - br_i, ih - br_i));
                insideInnerRounded = insideInnerRounded && (d <= br_i);
            }

            // If pixel is inside inner rounded-rect → discard from border
            if (insideInnerRounded &&
                innerLocal.x >= 0.0 && innerLocal.x <= iw &&
                innerLocal.y >= 0.0 && innerLocal.y <= ih)
            {
                discard;
            } else {
                frag = vColor;
            }
            return;
        }
        // End of border draw, no normal colour-rects
        if (vTexSlot < 0.0) {
            frag = vColor;
        } else {
            // And here just images
            int slot = int(vTexSlot + 0.5);  // safe conversion
            if (slot == 0) frag = texture(tex0, vUV);
            if (slot == 1) frag = texture(tex1, vUV);
            if (slot == 2) frag = texture(tex2, vUV);
            if (slot == 3) frag = texture(tex3, vUV);
        }
    }
    )";

const char *Clayton::CLAYTON_TEXT_VERTEX_SHADER =
    GLSL_VERSION
    R"(
    precision mediump float;

    layout(location = 0) in vec2 aPos;
    layout(location = 1) in vec2 aUV;
    layout(location = 2) in vec4 aColor;

    uniform vec2 uScreen;

    out vec2 vUV;
    out vec4 vColor;

    void main() {
        vec2 p = (aPos / uScreen) * 2.0 - 1.0;
        p.y = -p.y;
        gl_Position = vec4(p, 0.0, 1.0);


    vec2 ndc = (aPos / uScreen) * 2.0 - 1.0;
    gl_Position = vec4(ndc * vec2(1.0, -1.0), 0.0, 1.0);

        
        vUV = aUV;
        vColor = aColor;
    }
    )";

const char *Clayton::CLAYTON_TEXT_FRAGMENT_SHADER =
    GLSL_VERSION
    R"(
    precision mediump float;

    in vec2 vUV;
    in vec4 vColor;

    uniform sampler2D uAtlas;
    out vec4 fragColor;

    void main() {
        float coverage = texture(uAtlas, vUV).r;
        fragColor = vec4(vColor.rgb, vColor.a * coverage);
    } 
    )";