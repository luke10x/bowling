#pragma once

#include "../framework/gl_header.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdio.h>

struct OilMap
{
    GLuint program = 0;
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;

    void init()
    {
        if (program != 0)
            return;

        const char *vs =
            GLSL_VERSION "\n"
            "precision highp float;\n"
            "precision highp int;\n"
            "layout(location=0) in vec2 aPos;\n"
            "layout(location=1) in vec2 aUV;\n"
            "out vec2 vUV;\n"
            "void main(){ vUV = vec2(aUV.x, 1.0 - aUV.y); gl_Position = vec4(aPos, 0.0, 1.0); }\n";

        const char *fs =
            GLSL_VERSION "\n"
            "precision highp float;\n"
            "precision highp int;\n"
            "in vec2 vUV;\n"
            "out vec4 frag;\n"
            "uniform float uLaneLenM;\n"
            "uniform float uLeftStartM;\n"
            "uniform float uLeftEndM;\n"
            "uniform float uRightStartM;\n"
            "uniform float uRightEndM;\n"
            "uniform float uOilThickness;\n"
            "uniform float uPushback01;\n"
            "\n"
            "float remap01(float x, float a, float b){ return clamp((x - a) / max(1e-6, (b - a)), 0.0, 1.0); }\n"
            "\n"
            "void main(){\n"
            "  // Lane vertical axis: vUV.y=0 near player (start), vUV.y=1 towards pins (end)\n"
            "  float laneM = vUV.y * uLaneLenM;\n"
            "  float ls = min(uLeftStartM, uLeftEndM);\n"
            "  float le = max(uLeftStartM, uLeftEndM);\n"
            "  float rs = min(uRightStartM, uRightEndM);\n"
            "  float re = max(uRightStartM, uRightEndM);\n"
            "\n"
            "  // Side column widths grow with oil thickness.\n"
            "  float sideW = clamp(uOilThickness, 0.0, 1.0) * 0.33;\n"
            "  float leftW = sideW;\n"
            "  float rightW = sideW;\n"
            "  float midW = max(1e-6, 1.0 - leftW - rightW);\n"
            "\n"
            "  float x = vUV.x;\n"
            "  float xMid01 = remap01(x, leftW, leftW + midW);\n"
            "  float startM = mix(ls, rs, xMid01);\n"
            "  float endM = mix(le, re, xMid01);\n"
            "  float yStart = startM / uLaneLenM;\n"
            "  float yEnd = endM / uLaneLenM;\n"
            "\n"
            "  // Row selection by lane position (3 rows):\n"
            "  // [0..yStart) fully-oiled: RED\n"
            "  // [yStart..yEnd) fade: interpolate BLACK->RED\n"
            "  // [yEnd..1] no-oil: BLACK\n"
            "  // Oil thickness scales red intensity (t=1 bright red, t=0 black).\n"
            "  float t = clamp(uOilThickness, 0.0, 1.0);\n"
            "  // Multiplicative shading: goes black if any factor is 0.\n"
            "  // - oilK: 1 in fully-oiled, fades to 0 across fade band, 0 in no-oil.\n"
            "  // - t: oil thickness (0..1)\n"
            "  // - center01: 1 at center, 0 at edges\n"
            "  float oilK;\n"
            "  if (vUV.y < yStart) {\n"
            "    oilK = 1.0;\n"
            "  } else if (vUV.y < yEnd) {\n"
            "    float f = remap01(vUV.y, yStart, yEnd);\n"
            "    oilK = 1.0 - f;\n"
            "  } else {\n"
            "    oilK = 0.0;\n"
            "  }\n"
            "  float thicknessK = clamp(oilK * t, 0.0, 1.0);\n"
            "  // Red channel shows oil thickness only.\n"
            "  float r = thicknessK;\n"
            "  // Blue channel shows sideforce: stronger near sides, scaled by pushback and thickness.\n"
            "  float side01 = clamp(abs(vUV.x - 0.5) / 0.5, 0.0, 1.0);\n"
            "  float b = thicknessK * clamp(uPushback01, 0.0, 1.0) * side01;\n"
            "  vec3 col = vec3(r, 0.0, b);\n"
            "\n"
            "\n"
            "  frag = vec4(col, 1.0);\n"
            "}\n";

        auto compile = [](GLenum type, const char *src) -> GLuint {
            GLuint s = glCreateShader(type);
            glShaderSource(s, 1, &src, nullptr);
            glCompileShader(s);
            GLint ok = 0;
            glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
            if (!ok)
            {
                char log[2048];
                glGetShaderInfoLog(s, sizeof(log), nullptr, log);
                printf("OilMap shader compile error: %s\n", log);
            }
            return s;
        };

        GLuint v = compile(GL_VERTEX_SHADER, vs);
        GLuint f = compile(GL_FRAGMENT_SHADER, fs);
        program = glCreateProgram();
        glAttachShader(program, v);
        glAttachShader(program, f);
        glLinkProgram(program);
        glDeleteShader(v);
        glDeleteShader(f);

        float verts[] = {
            // pos      // uv
            -1.0f, -1.0f, 0.0f, 0.0f,
            1.0f,  -1.0f, 1.0f, 0.0f,
            1.0f,  1.0f,  1.0f, 1.0f,
            -1.0f, 1.0f,  0.0f, 1.0f,
        };
        unsigned int idx[] = {0, 1, 2, 0, 2, 3};

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
        glBindVertexArray(0);
    }

    void render(
        float laneLenM,
        float leftStartM,
        float leftEndM,
        float rightStartM,
        float rightEndM,
        float oilThickness,
        float pushback01
    )
    {
        if (program == 0)
            init();

        glUseProgram(program);
        glUniform1f(glGetUniformLocation(program, "uLaneLenM"), laneLenM);
        glUniform1f(glGetUniformLocation(program, "uLeftStartM"), leftStartM);
        glUniform1f(glGetUniformLocation(program, "uLeftEndM"), leftEndM);
        glUniform1f(glGetUniformLocation(program, "uRightStartM"), rightStartM);
        glUniform1f(glGetUniformLocation(program, "uRightEndM"), rightEndM);
        glUniform1f(glGetUniformLocation(program, "uOilThickness"), oilThickness);
        glUniform1f(glGetUniformLocation(program, "uPushback01"), pushback01);

        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
};
