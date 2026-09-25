#pragma once

#include "framework/gl_header.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <cmath>

#include "framework/gl_util.h"

struct GasFactoryVertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec4 color;
    float cargoCenterX = 0.0f;
};

struct GasFactorySmokeVertex
{
    glm::vec3 position;
    glm::vec2 uv;
    glm::vec4 params;
};

struct GasFactorySparkVertex
{
    glm::vec3 origin;
    glm::vec4 params;
};

struct GasFactoryBiome
{
    static const char *FACTORY_VERTEX_SHADER;
    static const char *FACTORY_FRAGMENT_SHADER;
    static const char *SMOKE_VERTEX_SHADER;
    static const char *SMOKE_FRAGMENT_SHADER;
    static const char *SPARK_VERTEX_SHADER;
    static const char *SPARK_FRAGMENT_SHADER;

    GLuint vao = 0, vbo = 0, ebo = 0, shaderId = 0;
    GLuint smokeVao = 0, smokeVbo = 0, smokeEbo = 0, smokeShaderId = 0;
    GLuint sparkVao = 0, sparkVbo = 0, sparkShaderId = 0;
    GLsizei indexCount = 0;
    GLsizei smokeIndexCount = 0;
    GLsizei sparkCount = 0;
    float scrollZ = 0.0f;
    bool generated = false;

    std::vector<GasFactoryVertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<GasFactorySmokeVertex> smokeVertices;
    std::vector<uint32_t> smokeIndices;
    std::vector<GasFactorySparkVertex> sparkVertices;

    static constexpr float kNearZ = -78.0f;
    static constexpr float kFarZ = 450.0f;
    static constexpr float kCycleM = kFarZ - kNearZ;
    static constexpr float kScrollSpeed = 2.05f;

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
        uint32_t h = hash32(uint32_t(x) * 73856093U ^ uint32_t(z) * 19349663U ^ 0x4a7c15b3U);
        return float(h & 0x00ffffffU) / float(0x01000000U);
    }

    void initGasFactory()
    {
        this->scrollZ = 0.0f;
        this->shaderId = vtx::createShaderProgram(FACTORY_VERTEX_SHADER, FACTORY_FRAGMENT_SHADER);
        this->smokeShaderId = vtx::createShaderProgram(SMOKE_VERTEX_SHADER, SMOKE_FRAGMENT_SHADER);
        this->sparkShaderId = vtx::createShaderProgram(SPARK_VERTEX_SHADER, SPARK_FRAGMENT_SHADER);
        this->buildFactoryMesh();
        this->buildSmokeMesh();
        this->buildSparkMesh();
    }

    void update(float deltaTime)
    {
        this->scrollZ += deltaTime * kScrollSpeed;
        if (this->scrollZ > kCycleM)
            this->scrollZ = std::fmod(this->scrollZ, kCycleM);
    }

    void pushBox(const glm::vec3 &center, const glm::vec3 &half, const glm::vec4 &color, float cargoCenterX = 0.0f)
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
                vertices.push_back({p[faces[f][c]], normals[f], color, cargoCenterX});
            indices.push_back(base + 0); indices.push_back(base + 1); indices.push_back(base + 2);
            indices.push_back(base + 0); indices.push_back(base + 2); indices.push_back(base + 3);
        }
    }

    void pushCylinder(const glm::vec3 &center, float radius, float halfHeight, const glm::vec4 &color, int segments = 18)
    {
        for (int i = 0; i < segments; ++i)
        {
            const float a0 = float(i) / float(segments) * glm::two_pi<float>();
            const float a1 = float(i + 1) / float(segments) * glm::two_pi<float>();
            const glm::vec3 n0(std::cos(a0), 0.0f, std::sin(a0));
            const glm::vec3 n1(std::cos(a1), 0.0f, std::sin(a1));
            const glm::vec3 p0 = center + glm::vec3(n0.x * radius, -halfHeight, n0.z * radius);
            const glm::vec3 p1 = center + glm::vec3(n1.x * radius, -halfHeight, n1.z * radius);
            const glm::vec3 p2 = center + glm::vec3(n1.x * radius, halfHeight, n1.z * radius);
            const glm::vec3 p3 = center + glm::vec3(n0.x * radius, halfHeight, n0.z * radius);
            const uint32_t base = uint32_t(vertices.size());
            vertices.push_back({p0, n0, color});
            vertices.push_back({p1, n1, color});
            vertices.push_back({p2, n1, color});
            vertices.push_back({p3, n0, color});
            indices.push_back(base + 0); indices.push_back(base + 1); indices.push_back(base + 2);
            indices.push_back(base + 0); indices.push_back(base + 2); indices.push_back(base + 3);
        }
    }

    void pushCable(const glm::vec3 &a, const glm::vec3 &b, float thickness, const glm::vec4 &color)
    {
        const glm::vec3 mid = (a + b) * 0.5f;
        const glm::vec3 d = b - a;
        const float len = glm::length(d);
        if (len < 0.001f)
            return;
        glm::vec3 side = glm::normalize(glm::cross(glm::normalize(d), glm::vec3(0, 1, 0)));
        if (!std::isfinite(side.x))
            side = glm::vec3(1, 0, 0);
        pushBox(mid, glm::vec3(thickness, thickness, len * 0.5f), color);
    }

    void buildFactoryMesh()
    {
        vertices.clear();
        indices.clear();
        vertices.reserve(12000);
        indices.reserve(19000);

        pushBox(glm::vec3(0.0f, -28.5f, 170.0f), glm::vec3(145.0f, 1.5f, 340.0f), glm::vec4(0.10f, 0.105f, 0.10f, 1.0f));

        const glm::vec4 concrete(0.28f, 0.29f, 0.27f, 1.0f);
        const glm::vec4 plateDark(0.135f, 0.145f, 0.138f, 1.0f);
        const glm::vec4 plateCool(0.165f, 0.178f, 0.170f, 1.0f);
        const glm::vec4 plateRust(0.22f, 0.155f, 0.105f, 1.0f);
        const glm::vec4 darkSteel(0.10f, 0.13f, 0.14f, 1.0f);
        const glm::vec4 rusty(0.42f, 0.22f, 0.11f, 1.0f);
        const glm::vec4 utilityPipe(0.34f, 0.20f, 0.28f, 1.0f);
        const glm::vec4 conveyorBelt(0.055f, 0.064f, 0.066f, 1.0f);
        const glm::vec4 conveyorRail(0.26f, 0.29f, 0.28f, 1.0f);
        const glm::vec4 pole(0.18f, 0.19f, 0.18f, 1.0f);

        for (int zCell = 0; zCell < 22; ++zCell)
        {
            const float z0 = kNearZ + float(zCell) * (kCycleM / 22.0f);
            const float z1 = kNearZ + float(zCell + 1) * (kCycleM / 22.0f);
            for (int xCell = 0; xCell < 12; ++xCell)
            {
                const float x0 = -126.0f + float(xCell) * 21.0f;
                const float x1 = -126.0f + float(xCell + 1) * 21.0f;
                const float jitterX = (hash01(xCell, zCell) - 0.5f) * 1.2f;
                const float jitterZ = (hash01(xCell + 31, zCell + 7) - 0.5f) * 1.8f;
                const float cx = (x0 + x1) * 0.5f + jitterX;
                const float cz = (z0 + z1) * 0.5f + jitterZ;
                const float hx = (x1 - x0) * 0.5f - 0.55f;
                const float hz = (z1 - z0) * 0.5f - 0.70f;
                const float r = hash01(xCell * 17, zCell * 23);
                const glm::vec4 plateColor = r < 0.18f ? plateRust : (r < 0.58f ? plateDark : plateCool);
                pushBox(glm::vec3(cx, -26.78f + r * 0.10f, cz), glm::vec3(hx, 0.10f, hz), plateColor);
            }
        }

        for (int belt = 0; belt < 12; ++belt)
        {
            const float z = kNearZ + 42.0f + float(belt) * ((kCycleM - 66.0f) / 11.0f);
            const float y = -24.95f + hash01(belt, 501) * 0.22f;
            pushBox(glm::vec3(0.0f, y, z), glm::vec3(50.0f, 0.42f, 3.2f), conveyorBelt);
            pushBox(glm::vec3(0.0f, y + 0.34f, z - 3.7f), glm::vec3(50.0f, 0.55f, 0.28f), conveyorRail);
            pushBox(glm::vec3(0.0f, y + 0.34f, z + 3.7f), glm::vec3(50.0f, 0.55f, 0.28f), conveyorRail);
            for (int roller = 0; roller < 12; ++roller)
            {
                const float x = -47.0f + float(roller) * 94.0f / 11.0f;
                pushBox(glm::vec3(x, y + 0.60f, z), glm::vec3(0.42f, 0.18f, 3.5f), conveyorRail);
            }

            const int groupCount = 3 + int(hash01(belt, 1201) * 4.0f);
            const float dirFlag = (hash01(belt, 1307) < 0.5f) ? 2.0f : 3.0f;
            const float groupStart = -34.0f + hash01(belt, 1501) * 68.0f;
            for (int item = 0; item < groupCount; ++item)
            {
                const float spacing = 6.2f + hash01(belt * 17 + item, 1409) * 2.2f;
                const float startX = groupStart + (float(item) - float(groupCount - 1) * 0.5f) * spacing;
                const glm::vec4 cargoColor(
                    0.26f + hash01(belt, item) * 0.10f,
                    0.18f + hash01(belt + item, 73) * 0.08f,
                    0.12f + hash01(item, belt + 19) * 0.06f,
                    dirFlag
                );
                pushBox(
                    glm::vec3(startX, y + 1.45f + hash01(belt, item + 81) * 0.15f, z),
                    glm::vec3(2.3f + hash01(item, 7) * 1.0f, 0.95f + hash01(item, 11) * 0.35f, 1.8f + hash01(item, 13) * 0.8f),
                    cargoColor,
                    startX
                );
            }
        }

        for (int row = 0; row < 16; ++row)
        {
            const float z = kNearZ + 22.0f + float(row) * 31.0f;
            for (int side = -1; side <= 1; side += 2)
            {
                const float xBase = float(side) * (44.0f + hash01(row, side * 17) * 52.0f);
                const float h = 11.0f + hash01(row * 9, side * 31) * 32.0f;
                pushBox(glm::vec3(xBase, -27.0f + h * 0.5f, z), glm::vec3(8.0f + hash01(row, side) * 10.0f, h * 0.5f, 8.0f), concrete);
                pushBox(glm::vec3(xBase, -27.0f + h + 2.0f, z), glm::vec3(10.0f, 1.1f, 10.0f), darkSteel);

                if ((row + side) % 3 != 0)
                {
                    const float stackH = h + 25.0f + hash01(row * 3, side * 5) * 28.0f;
                    pushCylinder(glm::vec3(xBase + float(side) * 11.0f, -27.0f + stackH * 0.5f, z + 2.0f), 2.4f, stackH * 0.5f, darkSteel, 16);
                    pushCylinder(glm::vec3(xBase + float(side) * 11.0f, -27.0f + stackH + 1.2f, z + 2.0f), 3.0f, 1.2f, rusty, 16);
                }

                if (row % 2 == 0)
                {
                    const float tankX = xBase - float(side) * (13.0f + hash01(row, 91) * 7.0f);
                    pushCylinder(glm::vec3(tankX, -18.5f, z + 13.0f), 7.0f + hash01(row, 44) * 4.0f, 8.5f, rusty, 22);
                }
            }

            if (row % 3 == 0)
            {
                const float pipeY = 14.5f + hash01(row, 731) * 5.5f;
                pushBox(glm::vec3(0.0f, pipeY, z), glm::vec3(74.0f, 1.1f, 1.1f), utilityPipe);
                const float supportHalfH = glm::max(0.5f, (pipeY + 26.75f) * 0.5f);
                const float supportCenterY = -26.75f + supportHalfH;
                pushBox(glm::vec3(-37.0f, supportCenterY, z), glm::vec3(1.4f, supportHalfH, 1.4f), utilityPipe);
                pushBox(glm::vec3(37.0f, supportCenterY, z), glm::vec3(1.4f, supportHalfH, 1.4f), utilityPipe);
            }

            const float poleZ = z + 15.0f;
            pushBox(glm::vec3(-112.0f, -10.0f, poleZ), glm::vec3(1.0f, 18.0f, 1.0f), pole);
            pushBox(glm::vec3(112.0f, -10.0f, poleZ), glm::vec3(1.0f, 18.0f, 1.0f), pole);
            pushBox(glm::vec3(-112.0f, 11.5f, poleZ), glm::vec3(8.0f, 0.55f, 0.55f), pole);
            pushBox(glm::vec3(112.0f, 11.5f, poleZ), glm::vec3(8.0f, 0.55f, 0.55f), pole);
            if (row % 3 == 0)
                pushBox(glm::vec3(0.0f, 9.5f, poleZ), glm::vec3(112.0f, 0.18f, 0.18f), glm::vec4(0.07f, 0.08f, 0.075f, 1.0f));
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
        glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(vertices.size() * sizeof(GasFactoryVertex)), vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(indices.size() * sizeof(uint32_t)), indices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GasFactoryVertex), (void *)offsetof(GasFactoryVertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GasFactoryVertex), (void *)offsetof(GasFactoryVertex, normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(GasFactoryVertex), (void *)offsetof(GasFactoryVertex, color));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(GasFactoryVertex), (void *)offsetof(GasFactoryVertex, cargoCenterX));
        glBindVertexArray(0);
        generated = true;
        checkOpenGLError("gas factory init");
    }

    void buildSmokeMesh()
    {
        smokeVertices.clear();
        smokeIndices.clear();
        smokeVertices.reserve(64 * 7 * 4);
        smokeIndices.reserve(64 * 7 * 6);

        auto pushQuad = [&](const glm::vec3 &center, float width, float height, float phase, float density)
        {
            const uint32_t base = uint32_t(smokeVertices.size());
            const glm::vec3 right(width * 0.5f, 0.0f, 0.0f);
            const glm::vec3 up(0.0f, height * 0.5f, 0.0f);
            smokeVertices.push_back({center - right - up, glm::vec2(0, 0), glm::vec4(phase, width, height, density)});
            smokeVertices.push_back({center + right - up, glm::vec2(1, 0), glm::vec4(phase, width, height, density)});
            smokeVertices.push_back({center + right + up, glm::vec2(1, 1), glm::vec4(phase, width, height, density)});
            smokeVertices.push_back({center - right + up, glm::vec2(0, 1), glm::vec4(phase, width, height, density)});
            smokeIndices.push_back(base + 0); smokeIndices.push_back(base + 1); smokeIndices.push_back(base + 2);
            smokeIndices.push_back(base + 0); smokeIndices.push_back(base + 2); smokeIndices.push_back(base + 3);
        };

        for (int stack = 0; stack < 64; ++stack)
        {
            const int side = (stack % 2 == 0) ? -1 : 1;
            const float x = float(side) * (56.0f + hash01(stack, 5) * 48.0f);
            const float z = kNearZ + 20.0f + hash01(stack, 39) * (kCycleM - 60.0f);
            const float baseY = 8.0f + hash01(stack, 83) * 24.0f;
            const float phaseBase = hash01(stack, 991) * 9.0f;
            for (int puff = 0; puff < 7; ++puff)
            {
                const float t = float(puff) / 6.0f;
                pushQuad(
                    glm::vec3(x, baseY, z + (hash01(stack, puff) - 0.5f) * 2.0f),
                    8.5f,
                    4.8f,
                    phaseBase + t,
                    0.48f + t * 0.32f
                );
            }
        }

        smokeIndexCount = GLsizei(smokeIndices.size());
        if (smokeEbo) glDeleteBuffers(1, &smokeEbo);
        if (smokeVbo) glDeleteBuffers(1, &smokeVbo);
        if (smokeVao) glDeleteVertexArrays(1, &smokeVao);
        glGenVertexArrays(1, &smokeVao);
        glGenBuffers(1, &smokeVbo);
        glGenBuffers(1, &smokeEbo);
        glBindVertexArray(smokeVao);
        glBindBuffer(GL_ARRAY_BUFFER, smokeVbo);
        glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(smokeVertices.size() * sizeof(GasFactorySmokeVertex)), smokeVertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, smokeEbo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(smokeIndices.size() * sizeof(uint32_t)), smokeIndices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GasFactorySmokeVertex), (void *)offsetof(GasFactorySmokeVertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GasFactorySmokeVertex), (void *)offsetof(GasFactorySmokeVertex, uv));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(GasFactorySmokeVertex), (void *)offsetof(GasFactorySmokeVertex, params));
        glBindVertexArray(0);
    }

    void buildSparkMesh()
    {
        sparkVertices.clear();
        sparkVertices.reserve(16 * 12);

        for (int row = 0; row < 16; ++row)
        {
            if (row % 3 != 0)
                continue;

            const float z = kNearZ + 22.0f + float(row) * 31.0f;
            const float pipeY = 14.5f + hash01(row, 731) * 5.5f;
            const int emitterCount = hash01(row, 2003) < 0.38f ? 1 : 2;
            for (int emitter = 0; emitter < emitterCount; ++emitter)
            {
                const float x = -8.8f + hash01(row * 23, emitter * 29) * 17.6f;
                const float zJitter = (hash01(row * 31, emitter * 37) - 0.5f) * 1.6f;
                const float emitterPhase = hash01(row * 41, emitter * 43);
                for (int spark = 0; spark < 7; ++spark)
                {
                    const float phase = glm::fract(emitterPhase + float(spark) * 0.113f);
                    const float fall = 12.0f + hash01(row * 47 + spark, emitter * 53) * 15.0f;
                    const float drift = (hash01(row * 59 + spark, emitter * 61) - 0.5f) * 2.4f;
                    const float size = 6.5f + hash01(row * 67 + spark, emitter * 71) * 5.0f;
                    sparkVertices.push_back({
                        glm::vec3(x, pipeY - 1.25f, z + zJitter),
                        glm::vec4(phase, fall, drift, size)
                    });
                }
            }
        }

        sparkCount = GLsizei(sparkVertices.size());
        if (sparkVbo) glDeleteBuffers(1, &sparkVbo);
        if (sparkVao) glDeleteVertexArrays(1, &sparkVao);
        glGenVertexArrays(1, &sparkVao);
        glGenBuffers(1, &sparkVbo);
        glBindVertexArray(sparkVao);
        glBindBuffer(GL_ARRAY_BUFFER, sparkVbo);
        glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(sparkVertices.size() * sizeof(GasFactorySparkVertex)), sparkVertices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GasFactorySparkVertex), (void *)offsetof(GasFactorySparkVertex, origin));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(GasFactorySparkVertex), (void *)offsetof(GasFactorySparkVertex, params));
        glBindVertexArray(0);
    }

    void renderGasFactory(const glm::mat4 &cameraMatrix, const glm::mat4 &projectionMatrix, float timeSeconds)
    {
        if (!generated)
        {
            buildFactoryMesh();
            buildSmokeMesh();
            buildSparkMesh();
        }

        const glm::mat4 viewMatrix = glm::inverse(cameraMatrix);
        const glm::vec3 cameraPos = glm::vec3(cameraMatrix[3]);
        const float tileOffsets[2] = {-scrollZ, -scrollZ + kCycleM};

        glUseProgram(shaderId);
        glUniformMatrix4fv(glGetUniformLocation(shaderId, "u_worldToView"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shaderId, "u_projection"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
        glUniform3fv(glGetUniformLocation(shaderId, "u_cameraPos"), 1, glm::value_ptr(cameraPos));
        glUniform1f(glGetUniformLocation(shaderId, "u_time"), timeSeconds);
        glBindVertexArray(vao);
        for (float zOffset : tileOffsets)
        {
            const glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, zOffset));
            glUniformMatrix4fv(glGetUniformLocation(shaderId, "u_modelToWorld"), 1, GL_FALSE, glm::value_ptr(model));
            glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
        }
        glBindVertexArray(0);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glUseProgram(smokeShaderId);
        glUniformMatrix4fv(glGetUniformLocation(smokeShaderId, "u_worldToView"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(glGetUniformLocation(smokeShaderId, "u_projection"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
        glUniform1f(glGetUniformLocation(smokeShaderId, "u_time"), timeSeconds);
        glBindVertexArray(smokeVao);
        for (float zOffset : tileOffsets)
        {
            const glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, zOffset));
            glUniformMatrix4fv(glGetUniformLocation(smokeShaderId, "u_modelToWorld"), 1, GL_FALSE, glm::value_ptr(model));
            glDrawElements(GL_TRIANGLES, smokeIndexCount, GL_UNSIGNED_INT, 0);
        }
        glBindVertexArray(0);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glUseProgram(sparkShaderId);
        glUniformMatrix4fv(glGetUniformLocation(sparkShaderId, "u_worldToView"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(glGetUniformLocation(sparkShaderId, "u_projection"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
        glUniform1f(glGetUniformLocation(sparkShaderId, "u_time"), timeSeconds);
        glBindVertexArray(sparkVao);
        for (float zOffset : tileOffsets)
        {
            const glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, zOffset));
            glUniformMatrix4fv(glGetUniformLocation(sparkShaderId, "u_modelToWorld"), 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_POINTS, 0, sparkCount);
        }
        glBindVertexArray(0);

        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
    }
};

const char *GasFactoryBiome::FACTORY_VERTEX_SHADER = GLSL_VERSION R"(
    precision highp float;
    layout(location = 0) in vec3 a_pos;
    layout(location = 1) in vec3 a_normal;
    layout(location = 2) in vec4 a_color;
    layout(location = 3) in float a_cargoCenterX;
    uniform mat4 u_modelToWorld;
    uniform mat4 u_worldToView;
    uniform mat4 u_projection;
    uniform float u_time;
    out vec3 v_worldPos;
    out vec3 v_normal;
    out vec4 v_color;
    out float v_isCargo;
    void main()
    {
        vec3 pos = a_pos;
        v_isCargo = a_color.a > 1.5 ? 1.0 : 0.0;
        if (a_color.a > 1.5)
        {
            float belt = floor((pos.z + 43.0) / 76.0);
            float dir = (a_color.a < 2.5) ? 1.0 : -1.0;
            float speed = 2.6 + mod(belt * 1.3, 1.6);
            float phase = fract(sin((belt + 3.0) * 17.17) * 43758.5453) * 170.0;
            float wrapped = mod(a_cargoCenterX * dir + u_time * speed + phase + 85.0, 170.0) - 85.0;
            pos.x = wrapped * dir + (a_pos.x - a_cargoCenterX);
        }
        vec4 worldPos = u_modelToWorld * vec4(pos, 1.0);
        v_worldPos = worldPos.xyz;
        v_normal = normalize(mat3(u_modelToWorld) * a_normal);
        v_color = vec4(a_color.rgb, 1.0);
        gl_Position = u_projection * u_worldToView * worldPos;
    }
)";

const char *GasFactoryBiome::FACTORY_FRAGMENT_SHADER = GLSL_VERSION R"(
    precision highp float;
    in vec3 v_worldPos;
    in vec3 v_normal;
    in vec4 v_color;
    in float v_isCargo;
    uniform vec3 u_cameraPos;
    uniform float u_time;
    out vec4 FragColor;
    float hash21(vec2 p)
    {
        p = fract(p * vec2(123.34, 456.21));
        p += dot(p, p + 45.32);
        return fract(p.x * p.y);
    }
    void main()
    {
        if (v_isCargo > 0.5 && v_worldPos.z < -36.0)
            discard;
        vec3 n = normalize(v_normal);
        vec3 lightDir = normalize(vec3(-0.32, 0.86, -0.18));
        float diffuse = clamp(dot(n, lightDir), 0.0, 1.0);
        float rim = pow(1.0 - clamp(dot(n, normalize(u_cameraPos - v_worldPos)), 0.0, 1.0), 2.4);
        float grime = hash21(floor(v_worldPos.xz * 0.22)) * 0.12;
        float beacon = smoothstep(0.92, 1.0, sin(u_time * 3.4 + floor(v_worldPos.z * 0.06)) * 0.5 + 0.5);
        vec3 color = v_color.rgb * (0.38 + diffuse * 0.62 - grime);
        color += vec3(0.16, 0.24, 0.22) * rim;
        color += vec3(0.62, 0.95, 0.70) * beacon * step(10.0, v_worldPos.y) * 0.08;
        float fogT = smoothstep(110.0, 390.0, v_worldPos.z);
        color = mix(color, vec3(0.11, 0.16, 0.145), fogT * 0.52);
        FragColor = vec4(color, 1.0);
    }
)";

const char *GasFactoryBiome::SMOKE_VERTEX_SHADER = GLSL_VERSION R"(
    precision highp float;
    layout(location = 0) in vec3 a_pos;
    layout(location = 1) in vec2 a_uv;
    layout(location = 2) in vec4 a_params;
    uniform mat4 u_modelToWorld;
    uniform mat4 u_worldToView;
    uniform mat4 u_projection;
    uniform float u_time;
    out vec2 v_uv;
    out float v_alpha;
    out float v_seed;
    void main()
    {
        float phase = a_params.x;
        float baseAge = fract(phase);
        float seed = floor(phase);
        float age = fract(baseAge + u_time * 0.085);
        float fade = smoothstep(0.0, 0.16, age) * (1.0 - smoothstep(0.72, 1.0, age));
        float grow = smoothstep(0.0, 1.0, age);
        vec2 centeredUv = a_uv * 2.0 - 1.0;
        vec3 pos = a_pos;
        pos.x += centeredUv.x * a_params.y * (0.30 + grow * 1.05);
        pos.y += centeredUv.y * a_params.z * (0.30 + grow * 1.10);
        pos.x += sin(u_time * 0.22 + seed) * 1.8 + grow * 8.5;
        pos.y += grow * 43.0;
        pos.z += sin(u_time * 0.14 + seed * 1.7) * 1.4;
        vec4 worldPos = u_modelToWorld * vec4(pos, 1.0);
        v_uv = a_uv;
        v_alpha = fade * a_params.w;
        v_seed = phase;
        gl_Position = u_projection * u_worldToView * worldPos;
    }
)";

const char *GasFactoryBiome::SMOKE_FRAGMENT_SHADER = GLSL_VERSION R"(
    precision highp float;
    in vec2 v_uv;
    in float v_alpha;
    in float v_seed;
    out vec4 FragColor;
    float hash21(vec2 p)
    {
        p = fract(p * vec2(123.34, 456.21));
        p += dot(p, p + 45.32);
        return fract(p.x * p.y);
    }
    float noise(vec2 p)
    {
        vec2 i = floor(p);
        vec2 f = fract(p);
        f = f * f * (3.0 - 2.0 * f);
        float a = hash21(i);
        float b = hash21(i + vec2(1.0, 0.0));
        float c = hash21(i + vec2(0.0, 1.0));
        float d = hash21(i + vec2(1.0, 1.0));
        return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
    }
    void main()
    {
        vec2 p = v_uv * 2.0 - 1.0;
        float d = length(p);
        float cloud = 1.0 - smoothstep(0.42, 1.0, d);
        float n = noise(v_uv * 5.0 + v_seed);
        float alpha = cloud * smoothstep(0.20, 0.82, n) * v_alpha * 0.58;
        FragColor = vec4(vec3(0.52, 0.56, 0.54), alpha);
    }
)";

const char *GasFactoryBiome::SPARK_VERTEX_SHADER = GLSL_VERSION R"(
    precision highp float;
    layout(location = 0) in vec3 a_origin;
    layout(location = 1) in vec4 a_params;
    uniform mat4 u_modelToWorld;
    uniform mat4 u_worldToView;
    uniform mat4 u_projection;
    uniform float u_time;
    out float v_alpha;
    out float v_heat;
    void main()
    {
        float phase = fract(a_params.x + u_time * 0.18);
        float sparkActive = smoothstep(0.0, 0.035, phase) * (1.0 - smoothstep(0.36, 0.58, phase));
        float fallPhase = clamp(phase / 0.58, 0.0, 1.0);
        float fall = fallPhase * fallPhase;
        vec3 pos = a_origin;
        pos.x += sin(u_time * 3.1 + a_params.x * 37.0) * 0.16 + a_params.z * fall;
        pos.y -= a_params.y * fall;
        pos.z += cos(u_time * 2.4 + a_params.x * 29.0) * 0.10;
        vec4 worldPos = u_modelToWorld * vec4(pos, 1.0);
        vec4 viewPos = u_worldToView * worldPos;
        float depthScale = clamp(72.0 / max(18.0, -viewPos.z), 0.35, 1.6);
        gl_Position = u_projection * viewPos;
        gl_PointSize = a_params.w * depthScale;
        v_alpha = sparkActive * (0.95 + 0.35 * (1.0 - fallPhase));
        v_heat = 1.0 - smoothstep(0.10, 0.90, fallPhase);
    }
)";

const char *GasFactoryBiome::SPARK_FRAGMENT_SHADER = GLSL_VERSION R"(
    precision highp float;
    in float v_alpha;
    in float v_heat;
    out vec4 FragColor;
    void main()
    {
        vec2 p = gl_PointCoord * 2.0 - 1.0;
        float d = length(p);
        float core = 1.0 - smoothstep(0.0, 0.36, d);
        float glow = 1.0 - smoothstep(0.18, 1.0, d);
        float alpha = (core * 1.20 + glow * 0.62) * v_alpha;
        if (alpha < 0.015)
            discard;
        vec3 hot = vec3(1.0, 0.82, 0.42);
        vec3 cool = vec3(0.55, 0.82, 1.0);
        FragColor = vec4(mix(cool, hot, v_heat), alpha);
    }
)";
