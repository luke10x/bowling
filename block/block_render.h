#pragma once

#include <cmath>
#include <vector>

#include "block.h"
#include "../mesh.h"

struct FracturedBlockRenderFragment
{
    AssetMesh mesh;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    MeshData meshData = {};
};

inline void Block_ClearRenderFragments(std::vector<FracturedBlockRenderFragment> &fragments)
{
    for (FracturedBlockRenderFragment &fragment : fragments)
        fragment.mesh.releaseGpu();
    fragments.clear();
}

inline void Block_BuildRenderFragmentMesh(
    FracturedBlockRenderFragment &outFragment,
    const FracturedBlockFragmentGeometry &geom,
    float blockWidth,
    float blockHeight,
    float blockThickness
)
{
    outFragment.mesh.releaseGpu();
    outFragment.vertices.clear();
    outFragment.indices.clear();

    if (geom.frontFace.size() < 3)
        return;

    glm::vec2 uvMin = geom.frontFace[0];
    glm::vec2 uvMax = geom.frontFace[0];
    for (const glm::vec2 &p : geom.frontFace)
    {
        uvMin = glm::min(uvMin, p);
        uvMax = glm::max(uvMax, p);
    }
    const glm::vec2 uvExtent = glm::max(uvMax - uvMin, glm::vec2(1.0e-4f));

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
        outFragment.vertices.push_back(v);
        return uint32_t(outFragment.vertices.size() - 1);
    };

    auto addTriangle = [&](uint32_t a, uint32_t b, uint32_t c)
    {
        outFragment.indices.push_back(a);
        outFragment.indices.push_back(b);
        outFragment.indices.push_back(c);
    };

    const float halfThickness = 0.5f * blockThickness;
    const float thicknessRatio = glm::max(0.001f, blockThickness / glm::max(blockWidth, 1.0e-4f));

    std::vector<uint32_t> frontIndices;
    std::vector<uint32_t> backIndices;
    frontIndices.reserve(geom.frontFace.size());
    backIndices.reserve(geom.frontFace.size());

    for (const glm::vec2 &p : geom.frontFace)
    {
        const glm::vec2 uv = (p - uvMin) / uvExtent;
        frontIndices.push_back(pushVertex(glm::vec3(p.x, p.y, -halfThickness), glm::vec3(0.0f, 0.0f, -1.0f), uv));
        backIndices.push_back(pushVertex(glm::vec3(p.x, p.y, halfThickness), glm::vec3(0.0f, 0.0f, 1.0f), uv));
    }

    for (size_t i = 1; i + 1 < frontIndices.size(); ++i)
        addTriangle(frontIndices[0], frontIndices[i], frontIndices[i + 1]);
    for (size_t i = 1; i + 1 < backIndices.size(); ++i)
        addTriangle(backIndices[0], backIndices[i + 1], backIndices[i]);

    for (size_t i = 0; i < geom.frontFace.size(); ++i)
    {
        const size_t next = (i + 1) % geom.frontFace.size();
        const glm::vec2 &a2 = geom.frontFace[i];
        const glm::vec2 &b2 = geom.frontFace[next];
        const glm::vec3 aFront(a2.x, a2.y, -halfThickness);
        const glm::vec3 bFront(b2.x, b2.y, -halfThickness);
        const glm::vec3 aBack(a2.x, a2.y, halfThickness);
        const glm::vec3 bBack(b2.x, b2.y, halfThickness);
        glm::vec3 normal = glm::normalize(glm::cross(bFront - aFront, aBack - aFront));
        if (!std::isfinite(normal.x) || !std::isfinite(normal.y) || !std::isfinite(normal.z))
            normal = glm::vec3(1.0f, 0.0f, 0.0f);

        const glm::vec2 edge = b2 - a2;
        const bool horizontalLike = std::abs(edge.x) >= std::abs(edge.y);
        glm::vec2 uv0(0.0f, 0.0f);
        glm::vec2 uv1(1.0f, 0.0f);
        glm::vec2 uv2(1.0f, thicknessRatio);
        glm::vec2 uv3(0.0f, thicknessRatio);
        if (!horizontalLike)
        {
            uv0 = glm::vec2(0.0f, 0.0f);
            uv1 = glm::vec2(0.0f, 1.0f);
            uv2 = glm::vec2(thicknessRatio, 1.0f);
            uv3 = glm::vec2(thicknessRatio, 0.0f);
        }

        const uint32_t i0 = pushVertex(aFront, normal, uv0);
        const uint32_t i1 = pushVertex(bFront, normal, uv1);
        const uint32_t i2 = pushVertex(bBack, normal, uv2);
        const uint32_t i3 = pushVertex(aBack, normal, uv3);
        addTriangle(i0, i1, i2);
        addTriangle(i0, i2, i3);
    }

    outFragment.meshData.vertexCount = uint32_t(outFragment.vertices.size());
    outFragment.meshData.indexCount = uint32_t(outFragment.indices.size());
    outFragment.meshData.vertices = outFragment.vertices.data();
    outFragment.meshData.indices = outFragment.indices.data();
    outFragment.mesh.sendMeshDataToGpu(&outFragment.meshData);
}

inline void Block_RebuildRenderFragments(
    std::vector<FracturedBlockRenderFragment> &outFragments,
    const std::vector<FracturedBlockFragmentGeometry> &fragments,
    float blockWidth,
    float blockHeight,
    float blockThickness
)
{
    Block_ClearRenderFragments(outFragments);
    outFragments.resize(fragments.size());
    for (size_t i = 0; i < fragments.size(); ++i)
    {
        Block_BuildRenderFragmentMesh(
            outFragments[i],
            fragments[i],
            blockWidth,
            blockHeight,
            blockThickness
        );
    }
}
