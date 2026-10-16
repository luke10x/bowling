#pragma once

#include <array>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <vector>

// CPU wall time for completed frames. Each mark closes the preceding zone.
// The graph is intentionally one frame behind so its last zone includes SwapWindow.
struct FrameProfileGraph
{
    enum Zone
    {
        Events,
        State,
        Controls,
        RollAndScore,
        Physics,
        Updates,
        Offscreen,
        WorldBackdrop,
        WorldPins,
        WorldBotAvatar,
        WorldLane,
        WorldBall,
        FxTransparent,
        FxCollectableLogic,
        FxCollectables,
        FxSnow,
        FxBallTrail,
        FxAmbientParticles,
        FxFlyingCollectables,
        FxDecals,
        FxOverlay,
        UiPrep,
        UiLayout,
        UiDraw,
        ProfileGraphDraw,
        Present,
        Count
    };

    static constexpr int History = 100;
    static constexpr float ScaleMs = 100.0f;
    // One row owns each zone's label, prior estimate, and default color. Colors
    // are independent of render-stage group; changing a zone takes one edit.
    struct ZoneInfo
    {
        const char *key;
        const char *name;
        float expectedPercent; // -1 means no comparable prior measurement.
        uint32_t rgb;
    };
    static constexpr std::array<ZoneInfo, Count> Zones = {{
        {"EVT",   "Events/setup",         0.2f, 0x9A7DE3},
        {"STATE", "State/story",          0.7f, 0x35B7C6},
        {"CTRL",  "Aim/swing controls",   0.1f, 0xE97F36},
        {"SCORE", "Throw/scoring",        0.0f, 0xD9528A},
        {"PHYS",  "Physics step",         0.2f, 0xDC4242},
        {"UPD",   "Post-physics updates", 0.1f, 0x65B95B},
        {"FBO",   "UI previews/decals",   6.6f, 0x3F83D8},
        {"BG",    "Backdrop/biome",       8.4f, 0xC485DD},
        {"PINS",  "Pin meshes/effects",  -1.0f, 0xE3BE39},
        {"BOT",   "Bot avatar",          -1.0f, 0x5766D8},
        {"LANE",  "Lane and props",       6.7f, 0x38B87B},
        {"BALL",  "Ball and chest",       9.1f, 0xE3655B},
        {"TRAN",  "Transparent world",   -1.0f, 0x32A7A1},
        {"CLOG",  "Collectable logic",   -1.0f, 0xC56A3C},
        {"COIN",  "World collectables",   6.1f, 0x96B938},
        {"SNOW",  "Snow",                -1.0f, 0x68A9E0},
        {"TRAIL", "Ball trails/rings",   -1.0f, 0xD960B7},
        {"AMBI",  "Ambient particles",   -1.0f, 0xDB8F31},
        {"FLY",   "Flying rewards",      -1.0f, 0x784CB6},
        {"DECAL", "Decals",              -1.0f, 0xD64E62},
        {"HUD",   "HUD/effects",          0.1f, 0x49B5B0},
        {"PREP",  "UI preparation",      0.1f, 0xA9764B},
        {"LAY",   "UI layout",           0.7f, 0x6BA8CF},
        {"DRAW",  "UI draw",             7.2f, 0xE2A15A},
        {"GRAPH", "Profile graph",      -1.0f, 0xA6A6B0},
        {"SWAP",  "Present/swap",        -1.0f, 0xE5D569}
    }};

    struct Vertex { float x, y, r, g, b, a; };
    std::array<std::array<float, Count>, History> frames{};
    std::array<float, Count> current{};
    std::array<uint32_t, Count> zoneColors{};
    std::vector<Vertex> vertices;
    int nextFrame = 0;
    int recordedFrames = 0;
    int nextZone = 0;
    Uint64 previousTick = 0;
    Uint64 lastFrameEndTick = 0;
    Uint64 reportStartedAt = 0;
    int reportFrames = 0;
    int reportExtraParticlesOnFrames = 0;
    int reportGapSamples = 0;
    double reportGapMs = 0.0;
    bool extraParticlesEnabled = true;
    std::array<double, Count> reportTotalsMs{};
    double ticksPerMs = 1.0;
    GLuint program = 0, vao = 0, vbo = 0;

    FrameProfileGraph()
    {
        for (int zone = 0; zone < Count; ++zone)
            zoneColors[zone] = Zones[zone].rgb;
    }

    void setZoneColor(Zone zone, uint32_t rgb)
    {
        if (zone >= 0 && zone < Count) zoneColors[zone] = rgb & 0xFFFFFF;
    }

    static std::array<float, 3> colorComponents(uint32_t rgb)
    {
        return {{((rgb >> 16) & 0xFF) / 255.0f,
                 ((rgb >> 8) & 0xFF) / 255.0f,
                 (rgb & 0xFF) / 255.0f}};
    }

    static std::array<int, Count> rankedZones(const std::array<double, Count> &totals)
    {
        std::array<int, Count> order{};
        for (int zone = 0; zone < Count; ++zone) order[zone] = zone;
        std::sort(order.begin(), order.end(), [&](int a, int b) {
            return totals[a] == totals[b] ? a < b : totals[a] > totals[b];
        });
        return order;
    }

    void init()
    {
        ticksPerMs = (double)SDL_GetPerformanceFrequency() / 1000.0;
        static const char *vertexShader = GLSL_VERSION R"(
            layout(location = 0) in vec2 position;
            layout(location = 1) in vec4 color;
            out vec4 vColor;
            void main() { vColor = color; gl_Position = vec4(position, 0.0, 1.0); }
        )";
        static const char *fragmentShader = GLSL_VERSION R"(
            precision mediump float;
            in vec4 vColor;
            out vec4 fragColor;
            void main() { fragColor = vColor; }
        )";
        program = vtx::createShaderProgram(vertexShader, fragmentShader);
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        vertices.reserve(History * Count * 6 + 6000);
    }

    void beginFrame()
    {
        const Uint64 now = SDL_GetPerformanceCounter();
        if (lastFrameEndTick != 0)
        {
            reportGapMs += (double)(now - lastFrameEndTick) / ticksPerMs;
            ++reportGapSamples;
        }
        current.fill(0.0f);
        nextZone = 0;
        previousTick = now;
        if (reportStartedAt == 0) reportStartedAt = previousTick;
    }

    void mark(Zone zone)
    {
        // Marks must be made once, in enum order. A skipped zone is recorded as zero.
        if (zone < nextZone || zone >= Count) return;
        const Uint64 now = SDL_GetPerformanceCounter();
        current[zone] = (float)((double)(now - previousTick) / ticksPerMs);
        previousTick = now;
        nextZone = zone + 1;
    }

    void endFrame()
    {
        mark(Present);
        frames[nextFrame] = current;
        nextFrame = (nextFrame + 1) % History;
        if (recordedFrames < History) ++recordedFrames;
        for (int zone = 0; zone < Count; ++zone)
            reportTotalsMs[zone] += current[zone];
        ++reportFrames;
        if (extraParticlesEnabled) ++reportExtraParticlesOnFrames;
        const Uint64 now = SDL_GetPerformanceCounter();
        const double elapsedMs = (double)(now - reportStartedAt) / ticksPerMs;
        if (elapsedMs >= 5000.0)
        {
            printReport(elapsedMs);
            reportTotalsMs.fill(0.0);
            reportFrames = 0;
            reportExtraParticlesOnFrames = 0;
            reportGapSamples = 0;
            reportGapMs = 0.0;
            reportStartedAt = SDL_GetPerformanceCounter();
        }
        lastFrameEndTick = SDL_GetPerformanceCounter();
    }

    void printReport(double elapsedMs) const
    {
        double totalMs = 0.0;
        for (double zoneMs : reportTotalsMs) totalMs += zoneMs;
        std::fprintf(stderr, "\nFrame profile: %.1fs, %d frames, %.1f FPS\n",
                     elapsedMs / 1000.0, reportFrames,
                     reportFrames * 1000.0 / elapsedMs);
        std::fprintf(stderr, "Extra particles: on %d/%d frames; between-loop gap: %.3f ms/frame (%d samples)\n",
                     reportExtraParticlesOnFrames, reportFrames,
                     reportGapSamples > 0 ? reportGapMs / reportGapSamples : 0.0,
                     reportGapSamples);
        std::fprintf(stderr, "%4s | %-5s | %-22s | %-9s | %8s | %8s | %8s\n",
                     "Rank", "Key", "Zone", "Color", "Expected", "Measured", "Avg ms");
        std::fprintf(stderr, "-----+-------+------------------------+-----------+----------+----------+---------\n");
        const auto order = rankedZones(reportTotalsMs);
        for (int rank = 0; rank < Count; ++rank)
        {
            const int zone = order[rank];
            const double measured = totalMs > 0.0 ? 100.0 * reportTotalsMs[zone] / totalMs : 0.0;
            const double averageMs = reportFrames > 0 ? reportTotalsMs[zone] / reportFrames : 0.0;
            char expected[12] = "--";
            if (Zones[zone].expectedPercent >= 0)
                std::snprintf(expected, sizeof(expected), "%.1f%%", Zones[zone].expectedPercent);
            char color[12];
            std::snprintf(color, sizeof(color), "#%06X", zoneColors[zone]);
            std::fprintf(stderr, "%4.2d | %-5s | %-22s | %-9s | %8s | %7.1f%% | %8.3f\n",
                         rank + 1, Zones[zone].key, Zones[zone].name, color,
                         expected, measured, averageMs);
        }
        std::fprintf(stderr, "     | %-5s | %-22s | %-9s | %7d%% | %7.1f%% | %8.3f\n",
                     "TOTAL", "Measured loop work", "", 100, totalMs > 0.0 ? 100.0 : 0.0,
                     reportFrames > 0 ? totalMs / reportFrames : 0.0);
        std::fflush(stderr);
    }

    void quad(float x0, float y0, float x1, float y1,
              float r, float g, float b, float a)
    {
        const Vertex tl{x0, y1, r, g, b, a}, tr{x1, y1, r, g, b, a};
        const Vertex bl{x0, y0, r, g, b, a}, br{x1, y0, r, g, b, a};
        vertices.insert(vertices.end(), {bl, br, tr, bl, tr, tl});
    }

    static std::array<uint8_t, 7> glyph(char c)
    {
        switch (c)
        {
        case '0': return {{14,17,19,21,25,17,14}};
        case '1': return {{4,12,4,4,4,4,14}};
        case '2': return {{14,17,1,2,4,8,31}};
        case '3': return {{30,1,1,14,1,1,30}};
        case '4': return {{2,6,10,18,31,2,2}};
        case '5': return {{31,16,16,30,1,1,30}};
        case '6': return {{14,16,16,30,17,17,14}};
        case '7': return {{31,1,2,4,8,8,8}};
        case '8': return {{14,17,17,14,17,17,14}};
        case '9': return {{14,17,17,15,1,1,14}};
        case 'A': return {{14,17,17,31,17,17,17}};
        case 'B': return {{30,17,17,30,17,17,30}};
        case 'C': return {{15,16,16,16,16,16,15}};
        case 'D': return {{30,17,17,17,17,17,30}};
        case 'E': return {{31,16,16,30,16,16,31}};
        case 'F': return {{31,16,16,30,16,16,16}};
        case 'G': return {{15,16,16,23,17,17,15}};
        case 'H': return {{17,17,17,31,17,17,17}};
        case 'I': return {{31,4,4,4,4,4,31}};
        case 'L': return {{16,16,16,16,16,16,31}};
        case 'M': return {{17,27,21,21,17,17,17}};
        case 'N': return {{17,25,21,19,17,17,17}};
        case 'O': return {{14,17,17,17,17,17,14}};
        case 'P': return {{30,17,17,30,16,16,16}};
        case 'R': return {{30,17,17,30,20,18,17}};
        case 'S': return {{15,16,16,14,1,1,30}};
        case 'T': return {{31,4,4,4,4,4,4}};
        case 'U': return {{17,17,17,17,17,17,14}};
        case 'V': return {{17,17,17,17,17,10,4}};
        case 'W': return {{17,17,17,21,21,21,10}};
        case 'X': return {{17,17,10,4,10,17,17}};
        case 'Y': return {{17,17,10,4,4,4,4}};
        default: return {};
        }
    }

    void label(float xPx, float yPx, float scale, const char *text,
               float width, float height)
    {
        for (const char *p = text; *p; ++p, xPx += 6.0f * scale)
        {
            const auto rows = glyph(*p);
            for (int row = 0; row < 7; ++row)
                for (int col = 0; col < 5; ++col)
                    if (rows[row] & (1 << (4 - col)))
                    {
                        const float x = xPx + col * scale;
                        const float y = yPx + (6 - row) * scale;
                        quad(-1.0f + 2.0f * x / width,
                             -1.0f + 2.0f * y / height,
                             -1.0f + 2.0f * (x + scale) / width,
                             -1.0f + 2.0f * (y + scale) / height,
                             1.0f, 1.0f, 1.0f, 0.96f);
                    }
        }
    }

    void draw(int drawableWidth, int drawableHeight, float footerClearancePx)
    {
        if (!program || recordedFrames == 0 || drawableWidth <= 0 || drawableHeight <= 0) return;
        vertices.clear();
        const float width = (float)drawableWidth;
        const float height = (float)drawableHeight;
        const bool compact = width < 480.0f;
        const float graphWidth = std::min(width * (compact ? 0.90f : 0.70f), 700.0f);
        const int legendColumns = graphWidth >= 490.0f ? 7 : graphWidth >= 350.0f ? 5 : 4;
        const int legendRows = (Count + legendColumns - 1) / legendColumns;
        const float legendHeight = legendRows * 15.0f + 12.0f;
        const float bottomPx = std::max(0.0f, footerClearancePx) + 6.0f;
        const float graphHeight = std::min(
            std::min(height * (compact ? 0.43f : 0.34f), compact ? 240.0f : 220.0f),
            height - bottomPx - 6.0f);
        if (graphHeight <= legendHeight + 20.0f) return;
        const float x0 = -graphWidth / width;
        const float x1 = graphWidth / width;
        const float y0 = -1.0f + 2.0f * bottomPx / height;
        const float y1 = y0 + 2.0f * (graphHeight - legendHeight) / height;
        quad(x0, y0, x1, y0 + 2.0f * graphHeight / height,
             0.02f, 0.025f, 0.04f, 0.80f);
        for (float ms : {16.67f, 33.33f, 66.67f})
        {
            const float y = y0 + (y1 - y0) * ms / ScaleMs;
            quad(x0, y, x1, y + 2.0f / height, 0.8f, 0.8f, 0.8f, 0.30f);
        }
        std::array<double, Count> recentTotalsMs{};
        for (int col = 0; col < recordedFrames; ++col)
        {
            const int index = (nextFrame - recordedFrames + col + History) % History;
            const float left = x0 + (x1 - x0) * col / History;
            const float right = x0 + (x1 - x0) * (col + 1) / History;
            float total = 0.0f;
            for (int zone = 0; zone < Count; ++zone)
            {
                const float from = std::min(total, ScaleMs);
                const float zoneMs = frames[index][zone];
                recentTotalsMs[zone] += zoneMs;
                total += zoneMs;
                const float to = std::min(total, ScaleMs);
                if (to <= from) continue;
                const auto c = colorComponents(zoneColors[zone]);
                quad(left, y0 + (y1 - y0) * from / ScaleMs,
                     right, y0 + (y1 - y0) * to / ScaleMs,
                     c[0], c[1], c[2], 0.95f);
            }
        }
        const float slotWidth = graphWidth / legendColumns;
        const float scale = slotWidth >= 90.0f ? 1.5f :
            std::min(1.0f, (slotWidth - 12.0f) / 48.0f);
        const float graphLeftPx = (width - graphWidth) * 0.5f;
        const float legendBottomPx = bottomPx + graphHeight - legendHeight;
        const auto order = rankedZones(recentTotalsMs);
        for (int rank = 0; rank < Count; ++rank)
        {
            const int zone = order[rank];
            const int column = rank % legendColumns;
            const int row = rank / legendColumns;
            const float x = graphLeftPx + graphWidth * column / legendColumns + 4.0f;
            const float y = legendBottomPx + 8.0f + (legendRows - 1 - row) * 15.0f;
            const auto c = colorComponents(zoneColors[zone]);
            quad(-1.0f + 2.0f * x / width, -1.0f + 2.0f * y / height,
                 -1.0f + 2.0f * (x + 5.0f) / width,
                 -1.0f + 2.0f * (y + 7.0f * scale) / height,
                 c[0], c[1], c[2], 1.0f);
            char numberedKey[16];
            std::snprintf(numberedKey, sizeof(numberedKey), "%02d %s", rank + 1, Zones[zone].key);
            label(x + 8.0f, y, scale, numberedKey, width, height);
        }

        GLint oldProgram = 0, oldVao = 0, oldBuffer = 0;
        GLint oldBlendSrc = 0, oldBlendDst = 0;
        GLint oldBlendSrcAlpha = 0, oldBlendDstAlpha = 0;
        GLint oldViewport[4] = {};
        GLboolean oldDepthMask = GL_TRUE;
        const GLboolean oldDepth = glIsEnabled(GL_DEPTH_TEST);
        const GLboolean oldScissor = glIsEnabled(GL_SCISSOR_TEST);
        const GLboolean oldCull = glIsEnabled(GL_CULL_FACE);
        const GLboolean oldBlend = glIsEnabled(GL_BLEND);
        glGetIntegerv(GL_CURRENT_PROGRAM, &oldProgram);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &oldVao);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &oldBuffer);
        glGetIntegerv(GL_BLEND_SRC_RGB, &oldBlendSrc);
        glGetIntegerv(GL_BLEND_DST_RGB, &oldBlendDst);
        glGetIntegerv(GL_BLEND_SRC_ALPHA, &oldBlendSrcAlpha);
        glGetIntegerv(GL_BLEND_DST_ALPHA, &oldBlendDstAlpha);
        glGetIntegerv(GL_VIEWPORT, oldViewport);
        glGetBooleanv(GL_DEPTH_WRITEMASK, &oldDepthMask);
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glDisable(GL_SCISSOR_TEST);
        glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glViewport(0, 0, drawableWidth, drawableHeight);
        glUseProgram(program);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STREAM_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)(2 * sizeof(float)));
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vertices.size());
        glBindVertexArray(oldVao);
        glBindBuffer(GL_ARRAY_BUFFER, oldBuffer);
        glUseProgram(oldProgram);
        glBlendFuncSeparate(oldBlendSrc, oldBlendDst, oldBlendSrcAlpha, oldBlendDstAlpha);
        glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);
        if (!oldBlend) glDisable(GL_BLEND);
        if (oldCull) glEnable(GL_CULL_FACE);
        if (oldScissor) glEnable(GL_SCISSOR_TEST);
        if (oldDepth) glEnable(GL_DEPTH_TEST);
        glDepthMask(oldDepthMask);
    }
};
