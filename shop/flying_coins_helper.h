#pragma once

#include "../mesh.h"
#include "../coins.h"
#include <algorithm>
#include <array>
#include <glm/gtc/quaternion.hpp>

void renderFlyingCollectables(
    ShaderProgram *mainShader,
    AssetMesh *coinMesh,
    AssetMesh *gemMesh,
    AssetMesh *runeBoomMesh,
    AssetMesh *runeBoltMesh,
    AssetMesh *runeFreezeMesh,
    AssetMesh *runeSkullMesh,
    AssetMesh *runeGuardPinsMesh,
    AssetMesh *runeFootballMesh,
    Texture *everythingTexture,
    CoinLane *coinLane,
    std::array<MeshInstanceBatch, 8> &gpuBatches,
    float screenWidth,
    float screenHeight,
    bool isAbove, /* vary */
    int hudLevel,
    float pixelScale
)
{
    // Flying rewards share geometry with the world meshes. Their dedicated
    // instance buffers leave each mesh's one-item instance data untouched.
    constexpr size_t kKindCount = 8;
    const std::array<AssetMesh *, kKindCount> kindMeshes = {{
        coinMesh, gemMesh ? gemMesh : coinMesh, runeBoomMesh, runeBoltMesh,
        runeFreezeMesh, runeSkullMesh, runeGuardPinsMesh, runeFootballMesh
    }};
    std::array<AssetMesh *, kKindCount> batchMeshes{};
    static thread_local std::array<std::vector<InstanceData>, kKindCount> batches;
    for (auto &batch : batches)
        batch.clear();
    size_t batchCount = 0;

    const auto visibleOnThisSide = [isAbove, hudLevel](const CoinFlyAnimation &fly) {
        return fly.isVisible() && (isAbove ? fly.currentPos.y < hudLevel
                                           : fly.currentPos.y >= hudLevel);
    };
    for (const CoinFlyAnimation &fly : coinLane->flyAnimations)
    {
        if (!visibleOnThisSide(fly))
            continue;

        const size_t kind = static_cast<size_t>(fly.visualKind);
        AssetMesh *mesh = kind < kindMeshes.size() && kindMeshes[kind]
            ? kindMeshes[kind] : coinMesh;
        if (!mesh)
            continue;
        size_t batchIndex = 0;
        while (batchIndex < batchCount && batchMeshes[batchIndex] != mesh)
            ++batchIndex;
        if (batchIndex == batchCount)
            batchMeshes[batchCount++] = mesh;

        InstanceData instance{};
        instance.instRot = glm::angleAxis(fly.rotationY, glm::vec3(0.0f, 1.0f, 0.0f));
        instance.textureScale = glm::vec3(1.0f);
        instance.positionOffset = glm::vec3(fly.currentPos.x, fly.currentPos.y, 10.0f);
        instance.scaleOffset = glm::vec3(fly.currentScale * CoinFlyConfig::PIXEL_SIZE * pixelScale);
        batches[batchIndex].push_back(instance);
    }
    if (batchCount == 0)
        return;

    const glm::mat4 orthoProj =
        glm::ortho(0.0f, screenWidth, 0.0f, screenHeight, -100.0f, 100.0f);
    const glm::mat4 identity(1.0f);

    // Bind texture
    mainShader->updateDiffuseTexture(*everythingTexture);
    mainShader->updateTextureParamsInOneGo(
        glm::vec3(1.0f, 1.0f, 1.0f),
        glm::vec2(1.0f, 1.0f),
        glm::vec2(1.0f),
        1.0f
    );
    mainShader->updateUseTextureAlpha(false);
    mainShader->updateColorTintMix(glm::vec3(1.0f), 0.0f, 1.0f);

    // ✅ Light position in VIEW SPACE (for ortho screen-space, view = identity)
    mainShader->updateLightPos(
        glm::vec3(screenWidth / 2.0f, screenHeight / 2.0f, 10.0f)
    );

    for (size_t batchIndex = 0; batchIndex < batchCount; ++batchIndex)
    {
        AssetMesh &mesh = *batchMeshes[batchIndex];
        const auto &batch = batches[batchIndex];
        if (batch.size() == 1 && mesh.instanceData.size() == 1)
        {
            // One reward needs no instance-buffer upload. Keep the original
            // single-item path cheaper than uploading and restoring a batch.
            const InstanceData &instance = batch.front();
            glm::mat4 model = glm::translate(identity, instance.positionOffset);
            model *= glm::mat4_cast(instance.instRot);
            model = glm::scale(model, instance.scaleOffset);
            mainShader->renderRealMesh(mesh, model, identity, orthoProj);
            continue;
        }
        MeshInstanceBatch &gpuBatch = gpuBatches[batchIndex];
        gpuBatch.upload(mesh, batch);
        mainShader->renderRealMesh(mesh, identity, identity, orthoProj, &gpuBatch);
    }
}
