#pragma once

#include <glm/glm.hpp>

#include "../rendertexture.h"
#include "../minigames/count_masters/count_masters.h"
#include "clayton.h"
#include "claytheme.h"

struct ClayToTexDecalAtlas
{
    static inline constexpr int WIDTH = 512;
    static inline constexpr int HEIGHT = 512;
    static inline constexpr int COLS = 2;
    static inline constexpr int ROWS = CountMastersState::GATE_COUNT;

    // This atlas is rendered by Clay after the existing framebuffer-to-texture pass.
    // It is intentionally separate from the static/generated artwork atlas: this one is
    // dynamic text/Clay content that can already sample textures created by earlier passes.
    RenderTexture texture;
    bool initialized = false;

    void init()
    {
        texture.width = WIDTH;
        texture.height = HEIGHT;
        texture.renderTextureInit(false);
        initialized = true;
    }

    static inline glm::vec2 tileSize()
    {
        return glm::vec2(1.0f / float(COLS), 1.0f / float(ROWS));
    }

    static inline glm::vec2 atlasStartForChoice(int gateIndex, bool rightSide)
    {
        const glm::vec2 tile = tileSize();
        const int row = glm::clamp(gateIndex, 0, ROWS - 1);
        const int col = rightSide ? 1 : 0;
        return glm::vec2(float(col) * tile.x, 1.0f - float(row + 1) * tile.y);
    }

    static inline float atlasScaleForSingleCell()
    {
        // Keep texCoords inside the smallest cell, then compensate with texture density.
        return (1.0f / float(ROWS)) - 0.001f;
    }

    static inline glm::vec3 textureScaleForSingleCell()
    {
        const float s = atlasScaleForSingleCell();
        return glm::vec3(1.0f / s);
    }

    void renderCountMastersGateLabels(
        Clayton *clayton,
        const CountMastersState &cm,
        int screenWidth,
        int screenHeight,
        double deltaTime
    )
    {
        if (!initialized || clayton == nullptr)
            return;

        texture.bindForWriting();
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glDisable(GL_SCISSOR_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ClayArena_Reset(&clayton->clayArena);
        Clay_SetLayoutDimensions((Clay_Dimensions){float(WIDTH), float(HEIGHT)});
        Clay_BeginLayout();

        CLAY(
            CLAY_ID("ClayToTexDecalAtlasRoot"),
            {
                .layout = {
                    .sizing = {CLAY_SIZING_FIXED(WIDTH), CLAY_SIZING_FIXED(HEIGHT)},
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
                .backgroundColor = {0, 0, 0, 0},
            }
        )
        {
            for (int row = 0; row < CountMastersState::GATE_COUNT; ++row)
            {
                CLAY(
                    CLAY_IDI("ClayToTexDecalAtlasRow", row),
                    {
                        .layout = {
                            .sizing = {CLAY_SIZING_FIXED(WIDTH), CLAY_SIZING_FIXED(HEIGHT / ROWS)},
                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        },
                    }
                )
                {
                    for (int col = 0; col < COLS; ++col)
                    {
                        char label[16] = {};
                        const CountMastersGateChoice choice = (col == 0) ? cm.gates[row].left : cm.gates[row].right;
                        CountMastersState::FormatChoice(label, sizeof(label), choice);
                        Clay_String labelText = ClayArena_AllocString(&clayton->clayArena, label);
                        Clay_TextElementConfig textCfg = {
                            .textColor = {255, 255, 255, 255},
                            .fontId = CLAY_FONT_NOTO,
                            .fontSize = 64,
                            .letterSpacing = 1,
                            .wrapMode = CLAY_TEXT_WRAP_NONE,
                            .textAlignment = CLAY_TEXT_ALIGN_CENTER,
                        };

                        CLAY(
                            CLAY_IDI("ClayToTexDecalAtlasCell", row * COLS + col),
                            {
                                .layout = {
                                    .sizing = {CLAY_SIZING_FIXED(WIDTH / COLS), CLAY_SIZING_FIXED(HEIGHT / ROWS)},
                                    .padding = {18, 18, 18, 18},
                                    .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                },
                                .backgroundColor = {0, 0, 0, 0},
                            }
                        )
                        {
                            CLAY(
                                CLAY_IDI("ClayToTexDecalAtlasPill", row * COLS + col),
                                {
                                    .layout = {
                                        .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)},
                                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                    },
                                    .backgroundColor = {28, 86, 224, 230},
                                    .cornerRadius = {16, 16, 16, 16},
                                    .border = {.color = {168, 212, 255, 245}, .width = CLAY_BORDER_ALL(3)},
                                }
                            )
                            {
                                CLAY_TEXT(labelText, CLAY_TEXT_CONFIG(textCfg));
                            }
                        }
                    }
                }
            }
        }

        Clay_RenderCommandArray cmds = Clay_EndLayout();
        clayton->renderClayton(cmds, WIDTH, HEIGHT, deltaTime);
        glDisable(GL_SCISSOR_TEST);
        texture.unbind(screenWidth, screenHeight);
        glDepthMask(GL_TRUE);
    }
};
