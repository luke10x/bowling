#pragma once

#include "framework/gl_header.h"
#include "framework/gl_util.h"
#include "framework/boot.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <vector>

struct PinElectricVeins
{
    struct Vertex { glm::vec3 position; glm::vec2 uv; };

    GLuint shader = 0;
    GLuint vao = 0;
    GLuint vbo = 0;
    float time = 0.0f;

    void init()
    {
        shader = vtx::createShaderProgram(VS, FS);
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * 10 * 32 * 6, nullptr, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, position));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, uv));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);
    }

    void render(
        float dt,
        const glm::vec3 &ballPosition,
        const glm::vec3 pinPositions[10],
        uint16_t activeMask,
        const glm::mat4 &view,
        const glm::mat4 &projection
    )
    {
        if (!shader || activeMask == 0u)
            return;
        time += glm::clamp(dt, 0.0f, 0.05f);

        float nearestPinDistance = 2.0f;
        for (int i = 0; i < 10; ++i)
        {
            if ((activeMask & (uint16_t)(1u << i)) == 0u)
                continue;
            const glm::vec2 toPin(
                pinPositions[i].x - ballPosition.x,
                pinPositions[i].z - ballPosition.z
            );
            nearestPinDistance = glm::min(nearestPinDistance, glm::length(toPin));
        }
        if (nearestPinDistance >= 2.0f)
            return;
        const float proximity = glm::smoothstep(0.0f, 1.0f, 1.0f - nearestPinDistance / 2.0f);

        std::vector<Vertex> vertices;
        constexpr int segmentCount = 32;
        vertices.reserve(10 * segmentCount * 6);
        constexpr float halfWidth = 0.052f;
        for (int i = 0; i < 10; ++i)
        {
            if ((activeMask & (uint16_t)(1u << i)) == 0u)
                continue;
            glm::vec3 a = ballPosition;
            glm::vec3 b = pinPositions[i];
            // The veins crawl over the lane between the ball and each pin.
            a.y = b.y = 0.17f;
            glm::vec2 d(b.x - a.x, b.z - a.z);
            const float len = glm::length(d);
            if (len < 0.01f)
                continue;
            const glm::vec2 tangent = d / len;
            const glm::vec2 normal(-tangent.y, tangent.x);
            const float phase = time * 8.0f + (float)i * 1.73f;
            const float amplitude = glm::clamp(0.055f + len * 0.018f, 0.055f, 0.22f);
            auto pointAt = [&](float t) {
                glm::vec3 base = glm::mix(a, b, t);
                const float endpointFade = std::sin(t * 3.14159265f);
                const float lateralWave =
                    std::sin(t * 18.0f + phase) * 0.62f +
                    std::sin(t * 43.0f - phase * 1.7f) * 0.25f +
                    std::sin(t * 79.0f + phase * 0.63f) * 0.13f;
                const float verticalWave =
                    0.085f +
                    std::sin(t * 23.0f - phase * 1.21f) * 0.052f +
                    std::sin(t * 57.0f + phase * 0.77f) * 0.022f;
                base.y += verticalWave * endpointFade;
                return base + glm::vec3(normal.x, 0.0f, normal.y) *
                    (lateralWave * amplitude * endpointFade);
            };
            const glm::vec3 side(normal.x * halfWidth, 0.0f, normal.y * halfWidth);
            for (int segment = 0; segment < segmentCount; ++segment)
            {
                const float t0 = (float)segment / (float)segmentCount;
                const float t1 = (float)(segment + 1) / (float)segmentCount;
                const glm::vec3 p0 = pointAt(t0);
                const glm::vec3 p1 = pointAt(t1);
                vertices.push_back({p0 - side, {t0, -1.0f}});
                vertices.push_back({p0 + side, {t0,  1.0f}});
                vertices.push_back({p1 + side, {t1,  1.0f}});
                vertices.push_back({p0 - side, {t0, -1.0f}});
                vertices.push_back({p1 + side, {t1,  1.0f}});
                vertices.push_back({p1 - side, {t1, -1.0f}});
            }
        }
        if (vertices.empty())
            return;

        GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
        GLboolean depthWasEnabled = glIsEnabled(GL_DEPTH_TEST);
        GLboolean cullWasEnabled = glIsEnabled(GL_CULL_FACE);
        GLboolean depthMaskWasEnabled = GL_TRUE;
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMaskWasEnabled);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);

        glUseProgram(shader);
        glUniformMatrix4fv(glGetUniformLocation(shader, "u_view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader, "u_projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform1f(glGetUniformLocation(shader, "u_time"), time);
        glUniform1f(glGetUniformLocation(shader, "u_proximity"), proximity);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex) * vertices.size(), vertices.data());
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vertices.size());
        glBindVertexArray(0);

        glDepthMask(depthMaskWasEnabled);
        if (!depthWasEnabled) glDisable(GL_DEPTH_TEST);
        if (cullWasEnabled) glEnable(GL_CULL_FACE);
        if (!blendWasEnabled) glDisable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    static const char *VS;
    static const char *FS;
};

const char *PinElectricVeins::VS = GLSL_VERSION R"(
precision highp float;
layout(location=0) in vec3 a_position;
layout(location=1) in vec2 a_uv;
uniform mat4 u_view;
uniform mat4 u_projection;
out vec2 v_uv;
void main() {
    v_uv = a_uv;
    gl_Position = u_projection * u_view * vec4(a_position, 1.0);
}
)";

const char *PinElectricVeins::FS = GLSL_VERSION R"(
precision highp float;
in vec2 v_uv;
uniform float u_time;
uniform float u_proximity;
out vec4 fragColor;

float hash(float n) { return fract(sin(n) * 43758.5453); }
void main() {
    float stepT = floor(u_time * 18.0);
    float bend = sin(v_uv.x * 19.0 + u_time * 13.0) * 0.16;
    bend += (hash(floor(v_uv.x * 9.0) + stepT) - 0.5) * 0.30;
    float coreEdge = mix(0.38, 0.18, u_proximity);
    float core = 1.0 - smoothstep(0.045, coreEdge, abs(v_uv.y - bend));
    float branchA = 1.0 - smoothstep(0.035, 0.12,
        abs(v_uv.y - bend - sin(v_uv.x * 31.0 - u_time * 17.0) * 0.48));
    float branchGate = smoothstep(0.12, 0.35, v_uv.x) * (1.0 - smoothstep(0.68, 0.92, v_uv.x));
    float veins = max(core, branchA * branchGate * 0.62);
    float endFade = smoothstep(0.0, 0.07, v_uv.x) * (1.0 - smoothstep(0.93, 1.0, v_uv.x));
    float pulse = 0.72 + 0.28 * sin(u_time * 22.0 + v_uv.x * 15.0);
    float alpha = veins * endFade * (0.82 + 0.28 * pulse) * u_proximity;
    if (alpha < 0.04) discard;
    vec3 color = mix(vec3(0.72, 0.80, 1.0), vec3(1.0), core);
    float intensity = mix(0.65, 1.0, u_proximity);
    fragColor = vec4(color * (1.15 + core * 0.85) * intensity, alpha * 0.90);
}
)";
