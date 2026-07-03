#pragma once

#include <cmath>
#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

#include "mesh.h"
#include "texture.h"

struct TrafficBoxMesh
{
    AssetMesh mesh;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    MeshData meshData = {};
};

struct TrafficCarState
{
    int side = 0;
    int lane = 0;
    float speed = 0.0f;
};

struct Traffic
{
    TrafficBoxMesh groundMesh;
    TrafficBoxMesh roadMesh;
    TrafficBoxMesh carMesh;
    std::vector<TrafficCarState> cars;
    bool generated = false;

    static constexpr int kSides = 2;
    static constexpr int kLanesPerSide = 3;
    static constexpr int kCarsPerLane = 9;
    // Center of each road deck from world origin on X.
    // Lower values pull the whole traffic block closer to the bowling lane.
    static constexpr float kRoadCenterX = 3.0f;
    // Full road slab width on X for one side.
    static constexpr float kRoadWidth = 5.00f;
    // Vertical placement of the traffic roads and cars.
    static constexpr float kRoadY = -15.0f;
    static constexpr float kRoadLength = 360.0f;
    static constexpr float kRoadThickness = 0.10f;
    // Distance between neighboring traffic lanes on X inside one road slab.
    static constexpr float kLaneSpacing = 1.6;
    static constexpr float kTrafficExtent = 120.0f;
    // Big city underlay so the lower skyline sits over grey ground instead of open sky.
    // Kept 0.5m below the traffic roads.
    static constexpr float kGroundY = -15.5f;
    static constexpr float kGroundWidth = 140.0f;
    static constexpr float kGroundLength = 360.0f;
    static constexpr float kGroundThickness = 0.12f;

    static uint32_t hash32(uint32_t x)
    {
        x ^= x >> 16;
        x *= 0x7feb352dU;
        x ^= x >> 15;
        x *= 0x846ca68bU;
        x ^= x >> 16;
        return x;
    }

    static float hash01(int a, int b)
    {
        uint32_t h = hash32(uint32_t(a) * 73856093U ^ uint32_t(b) * 19349663U ^ 0x85ebca6bU);
        return float(h & 0x00ffffffU) / float(0x01000000U);
    }

    void loadTrafficShader()
    {
        // Traffic uses the shared main mesh shader.
    }

    void initTraffic()
    {
        this->generated = false;
        ensureGeometry();
    }

    void ensureGeometry()
    {
        if (generated)
            return;

        buildBoxMesh(this->groundMesh, 1.0f, 1.0f, 1.0f);
        buildBoxMesh(this->roadMesh, 1.0f, 1.0f, 1.0f);
        buildBoxMesh(this->carMesh, 1.0f, 1.0f, 1.0f);
        buildGroundInstances();
        buildRoadInstances();
        buildCarInstances();
        generated = true;
    }

    static void buildBoxMesh(TrafficBoxMesh &outMesh, float width, float height, float depth)
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

        addQuad(glm::vec3(-hx, -hy, -hz), glm::vec3(hx, -hy, -hz), glm::vec3(hx, hy, -hz), glm::vec3(-hx, hy, -hz),
                glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec2(0.0f, 1.0f));
        addQuad(glm::vec3(-hx, -hy, hz), glm::vec3(-hx, hy, hz), glm::vec3(hx, hy, hz), glm::vec3(hx, -hy, hz),
                glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 1.0f), glm::vec2(1.0f, 1.0f), glm::vec2(1.0f, 0.0f));
        addQuad(glm::vec3(-hx, hy, -hz), glm::vec3(hx, hy, -hz), glm::vec3(hx, hy, hz), glm::vec3(-hx, hy, hz),
                glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec2(0.0f, 1.0f));
        addQuad(glm::vec3(-hx, -hy, -hz), glm::vec3(-hx, -hy, hz), glm::vec3(hx, -hy, hz), glm::vec3(hx, -hy, -hz),
                glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 1.0f), glm::vec2(1.0f, 1.0f), glm::vec2(1.0f, 0.0f));
        addQuad(glm::vec3(-hx, -hy, -hz), glm::vec3(-hx, hy, -hz), glm::vec3(-hx, hy, hz), glm::vec3(-hx, -hy, hz),
                glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 1.0f), glm::vec2(1.0f, 1.0f), glm::vec2(1.0f, 0.0f));
        addQuad(glm::vec3(hx, -hy, -hz), glm::vec3(hx, -hy, hz), glm::vec3(hx, hy, hz), glm::vec3(hx, hy, -hz),
                glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec2(0.0f, 1.0f));

        outMesh.meshData.vertexCount = uint32_t(outMesh.vertices.size());
        outMesh.meshData.indexCount = uint32_t(outMesh.indices.size());
        outMesh.meshData.vertices = outMesh.vertices.data();
        outMesh.meshData.indices = outMesh.indices.data();
        outMesh.mesh.sendMeshDataToGpu(&outMesh.meshData);
    }

    void buildGroundInstances()
    {
        this->groundMesh.mesh.instanceData.clear();
        this->groundMesh.mesh.instanceData.reserve(1);
        InstanceData inst{};
        inst.instRot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        inst.textureScale = glm::vec3(0.45f, 0.02f, 2.8f);
        inst.positionOffset = glm::vec3(0.0f, kGroundY, 0.0f);
        inst.scaleOffset = glm::vec3(kGroundWidth, kGroundThickness, kGroundLength);
        inst.atlasStart = glm::vec2(0.0f);
        this->groundMesh.mesh.instanceData.push_back(inst);
        this->groundMesh.mesh.sendInstanceDataToGpu();
    }

    void buildRoadInstances()
    {
        this->roadMesh.mesh.instanceData.clear();
        this->roadMesh.mesh.instanceData.reserve(2);
        for (int side = 0; side < kSides; ++side)
        {
            float sideSign = side == 0 ? -1.0f : 1.0f;
            InstanceData inst{};
            inst.instRot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            inst.textureScale = glm::vec3(0.30f, 0.02f, 2.5f);
            inst.positionOffset = glm::vec3(sideSign * kRoadCenterX, kRoadY, 0.0f);
            inst.scaleOffset = glm::vec3(kRoadWidth, kRoadThickness, kRoadLength);
            inst.atlasStart = glm::vec2(0.0f);
            this->roadMesh.mesh.instanceData.push_back(inst);
        }
        this->roadMesh.mesh.sendInstanceDataToGpu();
    }

    void buildCarInstances()
    {
        this->carMesh.mesh.instanceData.clear();
        this->cars.clear();
        this->carMesh.mesh.instanceData.reserve(kSides * kLanesPerSide * kCarsPerLane);
        this->cars.reserve(kSides * kLanesPerSide * kCarsPerLane);

        for (int side = 0; side < kSides; ++side)
        {
            float sideSign = side == 0 ? -1.0f : 1.0f;
            for (int lane = 0; lane < kLanesPerSide; ++lane)
            {
                float laneOffset = (float(lane) - float(kLanesPerSide - 1) * 0.5f) * kLaneSpacing;
                for (int idx = 0; idx < kCarsPerLane; ++idx)
                {
                    int seedA = side * 1000 + lane * 100 + idx;
                    float sizeNoise = hash01(seedA, 3);
                    float speedNoise = hash01(seedA, 9);
                    float phaseNoise = hash01(seedA, 27);

                    // Car body width on X.
                    float carWidth = glm::mix(0.7875f, 1.20f, sizeNoise);
                    float carHeight = glm::mix(0.50f, 0.90f, sizeNoise);
                    float carLength = glm::mix(2.00f, 3.40f, sizeNoise);
                    float speed = glm::mix(5.0f, 11.0f, speedNoise);
                    float startZ = glm::mix(-kTrafficExtent, kTrafficExtent, phaseNoise);

                    InstanceData inst{};
                    inst.instRot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
                    inst.textureScale = glm::vec3(0.16f, 0.16f, 0.16f);
                    inst.positionOffset = glm::vec3(sideSign * kRoadCenterX + laneOffset, kRoadY + 0.45f, startZ);
                    inst.scaleOffset = glm::vec3(carWidth, carHeight, carLength);
                    inst.atlasStart = glm::vec2(0.0f);
                    this->carMesh.mesh.instanceData.push_back(inst);

                    TrafficCarState state{};
                    state.side = side;
                    state.lane = lane;
                    state.speed = side == 0 ? speed : -speed;
                    this->cars.push_back(state);
                }
            }
        }

        this->carMesh.mesh.sendInstanceDataToGpu();
    }

    void update(float deltaTime)
    {
        this->ensureGeometry();
        for (size_t i = 0; i < this->cars.size() && i < this->carMesh.mesh.instanceData.size(); ++i)
        {
            InstanceData &inst = this->carMesh.mesh.instanceData[i];
            const TrafficCarState &state = this->cars[i];
            inst.positionOffset.z += state.speed * deltaTime;
            if (inst.positionOffset.z > kTrafficExtent)
                inst.positionOffset.z -= 2.0f * kTrafficExtent;
            if (inst.positionOffset.z < -kTrafficExtent)
                inst.positionOffset.z += 2.0f * kTrafficExtent;
        }
        this->carMesh.mesh.sendInstanceDataToGpu();
    }

    void renderTraffic3d(
        ShaderProgram &shader,
        Texture &diffuseTexture,
        const glm::mat4 &viewMatrix,
        const glm::mat4 &projectionMatrix)
    {
        this->ensureGeometry();

        shader.updateDiffuseTexture(diffuseTexture);
        shader.updateUseTextureAlpha(false);
        shader.updateTextureParamsInOneGo(
            glm::vec3(0.45f, 0.02f, 2.8f),
            glm::vec2(1.0f, 1.0f),
            glm::vec2(0.0f, 0.0f),
            1.0f
        );
        shader.updateColorTintMix(glm::vec3(0.24f, 0.25f, 0.28f), 0.92f, 1.0f);
        shader.renderRealMesh(this->groundMesh.mesh, glm::mat4(1.0f), viewMatrix, projectionMatrix);

        shader.updateTextureParamsInOneGo(
            glm::vec3(0.30f, 0.02f, 2.5f),
            glm::vec2(1.0f, 1.0f),
            glm::vec2(0.0f, 0.0f),
            1.0f
        );
        shader.updateColorTintMix(glm::vec3(0.10f, 0.12f, 0.16f), 0.88f, 1.0f);
        shader.renderRealMesh(this->roadMesh.mesh, glm::mat4(1.0f), viewMatrix, projectionMatrix);

        shader.updateTextureParamsInOneGo(
            glm::vec3(0.16f, 0.16f, 0.16f),
            glm::vec2(1.0f, 1.0f),
            glm::vec2(0.0f, 0.0f),
            1.0f
        );
        shader.updateColorTintMix(glm::vec3(0.98f, 0.48f, 0.18f), 0.78f, 1.0f);
        shader.renderRealMesh(this->carMesh.mesh, glm::mat4(1.0f), viewMatrix, projectionMatrix);
        shader.updateColorTintMix(glm::vec3(1.0f), 0.0f, 1.0f);
    }
};
