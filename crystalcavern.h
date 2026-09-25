#pragma once

#include "framework/gl_header.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <cmath>

#include "framework/gl_util.h"

struct CrystalCavernVertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec4 color;
};

struct CrystalCavernBiome
{
    static const char *CRYSTAL_VERTEX_SHADER;
    static const char *CRYSTAL_FRAGMENT_SHADER;

    GLuint vao = 0, vbo = 0, ebo = 0, shaderId = 0;
    GLsizei indexCount = 0;
    float scrollZ = 0.0f;
    float waterLineY = -20.0f;
    bool generated = false;

    std::vector<CrystalCavernVertex> vertices;
    std::vector<uint32_t> indices;

    static constexpr float kNearZ = -74.0f;
    static constexpr float kFarZ = 455.0f;
    static constexpr float kCycleM = kFarZ - kNearZ;
    static constexpr float kScrollSpeed = 1.85f;

    static uint32_t hash32(uint32_t x)
    {
        x ^= x >> 16;
        x *= 0x7feb352dU;
        x ^= x >> 15;
        x *= 0x846ca68bU;
        x ^= x >> 16;
        return x;
    }

    static float hash01(int x, int z)
    {
        uint32_t h = hash32(uint32_t(x) * 73856093U ^ uint32_t(z) * 19349663U ^ 0x63c7d91bU);
        return float(h & 0x00ffffffU) / float(0x01000000U);
    }

    void initCrystalCavern()
    {
        scrollZ = 0.0f;
        shaderId = vtx::createShaderProgram(CRYSTAL_VERTEX_SHADER, CRYSTAL_FRAGMENT_SHADER);
        buildMesh();
    }

    void update(float deltaTime)
    {
        scrollZ += deltaTime * kScrollSpeed;
        if (scrollZ > kCycleM)
            scrollZ = std::fmod(scrollZ, kCycleM);
    }

    void setWaterLineY(float y)
    {
        if (std::abs(waterLineY - y) <= 1.0e-4f)
            return;
        waterLineY = y;
        if (generated)
            buildMesh();
    }

    void pushBox(const glm::vec3 &center, const glm::vec3 &half, const glm::vec4 &color)
    {
        const glm::vec3 p[8] = {
            center + glm::vec3(-half.x, -half.y, -half.z),
            center + glm::vec3( half.x, -half.y, -half.z),
            center + glm::vec3( half.x,  half.y, -half.z),
            center + glm::vec3(-half.x,  half.y, -half.z),
            center + glm::vec3(-half.x, -half.y,  half.z),
            center + glm::vec3( half.x, -half.y,  half.z),
            center + glm::vec3( half.x,  half.y,  half.z),
            center + glm::vec3(-half.x,  half.y,  half.z),
        };
        const int faces[6][4] = {
            {0, 1, 2, 3}, {5, 4, 7, 6}, {4, 0, 3, 7},
            {1, 5, 6, 2}, {3, 2, 6, 7}, {4, 5, 1, 0}
        };
        const glm::vec3 normals[6] = {
            glm::vec3(0, 0, -1), glm::vec3(0, 0, 1), glm::vec3(-1, 0, 0),
            glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), glm::vec3(0, -1, 0)
        };
        for (int f = 0; f < 6; ++f)
        {
            const uint32_t base = uint32_t(vertices.size());
            for (int c = 0; c < 4; ++c)
                vertices.push_back({p[faces[f][c]], normals[f], color});
            indices.push_back(base + 0); indices.push_back(base + 1); indices.push_back(base + 2);
            indices.push_back(base + 0); indices.push_back(base + 2); indices.push_back(base + 3);
        }
    }

    void pushCrystal(const glm::vec3 &base, float radius, float height, const glm::vec4 &color, int sides = 6)
    {
        const float dir = height >= 0.0f ? 1.0f : -1.0f;
        const float h = std::abs(height);
        const glm::vec3 axis(0.0f, dir, 0.0f);
        const glm::vec3 embeddedBase = base - axis * (h / 3.0f);
        const glm::vec3 bottomTip = embeddedBase - axis * (h * 0.08f);
        const glm::vec3 lowerCenter = embeddedBase + axis * (h * 0.06f);
        const glm::vec3 shoulderCenter = embeddedBase + axis * (h * 0.42f);
        const glm::vec3 neckCenter = embeddedBase + axis * (h * 0.78f);
        const glm::vec3 topTip = embeddedBase + axis * h;
        const float lowerRadius = radius * 0.46f;
        const float shoulderRadius = radius * 1.18f;
        const float neckRadius = radius * 0.74f;

        auto ringPoint = [&](const glm::vec3 &center, float r, int i)
        {
            const float a = (float(i) / float(sides)) * glm::two_pi<float>() + 0.18f;
            return center + glm::vec3(std::cos(a) * r, 0.0f, std::sin(a) * r);
        };

        for (int i = 0; i < sides; ++i)
        {
            const glm::vec3 lo0 = ringPoint(lowerCenter, lowerRadius, i);
            const glm::vec3 lo1 = ringPoint(lowerCenter, lowerRadius, i + 1);
            const glm::vec3 sh0 = ringPoint(shoulderCenter, shoulderRadius, i);
            const glm::vec3 sh1 = ringPoint(shoulderCenter, shoulderRadius, i + 1);
            const glm::vec3 ne0 = ringPoint(neckCenter, neckRadius, i);
            const glm::vec3 ne1 = ringPoint(neckCenter, neckRadius, i + 1);

            auto tri = [&](const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c)
            {
                glm::vec3 normal = glm::normalize(glm::cross(b - a, c - a));
                if (!std::isfinite(normal.x))
                    normal = glm::vec3(0.0f, dir, 0.0f);
                const uint32_t v = uint32_t(vertices.size());
                vertices.push_back({a, normal, color});
                vertices.push_back({b, normal, color});
                vertices.push_back({c, normal, color});
                indices.push_back(v + 0); indices.push_back(v + 1); indices.push_back(v + 2);
            };
            auto quad = [&](const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c, const glm::vec3 &d)
            {
                tri(a, b, c);
                tri(a, c, d);
            };

            tri(bottomTip, lo1, lo0);
            quad(lo0, lo1, sh1, sh0);
            quad(sh0, sh1, ne1, ne0);
            tri(ne0, ne1, topTip);
        }
    }

    void buildMesh()
    {
        vertices.clear();
        indices.clear();
        vertices.reserve(18000);
        indices.reserve(26000);

        const float waterY = waterLineY;
        const glm::vec4 rockA(0.255f, 0.165f, 0.305f, 1.0f);
        const glm::vec4 rockB(0.185f, 0.135f, 0.245f, 1.0f);
        const glm::vec4 rockPurple(0.345f, 0.155f, 0.405f, 1.0f);
        const glm::vec4 gemColors[5] = {
            glm::vec4(0.22f, 0.92f, 1.00f, 3.2f),
            glm::vec4(1.00f, 0.38f, 0.92f, 3.0f),
            glm::vec4(0.34f, 0.78f, 1.00f, 2.8f),
            glm::vec4(0.92f, 0.48f, 1.00f, 2.9f),
            glm::vec4(0.18f, 0.86f, 0.96f, 3.1f),
        };
        const auto outerTerrainRise = [](float x)
        {
            const float absX = std::abs(x);
            const float approach = glm::smoothstep(30.0f, 40.0f, absX) * 6.0f;
            const float beyond = glm::max(0.0f, absX - 40.0f) * 0.58f;
            return approach + beyond;
        };

        for (int row = 0; row < 30; ++row)
        {
            const float z = kNearZ + 12.0f + float(row) * (kCycleM - 24.0f) / 29.0f;
            for (int side = -1; side <= 1; side += 2)
            {
                for (int col = 0; col < 6; ++col)
                {
                    const int seed = row * 101 + col * 17 + (side > 0 ? 900 : 0);
                    const float x = float(side) * (18.5f + float(col) * 4.9f + (hash01(seed, 11) - 0.5f) * 4.8f);
                    const float zc = z + (hash01(seed, 23) - 0.5f) * 10.5f;
                    const float density = glm::smoothstep(18.0f, 34.0f, std::abs(x));
                    if (hash01(seed, 31) > glm::mix(0.72f, 0.98f, density))
                        continue;

                    float halfX = 2.0f + hash01(seed, 41) * 4.4f;
                    float halfZ = 2.4f + hash01(seed, 43) * 5.8f;
                    float halfY = 3.6f + hash01(seed, 47) * 8.0f;
                    if (hash01(seed, 53) < 0.24f)
                    {
                        halfX *= 1.65f;
                        halfZ *= 1.35f;
                        halfY *= 0.72f;
                    }
                    else if (hash01(seed, 59) > 0.77f)
                    {
                        halfX *= 0.62f;
                        halfZ *= 0.78f;
                        halfY *= 1.60f;
                    }
                    halfY += outerTerrainRise(x);

                    pushBox(
                        glm::vec3(x, waterY, zc),
                        glm::vec3(halfX, halfY, halfZ),
                        hash01(seed, 61) < 0.46f ? rockPurple : (hash01(seed, 67) < 0.58f ? rockA : rockB)
                    );

                    if (std::abs(x) > 40.0f && hash01(seed, 347) > 0.48f)
                    {
                        const int shards = 2 + int(hash01(seed, 349) * 3.0f);
                        for (int s = 0; s < shards; ++s)
                        {
                            const float dx = (hash01(seed + s * 19, 353) - 0.5f) * halfX * 0.95f;
                            const float dz = (hash01(seed + s * 23, 359) - 0.5f) * halfZ * 0.95f;
                            const float h = 4.5f + hash01(seed + s * 29, 367) * 8.5f;
                            const float r = 0.55f + hash01(seed + s * 31, 373) * 1.15f;
                            const int colorIdx = int(hash01(seed + s * 37, 379) * 5.0f) % 5;
                            pushCrystal(glm::vec3(x + dx, waterY + halfY + 0.10f, zc + dz), r, h, gemColors[colorIdx], 5 + (s % 3));
                        }
                    }
                }
            }

            for (int mcol = 0; mcol < 18; ++mcol)
            {
                const int seed = row * 211 + mcol * 29;
                float t = (float(mcol) + 0.5f) / 18.0f;
                float x = (t - 0.5f) * 31.0f + (hash01(seed, 71) - 0.5f) * 7.4f;
                const float absX = std::abs(x);
                if (absX < 4.6f || absX > 18.0f)
                    continue;

                const float clusterNoise = hash01(seed, 73);
                float keep = glm::smoothstep(5.0f, 18.0f, absX) * glm::smoothstep(0.18f, 0.82f, clusterNoise);
                if (hash01(seed, 79) > keep)
                    continue;

                const float zc = z + (hash01(seed, 83) - 0.5f) * 9.0f;
                const bool crystalPocket = hash01(seed, 89) > 0.54f;
                const float halfX = crystalPocket ? (2.8f + hash01(seed, 97) * 3.0f) : (1.2f + hash01(seed, 101) * 2.2f);
                const float halfZ = crystalPocket ? (2.5f + hash01(seed, 103) * 3.4f) : (1.2f + hash01(seed, 107) * 2.4f);
                const float halfY = crystalPocket ? (2.9f + hash01(seed, 109) * 3.4f) : (1.8f + hash01(seed, 113) * 2.6f);

                pushBox(
                    glm::vec3(x, waterY, zc),
                    glm::vec3(halfX, halfY, halfZ),
                    hash01(seed, 127) < 0.62f ? rockPurple : rockB
                );

                if (crystalPocket)
                {
                    const int shards = 3 + int(hash01(seed, 131) * 4.0f);
                    for (int s = 0; s < shards; ++s)
                    {
                        const float dx = (hash01(seed + s * 13, 137) - 0.5f) * halfX * 1.05f;
                        const float dz = (hash01(seed + s * 17, 139) - 0.5f) * halfZ * 1.05f;
                        const float r = 0.38f + hash01(seed + s * 19, 149) * 0.92f;
                        const float h = 3.6f + hash01(seed + s * 23, 151) * 8.5f;
                        const int colorIdx = int(hash01(seed + s * 29, 157) * 5.0f) % 5;
                        pushCrystal(glm::vec3(x + dx, waterY + halfY + 0.10f, zc + dz), r, h, gemColors[colorIdx], 5 + (s % 3));
                    }
                }
            }

            for (int side = -1; side <= 1; side += 2)
            {
                const float wallX = float(side) * (44.0f + hash01(row, side * 11) * 30.0f);
                const float wallH = 18.0f + hash01(row * 17, side * 19) * 24.0f;
                const float wallHalfY = wallH * 0.5f + outerTerrainRise(wallX);
                const float wallHalfX = 12.0f + hash01(row, side * 31) * 20.0f;
                const float wallHalfZ = 10.0f + hash01(row, side * 37) * 14.0f;
                pushBox(
                    glm::vec3(wallX, waterY, z),
                    glm::vec3(wallHalfX, wallHalfY, wallHalfZ),
                    hash01(row, side * 41) < 0.34f ? rockPurple : (hash01(row, side * 43) < 0.55f ? rockA : rockB)
                );
                if (row % 3 == (side > 0 ? 1 : 2))
                {
                    const int shards = 2 + int(hash01(row, side * 383) * 3.0f);
                    for (int s = 0; s < shards; ++s)
                    {
                        const float dx = (hash01(row * 389 + s, side * 397) - 0.5f) * wallHalfX * 0.85f;
                        const float dz = (hash01(row * 401 + s, side * 409) - 0.5f) * wallHalfZ * 0.85f;
                        const float h = 6.0f + hash01(row * 419 + s, side * 421) * 10.0f;
                        const float r = 0.70f + hash01(row * 431 + s, side * 433) * 1.35f;
                        const int colorIdx = int(hash01(row * 439 + s, side * 443) * 5.0f) % 5;
                        pushCrystal(glm::vec3(wallX + dx, waterY + wallHalfY + 0.10f, z + dz), r, h, gemColors[colorIdx], 5 + (s % 3));
                    }
                }
                for (int ledge = 0; ledge < 3; ++ledge)
                {
                    const float ledgeX = float(side) * (28.0f + float(ledge) * 12.5f + hash01(row * 223 + ledge, side * 227) * 7.0f);
                    const float ledgeY = waterY - 0.8f + float(ledge) * 1.9f + hash01(row * 229 + ledge, side * 233) * 0.8f;
                    const float ledgeHalfY = 1.1f + hash01(row * 263 + ledge, side * 269) * 1.6f + outerTerrainRise(ledgeX) * 0.55f;
                    pushBox(
                        glm::vec3(ledgeX, ledgeY, z + (hash01(row * 239 + ledge, side * 241) - 0.5f) * 12.0f),
                        glm::vec3(7.0f + hash01(row * 251 + ledge, side * 257) * 9.0f, ledgeHalfY, 5.0f + hash01(row * 271 + ledge, side * 277) * 8.0f),
                        ledge == 0 ? rockB : (hash01(row, side * (ledge + 281)) < 0.5f ? rockA : rockPurple)
                    );
                }

                const int clusterCount = 3 + int(hash01(row * 23, side * 29) * 3.0f);
                for (int c = 0; c < clusterCount; ++c)
                {
                    const float nearLane = hash01(row * 43, c + side * 7) < 0.28f ? 1.0f : 0.0f;
                    const float x = float(side) * glm::mix(
                        43.0f + hash01(row * 59 + c, side * 61) * 42.0f,
                        24.0f + hash01(row * 47 + c, side * 53) * 16.0f,
                        nearLane
                    );
                    const float cz = z + (hash01(row * 67, c + side * 71) - 0.5f) * 18.0f;
                    const int shards = 5 + int(hash01(row * 73 + c, side * 79) * 6.0f);
                    const float padHalfY = 2.70f + hash01(row * 293 + c, side * 307) * 1.60f + outerTerrainRise(x) * 0.55f;
                    pushBox(
                        glm::vec3(x, waterY, cz),
                        glm::vec3(6.8f + hash01(row * 281 + c, side * 283) * 7.6f, padHalfY, 5.4f + hash01(row * 311 + c, side * 313) * 6.4f),
                        hash01(row * 317 + c, side * 331) < 0.5f ? rockA : rockB
                    );
                    for (int s = 0; s < shards; ++s)
                    {
                        const float dx = (hash01(row * 83 + s, c * 89) - 0.5f) * 6.0f;
                        const float dz = (hash01(row * 97 + s, c * 101) - 0.5f) * 6.0f;
                        const float r = 0.55f + hash01(row * 103 + s, c * 107) * 1.55f;
                        const float h = 5.0f + hash01(row * 109 + s, c * 113) * 13.0f;
                        const int colorIdx = int(hash01(row * 127 + s, c * 131) * 5.0f) % 5;
                        const float crystalX = float(side) * glm::max(22.0f, std::abs(x + dx));
                        pushCrystal(glm::vec3(crystalX, waterY + padHalfY + 0.12f, cz + dz), r, h, gemColors[colorIdx], 5 + (s % 3));
                    }
                }

            }

            if (row % 5 == 2)
            {
                const int side = hash01(row, 211) < 0.5f ? -1 : 1;
                const float x = float(side) * (23.0f + hash01(row, 213) * 11.0f);
                const float zc = z + (hash01(row, 223) - 0.5f) * 10.0f;
                const int colorIdx = int(hash01(row, 227) * 5.0f) % 5;
                pushBox(glm::vec3(x, waterY, zc), glm::vec3(5.4f, 3.2f, 5.8f), rockA);
                pushCrystal(glm::vec3(x, waterY + 3.32f, zc), 1.0f + hash01(row, 229) * 0.9f, 5.5f + hash01(row, 233) * 6.5f, gemColors[colorIdx], 6);
            }
        }

        indexCount = GLsizei(indices.size());
        if (ebo) glDeleteBuffers(1, &ebo);
        if (vbo) glDeleteBuffers(1, &vbo);
        if (vao) glDeleteVertexArrays(1, &vao);
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(vertices.size() * sizeof(CrystalCavernVertex)), vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(indices.size() * sizeof(uint32_t)), indices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(CrystalCavernVertex), (void *)offsetof(CrystalCavernVertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(CrystalCavernVertex), (void *)offsetof(CrystalCavernVertex, normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(CrystalCavernVertex), (void *)offsetof(CrystalCavernVertex, color));
        glBindVertexArray(0);
        generated = true;
        checkOpenGLError("crystal cavern init");
    }

    void renderCrystalCavern(const glm::mat4 &cameraMatrix, const glm::mat4 &projectionMatrix, float timeSeconds)
    {
        if (!generated)
            buildMesh();

        const glm::mat4 viewMatrix = glm::inverse(cameraMatrix);
        const glm::vec3 cameraPos = glm::vec3(cameraMatrix[3]);
        const float tileOffsets[2] = {-scrollZ, -scrollZ + kCycleM};

        glUseProgram(shaderId);
        glUniformMatrix4fv(glGetUniformLocation(shaderId, "u_worldToView"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shaderId, "u_projection"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
        glUniform3fv(glGetUniformLocation(shaderId, "u_cameraPos"), 1, glm::value_ptr(cameraPos));
        glUniform1f(glGetUniformLocation(shaderId, "u_time"), timeSeconds);
        glUniform1f(glGetUniformLocation(shaderId, "u_waterLineY"), waterLineY);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBindVertexArray(vao);
        for (float zOffset : tileOffsets)
        {
            const glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, zOffset));
            glUniformMatrix4fv(glGetUniformLocation(shaderId, "u_modelToWorld"), 1, GL_FALSE, glm::value_ptr(model));
            glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
        }
        glBindVertexArray(0);
        glDisable(GL_BLEND);
    }
};

const char *CrystalCavernBiome::CRYSTAL_VERTEX_SHADER = GLSL_VERSION R"(
    precision highp float;
    layout(location = 0) in vec3 a_pos;
    layout(location = 1) in vec3 a_normal;
    layout(location = 2) in vec4 a_color;
    uniform mat4 u_modelToWorld;
    uniform mat4 u_worldToView;
    uniform mat4 u_projection;
    out vec3 v_worldPos;
    out vec3 v_normal;
    out vec4 v_color;
    void main()
    {
        vec4 worldPos = u_modelToWorld * vec4(a_pos, 1.0);
        v_worldPos = worldPos.xyz;
        v_normal = normalize(mat3(u_modelToWorld) * a_normal);
        v_color = a_color;
        gl_Position = u_projection * u_worldToView * worldPos;
    }
)";

const char *CrystalCavernBiome::CRYSTAL_FRAGMENT_SHADER = GLSL_VERSION R"(
    precision highp float;
    in vec3 v_worldPos;
    in vec3 v_normal;
    in vec4 v_color;
    uniform vec3 u_cameraPos;
    uniform float u_time;
    uniform float u_waterLineY;
    out vec4 FragColor;
    float hash21(vec2 p)
    {
        p = fract(p * vec2(123.34, 456.21));
        p += dot(p, p + 45.32);
        return fract(p.x * p.y);
    }
    void main()
    {
        vec3 n = normalize(v_normal);
        vec3 lightDir = normalize(vec3(-0.20, 0.82, -0.42));
        float diffuse = clamp(dot(n, lightDir), 0.0, 1.0);
        float rim = pow(1.0 - clamp(dot(n, normalize(u_cameraPos - v_worldPos)), 0.0, 1.0), 2.1);
        float pulse = 0.72 + 0.28 * sin(u_time * 1.8 + v_worldPos.z * 0.05 + v_worldPos.x * 0.09);
        float backdrop = 1.0 - step(0.9, v_color.a);
        if (backdrop > 0.5)
        {
            float yT = smoothstep(u_waterLineY, u_waterLineY + 75.0, v_worldPos.y);
            vec3 color = mix(vec3(0.052, 0.038, 0.120), vec3(0.155, 0.075, 0.265), yT);
            float horizon = (1.0 - smoothstep(u_waterLineY + 2.0, u_waterLineY + 48.0, v_worldPos.y)) *
                            (1.0 - smoothstep(120.0, 360.0, abs(v_worldPos.x)));
            color += horizon * vec3(0.34, 0.12, 0.46);
            float star = step(0.986, hash21(floor(v_worldPos.xy * vec2(0.45, 0.62))));
            float shimmer = 0.65 + 0.35 * sin(u_time * 1.7 + v_worldPos.x * 0.13 + v_worldPos.y * 0.09);
            color += star * shimmer * vec3(0.35, 0.72, 0.95);
            color += vec3(0.03, 0.07, 0.14) * smoothstep(8.0, 28.0, abs(v_worldPos.x));
            FragColor = vec4(color, 1.0);
            return;
        }
        float gem = step(2.0, v_color.a);
        float pathGlow = 0.0;
        float floorGlow = 0.0;
        vec3 crystalDeep = vec3(0.28, 0.12, 0.86);
        vec3 crystalViolet = vec3(0.78, 0.30, 1.00);
        vec3 crystalCyan = vec3(0.28, 0.92, 1.00);
        vec3 crystalRose = vec3(1.00, 0.38, 0.86);
        vec3 crystalWhite = vec3(0.86, 0.92, 1.00);
        float palettePhase = fract(u_time * 0.220 + v_worldPos.z * 0.010 + v_worldPos.x * 0.017);
        vec3 morphColor = mix(crystalDeep, crystalViolet, smoothstep(0.00, 0.22, palettePhase));
        morphColor = mix(morphColor, crystalCyan, smoothstep(0.22, 0.45, palettePhase));
        morphColor = mix(morphColor, crystalWhite, smoothstep(0.45, 0.58, palettePhase));
        morphColor = mix(morphColor, crystalRose, smoothstep(0.58, 0.78, palettePhase));
        morphColor = mix(morphColor, crystalDeep, smoothstep(0.78, 1.00, palettePhase));
        vec3 gemBase = mix(v_color.rgb, morphColor, 0.96);
        vec3 rockTint = v_color.rgb * (0.52 + diffuse * 0.58) + vec3(0.12, 0.075, 0.18) * rim;
        vec3 gemTint = gemBase * (0.44 + diffuse * 0.30) + gemBase * (v_color.a * pulse * 0.92 + rim * 0.92);
        vec3 color = mix(rockTint, gemTint, gem);
        color += pathGlow * vec3(0.03, 0.16, 0.28) * (0.55 + 0.10 * sin(u_time * 1.4 + v_worldPos.z * 0.12));
        color += floorGlow * mix(vec3(0.06, 0.18, 0.34), morphColor, 0.34) * (0.07 + 0.13 * pulse);
        color += gem * gemBase * pow(max(0.0, dot(n, normalize(u_cameraPos - v_worldPos))), 8.0) * 0.85;
        color += gem * vec3(0.22, 0.30, 0.38) * rim;
        color -= hash21(floor(v_worldPos.xz * 0.28)) * vec3(0.018);
        float fogT = smoothstep(120.0, 410.0, v_worldPos.z);
        color = mix(color, vec3(0.040, 0.035, 0.075), fogT * 0.42);
        float alpha = mix(1.0, 0.48 + 0.14 * pulse, gem);
        FragColor = vec4(color, alpha);
    }
)";
