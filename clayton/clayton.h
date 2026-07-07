#pragma once

#include <SDL.h>
#include <stdlib.h>

#define CLAY_IMPLEMENTATION
#define CLAY_RENDERER_GLES3_IMPLEMENTATION
#include "clay_renderer_gles3.h"
#include "stb_loader.h"
#include <clay.h>
#include "claytheme.h"
#undef CLAY_IMPLEMENTATION
#undef CLAY_RENDERER_GLES3_IMPLEMENTATION
#include "./clayton_click.h"

#include "../score.h"
#include "../shop.h"
#include "../storyline.h"
#include "../tegel/txl_runtime.h"
#include "./clayton_click.h"
#include "clayarena.h"

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
    Gles3_ImageConfig pin2Image;
    Gles3_ImageConfig pin3Image;
    Gles3_ImageConfig oilImage;
    // RenderTexture-backed UI previews are vertically flipped in UV space.
    // Use these configs when displaying FBO textures so only those images are flipped.
    Gles3_ImageConfig housesPinImage;
    Gles3_ImageConfig housesPin2Image;
    Gles3_ImageConfig housesPin3Image;
    Gles3_ImageConfig botPreviewImage;
    Gles3_ImageConfig botPreview2Image;
    Gles3_ImageConfig botPreview3Image;
    Gles3_ImageConfig trackerDiagramImage;
    Gles3_ImageConfig trackerAlgoImages[8];
    Gles3_ImageConfig trackerSelectedAlgoImages[8][4];
    Gles3_ImageConfig trackerSsgImages[8];
    Gles3_ImageConfig trackerEnvelopeImages[4];
    Gles3_ImageConfig trackerOscilloscopeImages[6];

    Clay_Vector2 scrollDelta;

    Clay_TextElementConfig smallFontCfg;
    TxlLanguage uiLanguage = TXL_LANG_EN_US;

    // Scratch buffers for Clay_String formatting helpers.
    // Needs enough slots for multiple scoreboards rendered in the same frame (e.g. BOT mode shows 2).
    char charBuf[100][16];

    bool shouldShowHiScore = false;
    bool shouldShowHiScoreWithLatest = false;
    bool shouldShowOilStatus = false;
    bool shouldShowHouses = false;
    bool shouldShowBotSelect = false;
    bool shouldShowSettings = false;
    const char *newGameTitle = "TRY AGAIN";
    const char *newGameButtonLabel = "TRY AGAIN";
    const char *housesActionLabel = "SWITCH HOUSE";
    const char *botsActionLabel = "SELECT BOT";
    const char *shopActionLabel = "BUY";
    bool housesActionEnabled = true;
    bool botsActionEnabled = true;
    bool shopActionEnabled = true;

    Clayton_Click closeShopClick;
    Clayton_Click shopInventoryTabClick;
    Clayton_Click shopStoreTabClick;
    Clayton_Click buyClick;
    Clayton_Click playAgainClick;

    // Sound Settings clicks
    Clayton_Click musicVolClicks[5]; // 5 volume buttons for music
    Clayton_Click sfxVolClicks[5];   // 5 volume buttons for SFX
    Clayton_Click qualityClicks[3];  // 3 quality buttons
    Clayton_Click bufferClicks[4];   // 4 SDL buffer size buttons
    Clayton_Click prevSongClick;
    Clayton_Click nextSongClick;
	    Clayton_Click closeClick;
    Clayton_Click hiScoreCloseClick;
    Clayton_Click oilStatusCloseClick;
    Clayton_Click oilReoilClick;
    Clayton_Click housesCloseClick;
    Clayton_Click housesSelectClick;
    Clayton_Click botSelectCloseClick;
    Clayton_Click botSelectSelectClick;
    // Menu window clicks
    Clayton_Click menuCloseClick;
    Clayton_Click menuRenameClick;
    Clayton_Click menuSchoolClick;
    Clayton_Click menuLanguageClick;
    Clayton_Click menuCampaignClick;
    Clayton_Click menuPracticeClick;
    Clayton_Click menuFreestyleClick;
    Clayton_Click menuDeviceShareClick;
    Clayton_Click menuTrackerClick;
    Clayton_Click menuSettingsClick;
    Clayton_Click settingsCloseClick;
    Clayton_Click settingsResetProgressClick;
    Clayton_Click settingsResetConfirmYesClick;
    Clayton_Click settingsResetConfirmNoClick;
    Clayton_Click settingsCheckUpdateClick;
    Clayton_Click settingsApplyUpdateClick;
    Clayton_Click languageCloseClick;
    Clayton_Click languageEnglishClick;
    Clayton_Click languageChineseClick;
    Clayton_Click languageLithuanianClick;
    Clayton_Click languageJapaneseClick;

    // BOT match result window clicks
    Clayton_Click botResultCloseClick;
    Clayton_Click campaignEndgameCloseClick;

    // Greetings window
    Clayton_Click greetingsReadyClick;

	    // Adaptive audio controls
	    Clayton_Click useSynthClick;
    Clayton_Click useWavClick;
    Clayton_Click disableAudioClick;

    ClayArena clayArena;

    // Reference to sound system (set by game)
    // GameSoundSystem* soundSystem;

    void loadClayton(float screenWidth, float screenHeight)
    {
        Clay_SetMaxElementCount(32768);
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

        Gles3_Initialize(&this->renderer, 65536);
    }

    void initClayton(float screenWidth, float screenHeight)
    {
        this->loadClayton(screenWidth, screenHeight);

        if (!Stb_LoadImage(&this->renderer.imageTextures[0], ASSET_PATH "everything_tex.png"))
            abort();

        this->pinImage = Gles3_ImageConfig{
            .textureToUse = 1,
            .u0 = 0.0f,
            .v0 = 0.0f,
            .u1 = 1.0f,
            .v1 = 1.0f,
        };
        this->pin2Image = Gles3_ImageConfig{
            .textureToUse = 2,
            .u0 = 0.0f,
            .v0 = 0.0f,
            .u1 = 1.0f,
            .v1 = 1.0f,
        };
        this->pin3Image = Gles3_ImageConfig{
            .textureToUse = 3,
            .u0 = 0.0f,
            .v0 = 0.0f,
            .u1 = 1.0f,
            .v1 = 1.0f,
        };
        this->oilImage = Gles3_ImageConfig{
            .textureToUse = 3,
            .u0 = 0.0f,
            .v0 = 0.0f,
            .u1 = 1.0f,
            .v1 = 1.0f,
        };

        // Flipped variants for FBO previews (v is inverted).
        this->housesPinImage = Gles3_ImageConfig{
            .textureToUse = 1,
            .u0 = 0.0f,
            .v0 = 1.0f,
            .u1 = 1.0f,
            .v1 = 0.0f,
        };
        this->housesPin2Image = Gles3_ImageConfig{
            .textureToUse = 2,
            .u0 = 0.0f,
            .v0 = 1.0f,
            .u1 = 1.0f,
            .v1 = 0.0f,
        };
        this->housesPin3Image = Gles3_ImageConfig{
            .textureToUse = 3,
            .u0 = 0.0f,
            .v0 = 1.0f,
            .u1 = 1.0f,
            .v1 = 0.0f,
        };

        // Flipped variants for avatar preview FBOs (same slots 1/2/3).
        this->botPreviewImage = Gles3_ImageConfig{
            .textureToUse = 1,
            .u0 = 0.0f,
            .v0 = 1.0f,
            .u1 = 1.0f,
            .v1 = 0.0f,
        };
        this->botPreview2Image = Gles3_ImageConfig{
            .textureToUse = 2,
            .u0 = 0.0f,
            .v0 = 1.0f,
            .u1 = 1.0f,
            .v1 = 0.0f,
        };
        this->botPreview3Image = Gles3_ImageConfig{
            .textureToUse = 3,
            .u0 = 0.0f,
            .v0 = 1.0f,
            .u1 = 1.0f,
            .v1 = 0.0f,
        };
        this->trackerDiagramImage = Gles3_ImageConfig{
            .textureToUse = 3,
            .u0 = 0.0f,
            .v0 = 0.0f,
            .u1 = 1.0f,
            .v1 = 1.0f,
        };
        for (int i = 0; i < 8; i++)
        {
            float u0 = i / 8.0f;
            float u1 = (i + 1) / 8.0f;
            this->trackerAlgoImages[i] = Gles3_ImageConfig{.textureToUse = 3, .u0 = u0, .v0 = 0.0f, .u1 = u1, .v1 = 1.0f / 12.0f};
            this->trackerSsgImages[i] = Gles3_ImageConfig{.textureToUse = 3, .u0 = u0, .v0 = 1.0f / 12.0f, .u1 = u1, .v1 = 2.0f / 12.0f};
            for (int op = 0; op < 4; op++)
            {
                float v0 = (2.0f + op) / 12.0f;
                float v1 = v0 + 1.0f / 12.0f;
                this->trackerSelectedAlgoImages[i][op] = Gles3_ImageConfig{.textureToUse = 3, .u0 = u0, .v0 = v0, .u1 = u1, .v1 = v1};
            }
        }
        for (int op = 0; op < 4; op++)
        {
            int col = op & 1;
            int row = op >> 1;
            float v0 = 0.5f + row * 0.25f;
            float v1 = v0 + 0.25f;
            this->trackerEnvelopeImages[op] = Gles3_ImageConfig{
                .textureToUse = 3,
                .u0 = col * 0.5f,
                .v0 = v0,
                .u1 = col * 0.5f + 0.5f,
                .v1 = v1,
            };
        }
        for (int ch = 0; ch < 6; ch++)
        {
            float v0 = ch / 6.0f;
            float v1 = (ch + 1) / 6.0f;
            this->trackerOscilloscopeImages[ch] = Gles3_ImageConfig{
                .textureToUse = 2,
                .u0 = 0.0f,
                .v0 = v0,
                .u1 = 1.0f,
                .v1 = v1,
            };
        }

        loadFontsForLanguage(TXL_LANG_EN_US);

        this->smallFontCfg = {
            .textColor = CLAY_COLOR_TEXT_DARK,
            .fontId = CLAY_FONT_NOTO,
            .fontSize = CLAY_FONT_SIZE_SM,
        };

        // for (int i = 0; i < CAROUSEL_MAX_CARDS; i++)
        // {
        //     initClaytonClickIni(&this->buyClicks[i], "BuyButt", i);
        // }
    }

    void loadFontsForLanguage(TxlLanguage language)
    {
        uiLanguage = language;
        const int atlasW = 512;
        const int atlasH = 512;
        const TxlEmbeddedFont uiFont = Txl_UiFont(language);
        const TxlEmbeddedFont symbolFont = Txl_SymbolFont();
        const TxlEmbeddedFont monoFont = Txl_MonoFont();

        char uiCharsBuf[8192];
        snprintf(
            uiCharsBuf,
            sizeof(uiCharsBuf),
            "%s%s%s",
            k_txl_shared_chars,
            Txl_CharsForLanguage(language),
            Story_AllCharsForLanguage(language)
        );

        if (!Stb_LoadFontBytesWithChars(
                &this->renderer.fontTextures[0],
                &this->stbFonts[0],
                uiFont.data,
                uiFont.size,
                32.0f,
                atlasW,
                atlasH,
                uiCharsBuf
            ))
            abort();

        {
            const int glyphCount = this->stbFonts[0].useCustomChars
                ? this->stbFonts[0].customCharCount
                : this->stbFonts[0].charCount;
            const float atlasArea = (float)(atlasW * atlasH);
            const float usedPct = atlasArea > 0.0f
                ? (100.0f * (float)this->stbFonts[0].usedPixelCount / atlasArea)
                : 0.0f;
            printf(
                "[TXL Font] ui lang=%s chars=%d atlas_used=%d/%d (%.2f%%)\n",
                language == TXL_LANG_ZH_CN ? "zh_cn" :
                language == TXL_LANG_LT_LT ? "lt_lt" :
                language == TXL_LANG_JP_JP ? "jp_jp" : "en_us",
                glyphCount,
                this->stbFonts[0].usedPixelCount,
                atlasW * atlasH,
                usedPct
            );
        }

        if (!Stb_LoadFontBytesWithChars(
                &this->renderer.fontTextures[1],
                &this->stbFonts[1],
                symbolFont.data,
                symbolFont.size,
                32.0f,
                atlasW,
                atlasH,
                "◀▶▼▲✓"
            ))
            abort();

        if (!Stb_LoadFontBytesWithChars(
                &this->renderer.fontTextures[2],
                &this->stbFonts[2],
                monoFont.data,
                monoFont.size,
                48.0f,
                atlasW,
                atlasH,
                k_txl_shared_chars
            ))
            abort();

        {
            const int glyphCount = this->stbFonts[2].useCustomChars
                ? this->stbFonts[2].customCharCount
                : this->stbFonts[2].charCount;
            const float atlasArea = (float)(atlasW * atlasH);
            const float usedPct = atlasArea > 0.0f
                ? (100.0f * (float)this->stbFonts[2].usedPixelCount / atlasArea)
                : 0.0f;
            printf(
                "[TXL Font] mono chars=%d atlas_used=%d/%d (%.2f%%)\n",
                glyphCount,
                this->stbFonts[2].usedPixelCount,
                atlasW * atlasH,
                usedPct
            );
        }
    }

    inline Clay_String txl(TxlKey key)
    {
        return ClayArena_AllocString(&this->clayArena, Txl_Get(uiLanguage, key));
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
    
    static inline Clay_String clayInt(char* buf, int capacity, int value)
    {
        if (!buf || capacity <= 0)
            return { false, 0, nullptr };

        int len = 0;

        if (value == 0)
        {
            if (capacity > 1)
                buf[len++] = '0';
        }
        else
        {
            int v = value;
            char tmp[16];
            int t = 0;

            while (v > 0 && t < (int)sizeof(tmp))
            {
                tmp[t++] = char('0' + (v % 10));
                v /= 10;
            }

            while (t-- && len < capacity)
                buf[len++] = tmp[t];
        }

        return {
            .isStaticallyAllocated = true,
            .length = len,
            .chars = buf,
        };
    }
    

    static inline Clay_String
    rollSymbol2(char *buf, int roll, int /*prev*/, bool isStrike, bool isSpare, bool secondRoll)
    {
        if (roll < 0)
            return CLAY_STRING(" ");

        if (isStrike && !secondRoll)
            return CLAY_STRING("X");

        if (secondRoll && isSpare)
            return CLAY_STRING("/");

        if (roll == 10 && secondRoll && isStrike)
            return CLAY_STRING("X"); // second roll can be strike in frame-10

        if (roll == 0)
            return CLAY_STRING("-");

        return clayInt(buf, 2, roll);
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
        char *username,
        int32_t *username_len
    )
    {
        // Backward-compatible wrapper (single scoreboard, neutral styling).
        constructClayScoreboardStyled(sb, boardWidth, username, username_len, /*isActiveTurn=*/true, /*boardKey=*/0);
    }

    void constructClayScoreboardStyled(
        const BowlingScoreboard *sb,
        float boardWidth,
        char *username,
        int32_t *username_len,
        bool isActiveTurn,
        int boardKey
    )
    {
        float u1 = boardWidth / 24; // + 9*2 + 3 + 3(result) = 24
        float u2 = 2 * u1;
        float u3 = 3 * u1;
        uint16_t bigSize = (boardWidth < 600 ? 32 : 64);
        Clay_TextElementConfig bigFontCfg = {
            .textColor = {255, 25, 25, 255},
            .fontId = CLAY_FONT_NOTO,
            .fontSize = (uint16_t)(boardWidth < 600 ? 32 : 64),
        };

        float smallSquare = 16;
        int cumulative[10];
        int running = 0;
        const int base = glm::clamp(boardKey, 0, 1) * 50;

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

        // BOT mode renders two scoreboards; keep their identity colors stable:
        // - player (boardKey=0): blue
        // - Angel  (boardKey=1): red
        // When not active, keep the background fully white (no tint).
        const bool isAngel = (boardKey == 1);
        const Clay_Color roleBorder = isAngel ? Clay_Color{180, 60, 60, 255} : Clay_Color{60, 120, 220, 255};
        const Clay_Color roleBg = isAngel ? Clay_Color{255, 235, 235, 255} : Clay_Color{225, 242, 255, 255};
        const Clay_Color inactiveBorder = Clay_Color{80, 80, 80, 255};
        const Clay_Color inactiveBg = Clay_Color{255, 255, 255, 255};

        Clay_Color bg = isActiveTurn ? roleBg : inactiveBg;
        Clay_Color border = isActiveTurn ? roleBorder : inactiveBorder;

        CLAY(
            CLAY_IDI("ScoreboardWrapper", boardKey),
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
                CLAY_IDI("ScoreboardBar", boardKey),
                {
                    .layout =
                        {
                            .sizing = {CLAY_SIZING_FIT(), CLAY_SIZING_FIT()},
                            .childGap = 0,
                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        },
                    .backgroundColor = bg,
                    .border = {.color = border, .width = CLAY_BORDER_ALL(2)},
                }
            )
            {

                /* -------- FRAMES -------- */
                for (int i = 0; i < 10; ++i)
                {
                    const Frame &f = sb->frames[i];
                    const bool last = (i == 9);
                    const int idx = boardKey * 100 + i;

                    const Clay_String r1 = rollSymbol2(
                        this->charBuf[base + i * 2 + 0], f.roll1, 0, f.isStrike, f.isSpare, false
                    );

                    const Clay_String r2 =
                        (!last && f.isStrike)
                            ? CLAY_STRING(" ")
                            : rollSymbol2(
                                  this->charBuf[base + i * 2 + 1],
                                  f.roll2,
                                  f.roll1,
                                  f.isStrike,
                                  f.isSpare,
                                  true
                              );

                    const Clay_String r3 = last
                        ? (f.roll3 < 0 ? CLAY_STRING(" ")
                                       : (f.roll3 == 10 ? CLAY_STRING("X")
                                                        : (f.roll3 == 0 ? CLAY_STRING("-")
                                                                       : clayInt(this->charBuf[base + 25], 2, f.roll3))))
                        : CLAY_STRING(" ");

                    CLAY(
                        CLAY_IDI("Frame", idx),
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
                            CLAY_IDI("RollRow", idx),
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
                                CLAY_IDI("Eachroll-1", idx),
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
                                CLAY_IDI("Eachroll-2", idx),
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
                                    CLAY_IDI("Eachroll-3", idx),
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
                            CLAY_IDI("Divider", idx),
                            {
                                .layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1)}},
                                .backgroundColor = {0, 0, 0, 255},
                            }
                        ){};

                        /* -------- CUMULATIVE -------- */
                        CLAY(
                            CLAY_IDI("ScoreRow", idx),
                            {
                                .layout = {
                                    .sizing = {CLAY_SIZING_FIXED(u2), CLAY_SIZING_FIXED(u2)},
                                    .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                },
                            }
                        )
                        {
                            if (cumulative[i] >= 0)
                            {
                                Clay_TextElementConfig scoreTextCfg = smallFontCfg;
                                if (Bowling_FrameHasAnySplit(f))
                                    scoreTextCfg.textColor = (Clay_Color){220, 36, 36, 255};
                                CLAY_TEXT(
                                    clayInt(this->charBuf[base + 30 + i], 5, cumulative[i]),
                                    CLAY_TEXT_CONFIG(scoreTextCfg)
                                );
                            }
                        };
                    };
                }

                /* -------- TOTAL -------- */
                CLAY(
                    CLAY_IDI("Total result section", boardKey),
                    {
                        .layout = {
                            .sizing = {CLAY_SIZING_FIXED(u3), CLAY_SIZING_GROW(u3)},
                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                        },
                    }
                )
                {
                    CLAY_TEXT(
                        clayInt(this->charBuf[base + 26], 16, sb->totalScore),
                        CLAY_TEXT_CONFIG(bigFontCfg)
                    );
                }
            };
        };
    }
};
