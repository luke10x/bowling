#pragma once

#include "../mesh.h"
#include "../coins.h"

void renderFlyingCollectables(
    ShaderProgram *mainShader,
    AssetMesh *coinMesh,
    AssetMesh *gemMesh,
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

    // ✅ Light position in VIEW SPACE (for ortho screen-space, view = identity)
    mainShader->updateLightPos(
        glm::vec3(screenWidth / 2.0f, screenHeight / 2.0f, 10.0f)
    );

    for (const auto &fly : coinLane->flyAnimations)
    {
        if (!fly.active)
            continue;

        if (!isAbove && fly.currentPos.y < hudLevel)
            continue;
        if (isAbove && fly.currentPos.y >= hudLevel)
            continue;

        glm::mat4 model =
            glm::translate(glm::mat4(1.0f), glm::vec3(fly.currentPos.x, fly.currentPos.y, 10.0f));
        model = glm::rotate(model, fly.rotationY, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(fly.currentScale * CoinFlyConfig::PIXEL_SIZE * pixelScale));

        AssetMesh *mesh = (fly.visualKind == CollectableVisualKind::Gem && gemMesh) ? gemMesh : coinMesh;
        mainShader->renderRealMesh(*mesh, model, identityView, orthoProj);
    }
}
