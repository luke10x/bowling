#pragma once

#include "../framework/gl_header.h"
#include "../rendertexture.h"
#include "../sounds/songs_data.h"
#include <algorithm>
#include <cmath>
#include <vector>

struct TrackerDiagramRect
{
    float u0, v0, u1, v1;
};

struct TrackerDiagramVertex
{
    float x, y;
    float r, g, b, a;
};

struct TrackerDiagramRenderer
{
    GLuint program = 0;
    GLuint vao = 0;
    GLuint vbo = 0;
    std::vector<TrackerDiagramVertex> verts;
    int atlasW = 1024;
    int atlasH = 1024;

    void init()
    {
        if (program) return;
        const char *vs =
            GLSL_VERSION "\n"
            "precision highp float;\n"
            "layout(location=0) in vec2 aPos;\n"
            "layout(location=1) in vec4 aColor;\n"
            "uniform vec2 uSize;\n"
            "out vec4 vColor;\n"
            "void main(){\n"
            "  vec2 p = vec2(aPos.x / uSize.x * 2.0 - 1.0, aPos.y / uSize.y * 2.0 - 1.0);\n"
            "  gl_Position = vec4(p, 0.0, 1.0);\n"
            "  vColor = aColor;\n"
            "}\n";
        const char *fs =
            GLSL_VERSION "\n"
            "precision highp float;\n"
            "in vec4 vColor;\n"
            "out vec4 frag;\n"
            "void main(){ frag = vColor; }\n";
        auto compile = [](GLenum type, const char *src) {
            GLuint s = glCreateShader(type);
            glShaderSource(s, 1, &src, nullptr);
            glCompileShader(s);
            GLint ok = 0;
            glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
            if (!ok)
            {
                char log[1024];
                glGetShaderInfoLog(s, sizeof(log), nullptr, log);
                printf("Tracker diagram shader compile error: %s\n", log);
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
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(TrackerDiagramVertex), (void *)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(TrackerDiagramVertex), (void *)(2 * sizeof(float)));
        glBindVertexArray(0);
    }

    static TrackerDiagramRect algoRect(int alg)
    {
        float w = 1.0f / 8.0f;
        return {alg * w, 0.0f, (alg + 1) * w, 0.125f};
    }

    static TrackerDiagramRect ssgRect(int ssg)
    {
        float w = 1.0f / 8.0f;
        int idx = std::max(0, std::min(7, ssg));
        return {idx * w, 0.125f, (idx + 1) * w, 0.25f};
    }

    static TrackerDiagramRect envelopeRect(int op)
    {
        int col = op & 1;
        int row = (op >> 1) & 1;
        float x0 = col * 0.5f;
        float y0 = 0.25f + row * 0.375f;
        return {x0, y0, x0 + 0.5f, y0 + 0.375f};
    }

    void rectBg(float x, float y, float w, float h, float r, float g, float b, float a)
    {
        tri(x, y, x + w, y, x + w, y + h, r, g, b, a);
        tri(x, y, x + w, y + h, x, y + h, r, g, b, a);
    }

    void tri(float x0, float y0, float x1, float y1, float x2, float y2, float r, float g, float b, float a)
    {
        verts.push_back({x0, y0, r, g, b, a});
        verts.push_back({x1, y1, r, g, b, a});
        verts.push_back({x2, y2, r, g, b, a});
    }

    void line(float x0, float y0, float x1, float y1, float width, float r, float g, float b, float a)
    {
        float dx = x1 - x0;
        float dy = y1 - y0;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len < 0.001f) return;
        float nx = -dy / len * width * 0.5f;
        float ny = dx / len * width * 0.5f;
        tri(x0 + nx, y0 + ny, x1 + nx, y1 + ny, x1 - nx, y1 - ny, r, g, b, a);
        tri(x0 + nx, y0 + ny, x1 - nx, y1 - ny, x0 - nx, y0 - ny, r, g, b, a);
    }

    void polyline(const float *xy, int count, float width, float r, float g, float b, float a)
    {
        for (int i = 0; i + 1 < count; i++)
            line(xy[i * 2], xy[i * 2 + 1], xy[(i + 1) * 2], xy[(i + 1) * 2 + 1], width, r, g, b, a);
    }

    void circle(float cx, float cy, float radius, float r, float g, float b, float a)
    {
        const int steps = 18;
        for (int i = 0; i < steps; i++)
        {
            float a0 = (float)i / steps * 6.2831853f;
            float a1 = (float)(i + 1) / steps * 6.2831853f;
            tri(cx, cy, cx + std::cos(a0) * radius, cy + std::sin(a0) * radius,
                cx + std::cos(a1) * radius, cy + std::sin(a1) * radius, r, g, b, a);
        }
    }

    void drawFrame(float x, float y, float w, float h)
    {
        rectBg(x, y, w, h, 0.06f, 0.07f, 0.10f, 1.0f);
        line(x + 1, y + 1, x + w - 1, y + 1, 2, 0.22f, 0.26f, 0.34f, 1);
        line(x + w - 1, y + 1, x + w - 1, y + h - 1, 2, 0.22f, 0.26f, 0.34f, 1);
        line(x + w - 1, y + h - 1, x + 1, y + h - 1, 2, 0.22f, 0.26f, 0.34f, 1);
        line(x + 1, y + h - 1, x + 1, y + 1, 2, 0.22f, 0.26f, 0.34f, 1);
    }

    void drawAlgorithm(int alg, float x, float y, float w, float h)
    {
        drawFrame(x, y, w, h);
        float px[4] = {x + w * 0.24f, x + w * 0.24f, x + w * 0.56f, x + w * 0.56f};
        float py[4] = {y + h * 0.76f, y + h * 0.52f, y + h * 0.52f, y + h * 0.28f};
        auto edge = [&](int a, int b) { line(px[a], py[a], px[b], py[b], 3, 0.45f, 0.62f, 0.82f, 1); };
        switch (alg & 7)
        {
        case 0: edge(0, 1); edge(1, 2); edge(2, 3); break;
        case 1: edge(0, 2); edge(1, 2); edge(2, 3); break;
        case 2: edge(0, 1); edge(0, 2); edge(2, 3); break;
        case 3: edge(0, 3); edge(1, 3); edge(2, 3); break;
        case 4: edge(0, 1); edge(2, 3); break;
        case 5: edge(0, 3); edge(1, 3); edge(2, 3); break;
        case 6: edge(0, 2); edge(1, 2); break;
        default: break;
        }
        for (int i = 0; i < 4; i++)
            circle(px[i], py[i], 11, 0.83f, 0.78f, 0.42f, 1);
        line(x + w * 0.76f, y + h * 0.50f, x + w * 0.92f, y + h * 0.50f, 4, 0.68f, 0.90f, 0.58f, 1);
    }

    void drawSsg(int ssg, float x, float y, float w, float h)
    {
        drawFrame(x, y, w, h);
        float x1 = x + w * 0.10f, yTop = y + h * 0.24f, yBot = y + h * 0.76f, sw = w * 0.20f;
        float pts[18] = {};
        int n = 0;
        auto P = [&](float px, float py) { pts[n * 2] = px; pts[n * 2 + 1] = py; n++; };
        switch (ssg)
        {
        case 1: P(x1,yBot);P(x1,yTop);P(x1+sw,yBot);P(x1+sw,yTop);P(x1+sw*2,yBot);P(x1+sw*2,yTop);P(x1+sw*3,yBot);P(x1+sw*3,yTop);P(x1+sw*4,yBot); break;
        case 2: P(x1,yBot);P(x1,yTop);P(x1+sw,yBot);P(x1+sw*4,yBot); break;
        case 3: P(x1,yBot);P(x1,yTop);P(x1+sw,yBot);P(x1+sw*2,yTop);P(x1+sw*3,yBot);P(x1+sw*4,yTop); break;
        case 4: P(x1,yBot);P(x1,yTop);P(x1+sw,yBot);P(x1+sw,yTop);P(x1+sw*4,yTop); break;
        case 5: P(x1,yBot);P(x1+sw,yTop);P(x1+sw,yBot);P(x1+sw*2,yTop);P(x1+sw*2,yBot);P(x1+sw*3,yTop);P(x1+sw*3,yBot);P(x1+sw*4,yTop); break;
        case 6: P(x1,yBot);P(x1+sw,yTop);P(x1+sw*4,yTop); break;
        case 7: P(x1,yBot);P(x1+sw,yTop);P(x1+sw*2,yBot);P(x1+sw*3,yTop);P(x1+sw*4,yBot); break;
        default: P(x1,yBot);P(x1+sw,yTop);P(x1+sw,yBot);P(x1+sw*4,yBot); break;
        }
        polyline(pts, n, 3, 0.85f, 0.58f, 0.78f, 1);
    }

    void drawEnvelope(const xfm_patch_opn_operator &o, int op, float x, float y, float w, float h)
    {
        drawFrame(x, y, w, h);
        float x0 = x + 18, yTop = y + 24, yBot = y + h - 24;
        float usable = w - 36;
        float sl = 1.0f - o.SL / 15.0f;
        float sr = o.SR / 31.0f;
        float ySL = yTop + (yBot - yTop) * (1.0f - sl);
        auto rateW = [](int rate, int maxRate, float base) {
            float r = rate / (float)maxRate;
            return base * (1.0f - r * r * r * 0.92f);
        };
        float wAtk = rateW(o.AR, 31, usable * 0.20f);
        float wDec = rateW(o.DR, 31, usable * 0.20f);
        float wSus = usable * 0.36f * (1.0f - sr) + 8.0f * sr;
        float wRel = rateW(o.RR, 15, usable * 0.24f);
        float cx = x0;
        float pts[10] = {cx, yBot, cx + wAtk, yTop, cx + wAtk + wDec, ySL,
                         cx + wAtk + wDec + wSus, ySL, cx + wAtk + wDec + wSus + wRel, yBot};
        line(x0, ySL, x + w - 18, ySL, 1.5f, 0.35f, 0.36f, 0.43f, 1);
        polyline(pts, 5, 4, 0.35f + op * 0.12f, 0.78f, 0.56f, 1);
    }

    void render(RenderTexture &target, const xfm_patch_opn &patch, int screenW, int screenH)
    {
        init();
        atlasW = target.width;
        atlasH = target.height;
        verts.clear();
        target.bindForWriting();
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glClearColor(0.035f, 0.04f, 0.06f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        for (int i = 0; i < 8; i++)
            drawAlgorithm(i, i * (atlasW / 8.0f), 0, atlasW / 8.0f, atlasH * 0.125f);
        for (int i = 0; i < 8; i++)
            drawSsg(i + 1, i * (atlasW / 8.0f), atlasH * 0.125f, atlasW / 8.0f, atlasH * 0.125f);
        for (int op = 0; op < 4; op++)
        {
            int col = op & 1, row = op >> 1;
            drawEnvelope(patch.op[op], op, col * atlasW * 0.5f, atlasH * 0.25f + row * atlasH * 0.375f,
                         atlasW * 0.5f, atlasH * 0.375f);
        }
        glUseProgram(program);
        glUniform2f(glGetUniformLocation(program, "uSize"), (float)atlasW, (float)atlasH);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(TrackerDiagramVertex), verts.data(), GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)verts.size());
        glBindVertexArray(0);
        target.unbind(screenW, screenH);
    }
};
