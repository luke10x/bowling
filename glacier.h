#pragma once

#include <cmath>
#include <cstdint>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "mesh.h"
#include "texture.h"

struct GlacierBoxMesh
{
    AssetMesh mesh;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    MeshData meshData = {};
};

struct GlacierBackdrop
{
    GlacierBoxMesh glacierMesh;
    float scrollZ = 0.0f;
    bool generated = false;

    static constexpr int kRows = 22;
    static constexpr int kColsPerSide = 6;
    static constexpr float kRowSpacing = 13.0f;
    static constexpr float kColSpacing = 4.8f;

    // static constexpr int   kMiddleCols   = 4;      // number of potential middle positions per row
    // static constexpr float kMaxMiddleX   = 10.0f;  // maximum distance from center for middle glaciers
    static constexpr int   kMiddleCols   = 30;      // was 4 → ~7.5× more candidates
    static constexpr float kMaxMiddleX   = 14.0f;   // wider search range for clusters

    static constexpr float kSideBaseX = 22.5f;
    static constexpr float kDefaultWaterLineY = -20.0f;
    static constexpr float kScrollSpeed = 0.45f;
    float waterLineY = kDefaultWaterLineY;

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
        uint32_t h = hash32(uint32_t(x) * 73856093U ^ uint32_t(z) * 19349663U ^ 0x7f4a7c15U);
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
        for (int i = 0; i < 4; ++i)
        {
            sum += valueNoise(x * freq, z * freq) * amp;
            freq *= 2.07f;
            amp *= 0.5f;
        }
        return sum;
    }

    void loadGlacierShader()
    {
        // Glacier backdrop uses the shared mesh shader.
    }

    void initGlacier()
    {
        this->scrollZ = 0.0f;
        this->generated = false;
        ensureGeometry();
    }

    void setWaterLineY(float y)
    {
        if (std::abs(this->waterLineY - y) <= 1.0e-4f)
            return;
        this->waterLineY = y;
        if (!generated)
            return;
        rebuildInstances();
        generated = true;
    }

    void ensureGeometry()
    {
        if (generated)
            return;

        buildUnitBoxMesh(this->glacierMesh, 1.0f, 1.0f, 1.0f);
        rebuildInstances();
        generated = true;
    }

    static void buildUnitBoxMesh(GlacierBoxMesh &outMesh, float width, float height, float depth)
    {
        outMesh.mesh.releaseGpu();
        outMesh.vertices.clear();
        outMesh.indices.clear();

        auto pushVertex = [&](const glm::vec3 &pos, const glm::vec3 &normal, const glm::vec2 &uv)
        {
            Vertex v{};
            v.position.x = pos.x;
            v.position.y = pos.y;
            v.position.z = pos.z;
            v.color.r = 1.0f;
            v.color.g = 1.0f;
            v.color.b = 1.0f;
            v.color.a = 1.0f;
            v.texCoords.u = uv.x;
            v.texCoords.v = uv.y;
            v.normal.x = normal.x;
            v.normal.y = normal.y;
            v.normal.z = normal.z;
            outMesh.vertices.push_back(v);
            return uint32_t(outMesh.vertices.size() - 1);
        };

        auto addQuad = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, glm::vec3 normal,
                           glm::vec2 uva, glm::vec2 uvb, glm::vec2 uvc, glm::vec2 uvd)
        {
            uint32_t i0 = pushVertex(a, normal, uva);
            uint32_t i1 = pushVertex(b, normal, uvb);
            uint32_t i2 = pushVertex(c, normal, uvc);
            uint32_t i3 = pushVertex(d, normal, uvd);
            outMesh.indices.push_back(i0);
            outMesh.indices.push_back(i1);
            outMesh.indices.push_back(i2);
            outMesh.indices.push_back(i0);
            outMesh.indices.push_back(i2);
            outMesh.indices.push_back(i3);
        };

        const float hx = 0.5f * width;
        const float hy = 0.5f * height;
        const float hz = 0.5f * depth;

        addQuad(
            glm::vec3(-hx, -hy, -hz), glm::vec3(hx, -hy, -hz),
            glm::vec3(hx, hy, -hz), glm::vec3(-hx, hy, -hz),
            glm::vec3(0.0f, 0.0f, -1.0f),
            glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.0f),
            glm::vec2(1.0f, 1.0f), glm::vec2(0.0f, 1.0f)
        );
        addQuad(
            glm::vec3(-hx, -hy, hz), glm::vec3(-hx, hy, hz),
            glm::vec3(hx, hy, hz), glm::vec3(hx, -hy, hz),
            glm::vec3(0.0f, 0.0f, 1.0f),
            glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 1.0f),
            glm::vec2(1.0f, 1.0f), glm::vec2(1.0f, 0.0f)
        );
        addQuad(
            glm::vec3(-hx, hy, -hz), glm::vec3(hx, hy, -hz),
            glm::vec3(hx, hy, hz), glm::vec3(-hx, hy, hz),
            glm::vec3(0.0f, 1.0f, 0.0f),
            glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.0f),
            glm::vec2(1.0f, 1.0f), glm::vec2(0.0f, 1.0f)
        );
        addQuad(
            glm::vec3(-hx, -hy, -hz), glm::vec3(-hx, -hy, hz),
            glm::vec3(hx, -hy, hz), glm::vec3(hx, -hy, -hz),
            glm::vec3(0.0f, -1.0f, 0.0f),
            glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 1.0f),
            glm::vec2(1.0f, 1.0f), glm::vec2(1.0f, 0.0f)
        );
        addQuad(
            glm::vec3(-hx, -hy, -hz), glm::vec3(-hx, hy, -hz),
            glm::vec3(-hx, hy, hz), glm::vec3(-hx, -hy, hz),
            glm::vec3(-1.0f, 0.0f, 0.0f),
            glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 1.0f),
            glm::vec2(1.0f, 1.0f), glm::vec2(1.0f, 0.0f)
        );
        addQuad(
            glm::vec3(hx, -hy, -hz), glm::vec3(hx, -hy, hz),
            glm::vec3(hx, hy, hz), glm::vec3(hx, hy, -hz),
            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.0f),
            glm::vec2(1.0f, 1.0f), glm::vec2(0.0f, 1.0f)
        );

        outMesh.meshData.vertexCount = uint32_t(outMesh.vertices.size());
        outMesh.meshData.indexCount = uint32_t(outMesh.indices.size());
        outMesh.meshData.vertices = outMesh.vertices.data();
        outMesh.meshData.indices = outMesh.indices.data();
        outMesh.mesh.sendMeshDataToGpu(&outMesh.meshData);
    }

    void rebuildInstances()
    {
        this->glacierMesh.mesh.instanceData.clear();
        this->glacierMesh.mesh.instanceData.reserve(kRows * (kColsPerSide * 2 + kMiddleCols));

        for (int row = 0; row < kRows; ++row)
        {
            // ---- side glaciers (existing code) ----
            for (int side = 0; side < 2; ++side)
            {
                const float sideSign = side == 0 ? -1.0f : 1.0f;
                const float rowWiggle = (fbm(float(side) * 3.1f + 1.0f, float(row) * 0.11f + 5.0f) - 0.5f) * 4.5f;
                for (int col = 0; col < kColsPerSide; ++col)
                {
                    const int sideCol = side * 100 + col;
                    const float widthNoise = fbm(float(col) * 0.29f + 4.0f, float(row) * 0.13f);
                    const float depthNoise = fbm(float(col) * 0.17f + 9.0f, float(row) * 0.19f + 2.0f);
                    const float heightNoise = fbm(float(col) * 0.09f + 13.0f, float(row) * 0.07f + 6.0f);
                    const float jitter = hash01(sideCol, row);
                    const float lean = hash01(sideCol + 17, row + 53) - 0.5f;
                    const float profileRoll = hash01(sideCol + 401, row + 13);
                    const float xNoise = fbm(float(sideCol) * 0.23f + 7.0f, float(row) * 0.15f + 12.0f) - 0.5f;
                    const float zNoise = fbm(float(sideCol) * 0.19f + 17.0f, float(row) * 0.17f + 21.0f) - 0.5f;
                    const float baseAbsX = kSideBaseX + rowWiggle + float(col) * kColSpacing + xNoise * 3.0f + jitter * 1.6f;
                    const float absX = glm::max(4.5f, baseAbsX);
                    const float densityT = glm::smoothstep(20.0f, 30.0f, absX);
                    const float keepChance = glm::mix(0.18f, 0.97f, densityT);
                    if (hash01(sideCol + 211, row + 97) > keepChance)
                        continue;

                    const float outerT = glm::smoothstep(25.0f, 38.0f, absX);
                    float width = glm::mix(3.8f, 10.0f, glm::clamp(widthNoise, 0.0f, 1.0f));
                    float depth = glm::mix(4.8f, 14.5f, glm::clamp(depthNoise, 0.0f, 1.0f));
                    float height = glm::mix(8.0f, 22.0f, glm::clamp(heightNoise, 0.0f, 1.0f));
                    height *= glm::mix(0.55f, 1.75f, outerT);
                    height += glm::mix(0.0f, 10.0f, outerT) * glm::clamp(heightNoise, 0.0f, 1.0f);

                    if (profileRoll < 0.26f)
                    {
                        width *= 1.7f;
                        depth *= 1.35f;
                        height *= 0.62f;
                    }
                    else if (profileRoll > 0.76f)
                    {
                        width *= 0.58f;
                        depth *= 0.72f;
                        height *= 1.75f;
                    }

                    const float x = sideSign * absX;
                    const float z = float(row) * kRowSpacing + zNoise * 8.0f + (jitter - 0.5f) * 6.0f;
                    const float y = this->waterLineY;

                    InstanceData inst{};
                    inst.instRot = glm::normalize(
                        glm::angleAxis(lean * 0.08f, glm::vec3(0.0f, 0.0f, 1.0f)) *
                        glm::angleAxis(lean * 0.03f, glm::vec3(0.0f, 1.0f, 0.0f))
                    );
                    inst.textureScale = glm::vec3(0.08f, 0.30f, 0.08f);
                    inst.positionOffset = glm::vec3(x, y, z);
                    inst.scaleOffset = glm::vec3(width, height, depth);
                    inst.atlasStart = glm::vec2(0.0f);
                    this->glacierMesh.mesh.instanceData.push_back(inst);
                }
            }

            // ---- NEW: clustered middle glaciers (X‑randomised per row) ----
            // Per‑row shift of the noise field → cluster positions vary a lot on X
            float rowOffsetX = hash01(row, 12345) * 30.0f;   // random shift of up to ±15u

            for (int mcol = 0; mcol < kMiddleCols; ++mcol)
            {
                // 1. Candidate X position – base grid plus heavy jitter
                float t = (float(mcol) + 0.5f) / float(kMiddleCols);
                float xPos = (t - 0.5f) * 2.0f * kMaxMiddleX;
                // BIG jitter: ±8u instead of ±2.5u → glaciers spread widely
                float jitterX = (hash01(mcol + 1000, row) - 0.5f) * 16.0f;
                xPos += jitterX;
                float absX = glm::abs(xPos);
                if (absX > kMaxMiddleX + 4.0f) continue; // safety margin

                // 2. Perlin cluster density – noise field is shifted by rowOffsetX
                //    This makes the clusters appear at random X locations per row.
                float clusterDensity = fbm(xPos * 0.06f + rowOffsetX, float(row) * 0.06f + 456.0f);
                clusterDensity = glm::smoothstep(0.3f, 0.8f, clusterDensity);

                // 3. Base probability: zero at centre, rises toward edges
                float prob = glm::smoothstep(0.0f, kMaxMiddleX, absX) * 0.95f;
                prob *= clusterDensity;
                prob = glm::clamp(prob, 0.0f, 0.98f);

                // 4. Keep or discard
                if (hash01(mcol + 3000, row + 5000) > prob)
                    continue;

                // 5. Z position – with extra scatter within the cluster
                float zJitter = (hash01(mcol + 7000, row + 8000) - 0.5f) * 8.0f;
                float zClusterOffset = (clusterDensity - 0.5f) * 4.0f;
                float z = float(row) * kRowSpacing + zJitter + zClusterOffset;

                // 6. Size – still low and symmetric about the water line
                float heightNoise = fbm(float(mcol) * 0.5f + 400.0f, float(row) * 0.3f + 500.0f);
                float height = glm::mix(0.8f, 5.0f, heightNoise);
                float widthNoise = fbm(float(mcol) * 0.4f + 600.0f, float(row) * 0.3f + 700.0f);
                float depthNoise = fbm(float(mcol) * 0.4f + 800.0f, float(row) * 0.3f + 900.0f);
                float width  = glm::mix(1.0f, 3.5f, widthNoise);
                float depth  = glm::mix(1.0f, 4.0f, depthNoise);

                // cluster bonus: slightly bigger in dense areas
                width  *= (1.0f + clusterDensity * 0.5f);
                depth  *= (1.0f + clusterDensity * 0.5f);
                height *= (1.0f + clusterDensity * 0.3f);

                // 7. Place at waterLineY (symmetric above/below)
                float lean = (hash01(mcol + 4000, row + 6000) - 0.5f) * 0.1f;
                float y = this->waterLineY;

                InstanceData inst{};
                inst.instRot = glm::normalize(glm::angleAxis(lean, glm::vec3(0.0f, 0.0f, 1.0f)));
                inst.textureScale = glm::vec3(0.08f, 0.30f, 0.08f);
                inst.positionOffset = glm::vec3(xPos, y, z);
                inst.scaleOffset = glm::vec3(width, height, depth);
                inst.atlasStart = glm::vec2(0.0f);
                this->glacierMesh.mesh.instanceData.push_back(inst);
            }
        }

        this->glacierMesh.mesh.sendInstanceDataToGpu();
    }

    void update(float deltaTime)
    {
        this->ensureGeometry();
        this->scrollZ += deltaTime * kScrollSpeed;

        const float cycle = kRows * kRowSpacing;
        for (InstanceData &inst : this->glacierMesh.mesh.instanceData)
        {
            float z = inst.positionOffset.z - deltaTime * kScrollSpeed;
            while (z < -kRowSpacing)
                z += cycle;
            inst.positionOffset.z = z;
        }

        this->glacierMesh.mesh.sendInstanceDataToGpu();
    }

    void renderGlacier3d(
        ShaderProgram &shader,
        Texture &diffuseTexture,
        const glm::mat4 &viewMatrix,
        const glm::mat4 &projectionMatrix)
    {
        this->ensureGeometry();

        shader.updateDiffuseTexture(diffuseTexture);
        shader.updateUseTextureAlpha(false);
        shader.updateTextureParamsInOneGo(
            glm::vec3(0.08f, 0.30f, 0.08f),
            glm::vec2(1.0f, 1.0f),
            glm::vec2(0.0f, 0.0f),
            1.0f
        );
        shader.updateColorTintMix(glm::vec3(0.86f, 0.94f, 1.0f), 0.90f, 1.0f);
        shader.renderRealMesh(this->glacierMesh.mesh, glm::mat4(1.0f), viewMatrix, projectionMatrix);
        shader.updateColorTintMix(glm::vec3(1.0f), 0.0f, 1.0f);
    }
};
