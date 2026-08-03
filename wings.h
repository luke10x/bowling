#pragma once

#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "animation/anim_player.h"
#include "framework/gl_header.h"
#include "framework/gl_util.h"

enum WingsAvatarSlot
{
    WINGS_AVATAR_ANGEL = 0,
    WINGS_AVATAR_CHERUB = 1,
    WINGS_AVATAR_SERAPH = 2,
    WINGS_AVATAR_THRONE = 3,
    WINGS_AVATAR_COUNT = 4,
};

struct WingRenderVertex
{
    glm::vec3 position;
    glm::vec2 uv;
    glm::vec4 color;
};

struct WingsState
{
    static constexpr int kLayerCount = 3;
    static constexpr int kVerticesPerLayer = 52;
    static constexpr int kIndicesPerLayer = 84;
    static constexpr int kVertexCount = kVerticesPerLayer * kLayerCount;
    static constexpr int kIndexCount = kIndicesPerLayer * kLayerCount;
    static constexpr int kSmoothedPoints = 8;

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLuint shaderId = 0;
    bool initialized = false;

    std::array<WingRenderVertex, kVertexCount> vertices = {};
    std::array<uint32_t, kIndexCount> indices = {};
    std::array<glm::vec3, kSmoothedPoints> smoothedEdgePoints = {};
    bool smoothingValid = false;
    int lastAvatarSlot = -1;
    int backBoneByAvatar[WINGS_AVATAR_COUNT] = {-2, -2, -2, -2};
    float time = 0.0f;

    void initWings();
};

inline int Wings_FindBackBoneIndex(const AssmanAnimPlayer &anim)
{
    if (!anim.anim.header)
        return -1;

    int best = -1;
    int bestScore = -100000;
    const uint32_t n = anim.anim.header->boneCount;
    for (uint32_t i = 0; i < n; ++i)
    {
        const char *raw = animNameCStr(anim.anim.bones[i].name);
        if (!raw)
            continue;

        std::string s(raw);
        for (char &c : s)
            c = (char)std::tolower((unsigned char)c);

        int score = -10;
        if (s.find("spine") != std::string::npos) score = 40;
        if (s.find("spine1") != std::string::npos || s.find("spine.001") != std::string::npos) score = 55;
        if (s.find("spine2") != std::string::npos || s.find("spine.002") != std::string::npos) score = 70;
        if (s.find("chest") != std::string::npos) score = 85;
        if (s.find("upperchest") != std::string::npos) score = 95;
        if (s.find("torso") != std::string::npos) score = 60;
        if (s.find("neck") != std::string::npos) score = 30;
        if (s.find("head") != std::string::npos) score -= 40;
        if (s.find("arm") != std::string::npos || s.find("hand") != std::string::npos) score -= 80;
        if (s.find("leg") != std::string::npos || s.find("foot") != std::string::npos) score -= 80;

        if (score > bestScore)
        {
            bestScore = score;
            best = (int)i;
        }
    }

    return bestScore > 0 ? best : -1;
}

inline int Wings_BackBoneForAvatar(WingsState *wings, const AssmanAnimPlayer *anim, int avatarSlot)
{
    if (!wings || !anim || avatarSlot < 0 || avatarSlot >= WINGS_AVATAR_COUNT)
        return -1;
    if (wings->backBoneByAvatar[avatarSlot] == -2)
        wings->backBoneByAvatar[avatarSlot] = Wings_FindBackBoneIndex(*anim);
    return wings->backBoneByAvatar[avatarSlot];
}

inline void WingsState::initWings()
{
    if (this->vao != 0)
    {
        glDeleteVertexArrays(1, &this->vao);
        this->vao = 0;
    }
    if (this->vbo != 0)
    {
        glDeleteBuffers(1, &this->vbo);
        this->vbo = 0;
    }
    if (this->ebo != 0)
    {
        glDeleteBuffers(1, &this->ebo);
        this->ebo = 0;
    }
    if (this->shaderId != 0)
    {
        glDeleteProgram(this->shaderId);
        this->shaderId = 0;
    }

    {
        static const char *vertexShader = GLSL_VERSION R"(
            precision highp float;
            layout(location = 0) in vec3 a_pos;
            layout(location = 1) in vec2 a_uv;
            layout(location = 2) in vec4 a_color;
            uniform mat4 u_worldToView;
            uniform mat4 u_projection;
            out vec2 v_uv;
            out vec4 v_color;
            void main()
            {
                v_uv = a_uv;
                v_color = a_color;
                gl_Position = u_projection * u_worldToView * vec4(a_pos, 1.0);
            }
        )";
        static const char *fragmentShader = GLSL_VERSION R"(
            precision highp float;
            in vec2 v_uv;
            in vec4 v_color;
            uniform float u_time;
            out vec4 FragColor;

            float hash21(vec2 p)
            {
                p = fract(p * vec2(123.34, 345.45));
                p += dot(p, p + 34.345);
                return fract(p.x * p.y);
            }

            float noise(vec2 p)
            {
                vec2 i = floor(p);
                vec2 f = fract(p);
                vec2 u = f * f * (3.0 - 2.0 * f);
                return mix(
                    mix(hash21(i + vec2(0.0, 0.0)), hash21(i + vec2(1.0, 0.0)), u.x),
                    mix(hash21(i + vec2(0.0, 1.0)), hash21(i + vec2(1.0, 1.0)), u.x),
                    u.y
                );
            }

            void main()
            {
                float n = noise(v_uv * vec2(4.0, 7.0) + vec2(u_time * 0.25, -u_time * 0.14));
                float shimmer = 0.5 + 0.5 * sin(6.28318 * (n + v_uv.y * 0.7 + u_time * 0.11));
                vec3 chroma = 0.55 + 0.45 * cos(6.28318 * (vec3(0.00, 0.34, 0.67) + n + u_time * 0.08));
                float feather = smoothstep(0.02, 0.18, v_uv.x) * (1.0 - smoothstep(0.82, 1.0, v_uv.x));
                feather *= smoothstep(0.0, 0.20, v_uv.y) * (1.0 - smoothstep(0.92, 1.0, v_uv.y));
                vec3 tinted = mix(v_color.rgb, chroma, 0.27 + shimmer * 0.16);
                vec3 color = mix(tinted, vec3(1.0), 0.30);
                float centerOpacity = mix(2.0, 1.0, smoothstep(0.18, 0.86, v_uv.x));
                float alpha = v_color.a * feather * centerOpacity * (0.68 + shimmer * 0.22);
                alpha = clamp(alpha, 0.0, 1.0);
                FragColor = vec4(color, alpha);
            }
        )";
        this->shaderId = vtx::createShaderProgram(vertexShader, fragmentShader);
    }

    const uint32_t layerIndices[kIndicesPerLayer] = {
        0, 1, 2,  0, 2, 3,
        3, 2, 4,  3, 4, 5,
        6, 9, 7,  7, 9, 8,
        10, 13, 11,  11, 13, 12,
        14, 17, 15,  15, 17, 16,
        18, 21, 19,  19, 21, 20,
        22, 25, 23,  23, 25, 24,
        26, 28, 27,  26, 29, 28,
        29, 30, 28,  29, 31, 30,
        32, 35, 33,  33, 35, 34,
        36, 39, 37,  37, 39, 38,
        40, 43, 41,  41, 43, 42,
        44, 47, 45,  45, 47, 46,
        48, 51, 49,  49, 51, 50,
    };
    for (int layer = 0; layer < kLayerCount; ++layer)
    {
        const uint32_t baseVertex = uint32_t(layer * kVerticesPerLayer);
        for (int i = 0; i < kIndicesPerLayer; ++i)
            this->indices[(size_t)(layer * kIndicesPerLayer + i)] = baseVertex + layerIndices[i];
    }

    glGenVertexArrays(1, &this->vao);
    glGenBuffers(1, &this->vbo);
    glGenBuffers(1, &this->ebo);

    glBindVertexArray(this->vao);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(WingRenderVertex) * this->vertices.size(), this->vertices.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * this->indices.size(), this->indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(WingRenderVertex), (void *)offsetof(WingRenderVertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(WingRenderVertex), (void *)offsetof(WingRenderVertex, uv));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(WingRenderVertex), (void *)offsetof(WingRenderVertex, color));

    glBindVertexArray(0);
    this->initialized = true;
    this->smoothingValid = false;
    checkOpenGLError("wings init");
}

inline void initWings(WingsState *wings)
{
    if (wings)
        wings->initWings();
}

inline glm::vec3 Wings_NormalizeOr(const glm::vec3 &v, const glm::vec3 &fallback)
{
    float len2 = glm::dot(v, v);
    if (len2 <= 1.0e-8f || !std::isfinite(len2))
        return fallback;
    return v / std::sqrt(len2);
}

inline glm::vec3 Wings_WorldPoint(const glm::mat4 &model, const glm::mat4 &bone, const glm::vec3 &local)
{
    return glm::vec3(model * bone * glm::vec4(local, 1.0f));
}

inline void renderWings(
    WingsState *wings,
    const AssmanAnimPlayer *anim,
    int avatarSlot,
    int backBoneIndex,
    const glm::mat4 &modelMatrix,
    const glm::mat4 &worldToView,
    const glm::mat4 &projection,
    float avatarHeightM,
    float deltaTime,
    float rawTime)
{
    if (!wings || !anim || !anim->anim.header || backBoneIndex < 0)
        return;
    if (backBoneIndex >= (int)anim->globalMatrices.size())
        return;
    if (!wings->initialized || wings->vao == 0 || wings->vbo == 0 || wings->shaderId == 0)
        wings->initWings();

    if (wings->lastAvatarSlot != avatarSlot)
    {
        wings->smoothingValid = false;
        wings->lastAvatarSlot = avatarSlot;
    }

    wings->time += deltaTime;
    const float h = glm::clamp(avatarHeightM, 0.85f, 3.8f);
    const float size = 5.0f;
    const float rootGap = h * 0.075f * size;
    const float span = h * 0.56f * size;
    const float lift = h * 0.42f * size;
    const float drop = h * 0.28f * size;
    const float back = h * 0.050f * size;
    const float flap = std::sin(rawTime * 3.8f);
    const float flapFast = std::sin(rawTime * 6.4f + 0.7f);
    const float flutter = (flap * 0.070f + flapFast * 0.022f) * h * size;

    const glm::mat4 bone = anim->globalMatrices[(size_t)backBoneIndex];
    const glm::vec3 root = Wings_WorldPoint(modelMatrix, bone, glm::vec3(0.0f, -h * 0.025f, 0.0f));

    glm::vec3 side = Wings_NormalizeOr(glm::vec3(modelMatrix * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f)), glm::vec3(1.0f, 0.0f, 0.0f));
    side.y *= 0.15f;
    side = Wings_NormalizeOr(side, glm::vec3(1.0f, 0.0f, 0.0f));
    const glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 backDir = Wings_NormalizeOr(glm::cross(side, up), glm::vec3(0.0f, 0.0f, 1.0f));
    if (backDir.z < 0.0f)
        backDir = -backDir;
    const glm::vec3 wingBackOffset = backDir * 1.25f;
    const glm::vec3 wingInwardOffset = side * 0.25f; // 0.25m per side = wings sit 0.5m closer together.

    std::array<glm::vec3, WingsState::kSmoothedPoints> desired = {};
    const float flapBack = flap * h * size * 0.030f;
    const float flapSpread = 1.0f + flap * 0.055f;
    desired[0] = root - side * (rootGap + span * 0.42f * flapSpread) + up * (lift * 0.72f + flutter) + backDir * (back + flapBack) + wingBackOffset + wingInwardOffset;
    desired[1] = root - side * (rootGap + span * 0.95f * flapSpread) + up * (lift * 0.18f + flutter * 1.7f) + backDir * (back * 1.35f + flapBack * 1.6f) + wingBackOffset + wingInwardOffset;
    desired[2] = root - side * (rootGap + span * 0.78f * flapSpread) - up * (drop * 0.70f - flutter * 0.5f) + backDir * (back * 1.15f + flapBack * 1.2f) + wingBackOffset + wingInwardOffset;
    desired[3] = root - side * (rootGap + span * 0.28f * flapSpread) - up * (drop * 0.20f) + backDir * (back * 0.65f + flapBack * 0.6f) + wingBackOffset + wingInwardOffset;
    desired[4] = root + side * (rootGap + span * 0.42f * flapSpread) + up * (lift * 0.72f + flutter) + backDir * (back + flapBack) + wingBackOffset - wingInwardOffset;
    desired[5] = root + side * (rootGap + span * 0.95f * flapSpread) + up * (lift * 0.18f + flutter * 1.7f) + backDir * (back * 1.35f + flapBack * 1.6f) + wingBackOffset - wingInwardOffset;
    desired[6] = root + side * (rootGap + span * 0.78f * flapSpread) - up * (drop * 0.70f - flutter * 0.5f) + backDir * (back * 1.15f + flapBack * 1.2f) + wingBackOffset - wingInwardOffset;
    desired[7] = root + side * (rootGap + span * 0.28f * flapSpread) - up * (drop * 0.20f) + backDir * (back * 0.65f + flapBack * 0.6f) + wingBackOffset - wingInwardOffset;

    if (!wings->smoothingValid)
    {
        wings->smoothedEdgePoints = desired;
        wings->smoothingValid = true;
    }
    else
    {
        const float catchup = 1.0f - std::exp(-glm::clamp(deltaTime, 0.0f, 0.08f) * 7.0f);
        for (int i = 0; i < WingsState::kSmoothedPoints; ++i)
            wings->smoothedEdgePoints[(size_t)i] = glm::mix(wings->smoothedEdgePoints[(size_t)i], desired[(size_t)i], catchup);
    }

    const glm::vec4 warmBase = glm::vec4(0.93f, 0.99f, 1.0f, 1.0f);
    const glm::vec4 coolBase = glm::vec4(1.0f, 0.89f, 0.99f, 1.0f);
    auto setVertex = [&](int i, glm::vec3 p, glm::vec2 uv, glm::vec4 c) {
        wings->vertices[(size_t)i] = {p, uv, c};
    };

    const glm::vec3 wingAnchor = root + wingBackOffset;
    const glm::vec3 lRootTopBase = root - side * rootGap + up * (h * 0.12f * size + flutter * 0.25f) + backDir * (back * 0.35f + flapBack * 0.35f) + wingBackOffset + wingInwardOffset;
    const glm::vec3 lRootBotBase = root - side * rootGap - up * (h * 0.08f * size - flutter * 0.10f) + backDir * (back * 0.25f + flapBack * 0.25f) + wingBackOffset + wingInwardOffset;
    const glm::vec3 rRootTopBase = root + side * rootGap + up * (h * 0.12f * size + flutter * 0.25f) + backDir * (back * 0.35f + flapBack * 0.35f) + wingBackOffset - wingInwardOffset;
    const glm::vec3 rRootBotBase = root + side * rootGap - up * (h * 0.08f * size - flutter * 0.10f) + backDir * (back * 0.25f + flapBack * 0.25f) + wingBackOffset - wingInwardOffset;
    auto curve3 = [](const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c, float t, float split) {
        if (t <= split)
            return glm::mix(a, b, t / split);
        return glm::mix(b, c, (t - split) / (1.0f - split));
    };
    auto leftUpperAt = [&](float t) {
        return curve3(lRootTopBase, wings->smoothedEdgePoints[0], wings->smoothedEdgePoints[1], t, 0.52f);
    };
    auto leftLowerAt = [&](float t) {
        return curve3(lRootBotBase, wings->smoothedEdgePoints[3], wings->smoothedEdgePoints[2], t, 0.42f);
    };
    auto rightUpperAt = [&](float t) {
        return curve3(rRootTopBase, wings->smoothedEdgePoints[4], wings->smoothedEdgePoints[5], t, 0.52f);
    };
    auto rightLowerAt = [&](float t) {
        return curve3(rRootBotBase, wings->smoothedEdgePoints[7], wings->smoothedEdgePoints[6], t, 0.42f);
    };

    std::array<glm::vec3, 20> leftFeatherVerts = {};
    std::array<glm::vec3, 20> rightFeatherVerts = {};
    auto makeFeatherPanel = [&](std::array<glm::vec3, 20> &out, int featherIndex, const glm::vec3 &upper, const glm::vec3 &lower, const glm::vec3 &acrossDir, float widthMul) {
        const int i = featherIndex * 4;
        const glm::vec3 center = glm::mix(upper, lower, 0.60f) - up * 0.32f + backDir * (0.05f + flapBack * 0.35f);
        const float length = h * size * (0.115f + 0.020f * widthMul);
        const float topHalf = span * (0.013f + 0.004f * widthMul);
        const float bottomHalf = span * (0.032f + 0.006f * widthMul);
        const glm::vec3 topCenter = center + up * (length * 0.50f);
        const glm::vec3 bottomCenter = center - up * (length * 0.50f);
        out[(size_t)i + 0] = topCenter - acrossDir * topHalf;
        out[(size_t)i + 1] = topCenter + acrossDir * topHalf;
        out[(size_t)i + 2] = bottomCenter + acrossDir * bottomHalf;
        out[(size_t)i + 3] = bottomCenter - acrossDir * bottomHalf;
    };

    constexpr float featherT[5] = {0.10f, 0.30f, 0.50f, 0.70f, 0.90f};
    constexpr float featherWidth[5] = {0.55f, 0.85f, 1.15f, 1.00f, 0.70f};
    for (int i = 0; i < 5; ++i)
    {
        const float t = featherT[i];
        makeFeatherPanel(leftFeatherVerts, i, leftUpperAt(t), leftLowerAt(t), -side, featherWidth[i]);
        makeFeatherPanel(rightFeatherVerts, i, rightUpperAt(t), rightLowerAt(t), side, featherWidth[i]);
    }

    auto layeredPoint = [&](glm::vec3 p, float layerScale, float layerOffset) {
        return wingAnchor + (p - wingAnchor) * layerScale + backDir * layerOffset;
    };
    auto layeredColor = [&](glm::vec4 c, float alphaScale) {
        c.a = glm::clamp(c.a * alphaScale, 0.0f, 1.0f);
        return c;
    };

    for (int layer = 0; layer < WingsState::kLayerCount; ++layer)
    {
        const int base = layer * WingsState::kVerticesPerLayer;
        const float layerScale = (layer == 0) ? 1.0f : (layer == 1 ? 1.2f : 1.44f);
        const float alphaScale = (layer == 0) ? 1.0f : (layer == 1 ? 0.72f : 0.50f);
        const float layerOffset = float(layer) * h * 0.035f;
        const glm::vec4 warm = layeredColor(warmBase, alphaScale);
        const glm::vec4 cool = layeredColor(coolBase, alphaScale);

        setVertex(base + 0, layeredPoint(lRootTopBase, layerScale, layerOffset), glm::vec2(0.05f, 0.86f), warm);
        setVertex(base + 1, layeredPoint(lRootBotBase, layerScale, layerOffset), glm::vec2(0.05f, 0.22f), cool);
        setVertex(base + 2, layeredPoint(wings->smoothedEdgePoints[0], layerScale, layerOffset), glm::vec2(0.45f, 0.98f), warm);
        setVertex(base + 3, layeredPoint(wings->smoothedEdgePoints[3], layerScale, layerOffset), glm::vec2(0.34f, 0.10f), cool);
        setVertex(base + 4, layeredPoint(wings->smoothedEdgePoints[1], layerScale, layerOffset), glm::vec2(0.94f, 0.58f), warm);
        setVertex(base + 5, layeredPoint(wings->smoothedEdgePoints[2], layerScale, layerOffset), glm::vec2(0.82f, 0.04f), cool);
        for (int i = 0; i < 20; ++i)
            setVertex(base + 6 + i, layeredPoint(leftFeatherVerts[(size_t)i], layerScale, layerOffset), glm::vec2(0.18f + 0.16f * float(i / 4), (i % 4 < 2) ? 0.78f : 0.24f), cool);
        setVertex(base + 26, layeredPoint(rRootTopBase, layerScale, layerOffset), glm::vec2(0.05f, 0.86f), warm);
        setVertex(base + 27, layeredPoint(rRootBotBase, layerScale, layerOffset), glm::vec2(0.05f, 0.22f), cool);
        setVertex(base + 28, layeredPoint(wings->smoothedEdgePoints[4], layerScale, layerOffset), glm::vec2(0.45f, 0.98f), warm);
        setVertex(base + 29, layeredPoint(wings->smoothedEdgePoints[7], layerScale, layerOffset), glm::vec2(0.34f, 0.10f), cool);
        setVertex(base + 30, layeredPoint(wings->smoothedEdgePoints[5], layerScale, layerOffset), glm::vec2(0.94f, 0.58f), warm);
        setVertex(base + 31, layeredPoint(wings->smoothedEdgePoints[6], layerScale, layerOffset), glm::vec2(0.82f, 0.04f), cool);
        for (int i = 0; i < 20; ++i)
            setVertex(base + 32 + i, layeredPoint(rightFeatherVerts[(size_t)i], layerScale, layerOffset), glm::vec2(0.18f + 0.16f * float(i / 4), (i % 4 < 2) ? 0.78f : 0.24f), cool);
    }

    glBindBuffer(GL_ARRAY_BUFFER, wings->vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(WingRenderVertex) * wings->vertices.size(), wings->vertices.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
    GLboolean cullWasEnabled = glIsEnabled(GL_CULL_FACE);
    GLboolean depthWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean depthMaskWasEnabled = GL_TRUE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMaskWasEnabled);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glUseProgram(wings->shaderId);
    glUniformMatrix4fv(glGetUniformLocation(wings->shaderId, "u_worldToView"), 1, GL_FALSE, glm::value_ptr(worldToView));
    glUniformMatrix4fv(glGetUniformLocation(wings->shaderId, "u_projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform1f(glGetUniformLocation(wings->shaderId, "u_time"), wings->time);

    glBindVertexArray(wings->vao);
    glDrawElements(GL_TRIANGLES, WingsState::kIndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glDepthMask(depthMaskWasEnabled);
    if (depthWasEnabled) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (cullWasEnabled) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (blendWasEnabled) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    checkOpenGLError("render wings");
}
