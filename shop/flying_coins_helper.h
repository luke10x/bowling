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
    bool beforeHudPass,
    int hudLevel,
    float pixelScale
)
{
    // This function is called twice at different points in the frame: first
    // for rewards below the HUD edge in the world pass, then for rewards in
    // front of the HUD after Clay. Never mix the two sides in one batch.
    constexpr size_t kKindCount = 8;
    const std::array<AssetMesh *, kKindCount> kindMeshes = {{
        coinMesh, gemMesh ? gemMesh : coinMesh, runeBoomMesh, runeBoltMesh,
        runeFreezeMesh, runeSkullMesh, runeGuardPinsMesh, runeFootballMesh
    }};
    const auto visibleOnThisSide = [beforeHudPass, hudLevel](const CoinFlyAnimation &fly) {
        return fly.isVisible() && (fly.forceHudOverlay ? !beforeHudPass :
            (beforeHudPass ? fly.currentPos.y < hudLevel : fly.currentPos.y >= hudLevel));
    };
    const bool anyVisible = std::any_of(
        coinLane->flyAnimations.begin(), coinLane->flyAnimations.end(), visibleOnThisSide);
    if (!anyVisible)
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
    // Before the HUD, coins fly across the 3D scene and need both the atlas
    // alpha and partial opacity. The later HUD overlay uses full opacity.
    mainShader->updateUseTextureAlpha(beforeHudPass);
    mainShader->updateColorTintMix(
        glm::vec3(1.0f), 0.0f, beforeHudPass ? 0.70f : 1.0f);

    // ✅ Light position in VIEW SPACE (for ortho screen-space, view = identity)
    mainShader->updateLightPos(
        glm::vec3(screenWidth / 2.0f, screenHeight / 2.0f, 10.0f)
    );

    // Batch only adjacent rewards using the same mesh. Grouping every coin
    // ahead of every gem changed alpha/depth draw order when their flight
    // paths overlap, especially at the HUD boundary.
    static thread_local std::vector<InstanceData> runInstances;
    runInstances.clear();
    AssetMesh *runMesh = nullptr;
    const auto drawRun = [&]() {
        if (!runMesh || runInstances.empty())
            return;
        if (runInstances.size() == 1 && runMesh->instanceData.size() == 1)
        {
            // One reward needs no instance-buffer upload. Keep the original
            // single-item path cheaper than uploading a batch.
            const InstanceData &instance = runInstances.front();
            glm::mat4 model = glm::translate(identity, instance.positionOffset);
            model *= glm::mat4_cast(instance.instRot);
            model = glm::scale(model, instance.scaleOffset);
            mainShader->renderRealMesh(*runMesh, model, identity, orthoProj);
            return;
        }
        size_t batchIndex = 0;
        while (batchIndex < kindMeshes.size() && kindMeshes[batchIndex] != runMesh)
            ++batchIndex;
        if (batchIndex == kindMeshes.size())
            return;
        MeshInstanceBatch &gpuBatch = gpuBatches[batchIndex];
        gpuBatch.upload(*runMesh, runInstances);
        mainShader->renderRealMesh(*runMesh, identity, identity, orthoProj, &gpuBatch);
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
        if (mesh != runMesh)
        {
            drawRun();
            runInstances.clear();
            runMesh = mesh;
        }

        InstanceData instance{};
        instance.instRot = glm::angleAxis(fly.rotationY, glm::vec3(0.0f, 1.0f, 0.0f));
        instance.textureScale = glm::vec3(1.0f);
        instance.positionOffset = glm::vec3(fly.currentPos.x, fly.currentPos.y, 10.0f);
        instance.scaleOffset = glm::vec3(fly.currentScale * CoinFlyConfig::PIXEL_SIZE * pixelScale);
        runInstances.push_back(instance);
    }
    drawRun();
    if (beforeHudPass)
    {
        mainShader->updateUseTextureAlpha(false);
        mainShader->updateColorTintMix(glm::vec3(1.0f), 0.0f, 1.0f);
    }
}
