#pragma once

#include "framework/gl_header.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <cmath>

#include "framework/gl_util.h"

struct ForestTerrainVertex
{
    glm::vec3 position;
    glm::vec3 normal;
};

struct ForestTreeVertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
};

struct ForestTerrain
{
    static const char *FOREST_VERTEX_SHADER;
    static const char *FOREST_FRAGMENT_SHADER;
    static const char *TREE_VERTEX_SHADER;
    static const char *TREE_FRAGMENT_SHADER;

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLuint shaderId = 0;
    GLuint treeVao = 0;
    GLuint treeVbo = 0;
    GLuint treeEbo = 0;
    GLuint treeShaderId = 0;
    GLsizei indexCount = 0;
    GLsizei treeIndexCount = 0;
    float scrollZ = 0.0f;

    std::vector<ForestTerrainVertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<ForestTreeVertex> treeVertices;
    std::vector<uint32_t> treeIndices;
    bool generated = false;

    static constexpr int kGridX = 96;
    static constexpr int kGridZ = 192;
    static constexpr float kHalfWidthMeters = 125.0f;
    static constexpr float kNearZ = -70.0f;
    static constexpr float kFarZ = 460.0f;
    static constexpr float kBaseY = -30.0f;
    static constexpr float kMinY = -35.0f;
    static constexpr float kMaxY = -15.0f;
    static constexpr float kScrollSpeed = 2.2f;
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
        uint32_t h = hash32(uint32_t(x) * 73856093U ^ uint32_t(z) * 19349663U ^ 0x4f1bbcdcU);
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
            freq *= 2.01f;
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
            const glm::vec2 ring = periodicZ(z, zRadius * freq, phaseY + float(i) * 0.73f);
            sum += valueNoise(x * xScale * freq + ring.x + phaseX, ring.y + phaseY) * amp;
            freq *= 2.0f;
            amp *= 0.5f;
        }
        return sum;
    }

    static float heightAt(float x, float z)
    {
        const float broad = periodicFbm(x, z, 0.010f, 9.0f, 13.0f, 37.0f);
        const float detail = periodicFbm(x, z, 0.028f, 16.0f, 97.0f, 11.0f);
        const float pondNoise = periodicFbm(x, z, 0.016f, 11.0f, 401.0f, 503.0f);

        const float sideRise = glm::smoothstep(32.0f, 72.0f, std::abs(x)) * 8.5f;
        const float mountainScatter = glm::max(0.0f, broad - 0.42f) * 16.0f;
        const float gentleRoll = (detail - 0.5f) * 3.8f;
        const float ponds = glm::smoothstep(0.0f, 0.22f, 0.34f - pondNoise) * 4.5f;

        float h = kBaseY + sideRise + mountainScatter + gentleRoll - ponds;
        return glm::clamp(h, kMinY, kMaxY);
    }

    void loadForestShader()
    {
        this->shaderId = vtx::createShaderProgram(FOREST_VERTEX_SHADER, FOREST_FRAGMENT_SHADER);
        this->treeShaderId = vtx::createShaderProgram(TREE_VERTEX_SHADER, TREE_FRAGMENT_SHADER);
    }

    void initForest()
    {
        this->scrollZ = 0.0f;
        this->loadForestShader();
        this->buildTerrainMesh();
        this->buildTreeMesh();
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
            GLsizeiptr(this->vertices.size() * sizeof(ForestTerrainVertex)),
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
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ForestTerrainVertex), (void *)offsetof(ForestTerrainVertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ForestTerrainVertex), (void *)offsetof(ForestTerrainVertex, normal));

        glBindVertexArray(0);
        this->generated = true;
        checkOpenGLError("forest terrain init");
    }

    void buildTreeMesh()
    {
        this->treeVertices.clear();
        this->treeIndices.clear();
        this->treeVertices.reserve(2400);
        this->treeIndices.reserve(3600);

        auto pushTreeVertex = [&](const glm::vec3 &pos, const glm::vec3 &normal, const glm::vec3 &color)
        {
            this->treeVertices.push_back({pos, normal, color});
            return uint32_t(this->treeVertices.size() - 1);
        };

        auto addQuad = [&](const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c, const glm::vec3 &d,
                           const glm::vec3 &normal, const glm::vec3 &color)
        {
            uint32_t i0 = pushTreeVertex(a, normal, color);
            uint32_t i1 = pushTreeVertex(b, normal, color);
            uint32_t i2 = pushTreeVertex(c, normal, color);
            uint32_t i3 = pushTreeVertex(d, normal, color);
            this->treeIndices.push_back(i0);
            this->treeIndices.push_back(i1);
            this->treeIndices.push_back(i2);
            this->treeIndices.push_back(i0);
            this->treeIndices.push_back(i2);
            this->treeIndices.push_back(i3);
        };

        const int treeRows = 44;
        const int treeCols = 28;
        for (int rz = 0; rz < treeRows; ++rz)
        {
            float rowT = float(rz) / float(glm::max(1, treeRows));
            float baseZ = glm::mix(kNearZ, kFarZ, rowT);
            for (int cx = 0; cx < treeCols; ++cx)
            {
                float colT = float(cx) / float(glm::max(1, treeCols - 1));
                float baseX = glm::mix(-110.0f, 110.0f, colT);
                float jitterX = (hash01(cx + 700, rz + 1100) - 0.5f) * 8.0f;
                float jitterZ = (hash01(cx + 1400, rz + 1700) - 0.5f) * 10.0f;
                float x = baseX + jitterX;
                float z = baseZ + jitterZ * 0.02f;

                const float centerClear01 = 1.0f - glm::smoothstep(8.0f, 28.0f, std::abs(x));
                const float keepChance =
                    glm::mix(0.92f, 0.08f, centerClear01) *
                    glm::mix(0.55f, 1.0f, glm::smoothstep(18.0f, 72.0f, std::abs(x)));
                if (hash01(cx + 2100, rz + 2400) > keepChance)
                    continue;

                float terrainY = heightAt(x, z);
                float sizeNoise = periodicFbm(x, z, 0.02f, 13.0f, 201.0f, 17.0f);
                // Large on purpose so they read from far away.
                float treeHeight = glm::mix(10.0f, 18.0f, sizeNoise);
                float canopyWidth = glm::mix(4.5f, 8.0f, sizeNoise);
                float trunkHeight = treeHeight * 0.24f;
                float topY = terrainY + treeHeight;
                float trunkTopY = terrainY + trunkHeight;

                glm::vec3 trunkColor = glm::vec3(0.22f, 0.15f, 0.09f);
                glm::vec3 canopyColor = glm::mix(
                    glm::vec3(0.07f, 0.18f, 0.06f),
                    glm::vec3(0.14f, 0.28f, 0.10f),
                    glm::clamp(sizeNoise, 0.0f, 1.0f)
                );

                float trunkHalfW = glm::mix(0.22f, 0.38f, sizeNoise);
                addQuad(
                    glm::vec3(x - trunkHalfW, terrainY, z),
                    glm::vec3(x + trunkHalfW, terrainY, z),
                    glm::vec3(x + trunkHalfW, trunkTopY, z),
                    glm::vec3(x - trunkHalfW, trunkTopY, z),
                    glm::vec3(0.0f, 0.0f, 1.0f),
                    trunkColor
                );

                auto addCrossCanopy = [&](float angleRadians)
                {
                    glm::vec2 dir = glm::vec2(std::cos(angleRadians), std::sin(angleRadians));
                    glm::vec2 perp(-dir.y, dir.x);
                    float halfW = canopyWidth * 0.5f;
                    glm::vec3 bottomL(x - perp.x * halfW, trunkTopY, z - perp.y * halfW);
                    glm::vec3 bottomR(x + perp.x * halfW, trunkTopY, z + perp.y * halfW);
                    glm::vec3 topR(x + dir.x * 0.2f, topY, z + dir.y * 0.2f);
                    glm::vec3 topL(x - dir.x * 0.2f, topY, z - dir.y * 0.2f);
                    glm::vec3 normal = glm::normalize(glm::vec3(dir.y, 0.0f, -dir.x));
                    addQuad(bottomL, bottomR, topR, topL, normal, canopyColor);
                };

                addCrossCanopy(0.0f);
                addCrossCanopy(0.78539816339f);
            }
        }

        this->treeIndexCount = GLsizei(this->treeIndices.size());

        if (this->treeEbo != 0)
        {
            glDeleteBuffers(1, &this->treeEbo);
            this->treeEbo = 0;
        }
        if (this->treeVbo != 0)
        {
            glDeleteBuffers(1, &this->treeVbo);
            this->treeVbo = 0;
        }
        if (this->treeVao != 0)
        {
            glDeleteVertexArrays(1, &this->treeVao);
            this->treeVao = 0;
        }

        glGenVertexArrays(1, &this->treeVao);
        glGenBuffers(1, &this->treeVbo);
        glGenBuffers(1, &this->treeEbo);

        glBindVertexArray(this->treeVao);
        glBindBuffer(GL_ARRAY_BUFFER, this->treeVbo);
        glBufferData(
            GL_ARRAY_BUFFER,
            GLsizeiptr(this->treeVertices.size() * sizeof(ForestTreeVertex)),
            this->treeVertices.data(),
            GL_STATIC_DRAW
        );
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->treeEbo);
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            GLsizeiptr(this->treeIndices.size() * sizeof(uint32_t)),
            this->treeIndices.data(),
            GL_STATIC_DRAW
        );

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ForestTreeVertex), (void *)offsetof(ForestTreeVertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ForestTreeVertex), (void *)offsetof(ForestTreeVertex, normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(ForestTreeVertex), (void *)offsetof(ForestTreeVertex, color));

        glBindVertexArray(0);
        checkOpenGLError("forest tree init");
    }

    void renderForest(const glm::mat4 &cameraMatrix, const glm::mat4 &projectionMatrix)
    {
        if (!this->generated)
            this->buildTerrainMesh();

        const glm::mat4 viewMatrix = glm::inverse(cameraMatrix);
        const glm::vec3 cameraPos = glm::vec3(cameraMatrix[3]);
        const float tileOffsets[2] = {
            -this->scrollZ,
            -this->scrollZ + kScrollCycleMeters,
        };

        glUseProgram(this->shaderId);
        glUniformMatrix4fv(glGetUniformLocation(this->shaderId, "u_worldToView"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(glGetUniformLocation(this->shaderId, "u_projection"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
        glUniform3fv(glGetUniformLocation(this->shaderId, "u_cameraPos"), 1, glm::value_ptr(cameraPos));
        glBindVertexArray(this->vao);
        for (float zOffset : tileOffsets)
        {
            const glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, zOffset));
            glUniformMatrix4fv(glGetUniformLocation(this->shaderId, "u_modelToWorld"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
            glDrawElements(GL_TRIANGLES, this->indexCount, GL_UNSIGNED_INT, 0);
        }
        glBindVertexArray(0);

        glUseProgram(this->treeShaderId);
        glUniformMatrix4fv(glGetUniformLocation(this->treeShaderId, "u_worldToView"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(glGetUniformLocation(this->treeShaderId, "u_projection"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
        glUniform3fv(glGetUniformLocation(this->treeShaderId, "u_cameraPos"), 1, glm::value_ptr(cameraPos));
        glBindVertexArray(this->treeVao);
        for (float zOffset : tileOffsets)
        {
            const glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, zOffset));
            glUniformMatrix4fv(glGetUniformLocation(this->treeShaderId, "u_modelToWorld"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
            glDrawElements(GL_TRIANGLES, this->treeIndexCount, GL_UNSIGNED_INT, 0);
        }
        glBindVertexArray(0);
    }
};

const char *ForestTerrain::FOREST_VERTEX_SHADER = GLSL_VERSION R"(
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

const char *ForestTerrain::FOREST_FRAGMENT_SHADER = GLSL_VERSION R"(
    precision highp float;

    in vec3 v_worldPos;
    in vec3 v_normal;

    uniform vec3 u_cameraPos;

    out vec4 FragColor;

    void main()
    {
        vec3 normal = normalize(v_normal);
        vec3 lightDir = normalize(vec3(-0.30, 0.92, 0.18));
        vec3 viewDir = normalize(u_cameraPos - v_worldPos);

        float diffuse = clamp(dot(normal, lightDir), 0.0, 1.0);
        float fresnel = pow(1.0 - clamp(dot(normal, viewDir), 0.0, 1.0), 2.0);

        float lowlandT = smoothstep(-25.0, -18.0, v_worldPos.y);
        float highlandT = smoothstep(-16.0, -7.5, v_worldPos.y);

        vec3 pond = vec3(0.11, 0.18, 0.16);
        vec3 meadow = vec3(0.23, 0.34, 0.20);
        vec3 ridge = vec3(0.36, 0.39, 0.27);

        vec3 color = mix(pond, meadow, lowlandT);
        color = mix(color, ridge, highlandT * 0.85);
        color *= 0.60 + diffuse * 0.55;
        color += vec3(0.03, 0.05, 0.02) * fresnel;

        float fogT = smoothstep(130.0, 380.0, v_worldPos.z);
        color = mix(color, vec3(0.48, 0.58, 0.49), fogT * 0.42);

        FragColor = vec4(color, 1.0);
    }
)";

const char *ForestTerrain::TREE_VERTEX_SHADER = GLSL_VERSION R"(
    precision highp float;

    layout(location = 0) in vec3 a_pos;
    layout(location = 1) in vec3 a_normal;
    layout(location = 2) in vec3 a_color;

    uniform mat4 u_modelToWorld;
    uniform mat4 u_worldToView;
    uniform mat4 u_projection;

    out vec3 v_worldPos;
    out vec3 v_normal;
    out vec3 v_color;

    void main()
    {
        vec4 worldPos = u_modelToWorld * vec4(a_pos, 1.0);
        v_worldPos = worldPos.xyz;
        v_normal = normalize(mat3(u_modelToWorld) * a_normal);
        v_color = a_color;
        gl_Position = u_projection * u_worldToView * worldPos;
    }
)";

const char *ForestTerrain::TREE_FRAGMENT_SHADER = GLSL_VERSION R"(
    precision highp float;

    in vec3 v_worldPos;
    in vec3 v_normal;
    in vec3 v_color;

    uniform vec3 u_cameraPos;

    out vec4 FragColor;

    void main()
    {
        vec3 normal = normalize(v_normal);
        vec3 lightDir = normalize(vec3(-0.22, 0.95, 0.14));
        vec3 viewDir = normalize(u_cameraPos - v_worldPos);
        float diffuse = clamp(dot(normal, lightDir), 0.0, 1.0);
        float fresnel = pow(1.0 - clamp(dot(normal, viewDir), 0.0, 1.0), 1.6);
        vec3 color = v_color * (0.45 + diffuse * 0.60);
        color += vec3(0.03, 0.04, 0.02) * fresnel;
        float fogT = smoothstep(120.0, 360.0, v_worldPos.z);
        color = mix(color, vec3(0.42, 0.51, 0.44), fogT * 0.48);
        FragColor = vec4(color, 1.0);
    }
)";
