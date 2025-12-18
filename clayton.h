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

#include "score.h"

void Gles3_ErrorHandler(Clay_ErrorData errorData)
{
    printf("[ClaY ErroR] %s", errorData.errorText.chars);
}

struct Clayton
{
    Gles3_Renderer renderer;
    Stb_FontData stbFonts[MAX_FONTS];

    Gles3_ImageConfig pinImage;
    // Gles3_ImageConfig parkImage;

    Clay_Vector2 scrollDelta;

    void loadClayton(float screenWidth, float screenHeight)
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
    }

    void initClayton(float screenWidth, float screenHeight)
    {
        this->loadClayton(screenWidth, screenHeight);

        if (!Stb_LoadImage(
                &this->renderer.imageTextures[0],
                "assets/files/everything_tex.png"))
            abort();

        // if (!Stb_LoadImage(
        //         &this->renderer.imageTextures[1],
        //         "assets/files/park.jpg"))
        //     abort();

        this->pinImage = Gles3_ImageConfig{
            .textureToUse = 0,
            .u0 = 0.0f,
            .v0 = 0.75f,
            .u1 = 0.125f,
            .v1 = 1.0f,
        };
        // this->parkImage = Gles3_ImageConfig{
        //     .textureToUse = 1,
        //     .u0 = 0.0f,
        //     .v0 = 0.0f,
        //     .u1 = 1.0f,
        //     .v1 = 1.0f,
        // };

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
            break;
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
            true, // enableDragScrolling
            (Clay_Vector2){this->scrollDelta.x, this->scrollDelta.y},
            deltaTime);

        Clay_SetLayoutDimensions((Clay_Dimensions){
            .width = (float)screenWidth,
            .height = (float)screenHeight,
        });

        Gles3_Render(&this->renderer, cmds, this->stbFonts);
    }

    static inline Clay_String clayChar(char c)
    {
        static char buf[2];
        buf[0] = c;
        buf[1] = '\0'; // not relied upon, but harmless

        return Clay_String{
            .isStaticallyAllocated = false,
            .length = 1,
            .chars = buf,
        };
    }
    static inline Clay_String clayInt(int value)
    {
        // 32 independent slots per frame
        static char bufs[32][8];
        static int index = 0;

        char *buf = bufs[index++ & 31];
        int len = 0;

        if (value == 0)
        {
            buf[len++] = '0';
        }
        else
        {
            int v = value;
            char tmp[8];
            int t = 0;

            while (v > 0)
            {
                tmp[t++] = char('0' + (v % 10));
                v /= 10;
            }
            while (t--)
                buf[len++] = tmp[t];
        }

        return {
            .isStaticallyAllocated = true, // IMPORTANT
            .length = len,
            .chars = buf,
        };
    }

    static inline Clay_String rollSymbol2(
        int roll,
        int /*prev*/,
        bool isStrike,
        bool isSpare,
        bool secondRoll)
    {
        if (roll < 0)
            return CLAY_STRING(" ");

        if (isStrike && !secondRoll)
            return CLAY_STRING("X");

        if (secondRoll && isSpare)
            return CLAY_STRING("/");

        if (roll == 0)
            return CLAY_STRING("0");

        return clayInt(roll);
    }

    static bool frameIsComplete(const BowlingScoreboard *sb, int i)
    {
        const Frame &f = sb->frames[i];

        if (i < 9)
        {
            if (f.roll1 < 0)
                return false;
            if (f.isStrike)
            {
                const Frame &n1 = sb->frames[i + 1];
                if (n1.roll1 < 0)
                    return false;
                if (n1.isStrike)
                {
                    if (i + 2 < 10)
                        return sb->frames[i + 2].roll1 >= 0;
                    return n1.roll2 >= 0;
                }
                return n1.roll2 >= 0;
            }
            if (f.roll2 < 0)
                return false;
            if (f.isSpare)
                return sb->frames[i + 1].roll1 >= 0;
            return true;
        }
        else
        {
            if (f.roll1 < 0)
                return false;
            if (f.isStrike || f.isSpare)
                return f.roll3 >= 0;
            return f.roll2 >= 0;
        }
    }

    void constructClayScoreboard(const BowlingScoreboard *sb)
    {
        Clay_TextElementConfig smallFontCfg = {
            .fontSize = 24,
            .textColor = {25, 25, 25, 255},
            .fontId = 0,
        };
        Clay_TextElementConfig bigFontCfg = {
            .fontSize = 48,
            .textColor = {255, 25, 25, 255},
            .fontId = 0,
        };

        int cumulative[10];
        int running = 0;

        for (int i = 0; i < 10; ++i)
        {
            if (frameIsComplete(sb, i) && sb->frames[i].frameScore >= 0)
            {
                running += sb->frames[i].frameScore;
                cumulative[i] = running;
            }
            else
            {
                cumulative[i] = -1;
            }
        }

        CLAY(
            CLAY_ID("ScoreboardBar"),
            {
                .layout = {
                    .sizing = {CLAY_SIZING_FIT(), CLAY_SIZING_FIT()},
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                    .childGap = 5,
                },
            })
        {
            /* -------- NAME -------- */
            CLAY(
                CLAY_ID("Name section"),
                {
                    .layout = {
                        .sizing = {CLAY_SIZING_FIXED(120), CLAY_SIZING_GROW(0)},
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                    },
                    .border = {.color = {0, 0, 100, 255}, .width = {5, 5, 5, 5}},
                    .cornerRadius = {20, 0, 20, 0},
                    .backgroundColor = {255, 255, 255, 255},
                })
            {
                CLAY_TEXT(CLAY_STRING("Lapee"), CLAY_TEXT_CONFIG(bigFontCfg));
            }

            /* -------- FRAMES -------- */
            for (int i = 0; i < 10; ++i)
            {
                const Frame &f = sb->frames[i];
                const bool last = (i == 9);

                const Clay_String r1 = rollSymbol2(
                    f.roll1, 0, f.isStrike, f.isSpare, false);

                const Clay_String r2 = rollSymbol2(
                    f.roll2, f.roll1, f.isStrike, f.isSpare, true);

                const Clay_String r3 = last
                                           ? (f.roll3 < 0 ? CLAY_STRING(" ")
                                                          : (f.roll3 == 10 ? CLAY_STRING("X") : clayInt(f.roll3)))
                                           : CLAY_STRING(" ");

                CLAY(
                    CLAY_IDI("Frame", i),
                    {
                        .layout = {
                            .sizing = {
                                last ? CLAY_SIZING_FIT() : CLAY_SIZING_FIT(),
                                CLAY_SIZING_GROW(0),
                            },
                            .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        },
                        .border = {.color = {0, 0, 100, 255}, .width = {5, 5, 5, 5}},
                        .backgroundColor = {255, 255, 255, 255},
                    })
                {
                    /* -------- ROLLS -------- */
                    CLAY(
                        CLAY_IDI("RollRow", i),
                        {
                            .layout = {
                                .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIT()},
                                .layoutDirection = CLAY_LEFT_TO_RIGHT,
                                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                // .childGap = 6,
                            },
                        })
                    {
                        CLAY(CLAY_IDI("Eachroll-1", i), {
                                                            .layout = {
                                                                .sizing = {CLAY_SIZING_FIXED(20), CLAY_SIZING_FIXED(20)},
                                                                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                                            },
                                                        })
                        {

                            CLAY_TEXT(r1, CLAY_TEXT_CONFIG(smallFontCfg));
                        }
                        CLAY(CLAY_IDI("Eachroll-2", i), {
                                                            .layout = {
                                                                .sizing = {CLAY_SIZING_FIXED(20), CLAY_SIZING_FIXED(20)},
                                                                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                                            },
                                                            .border = {.width = {.left = 1}, .color = {0, 0, 0, 255}},
                                                        })
                        {
                            CLAY_TEXT(r2, CLAY_TEXT_CONFIG(smallFontCfg));
                        }
                        if (last)
                        {
                            CLAY(CLAY_IDI("Eachroll-3", i), {
                                                                .layout = {
                                                                    .sizing = {CLAY_SIZING_FIXED(20), CLAY_SIZING_FIXED(20)},
                                                                    .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                                                },
                                                                .border = {.width = {.left = 1}, .color = {0, 0, 0, 255}},
                                                            })
                            {
                                CLAY_TEXT(r3, CLAY_TEXT_CONFIG(smallFontCfg));
                            }
                        }
                    };

                    /* -------- DIVIDER -------- */
                    CLAY(
                        CLAY_IDI("Divider", i),
                        {
                            .layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1)}},
                            .backgroundColor = {0, 0, 0, 255},
                        }){};

                    /* -------- CUMULATIVE -------- */
                    CLAY(
                        CLAY_IDI("ScoreRow", i),
                        {
                            .layout = {
                                .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(40)},
                                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                            },
                        })
                    {
                        if (cumulative[i] >= 0)
                            CLAY_TEXT(clayInt(cumulative[i]), CLAY_TEXT_CONFIG(smallFontCfg));
                    };
                };
            }

            /* -------- TOTAL -------- */
            CLAY(
                CLAY_ID("Total result section"),
                {
                    .layout = {
                        .sizing = {CLAY_SIZING_FIXED(100), CLAY_SIZING_GROW(0)},
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                    },
                    .border = {.color = {0, 0, 100, 255}, .width = {5, 5, 5, 5}},
                    .cornerRadius = {0, 20, 0, 20},
                    .backgroundColor = {255, 255, 255, 255},
                })
            {
                CLAY_TEXT(clayInt(sb->totalScore), CLAY_TEXT_CONFIG(bigFontCfg));
            }
        };
    }
};
