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

struct TrackerDiagramColor
{
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
        return {alg * w, 0.0f, (alg + 1) * w, 1.0f / 16.0f};
    }

    static TrackerDiagramRect selectedAlgoRect(int alg, int selectedOp)
    {
        float w = 1.0f / 8.0f;
        int op = std::max(0, std::min(3, selectedOp));
        float rowH = 1.0f / 16.0f;
        float y0 = rowH * (2 + op);
        return {alg * w, y0, (alg + 1) * w, y0 + rowH};
    }

    static TrackerDiagramRect ssgRect(int ssg)
    {
        float w = 1.0f / 8.0f;
        int idx = std::max(0, std::min(7, ssg));
        return {idx * w, 1.0f / 16.0f, (idx + 1) * w, 2.0f / 16.0f};
    }

    static TrackerDiagramRect envelopeRect(int op)
    {
        int col = op & 1;
        int row = (op >> 1) & 1;
        float x0 = col * 0.5f;
        float y0 = (6.0f + row * 2.0f) / 16.0f;
        return {x0, y0, x0 + 0.5f, y0 + 2.0f / 16.0f};
    }

    static TrackerDiagramRect operatorEnvelopeRect(int op)
    {
        int col = op & 1;
        int row = (op >> 1) & 1;
        float x0 = col * 0.5f;
        float y0 = (10.0f + row * 2.0f) / 16.0f;
        return {x0, y0, x0 + 0.5f, y0 + 2.0f / 16.0f};
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

    static TrackerDiagramColor operatorSpectrumColor(int op, float alpha = 1.0f)
    {
        static constexpr TrackerDiagramColor COLORS[4] = {
            {1.00f, 0.34f, 0.42f, 1.0f},
            {1.00f, 0.78f, 0.24f, 1.0f},
            {0.22f, 0.82f, 1.00f, 1.0f},
            {0.70f, 0.48f, 1.00f, 1.0f},
        };
        TrackerDiagramColor color = COLORS[std::max(0, std::min(3, op))];
        color.a = alpha;
        return color;
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

    void arrowLine(float x0, float y0, float x1, float y1, float width, float r, float g, float b, float a)
    {
        line(x0, y0, x1, y1, width, r, g, b, a);
        float dx = x1 - x0;
        float dy = y1 - y0;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len < 0.001f) return;
        float ux = dx / len;
        float uy = dy / len;
        float nx = -uy;
        float ny = ux;
        float size = width * 3.0f;
        tri(x1, y1,
            x1 - ux * size + nx * size * 0.55f, y1 - uy * size + ny * size * 0.55f,
            x1 - ux * size - nx * size * 0.55f, y1 - uy * size - ny * size * 0.55f,
            r, g, b, a);
    }

    void digitSegment(float x0, float y0, float x1, float y1, float width, float r, float g, float b, float a)
    {
        line(x0, y0, x1, y1, width, r, g, b, a);
        circle(x0, y0, width * 0.5f, r, g, b, a);
        circle(x1, y1, width * 0.5f, r, g, b, a);
    }

    void digitLabel(float cx, float cy, float height, int digit, bool selected)
    {
        const float w = height * 0.52f;
        const float xL = cx - w * 0.5f;
        const float xR = cx + w * 0.5f;
        const float yT = cy - height * 0.5f;
        const float yM = cy;
        const float yB = cy + height * 0.5f;
        const float lineW = std::max(1.4f, height * 0.15f);
        const float r = selected ? 1.00f : 0.08f;
        const float g = selected ? 0.94f : 0.10f;
        const float b = selected ? 0.72f : 0.14f;
        auto seg = [&](char s) {
            switch (s)
            {
            case 'a': digitSegment(xL, yT, xR, yT, lineW, r, g, b, 1.0f); break;
            case 'b': digitSegment(xR, yT, xR, yM, lineW, r, g, b, 1.0f); break;
            case 'c': digitSegment(xR, yM, xR, yB, lineW, r, g, b, 1.0f); break;
            case 'd': digitSegment(xL, yB, xR, yB, lineW, r, g, b, 1.0f); break;
            case 'e': digitSegment(xL, yM, xL, yB, lineW, r, g, b, 1.0f); break;
            case 'f': digitSegment(xL, yT, xL, yM, lineW, r, g, b, 1.0f); break;
            case 'g': digitSegment(xL, yM, xR, yM, lineW, r, g, b, 1.0f); break;
            }
        };
        switch (digit)
        {
        case 1:
        {
            const float xC = cx + w * 0.12f;
            digitSegment(xC - w * 0.30f, yT + height * 0.16f, xC, yT, lineW, r, g, b, 1.0f);
            digitSegment(xC, yT, xC, yB, lineW, r, g, b, 1.0f);
            digitSegment(xC - w * 0.34f, yB, xC + w * 0.34f, yB, lineW, r, g, b, 1.0f);
        } break;
        case 2: seg('a'); seg('b'); seg('g'); seg('e'); seg('d'); break;
        case 3: seg('a'); seg('b'); seg('g'); seg('c'); seg('d'); break;
        default: seg('f'); seg('g'); seg('b'); seg('c'); break;
        }
    }

    void opNode(float cx, float cy, float radius, int op, bool selected = false)
    {
        circle(cx, cy, radius + 1.5f, 0.10f, 0.12f, 0.17f, 1.0f);
        if (selected)
            circle(cx, cy, radius, 0.88f, 0.22f, 0.28f, 1.0f);
        else
            circle(cx, cy, radius, 0.82f, 0.76f, 0.38f, 1.0f);
        digitLabel(cx, cy, radius * 1.12f, op, selected);
    }

    void outputNode(float cx, float cy, float radius)
    {
        circle(cx, cy, radius, 0.42f, 0.90f, 0.48f, 1.0f);
        arrowLine(cx + radius, cy, cx + radius * 2.6f, cy, 2.2f, 0.42f, 0.90f, 0.48f, 1.0f);
    }

    void drawAlgorithm(int alg, float x, float y, float w, float h, int selectedOp = -1)
    {
        float bx = x + w * 0.04f;
        float by = y + h * 0.08f;
        float uw = w * 0.86f;
        float uh = h * 0.84f;
        float r = std::max(5.0f, std::min(w, h) * 0.085f);
        auto mx = [&](float t) { return bx + uw * t; };
        auto my = [&](float t) { return by + uh * t; };
        auto mod = [&](float x0, float y0, float x1, float y1) {
            line(x0, y0, x1, y1, 2.6f, 0.45f, 0.58f, 0.76f, 1.0f);
        };
        auto out = [&](float x0, float y0, float x1, float y1) {
            arrowLine(x0, y0, x1, y1, 2.8f, 0.42f, 0.90f, 0.48f, 1.0f);
        };
        auto node = [&](float px, float py, int op) { opNode(px, py, r, op, selectedOp == (op - 1)); };
        auto output = [&](float px, float py) { outputNode(px, py, r * 0.40f); };

        switch (alg & 7)
        {
        case 0:
        {
            float y0 = my(0.50f);
            float x1 = mx(0.12f), x2 = mx(0.33f), x3 = mx(0.54f), x4 = mx(0.75f), xo = mx(0.93f);
            mod(x1 + r, y0, x2 - r, y0); mod(x2 + r, y0, x3 - r, y0); mod(x3 + r, y0, x4 - r, y0); out(x4 + r, y0, xo, y0);
            node(x1, y0, 1); node(x2, y0, 2); node(x3, y0, 3); node(x4, y0, 4); output(xo, y0);
        } break;
        case 1:
        {
            float x1 = mx(0.16f), x3 = mx(0.42f), x4 = mx(0.66f), xo = mx(0.91f);
            float y1 = my(0.28f), y2 = my(0.72f), ym = my(0.50f);
            mod(x1 + r, y1, x3 - r, ym); mod(x1 + r, y2, x3 - r, ym); mod(x3 + r, ym, x4 - r, ym); out(x4 + r, ym, xo, ym);
            node(x1, y1, 1); node(x1, y2, 2); node(x3, ym, 3); node(x4, ym, 4); output(xo, ym);
        } break;
        case 2:
        {
            float x1 = mx(0.16f), x3 = mx(0.42f), x4 = mx(0.66f), xo = mx(0.91f);
            float y1 = my(0.28f), y2 = my(0.72f), ym = my(0.50f);
            mod(x1 + r, y1, x3, y1); mod(x3, y1, x4 - r, ym); mod(x1 + r, y2, x3 - r, y2); mod(x3 + r, y2, x4 - r, ym); out(x4 + r, ym, xo, ym);
            node(x1, y1, 1); node(x1, y2, 2); node(x3, y2, 3); node(x4, ym, 4); output(xo, ym);
        } break;
        case 3:
        {
            float x1 = mx(0.16f), x2 = mx(0.42f), x4 = mx(0.66f), xo = mx(0.91f);
            float y1 = my(0.28f), y3 = my(0.72f), ym = my(0.50f);
            mod(x1 + r, y1, x2 - r, y1); mod(x2 + r, y1, x4 - r, ym); mod(x1 + r, y3, x2, y3); mod(x2, y3, x4 - r, ym); out(x4 + r, ym, xo, ym);
            node(x1, y1, 1); node(x2, y1, 2); node(x1, y3, 3); node(x4, ym, 4); output(xo, ym);
        } break;
        case 4:
        {
            float x1 = mx(0.18f), x2 = mx(0.48f), xo = mx(0.84f);
            float y1 = my(0.32f), y2 = my(0.68f), ym = my(0.50f);
            mod(x1 + r, y1, x2 - r, y1); out(x2 + r, y1, xo, ym); mod(x1 + r, y2, x2 - r, y2); out(x2 + r, y2, xo, ym);
            node(x1, y1, 1); node(x2, y1, 2); node(x1, y2, 3); node(x2, y2, 4); output(xo, ym);
        } break;
        case 5:
        {
            float x1 = mx(0.18f), x2 = mx(0.50f), xo = mx(0.84f);
            float y1 = my(0.24f), y2 = my(0.50f), y3 = my(0.76f);
            mod(x1 + r, y2, x2 - r, y1); out(x2 + r, y1, xo, y2); mod(x1 + r, y2, x2 - r, y2); out(x2 + r, y2, xo, y2); mod(x1 + r, y2, x2 - r, y3); out(x2 + r, y3, xo, y2);
            node(x1, y2, 1); node(x2, y1, 2); node(x2, y2, 3); node(x2, y3, 4); output(xo, y2);
        } break;
        case 6:
        {
            float x1 = mx(0.18f), x2 = mx(0.50f), xo = mx(0.84f);
            float y1 = my(0.24f), y2 = my(0.50f), y3 = my(0.76f);
            mod(x1 + r, y1, x2 - r, y1); out(x2 + r, y1, xo, y2); out(x2 + r, y2, xo, y2); out(x2 + r, y3, xo, y2);
            node(x1, y1, 1); node(x2, y1, 2); node(x2, y2, 3); node(x2, y3, 4); output(xo, y2);
        } break;
        case 7:
        default:
        {
            float y0 = my(0.36f), yo = my(0.74f), xo = mx(0.50f);
            float x1 = mx(0.14f), x2 = mx(0.38f), x3 = mx(0.62f), x4 = mx(0.86f);
            out(x1, y0 + r, xo, yo); out(x2, y0 + r, xo, yo); out(x3, y0 + r, xo, yo); out(x4, y0 + r, xo, yo);
            node(x1, y0, 1); node(x2, y0, 2); node(x3, y0, 3); node(x4, y0, 4); output(xo, yo);
        } break;
        }
    }

    void drawSsg(int ssg, float x, float y, float w, float h)
    {
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

    void drawEnvelopeCurve(const xfm_patch_opn_operator &o, int op, float x, float y, float w, float h, float width, float alpha)
    {
        float x0 = x + 18, yTop = y + 24, yBot = y + h - 24;
        float usable = w - 36;
        float sl = 1.0f - o.SL / 15.0f;
        float sr = o.SR / 31.0f;
        float ySL = yTop + (yBot - yTop) * (1.0f - sl);
        auto rateW = [](int rate, int maxRate, float base) {
            float r = rate / (float)maxRate;
            return base * (1.0f - r * r * r * 0.92f);
        };
        auto decayRateW = [](int rate, int maxRate, float base) {
            float r = std::max(0.0f, std::min(1.0f, rate / (float)maxRate));
            return base * (1.0f - std::pow(r, 0.45f) * 0.92f);
        };
        float wAtk = rateW(o.AR, 31, usable * 0.20f);
        float wDec = decayRateW(o.DR, 31, usable * 0.28f);
        float wSus = usable * 0.36f * (1.0f - sr) + 8.0f * sr;
        float wRel = rateW(o.RR, 15, usable * 0.24f);
        float cx = x0;
        float pts[10] = {cx, yBot, cx + wAtk, yTop, cx + wAtk + wDec, ySL,
                         cx + wAtk + wDec + wSus, ySL, cx + wAtk + wDec + wSus + wRel, yBot};
        TrackerDiagramColor color = operatorSpectrumColor(op, alpha);
        polyline(pts, 5, width, color.r, color.g, color.b, color.a);
    }

    void drawEnvelopeSlot(const xfm_patch_opn &patch, int selectedOp, float x, float y, float w, float h, bool showOtherOperators)
    {
        const xfm_patch_opn_operator &selected = patch.op[selectedOp];
        float x0 = x + 18, yTop = y + 24, yBot = y + h - 24;
        float sl = 1.0f - selected.SL / 15.0f;
        float ySL = yTop + (yBot - yTop) * (1.0f - sl);
        line(x0, ySL, x + w - 18, ySL, 1.5f, 0.35f, 0.36f, 0.43f, 0.75f);

        if (showOtherOperators)
        {
            for (int op = 0; op < 4; op++)
            {
                if (op == selectedOp) continue;
                drawEnvelopeCurve(patch.op[op], op, x, y, w, h, 6.0f, 0.48f);
            }
        }
        drawEnvelopeCurve(selected, selectedOp, x, y, w, h, 8.0f, 1.0f);
    }

    void render(RenderTexture &target, const xfm_patch_opn &patch, int screenW, int screenH, int pixelRatio)
    {
        init();
        atlasW = target.width;
        atlasH = target.height;
        verts.clear();
        target.bindForWriting();
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        float rowH = atlasH / 16.0f;
        for (int i = 0; i < 8; i++)
            drawAlgorithm(i, i * (atlasW / 8.0f), 0, atlasW / 8.0f, rowH);
        for (int i = 0; i < 8; i++)
            drawSsg(i + 1, i * (atlasW / 8.0f), rowH, atlasW / 8.0f, rowH);
        for (int op = 0; op < 4; op++)
        {
            for (int alg = 0; alg < 8; alg++)
                drawAlgorithm(alg, alg * (atlasW / 8.0f), rowH * (2 + op), atlasW / 8.0f, rowH, op);
        }
        for (int op = 0; op < 4; op++)
        {
            int col = op & 1, row = op >> 1;
            drawEnvelopeSlot(patch, op, col * atlasW * 0.5f, rowH * (6 + row * 2),
                             atlasW * 0.5f, rowH * 2, false);
            drawEnvelopeSlot(patch, op, col * atlasW * 0.5f, rowH * (10 + row * 2),
                             atlasW * 0.5f, rowH * 2, true);
        }
        glUseProgram(program);
        glUniform2f(glGetUniformLocation(program, "uSize"), (float)atlasW, (float)atlasH);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(TrackerDiagramVertex), verts.data(), GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)verts.size());
        glBindVertexArray(0);
        target.unbind(screenW * pixelRatio, screenH * pixelRatio);
    }
};
