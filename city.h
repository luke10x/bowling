#pragma once

#include <cmath>
#include <cstdint>

#include <glm/glm.hpp>

#include "mesh.h"
#include "texture.h"

struct CityBoxMesh
{
    AssetMesh mesh;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    MeshData meshData = {};
};

struct City
{
    CityBoxMesh towerMesh;
    float scrollZ = 0.0f;
    bool generated = false;

    static constexpr int kCityRows = 22;
    static constexpr int kCityColsPerSide = 8;
    static constexpr float kCityRowSpacing = 11.0f;
    static constexpr float kCityColSpacing = 6.8f;
    static constexpr float kCitySideBaseX = 8.5f;
    static constexpr float kCityBaseY = -18.0f;
    static constexpr float kCityDepth = 7.0f;
    static constexpr float kCityScrollSpeed = 1.2f;

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
        uint32_t h = hash32(uint32_t(x) * 73856093U ^ uint32_t(z) * 19349663U ^ 0x9e3779b9U);
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
            freq *= 2.03f;
            amp *= 0.5f;
        }
        return sum;
    }

    void loadCityShader()
    {
        // 3D city uses the main mesh shader; keep this for hot-reload symmetry with aurora.
    }

    void initCity()
    {
        this->scrollZ = 0.0f;
        this->generated = false;
        ensureGeometry();
    }

    void renderCity(float, const glm::mat4 &)
    {
        // No-op: previews can keep aurora-only backgrounds.
    }

    void ensureGeometry()
    {
        if (generated)
            return;

        buildUnitBoxMesh(this->towerMesh, 1.0f, 1.0f, 1.0f);
        rebuildInstances();
        generated = true;
    }

    static void buildUnitBoxMesh(CityBoxMesh &outMesh, float width, float height, float depth)
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
        this->towerMesh.mesh.instanceData.clear();
        this->towerMesh.mesh.instanceData.reserve(kCityRows * kCityColsPerSide * 2);

        for (int row = 0; row < kCityRows; ++row)
        {
            for (int side = 0; side < 2; ++side)
            {
                float sideSign = side == 0 ? -1.0f : 1.0f;
                for (int col = 0; col < kCityColsPerSide; ++col)
                {
                    int pairX = col;
                    int sideX = side * 100 + col;
                    float widthNoise = fbm(float(pairX) * 0.31f, float(row) * 0.17f);
                    float depthNoise = fbm(float(pairX) * 0.19f + 7.1f, float(row) * 0.23f + 3.8f);
                    float heightNoise = fbm(float(pairX) * 0.11f + 13.4f, float(row) * 0.09f + 5.2f);
                    float sideHeightJitter = glm::mix(0.92f, 1.08f, hash01(sideX + 41, row + 17));
                    float jitterNoise = hash01(sideX, row);

                    float width = glm::mix(3.0f, 8.5f, glm::clamp(widthNoise, 0.0f, 1.0f));
                    float depth = glm::mix(3.5f, 9.5f, glm::clamp(depthNoise, 0.0f, 1.0f));
                    float height = glm::mix(10.0f, 48.0f, std::pow(glm::clamp(heightNoise, 0.0f, 1.0f), 1.35f));
                    height *= sideHeightJitter;

                    float x = sideSign * (kCitySideBaseX + float(col) * kCityColSpacing + jitterNoise * 1.6f);
                    float z = float(row) * kCityRowSpacing + (jitterNoise - 0.5f) * 2.8f;
                    float y = kCityBaseY + 0.5f * height;

                    InstanceData inst{};
                    inst.instRot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
                    inst.textureScale = glm::vec3(0.08f, 0.30f, 0.08f);
                    inst.positionOffset = glm::vec3(x, y, z);
                    inst.scaleOffset = glm::vec3(width, height, depth);
                    inst.atlasStart = glm::vec2(0.0f);
                    this->towerMesh.mesh.instanceData.push_back(inst);
                }
            }
        }

        this->towerMesh.mesh.sendInstanceDataToGpu();
    }

    void update(float deltaTime)
    {
        this->ensureGeometry();
        this->scrollZ += deltaTime * kCityScrollSpeed;

        float cycle = kCityRows * kCityRowSpacing;
        for (InstanceData &inst : this->towerMesh.mesh.instanceData)
        {
            float z = inst.positionOffset.z - deltaTime * kCityScrollSpeed;
            while (z < -kCityRowSpacing)
                z += cycle;
            inst.positionOffset.z = z;
        }

        this->towerMesh.mesh.sendInstanceDataToGpu();
    }

    void renderCity3d(
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
        shader.updateColorTintMix(glm::vec3(0.18f, 0.82f, 0.88f), 0.72f, 1.0f);
        shader.renderRealMesh(this->towerMesh.mesh, glm::mat4(1.0f), viewMatrix, projectionMatrix);
        shader.updateColorTintMix(glm::vec3(1.0f), 0.0f, 1.0f);
    }
};
