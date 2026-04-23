#include "../shop.h"

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

// Draw a single catalog item card
void DrawCatalogItem(
    UserContext *usr,
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
    ClayArena *arena = &usr->clayArena; // ← Embedded arena

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
                 .backgroundColor = {180, 180, 220, (float)(Clay_Hovered() ? 120 : 255)}}
            )
            {
                CLAY_TEXT(CLAY_STRING("BALL NAME"), CLAY_TEXT_CONFIG(bodyCfg));

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
                    int len = snprintf(rarityLableBuf, sizeof(rarityLableBuf), "%s", rarity);
                    Clay_String rarityLable = ClayArena_AllocString(arena, rarityLableBuf);
                    CLAY_TEXT(rarityLable, CLAY_TEXT_CONFIG(rarityCfg));
                }
            }

            // Ball image preview area
            CLAY(CLAY_IDI("BallPreview", nr), CLAY_THEME_BALL_PREVIEW)
            {
                CLAY(
                    CLAY_IDI("IconImage", nr),
                    {.layout =
                         {.sizing =
                              {.width = CLAY_SIZING_FIXED(100), .height = CLAY_SIZING_FIXED(120)}},
                     .image = {.imageData = &usr->clayton.pinImage}}
                )
                {
                }
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
                DrawStatRow(arena, "MASS", mass, nr + 10);
                DrawStatRow(arena, "SPIN", spin, nr + 20);
                DrawStatRow(arena, "SKID", skid, nr + 30);
                DrawStatRow(arena, "BITE", bite, nr + 40);
            }

            // // Buy button (disabled if can't afford)
            char buf[64];
            int len = snprintf(buf, sizeof(buf), "%s", "BUY NOW");
            Clay_String lable = ClayArena_AllocString(arena, buf);
            if (canAfford)
            {
                CLAY(CLAY_IDI("BuyButton", nr), CLAY_THEME_BTN_BUY)
                {
                    CLAY_TEXT(lable, CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_BUTTON));
                }
            }
            else
            {
                Clay_TextElementConfig disabledCfg = {
                    .textColor = CLAY_COLOR_TEXT_SECONDARY,
                    .fontId = CLAY_FONT_NOTO,
                    .fontSize = CLAY_FONT_SIZE_SM
                };
                CLAY(CLAY_IDI("BuyButtonDisabled", nr), CLAY_THEME_BTN_BUY_DISABLED)
                {
                    CLAY_TEXT(lable, CLAY_TEXT_CONFIG(disabledCfg));
                }
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
    std::cerr << "Carousel pointer is now down" << std::endl;

    SDL_SetRelativeMouseMode(SDL_TRUE);

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
    cs->scrollOffset += dx * scale;
    // std::cerr << "scale=" << scale << std::endl;
}

void Carousel_OnPointerUp(CarouselState *cs, float x, float /*y*/, float deltaTime)
{
    SDL_SetRelativeMouseMode(SDL_FALSE);
    cs->isGrabbed = false;
    std::cerr << "Carousel pointer stops" << std::endl;

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
    // std::cerr << " slotWidth=" << slotWidth << " nearest=" << nearest << " targetPos=" << targetPos
    //           << std::endl;
}
// ============================================================================
// CAROUSEL: RENDER (Clay UI integration)
// ============================================================================
void Carousel_RenderCard(UserContext *usr, int absIdx, CarouselState *cs, int nr)
{
    const CatalogItem *item = &cs->items[absIdx]; // 👈 use wired data
    bool canAfford = (usr->bank >= item->price);

    Clay_Color tint = {255, 255, 255, static_cast<float>(255)};

    CLAY(
        CLAY_IDI("CarouselCard", nr),
        {
            .layout = {
                .sizing = {CLAY_SIZING_PERCENT(CAROUSEL_CARD_WIDTH), CLAY_SIZING_GROW()},
            },
        }
    )
    {
        // Reuse your existing card renderer (adjust nr to avoid ID conflicts)
        DrawCatalogItem(
            usr,
            item->name,
            item->rarity,
            item->price,
            item->mass,
            item->spin,
            item->skid,
            item->bite,
            usr->bank >= item->price,
            nr + 100
        );
    }
}

void Carousel_Render(CarouselState *cs, UserContext *usr, const CatalogItem *items, int count)
{
    // Outer container with horizontal clipping + scroll offset

    float offset = Carousel_GetCenteredOffset(cs);
    CLAY(
        CLAY_ID("CarouselContainer"),
        {.layout =
             {
                 .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(420)},
                 .padding = {10, 10, 10, 10},
             },
         .backgroundColor = {0, 0, 0, 100},
         .clip = {.horizontal = true, .vertical = false, .childOffset = {offset, 0}}}
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
            for (int i = 0; i < cs->cardCount; i++)
            { // 👈 uses wired count
                Carousel_RenderCard(usr, i, cs, i);
            }
        }
    }
}

// ============================================================================
// INIT & INTEGRATION HELPERS
// ============================================================================

// Replace your ShopGrid section with this call
void RenderShopUI_Carousel(float playerCoins, const char *resetCountdown, UserContext *usr)
{
    ClayArena *arena = &usr->clayArena;

    Clay_TextElementConfig titleCfg = CLAY_THEME_TEXT_TITLE;
    Clay_TextElementConfig priceCfg = CLAY_THEME_TEXT_PRICE;
    Clay_TextElementConfig buttonCfg = CLAY_THEME_TEXT_BUTTON;
    Clay_TextElementConfig countdownCfg = CLAY_THEME_TEXT_COUNTDOWN;

    CLAY(CLAY_ID("ShopOverlay"), CLAY_THEME_OVERLAY)
    {
        CLAY(CLAY_ID("ShopContainer"), CLAY_THEME_SHOP_CONTAINER)
        {

            CLAY(CLAY_ID("ShopPaddingAboveCarousel"), CLAY_THEME_SHOP_CONTAINER_PADDING)
            {

                CLAY(
                    CLAY_ID("ShopTitle"),
                    {.layout = {
                         .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                         .padding = {0, 0, 5, 0},
                         .childGap = 10,
                         .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                         .layoutDirection = CLAY_LEFT_TO_RIGHT
                     }}
                )
                {
                    CLAY_TEXT(CLAY_STRING("SHOP: IMPROVE YOUR RUN"), CLAY_TEXT_CONFIG(titleCfg));
                    CLAY(
                        CLAY_ID("TitleDividerShop"),
                        {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(1)}}}
                    ){ };
                    CLAY(usr->closeShopClick.clayId, CLAY_THEME_BTN_DANGER)
                    {
                        CLAY_TEXT(CLAY_STRING("x"), CLAY_TEXT_CONFIG(buttonCfg));
                    }
                }
                CLAY(CLAY_ID("ShopHeader"), CLAY_THEME_SHOP_HEADER)
                {
                    CLAY_TEXT(CLAY_STRING("Current balance"), CLAY_TEXT_CONFIG(titleCfg));
                    char bankAmountBuf[64];
                    int len = snprintf(bankAmountBuf, sizeof(bankAmountBuf), "$ %d", usr->bank);
                    Clay_String bankAmount = ClayArena_AllocString(arena, bankAmountBuf);
                    CLAY_TEXT(bankAmount, CLAY_TEXT_CONFIG(priceCfg));
                }
            }

            Carousel_Render(&usr->carousel, usr, usr->carousel.items, usr->carousel.cardCount);

            CLAY(CLAY_ID("ShopPaddingBellowCarousel"), CLAY_THEME_SHOP_CONTAINER_PADDING)
            {

                CLAY(CLAY_ID("ShopFooter"), CLAY_THEME_SHOP_FOOTER)
                {
                    char cdBuf[64];
                    int len = snprintf(cdBuf, sizeof(cdBuf), "Resets in %s", "2Hours");
                    Clay_String countdownStr = ClayArena_AllocString(arena, cdBuf);
                    CLAY_TEXT(countdownStr, CLAY_TEXT_CONFIG(countdownCfg));
                }
            }
        }
    }
}