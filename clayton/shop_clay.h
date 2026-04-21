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
    Clay_String imagePlaceholder,
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

// ============================================================================
// CAROUSEL: INPUT HANDLERS (call from unified pointer system)
// ============================================================================
void Carousel_OnPointerDown(CarouselState *cs, float x, float y, float time)
{
    if (!Clay_PointerOver(CLAY_ID("CarouselContainer"))) {
        return;
    }
    std::cerr << "Carousel pointer is now down" << std::endl;

    SDL_SetRelativeMouseMode(SDL_TRUE);

    cs->isGrabbed = true;
    cs->startingX = x;
    // TODO UsE BELT metaphor wher belt sticks to positions
    // TODO need to remake, here we just selecting a card,
    // durng the move we get offset
    // float pitch = CAROUSEL_CARD_WIDTH + CAROUSEL_CARD_SPACING;
    // float localX = x - cs->containerX;
    // float worldX = localX - cs->scrollOffset;
    // int cardIdx = (int)(worldX / pitch);
    // if (cardIdx < 0 || cardIdx >= cs->cardCount)
    //     return;


    // int centerIdx = Carousel_NearestIndex(
    //     cs->scrollOffset,
    //     CAROUSEL_CARD_WIDTH,
    //     CAROUSEL_CARD_SPACING,
    //     cs->containerWidth,
    //     cs->cardCount
    // );


    // cs->pointerDownTime = time;
    // cs->pointerDownX = x;
    // cs->pointerCurrentX = x;
    // cs->totalMovement = 0.0f;
    // cs->pressedCardAbsoluteIndex = cardIdx;
    // cs->pressedCardRelativePos = cardIdx - centerIdx; // -1, 0, or +1

    // // Cancel ongoing animations
    // cs->isAutoDragging = false;
    // cs->animTargetIndex = -1;

    // // Center card never triggers autodrag
    // if (cs->pressedCardRelativePos == 0)
    // {
    //     cs->pressedCardRelativePos = 0; // lock to drag-only
    // }
}

void Carousel_OnPointerMove_bak(CarouselState *cs, float x, float y)
{
    Clay_ElementData cd = Clay_GetElementData(CLAY_ID("CarouselBelt"));
    int beltWidthInPx = cd.boundingBox.width;
    float cardWidthInPx = beltWidthInPx / cs->cardCount;

    int delta = x;
    if (cs->isGrabbed) {
        cs->scrollOffset += (int)(delta);
        std::cerr << "Carousel pointer moved to x=" << x << " ofset="<< cs->scrollOffset << " added: " << delta << std::endl;
    }
}

void Carousel_OnPointerMove(CarouselState *cs, float x, float /*y*/)
{
    if (!cs->isGrabbed)
        return;

    Clay_ElementData cd = Clay_GetElementData(CLAY_ID("CarouselBelt"));
    float beltWidthInPx = (float)cd.boundingBox.width;
    float w = beltWidthInPx / (float)cs->cardCount;

    // --- per-frame delta (fix your current bug: you used x directly) ---
    // float dx = x - cs->startingX;
    // cs->startingX = x;
    float dx = x ; // No this is not a bug it is dogs bollocks, trust me this works well 

    // --- current position on belt ---
    float pos = cs->scrollOffset;

    // --- find nearest slot ---
    int nearestIndex = (int)glm::round(pos / w);
    float slotPos = nearestIndex * w;

    // --- distance to slot ---
    float d = slotPos - pos;

    // --- directions ---
    float dirToSlot = (d > 0.0f) ? 1.0f : -1.0f;
    float dirMove   = (dx > 0.0f) ? 1.0f : -1.0f;

    // --- normalised distance (0 = at slot, 1 = midpoint) ---
    float t = glm::clamp(glm::abs(d) / (0.5f * w), 0.0f, 1.0f);

    // --- influence (strong near slot) ---
    float influence = 1.0f - t;
    influence *= influence; // sharpen (optional but recommended)

    // --- speed factor ---
    float factor = 1.0f;

    if (dirMove == dirToSlot) {
        // moving toward slot → speed up
        factor += influence * 1.8f;   // tune
    } else {
        // moving away → slow down
        factor -= influence * 0.8f;   // tune
    }

    // --- apply ---
    float delta = dx * factor;
    cs->scrollOffset += delta;

    std::cerr << "x=" << x
              << " dx=" << dx
              << " factor=" << factor
              << " offset=" << cs->scrollOffset
              << std::endl;
}

void Carousel_OnPointerUp(CarouselState *cs, float x, float /*y*/, float deltaTime)
{
    SDL_SetRelativeMouseMode(SDL_FALSE);
    cs->isGrabbed = false;
    std::cerr << "Carousel pointer stops" << std::endl;

    float v = x / glm::max(deltaTime, 1e-4f); // px/s from last delta
    cs->velocity = 0.7f * cs->velocity + 0.3f * v; // light smoothing
    cs->velocity = 0.0f;
}

void Carousel_Update(CarouselState *cs, float deltaTime)
{
    if (cs->isGrabbed) return;
    
    Clay_ElementData cd = Clay_GetElementData(CLAY_ID("CarouselBelt"));
    float slotWidth = (float)cd.boundingBox.width 
    // / (float)cs->cardCount;
    ;
    
    // Find nearest slot
    int nearest = (int)glm::round(cs->scrollOffset / slotWidth);
    float targetPos = (float)nearest * slotWidth;
    float error = targetPos - cs->scrollOffset;
    float dist = glm::abs(error);
    
    // === 1. GRAVITY PULL ===
    // Constant acceleration toward the nearest slot (direction only)
    const float GRAVITY = 7000.0f; // pixels/s²
    cs->velocity += glm::sign(error) * GRAVITY * deltaTime;
    
    // === 2. PROXIMITY-BASED DAMPING ===
    // Damping scales from low (far away) to high (on target)
    float proximity = 1.0f - glm::min(dist / slotWidth, 1.0f);
    
    // Quadratic ramp: gentle at first, aggressive in the last 30%
    float damping = 2.0f + 25.0f * (proximity * proximity);
    
    // Frame-rate independent exponential decay
    float decay = glm::clamp(glm::exp(-damping * deltaTime), 0.0f, 1.0f);
    cs->velocity *= decay;
    
    // === 3. INTEGRATE ===
    cs->scrollOffset += cs->velocity * deltaTime;
    
    // === 4. GUARANTEED SETTLE ===
    // Only snap when truly at equilibrium (discrete precision cleanup)
    if (dist < 0.15f && glm::abs(cs->velocity) < 1.0f) {
        cs->scrollOffset = targetPos;
        cs->velocity = 0.0f;
    }
    std::cerr 
        << " nearest=" << nearest
        << " targetPos=" << targetPos
        << std::endl;
}
// ============================================================================
// CAROUSEL: UPDATE (call every frame with deltaTime)
// ============================================================================

// ============================================================================
// CAROUSEL: RENDER (Clay UI integration)
// ============================================================================
void Carousel_RenderCard(UserContext *usr, int absIdx, CarouselState *cs, int nr)
{
    // int centerIdx = Carousel_NearestIndex(
    //     cs->scrollOffset,
    //     CAROUSEL_CARD_WIDTH,
    //     CAROUSEL_CARD_SPACING,
    //     cs->containerWidth,
    //     cs->cardCount
    // );
    // int relPos = absIdx - centerIdx;

    // // Visual feedback: opacity/scale based on position
    // uint8_t alpha = 255;
    // if (relPos == 0)
    //     alpha = 255;
    // else if (relPos == -1 || relPos == 1)
    //     alpha = 217; // ~0.85
    // else
    //     alpha = 0; // skip off-screen

    // if (alpha == 0)
    //     return;

    const CatalogItem *item = &cs->items[absIdx]; // 👈 use wired data
    bool canAfford = (usr->bank >= item->price);

    Clay_Color tint = {255, 255, 255, static_cast<float>(255)};

    CLAY(
        CLAY_IDI("CarouselCard", nr),
        {.layout =
             {
                 .sizing = {CLAY_SIZING_PERCENT(CAROUSEL_CARD_WIDTH), CLAY_SIZING_GROW()},
                //  .padding = {4, 4, 4, 4},
             },
         .backgroundColor = tint}
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
            item->imagePlaceholder,
            nr + 100
        );
    }
}

void Carousel_Render(CarouselState *cs, UserContext *usr, const CatalogItem *items, int count)
{
    // Outer container with horizontal clipping + scroll offset
    CLAY(
        CLAY_ID("CarouselContainer"),
        {.layout =
             {
                 .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(420)},
                 .padding = {10, 10, 10, 10},
             },
         .clip = {.horizontal = true, .vertical = false, .childOffset = {cs->scrollOffset, 0}}}
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

    // Cache container bounds for next frame's hit detection
    // (In practice, query Clay for actual bounds if available)
    cs->containerWidth = 800.0f; // adjust to your layout
    cs->containerHeight = 420.0f;
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
                    CLAY_ID("TitleDividerS"),
                    {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1)}}}
                ){};
                CLAY(usr->closeShopClick.clayId, CLAY_THEME_BTN_DANGER)
                {
                    CLAY_TEXT(CLAY_STRING("x"), CLAY_TEXT_CONFIG(buttonCfg));
                }
            }

            // Header: SHOP title + player currency
            CLAY(CLAY_ID("ShopHeaderd"), CLAY_THEME_SHOP_HEADER)
            {
                CLAY_TEXT(CLAY_STRING("SHOPqdd"), CLAY_TEXT_CONFIG(titleCfg));
                char bankAmountBuf[64];
                int len = snprintf(bankAmountBuf, sizeof(bankAmountBuf), "$ %d", usr->bank);
                Clay_String bankAmount = ClayArena_AllocString(arena, bankAmountBuf);
                CLAY_TEXT(bankAmount, CLAY_TEXT_CONFIG(priceCfg));
            }

            Carousel_Render(&usr->carousel, usr, usr->carousel.items, usr->carousel.cardCount);

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
inline void buildShopClay(UserContext *usr, Shop *shop)
{

    if (!usr || !shop)
        return;

    ClayArena *arena = &usr->clayArena; // ← Embedded arena

    // Theme font configs
    Clay_TextElementConfig labelCfg = CLAY_THEME_TEXT_LABEL;
    Clay_TextElementConfig buttonCfg = CLAY_THEME_TEXT_BUTTON;
    Clay_TextElementConfig titleCfg = CLAY_THEME_TEXT_TITLE;
    Clay_TextElementConfig scoreCfg = CLAY_THEME_TEXT_LARGE;

    // RenderShopUI(7.0f, "Cauntdaun", usr);
    RenderShopUI_Carousel(7.0f, "Cauntdaun", usr);
}