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
            "uniform float uHouseLeftStartM;\n"
            "uniform float uHouseLeftEndM;\n"
            "uniform float uOilThickness;\n"
            "uniform float uPushback01;\n"
            "\n"
            "float remap01(float x, float a, float b){ return clamp((x - a) / max(1e-6, (b - a)), 0.0, 1.0); }\n"
            "float lineMask(float dist, float halfWidth){ return 1.0 - smoothstep(halfWidth, halfWidth + 0.0025, dist); }\n"
            "float circleMask(vec2 p, vec2 c, float r){ float d = length(p - c) - r; return 1.0 - smoothstep(0.0, 0.0235, d); }\n"
            "\n"
            "void main(){\n"
            "  // Reserve a thin indicator column on the right for house oil markers (◀).\n"
            "  float indicatorW = 0.10;\n"
            "  bool inIndicators = (vUV.x > 1.0 - indicatorW);\n"
            "  float mapX = clamp(vUV.x / max(1e-6, (1.0 - indicatorW)), 0.0, 1.0);\n"
            "\n"
            "  // Lane vertical axis: vUV.y=0 near player (start), vUV.y=1 towards pins (end)\n"
            "  float laneM = vUV.y * uLaneLenM;\n"
            "  float ls = min(uLeftStartM, uLeftEndM);\n"
            "  float le = max(uLeftStartM, uLeftEndM);\n"
            "  float rs = min(uRightStartM, uRightEndM);\n"
            "  float re = max(uRightStartM, uRightEndM);\n"
            "  float hls = min(uHouseLeftStartM, uHouseLeftEndM);\n"
            "  float hle = max(uHouseLeftStartM, uHouseLeftEndM);\n"
            "\n"
            "  // Side column widths grow with oil thickness.\n"
            "  float sideW = clamp(uOilThickness, 0.0, 1.0) * 0.33;\n"
            "  float leftW = sideW;\n"
            "  float rightW = sideW;\n"
            "  float midW = max(1e-6, 1.0 - leftW - rightW);\n"
            "\n"
            "  float x = mapX;\n"
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
            "  // Visual boost: small oil amounts would read as black, so lift red to a 20% floor,\n"
            "  // but still show true black when there is effectively no oil.\n"
            "  float visCutoff = 0.02;\n"
            "  float visFloor = 0.20;\n"
            "  float visK = (thicknessK < visCutoff) ? 0.0 : (visFloor + (1.0 - visFloor) * thicknessK);\n"
            "  // Red channel shows (boosted) oil thickness.\n"
            "  float r = visK;\n"
            "  // Blue channel shows sideforce: stronger near sides, scaled by pushback and thickness.\n"
            "  float side01 = clamp(abs(mapX - 0.5) / 0.5, 0.0, 1.0);\n"
            "  float b = visK * clamp(uPushback01, 0.0, 1.0) * side01;\n"
            "  vec3 col = vec3(r, 0.0, b);\n"
            "\n"
            "\n"
            "  if (inIndicators) {\n"
            "    float y0 = clamp(hls / max(1e-6, uLaneLenM), 0.0, 1.0);\n"
            "    float y1 = clamp(hle / max(1e-6, uLaneLenM), 0.0, 1.0);\n"
            "    float localX = (vUV.x - (1.0 - indicatorW)) / max(1e-6, indicatorW);\n"
            "    // Flip horizontally so the markers point RIGHT (▶).\n"
            "    localX = 1.0 - localX;\n"
            "    float triW = 0.85;\n"
            "    float triH = 0.020;\n"
            "    float a0 = triH * (1.0 - localX / max(1e-6, triW));\n"
            "    float tri0 = step(localX, triW) * step(abs(vUV.y - y0), a0);\n"
            "    float tri1 = step(localX, triW) * step(abs(vUV.y - y1), a0);\n"
            "    float tri = clamp(tri0 + tri1, 0.0, 1.0);\n"
            "    // Make the indicator margin transparent except the triangles.\n"
            "    frag = vec4(vec3(1.0), tri);\n"
            "    return;\n"
            "  }\n"
            "\n"
            "  // ===== White overlays drawn on top of oil colors =====\n"
            "  float laneBorderW = 0.006;\n"
            "  float sideBorderW = 0.006 * 4.0;\n"
            "  float border = 0.0;\n"
            "  border = max(border, lineMask(abs(mapX - 0.0), sideBorderW));\n"
            "  border = max(border, lineMask(abs(mapX - 1.0), sideBorderW));\n"
            "  border = max(border, lineMask(abs(vUV.y - 0.0), laneBorderW));\n"
	        "  border = max(border, lineMask(abs(vUV.y - 1.0), laneBorderW));\n"
	        "\n"
            "  // No internal borders; only the outer lane border.\n"
            "  // Pins near the end of the lane (small circles, approximate placement).\n"
            "  float pins = 0.0;\n"
            "  float pinR = 0.010;\n"
            "  float py0 = 0.90;\n"
            "  float rowDy = 0.028;\n"
            "  float dx = 0.250;\n"
            "  vec2 p = vec2(mapX, vUV.y);\n"
            "  pins = max(pins, circleMask(p, vec2(0.50, py0 + 0.0 * rowDy), pinR));\n"
            "  pins = max(pins, circleMask(p, vec2(0.50 - dx * 0.5, py0 + 1.0 * rowDy), pinR));\n"
            "  pins = max(pins, circleMask(p, vec2(0.50 + dx * 0.5, py0 + 1.0 * rowDy), pinR));\n"
            "  pins = max(pins, circleMask(p, vec2(0.50 - dx, py0 + 2.0 * rowDy), pinR));\n"
            "  pins = max(pins, circleMask(p, vec2(0.50 + 0.0, py0 + 2.0 * rowDy), pinR));\n"
            "  pins = max(pins, circleMask(p, vec2(0.50 + dx, py0 + 2.0 * rowDy), pinR));\n"
            "  pins = max(pins, circleMask(p, vec2(0.50 - dx * 1.5, py0 + 3.0 * rowDy), pinR));\n"
            "  pins = max(pins, circleMask(p, vec2(0.50 - dx * 0.5, py0 + 3.0 * rowDy), pinR));\n"
            "  pins = max(pins, circleMask(p, vec2(0.50 + dx * 0.5, py0 + 3.0 * rowDy), pinR));\n"
	            "  pins = max(pins, circleMask(p, vec2(0.50 + dx * 1.5, py0 + 3.0 * rowDy), pinR));\n"
	            "\n"
            "  // Lane markings (approx): dots + arrows (given distances from foul line).\n"
            "  float marks = 0.0;\n"
            "  // Lane markings are specified as distance from the foul line (lane start).\n"
            "  // In this oilmap, lane start (foul line) is at vUV.y = 0.\n"
            "  float yDots = clamp(1.83 / max(1e-6, uLaneLenM), 0.0, 1.0);\n"
            "  // 7 dots in one line across the lane.\n"
            "  float dotsLine = 0.0;\n"
            "  float dotXs[7];\n"
            "  dotXs[0] = 0.10; dotXs[1] = 0.23; dotXs[2] = 0.36; dotXs[3] = 0.50;\n"
            "  dotXs[4] = 0.64; dotXs[5] = 0.77; dotXs[6] = 0.90;\n"
            "  for (int i = 0; i < 7; i++) {\n"
            "    dotsLine = max(dotsLine, circleMask(p, vec2(dotXs[i], yDots), 0.008));\n"
            "  }\n"
            "  marks = max(marks, dotsLine);\n"
            "  float yArrow0 = clamp(3.66 / max(1e-6, uLaneLenM), 0.0, 1.0);\n"
            "  float yArrow1 = clamp(4.88 / max(1e-6, uLaneLenM), 0.0, 1.0);\n"
            "  float yArrows = 0.5 * (yArrow0 + yArrow1);\n"
            "  // 7 arrows in one line across the lane (simple upward-pointing triangles).\n"
            "  float ax = mapX;\n"
            "  float triWm = 0.030;\n"
            "  float triHm = 0.020;\n"
            "  float arrowLine = 0.0;\n"
            "  float arrowXs[7];\n"
            "  arrowXs[0] = 0.10; arrowXs[1] = 0.23; arrowXs[2] = 0.36; arrowXs[3] = 0.50;\n"
            "  arrowXs[4] = 0.64; arrowXs[5] = 0.77; arrowXs[6] = 0.90;\n"
            "  for (int i = 0; i < 7; i++) {\n"
            "    float dxA = abs(ax - arrowXs[i]);\n"
            "    // Flock shape: outer arrows are further back (towards foul line).\n"
            "    // Offsets: 1.5ft, 1ft, 0.5ft, 0ft, 0.5ft, 1ft, 1.5ft.\n"
            "    float offM = 0.0;\n"
            "    if (i == 0 || i == 6) offM = 0.4572;\n"
            "    else if (i == 1 || i == 5) offM = 0.3048;\n"
            "    else if (i == 2 || i == 4) offM = 0.1524;\n"
            "    float yI = clamp(yArrows - offM / max(1e-6, uLaneLenM), 0.0, 1.0);\n"
            "    float dyA = (vUV.y - yI);\n"
            "    float tri = step(dxA, triWm) * step(0.0, dyA) * step(dyA, triHm * (1.0 - dxA / max(1e-6, triWm)));\n"
            "    arrowLine = max(arrowLine, tri);\n"
            "  }\n"
            "  marks = max(marks, arrowLine);\n"
	            "\n"
            "  float overlay = clamp(max(border, max(pins, marks)), 0.0, 1.0);\n"
            "  col = mix(col, vec3(1.0), overlay);\n"
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
        float houseLeftStartM,
        float houseLeftEndM,
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
        glUniform1f(glGetUniformLocation(program, "uHouseLeftStartM"), houseLeftStartM);
        glUniform1f(glGetUniformLocation(program, "uHouseLeftEndM"), houseLeftEndM);
        glUniform1f(glGetUniformLocation(program, "uOilThickness"), oilThickness);
        glUniform1f(glGetUniformLocation(program, "uPushback01"), pushback01);

        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
};
