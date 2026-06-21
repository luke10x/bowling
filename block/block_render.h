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
    const glm::vec2 uvBase = glm::vec2(0.5f * blockWidth, 0.5f * blockHeight);

    std::vector<uint32_t> frontIndices;
    std::vector<uint32_t> backIndices;
    frontIndices.reserve(geom.frontFace.size());
    backIndices.reserve(geom.frontFace.size());

    for (const glm::vec2 &p : geom.frontFace)
    {
        const glm::vec2 uv = (p + uvBase) / glm::vec2(blockWidth, blockHeight);
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

        const float edgeLen = glm::length(b2 - a2);
        const uint32_t i0 = pushVertex(aFront, normal, glm::vec2(0.0f, 0.0f));
        const uint32_t i1 = pushVertex(bFront, normal, glm::vec2(edgeLen / blockThickness, 0.0f));
        const uint32_t i2 = pushVertex(bBack, normal, glm::vec2(edgeLen / blockThickness, 1.0f));
        const uint32_t i3 = pushVertex(aBack, normal, glm::vec2(0.0f, 1.0f));
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
