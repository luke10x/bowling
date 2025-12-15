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
            scrollDelta.x += event->wheel.x * 3.0f;
            scrollDelta.y += event->wheel.y * 3.0f;
            break;
        }
        }
    }

    void renderClayton(Clay_RenderCommandArray cmds, int screenWidth, int screenHeight, double deltaTime)
    {
        int mouseX = 0;
        int mouseY = 0;
        Uint32 mouseState = SDL_GetMouseState(&mouseX, &mouseY);
        Clay_Vector2 mousePosition = (Clay_Vector2){(float)mouseX, (float)mouseY};
        Clay_SetPointerState(mousePosition, mouseState & SDL_BUTTON(1));

        Clay_UpdateScrollContainers(
            true,
            (Clay_Vector2){this->scrollDelta.x, this->scrollDelta.y},
            deltaTime);
        this->scrollDelta.x = 0.0f;
        this->scrollDelta.y = 0.0f;

        this->renderer.screenWidth = screenWidth;
        this->renderer.screenHeight = screenHeight;
        Clay_SetLayoutDimensions((Clay_Dimensions){
            .width = (float)screenWidth,
            .height = (float)screenHeight,
        });
        Gles3_Render(&this->renderer, cmds, this->stbFonts);
    }
};
