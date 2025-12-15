#pragma once

#include <stdlib.h>

#include <SDL.h>

// Clayton libs
#define STB_TRUETYPE_IMPLEMENTATION
#define CLAY_IMPLEMENTATION
#define CLAY_RENDERER_GLES3_IMPLEMENTATION

#include <clay.h>

#include "renderers/GLES3/clay_renderer_gles3.h"
#include "renderers/GLES3/clay_renderer_gles3_loader_stb.c"

void Gles3_ErrorHandler(Clay_ErrorData errorData)
{
    printf("[ClaY ErroR] %s", errorData.errorText.chars);
}

struct Clayton
{
    Gles3_Renderer renderer;
    Stb_FontData stbFonts[MAX_FONTS];

    Gles3_ImageConfig pinImage;
    Gles3_ImageConfig parkImage;

    Clay_Vector2 scrollDelta;

    void initClayton(float screenWidth, float screenHeight)
    {
        size_t clayRequiredMemory = Clay_MinMemorySize();
        this->renderer.clayMemory = (Clay_Arena){
            .capacity = clayRequiredMemory,
            .memory = (char *)malloc(clayRequiredMemory),
        };
        Clay_Context *clayCtx = Clay_Initialize(
            this->renderer.clayMemory,
            (Clay_Dimensions){
                .width = screenWidth,
                .height = screenHeight,
            },
            (Clay_ErrorHandler){
                .errorHandlerFunction = Gles3_ErrorHandler,
            });

        // Note that MeasureText has to be set after the Context is set!
        Clay_SetCurrentContext(clayCtx);
        Clay_SetMeasureTextFunction(Stb_MeasureText, &this->stbFonts);
        Gles3_SetRenderTextFunction(&this->renderer, Stb_RenderText, &this->stbFonts);

        Gles3_Initialize(&this->renderer, 4096);

        if (!Stb_LoadImage(
                &this->renderer.imageTextures[0],
                "assets/files/everything_tex.png"))
            abort();

        if (!Stb_LoadImage(
                &this->renderer.imageTextures[1],
                "assets/files/park.jpg"))
            abort();

        this->pinImage = Gles3_ImageConfig{
            .textureToUse = 0,
            .u0 = 0.0f,
            .v0 = 0.75f,
            .u1 = 0.125f,
            .v1 = 1.0f,
        };
        this->parkImage = Gles3_ImageConfig{
            .textureToUse = 1,
            .u0 = 0.0f,
            .v0 = 0.0f,
            .u1 = 1.0f,
            .v1 = 1.0f,
        };

        int atlasW = 512;
        int atlasH = 512;
        if (!Stb_LoadFont(
                &this->renderer.fontTextures[0],
                &this->stbFonts[0],
                "assets/files/Roboto-Regular.ttf",
                48.0f, // bake pixel height
                atlasW,
                atlasH))
            abort();

        if (!Stb_LoadFont(
                &this->renderer.fontTextures[1],
                &this->stbFonts[1],
                "assets/files/SUSEMono-Medium.ttf",
                48.0f, // bake pixel height
                atlasW,
                atlasH))
            abort();

        Clay_SetDebugModeEnabled(true);
    }

    void processClaytonEvent(SDL_Event *event, double deltaTime)
    {

        switch (event->type)
        {
        case SDL_MOUSEWHEEL:
        {
            scrollDelta.x += event->wheel.x;
            scrollDelta.y += event->wheel.y;
            // break;
        }
        }
    }

    void renderClayton(Clay_RenderCommandArray cmds, float pixelRatio, int screenWidth, int screenHeight, double deltaTime)
    {
        int mouseX = 0;
        int mouseY = 0;
        Uint32 mouseState = SDL_GetMouseState(&mouseX, &mouseY);
        Clay_Vector2 mousePosition = (Clay_Vector2){
            (float)mouseX * pixelRatio,
            (float)mouseY * pixelRatio};
        Clay_SetPointerState(mousePosition, mouseState & SDL_BUTTON(1));

        Clay_UpdateScrollContainers(
            true,
            (Clay_Vector2){this->scrollDelta.x, this->scrollDelta.y},
            deltaTime);
        // this->scrollDelta.x = 0.0f;
        // this->scrollDelta.y = 0.0f;

        Clay_SetLayoutDimensions((Clay_Dimensions){
            .width = (float)screenWidth,
            .height = (float)screenHeight,
        });

        // this->renderer.screenWidth = screenWidth;
        // this->renderer.screenHeight = screenHeight;
        Gles3_Render(&this->renderer, cmds, this->stbFonts);
    }
    void constructClayScoreboard()
    {
        CLAY(
            CLAY_ID("ScoreboardBar"),
            {
                .layout = {
                    .sizing = {
                        .width = CLAY_SIZING_GROW(0),
                        .height = CLAY_SIZING_FIXED(110),
                    },
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                },
                .backgroundColor = {0, 0, 0, 0},
            })
        {
            CLAY(
                CLAY_ID("Name section"),
                {
                    .layout = {
                        .sizing = {
                            CLAY_SIZING_FIXED(190),
                            .height = CLAY_SIZING_GROW(0),
                        },
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    },
                    .border = {
                        .color = {0, 0, 100, 255},
                        .width = {5, 5, 5, 5},
                    },
                    .backgroundColor = {255, 255, 255, 255},
                })
            {
                CLAY_TEXT(
                    CLAY_STRING("Green Text1"),
                    CLAY_TEXT_CONFIG({
                        .textColor = {25, 25, 25, 200},
                        .fontId = 0,
                        .fontSize = 48,
                    }));
            }
            for (int i = 0; i < 10; ++i)
            {
                const bool last = (i == 9);

                CLAY(
                    CLAY_IDI("Frame", i),
                    {
                        .layout = {
                            .sizing = {
                                .width = last
                                             ? CLAY_SIZING_FIXED(90)
                                             : CLAY_SIZING_FIXED(60),
                                .height = CLAY_SIZING_GROW(0),
                            },
                            .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        },
                        .border = {
                            .color = {0, 0, 100, 255},
                            .width = {5, 5, 5, 5},
                        },
                        .backgroundColor = {255, 255, 255, 255},
                    })
                {
                    /* -------- ROLLS ROW -------- */
                    CLAY(
                        CLAY_IDI("RollRow", i),
                        {
                            .layout = {
                                .sizing = {
                                    .width = CLAY_SIZING_GROW(0),
                                    .height = CLAY_SIZING_FIXED(45),
                                },
                                .layoutDirection = CLAY_LEFT_TO_RIGHT,
                                .childAlignment = {
                                    .x = CLAY_ALIGN_X_CENTER,
                                    .y = CLAY_ALIGN_Y_CENTER,
                                },
                                .childGap = 6,
                            },
                        })
                    {
                        // CLAY_TEXT(
                        //     CLAY_STRING(r1[i]),   // "X", "7", "-"
                        //     CLAY_TEXT_CONFIG({
                        //         .fontId   = 0,
                        //         .fontSize = 22,
                        //         .textColor = {0, 0, 0, 255},
                        //     }));

                        // CLAY_TEXT(
                        //     CLAY_STRING(r2[i]),
                        //     CLAY_TEXT_CONFIG({
                        //         .fontId   = 0,
                        //         .fontSize = 22,
                        //         .textColor = {0, 0, 0, 255},
                        //     }));

                        if (last)
                        {
                            // CLAY_TEXT(
                            //     CLAY_STRING(r3[i]),
                            //     CLAY_TEXT_CONFIG({
                            //         .fontId   = 0,
                            //         .fontSize = 22,
                            //         .textColor = {0, 0, 0, 255},
                            //     }));
                        }
                    };

                    /* -------- DIVIDER -------- */
                    CLAY(
                        CLAY_IDI("Divider", i),
                        {
                            .layout = {
                                .sizing = {
                                    .width = CLAY_SIZING_GROW(0),
                                    .height = CLAY_SIZING_FIXED(1),
                                },
                            },
                            .backgroundColor = {0, 0, 0, 255},
                        }){};

                    /* -------- CUMULATIVE ROW -------- */
                    CLAY(
                        CLAY_IDI("ScoreRow", i),
                        {
                            .layout = {
                                .sizing = {
                                    .width = CLAY_SIZING_GROW(0),
                                    .height = CLAY_SIZING_GROW(0),
                                },
                                .childAlignment = {
                                    .x = CLAY_ALIGN_X_CENTER,
                                    .y = CLAY_ALIGN_Y_CENTER,
                                },
                            },
                        }){
                        // if (cumulative[i] >= 0)
                        // {
                        //     CLAY_TEXT(
                        //         CLAY_STRING(scoreStr[i]), // preformatted " 120"
                        //         CLAY_TEXT_CONFIG({
                        //             .fontId   = 1,
                        //             .fontSize = 20,
                        //             .textColor = {0, 0, 0, 255},
                        //         }));
                        // }
                    };
                };
            }
        };
    }
};
