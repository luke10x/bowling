#pragma once

#include "framework/gl_header.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <cmath>

#include "framework/gl_util.h"

struct AshlandTerrainVertex
{
    glm::vec3 position;
    glm::vec3 normal;
};

struct AshlandSmokeVertex
{
    glm::vec3 position;
    glm::vec2 uv;
    glm::vec4 params;
};

struct AshlandTerrain
{
    static const char *ASHLAND_VERTEX_SHADER;
    static const char *ASHLAND_FRAGMENT_SHADER;
    static const char *LAVA_VERTEX_SHADER;
    static const char *LAVA_FRAGMENT_SHADER;
    static const char *SMOKE_VERTEX_SHADER;
    static const char *SMOKE_FRAGMENT_SHADER;

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLuint shaderId = 0;
    GLuint lavaVao = 0;
    GLuint lavaVbo = 0;
    GLuint lavaEbo = 0;
    GLuint lavaShaderId = 0;
    GLuint smokeVao = 0;
    GLuint smokeVbo = 0;
    GLuint smokeEbo = 0;
    GLuint smokeShaderId = 0;
    GLsizei indexCount = 0;
    GLsizei lavaIndexCount = 0;
    GLsizei smokeIndexCount = 0;
    float scrollZ = 0.0f;

    std::vector<AshlandTerrainVertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<AshlandTerrainVertex> lavaVertices;
    std::vector<uint32_t> lavaIndices;
    std::vector<AshlandSmokeVertex> smokeVertices;
    std::vector<uint32_t> smokeIndices;
    bool generated = false;

    static constexpr int kGridX = 112;
    static constexpr int kGridZ = 208;
    static constexpr float kHalfWidthMeters = 125.0f;
    static constexpr float kNearZ = -70.0f;
    static constexpr float kFarZ = 460.0f;
    static constexpr float kBaseY = -27.0f;
    static constexpr float kMinY = -37.0f;
    static constexpr float kMaxY = -10.0f;
    static constexpr float kScrollSpeed = 1.75f;
    static constexpr float kScrollCycleMeters = kFarZ - kNearZ;

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
        uint32_t h = hash32(uint32_t(x) * 73856093U ^ uint32_t(z) * 19349663U ^ 0xa51a9d13U);
        return float(h & 0x00ffffffU) / float(0x01000000U);
    }

    static float smoothstep01(float t)
    {
        t = glm::clamp(t, 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    static float valueNoise(float x, float z)
    {
        int ix = int(std::floor(x));
        int iz = int(std::floor(z));
        float fx = smoothstep01(x - float(ix));
        float fz = smoothstep01(z - float(iz));

        float a = hash01(ix, iz);
        float b = hash01(ix + 1, iz);
        float c = hash01(ix, iz + 1);
        float d = hash01(ix + 1, iz + 1);

        return glm::mix(glm::mix(a, b, fx), glm::mix(c, d, fx), fz);
    }

    static glm::vec2 periodicZ(float z, float radius, float phase)
    {
        constexpr float kTau = 6.28318530718f;
        const float z01 = (z - kNearZ) / kScrollCycleMeters;
        const float theta = z01 * kTau + phase;
        return glm::vec2(std::cos(theta), std::sin(theta)) * radius;
    }

    static float periodicFbm(float x, float z, float xScale, float zRadius, float phaseX, float phaseY)
    {
        float sum = 0.0f;
        float amp = 0.5f;
        float freq = 1.0f;
        for (int i = 0; i < 5; ++i)
        {
            const glm::vec2 ring = periodicZ(z, zRadius * freq, phaseY + float(i) * 0.67f);
            sum += valueNoise(x * xScale * freq + ring.x + phaseX, ring.y + phaseY) * amp;
            freq *= 2.0f;
            amp *= 0.5f;
        }
        return sum;
    }

    static float lavaField(float x, float z)
    {
        const float warpA = periodicFbm(x, z, 0.012f, 13.0f, 91.0f, 37.0f);
        const float warpB = periodicFbm(x, z, 0.018f, 17.0f, 229.0f, 151.0f);
        const float warpedX = x + (warpA - 0.5f) * 42.0f;
        const float warpedZ = z + (warpB - 0.5f) * 34.0f;
        const float veins = std::abs(periodicFbm(warpedX, warpedZ, 0.024f, 20.0f, 617.0f, 773.0f) - 0.5f);
        return 1.0f - glm::smoothstep(0.012f, 0.055f, veins);
    }

    static float landHeightBeforeRiverCut(float x, float z)
    {
        const float broad = periodicFbm(x, z, 0.009f, 8.0f, 23.0f, 41.0f);
        const float rock = periodicFbm(x, z, 0.032f, 18.0f, 311.0f, 109.0f);
        const float cinder = periodicFbm(x, z, 0.078f, 26.0f, 547.0f, 331.0f);

        const float sideRise = glm::smoothstep(34.0f, 78.0f, std::abs(x)) * 7.0f;
        const float ridgeLift = glm::max(0.0f, broad - 0.40f) * 13.0f;
        const float crackedRoll = (rock - 0.5f) * 4.8f;
        const float ashPebble = (cinder - 0.5f) * 1.2f;
        return kBaseY + sideRise + ridgeLift + crackedRoll + ashPebble;
    }

    static float riverCutAt(float x, float z)
    {
        const float lava = lavaField(x, z);
        return lava * glm::mix(5.5f, 11.0f, periodicFbm(x, z, 0.015f, 12.0f, 811.0f, 977.0f));
    }

    static float heightAt(float x, float z)
    {
        const float base = landHeightBeforeRiverCut(x, z);
        const float riverCut = riverCutAt(x, z);
        return glm::clamp(base - riverCut, kMinY, kMaxY);
    }

    static float lavaSurfaceHeightAt(float x, float z)
    {
        const float base = landHeightBeforeRiverCut(x, z);
        const float riverCut = riverCutAt(x, z);

        const float broad = periodicFbm(x, z, 0.006f, 8.0f, 1201.0f, 1301.0f);
        const float slowRoll = (broad - 0.5f) * 0.45f;
        return glm::clamp(base - riverCut * 0.72f + 0.18f + slowRoll, kMinY + 0.4f, kMaxY);
    }

    void loadAshlandShader()
    {
        this->shaderId = vtx::createShaderProgram(ASHLAND_VERTEX_SHADER, ASHLAND_FRAGMENT_SHADER);
        this->lavaShaderId = vtx::createShaderProgram(LAVA_VERTEX_SHADER, LAVA_FRAGMENT_SHADER);
        this->smokeShaderId = vtx::createShaderProgram(SMOKE_VERTEX_SHADER, SMOKE_FRAGMENT_SHADER);
    }

    void initAshland()
    {
        this->scrollZ = 0.0f;
        this->loadAshlandShader();
        this->buildTerrainMesh();
        this->buildLavaMesh();
        this->buildSmokeMesh();
    }

    void update(float deltaTime)
    {
        this->scrollZ += deltaTime * kScrollSpeed;
        if (this->scrollZ > kScrollCycleMeters)
            this->scrollZ = std::fmod(this->scrollZ, kScrollCycleMeters);
    }

    void buildTerrainMesh()
    {
        this->vertices.clear();
        this->indices.clear();
        this->vertices.reserve((kGridX + 1) * (kGridZ + 1));
        this->indices.reserve(kGridX * kGridZ * 6);

        const float dx = (2.0f * kHalfWidthMeters) / float(kGridX);
        const float dz = (kFarZ - kNearZ) / float(kGridZ);

        for (int z = 0; z <= kGridZ; ++z)
        {
            const float worldZ = glm::mix(kNearZ, kFarZ, float(z) / float(kGridZ));
            for (int x = 0; x <= kGridX; ++x)
            {
                const float worldX = glm::mix(-kHalfWidthMeters, kHalfWidthMeters, float(x) / float(kGridX));
                const float h = heightAt(worldX, worldZ);
                const float hx0 = heightAt(worldX - dx, worldZ);
                const float hx1 = heightAt(worldX + dx, worldZ);
                const float hz0 = heightAt(worldX, worldZ - dz);
                const float hz1 = heightAt(worldX, worldZ + dz);
                const glm::vec3 tangentX = glm::vec3(2.0f * dx, hx1 - hx0, 0.0f);
                const glm::vec3 tangentZ = glm::vec3(0.0f, hz1 - hz0, 2.0f * dz);
                const glm::vec3 normal = glm::normalize(glm::cross(tangentZ, tangentX));
                this->vertices.push_back({glm::vec3(worldX, h, worldZ), normal});
            }
        }

        const int stride = kGridX + 1;
        for (int z = 0; z < kGridZ; ++z)
        {
            for (int x = 0; x < kGridX; ++x)
            {
                const uint32_t i0 = uint32_t(z * stride + x);
                const uint32_t i1 = i0 + 1;
                const uint32_t i2 = uint32_t((z + 1) * stride + x);
                const uint32_t i3 = i2 + 1;
                this->indices.push_back(i0);
                this->indices.push_back(i2);
                this->indices.push_back(i1);
                this->indices.push_back(i1);
                this->indices.push_back(i2);
                this->indices.push_back(i3);
            }
        }

        this->indexCount = GLsizei(this->indices.size());
        if (this->ebo != 0) glDeleteBuffers(1, &this->ebo);
        if (this->vbo != 0) glDeleteBuffers(1, &this->vbo);
        if (this->vao != 0) glDeleteVertexArrays(1, &this->vao);

        glGenVertexArrays(1, &this->vao);
        glGenBuffers(1, &this->vbo);
        glGenBuffers(1, &this->ebo);

        glBindVertexArray(this->vao);
        glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
        glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(this->vertices.size() * sizeof(AshlandTerrainVertex)), this->vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(this->indices.size() * sizeof(uint32_t)), this->indices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(AshlandTerrainVertex), (void *)offsetof(AshlandTerrainVertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(AshlandTerrainVertex), (void *)offsetof(AshlandTerrainVertex, normal));
        glBindVertexArray(0);
        this->generated = true;
        checkOpenGLError("ashland terrain init");
    }

    void buildLavaMesh()
    {
        this->lavaVertices.clear();
        this->lavaIndices.clear();
        this->lavaVertices.reserve((kGridX + 1) * (kGridZ + 1));
        this->lavaIndices.reserve(kGridX * kGridZ * 6);

        for (int z = 0; z <= kGridZ; ++z)
        {
            const float worldZ = glm::mix(kNearZ, kFarZ, float(z) / float(kGridZ));
            for (int x = 0; x <= kGridX; ++x)
            {
                const float worldX = glm::mix(-kHalfWidthMeters, kHalfWidthMeters, float(x) / float(kGridX));
                this->lavaVertices.push_back({
                    glm::vec3(worldX, lavaSurfaceHeightAt(worldX, worldZ), worldZ),
                    glm::vec3(0.0f, 1.0f, 0.0f)
                });
            }
        }

        const int stride = kGridX + 1;
        for (int z = 0; z < kGridZ; ++z)
        {
            for (int x = 0; x < kGridX; ++x)
            {
                const uint32_t i0 = uint32_t(z * stride + x);
                const uint32_t i1 = i0 + 1;
                const uint32_t i2 = uint32_t((z + 1) * stride + x);
                const uint32_t i3 = i2 + 1;
                this->lavaIndices.push_back(i0);
                this->lavaIndices.push_back(i2);
                this->lavaIndices.push_back(i1);
                this->lavaIndices.push_back(i1);
                this->lavaIndices.push_back(i2);
                this->lavaIndices.push_back(i3);
            }
        }

        this->lavaIndexCount = GLsizei(this->lavaIndices.size());
        if (this->lavaEbo != 0) glDeleteBuffers(1, &this->lavaEbo);
        if (this->lavaVbo != 0) glDeleteBuffers(1, &this->lavaVbo);
        if (this->lavaVao != 0) glDeleteVertexArrays(1, &this->lavaVao);

        glGenVertexArrays(1, &this->lavaVao);
        glGenBuffers(1, &this->lavaVbo);
        glGenBuffers(1, &this->lavaEbo);

        glBindVertexArray(this->lavaVao);
        glBindBuffer(GL_ARRAY_BUFFER, this->lavaVbo);
        glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(this->lavaVertices.size() * sizeof(AshlandTerrainVertex)), this->lavaVertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->lavaEbo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(this->lavaIndices.size() * sizeof(uint32_t)), this->lavaIndices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(AshlandTerrainVertex), (void *)offsetof(AshlandTerrainVertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(AshlandTerrainVertex), (void *)offsetof(AshlandTerrainVertex, normal));
        glBindVertexArray(0);
        checkOpenGLError("ashland lava init");
    }

    void buildSmokeMesh()
    {
        this->smokeVertices.clear();
        this->smokeIndices.clear();
        this->smokeVertices.reserve(52 * 8 * 4);
        this->smokeIndices.reserve(52 * 8 * 6);

        auto pushQuad = [&](const glm::vec3 &center, float width, float height, float phase, float softness)
        {
            const uint32_t base = uint32_t(this->smokeVertices.size());
            const glm::vec3 right(width * 0.5f, 0.0f, 0.0f);
            const glm::vec3 up(0.0f, height * 0.5f, 0.0f);
            const glm::vec4 params(phase, width, height, softness);
            this->smokeVertices.push_back({center - right - up, glm::vec2(0.0f, 0.0f), params});
            this->smokeVertices.push_back({center + right - up, glm::vec2(1.0f, 0.0f), params});
            this->smokeVertices.push_back({center + right + up, glm::vec2(1.0f, 1.0f), params});
            this->smokeVertices.push_back({center - right + up, glm::vec2(0.0f, 1.0f), params});
            this->smokeIndices.push_back(base + 0);
            this->smokeIndices.push_back(base + 1);
            this->smokeIndices.push_back(base + 2);
            this->smokeIndices.push_back(base + 0);
            this->smokeIndices.push_back(base + 2);
            this->smokeIndices.push_back(base + 3);
        };

        constexpr int kVentCount = 52;
        constexpr int kPuffsPerVent = 8;
        for (int vent = 0; vent < kVentCount; ++vent)
        {
            float x = 0.0f;
            float z = 0.0f;
            for (int attempt = 0; attempt < 18; ++attempt)
            {
                const float rx = hash01(vent * 29 + attempt * 7, 101);
                const float rz = hash01(vent * 43 + attempt * 11, 307);
                x = glm::mix(-kHalfWidthMeters * 0.86f, kHalfWidthMeters * 0.86f, rx);
                z = glm::mix(kNearZ + 18.0f, kFarZ - 28.0f, rz);
                if (lavaField(x, z) > 0.34f || attempt == 17)
                    break;
            }

            const float baseY = glm::max(heightAt(x, z), lavaSurfaceHeightAt(x, z)) + 3.0f;
            const float ventPhase = hash01(vent * 97, 701) * 12.0f;
            const float sideDrift = (hash01(vent * 13, 991) - 0.5f) * 2.4f;
            for (int puff = 0; puff < kPuffsPerVent; ++puff)
            {
                const float t = float(puff) / float(kPuffsPerVent - 1);
                const float phase = ventPhase + t * 1.7f;
                const float width = glm::mix(5.2f, 13.5f, t) * glm::mix(0.86f, 1.18f, hash01(vent * 31 + puff, 829));
                const float height = glm::mix(2.8f, 6.4f, t);
                const glm::vec3 center(
                    x + sideDrift * t + (hash01(vent * 17 + puff, 409) - 0.5f) * 1.6f,
                    baseY + 1.6f + t * 15.0f,
                    z + (hash01(vent * 23 + puff, 613) - 0.5f) * 2.3f
                );
                pushQuad(center, width, height, phase, glm::mix(0.56f, 0.78f, t));
            }
        }

        this->smokeIndexCount = GLsizei(this->smokeIndices.size());
        if (this->smokeEbo != 0) glDeleteBuffers(1, &this->smokeEbo);
        if (this->smokeVbo != 0) glDeleteBuffers(1, &this->smokeVbo);
        if (this->smokeVao != 0) glDeleteVertexArrays(1, &this->smokeVao);

        glGenVertexArrays(1, &this->smokeVao);
        glGenBuffers(1, &this->smokeVbo);
        glGenBuffers(1, &this->smokeEbo);

        glBindVertexArray(this->smokeVao);
        glBindBuffer(GL_ARRAY_BUFFER, this->smokeVbo);
        glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(this->smokeVertices.size() * sizeof(AshlandSmokeVertex)), this->smokeVertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->smokeEbo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(this->smokeIndices.size() * sizeof(uint32_t)), this->smokeIndices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(AshlandSmokeVertex), (void *)offsetof(AshlandSmokeVertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(AshlandSmokeVertex), (void *)offsetof(AshlandSmokeVertex, uv));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(AshlandSmokeVertex), (void *)offsetof(AshlandSmokeVertex, params));
        glBindVertexArray(0);
        checkOpenGLError("ashland smoke init");
    }

    void renderAshland(const glm::mat4 &cameraMatrix, const glm::mat4 &projectionMatrix, float timeSeconds)
    {
        if (!this->generated)
        {
            this->buildTerrainMesh();
            this->buildLavaMesh();
            this->buildSmokeMesh();
        }

        const glm::mat4 viewMatrix = glm::inverse(cameraMatrix);
        const glm::vec3 cameraPos = glm::vec3(cameraMatrix[3]);
        glUseProgram(this->shaderId);
        glUniformMatrix4fv(glGetUniformLocation(this->shaderId, "u_worldToView"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(glGetUniformLocation(this->shaderId, "u_projection"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
        glUniform3fv(glGetUniformLocation(this->shaderId, "u_cameraPos"), 1, glm::value_ptr(cameraPos));
        glUniform1f(glGetUniformLocation(this->shaderId, "u_time"), timeSeconds);

        glBindVertexArray(this->vao);
        const float tileOffsets[2] = {-this->scrollZ, -this->scrollZ + kScrollCycleMeters};
        for (float zOffset : tileOffsets)
        {
            const glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, zOffset));
            glUniformMatrix4fv(glGetUniformLocation(this->shaderId, "u_modelToWorld"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
            glDrawElements(GL_TRIANGLES, this->indexCount, GL_UNSIGNED_INT, 0);
        }
        glBindVertexArray(0);

        glUseProgram(this->lavaShaderId);
        glUniformMatrix4fv(glGetUniformLocation(this->lavaShaderId, "u_worldToView"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(glGetUniformLocation(this->lavaShaderId, "u_projection"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
        glUniform3fv(glGetUniformLocation(this->lavaShaderId, "u_cameraPos"), 1, glm::value_ptr(cameraPos));
        glUniform1f(glGetUniformLocation(this->lavaShaderId, "u_time"), timeSeconds);

        glBindVertexArray(this->lavaVao);
        for (float zOffset : tileOffsets)
        {
            const glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, zOffset));
            glUniformMatrix4fv(glGetUniformLocation(this->lavaShaderId, "u_modelToWorld"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
            glDrawElements(GL_TRIANGLES, this->lavaIndexCount, GL_UNSIGNED_INT, 0);
        }
        glBindVertexArray(0);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glUseProgram(this->smokeShaderId);
        glUniformMatrix4fv(glGetUniformLocation(this->smokeShaderId, "u_worldToView"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(glGetUniformLocation(this->smokeShaderId, "u_projection"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
        glUniform1f(glGetUniformLocation(this->smokeShaderId, "u_time"), timeSeconds);

        glBindVertexArray(this->smokeVao);
        for (float zOffset : tileOffsets)
        {
            const glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, zOffset));
            glUniformMatrix4fv(glGetUniformLocation(this->smokeShaderId, "u_modelToWorld"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
            glDrawElements(GL_TRIANGLES, this->smokeIndexCount, GL_UNSIGNED_INT, 0);
        }
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
    }
};

const char *AshlandTerrain::ASHLAND_VERTEX_SHADER = GLSL_VERSION R"(
    precision highp float;

    layout(location = 0) in vec3 a_pos;
    layout(location = 1) in vec3 a_normal;

    uniform mat4 u_modelToWorld;
    uniform mat4 u_worldToView;
    uniform mat4 u_projection;

    out vec3 v_worldPos;
    out vec3 v_normal;

    void main()
    {
        vec4 worldPos = u_modelToWorld * vec4(a_pos, 1.0);
        v_worldPos = worldPos.xyz;
        v_normal = normalize(mat3(u_modelToWorld) * a_normal);
        gl_Position = u_projection * u_worldToView * worldPos;
    }
)";

const char *AshlandTerrain::ASHLAND_FRAGMENT_SHADER = GLSL_VERSION R"(
    precision highp float;

    in vec3 v_worldPos;
    in vec3 v_normal;

    uniform vec3 u_cameraPos;
    uniform float u_time;

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

    float fbm(vec2 p)
    {
        float sum = 0.0;
        float amp = 0.5;
        for (int i = 0; i < 5; ++i)
        {
            sum += noise(p) * amp;
            p *= 2.03;
            amp *= 0.5;
        }
        return sum;
    }

    float lavaVein(vec2 p)
    {
        vec2 warp = vec2(fbm(p * 0.035 + vec2(7.1, 2.4)), fbm(p * 0.031 + vec2(19.7, 31.3)));
        vec2 q = p + (warp - 0.5) * 42.0;
        float veins = abs(fbm(q * 0.024 + vec2(61.7, 77.3)) - 0.5);
        return 1.0 - smoothstep(0.012, 0.055, veins);
    }

    void main()
    {
        vec3 normal = normalize(v_normal);
        vec3 lightDir = normalize(vec3(-0.28, 0.86, 0.24));
        vec3 viewDir = normalize(u_cameraPos - v_worldPos);

        float diffuse = clamp(dot(normal, lightDir), 0.0, 1.0);
        float fresnel = pow(1.0 - clamp(dot(normal, viewDir), 0.0, 1.0), 1.7);
        float slope = 1.0 - clamp(normal.y, 0.0, 1.0);

        float lava = lavaVein(v_worldPos.xz);
        float glow = lavaVein(v_worldPos.xz + vec2(5.0, 0.0)) * 0.32 +
                     lavaVein(v_worldPos.xz + vec2(-5.0, 0.0)) * 0.32 +
                     lavaVein(v_worldPos.xz + vec2(0.0, 5.0)) * 0.22 +
                     lavaVein(v_worldPos.xz + vec2(0.0, -5.0)) * 0.22;
        glow = clamp(max(glow, lava), 0.0, 1.0);

        vec3 ash = vec3(0.12, 0.105, 0.095);
        vec3 basalt = vec3(0.19, 0.17, 0.16);
        vec3 cooled = vec3(0.30, 0.20, 0.16);
        vec3 emberGlow = vec3(0.86, 0.08, 0.025);

        float heightT = smoothstep(-32.0, -13.0, v_worldPos.y);
        vec3 color = mix(ash, basalt, heightT);
        color = mix(color, cooled, slope * 0.62);
        color *= 0.46 + diffuse * 0.54;
        color += vec3(0.05, 0.025, 0.015) * fresnel;

        color += emberGlow * glow * 0.24;

        float fogT = smoothstep(125.0, 390.0, v_worldPos.z);
        color = mix(color, vec3(0.23, 0.16, 0.13), fogT * 0.46);

        FragColor = vec4(color, 1.0);
    }
)";

const char *AshlandTerrain::LAVA_VERTEX_SHADER = GLSL_VERSION R"(
    precision highp float;

    layout(location = 0) in vec3 a_pos;
    layout(location = 1) in vec3 a_normal;

    uniform mat4 u_modelToWorld;
    uniform mat4 u_worldToView;
    uniform mat4 u_projection;
    uniform float u_time;

    out vec3 v_worldPos;
    out vec3 v_normal;

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
        vec3 pos = a_pos;
        float waveA = noise(pos.xz * 0.055 + vec2(u_time * 0.08, -u_time * 0.035));
        float waveB = noise(pos.xz * 0.115 + vec2(-u_time * 0.045, u_time * 0.06));
        pos.y += (waveA - 0.5) * 0.18 + (waveB - 0.5) * 0.07;

        vec4 worldPos = u_modelToWorld * vec4(pos, 1.0);
        v_worldPos = worldPos.xyz;
        v_normal = normalize(mat3(u_modelToWorld) * a_normal);
        gl_Position = u_projection * u_worldToView * worldPos;
    }
)";

const char *AshlandTerrain::LAVA_FRAGMENT_SHADER = GLSL_VERSION R"(
    precision highp float;

    in vec3 v_worldPos;
    in vec3 v_normal;

    uniform vec3 u_cameraPos;
    uniform float u_time;

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

    float fbm(vec2 p)
    {
        float sum = 0.0;
        float amp = 0.5;
        for (int i = 0; i < 5; ++i)
        {
            sum += noise(p) * amp;
            p *= 2.03;
            amp *= 0.5;
        }
        return sum;
    }

    float lavaVein(vec2 p)
    {
        vec2 warp = vec2(fbm(p * 0.035 + vec2(7.1, 2.4)), fbm(p * 0.031 + vec2(19.7, 31.3)));
        vec2 q = p + (warp - 0.5) * 42.0;
        float veins = abs(fbm(q * 0.024 + vec2(61.7, 77.3)) - 0.5);
        return 1.0 - smoothstep(0.012, 0.055, veins);
    }

    void main()
    {
        float river = lavaVein(v_worldPos.xz);
        if (river < 0.14)
            discard;

        vec3 normal = normalize(v_normal);
        vec3 viewDir = normalize(u_cameraPos - v_worldPos);
        float fresnel = pow(1.0 - clamp(dot(normal, viewDir), 0.0, 1.0), 2.0);

        float crust = fbm(v_worldPos.xz * 0.16 + vec2(u_time * 0.05, -u_time * 0.035));
        float heat = smoothstep(0.20, 0.86, crust) * 0.28 + river * 0.34;
        vec3 deepRed = vec3(0.46, 0.015, 0.01);
        vec3 red = vec3(0.86, 0.035, 0.015);
        vec3 orangeCore = vec3(1.0, 0.22, 0.035);

        vec3 color = mix(deepRed, red, clamp(heat, 0.0, 1.0));
        color = mix(color, orangeCore, smoothstep(0.82, 1.0, heat) * 0.35);
        color += vec3(0.22, 0.02, 0.01) * fresnel;

        float edge = smoothstep(0.14, 0.30, river);
        color *= 0.62 + edge * 0.55;

        float fogT = smoothstep(125.0, 390.0, v_worldPos.z);
        color = mix(color, vec3(0.23, 0.06, 0.035), fogT * 0.30);

        FragColor = vec4(color, 1.0);
    }
)";

const char *AshlandTerrain::SMOKE_VERTEX_SHADER = GLSL_VERSION R"(
    precision highp float;

    layout(location = 0) in vec3 a_pos;
    layout(location = 1) in vec2 a_uv;
    layout(location = 2) in vec4 a_params;

    uniform mat4 u_modelToWorld;
    uniform mat4 u_worldToView;
    uniform mat4 u_projection;
    uniform float u_time;

    out vec2 v_uv;
    out vec2 v_flowUv;
    out float v_alpha;
    out float v_noiseSeed;

    void main()
    {
        float phase = a_params.x;
        float width = a_params.y;
        float height = a_params.z;
        float softness = a_params.w;
        float cycle = fract(u_time * 0.045 + phase);
        float rise = cycle * 5.8;
        float fadeIn = smoothstep(0.0, 0.10, cycle);
        float fadeOut = 1.0 - smoothstep(0.72, 1.0, cycle);
        vec2 windDir = normalize(vec2(1.0, -0.34));
        vec2 planeDrift = windDir * (cycle * 12.0);
        vec3 drift = vec3(
            planeDrift.x + sin(u_time * 0.38 + phase) * (0.42 + width * 0.055),
            rise,
            planeDrift.y + cos(u_time * 0.27 + phase * 1.7) * (0.32 + width * 0.030)
        );

        vec3 pos = a_pos + drift;
        float swing = sin(u_time * 0.58 + phase + a_uv.y * 2.4);
        pos.x += (a_uv.y - 0.5) * swing * (1.4 + width * 0.10);
        pos.z += (a_uv.y - 0.5) * cos(u_time * 0.43 + phase) * (0.6 + width * 0.045);
        pos.y += (a_uv.x - 0.5) * cos(u_time * 0.18 + phase) * 0.42;

        v_uv = a_uv;
        v_flowUv = (a_pos.xz + windDir * u_time * 18.0 + vec2(phase * 4.1, -phase * 2.7)) * 0.055;
        v_alpha = fadeIn * fadeOut * mix(0.48, 0.78, softness);
        v_noiseSeed = phase + height;
        gl_Position = u_projection * u_worldToView * u_modelToWorld * vec4(pos, 1.0);
    }
)";

const char *AshlandTerrain::SMOKE_FRAGMENT_SHADER = GLSL_VERSION R"(
    precision highp float;

    in vec2 v_uv;
    in vec2 v_flowUv;
    in float v_alpha;
    in float v_noiseSeed;

    uniform float u_time;

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
        vec2 centered = v_uv * 2.0 - 1.0;
        centered.x *= 0.78;
        centered.y *= 1.12;
        float d = length(centered);
        float softBody = 1.0 - smoothstep(0.08, 1.18, d);
        float feather = 1.0 - smoothstep(0.46, 1.18, d);
        float mottled = noise(v_uv * 2.0 + v_flowUv + vec2(v_noiseSeed, 0.0));
        float wisps = noise(v_uv * 5.8 + v_flowUv * 1.7 + vec2(v_noiseSeed * 0.41, 3.7));
        float cloud = softBody * feather;
        cloud *= mix(0.42, 0.92, mottled);
        cloud *= mix(0.70, 1.0, wisps);
        float alpha = cloud * v_alpha * 0.72;
        if (alpha < 0.006)
            discard;

        vec3 smoke = mix(vec3(0.46, 0.44, 0.41), vec3(0.82, 0.80, 0.74), smoothstep(0.0, 1.0, v_uv.y));
        FragColor = vec4(smoke, alpha);
    }
)";
