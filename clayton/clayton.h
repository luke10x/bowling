#pragma once

#include <SDL.h>
#include <stdlib.h>

#define CLAY_IMPLEMENTATION
#define CLAY_RENDERER_GLES3_IMPLEMENTATION
#include "renderers/GLES3/clay_renderer_gles3.h"
#include "stb_loader.h"
#include <clay.h>
#undef CLAY_IMPLEMENTATION
#undef CLAY_RENDERER_GLES3_IMPLEMENTATION

#include "../score.h"

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#ifndef ASSET_PATH
#if defined(__ANDROID__) || defined(ANDROID)
#define ASSET_PATH "files/"
#elif TARGET_OS_IPHONE
#define ASSET_PATH ""   // iOS
#elif TARGET_OS_OSX
#define ASSET_PATH "assets/files/"  // macOS
#else
#define ASSET_PATH "assets/files/"
#endif
#endif


void Gles3_ErrorHandler(Clay_ErrorData errorData)
{
    printf("[ClaY ErroR] %s", errorData.errorText.chars);
}

struct Clayton
{
    Gles3_Renderer renderer;
    Stb_FontData stbFonts[MAX_FONTS];

    Gles3_ImageConfig pinImage;

    Clay_Vector2 scrollDelta;

    Clay_TextElementConfig smallFontCfg;

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
            }
        );

        // Note that MeasureText has to be set after the Context is set!
        Clay_SetCurrentContext(clayCtx);
        Clay_SetMeasureTextFunction(Stb_MeasureText, &this->stbFonts);
        Gles3_SetRenderTextFunction(&this->renderer, Stb_RenderText, &this->stbFonts);

        Gles3_Initialize(&this->renderer, 4096);
    }

    void initClayton(float screenWidth, float screenHeight)
    {
        this->loadClayton(screenWidth, screenHeight);

        if (!Stb_LoadImage(&this->renderer.imageTextures[0], ASSET_PATH "everything_tex.png"))
            abort();

        this->pinImage = Gles3_ImageConfig{
            .textureToUse = 0,
            .u0 = 0.0f,
            .v0 = 0.75f,
            .u1 = 0.125f,
            .v1 = 1.0f,
        };

        int atlasW = 512;
        int atlasH = 512;
        if (!Stb_LoadFont(
                &this->renderer.fontTextures[0],
                &this->stbFonts[0],
                ASSET_PATH "Roboto-Regular.ttf",
                32.0f, // bake pixel height
                atlasW,
                atlasH
            ))
            abort();

        if (!Stb_LoadFont(
                &this->renderer.fontTextures[1],
                &this->stbFonts[1],
                ASSET_PATH "SUSEMono-Medium.ttf",
                32.0f, // bake pixel height
                atlasW,
                atlasH
            ))
            abort();

        if (!Stb_LoadFont(
                &this->renderer.fontTextures[2],
                &this->stbFonts[2],
                ASSET_PATH "RobotoMono-Regular.ttf",
                48.0f, // bake pixel height
                atlasW,
                atlasH
            ))
            abort();

        this->smallFontCfg = {
            .textColor = {25, 25, 25, 255},
            .fontId = 0,
            .fontSize = (uint16_t)16,
        };
    }

    void processClaytonEvent(SDL_Event *event, double deltaTime, float pixelRatio)
    {
        int mouseX = -1;
        int mouseY = -1;
        bool mouseClicked = false;
        switch (event->type)
        {
        case SDL_MOUSEBUTTONDOWN:
            mouseX = pixelRatio * static_cast<float>(event->button.x);
            mouseY = pixelRatio * static_cast<float>(event->button.y);
            std::cerr << "mouseX=" << mouseX << std::endl;
            mouseClicked = true;
            break;
        case SDL_MOUSEWHEEL:
        {
            scrollDelta.x += event->wheel.x;
            scrollDelta.y += event->wheel.y;
            break;
        }
        }

        if (mouseX == -1 || mouseY == -1)
        { // fallback for Desktop to get all hovers to work
            Uint32 mouseState = SDL_GetMouseState(&mouseX, &mouseY);
            if (mouseState & SDL_BUTTON(1))
            {
                mouseClicked = true;
            }
            mouseX *= pixelRatio;
            mouseY *= pixelRatio;
        }
        Clay_Vector2 mousePosition = (Clay_Vector2){(float)mouseX, (float)mouseY};
        Clay_SetPointerState(mousePosition, mouseClicked);

        Clay_UpdateScrollContainers(
            true, // enableDragScrolling
            (Clay_Vector2){this->scrollDelta.x, this->scrollDelta.y},
            deltaTime
        );
    }

    void
    renderClayton(Clay_RenderCommandArray cmds, int screenWidth, int screenHeight, double deltaTime)
    {

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

    static inline Clay_String
    rollSymbol2(int roll, int /*prev*/, bool isStrike, bool isSpare, bool secondRoll)
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

    void constructClayScoreboard(
        const BowlingScoreboard *sb,
        float boardWidth,
        Clay_ElementId renameButtonId,
        char *username,
        int32_t *username_len
    )
    {
        float u1 = boardWidth / 24; // + 9*2 + 3 + 3(result) = 24
        float u2 = 2 * u1;
        float u3 = 3 * u1;
        uint16_t bigSize = (boardWidth < 600 ? 32 : 64);
        Clay_TextElementConfig bigFontCfg = {
            .textColor = {255, 25, 25, 255},
            .fontId = 0,
            .fontSize = (uint16_t)(boardWidth < 600 ? 32 : 64),
        };

        float smallSquare = 16;
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
            CLAY_ID("ScoreboardWrapper"),
            {
                .layout = {
                    .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                    .childGap = 0,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
            },
        )
        {
            CLAY(
                CLAY_ID("ScoreboardBar"),
                {
                    .layout =
                        {
                            .sizing = {CLAY_SIZING_FIT(), CLAY_SIZING_FIT()},
                            .childGap = 0,
                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        },
                    .backgroundColor = {255, 255, 255, 255},
                    .cornerRadius = {8, 8, 0, 0},
                    .border = {.color = {0, 0, 100, 255}, .width = CLAY_BORDER_ALL(2)},
                }
            )
            {

                /* -------- FRAMES -------- */
                for (int i = 0; i < 10; ++i)
                {
                    const Frame &f = sb->frames[i];
                    const bool last = (i == 9);

                    const Clay_String r1 = rollSymbol2(f.roll1, 0, f.isStrike, f.isSpare, false);

                    const Clay_String r2 =
                        rollSymbol2(f.roll2, f.roll1, f.isStrike, f.isSpare, true);

                    const Clay_String r3 = last
                        ? (f.roll3 < 0 ? CLAY_STRING(" ")
                                       : (f.roll3 == 10 ? CLAY_STRING("X") : clayInt(f.roll3)))
                        : CLAY_STRING(" ");

                    CLAY(
                        CLAY_IDI("Frame", i),
                        {
                            .layout = {
                                .sizing =
                                    {
                                        last ? CLAY_SIZING_FIXED(u3) : CLAY_SIZING_FIXED(u2),
                                        CLAY_SIZING_GROW(0),
                                    },
                                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                            },
                        }
                    )
                    {
                        /* -------- ROLLS -------- */
                        CLAY(
                            CLAY_IDI("RollRow", i),
                            {
                                .layout = {
                                    .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIT()},
                                    .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                                    // .childGap = 6,
                                },
                            }
                        )
                        {
                            CLAY(
                                CLAY_IDI("Eachroll-1", i),
                                {
                                    .layout = {
                                        .sizing = {CLAY_SIZING_FIXED(u1), CLAY_SIZING_FIXED(u1)},
                                        .childAlignment = {
                                            CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER
                                        },
                                    },
                                }
                            )
                            {

                                CLAY_TEXT(r1, CLAY_TEXT_CONFIG(smallFontCfg));
                            }
                            CLAY(
                                CLAY_IDI("Eachroll-2", i),
                                {
                                    .layout =
                                        {
                                            .sizing =
                                                {CLAY_SIZING_FIXED(u1), CLAY_SIZING_FIXED(u1)},
                                            .childAlignment =
                                                {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                        },
                                    .border = {
                                        .color = {0, 0, 0, 255},
                                        .width = {.left = 1},
                                    },
                                }
                            )
                            {
                                CLAY_TEXT(r2, CLAY_TEXT_CONFIG(smallFontCfg));
                            }
                            if (last)
                            {
                                CLAY(
                                    CLAY_IDI("Eachroll-3", i),
                                    {
                                        .layout =
                                            {
                                                .sizing =
                                                    {CLAY_SIZING_FIXED(u1), CLAY_SIZING_FIXED(u1)},
                                                .childAlignment =
                                                    {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                            },
                                        .border = {
                                            .color = {0, 0, 0, 255},
                                            .width = {.left = 1},
                                        },
                                    }
                                )
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
                            }
                        ){};

                        /* -------- CUMULATIVE -------- */
                        CLAY(
                            CLAY_IDI("ScoreRow", i),
                            {
                                .layout = {
                                    .sizing = {CLAY_SIZING_FIXED(u2), CLAY_SIZING_FIXED(u2)},
                                    .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                },
                            }
                        )
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
                            .sizing = {CLAY_SIZING_FIXED(u3), CLAY_SIZING_GROW(u3)},
                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                        },
                    }
                )
                {
                    CLAY_TEXT(clayInt(sb->totalScore), CLAY_TEXT_CONFIG(bigFontCfg));
                }
            };
            /* -------- NAME -------- */
            CLAY(
                renameButtonId,
                {
                    .layout =
                        {
                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                            .padding = {10, 10, 10, 10},
                            .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_TOP},
                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        },
                    .backgroundColor = {255, 255, 255, 255},
                    .cornerRadius = {0, 0, 8, 8},
                    .border = {.color = {0, 0, 100, 255}, .width = {2, 2, 0, 2}},
                }
            )
            {
                Clay_String cs = Clay_String{
                    .isStaticallyAllocated = false,
                    .length = *username_len,
                    .chars = username,
                };
                CLAY_TEXT(cs, CLAY_TEXT_CONFIG(smallFontCfg));
            }
        };
    }
};
