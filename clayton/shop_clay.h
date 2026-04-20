// Helper: Draw a single stat row with label + bar
void DrawStatRow(ClayArena *arena, const char* label, float value /* 0.0 to 1.0 */, int nr) {
    CLAY(CLAY_IDI("StatRow", nr), CLAY_THEME_STAT_ROW) {
        char buf[64];
        int len = snprintf(buf, sizeof(buf), "%s", label);
        Clay_String lable = ClayArena_AllocString(arena, buf);
        CLAY_TEXT(lable, CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_STAT));
        CLAY(CLAY_IDI("StatBarBg", nr), CLAY_THEME_STAT_BAR_BG) {
            CLAY(CLAY_IDI("StatBarFill", nr), CLAY_THEME_STAT_BAR_FILL(value)) {}
        }
    }
}

// Draw a single catalog item card
void DrawCatalogItem(UserContext* usr, const char* name, const char* rarity, float price,
                     float mass, float spin, float skid, float bite,
                     bool canAfford, Clay_String imagePlaceholder, int nr) {
    
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

    CLAY(CLAY_IDI("CatalogItem", nr), CLAY_THEME_CATALOG_ITEM) {
        
        CLAY(
            CLAY_IDI("ItemHeader", nr), 
            {
                .layout = {
                    .sizing = { CLAY_SIZING_GROW(), CLAY_SIZING_GROW(), },
                    .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                }
            }
        ) {
            CLAY_TEXT(CLAY_STRING("BALL NAME"), CLAY_TEXT_CONFIG(bodyCfg));

            CLAY( CLAY_IDI("BalltitleSpacer", nr), {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1)}}}){};

            Clay_Color rarityColor = CLAY_COLOR_RARITY_COMMON;
            if (strcmp(rarity, "LEGENDARY") == 0) rarityColor = CLAY_COLOR_RARITY_LEGENDARY;
            if (strcmp(rarity, "EPIC") == 0) rarityColor = CLAY_COLOR_RARITY_EPIC;
            if (strcmp(rarity, "RARE") == 0) rarityColor = CLAY_COLOR_RARITY_RARE;
            CLAY(
                CLAY_IDI("RarityBadge", nr), 
                 { 
                    .layout=rarityBadgeLayoutCfg,
                    .backgroundColor=rarityColor
                 }
            ) {
                char rarityLableBuf[64];
                int len = snprintf(rarityLableBuf, sizeof(rarityLableBuf), "%s", rarity);
                Clay_String rarityLable = ClayArena_AllocString(arena, rarityLableBuf);
                CLAY_TEXT(rarityLable, CLAY_TEXT_CONFIG(rarityCfg));
            }
        }
        
        // Ball image preview area
        CLAY(CLAY_IDI("BallPreview", nr), CLAY_THEME_BALL_PREVIEW) {
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
        CLAY(CLAY_IDI("PriceRow", nr), CLAY_THEME_PRICE_ROW) {
            char buf[64];
            int len = snprintf(buf, sizeof(buf), "%.0f", price);
            Clay_String lable = ClayArena_AllocString(arena, buf);
            CLAY_TEXT(lable, CLAY_TEXT_CONFIG(priceCfg));
        } 
        // Stats section
        CLAY(CLAY_IDI("StatsSection", nr), 
            { .layout= {
                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()}, 
                .childGap = 4, 
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            }}
        ) {
            DrawStatRow(arena, "MASS", mass, nr + 10);
            DrawStatRow(arena, "SPIN", spin, nr + 20);
            DrawStatRow(arena, "SKID", skid, nr + 30);
            DrawStatRow(arena, "BITE", bite, nr + 40);
        }
        
        // // Buy button (disabled if can't afford)
        char buf[64];
        int len = snprintf(buf, sizeof(buf), "%s", "BUY NOW");
        Clay_String lable = ClayArena_AllocString(arena, buf);
        if (canAfford) {
            CLAY(CLAY_IDI("BuyButton", nr), CLAY_THEME_BTN_BUY) {
                CLAY_TEXT(lable, CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_BUTTON));
            }
        } else {
            Clay_TextElementConfig disabledCfg = { .textColor = CLAY_COLOR_TEXT_SECONDARY, .fontId = CLAY_FONT_NOTO, .fontSize = CLAY_FONT_SIZE_SM};
            CLAY( CLAY_IDI("BuyButtonDisabled", nr), CLAY_THEME_BTN_BUY_DISABLED) {
                CLAY_TEXT(lable, CLAY_TEXT_CONFIG(disabledCfg));
            }
        }
    }
}

// Main shop UI function
void RenderShopUI(float playerCoins, const char* resetCountdown, UserContext* usr) {
    
    Clay_TextElementConfig labelCfg = CLAY_THEME_TEXT_LABEL;
    Clay_TextElementConfig buttonCfg = CLAY_THEME_TEXT_BUTTON;
    Clay_TextElementConfig titleCfg = CLAY_THEME_TEXT_TITLE;
    Clay_TextElementConfig scoreCfg = CLAY_THEME_TEXT_LARGE;
    Clay_TextElementConfig priceCfg = CLAY_THEME_TEXT_PRICE;
    Clay_TextElementConfig countdownCfg = CLAY_THEME_TEXT_COUNTDOWN;

    ClayArena *arena = &usr->clayArena; // ← Embedded arena

    // Full-screen overlay background (optional)
    CLAY(CLAY_ID("ShopOverlayw"), CLAY_THEME_OVERLAY) {
        
        // Main shop container
        CLAY(CLAY_ID("ShopContainer2"), CLAY_THEME_SHOP_CONTAINER) {
            
            // Title bar
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
                CLAY( CLAY_ID("TitleDividerS"), {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1)}}}){};
                CLAY(usr->closeShopClick.clayId, CLAY_THEME_BTN_DANGER)
                {
                    CLAY_TEXT(CLAY_STRING("x"), CLAY_TEXT_CONFIG(buttonCfg));
                }
            }

            // Header: SHOP title + player currency
            CLAY(
                CLAY_ID("ShopHeaderd"),
                CLAY_THEME_SHOP_HEADER
            ) {
                CLAY_TEXT(CLAY_STRING("SHOPq"), CLAY_TEXT_CONFIG(titleCfg));
                char bankAmountBuf[64];
                int len = snprintf(bankAmountBuf, sizeof(bankAmountBuf), "$ %d", usr->bank);
                Clay_String bankAmount = ClayArena_AllocString(arena, bankAmountBuf);
                CLAY_TEXT(bankAmount, CLAY_TEXT_CONFIG(priceCfg));
            }
            
            // 2x2 Grid of items

            Clay_Vector2 cv = Clay_GetScrollOffset();
            // cv.x *=  0.5;
            //  cv.x *=  2.0;
            // cv.y *=  0.5;

            CLAY(CLAY_ID("ShopGrid"), 
            { 
                .layout = { 
                    .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()}, 
                    .padding = {8, 8, 8, 8}, 
                    .childGap = 10, 
                    .layoutDirection = CLAY_LEFT_TO_RIGHT, 
                }, 
                .clip = {.vertical = false, .horizontal = true , .childOffset = cv }, 
            }
        
        ) {
            CLAY(CLAY_ID("ShopGridInner"), CLAY_THEME_SHOP_GRID_INNER) {
                
                // Item 1
                CLAY(CLAY_ID("ShopCell1"), CLAY_THEME_SHOP_CELL) {
                    DrawCatalogItem(usr, "Strike Master", "RARE", 100.0f, 
                                   0.8f, 0.6f, 0.3f, 0.9f, 
                                   playerCoins >= 100.0f, CLAY_STRING("M"), 1);
                }
                
                // Item 2
                CLAY(CLAY_ID("ShopCell2"), CLAY_THEME_SHOP_CELL) {
                    DrawCatalogItem(usr, "Spin Doctor", "EPIC", 250.0f, 
                                   0.5f, 0.95f, 0.7f, 0.6f, 
                                   playerCoins >= 250.0f, CLAY_STRING("S"), 2);
                }
                
                // // Item 3
                CLAY(CLAY_ID("ShopCell3"), CLAY_THEME_SHOP_CELL) {
                    DrawCatalogItem(usr, "Pin Crusher", "COMMON", 50.0f, 
                                   0.95f, 0.3f, 0.8f, 0.4f, 
                                   playerCoins >= 50.0f, CLAY_STRING("K"), 3);
                }
                
                // // Item 4
                CLAY(CLAY_ID("ShopCell4"), CLAY_THEME_SHOP_CELL) {
                    DrawCatalogItem(usr, "Golden Strike", "LEGENDARY", 500.0f, 
                                   0.7f, 0.8f, 0.5f, 1.0f, 
                                   playerCoins >= 500.0f, CLAY_STRING("A"), 4);
                }
            }
            }
            
            // Footer: Reset countdown
            CLAY(CLAY_ID("ShopFooter"), CLAY_THEME_SHOP_FOOTER) {
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

    RenderShopUI(7.0f, "Cauntdaun", usr);

}