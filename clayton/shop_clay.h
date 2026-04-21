#include "../shop.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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

void Carousel_OnPointerMove(CarouselState *cs, float x, float y__)
{
    // if (cs->pressedCardAbsoluteIndex < 0)
    //     return;

    Clay_ElementData cd = Clay_GetElementData(CLAY_ID("CarouselBelt"));
    int beltWidthInPx = cd.boundingBox.width;
    float cardWidthInPx = beltWidthInPx / cs->cardCount;
    float w = cardWidthInPx;

    float k = glm::two_pi<float>() / w;   // frequency

    float amplitude = 16.0f;
    float y = amplitude * glm::sin(k * (x - 0.5f * w));


    float dx = x - cs->pointerDownX;
    cs->pointerCurrentX = x;
    cs->totalMovement = fabsf(dx);
    int delta = x - cs->startingX + y;
    cs->startingX = x;
    if (cs->isGrabbed) {

        // cs->scrollOffset = 0;
        cs->scrollOffset -= (int)(delta);
        std::cerr << "Carousel pointer moved to"<< cs->scrollOffset << " added: " << delta << std::endl;
    }


}

void Carousel_OnPointerUp(CarouselState *cs, float x, float y, float time)
{
    SDL_SetRelativeMouseMode(SDL_FALSE);
    // if (cs->pressedCardAbsoluteIndex < 0)
    //     return;

    float duration = time - cs->pointerDownTime;
    cs->isGrabbed = false;

        std::cerr << "Carousel pointer stops" << std::endl;
    // // AUTODRAG: short, still click on side card → animate to center
    // if (!cs->isDragging && cs->pressedCardRelativePos != 0 &&
    //     cs->totalMovement <= CAROUSEL_MOVEMENT_THRESHOLD &&
    //     duration <= CAROUSEL_CLICK_DURATION_THRESHOLD)
    // {
    //     cs->isAutoDragging = true;
    //     cs->animTargetIndex = cs->pressedCardAbsoluteIndex;
    //     cs->animStartOffset = cs->scrollOffset;
    //     cs->animStartTime = time;
    //     cs->animTargetOffset = Carousel_CenterOffsetForIndex(
    //         cs->animTargetIndex, CAROUSEL_CARD_WIDTH, CAROUSEL_CARD_SPACING, cs->containerWidth
    //     );
    //     cs->animDuration = CAROUSEL_AUTODRAG_DURATION;
    // }
    // // SNAP: drag released → decisively snap to nearest
    // else if (cs->isDragging)
    // {
    //     int nearest = Carousel_NearestIndex(
    //         cs->scrollOffset,
    //         CAROUSEL_CARD_WIDTH,
    //         CAROUSEL_CARD_SPACING,
    //         cs->containerWidth,
    //         cs->cardCount
    //     );
    //     cs->isAutoDragging = true; // reuse animation system
    //     cs->animTargetIndex = nearest;
    //     cs->animStartOffset = cs->scrollOffset;
    //     cs->animStartTime = time;
    //     cs->animTargetOffset = Carousel_CenterOffsetForIndex(
    //         nearest, CAROUSEL_CARD_WIDTH, CAROUSEL_CARD_SPACING, cs->containerWidth
    //     );
    //     cs->animDuration = CAROUSEL_SNAP_DURATION;
    // }

    // // Reset drag state
    // cs->isDragging = false;
    // cs->pressedCardAbsoluteIndex = -1;
    // cs->pressedCardRelativePos = 0;
    // cs->totalMovement = 0.0f;
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