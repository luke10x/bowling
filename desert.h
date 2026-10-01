#pragma once

#include "framework/gl_header.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <cmath>
#include <utility>

#include "framework/gl_util.h"

struct DesertTerrainVertex
{
    glm::vec3 position;
    glm::vec3 normal;
};

struct DesertRuinedSmokeVertex { glm::vec3 position; glm::vec2 uv; glm::vec4 params; };
struct DesertRuinedSparkVertex { glm::vec3 origin; glm::vec4 params; };

struct DesertTerrain
{
    static const char *DESERT_VERTEX_SHADER;
    static const char *DESERT_FRAGMENT_SHADER;
    static const char *RUIN_SMOKE_VERTEX_SHADER;
    static const char *RUIN_SMOKE_FRAGMENT_SHADER;
    static const char *RUIN_SPARK_VERTEX_SHADER;
    static const char *RUIN_SPARK_FRAGMENT_SHADER;

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLuint shaderId = 0;
    GLuint pyramidVao = 0;
    GLuint pyramidVbo = 0;
    GLuint pyramidEbo = 0;
    GLuint redMountainVao = 0, redMountainVbo = 0, redMountainEbo = 0;
    GLuint ruinCityVao = 0;
    GLuint ruinCityVbo = 0;
    GLuint ruinCityEbo = 0;
    GLuint ruinSmokeVao = 0;
    GLuint ruinSmokeVbo = 0;
    GLuint ruinSmokeEbo = 0;
    GLuint ruinSmokeShaderId = 0;
    GLuint ruinSparkVao = 0, ruinSparkVbo = 0, ruinSparkShaderId = 0;
    GLsizei indexCount = 0;
    GLsizei pyramidIndexCount = 0;
    GLsizei redMountainIndexCount = 0;
    GLsizei ruinCityIndexCount = 0;
    GLsizei ruinSmokeIndexCount = 0;
    GLsizei ruinSparkCount = 0;
    float scrollZ = 0.0f;
    bool greyFlat = false;
    bool redDesert = false;
    bool terrainStyleBuilt = false;

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
    static constexpr float kPyramidHorizonDistance = kFarZ + 8.0f;

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

    static float redCrackDepth(float x, float z)
    {
        auto h = [](glm::vec2 p) { p = glm::fract(p * glm::vec2(123.34f, 456.21f)); p += glm::dot(p, p + 45.32f); return glm::fract(p.x * p.y); };
        const glm::vec2 uv(x * 0.075f, z * 0.075f);
        const glm::vec2 cell = glm::floor(uv), f = glm::fract(uv);
        float nearest = 10.0f, second = 10.0f;
        for (int cy = -1; cy <= 1; ++cy) for (int cx = -1; cx <= 1; ++cx)
        {
            const glm::vec2 id = cell + glm::vec2(float(cx), float(cy));
            const glm::vec2 jitter(h(id), h(id + 19.37f));
            const float d = glm::length(glm::vec2(float(cx), float(cy)) + jitter - f);
            if (d < nearest) { second = nearest; nearest = d; } else if (d < second) second = d;
        }
        const float crack = 1.0f - smoothstep01(glm::clamp((second - nearest - 0.035f) / 0.07f, 0.0f, 1.0f));
        return crack * 1.15f;
    }

    void loadDesertShader()
    {
        this->shaderId = vtx::createShaderProgram(DESERT_VERTEX_SHADER, DESERT_FRAGMENT_SHADER);
    }

    void initDesert()
    {
        this->greyFlat = false;
        this->scrollZ = 0.0f;
        this->loadDesertShader();
        this->buildTerrainMesh();
        this->buildPyramidMesh();
        this->buildRuinedCityParallax();
        this->ruinSmokeShaderId = vtx::createShaderProgram(RUIN_SMOKE_VERTEX_SHADER, RUIN_SMOKE_FRAGMENT_SHADER);
        this->ruinSparkShaderId = vtx::createShaderProgram(RUIN_SPARK_VERTEX_SHADER, RUIN_SPARK_FRAGMENT_SHADER);
    }

    void initGreyDesert()
    {
        this->greyFlat = true;
        this->scrollZ = 0.0f;
        this->loadDesertShader();
        this->buildTerrainMesh();
        this->buildPyramidMesh();
    }

    float meshHeightAt(float x, float z) const
    {
        if (this->redDesert)
            return kBaseY - redCrackDepth(x, z);
        if (!this->greyFlat)
            return heightAt(x, z);
        // The central route stays flat, while the wasteland rises into broad
        // enclosing banks at either side.
        const float sideBank = glm::smoothstep(46.0f, 112.0f, std::abs(x));
        return kBaseY + sideBank * 18.0f;
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
                const float h = meshHeightAt(worldX, worldZ);
                const float hx0 = meshHeightAt(worldX - dx, worldZ);
                const float hx1 = meshHeightAt(worldX + dx, worldZ);
                const float hz0 = meshHeightAt(worldX, worldZ - dz);
                const float hz1 = meshHeightAt(worldX, worldZ + dz);
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
        this->terrainStyleBuilt = this->redDesert;
        checkOpenGLError("desert terrain init");
    }

    void buildPyramidMesh()
    {
        std::vector<DesertTerrainVertex> pyramidVertices;
        std::vector<uint32_t> pyramidIndices;

        auto addFace = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c, const glm::vec3 &center)
        {
            glm::vec3 normal = glm::normalize(glm::cross(b - a, c - a));
            const glm::vec3 midpoint = (a + b + c) / 3.0f;
            const glm::vec3 outward(midpoint.x - center.x, 0.0f, midpoint.z - center.z);
            if (glm::dot(normal, outward) < 0.0f)
            {
                std::swap(b, c);
                normal = glm::normalize(glm::cross(b - a, c - a));
            }
            const uint32_t first = uint32_t(pyramidVertices.size());
            pyramidVertices.push_back({a, normal});
            pyramidVertices.push_back({b, normal});
            pyramidVertices.push_back({c, normal});
            pyramidIndices.push_back(first);
            pyramidIndices.push_back(first + 1);
            pyramidIndices.push_back(first + 2);
        };

        auto addPyramid = [&](float centerX, float centerZ, float halfWidth, float halfDepth, float height)
        {
            const glm::vec3 center(centerX, kBaseY, centerZ);
            const glm::vec3 nw(centerX - halfWidth, kBaseY, centerZ - halfDepth);
            const glm::vec3 ne(centerX + halfWidth, kBaseY, centerZ - halfDepth);
            const glm::vec3 se(centerX + halfWidth, kBaseY, centerZ + halfDepth);
            const glm::vec3 sw(centerX - halfWidth, kBaseY, centerZ + halfDepth);
            const glm::vec3 apex(centerX, kBaseY + height, centerZ);
            addFace(nw, ne, apex, center);
            addFace(ne, se, apex, center);
            addFace(se, sw, apex, center);
            addFace(sw, nw, apex, center);
        };

        auto addBox = [&](float centerX, float centerZ, float halfWidth, float halfDepth, float height,
                          float baseY = kBaseY, float leanX = 0.0f)
        {
            const glm::vec3 center(centerX, baseY, centerZ);
            const glm::vec3 a(centerX - halfWidth, baseY, centerZ - halfDepth);
            const glm::vec3 b(centerX + halfWidth, baseY, centerZ - halfDepth);
            const glm::vec3 c(centerX + halfWidth, baseY, centerZ + halfDepth);
            const glm::vec3 d(centerX - halfWidth, baseY, centerZ + halfDepth);
            const glm::vec3 e(centerX - halfWidth + leanX, baseY + height, centerZ - halfDepth);
            const glm::vec3 f(centerX + halfWidth + leanX, baseY + height, centerZ - halfDepth);
            const glm::vec3 g(centerX + halfWidth + leanX, baseY + height, centerZ + halfDepth);
            const glm::vec3 h(centerX - halfWidth + leanX, baseY + height, centerZ + halfDepth);
            auto quad = [&](glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3)
            {
                addFace(p0, p1, p2, center);
                addFace(p0, p2, p3, center);
            };
            quad(a, b, f, e);
            quad(b, c, g, f);
            quad(c, d, h, g);
            quad(d, a, e, h);
            quad(e, f, g, h);
        };

        auto addGravestone = [&](float x, float z, float width, float depth, float height, float lean)
        {
            // A slab plus stepped semicircular crown reads as a worn,
            // headstone silhouette from the moving camera while remaining a
            // genuine 3D ground prop.
            const float bodyH = height * 0.72f;
            addBox(x, z, width, depth, bodyH, kBaseY, lean);
            const float capH = height - bodyH;
            for (int row = 0; row < 4; ++row)
            {
                const float t = (float(row) + 0.5f) / 4.0f;
                const float crownWidth = width * std::sqrt(glm::max(0.0f, 1.0f - t * t));
                addBox(x + lean * (bodyH + capH * t) / height,
                       z, crownWidth, depth * (1.0f - t * 0.12f), capH / 4.0f,
                       kBaseY + bodyH + capH * (float(row) / 4.0f), lean * 0.25f);
            }
        };

        auto addCemeteryMonument = [&](float x, float z, float scale, bool angel)
        {
            const float base = kBaseY;
            addBox(x, z, 2.8f * scale, 2.2f * scale, 0.55f * scale, base);
            addBox(x, z, 2.15f * scale, 1.65f * scale, 0.48f * scale, base + 0.55f * scale);
            addBox(x, z, 0.78f * scale, 0.70f * scale, 2.5f * scale, base + 1.03f * scale);
            if (angel)
            {
                // Low-poly statue silhouette: body, head, and swept wings.
                addBox(x, z, 0.58f * scale, 0.52f * scale, 1.55f * scale, base + 3.53f * scale);
                addPyramid(x, z, 0.42f * scale, 0.38f * scale, 0.72f * scale);
                addBox(x - 0.92f * scale, z, 0.56f * scale, 0.20f * scale, 1.55f * scale, base + 3.7f * scale, -0.28f * scale);
                addBox(x + 0.92f * scale, z, 0.56f * scale, 0.20f * scale, 1.55f * scale, base + 3.7f * scale, 0.28f * scale);
            }
            else
            {
                addPyramid(x, z, 0.58f * scale, 0.52f * scale, 2.7f * scale);
            }
        };

        auto addMashedHouse = [&](float x, float z, float scale, float lean)
        {
            const float w = 3.0f * scale;
            const float d = 2.5f * scale;
            const float h = 2.8f * scale;
            // Offset wall sections and a split roof create a collapsed,
            // suburbia-like shell instead of a clean intact house.
            addBox(x - 0.35f * scale, z, w, d, h, kBaseY, lean);
            addBox(x + 1.55f * scale, z + 0.35f * scale, 1.15f * scale,
                   1.8f * scale, 1.75f * scale, kBaseY, -lean * 0.7f);
            addBox(x - 0.25f * scale, z - 0.1f * scale, w * 0.72f,
                   d * 1.12f, 0.34f * scale, kBaseY + h + 0.08f * scale, lean * 0.3f);
            addBox(x + 1.15f * scale, z + 0.45f * scale, 1.4f * scale,
                   d * 0.8f, 0.28f * scale, kBaseY + 1.9f * scale, -lean);
            if (scale > 1.0f)
                addBox(x - 1.35f * scale, z + 0.2f * scale, 0.32f * scale,
                       0.34f * scale, 1.2f * scale, kBaseY + h * 0.65f, lean);
        };

        if (this->greyFlat)
        {
            // Sparse clusters of hard, narrow rock shards plus thin dead
            // brush spikes. All live on the same flat, scrolling terrain.
            for (int cluster = 0; cluster < 19; ++cluster)
            {
                const float side = hash01(cluster, 41) < 0.5f ? -1.0f : 1.0f;
                const float x = side * glm::mix(31.0f, 56.0f, hash01(cluster, 43));
                const float z = glm::mix(-30.0f, 430.0f, hash01(cluster, 67));
                const int shards = 2 + int(hash01(cluster, 83) * 3.0f);
                for (int shard = 0; shard < shards; ++shard)
                {
                    const float n = hash01(cluster * 17 + shard, 97);
                    addPyramid(x + (n - 0.5f) * 9.0f, z + (hash01(cluster, shard + 131) - 0.5f) * 9.0f,
                               0.8f + n * 2.5f, 0.7f + hash01(cluster, shard + 151) * 1.8f,
                               5.0f + hash01(cluster, shard + 173) * 17.0f);
                }
            }
            // Sparse rounded tombstones replace vegetation entirely.
            for (int tomb = 0; tomb < 92; ++tomb)
            {
                const float x = glm::mix(-108.0f, 108.0f, hash01(tomb, 211));
                const float z = glm::mix(-42.0f, 445.0f, hash01(tomb, 233));
                const float height = 1.8f + hash01(tomb, 251) * 2.8f;
                const float lean = hash01(tomb, 263) > 0.66f
                    ? (hash01(tomb, 271) - 0.5f) * 1.5f : 0.0f;
                addGravestone(x, z, 0.55f + hash01(tomb, 281) * 0.68f,
                              0.16f + hash01(tomb, 293) * 0.16f, height, lean);
            }
            for (int cross = 0; cross < 18; ++cross)
            {
                const float x = glm::mix(-86.0f, 86.0f, hash01(cross, 307));
                const float z = glm::mix(-38.0f, 440.0f, hash01(cross, 331));
                const float h = 2.6f + hash01(cross, 347) * 2.4f;
                // Upright stone and raised crossbar, both as real 3D boxes.
                addBox(x, z, 0.14f, 0.16f, h, kBaseY);
                addBox(x, z, 0.82f, 0.16f, 0.26f, kBaseY + h * 0.57f);
            }
            for (int monument = 0; monument < 13; ++monument)
            {
                const float side = hash01(monument, 419) < 0.5f ? -1.0f : 1.0f;
                const float x = side * glm::mix(58.0f, 108.0f, hash01(monument, 421));
                const float z = glm::mix(-24.0f, 438.0f, hash01(monument, 443));
                addCemeteryMonument(x, z, 0.72f + hash01(monument, 467) * 0.62f,
                                    hash01(monument, 491) > 0.38f);
            }
            for (int house = 0; house < 16; ++house)
            {
                const float side = hash01(house, 523) < 0.5f ? -1.0f : 1.0f;
                const float x = side * glm::mix(18.0f, 48.0f, hash01(house, 541));
                const float z = glm::mix(-18.0f, 438.0f, hash01(house, 557));
                addMashedHouse(x, z, 0.95f + hash01(house, 571) * 0.62f,
                               (hash01(house, 587) - 0.5f) * 0.65f);
            }
        }
        else
        {
            addPyramid(-82.0f, 0.0f, 38.0f, 31.0f, 55.0f);
            addPyramid(12.0f, 0.0f, 27.0f, 23.0f, 39.0f);
            addPyramid(78.0f, 0.0f, 32.0f, 27.0f, 47.0f);
        }

        this->pyramidIndexCount = GLsizei(pyramidIndices.size());
        if (this->pyramidEbo != 0) glDeleteBuffers(1, &this->pyramidEbo);
        if (this->pyramidVbo != 0) glDeleteBuffers(1, &this->pyramidVbo);
        if (this->pyramidVao != 0) glDeleteVertexArrays(1, &this->pyramidVao);

        glGenVertexArrays(1, &this->pyramidVao);
        glGenBuffers(1, &this->pyramidVbo);
        glGenBuffers(1, &this->pyramidEbo);
        glBindVertexArray(this->pyramidVao);
        glBindBuffer(GL_ARRAY_BUFFER, this->pyramidVbo);
        glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(pyramidVertices.size() * sizeof(DesertTerrainVertex)), pyramidVertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->pyramidEbo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(pyramidIndices.size() * sizeof(uint32_t)), pyramidIndices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(DesertTerrainVertex), (void *)offsetof(DesertTerrainVertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(DesertTerrainVertex), (void *)offsetof(DesertTerrainVertex, normal));
        glBindVertexArray(0);
        checkOpenGLError("desert pyramid init");

        std::vector<DesertTerrainVertex> mountainVertices;
        std::vector<uint32_t> mountainIndices;
        const int segments = 18;
        for (int i = 0; i < segments; ++i)
        {
            const float x0 = -190.0f + float(i) * 380.0f / float(segments);
            const float x1 = -190.0f + float(i + 1) * 380.0f / float(segments);
            const float h0 = 18.0f + 22.0f * (0.5f + 0.5f * std::sin(float(i) * 1.71f));
            const float h1 = 18.0f + 22.0f * (0.5f + 0.5f * std::sin(float(i + 1) * 1.71f));
            const uint32_t b = uint32_t(mountainVertices.size());
            const glm::vec3 n(0.0f, 0.0f, -1.0f);
            mountainVertices.push_back({{x0, kBaseY, 0.0f}, n}); mountainVertices.push_back({{x1, kBaseY, 0.0f}, n});
            mountainVertices.push_back({{x1, kBaseY + h1, 0.0f}, n}); mountainVertices.push_back({{x0, kBaseY + h0, 0.0f}, n});
            mountainIndices.insert(mountainIndices.end(), {b,b+1,b+2,b,b+2,b+3});
        }
        redMountainIndexCount = GLsizei(mountainIndices.size());
        glGenVertexArrays(1, &redMountainVao); glGenBuffers(1, &redMountainVbo); glGenBuffers(1, &redMountainEbo);
        glBindVertexArray(redMountainVao); glBindBuffer(GL_ARRAY_BUFFER, redMountainVbo);
        glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(mountainVertices.size() * sizeof(DesertTerrainVertex)), mountainVertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, redMountainEbo); glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(mountainIndices.size() * sizeof(uint32_t)), mountainIndices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(DesertTerrainVertex),(void*)offsetof(DesertTerrainVertex,position));
        glEnableVertexAttribArray(1); glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(DesertTerrainVertex),(void*)offsetof(DesertTerrainVertex,normal)); glBindVertexArray(0);
    }

    void buildRuinedCityParallax()
    {
        if (this->ruinSmokeShaderId == 0)
            this->ruinSmokeShaderId = vtx::createShaderProgram(RUIN_SMOKE_VERTEX_SHADER, RUIN_SMOKE_FRAGMENT_SHADER);
        if (this->ruinSparkShaderId == 0)
            this->ruinSparkShaderId = vtx::createShaderProgram(RUIN_SPARK_VERTEX_SHADER, RUIN_SPARK_FRAGMENT_SHADER);
        std::vector<DesertTerrainVertex> cityVertices;
        std::vector<uint32_t> cityIndices;
        auto face = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, glm::vec3 n) {
            const uint32_t i = uint32_t(cityVertices.size());
            cityVertices.push_back({a, n}); cityVertices.push_back({b, n});
            cityVertices.push_back({c, n}); cityVertices.push_back({d, n});
            cityIndices.insert(cityIndices.end(), {i, i + 1, i + 2, i, i + 2, i + 3});
        };
        auto box = [&](float x, float z, float w, float d, float h, float leanX) {
            const float y = kBaseY;
            glm::vec3 a(x-w,y,z-d), b(x+w,y,z-d), c(x+w,y,z+d), d0(x-w,y,z+d);
            glm::vec3 e(x-w+leanX,y+h,z-d), f(x+w+leanX,y+h,z-d);
            glm::vec3 g(x+w+leanX,y+h,z+d), h0(x-w+leanX,y+h,z+d);
            face(a,b,f,e,glm::vec3(0,0,-1)); face(b,c,g,f,glm::vec3(1,0,0));
            face(c,d0,h0,g,glm::vec3(0,0,1)); face(d0,a,e,h0,glm::vec3(-1,0,0));
            face(e,f,g,h0,glm::vec3(0,1,0));
        };
        for (int i = 0; i < 27; ++i) {
            const float x = -156.0f + float(i) * 12.0f;
            float h = 24.0f + hash01(i, 601) * 56.0f;
            if (hash01(i, 617) > 0.47f) h *= 0.32f;
            const float leanX = hash01(i, 659) > 0.42f
                ? (hash01(i, 673) - 0.5f) * 20.0f : 0.0f;
            box(x, 390.0f + (hash01(i, 631)-0.5f)*12.0f, 4.6f + hash01(i,647)*4.0f, 5.2f, h, leanX);
        }
        ruinCityIndexCount = GLsizei(cityIndices.size());
        if (ruinCityEbo) glDeleteBuffers(1, &ruinCityEbo);
        if (ruinCityVbo) glDeleteBuffers(1, &ruinCityVbo);
        if (ruinCityVao) glDeleteVertexArrays(1, &ruinCityVao);
        glGenVertexArrays(1,&ruinCityVao); glGenBuffers(1,&ruinCityVbo); glGenBuffers(1,&ruinCityEbo);
        glBindVertexArray(ruinCityVao); glBindBuffer(GL_ARRAY_BUFFER,ruinCityVbo);
        glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(cityVertices.size()*sizeof(DesertTerrainVertex)), cityVertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ruinCityEbo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(cityIndices.size()*sizeof(uint32_t)), cityIndices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(DesertTerrainVertex),(void*)offsetof(DesertTerrainVertex,position));
        glEnableVertexAttribArray(1); glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(DesertTerrainVertex),(void*)offsetof(DesertTerrainVertex,normal));
        glBindVertexArray(0);

        std::vector<DesertRuinedSmokeVertex> smokeVertices;
        std::vector<uint32_t> smokeIndices;
        auto smokeQuad = [&](glm::vec3 center, float w, float h, float phase, float density) {
            const uint32_t i = uint32_t(smokeVertices.size());
            smokeVertices.push_back({center + glm::vec3(-w * .5f, -h * .5f, 0), {0,0}, {phase,w,h,density}});
            smokeVertices.push_back({center + glm::vec3( w * .5f, -h * .5f, 0), {1,0}, {phase,w,h,density}});
            smokeVertices.push_back({center + glm::vec3( w * .5f,  h * .5f, 0), {1,1}, {phase,w,h,density}});
            smokeVertices.push_back({center + glm::vec3(-w * .5f,  h * .5f, 0), {0,1}, {phase,w,h,density}});
            smokeIndices.insert(smokeIndices.end(), {i,i+1,i+2,i,i+2,i+3});
        };
        for (int fire = 0; fire < 5; ++fire) {
            const int building = 2 + fire * 5;
            const float x = -156.0f + float(building) * 12.0f;
            const float z = 390.0f + (hash01(building, 631) - 0.5f) * 12.0f - 2.8f;
            float buildingH = 24.0f + hash01(building, 601) * 56.0f;
            if (hash01(building, 617) > 0.47f) buildingH *= 0.32f;
            const float baseY = kBaseY + buildingH;
            for (int puff = 0; puff < 7; ++puff)
                smokeQuad(glm::vec3(x, baseY + float(puff) * 2.4f, z), 7.0f, 5.5f,
                          float(fire) * 1.73f + float(puff) * .17f, .60f + float(puff) * .04f);
        }
        ruinSmokeIndexCount = GLsizei(smokeIndices.size());
        if (ruinSmokeEbo) glDeleteBuffers(1, &ruinSmokeEbo);
        if (ruinSmokeVbo) glDeleteBuffers(1, &ruinSmokeVbo);
        if (ruinSmokeVao) glDeleteVertexArrays(1, &ruinSmokeVao);
        glGenVertexArrays(1,&ruinSmokeVao); glGenBuffers(1,&ruinSmokeVbo); glGenBuffers(1,&ruinSmokeEbo);
        glBindVertexArray(ruinSmokeVao); glBindBuffer(GL_ARRAY_BUFFER,ruinSmokeVbo);
        glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(smokeVertices.size()*sizeof(DesertRuinedSmokeVertex)), smokeVertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ruinSmokeEbo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(smokeIndices.size()*sizeof(uint32_t)), smokeIndices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(DesertRuinedSmokeVertex),(void*)offsetof(DesertRuinedSmokeVertex,position));
        glEnableVertexAttribArray(1); glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,sizeof(DesertRuinedSmokeVertex),(void*)offsetof(DesertRuinedSmokeVertex,uv));
        glEnableVertexAttribArray(2); glVertexAttribPointer(2,4,GL_FLOAT,GL_FALSE,sizeof(DesertRuinedSmokeVertex),(void*)offsetof(DesertRuinedSmokeVertex,params));
        glBindVertexArray(0);

        std::vector<DesertRuinedSparkVertex> sparks;
        for (int fire = 0; fire < 5; ++fire) {
            const int building = 2 + fire * 5;
            const float x = -156.0f + float(building) * 12.0f;
            const float z = 390.0f + (hash01(building, 631) - .5f) * 12.0f;
            float h = 24.0f + hash01(building, 601) * 56.0f;
            if (hash01(building, 617) > .47f) h *= .32f;
            for (int s = 0; s < 8; ++s) sparks.push_back({{x, kBaseY + h, z}, {hash01(fire,s), 9.0f + hash01(s,fire)*10.0f, (hash01(s,91)-.5f)*2.0f, 5.0f + hash01(fire,s+31)*4.0f}});
        }
        ruinSparkCount = GLsizei(sparks.size());
        glGenVertexArrays(1,&ruinSparkVao); glGenBuffers(1,&ruinSparkVbo);
        glBindVertexArray(ruinSparkVao); glBindBuffer(GL_ARRAY_BUFFER,ruinSparkVbo);
        glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(sparks.size()*sizeof(DesertRuinedSparkVertex)), sparks.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(DesertRuinedSparkVertex),(void*)offsetof(DesertRuinedSparkVertex,origin));
        glEnableVertexAttribArray(1); glVertexAttribPointer(1,4,GL_FLOAT,GL_FALSE,sizeof(DesertRuinedSparkVertex),(void*)offsetof(DesertRuinedSparkVertex,params)); glBindVertexArray(0);
    }

    void renderDesert(const glm::mat4 &cameraMatrix, const glm::mat4 &projectionMatrix)
    {
        if (!this->generated)
            this->buildTerrainMesh();
        if (this->terrainStyleBuilt != this->redDesert)
            this->buildTerrainMesh();
        if (this->pyramidVao == 0)
            this->buildPyramidMesh();
        if (this->greyFlat && this->ruinCityVao == 0)
            this->buildRuinedCityParallax();

        const glm::mat4 viewMatrix = glm::inverse(cameraMatrix);
        const glm::vec3 cameraPos = glm::vec3(cameraMatrix[3]);
        glUseProgram(this->shaderId);
        glUniformMatrix4fv(glGetUniformLocation(this->shaderId, "u_worldToView"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(glGetUniformLocation(this->shaderId, "u_projection"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
        glUniform3fv(glGetUniformLocation(this->shaderId, "u_cameraPos"), 1, glm::value_ptr(cameraPos));
        glUniform1f(glGetUniformLocation(this->shaderId, "u_greyDesert"), this->greyFlat ? 1.0f : 0.0f);
        glUniform1f(glGetUniformLocation(this->shaderId, "u_redDesert"), this->redDesert ? 1.0f : 0.0f);
        glUniform1f(glGetUniformLocation(this->shaderId, "u_redMountain"), 0.0f);
        glUniform1f(glGetUniformLocation(this->shaderId, "u_time"), this->scrollZ);

        glBindVertexArray(this->vao);
        const float tileOffsets[3] = {
            -this->scrollZ - kScrollCycleMeters,
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

        glBindVertexArray(this->pyramidVao);
        if (this->greyFlat)
        {
            for (float zOffset : tileOffsets)
            {
                const glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, zOffset));
                glUniformMatrix4fv(glGetUniformLocation(this->shaderId, "u_modelToWorld"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
                glDrawElements(GL_TRIANGLES, this->pyramidIndexCount, GL_UNSIGNED_INT, 0);
            }
        }
        else if (!this->redDesert)
        {
            const float horizonZ = this->redDesert ? (kFarZ - 24.0f) : kPyramidHorizonDistance;
            const glm::mat4 pyramidModel = glm::translate(glm::mat4(1.0f), glm::vec3(cameraPos.x, 0.0f, cameraPos.z + horizonZ));
            glUniformMatrix4fv(glGetUniformLocation(this->shaderId, "u_modelToWorld"), 1, GL_FALSE, glm::value_ptr(pyramidModel));
            glDrawElements(GL_TRIANGLES, this->pyramidIndexCount, GL_UNSIGNED_INT, 0);
        }
        glBindVertexArray(0);
        if (this->redDesert)
        {
            // Bring the parallax wall forward until its ground edge meets the
            // visible end of the scrolling desert plane instead of floating
            // above the horizon.
            const glm::mat4 mountainModel = glm::translate(
                glm::mat4(1.0f),glm::vec3(cameraPos.x, -6.0f, cameraPos.z + kFarZ - 75.0f));
            glUniform1f(glGetUniformLocation(this->shaderId, "u_redMountain"), 1.0f);
            glUniformMatrix4fv(glGetUniformLocation(this->shaderId, "u_modelToWorld"), 1, GL_FALSE, glm::value_ptr(mountainModel));
            glBindVertexArray(this->redMountainVao);
            glDrawElements(GL_TRIANGLES, this->redMountainIndexCount, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            glUniform1f(glGetUniformLocation(this->shaderId, "u_redMountain"), 0.0f);
        }
        if (this->greyFlat)
        {
            // The skyline is intentionally a horizon parallax layer: do not
            // let the finite scrolling desert mesh occlude it.
            glDisable(GL_DEPTH_TEST);
            glUniform1f(glGetUniformLocation(this->shaderId, "u_ruinCity"), 1.0f);
            glUniformMatrix4fv(glGetUniformLocation(this->shaderId, "u_modelToWorld"), 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
            glBindVertexArray(this->ruinCityVao);
            glDrawElements(GL_TRIANGLES, this->ruinCityIndexCount, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            glUniform1f(glGetUniformLocation(this->shaderId, "u_ruinCity"), 0.0f);
            glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);
            glUseProgram(this->ruinSmokeShaderId);
            glUniformMatrix4fv(glGetUniformLocation(this->ruinSmokeShaderId,"u_worldToView"),1,GL_FALSE,glm::value_ptr(viewMatrix));
            glUniformMatrix4fv(glGetUniformLocation(this->ruinSmokeShaderId,"u_projection"),1,GL_FALSE,glm::value_ptr(projectionMatrix));
            glUniform1f(glGetUniformLocation(this->ruinSmokeShaderId,"u_time"),this->scrollZ);
            glUniform3fv(glGetUniformLocation(this->ruinSmokeShaderId,"u_cameraPos"),1,glm::value_ptr(cameraPos));
            glBindVertexArray(this->ruinSmokeVao);
            glDrawElements(GL_TRIANGLES, this->ruinSmokeIndexCount, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glUseProgram(this->ruinSparkShaderId);
            glUniformMatrix4fv(glGetUniformLocation(this->ruinSparkShaderId,"u_worldToView"),1,GL_FALSE,glm::value_ptr(viewMatrix)); glUniformMatrix4fv(glGetUniformLocation(this->ruinSparkShaderId,"u_projection"),1,GL_FALSE,glm::value_ptr(projectionMatrix)); glUniform1f(glGetUniformLocation(this->ruinSparkShaderId,"u_time"),this->scrollZ);
            glBindVertexArray(this->ruinSparkVao); glDrawArrays(GL_POINTS,0,this->ruinSparkCount); glBindVertexArray(0);
            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
            glEnable(GL_DEPTH_TEST);
        }
    }
};

const char *DesertTerrain::DESERT_VERTEX_SHADER = GLSL_VERSION R"(
    precision highp float;

    layout(location = 0) in vec3 a_pos;
    layout(location = 1) in vec3 a_normal;

    uniform mat4 u_modelToWorld;
    uniform mat4 u_worldToView;
    uniform mat4 u_projection;
    uniform float u_redDesert;
    uniform float u_redMountain;
    uniform float u_time;

    out vec3 v_worldPos;
    out vec3 v_normal;

    float hash21(vec2 p)
    {
        p = fract(p * vec2(123.34, 456.21));
        p += dot(p, p + 45.32);
        return fract(p.x * p.y);
    }

    void main()
    {
        vec3 shapedPos = a_pos;
        // The red biome's distant relief is broad and ridge-like rather than
        // reading as isolated Egyptian-style pyramids.
        if (u_redMountain > 0.5)
        {
            shapedPos.x *= 1.65;
            shapedPos.z *= 0.62;
            shapedPos.y = -24.0 + (shapedPos.y + 24.0) * 0.72;
        }
        vec4 worldPos = u_modelToWorld * vec4(shapedPos, 1.0);
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
    uniform float u_greyDesert;
    uniform float u_redDesert;
    uniform float u_redMountain;
    uniform float u_ruinCity;
    uniform float u_time;
    out vec4 FragColor;

    float canyonTint(float y)
    {
        return smoothstep(-33.0, -21.0, y);
    }

    float hash21(vec2 p)
    {
        p = fract(p * vec2(123.34, 456.21));
        p += dot(p, p + 45.32);
        return fract(p.x * p.y);
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
        // Lock the procedural cracks to the scrolling terrain tile.  The
        // terrain moves by -u_time in Z, so add the same distance back before
        // sampling the world-space field.
        vec2 crackUv = vec2(v_worldPos.x, v_worldPos.z + u_time);
        vec2 crackCell = floor(crackUv * 0.075);
        vec2 crackF = fract(crackUv * 0.075);
        float nearest = 10.0, second = 10.0;
        for (int cy = -1; cy <= 1; ++cy) for (int cx = -1; cx <= 1; ++cx)
        {
            vec2 id = crackCell + vec2(float(cx), float(cy));
            vec2 jitter = vec2(hash21(id), hash21(id + 19.37));
            float d = length(vec2(float(cx), float(cy)) + jitter - crackF);
            if (d < nearest) { second = nearest; nearest = d; }
            else if (d < second) second = d;
        }
        float cracks = 1.0 - smoothstep(0.035, 0.105, second - nearest);
        vec3 redEarth = mix(vec3(0.72, 0.70, 0.66), vec3(0.86, 0.82, 0.72), duneT);
        redEarth *= 0.62 + diffuse * 0.58;
        redEarth = mix(redEarth, vec3(0.10, 0.075, 0.075), cracks * 0.88);
        color = mix(color, redEarth, u_redDesert);

        float fogT = smoothstep(130.0, 380.0, v_worldPos.z);
        color = mix(color, vec3(0.70, 0.50, 0.28), fogT * 0.42);

        vec3 greyGround = vec3(0.34, 0.35, 0.34);
        vec3 greyRock = vec3(0.16, 0.17, 0.16);
        vec3 greyColor = mix(greyGround, greyRock, slope * 0.76);
        greyColor *= 0.48 + diffuse * 0.66;
        greyColor = mix(greyColor, vec3(0.62, 0.63, 0.61), fogT * 0.48);
        color = mix(color, greyColor, u_greyDesert);
        vec3 ruinedCity = mix(vec3(0.10, 0.11, 0.12), vec3(0.29, 0.25, 0.23), diffuse);
        ruinedCity = mix(ruinedCity, mix(vec3(0.18, 0.035, 0.055), vec3(0.34, 0.10, 0.18), diffuse), u_redDesert);
        if (u_redMountain > 0.5)
            color = mix(vec3(0.25, 0.055, 0.035), vec3(0.52, 0.16, 0.09), diffuse);
        color = mix(color, ruinedCity, u_ruinCity);

        FragColor = vec4(color, 1.0);
    }
)";

const char *DesertTerrain::RUIN_SMOKE_VERTEX_SHADER = GLSL_VERSION R"(
precision highp float; layout(location=0) in vec3 a_pos; layout(location=1) in vec2 a_uv; layout(location=2) in vec4 a_params;
uniform mat4 u_worldToView; uniform mat4 u_projection; uniform float u_time; out vec2 v_uv; out float v_alpha; out float v_seed; out float v_top;
void main(){ float age=fract(a_params.x+u_time*.085); float grow=smoothstep(0.,1.,age); vec2 c=a_uv*2.-1.; vec3 p=a_pos; p.x+=c.x*a_params.y*(.3+grow*1.05)+sin(u_time*.22+a_params.x*13.)*1.8+grow*8.5; p.y+=c.y*a_params.z*(.3+grow*1.1)+grow*43.; p.z+=sin(u_time*.14+a_params.x*17.)*1.4; v_uv=a_uv; v_alpha=smoothstep(0.,.16,age)*(1.-smoothstep(.72,1.,age))*a_params.w; v_seed=a_params.x; v_top=smoothstep(.15,1.,a_uv.y); gl_Position=u_projection*u_worldToView*vec4(p,1.); }
)";
const char *DesertTerrain::RUIN_SMOKE_FRAGMENT_SHADER = GLSL_VERSION R"(
precision highp float; in vec2 v_uv; in float v_alpha; in float v_seed; in float v_top; out vec4 FragColor;
float hash21(vec2 p){p=fract(p*vec2(123.34,456.21));p+=dot(p,p+45.32);return fract(p.x*p.y);} float noise(vec2 p){vec2 i=floor(p),f=fract(p);f=f*f*(3.-2.*f);return mix(mix(hash21(i),hash21(i+vec2(1,0)),f.x),mix(hash21(i+vec2(0,1)),hash21(i+vec2(1,1)),f.x),f.y);}
void main(){vec2 p=v_uv*2.-1.;float d=length(p);float cloud=(1.-smoothstep(.08,1.18,d))*(1.-smoothstep(.46,1.18,d));float n=noise(v_uv*5.+v_seed);float a=cloud*smoothstep(.2,.82,n)*v_alpha*.58*(1.-v_top*.52);if(a<.006)discard;FragColor=vec4(mix(vec3(.22,.19,.18),vec3(.68,.66,.62),v_top),a);}
)";
const char *DesertTerrain::RUIN_SPARK_VERTEX_SHADER = GLSL_VERSION R"(
precision highp float; layout(location=0) in vec3 a_origin; layout(location=1) in vec4 a_params; uniform mat4 u_worldToView; uniform mat4 u_projection; uniform float u_time; out float v_alpha;
void main(){float q=fract(a_params.x+u_time*.18);float risePhase=clamp(q/.58,0.,1.);vec3 p=a_origin;p.x+=sin(u_time*3.1+a_params.x*37.)*.16+a_params.z*q;p.y+=a_params.y*risePhase;p.z+=cos(u_time*2.4+a_params.x*29.)*.1;vec4 v=u_worldToView*vec4(p,1.);float sizeScale=mix(8.,.55,risePhase);gl_Position=u_projection*v;gl_PointSize=a_params.w*sizeScale*clamp(72./max(18.,-v.z),.35,1.6);v_alpha=smoothstep(0.,.035,q)*(1.-smoothstep(.36,.58,q));}
)";
const char *DesertTerrain::RUIN_SPARK_FRAGMENT_SHADER = GLSL_VERSION R"(
precision highp float; in float v_alpha; out vec4 FragColor; void main(){float d=length(gl_PointCoord*2.-1.);float a=(1.-smoothstep(.25,1.,d))*v_alpha;if(a<.01)discard;FragColor=vec4(1.,.22+.55*(1.-d),.025,a);}
)";
