#pragma once

#include "../mesh.h"
#include "../coins.h"

void renderFlyingCollectables(
    ShaderProgram *mainShader,
    AssetMesh *coinMesh,
    AssetMesh *gemMesh,
    AssetMesh *runeBoomMesh,
    AssetMesh *runeBoltMesh,
    AssetMesh *runeFreezeMesh,
    Texture *everythingTexture,
    CoinLane *coinLane,
    float screenWidth,
    float screenHeight,
    bool isAbove, /* vary */
    int hudLevel,
    float pixelScale
)
{
    

    // // Setup orthographic projection for screen-space coins
    glm::mat4 orthoProj =
        glm::ortho(0.0f, screenWidth, 0.0f, screenHeight, -100.0f, 100.0f);
    glm::mat4 identityView = glm::mat4(1.0f); // No camera transform for screen-space

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

    for (const auto &fly : coinLane->flyAnimations)
    {
        if (!fly.isVisible())
            continue;

        if (!isAbove && fly.currentPos.y < hudLevel)
            continue;
        if (isAbove && fly.currentPos.y >= hudLevel)
            continue;

        glm::mat4 model =
            glm::translate(glm::mat4(1.0f), glm::vec3(fly.currentPos.x, fly.currentPos.y, 10.0f));
        model = glm::rotate(model, fly.rotationY, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(fly.currentScale * CoinFlyConfig::PIXEL_SIZE * pixelScale));

        AssetMesh *mesh = coinMesh;
        if (fly.visualKind == CollectableVisualKind::Gem && gemMesh)
            mesh = gemMesh;
        else if (fly.visualKind == CollectableVisualKind::RuneBoom && runeBoomMesh)
        {
            mesh = runeBoomMesh;
        }
        else if (fly.visualKind == CollectableVisualKind::RuneBolt && runeBoltMesh)
        {
            mesh = runeBoltMesh;
        }
        else if (fly.visualKind == CollectableVisualKind::RuneFreeze && runeFreezeMesh)
        {
            mesh = runeFreezeMesh;
        }
        mainShader->updateUseTextureAlpha(false);
        mainShader->renderRealMesh(*mesh, model, identityView, orthoProj);
    }
    mainShader->updateUseTextureAlpha(false);
}
