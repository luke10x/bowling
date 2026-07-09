#pragma once

#include <glm/glm.hpp>

#include "../rendertexture.h"
#include "../minigames/count_masters/count_masters.h"
#include "../minigames/crowd_control/crowd_control.h"
#include "clayton.h"
#include "claytheme.h"

struct ClayToTexDecalAtlas
{
    static inline constexpr int WIDTH = 512;
    static inline constexpr int HEIGHT = 2048;
    static inline constexpr int COLS = 2;
    static inline constexpr int CHOICE_ROWS = CountMastersState::GATE_COUNT;
    static inline constexpr int CARD_ROWS = CrowdControlState::CARD_LABEL_SLOTS;
    static inline constexpr int CROWD_CONTROL_LABEL_ROWS = 4;

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
        return glm::vec2(1.0f / float(COLS), 1.0f / float(CHOICE_ROWS));
    }

    static inline glm::vec2 crowdCardTileSize()
    {
        return glm::vec2(1.0f, 1.0f / float(CARD_ROWS));
    }

    static inline constexpr float countMastersLabelAspect()
    {
        return 0.36f / 0.15f;
    }

    static inline constexpr float crowdControlLabelAspect()
    {
        return 0.36f / 0.22f;
    }

    static inline int countMastersLabelPixelHeight()
    {
        const int rowH = HEIGHT / CountMastersState::GATE_COUNT;
        const int cellW = WIDTH / COLS;
        return glm::clamp(
            int(float(cellW) / countMastersLabelAspect()),
            1,
            rowH
        );
    }

    static inline glm::vec2 countMastersLabelTileSize()
    {
        return glm::vec2(
            1.0f / float(COLS),
            float(countMastersLabelPixelHeight()) / float(HEIGHT)
        );
    }

    static inline int crowdControlLabelPixelHeight()
    {
        const int rowH = HEIGHT / CROWD_CONTROL_LABEL_ROWS;
        return glm::clamp(
            int(float(WIDTH) / crowdControlLabelAspect()),
            1,
            rowH
        );
    }

    static inline glm::vec2 crowdControlLabelTileSize()
    {
        return glm::vec2(
            1.0f,
            float(crowdControlLabelPixelHeight()) / float(HEIGHT)
        );
    }

    static inline glm::vec2 atlasStartForChoice(int gateIndex, bool rightSide)
    {
        const glm::vec2 tile = tileSize();
        const int row = glm::clamp(gateIndex, 0, CHOICE_ROWS - 1);
        const int col = rightSide ? 1 : 0;
        return glm::vec2(float(col) * tile.x, 1.0f - float(row + 1) * tile.y);
    }

    static inline glm::vec2 atlasStartForChoiceLabel(int gateIndex, bool rightSide)
    {
        const glm::vec2 rowTile = tileSize();
        const glm::vec2 labelTile = countMastersLabelTileSize();
        const int row = glm::clamp(gateIndex, 0, CHOICE_ROWS - 1);
        const int col = rightSide ? 1 : 0;
        const float rowStartY = 1.0f - float(row + 1) * rowTile.y;
        const float centeredLabelY = rowStartY + (rowTile.y - labelTile.y) * 0.5f;
        return glm::vec2(float(col) * rowTile.x, centeredLabelY);
    }

    static inline glm::vec2 atlasStartForCrowdControlCardSlot(int slot)
    {
        const glm::vec2 tile = crowdCardTileSize();
        const int row = glm::clamp(slot, 0, CARD_ROWS - 1);
        return glm::vec2(0.0f, 1.0f - float(row + 1) * tile.y);
    }

    static inline glm::vec2 atlasStartForCrowdControlLabelSlot(int slot)
    {
        const glm::vec2 labelTile = crowdControlLabelTileSize();
        const int row = glm::clamp(slot, 0, CROWD_CONTROL_LABEL_ROWS - 1);
        const float rowH = 1.0f / float(CROWD_CONTROL_LABEL_ROWS);
        const float rowStartY = 1.0f - float(row + 1) * rowH;
        const float centeredLabelY = rowStartY + (rowH - labelTile.y) * 0.5f;
        return glm::vec2(0.0f, centeredLabelY);
    }

    static inline float atlasScaleForSingleCell()
    {
        // Keep texCoords inside the smallest cell, then compensate with texture density.
        return (1.0f / float(CHOICE_ROWS)) - 0.001f;
    }

    static inline float atlasScaleForChoiceLabel()
    {
        return countMastersLabelTileSize().y - 0.001f;
    }

    static inline float cardAtlasScaleForSingleCell()
    {
        return (1.0f / float(CARD_ROWS)) - 0.001f;
    }

    static inline float atlasScaleForCrowdControlLabel()
    {
        return crowdControlLabelTileSize().y - 0.001f;
    }

    static inline glm::vec3 textureScaleForSingleCell()
    {
        const float s = atlasScaleForSingleCell();
        return glm::vec3(1.0f / s);
    }

    static inline glm::vec3 textureScaleForChoiceLabel()
    {
        const float s = atlasScaleForChoiceLabel();
        return glm::vec3(1.0f / s);
    }

    static inline glm::vec3 cardTextureScaleForSingleCell()
    {
        const float s = cardAtlasScaleForSingleCell();
        return glm::vec3(1.0f / s);
    }

    static inline glm::vec3 textureScaleForCrowdControlLabel()
    {
        const float s = atlasScaleForCrowdControlLabel();
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
                const int countMastersRowH = HEIGHT / CountMastersState::GATE_COUNT;
                const int countMastersCellW = WIDTH / COLS;
                const int labelH = countMastersLabelPixelHeight();
                CLAY(
                    CLAY_IDI("ClayToTexDecalAtlasRow", row),
                    {
                        .layout = {
                            .sizing = {CLAY_SIZING_FIXED(WIDTH), CLAY_SIZING_FIXED(countMastersRowH)},
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
                                    .sizing = {CLAY_SIZING_FIXED(WIDTH / COLS), CLAY_SIZING_FIXED(countMastersRowH)},
                                    .padding = {0, 0, 0, 0},
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
                                        .sizing = {CLAY_SIZING_FIXED((float)countMastersCellW), CLAY_SIZING_FIXED((float)labelH)},
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

    void renderCrowdControlCardLabels(
        Clayton *clayton,
        const CrowdControlState &cc,
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
            CLAY_ID("ClayToTexCrowdControlRoot"),
            {
                .layout = {
                    .sizing = {CLAY_SIZING_FIXED(WIDTH), CLAY_SIZING_FIXED(HEIGHT)},
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
                .backgroundColor = {0, 0, 0, 0},
            }
        )
        {
            const CrowdControlTuning tuning = CrowdControl_GetTuning();
            char leftMaxLabel[16] = {};
            char leftCurrentLabel[16] = {};
            char rightMaxLabel[16] = {};
            char rightCurrentLabel[16] = {};
            std::snprintf(leftMaxLabel, sizeof(leftMaxLabel), "+%d", tuning.leftUpgradePrice);
            std::snprintf(leftCurrentLabel, sizeof(leftCurrentLabel), "+%d", glm::max(0, cc.leftBeltVal));
            std::snprintf(rightMaxLabel, sizeof(rightMaxLabel), "+%d", tuning.rightUpgradePrice);
            std::snprintf(rightCurrentLabel, sizeof(rightCurrentLabel), "+%d", glm::max(0, cc.rightBeltVal));
            const char *labels[CROWD_CONTROL_LABEL_ROWS] = {
                leftMaxLabel,
                leftCurrentLabel,
                rightMaxLabel,
                rightCurrentLabel
            };
            for (int row = 0; row < CROWD_CONTROL_LABEL_ROWS; ++row)
            {
                const int crowdRowH = HEIGHT / CROWD_CONTROL_LABEL_ROWS;
                const int labelH = crowdControlLabelPixelHeight();
                CLAY(
                    CLAY_IDI("ClayToTexCrowdControlRow", row),
                    {
                        .layout = {
                            .sizing = {CLAY_SIZING_FIXED(WIDTH), CLAY_SIZING_FIXED(crowdRowH)},
                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        },
                    }
                )
                {
                    Clay_String labelText = ClayArena_AllocString(&clayton->clayArena, labels[row]);
                    Clay_TextElementConfig textCfg = {
                        .textColor = {255, 255, 255, 255},
                        .fontId = CLAY_FONT_NOTO,
                        .fontSize = 384,
                        .letterSpacing = 1,
                        .wrapMode = CLAY_TEXT_WRAP_NONE,
                        .textAlignment = CLAY_TEXT_ALIGN_CENTER,
                    };

                    CLAY(
                        CLAY_IDI("ClayToTexCrowdControlCell", row),
                        {
                            .layout = {
                                .sizing = {CLAY_SIZING_FIXED(WIDTH), CLAY_SIZING_FIXED(crowdRowH)},
                                .padding = {0, 0, 0, 0},
                                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                            },
                            .backgroundColor = {0, 0, 0, 0},
                        }
                    )
                    {
                        const bool powerCard = row >= 2;
                        const Clay_Color bg = !powerCard
                            ? (Clay_Color){40, 188, 88, 232}
                            : (Clay_Color){224, 64, 54, 232};
                        CLAY(
                            CLAY_IDI("ClayToTexCrowdControlPill", row),
                            {
                                .layout = {
                                    .sizing = {CLAY_SIZING_FIXED((float)WIDTH), CLAY_SIZING_FIXED((float)labelH)},
                                    .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                },
                                .backgroundColor = bg,
                                .cornerRadius = {16, 16, 16, 16},
                                .border = {.color = {255, 255, 255, 230}, .width = CLAY_BORDER_ALL(3)},
                            }
                        )
                        {
                            CLAY_TEXT(labelText, CLAY_TEXT_CONFIG(textCfg));
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
