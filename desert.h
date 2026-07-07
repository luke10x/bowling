#pragma once

#include "framework/gl_header.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <cmath>

#include "framework/gl_util.h"

struct DesertTerrainVertex
{
    glm::vec3 position;
    glm::vec3 normal;
};

struct DesertTerrain
{
    static const char *DESERT_VERTEX_SHADER;
    static const char *DESERT_FRAGMENT_SHADER;

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLuint shaderId = 0;
    GLsizei indexCount = 0;
    float scrollZ = 0.0f;

    std::vector<DesertTerrainVertex> vertices;
    std::vector<uint32_t> indices;
    bool generated = false;

    static constexpr int kGridX = 96;
    static constexpr int kGridZ = 192;
    static constexpr float kHalfWidthMeters = 125.0f;
    static constexpr float kNearZ = -70.0f;
    static constexpr float kFarZ = 460.0f;
    static constexpr float kBaseY = -24.0f;
    static constexpr float kMinY = -35.0f;
    static constexpr float kMaxY = -9.0f;
    static constexpr float kScrollSpeed = 1.6f;
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
        uint32_t h = hash32(uint32_t(x) * 73856093U ^ uint32_t(z) * 19349663U ^ 0x19c4d52bU);
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

        float ab = glm::mix(a, b, fx);
        float cd = glm::mix(c, d, fx);
        return glm::mix(ab, cd, fz);
    }

    static float fbm(float x, float z)
    {
        float sum = 0.0f;
        float amp = 0.5f;
        float freq = 1.0f;
        for (int i = 0; i < 5; ++i)
        {
            sum += valueNoise(x * freq, z * freq) * amp;
            freq *= 2.03f;
            amp *= 0.5f;
        }
        return sum;
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
            const glm::vec2 ring = periodicZ(z, zRadius * freq, phaseY + float(i) * 0.61f);
            sum += valueNoise(x * xScale * freq + ring.x + phaseX, ring.y + phaseY) * amp;
            freq *= 2.0f;
            amp *= 0.5f;
        }
        return sum;
    }

    static float canyonField(float x, float z)
    {
        float nx = periodicFbm(x, z, 0.010f, 10.0f, 77.0f, 19.0f);
        float nz = periodicFbm(x, z, 0.010f, 12.0f, 133.0f, 91.0f);
        float warpedX = x + (nx - 0.5f) * 22.0f;
        float warpedZ = z + (nz - 0.5f) * 22.0f;
        float veins = std::abs(periodicFbm(warpedX, warpedZ, 0.020f, 16.0f, 401.0f, 503.0f) - 0.5f);
        return 1.0f - glm::smoothstep(0.02f, 0.09f, veins);
    }

    static float heightAt(float x, float z)
    {
        const float broad = periodicFbm(x, z, 0.008f, 8.0f, 11.0f, 23.0f);
        const float dunes = periodicFbm(x, z, 0.022f, 14.0f, 57.0f, 89.0f);
        const float ripple = periodicFbm(x, z, 0.065f, 24.0f, 141.0f, 177.0f);
        const float canyon = canyonField(x, z);

        const float sideRise = glm::smoothstep(42.0f, 82.0f, std::abs(x)) * 4.0f;
        const float duneLift = glm::max(0.0f, broad - 0.44f) * 6.5f;
        const float duneShape = (dunes - 0.5f) * 3.4f;
        const float fineRipple = (ripple - 0.5f) * 0.8f;
        const float canyonCut = canyon * glm::mix(6.0f, 13.5f, periodicFbm(x, z, 0.014f, 11.0f, 811.0f, 977.0f));

        float h = kBaseY + sideRise + duneLift + duneShape + fineRipple - canyonCut;
        return glm::clamp(h, kMinY, kMaxY);
    }

    void loadDesertShader()
    {
        this->shaderId = vtx::createShaderProgram(DESERT_VERTEX_SHADER, DESERT_FRAGMENT_SHADER);
    }

    void initDesert()
    {
        this->scrollZ = 0.0f;
        this->loadDesertShader();
        this->buildTerrainMesh();
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
            const float zT = float(z) / float(kGridZ);
            const float worldZ = glm::mix(kNearZ, kFarZ, zT);
            for (int x = 0; x <= kGridX; ++x)
            {
                const float xT = float(x) / float(kGridX);
                const float worldX = glm::mix(-kHalfWidthMeters, kHalfWidthMeters, xT);
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

        if (this->ebo != 0)
        {
            glDeleteBuffers(1, &this->ebo);
            this->ebo = 0;
        }
        if (this->vbo != 0)
        {
            glDeleteBuffers(1, &this->vbo);
            this->vbo = 0;
        }
        if (this->vao != 0)
        {
            glDeleteVertexArrays(1, &this->vao);
            this->vao = 0;
        }

        glGenVertexArrays(1, &this->vao);
        glGenBuffers(1, &this->vbo);
        glGenBuffers(1, &this->ebo);

        glBindVertexArray(this->vao);
        glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
        glBufferData(
            GL_ARRAY_BUFFER,
            GLsizeiptr(this->vertices.size() * sizeof(DesertTerrainVertex)),
            this->vertices.data(),
            GL_STATIC_DRAW
        );
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ebo);
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            GLsizeiptr(this->indices.size() * sizeof(uint32_t)),
            this->indices.data(),
            GL_STATIC_DRAW
        );

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(DesertTerrainVertex), (void *)offsetof(DesertTerrainVertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(DesertTerrainVertex), (void *)offsetof(DesertTerrainVertex, normal));

        glBindVertexArray(0);
        this->generated = true;
        checkOpenGLError("desert terrain init");
    }

    void renderDesert(const glm::mat4 &cameraMatrix, const glm::mat4 &projectionMatrix)
    {
        if (!this->generated)
            this->buildTerrainMesh();

        const glm::mat4 viewMatrix = glm::inverse(cameraMatrix);
        const glm::vec3 cameraPos = glm::vec3(cameraMatrix[3]);
        glUseProgram(this->shaderId);
        glUniformMatrix4fv(glGetUniformLocation(this->shaderId, "u_worldToView"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(glGetUniformLocation(this->shaderId, "u_projection"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
        glUniform3fv(glGetUniformLocation(this->shaderId, "u_cameraPos"), 1, glm::value_ptr(cameraPos));

        glBindVertexArray(this->vao);
        const float tileOffsets[2] = {
            -this->scrollZ,
            -this->scrollZ + kScrollCycleMeters,
        };
        for (float zOffset : tileOffsets)
        {
            const glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, zOffset));
            glUniformMatrix4fv(glGetUniformLocation(this->shaderId, "u_modelToWorld"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
            glDrawElements(GL_TRIANGLES, this->indexCount, GL_UNSIGNED_INT, 0);
        }
        glBindVertexArray(0);
    }
};

const char *DesertTerrain::DESERT_VERTEX_SHADER = GLSL_VERSION R"(
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

const char *DesertTerrain::DESERT_FRAGMENT_SHADER = GLSL_VERSION R"(
    precision highp float;

    in vec3 v_worldPos;
    in vec3 v_normal;
    uniform vec3 u_cameraPos;
    out vec4 FragColor;

    float canyonTint(float y)
    {
        return smoothstep(-33.0, -21.0, y);
    }

    void main()
    {
        vec3 normal = normalize(v_normal);
        vec3 lightDir = normalize(vec3(-0.34, 0.90, 0.20));
        vec3 viewDir = normalize(u_cameraPos - v_worldPos);

        float diffuse = clamp(dot(normal, lightDir), 0.0, 1.0);
        float fresnel = pow(1.0 - clamp(dot(normal, viewDir), 0.0, 1.0), 1.8);
        float slope = 1.0 - clamp(normal.y, 0.0, 1.0);

        vec3 sand = vec3(0.78, 0.61, 0.22);
        vec3 amber = vec3(0.86, 0.69, 0.28);
        vec3 canyon = vec3(0.46, 0.18, 0.15);
        vec3 bordo = vec3(0.34, 0.08, 0.11);

        float duneT = smoothstep(-28.0, -14.0, v_worldPos.y);
        vec3 color = mix(sand, amber, duneT);
        color = mix(color, canyon, slope * 0.58);
        color = mix(color, bordo, (1.0 - canyonTint(v_worldPos.y)) * 0.48);
        color *= 0.58 + diffuse * 0.62;
        color += vec3(0.08, 0.05, 0.02) * fresnel;

        float fogT = smoothstep(130.0, 380.0, v_worldPos.z);
        color = mix(color, vec3(0.70, 0.50, 0.28), fogT * 0.42);

        FragColor = vec4(color, 1.0);
    }
)";
