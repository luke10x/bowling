#pragma once

#define CAROUSEL_MAX_CARDS 6

#include <string.h>  // for memcpy, strcmp
#include <stdio.h>   // for debug logging (optional)

#define CAROUSEL_CARD_WIDTH 0.7f
#define CAROUSEL_CARD_SPACING 0.0f
#define CAROUSEL_SIDE_VISIBLE_FRAC 0.35f // How much of side cards to show

#define CAROUSEL_MOVEMENT_THRESHOLD 8.0f       // px: click vs drag
#define CAROUSEL_CLICK_DURATION_THRESHOLD 0.5f // seconds
#define CAROUSEL_SNAP_STRENGTH 8.0f            // magnetic pull during drag
#define CAROUSEL_AUTODRAG_DURATION 0.30f       // seconds for side-card click animation
#define CAROUSEL_SNAP_DURATION 0.25f           // seconds for drag-release snap
#define CAROUSEL_DRAG_SMOOTHING 0.85f          // <1.0 = smoothed input

// ============================================================================
// CATALOG ITEM: Minimal struct for carousel compatibility
// ============================================================================
typedef struct
{
    int id;
    char name[20];
    char rarity[60]; // "COMMON", "RARE", "EPIC", "LEGENDARY"
    char theme[20];

    float price; // from 200 to 2000 
    float mass, radius, spin, skid, bite; // stats 0.0–1.0
    // Add more fields as needed (id, unlock condition, etc.)
    float launchBuff, hitBuff;
} CatalogItem;

const CatalogItem g_ballCatalog[] = {
    // ═══════════════════════════════════════════════════════════════
    // 🔥 FIRE THEME
    // ═══════════════════════════════════════════════════════════════
    { .id =  0, .name = "Ember Strike", .rarity = "COMMON", .theme = "Fire",   .price = 25.0f, .mass = 0.45f, .radius = 0.50f, .spin = 0.48f, .skid = 0.52f, .bite = 0.46f, .launchBuff = 0.50f, .hitBuff = 0.48f },
    { .id =  1, .name = "Flame Roll",   .rarity = "COMMON", .theme = "Fire",   .price = 28.0f, .mass = 0.47f, .radius = 0.52f, .spin = 0.50f, .skid = 0.54f, .bite = 0.48f, .launchBuff = 0.52f, .hitBuff = 0.50f },
    { .id =  2, .name = "Blaze Hook",   .rarity = "RARE",   .theme = "Fire",   .price = 65.0f, .mass = 0.60f, .radius = 0.62f, .spin = 0.65f, .skid = 0.58f, .bite = 0.63f, .launchBuff = 0.64f, .hitBuff = 0.62f },
    { .id =  3, .name = "Inferno Fury", .rarity = "EPIC",   .theme = "Fire",   .price = 150.0f,.mass = 0.78f, .radius = 0.80f, .spin = 0.82f, .skid = 0.75f, .bite = 0.81f, .launchBuff = 0.83f, .hitBuff = 0.80f },
    { .id =  4, .name = "Phoenix Rise", .rarity = "LEGEND", .theme = "Fire",   .price = 200.0f,.mass = 0.95f, .radius = 0.98f, .spin = 0.96f, .skid = 0.92f, .bite = 0.97f, .launchBuff = 0.98f, .hitBuff = 0.96f },

    // ═══════════════════════════════════════════════════════════════
    // ❄️ ICE THEME
    // ═══════════════════════════════════════════════════════════════
    { .id =  5, .name = "Frost Glide",  .rarity = "COMMON", .theme = "Ice",    .price = 24.0f, .mass = 0.42f, .radius = 0.48f, .spin = 0.44f, .skid = 0.58f, .bite = 0.40f, .launchBuff = 0.46f, .hitBuff = 0.42f },
    { .id =  6, .name = "Chill Strike", .rarity = "COMMON", .theme = "Ice",    .price = 27.0f, .mass = 0.44f, .radius = 0.50f, .spin = 0.46f, .skid = 0.60f, .bite = 0.42f, .launchBuff = 0.48f, .hitBuff = 0.44f },
    { .id =  7, .name = "Arctic Drift", .rarity = "RARE",   .theme = "Ice",    .price = 62.0f, .mass = 0.58f, .radius = 0.60f, .spin = 0.62f, .skid = 0.72f, .bite = 0.60f, .launchBuff = 0.62f, .hitBuff = 0.60f },
    { .id =  8, .name = "Glacier Bite", .rarity = "EPIC",   .theme = "Ice",    .price = 145.0f,.mass = 0.76f, .radius = 0.78f, .spin = 0.80f, .skid = 0.85f, .bite = 0.79f, .launchBuff = 0.81f, .hitBuff = 0.78f },
    { .id =  9, .name = "Blizzard King",.rarity = "LEGEND", .theme = "Ice",    .price = 195.0f,.mass = 0.93f, .radius = 0.96f, .spin = 0.94f, .skid = 0.98f, .bite = 0.95f, .launchBuff = 0.96f, .hitBuff = 0.94f },

    // ═══════════════════════════════════════════════════════════════
    // 🌌 GALAXY THEME
    // ═══════════════════════════════════════════════════════════════
    { .id = 10, .name = "Star Dust",    .rarity = "COMMON", .theme = "Galaxy", .price = 26.0f, .mass = 0.46f, .radius = 0.51f, .spin = 0.49f, .skid = 0.53f, .bite = 0.47f, .launchBuff = 0.51f, .hitBuff = 0.49f },
    { .id = 11, .name = "Nebula Roll",  .rarity = "COMMON", .theme = "Galaxy", .price = 29.0f, .mass = 0.48f, .radius = 0.53f, .spin = 0.51f, .skid = 0.55f, .bite = 0.49f, .launchBuff = 0.53f, .hitBuff = 0.51f },
    { .id = 12, .name = "Cosmic Hook",  .rarity = "RARE",   .theme = "Galaxy", .price = 70.0f, .mass = 0.62f, .radius = 0.64f, .spin = 0.68f, .skid = 0.60f, .bite = 0.66f, .launchBuff = 0.66f, .hitBuff = 0.64f },
    { .id = 13, .name = "Void Strike",  .rarity = "EPIC",   .theme = "Galaxy", .price = 160.0f,.mass = 0.80f, .radius = 0.82f, .spin = 0.84f, .skid = 0.77f, .bite = 0.83f, .launchBuff = 0.85f, .hitBuff = 0.82f },
    { .id = 14, .name = "Black Hole",   .rarity = "LEGEND", .theme = "Galaxy", .price = 200.0f,.mass = 0.98f, .radius = 1.00f, .spin = 0.99f, .skid = 0.94f, .bite = 1.00f, .launchBuff = 1.00f, .hitBuff = 0.98f },
    { .id = 15, .name = "UFO Spin",     .rarity = "RARE",   .theme = "Galaxy", .price = 68.0f, .mass = 0.61f, .radius = 0.63f, .spin = 0.70f, .skid = 0.59f, .bite = 0.65f, .launchBuff = 0.65f, .hitBuff = 0.63f },
    { .id = 16, .name = "Xenon Bite",   .rarity = "EPIC",   .theme = "Galaxy", .price = 155.0f,.mass = 0.79f, .radius = 0.81f, .spin = 0.83f, .skid = 0.76f, .bite = 0.82f, .launchBuff = 0.84f, .hitBuff = 0.81f },

    // ═══════════════════════════════════════════════════════════════
    // 🌿 NATURE THEME
    // ═══════════════════════════════════════════════════════════════
    { .id = 17, .name = "Forest Roll",  .rarity = "COMMON", .theme = "Nature", .price = 25.0f, .mass = 0.44f, .radius = 0.49f, .spin = 0.47f, .skid = 0.51f, .bite = 0.45f, .launchBuff = 0.49f, .hitBuff = 0.47f },
    { .id = 18, .name = "Vine Strike",  .rarity = "COMMON", .theme = "Nature", .price = 27.0f, .mass = 0.46f, .radius = 0.51f, .spin = 0.49f, .skid = 0.53f, .bite = 0.47f, .launchBuff = 0.51f, .hitBuff = 0.49f },
    { .id = 19, .name = "Thunder Oak",  .rarity = "RARE",   .theme = "Nature", .price = 64.0f, .mass = 0.59f, .radius = 0.61f, .spin = 0.64f, .skid = 0.57f, .bite = 0.62f, .launchBuff = 0.63f, .hitBuff = 0.61f },
    { .id = 20, .name = "Titan Root",   .rarity = "EPIC",   .theme = "Nature", .price = 148.0f,.mass = 0.77f, .radius = 0.79f, .spin = 0.81f, .skid = 0.74f, .bite = 0.80f, .launchBuff = 0.82f, .hitBuff = 0.79f },
    { .id = 21, .name = "World Tree",   .rarity = "LEGEND", .theme = "Nature", .price = 198.0f,.mass = 0.94f, .radius = 0.97f, .spin = 0.95f, .skid = 0.91f, .bite = 0.96f, .launchBuff = 0.97f, .hitBuff = 0.95f },

    // ═══════════════════════════════════════════════════════════════
    // 🌀 VOID THEME
    // ═══════════════════════════════════════════════════════════════
    { .id = 22, .name = "Shadow Drift", .rarity = "RARE",   .theme = "Void",   .price = 72.0f, .mass = 0.57f, .radius = 0.59f, .spin = 0.66f, .skid = 0.54f, .bite = 0.64f, .launchBuff = 0.60f, .hitBuff = 0.68f },
    { .id = 23, .name = "Abyss Hook",   .rarity = "EPIC",   .theme = "Void",   .price = 165.0f,.mass = 0.81f, .radius = 0.83f, .spin = 0.86f, .skid = 0.72f, .bite = 0.85f, .launchBuff = 0.78f, .hitBuff = 0.88f },
    { .id = 24, .name = "Nullifier",    .rarity = "LEGEND", .theme = "Void",   .price = 200.0f,.mass = 0.97f, .radius = 0.99f, .spin = 1.00f, .skid = 0.88f, .bite = 0.99f, .launchBuff = 0.92f, .hitBuff = 1.00f },

    // ═══════════════════════════════════════════════════════════════
    // ⚙️ TECH THEME
    // ═══════════════════════════════════════════════════════════════
    { .id = 25, .name = "Circuit Roll", .rarity = "COMMON", .theme = "Tech",   .price = 26.0f, .mass = 0.45f, .radius = 0.50f, .spin = 0.48f, .skid = 0.52f, .bite = 0.46f, .launchBuff = 0.50f, .hitBuff = 0.48f },
    { .id = 26, .name = "Neon Strike",  .rarity = "RARE",   .theme = "Tech",   .price = 67.0f, .mass = 0.60f, .radius = 0.62f, .spin = 0.67f, .skid = 0.56f, .bite = 0.64f, .launchBuff = 0.64f, .hitBuff = 0.62f },
    { .id = 27, .name = "Quantum Hook", .rarity = "EPIC",   .theme = "Tech",   .price = 152.0f,.mass = 0.78f, .radius = 0.80f, .spin = 0.83f, .skid = 0.75f, .bite = 0.81f, .launchBuff = 0.83f, .hitBuff = 0.80f },
    { .id = 28, .name = "Singularity",  .rarity = "LEGEND", .theme = "Tech",   .price = 200.0f,.mass = 0.96f, .radius = 0.99f, .spin = 0.98f, .skid = 0.93f, .bite = 0.98f, .launchBuff = 0.99f, .hitBuff = 0.97f },

    // ═══════════════════════════════════════════════════════════════
    // 🌊 OCEAN THEME
    // ═══════════════════════════════════════════════════════════════
    { .id = 29, .name = "Tide Roll",    .rarity = "COMMON", .theme = "Ocean",  .price = 24.0f, .mass = 0.44f, .radius = 0.49f, .spin = 0.47f, .skid = 0.51f, .bite = 0.45f, .launchBuff = 0.49f, .hitBuff = 0.47f },
    { .id = 30, .name = "Reef Strike",  .rarity = "RARE",   .theme = "Ocean",  .price = 63.0f, .mass = 0.58f, .radius = 0.60f, .spin = 0.63f, .skid = 0.58f, .bite = 0.61f, .launchBuff = 0.62f, .hitBuff = 0.60f },
    { .id = 31, .name = "Kraken Bite",  .rarity = "EPIC",   .theme = "Ocean",  .price = 149.0f,.mass = 0.76f, .radius = 0.78f, .spin = 0.80f, .skid = 0.74f, .bite = 0.79f, .launchBuff = 0.81f, .hitBuff = 0.78f },
    { .id = 32, .name = "Leviathan",    .rarity = "LEGEND", .theme = "Ocean",  .price = 199.0f,.mass = 0.94f, .radius = 0.97f, .spin = 0.95f, .skid = 0.90f, .bite = 0.96f, .launchBuff = 0.97f, .hitBuff = 0.95f },
    
    // ═══════════════════════════════════════════════════════════════
    // ✨ MYSTIC THEME
    // ═══════════════════════════════════════════════════════════════
    { .id = 33, .name = "Rune Ball",    .rarity = "EPIC",   .theme = "Mystic", .price = 170.0f,.mass = 0.82f, .radius = 0.84f, .spin = 0.79f, .skid = 0.73f, .bite = 0.84f, .launchBuff = 0.76f, .hitBuff = 0.86f },
    { .id = 34, .name = "Oracle Strike",.rarity = "LEGEND", .theme = "Mystic", .price = 200.0f,.mass = 0.95f, .radius = 0.98f, .spin = 0.93f, .skid = 0.89f, .bite = 0.97f, .launchBuff = 0.90f, .hitBuff = 0.99f }
};

const size_t g_ballCatalogCount = sizeof(g_ballCatalog) / sizeof(g_ballCatalog[0]);
typedef struct
{
    // my new imple
    bool isGrabbed;
    int startingX;
    // Motion state
    float scrollOffset;
    float velocity;
    bool isDragging;
    bool isAutoDragging;

    int closestBallIdx;
    int closest2ndBallIdx;
    CatalogItem items[CAROUSEL_MAX_CARDS];
    int cardCount;
    int bank;
} CarouselState;

void Carousel_Init(CarouselState *cs)
{
    memset(cs, 0, sizeof(CarouselState));
    cs->velocity = 0.0f;
    cs->isGrabbed = false;
    cs->startingX = 0.0f;
}


static inline float
Carousel_CenterOffsetForIndex(int idx, float cardW, float spacing, float containerW)
{
    float pitch = cardW + spacing;
    float cardCenterWorld = idx * pitch + cardW * 0.5f;
    return -(cardCenterWorld - containerW * 0.5f);
}

// Internal helper: updates closestBallIdx and closest2ndBallId as carousel array indices [0, cardCount)
static void Carousel_UpdateClosestIndices_bak(CarouselState *cs, float slotWidth)
{
    if (cs->cardCount <= 0 || slotWidth <= 0.0f) {
        cs->closestBallIdx = -1;
        cs->closest2ndBallIdx = -1;
        return;
    }

    float exact = cs->scrollOffset / slotWidth;
    
    // === Closest: round to nearest slot ===
    int nearestSlot = (int)glm::round(exact);
    nearestSlot = glm::clamp(nearestSlot, 1 - cs->cardCount, 0);
    int closestIdx = -nearestSlot;  // slot -i → carousel index i
    cs->closestBallIdx = closestIdx;
    
    // === 2nd Closest: adjacent slot on the side of fractional offset ===
    float frac = exact - (float)nearestSlot;  // [-0.5, 0.5]
    int secondSlot;
    
    if (frac > 0.0f) {
        // Offset toward more negative slots → next higher carousel index
        secondSlot = nearestSlot - 1;
    } else if (frac < 0.0f) {
        // Offset toward less negative slots → next lower carousel index  
        secondSlot = nearestSlot + 1;
    } else {
        // Exactly centered: prefer next higher index (adjust preference if needed)
        secondSlot = nearestSlot - 1;
    }
    
    secondSlot = glm::clamp(secondSlot, 1 - cs->cardCount, 0);
    int secondIdx = -secondSlot;
    
    // Ensure 2nd is valid and distinct from closest when possible
    if (secondIdx == closestIdx && cs->cardCount > 1) {
        if (closestIdx + 1 < cs->cardCount) {
            secondIdx = closestIdx + 1;
        } else if (closestIdx - 1 >= 0) {
            secondIdx = closestIdx - 1;
        }
    }
    
    cs->closest2ndBallIdx = (secondIdx != closestIdx) ? secondIdx : -1;
}
static void Carousel_UpdateClosestIndices(CarouselState *cs, float slotWidth)
{
    if (cs->cardCount <= 0 || slotWidth <= 0.0f) {
        cs->closestBallIdx = -1;
        cs->closest2ndBallIdx = -1;
        return;
    }

    float exact = cs->scrollOffset / slotWidth;  // e.g., -2.3 means "between slot -2 and -3"
    
    // === Closest: round to nearest slot → carousel index ===
    int nearestSlot = (int)glm::round(exact);
    nearestSlot = glm::clamp(nearestSlot, 1 - cs->cardCount, 0);
    int closestIdx = -nearestSlot;  // slot -i → carousel index i
    cs->closestBallIdx = closestIdx;
    
    // === Second closest: pick the adjacent index (±1) with smaller geometric distance ===
    cs->closest2ndBallIdx = -1;
    if (cs->cardCount > 1) {
        int candidates[2] = { closestIdx - 1, closestIdx + 1 };
        float bestDist = FLT_MAX;
        
        for (int cand : candidates) {
            // Skip out-of-bounds
            if (cand < 0 || cand >= cs->cardCount) continue;
            
            // Distance in "slot units" between exact scroll position and this candidate's slot
            float slotForCand = -(float)cand;  // carousel index → slot number
            float dist = glm::abs(exact - slotForCand);
            
            if (dist < bestDist) {
                bestDist = dist;
                cs->closest2ndBallIdx = cand;
            }
        }
    }
}


#include <string.h>  // for memcpy

// ─────────────────────────────────────────────────────────────
// a) Copy one ball from global catalog into CarouselState by name
// Returns true if added, false if not found or carousel full
// ─────────────────────────────────────────────────────────────
bool Carousel_AddBall(CarouselState* cs, const char* ballName) {
    if (!cs || cs->cardCount >= CAROUSEL_MAX_CARDS) return false;
    for (size_t i = 0; i < g_ballCatalogCount; ++i) {
        if (strcmp(g_ballCatalog[i].name, ballName) == 0) {
            // ✅ memcpy works even with const array members
            memcpy(&cs->items[cs->cardCount], &g_ballCatalog[i], sizeof(CatalogItem));
            cs->cardCount++;
            return true;
        }
    }
    return false;
}

// ─────────────────────────────────────────────────────────────
// b) Populate carousel with a curated list of 6 balls by name
// ─────────────────────────────────────────────────────────────
void Carousel_SetupDefaultShop(CarouselState* cs) {
    if (!cs) return;

    cs->cardCount = 0;
    cs->scrollOffset = 0.0f;
    cs->velocity = 0.0f;
    cs->isDragging = false;
    cs->isAutoDragging = false;
    cs->isGrabbed = false;
    cs->startingX = 0;

    const char* defaultBalls[] = {
        "Star Dust",
        "Arctic Drift",
        "Cosmic Hook",
        "Titan Root",
        "Quantum Hook",
        "Phoenix Rise"
    };


    for (int i = 0; i < 6; ++i) {
        Carousel_AddBall(cs, defaultBalls[i]);
    }
}