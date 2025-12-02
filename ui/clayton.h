#pragma once

// C++ libs
#include <stdlib.h>

// C libs
#define CLAY_IMPLEMENTATION
#include <clay.h>

enum {
    ATTR_POS = 0,    // vec2
    ATTR_RECT = 1,   // vec4 (x,y,w,h)
    ATTR_COLOR = 2,  // vec4
    ATTR_UV = 3      // vec4 (u0,v0,u1,v1)
};

#define INSTANCE_FLOATS_PER 12

typedef struct {
    Clay_LayoutConfig layoutElement;
    Clay_Arena clayMemory;

    GLuint quadVAO;
    GLuint quadVBO;

    // pre-allocated CPU-side instance arrays
    float *instance_data; // packed per-instance floats
    int instance_capacity; // how many instances it can hold
    int instance_count; // how many instances does it actually hold

    GLuint vbo_instance; // dynamic instance buffer

    GLuint quadShaderId;
    float width;
    float height;
} Gles3_Renderer;


void Gles3_ErrorHandler(Clay_ErrorData errorData)
{
    printf("[ClaY ErroR] %s", errorData.errorText.chars);
}

static inline Clay_Dimensions Gles3_MeasureText(
    Clay_StringSlice text,
    Clay_TextElementConfig *config, // etc
    void *userData                  // This will be passed
)
{
    Gles3_Renderer *renderer = (Gles3_Renderer *)userData;

    return Clay_Dimensions{
        .width = 100.0f,
        .height = 15.0f,
    };
}

void Gles3_Render(Gles3_Renderer self, Clay_RenderCommandArray cmds)
{
    for (int i = 0; i < cmds.length; i++)
    {
        Clay_RenderCommand *cmd = Clay_RenderCommandArray_Get(&cmds, i);
        Clay_BoundingBox boundingBox = (Clay_BoundingBox){
            .x = roundf(cmd->boundingBox.x),
            .y = roundf(cmd->boundingBox.y),
            .width = roundf(cmd->boundingBox.width),
            .height = roundf(cmd->boundingBox.height),
        };
        switch (cmd->commandType)
        {
            case CLAY_RENDER_COMMAND_TYPE_TEXT: {
                printf("Unhandled clay cmd: text\n");
                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_IMAGE: {
                printf("Unhandled clay cmd: image\n");
                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {
                printf("Unhandled clay cmd: scissor start\n");
                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END: {
                printf("Unhandled clay cmd: scissor end\n");
                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {

            // Acquire colour (u8)
            Clay_Color c;
            c.r = 255;
            c.g = 100;
            c.b = 100;
            c.a = 100;

            // Convert to float 0..1
            float rf = c.r / 255.0f;
            float gf = c.g / 255.0f;
            float bf = c.b / 255.0f;
            float af = c.a / 255.0f;

            // Ensure we don't overflow the capacity
            if (self.instance_count >= self.instance_capacity) {
                printf("Clay renderer: instance overflow!\n");
                break;
            }

            // Pointer to this instance's 12 floats
            int idx = self.instance_count * INSTANCE_FLOATS_PER;
            float *dst = &self.instance_data[idx];

            // Write RECT (4 floats): x,y,w,h
            dst[0] = boundingBox.x;
            dst[1] = boundingBox.y;
            dst[2] = boundingBox.width;
            dst[3] = boundingBox.height;

            // Write UV (4 floats) — always full quad
            dst[4] = 0.0f;
            dst[5] = 0.0f;
            dst[6] = 1.0f;
            dst[7] = 1.0f;

            // Write COLOR (4 floats)
            dst[8]  = rf;
            dst[9]  = gf;
            dst[10] = bf;
            dst[11] = af;

            self.instance_count++;
        break;
                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_BORDER: {
                printf("Unhandled clay cmd: border\n");
                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_CUSTOM: {
                printf("Unhandled clay cmd: custom\n");
                break;
            }
            default: {
                printf("Error: unhandled render command\n");
                exit(1);
            }
        }
    }
    // I suppose now I ned to do the render call here, 
    // Assitant please assist me just with those 2 things please 
    // ----------- FINAL DRAW CALL -----------------
    if (self.instance_count > 0)
    {
        glUseProgram(self.quadShaderId);

        // set uniforms
        GLint locScreen = glGetUniformLocation(self.quadShaderId, "uScreen");
        glUniform2f(locScreen,
                    (float)self.width,
                    (float)self.height);

        // rectangles are solid colour — disable atlas use
        glUniform1i(glGetUniformLocation(self.quadShaderId, "uUseAtlas"), 0);

        glBindVertexArray(self.quadVAO);

        // upload all instances at once
        glBindBuffer(GL_ARRAY_BUFFER, self.vbo_instance);
        glBufferSubData(GL_ARRAY_BUFFER,
                        0,
                        self.instance_count * INSTANCE_FLOATS_PER * sizeof(float),
                        self.instance_data);

        // draw unit quad (4 verts) instanced
        glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, self.instance_count);

        glBindVertexArray(0);
        glUseProgram(0);

        // reset counter for next frame
        self.instance_count = 0;
    }
}

struct Clayton
{
    static const char *CLAYTON_QUAD_VERTEX_SHADER;
    static const char *CLAYTON_QUAD_FRAGMENT_SHADER;

    Gles3_Renderer renderer;

    void initClayton(float width, float height, int max_instances)
    {
        // int clay_init_result = clay_gl_init("assets/files/Roboto-Regular.ttf", 512, 512, 200);
        Clay_SetMeasureTextFunction(Gles3_MeasureText, this);

        size_t clayRequiredMemory = Clay_MinMemorySize();
        this->renderer.clayMemory = (Clay_Arena){
            .capacity = clayRequiredMemory,
            .memory = (char *)malloc(clayRequiredMemory),
        };
        Clay_Initialize(
            this->renderer.clayMemory,
            (Clay_Dimensions){
                .width = width,
                .height = height,
            },
            (Clay_ErrorHandler){
                .errorHandlerFunction = Gles3_ErrorHandler,
            });
        this->renderer.layoutElement = (Clay_LayoutConfig){.padding = {5}};
        this->renderer.width = width;
        this->renderer.height = height;

        // compile shader
        this->renderer.quadShaderId = vtx::createShaderProgram(
            CLAYTON_QUAD_VERTEX_SHADER, CLAYTON_QUAD_FRAGMENT_SHADER);

        // create unit quad VBO (0..1)
        const float quad_verts[8] = { 0.0f,0.0f,  1.0f,0.0f,  1.0f,1.0f,  0.0f,1.0f };
        glGenVertexArrays(1, &this->renderer.quadVAO);
        glBindVertexArray(this->renderer.quadVAO);

        glGenBuffers(1, &this->renderer.quadVBO);
        glBindBuffer(GL_ARRAY_BUFFER, this->renderer.quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad_verts), quad_verts, GL_STATIC_DRAW);

        // attribute 0: aPos (vec2), per-vertex
        glEnableVertexAttribArray(ATTR_POS);
        glVertexAttribPointer(ATTR_POS, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glVertexAttribDivisor(ATTR_POS, 0);

        // create instance buffer big enough
        this->renderer.instance_capacity = (max_instances > 0 ? max_instances : 4096);
        this->renderer.instance_data = (float*)malloc(sizeof(float) * INSTANCE_FLOATS_PER * this->renderer.instance_capacity);
        this->renderer.instance_count = 0;

        glGenBuffers(1, &this->renderer.vbo_instance);
        glBindBuffer(GL_ARRAY_BUFFER, this->renderer.vbo_instance);
        glBufferData(GL_ARRAY_BUFFER,
            sizeof(float) * INSTANCE_FLOATS_PER * this->renderer.instance_capacity, 
            NULL,
            GL_DYNAMIC_DRAW
        );

        // set up instance attributes
        // layout in CPU-side: [rect(4), uv(4), color(4)] = 12 floats, bytes stride = 12 * sizeof(float)
        GLsizei stride = INSTANCE_FLOATS_PER * sizeof(float);

        // aRect at location ATTR_RECT (vec4) offset 0
        glEnableVertexAttribArray(ATTR_RECT);
        glVertexAttribPointer(ATTR_RECT, 4, GL_FLOAT, GL_FALSE, stride, (void*)(0));
        glVertexAttribDivisor(ATTR_RECT, 1);

        // aUV at location ATTR_UV offset 4 floats
        glEnableVertexAttribArray(ATTR_UV);
        glVertexAttribPointer(ATTR_UV, 4, GL_FLOAT, GL_FALSE, stride, (void*)(4 * sizeof(float)));
        glVertexAttribDivisor(ATTR_UV, 1);

        // aColor at location ATTR_COLOR offset 8 floats
        glEnableVertexAttribArray(ATTR_COLOR);
        glVertexAttribPointer(ATTR_COLOR, 4, GL_FLOAT, GL_FALSE, stride, (void*)(8 * sizeof(float)));
        glVertexAttribDivisor(ATTR_COLOR, 1);

        // unbind
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void renderClayton(Clay_RenderCommandArray cmds)
    {
        Gles3_Render(this->renderer, cmds);
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
    uniform vec2 uScreen;                     // screen size in pixels
    out vec4 vColor;
    out vec2 vUV;
    void main() {
        vec2 pos = vec2(aPos.x * aRect.z + aRect.x, aPos.y * aRect.w + aRect.y);
        vec2 ndc = pos / uScreen * 2.0 - 1.0; // ndc.y increases up; pos y increases down (we will inve
        ndc.y = -ndc.y;
        gl_Position = vec4(ndc, 0.0, 1.0);
        vColor = aColor;
        vUV = mix(aUV.xy, aUV.zw, aPos);
    }
    )";

const char *Clayton::CLAYTON_QUAD_FRAGMENT_SHADER = 
    GLSL_VERSION
    R"(
    precision mediump float;
    in vec4 vColor;
    in vec2 vUV;
    uniform sampler2D uAtlas; // R8 atlas for glyphs
    uniform int uUseAtlas;    // 1 = sample atlas alpha, 0 = solid colour
    out vec4 frag;
    void main(){
        if (uUseAtlas == 1) {
            float a = texture(uAtlas, vUV).r; // atlas stored in red channel
            frag = vec4(vColor.rgb, vColor.a * a);
        } else {
            frag = vColor;
        }
    }
    )";