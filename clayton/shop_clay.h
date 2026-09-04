#pragma once

#include "../shop.h"
#include "./clayton.h"

typedef struct ShopWindowRenderData
{
    int playerCoins;
    const char *countdownLabel;
    const char *actionLabel;
    const char *disabledActionLabel;
    const char *emptyStateLabel;
    bool inventoryTabActive;
    bool hasCards;
    bool actionEnabled;
    float transitionOffsetX;
    float transitionFlashAlpha;
} ShopWindowRenderData;

static inline Clay_Color
Shop_ButtonHoverColor(Clay_ElementId id, Clay_Color base, float rgbLift = 24.0f)
{
    if (!Clay_PointerOver(id))
        return base;

    Clay_Color out = base;
    out.r = glm::min(255.0f, out.r + rgbLift);
    out.g = glm::min(255.0f, out.g + rgbLift);
    out.b = glm::min(255.0f, out.b + rgbLift);
    return out;
}

static inline Clay_Color
Shop_TabColor(Clay_ElementId id, bool active, Clay_Color activeColor, Clay_Color inactiveColor, float inactiveRgbLift = 24.0f)
{
    return active ? activeColor : Shop_ButtonHoverColor(id, inactiveColor, inactiveRgbLift);
}

// Helper: Draw a single stat row with label + bar
void DrawStatRow(ClayArena *arena, const char *label, float value /* 0.0 to 1.0 */, int nr)
{
    CLAY(CLAY_IDI("StatRow", nr), CLAY_THEME_STAT_ROW)
    {
        char buf[64];
        int len = snprintf(buf, sizeof(buf), "%s", label);
        Clay_String lable = ClayArena_AllocString(arena, buf);
        CLAY_TEXT(lable, CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_STAT));
        CLAY(CLAY_IDI("StatBarBg", nr), CLAY_THEME_STAT_BAR_BG)
        {
            CLAY(CLAY_IDI("StatBarFill", nr), CLAY_THEME_STAT_BAR_FILL(value))
            {
            }
        }
    }
}

static inline const char *Txl_RarityLabel(TxlLanguage language, const char *rarity)
{
    if (!rarity)
        return Txl_Get(language, TXL_RARITY_COMMON);
    if (strcmp(rarity, "LEGENDARY") == 0)
        return Txl_Get(language, TXL_RARITY_LEGENDARY);
    if (strcmp(rarity, "EPIC") == 0)
        return Txl_Get(language, TXL_RARITY_EPIC);
    if (strcmp(rarity, "RARE") == 0)
        return Txl_Get(language, TXL_RARITY_RARE);
    return Txl_Get(language, TXL_RARITY_COMMON);
}

// Draw a single catalog item card
void DrawCatalogItem(
    Clayton *clayton,
    CarouselState *carousel,
    const char *name,
    const char *rarity,
    float price,
    float mass,
    float spin,
    float skid,
    float bite,
    bool canAfford,
    int nr
)
{
    Clay_TextElementConfig labelCfg = CLAY_THEME_TEXT_LABEL;
    Clay_TextElementConfig buttonCfg = CLAY_THEME_TEXT_BUTTON;
    Clay_TextElementConfig titleCfg = CLAY_THEME_TEXT_TITLE;
    Clay_TextElementConfig scoreCfg = CLAY_THEME_TEXT_LARGE;
    Clay_TextElementConfig priceCfg = CLAY_THEME_TEXT_PRICE;
    Clay_TextElementConfig bodyCfg = CLAY_THEME_TEXT_BODY;
    Clay_TextElementConfig rarityCfg = CLAY_THEME_TEXT_RARITY;
    Clay_ElementDeclaration rarityBadgeDecl = CLAY_THEME_RARITY_BADGE;
    Clay_LayoutConfig rarityBadgeLayoutCfg = rarityBadgeDecl.layout;

    ClayArena *arena = &clayton->clayArena;

    Clay_Color tint = {255, 255, 255, static_cast<float>(255)};

    // Compute a preview height that keeps a 16:6 aspect ratio while filling the card width.
    // (We subtract wrapper+card padding so the image area visually fills the card.)
    Clay_ElementData beltCd = Clay_GetElementData(CLAY_ID("CarouselBelt"));
    float slotWidthPx = (float)beltCd.boundingBox.width * CAROUSEL_CARD_WIDTH;
    float previewWidthPx = slotWidthPx - 48.0f; // wrapper padding (12*2) + card padding (12*2)
    if (previewWidthPx < 120.0f)
        previewWidthPx = 120.0f;
    float previewHeightPx = previewWidthPx * (6.0f / 16.0f);
    if (previewHeightPx < 60.0f)
        previewHeightPx = 60.0f;

    CLAY(
        CLAY_IDI("CarouselCard", nr),
        {
            .layout = {
                .sizing = {CLAY_SIZING_PERCENT(CAROUSEL_CARD_WIDTH), CLAY_SIZING_GROW()},
            },
        }
    )
    {

        CLAY(
            CLAY_IDI("CatalogItemWrapper", nr),
            {

                .layout = {
                    .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                    .padding = {12, 12, 12, 12},
                }
            }
        )
        {
            CLAY(CLAY_IDI("CatalogItem", nr), CLAY_THEME_CATALOG_ITEM)
            {

                CLAY(
                    CLAY_IDI("ItemHeader", nr),
                    {.layout =
                         {
                             .sizing =
                                 {
                                     CLAY_SIZING_GROW(),
                                     CLAY_SIZING_GROW(),
                                 },
                             .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                             .layoutDirection = CLAY_LEFT_TO_RIGHT,
                         },
                     .backgroundColor = {180, 180, 220, (float)(Clay_Hovered() ? 120 : 180)}}
                )
                {
                    // Use ClayArena so the label is easy to format/debug and consistent with other
                    // UI strings.
                    const char *ballName = (name && name[0]) ? name : "BALL";
                    Clay_String ballNameStr = ClayArena_FormatString(arena, "%s", ballName);
                    CLAY_TEXT(ballNameStr, CLAY_TEXT_CONFIG(bodyCfg));

                    CLAY(
                        CLAY_IDI("BalltitleSpacer", nr),
                        {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1)}}}
                    ){};

                    Clay_Color rarityColor = CLAY_COLOR_RARITY_COMMON;
                    if (strcmp(rarity, "LEGENDARY") == 0)
                        rarityColor = CLAY_COLOR_RARITY_LEGENDARY;
                    if (strcmp(rarity, "EPIC") == 0)
                        rarityColor = CLAY_COLOR_RARITY_EPIC;
                    if (strcmp(rarity, "RARE") == 0)
                        rarityColor = CLAY_COLOR_RARITY_RARE;
                    CLAY(
                        CLAY_IDI("RarityBadge", nr),
                        {.layout = rarityBadgeLayoutCfg, .backgroundColor = rarityColor}
                    )
                    {
                        char rarityLableBuf[64];
                        int len = snprintf(
                            rarityLableBuf,
                            sizeof(rarityLableBuf),
                            "%s",
                            Txl_RarityLabel(clayton->uiLanguage, rarity)
                        );
                        Clay_String rarityLable = ClayArena_AllocString(arena, rarityLableBuf);
                        CLAY_TEXT(rarityLable, CLAY_TEXT_CONFIG(rarityCfg));
                    }
                }

                // Ball image preview area (16:6, fill width)
                CLAY(
                    CLAY_IDI("BallPreview", nr),
                    {
                        .layout =
                            {
                                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(previewHeightPx)},
                                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                            },
                        .backgroundColor = CLAY_COLOR_PANEL_SECTION,
                        .cornerRadius = {
                            CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD
                        },
                    }
                )
                {
                    if (nr == carousel->closestBallIdx)
                    {
                        CLAY(
                            CLAY_IDI("IconImage1-", nr),
                            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()}},
                             .image = {.imageData = &clayton->pinImage}}
                        )
                        {
                        }
                    }
                    if (nr == carousel->closest2ndBallIdx)
                    {
                        CLAY(
                            CLAY_IDI("IconImage2-", nr),
                            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()}},
                             .image = {.imageData = &clayton->pin2Image}}
                        )
                        {
                        }
                    }
                    if (nr == carousel->closest3rdBallIdx)
                    {
                        CLAY(
                            CLAY_IDI("IconImage3-", nr),
                            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()}},
                             .image = {.imageData = &clayton->pin3Image}}
                        )
                        {
                        }
                    }
                    // std::cerr
                    //     << " nr=" << nr
                    //     << " usr->carousel.closestBallIdx=" << usr->carousel.closestBallIdx
                    //     << " usr->carousel.closestBall2Idx=" << usr->carousel.closest2ndBallIdx
                    //     << std::endl;
                }

                // Price row
                CLAY(CLAY_IDI("PriceRow", nr), CLAY_THEME_PRICE_ROW)
                {
                    char buf[64];
                    int len = snprintf(buf, sizeof(buf), "%.0f", price);
                    Clay_String lable = ClayArena_AllocString(arena, buf);
                    CLAY_TEXT(lable, CLAY_TEXT_CONFIG(priceCfg));
                }
                // Stats section
                CLAY(
                    CLAY_IDI("StatsSection", nr),
                    {.layout = {
                         .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                         .childGap = 4,
                         .layoutDirection = CLAY_TOP_TO_BOTTOM,
                     }}
                )
                {
                    DrawStatRow(arena, Txl_Get(clayton->uiLanguage, TXL_MASS), mass, nr + 1000);
                    DrawStatRow(arena, Txl_Get(clayton->uiLanguage, TXL_SPIN), spin, nr + 2000);
                    DrawStatRow(arena, Txl_Get(clayton->uiLanguage, TXL_SKID), skid, nr + 3000);
                    DrawStatRow(arena, Txl_Get(clayton->uiLanguage, TXL_BITE), bite, nr + 4000);
                }

                // // Buy button (disabled if can't afford)
            }
        }
    }
}

// ============================================================================
// CAROUSEL: INPUT HANDLERS (call from unified pointer system)
// ============================================================================

inline float Carousel_GetCenteredOffset(const CarouselState *cs)
{
    float offset = cs->scrollOffset;
    Clay_ElementData cd = Clay_GetElementData(CLAY_ID("CarouselBelt"));
    float cardWidth = (float)cd.boundingBox.width * CAROUSEL_CARD_WIDTH;
    float baseOffset = ((float)cd.boundingBox.width - cardWidth) / 2.0f;
    float centeredOffset = baseOffset + offset;
    return centeredOffset;
}

inline float Carousel_GetSlotWidth(const CarouselState *cs)
{
    Clay_ElementData cd = Clay_GetElementData(CLAY_ID("CarouselBelt"));
    return (float)cd.boundingBox.width * CAROUSEL_CARD_WIDTH;
}

void Carousel_OnPointerDown(CarouselState *cs, float x, float y, float time)
{
    if (!Clay_PointerOver(CLAY_ID("CarouselContainer")))
    {
        return;
    }
    // std::cerr << "Carousel pointer is now down" << std::endl;

    cs->isGrabbed = true;
    cs->startingX = x;
}

void Carousel_OnPointerMove(CarouselState *cs, float x, float /*y*/)
{
    if (!cs->isGrabbed)
        return;

    float slotWidth = Carousel_GetSlotWidth(cs);
    float dx = x; // your relative delta - trusted & working ✓

    // === SAME NEAREST-SLOT LOGIC AS UPDATE ===
    int nearest = (int)glm::round(cs->scrollOffset / slotWidth);
    nearest = glm::clamp(nearest, 0, 1 - cs->cardCount);
    float targetPos = (float)nearest * slotWidth;
    float error = targetPos - cs->scrollOffset;
    float dist = glm::abs(error);

    // Normalize: 0.0 = on slot, 1.0 = at midpoint between slots
    float normDist = glm::min(dist / (0.5f * slotWidth), 1.0f);

    // Sensitivity curve: matches Update's linear proportionality

#if defined(__EMSCRIPTEN__)
    const float MIN_SCALE = 0.8f; // "sticky" when on slot (hard to overshoot)
    const float MAX_SCALE = 1.6f; // full sensitivity at midpoint
#else
    const float MIN_SCALE = 0.3f; // "sticky" when on slot (hard to overshoot)
    const float MAX_SCALE = 0.6f; // full sensitivity at midpoint
#endif
    float scale = glm::mix(MIN_SCALE, MAX_SCALE, normDist);

    // Optional: subtle curve sharpening for more pronounced detent feel
    scale = MIN_SCALE + (MAX_SCALE - MIN_SCALE) * (normDist * normDist);
    cs->scrollOffset += dx * scale * 1.5f;

    Carousel_UpdateClosestIndices(cs, slotWidth);

    // std::cerr << "scale=" << scale << std::endl;
}

void Carousel_OnPointerUp(CarouselState *cs, float x, float /*y*/, float deltaTime)
{
    cs->isGrabbed = false;
    // std::cerr << "Carousel pointer stops" << std::endl;

    float v = x / glm::max(deltaTime, 1e-4f);      // px/s from last delta
    cs->velocity = 0.7f * cs->velocity + 0.3f * v; // light smoothing
    cs->velocity = 0.0f;
}

// ============================================================================
// CAROUSEL: UPDATE (call every frame with deltaTime)
// ============================================================================

void Carousel_Update(CarouselState *cs, float deltaTime)
{
    if (cs->isGrabbed)
        return;

    float slotWidth = Carousel_GetSlotWidth(cs);

    // Find nearest slot and calculate error
    int nearest = (int)glm::round(cs->scrollOffset / slotWidth);
    nearest = glm::clamp(nearest, 1 - cs->cardCount, 0);
    float targetPos = (float)nearest * slotWidth;
    float error = targetPos - cs->scrollOffset;

    // === PROPORTIONAL VELOCITY TARGET ===
    // Desired velocity scales with distance: fast when far, slow when near, zero at target
    const float Kp = 8.0f; // gain: pixels/s of velocity per pixel of error
    float targetVelocity = error * Kp;

    // === SMOOTH VELOCITY APPROACH ===
    // Instead of "friction", we blend current velocity toward the target velocity
    // Higher approachRate = snappier response; lower = smoother, more gradual
    const float approachRate = 12.0f;                         // 1/s
    float blend = 1.0f - glm::exp(-approachRate * deltaTime); // frame-rate independent
    cs->velocity += (targetVelocity - cs->velocity) * blend;

    // Integrate position
    cs->scrollOffset += cs->velocity * deltaTime;

    // === OPTIONAL: hard-snap at equilibrium for perfect precision ===
    if (glm::abs(error) < 0.1f && glm::abs(cs->velocity) < 0.5f)
    {
        cs->scrollOffset = targetPos;
        cs->velocity = 0.0f;
    }

    Carousel_UpdateClosestIndices(cs, slotWidth);

    // std::cerr << " slotWidth=" << slotWidth << " nearest=" << nearest << " targetPos=" <<
    // targetPos
    //           << std::endl;
}

static inline void Carousel_FocusIndexWhenLayoutReady(CarouselState *cs, BallShopState *ballShop)
{
    if (!cs || !ballShop || ballShop->pendingFocusIndex < 0 || cs->cardCount <= 0)
        return;

    const float slotWidth = Carousel_GetSlotWidth(cs);
    if (slotWidth <= 0.0f)
        return;

    const int idx = glm::clamp(ballShop->pendingFocusIndex, 0, cs->cardCount - 1);
    cs->scrollOffset = -(float)idx * slotWidth;
    cs->velocity = 0.0f;
    cs->isDragging = false;
    cs->isAutoDragging = false;
    cs->isGrabbed = false;
    Carousel_UpdateClosestIndices(cs, slotWidth);
    ballShop->pendingFocusIndex = -1;
}

void Carousel_Render(
    Clayton *clayton,
    CarouselState *carousel,
    float transitionOffsetX = 0.0f,
    BallShopState *ballShop = nullptr
)
{
    CatalogItem *items = carousel->items;
    int count = carousel->cardCount;

    Carousel_FocusIndexWhenLayoutReady(carousel, ballShop);

    // Outer container with horizontal clipping + scroll offset
    float offset = Carousel_GetCenteredOffset(carousel) + transitionOffsetX;
    CLAY(
        CLAY_ID("CarouselContainer"),
        {
            .layout =
                {
                    .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(360)},
                    .padding = {10, 10, 10, 10},
                },
            .backgroundColor = {0, 0, 0, 100},
            .cornerRadius = {CLAY_RADIUS_XL, CLAY_RADIUS_XL, CLAY_RADIUS_XL, CLAY_RADIUS_XL},
            .clip = {.horizontal = true, .vertical = false, .childOffset = {offset, 0}},
            // .border = {
            //     .color = CLAY_COLOR_BORDER,
            //     .width = CLAY_BORDER_OUTSIDE(CLAY_BORDER_WIDTH + 3),
            // },
        }
    )
    {
        // Horizontal row of cards
        CLAY(
            CLAY_ID("CarouselBelt"),
            {.layout = {
                 .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                 .childGap = (int)CAROUSEL_CARD_SPACING,
                 .layoutDirection = CLAY_LEFT_TO_RIGHT,
             }}
        )
        {
            for (int i = 0; i < carousel->cardCount; i++)
            {
                const CatalogItem *item = &carousel->items[i];
                DrawCatalogItem(
                    clayton,
                    carousel,
                    item->name,
                    item->rarity,
                    item->price,
                    item->mass,
                    item->spin,
                    item->skid,
                    item->bite,
                    carousel->bank >= item->price,
                    i
                );
            }
        }
    }
}

// ============================================================================
// INIT & INTEGRATION HELPERS
// ============================================================================

// Replace your ShopGrid section with this call
inline void RenderShopWindow_Carousel(
    Clayton *clayton,
    CarouselState *carousel,
    BallShopState *ballShop,
    const ShopWindowRenderData *renderData
)
{
    ClayArena *arena = &clayton->clayArena;

    Clay_TextElementConfig titleCfg = CLAY_THEME_TEXT_TITLE;
    Clay_TextElementConfig priceCfg = CLAY_THEME_TEXT_PRICE;
    Clay_TextElementConfig buttonCfg = CLAY_THEME_TEXT_BUTTON;
    Clay_TextElementConfig countdownCfg = CLAY_THEME_TEXT_COUNTDOWN;
    const ShopWindowRenderData fallbackData = {
        .playerCoins = carousel ? carousel->bank : 0,
        .countdownLabel = "--",
        .actionLabel = clayton->shopActionLabel ? clayton->shopActionLabel
                                                : Txl_Get(clayton->uiLanguage, TXL_BUY_NOW),
        .disabledActionLabel = Txl_Get(clayton->uiLanguage, TXL_CANT_AFFORD),
        .emptyStateLabel = Txl_Get(clayton->uiLanguage, TXL_SHOP_EMPTY),
        .inventoryTabActive = false,
        .hasCards = carousel && carousel->cardCount > 0,
        .actionEnabled = false,
        .transitionOffsetX = 0.0f,
        .transitionFlashAlpha = 0.0f,
    };
    const ShopWindowRenderData &data = renderData ? *renderData : fallbackData;

    // Root container exists for pointer-hit testing in win_stack.
    CLAY(
        CLAY_ID("ShopOverlay"),
        {
            .layout = {
                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                .padding = {0, 0, 0, 0},
                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
            },
        }
    )
    {
        CLAY(CLAY_ID("ShopContainer"), CLAY_THEME_SHOP_CONTAINER)
        {

            CLAY(CLAY_ID("ShopPaddingAboveCarousel"), CLAY_THEME_SHOP_CONTAINER_PADDING)
            {

                CLAY(
                    CLAY_ID("ShopTitle"),
                    {.layout = {
                         .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                         .padding = {0, 0, 16, 16},
                         .childGap = 10,
                         .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                         .layoutDirection = CLAY_LEFT_TO_RIGHT
                     }}
                )
                {
                    CLAY_TEXT(clayton->txl(TXL_BALLS), CLAY_TEXT_CONFIG(titleCfg));
                    CLAY(
                        CLAY_ID("TitleDividerShop"),
                        {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(1)}}}
                    ){};
                    char bankAmountBuf[64];
                    int len =
                        snprintf(bankAmountBuf, sizeof(bankAmountBuf), "$ %d", data.playerCoins);
                    Clay_String bankAmount = ClayArena_AllocString(arena, bankAmountBuf);
                    CLAY_TEXT(bankAmount, CLAY_TEXT_CONFIG(priceCfg));
                    CLAY(clayton->closeShopClick.clayId, CLAY_THEME_BTN_DANGER)
                    {
                        CLAY_TEXT(CLAY_STRING("x"), CLAY_TEXT_CONFIG(buttonCfg));
                    }
                }
            }

            CLAY(
                CLAY_ID("BallTabs"),
                {.layout = {
                     .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                     .padding = {12, 12, 0, 0},
                     .childGap = 8,
                     .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_BOTTOM},
                     .layoutDirection = CLAY_LEFT_TO_RIGHT
                 }}
            )
            {
                Clay_ElementDeclaration inventoryTab = CLAY_THEME_BTN_PRIMARY;
                Clay_ElementDeclaration shopTab = CLAY_THEME_BTN_PRIMARY;
                inventoryTab.cornerRadius.bottomLeft = 0;
                inventoryTab.cornerRadius.bottomRight = 0;
                shopTab.cornerRadius.bottomLeft = 0;
                shopTab.cornerRadius.bottomRight = 0;
                inventoryTab.backgroundColor = Shop_TabColor(
                    clayton->shopInventoryTabClick.clayId,
                    data.inventoryTabActive,
                    CLAY_COLOR_PANEL_SECTION,
                    CLAY_COLOR_TAB_INACTIVE
                );
                shopTab.backgroundColor = Shop_TabColor(
                    clayton->shopStoreTabClick.clayId,
                    !data.inventoryTabActive,
                    CLAY_COLOR_PANEL_SECTION,
                    CLAY_COLOR_TAB_INACTIVE
                );
                CLAY(clayton->shopInventoryTabClick.clayId, inventoryTab)
                {
                    CLAY_TEXT(clayton->txl(TXL_INVENTORY), CLAY_TEXT_CONFIG(buttonCfg));
                }
                CLAY(clayton->shopStoreTabClick.clayId, shopTab)
                {
                    CLAY_TEXT(clayton->txl(TXL_SHOP), CLAY_TEXT_CONFIG(buttonCfg));
                }
            }

            CLAY(
                CLAY_ID("BallsTabbedContent"),
                {
                    .layout =
                        {
                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                            .padding = {24, 24, 24, 24},
                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                            .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        },
                    .backgroundColor = CLAY_COLOR_PANEL_SECTION,

                    .cornerRadius = {0, 0, CLAY_RADIUS_XL, CLAY_RADIUS_XL},
                }
            )
            {
                CLAY(
                    CLAY_ID("BallsTabbedContentInner"),
                    {
                        .layout =
                            {
                                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                            },
                    }
                )
                {
                    if (data.hasCards)
                    {
                        Carousel_Render(clayton, carousel, data.transitionOffsetX, ballShop);
                    }
                    else
                    {
                        CLAY(
                            CLAY_ID("ShopEmptyState"),
                            {
                                .layout =
                                    {
                                        .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(420)},
                                        .padding = {24, 24, 24, 24},
                                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                    },
                                .backgroundColor = CLAY_COLOR_PANEL_SECTION,
                            }
                        )
                        {
                            CLAY_TEXT(
                                ClayArena_FormatString(
                                    arena, "%s", data.emptyStateLabel ? data.emptyStateLabel : ""
                                ),
                                CLAY_TEXT_CONFIG(titleCfg)
                            );
                        }
                    }

                    CLAY(
                        CLAY_ID("BeforyBuySpacer"),
                        {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(20)}}}
                    ){};

                    Clay_ElementDeclaration bottomPadding = CLAY_THEME_SHOP_CONTAINER_PADDING;
                    bottomPadding.layout.padding.top = 20;
                    bottomPadding.layout.padding.bottom = 20;
                    CLAY(CLAY_ID("ShopPaddingBellowCarousel"), bottomPadding)
                    {
                        const CatalogItem *item = (data.hasCards && carousel->closestBallIdx >= 0 &&
                                                   carousel->closestBallIdx < carousel->cardCount)
                            ? &carousel->items[carousel->closestBallIdx]
                            : nullptr;
                        (void)item;

                        if (data.actionEnabled)
                        {
                            char buf[64];
                            int len = snprintf(
                                buf, sizeof(buf), "%s", data.actionLabel ? data.actionLabel : ""
                            );
                            Clay_String lable = ClayArena_AllocString(arena, buf);

                            CLAY(clayton->buyClick.clayId, CLAY_THEME_BTN_BUY)
                            {
                                CLAY_TEXT(lable, CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_BUTTON));
                            }
                        }
                        else
                        {
                            char buf[64];
                            int len = snprintf(
                                buf,
                                sizeof(buf),
                                "%s",
                                data.disabledActionLabel ? data.disabledActionLabel : ""
                            );
                            Clay_String lable = ClayArena_AllocString(arena, buf);
                            Clay_TextElementConfig disabledCfg = {
                                .textColor = CLAY_COLOR_TEXT_SECONDARY,
                                .fontId = CLAY_FONT_NOTO,
                                .fontSize = CLAY_FONT_SIZE_SM
                            };
                            CLAY(CLAY_ID("BuyButtonDisabled"), CLAY_THEME_BTN_BUY_DISABLED)
                            {
                                CLAY_TEXT(lable, CLAY_TEXT_CONFIG(disabledCfg));
                            }
                        }
                        CLAY(CLAY_ID("ShopFooter"), CLAY_THEME_SHOP_FOOTER)
                        {
                            char cdBuf[64];
                            int len = snprintf(
                                cdBuf,
                                sizeof(cdBuf),
                                Txl_Get(clayton->uiLanguage, TXL_RESETS_IN_FMT),
                                data.countdownLabel ? data.countdownLabel : "--"
                            );
                            Clay_String countdownStr = ClayArena_AllocString(arena, cdBuf);
                            CLAY_TEXT(countdownStr, CLAY_TEXT_CONFIG(countdownCfg));
                        }
                    }
                }
            }
        }
    }
}

// Legacy wrapper: preserves the old overlay behavior for call sites that expect it.
inline void RenderShopUI_Carousel(
    Clayton *clayton, CarouselState *carousel, const ShopWindowRenderData *renderData
)
{
    CLAY(CLAY_ID("ShopOverlayDim"), CLAY_THEME_OVERLAY)
    {
        RenderShopWindow_Carousel(clayton, carousel, nullptr, renderData);
    }
}
