#pragma once

#include "ads_config.h"

#define CAROUSEL_MAX_CARDS 40
#define BALL_SHOP_STOCK_SIZE 5
#define BALL_SHOP_DEFAULT_REFRESH_MINUTES 30
#define BALL_SHOP_REWARDED_CARD_ID -1
#define BALL_SHOP_REWARDED_COINS ADS_MONEY_REWARD

#include <string.h>  // for memcpy, strcmp
#include <stdio.h>   // for debug logging (optional)
#include <stdint.h>
#include <float.h>
#include <math.h>
#include <glm/glm.hpp>

#define CAROUSEL_CARD_WIDTH 0.7f
#define CAROUSEL_CARD_SPACING 0.0f
#define CAROUSEL_SIDE_VISIBLE_FRAC 0.35f // How much of side cards to show

#define CAROUSEL_MOVEMENT_THRESHOLD 8.0f       // px: click vs drag
#define CAROUSEL_CLICK_DURATION_THRESHOLD 0.5f // seconds
#define CAROUSEL_SNAP_STRENGTH 8.0f            // magnetic pull during drag
#define CAROUSEL_AUTODRAG_DURATION 0.30f       // seconds for side-card click animation
#define CAROUSEL_SNAP_DURATION 0.25f           // seconds for drag-release snap
#define CAROUSEL_DRAG_SMOOTHING 0.85f          // <1.0 = smoothed input

// Stable ball catalog IDs. Keep these readable wherever a ball is selected.
static constexpr int kBallEmberStrike = 0;
static constexpr int kBallFlameRoll = 1;
static constexpr int kBallBlazeHook = 2;
static constexpr int kBallInfernoFury = 3;
static constexpr int kBallPhoenixRise = 4;
static constexpr int kBallFrostGlide = 5;
static constexpr int kBallChillStrike = 6;
static constexpr int kBallArcticDrift = 7;
static constexpr int kBallGlacierBite = 8;
static constexpr int kBallBlizzardKing = 9;
static constexpr int kBallCosmicHook = 12;
static constexpr int kBallVoidStrike = 13;
static constexpr int kBallBlackHole = 14;
static constexpr int kBallXenonBite = 16;
static constexpr int kBallStarDust = 10;
static constexpr int kBallNebulaRoll = 11;
static constexpr int kBallUfoSpin = 15;
static constexpr int kBallNeonStrike = 26;
static constexpr int kBallWorldTree = 21;
static constexpr int kBallShadowDrift = 22;
static constexpr int kBallForestRoll = 17;
static constexpr int kBallVineStrike = 18;
static constexpr int kBallThunderOak = 19;
static constexpr int kBallTitanRoot = 20;
static constexpr int kBallNullifier = 24;
static constexpr int kBallAbyssHook = 23;
static constexpr int kBallCircuitRoll = 25;
static constexpr int kBallQuantumHook = 27;
static constexpr int kBallSingularity = 28;
static constexpr int kBallTideRoll = 29;
static constexpr int kBallLeviathan = 32;
static constexpr int kBallReefStrike = 30;
static constexpr int kBallKrakenBite = 31;
static constexpr int kBallRuneBall = 33;
static constexpr int kBallOracleStrike = 34;

// ============================================================================
// CATALOG ITEM: Minimal struct for carousel compatibility
// ============================================================================
typedef struct CatalogItem
{
    int id;
    char name[20];
    char rarity[60]; // "COMMON", "RARE", "EPIC", "LEGENDARY"
    char theme[20];

	float price; // Ember Strike is the starter exception; everything else follows rarity bands.
	float mass, radius, spin, skid, bite; // stats 0.0–1.0
	// Catalog restitution (bounciness) in 0..1, mapped to Jolt in game.cpp.
	// Default matches current physics init (ballBody.mRestitution = 0.02).
	float restitution;
	// Add more fields as needed (id, unlock condition, etc.)
	float launchBuff, hitBuff;
} CatalogItem;

const CatalogItem g_ballCatalog[] = {
    // ═══════════════════════════════════════════════════════════════
    // 🔥 FIRE THEME
    // ═══════════════════════════════════════════════════════════════
    { .id = kBallEmberStrike, .name = "Ember Strike", .rarity = "COMMON", .theme = "Fire",   .price = 17.0f,  .mass = 0.36f, .radius = 0.49f, .spin = 0.64f, .skid = 0.50f, .bite = 0.55f, .restitution = 0.02f, .launchBuff = 0.25f, .hitBuff = 0.48f },
    { .id = kBallFlameRoll,   .name = "Flame Roll",   .rarity = "COMMON", .theme = "Fire",   .price = 53.0f,  .mass = 0.55f, .radius = 0.52f, .spin = 0.68f, .skid = 0.47f, .bite = 0.50f, .restitution = 0.02f, .launchBuff = 0.26f, .hitBuff = 0.50f },
    { .id = kBallBlazeHook,   .name = "Blaze Hook",   .rarity = "RARE",   .theme = "Fire",   .price = 429.0f, .mass = 0.50f, .radius = 0.60f, .spin = 0.72f, .skid = 0.55f, .bite = 0.58f, .restitution = 0.02f, .launchBuff = 0.31f, .hitBuff = 0.60f },
    { .id = kBallInfernoFury, .name = "Inferno Fury", .rarity = "EPIC",   .theme = "Fire",   .price = 1246.0f,.mass = 0.74f, .radius = 0.77f, .spin = 0.69f, .skid = 0.63f, .bite = 0.76f, .restitution = 0.02f, .launchBuff = 0.41f, .hitBuff = 0.82f },
    { .id = kBallPhoenixRise, .name = "Phoenix Rise", .rarity = "LEGEND", .theme = "Fire",   .price = 2814.0f,.mass = 0.58f, .radius = 0.86f, .spin = 0.88f, .skid = 0.79f, .bite = 0.72f, .restitution = 0.02f, .launchBuff = 0.47f, .hitBuff = 0.90f },

    // ═══════════════════════════════════════════════════════════════
    // ❄️ ICE THEME
    // ═══════════════════════════════════════════════════════════════
    { .id = kBallFrostGlide,   .name = "Frost Glide",  .rarity = "COMMON", .theme = "Ice",    .price = 39.0f,  .mass = 0.38f, .radius = 0.47f, .spin = 0.62f, .skid = 0.67f, .bite = 0.34f, .restitution = 0.02f, .launchBuff = 0.25f, .hitBuff = 0.41f },
    { .id = kBallChillStrike,  .name = "Chill Strike", .rarity = "COMMON", .theme = "Ice",    .price = 63.0f,  .mass = 0.49f, .radius = 0.50f, .spin = 0.64f, .skid = 0.62f, .bite = 0.39f, .restitution = 0.02f, .launchBuff = 0.27f, .hitBuff = 0.45f },
    { .id = kBallArcticDrift,  .name = "Arctic Drift", .rarity = "RARE",   .theme = "Ice",    .price = 544.0f, .mass = 0.44f, .radius = 0.58f, .spin = 0.65f, .skid = 0.82f, .bite = 0.43f, .restitution = 0.02f, .launchBuff = 0.33f, .hitBuff = 0.57f },
    { .id = kBallGlacierBite,  .name = "Glacier Bite", .rarity = "EPIC",   .theme = "Ice",    .price = 1434.0f,.mass = 0.79f, .radius = 0.76f, .spin = 0.61f, .skid = 0.88f, .bite = 0.74f, .restitution = 0.02f, .launchBuff = 0.40f, .hitBuff = 0.80f },
    { .id = kBallBlizzardKing, .name = "Blizzard King",.rarity = "LEGEND", .theme = "Ice",    .price = 2935.0f,.mass = 0.67f, .radius = 0.92f, .spin = 0.86f, .skid = 0.97f, .bite = 0.69f, .restitution = 0.02f, .launchBuff = 0.48f, .hitBuff = 0.92f },

    // ═══════════════════════════════════════════════════════════════
    // 🌌 GALAXY THEME
    // ═══════════════════════════════════════════════════════════════
    { .id = kBallStarDust,   .name = "Star Dust",    .rarity = "COMMON", .theme = "Galaxy", .price = 78.0f,  .mass = 0.43f, .radius = 0.50f, .spin = 0.62f, .skid = 0.55f, .bite = 0.41f, .restitution = 0.02f, .launchBuff = 0.26f, .hitBuff = 0.44f },
    { .id = kBallNebulaRoll, .name = "Nebula Roll",  .rarity = "COMMON", .theme = "Galaxy", .price = 98.0f,  .mass = 0.60f, .radius = 0.54f, .spin = 0.61f, .skid = 0.49f, .bite = 0.52f, .restitution = 0.02f, .launchBuff = 0.26f, .hitBuff = 0.52f },
    { .id = kBallCosmicHook, .name = "Cosmic Hook",  .rarity = "RARE",   .theme = "Galaxy", .price = 624.0f, .mass = 0.47f, .radius = 0.63f, .spin = 0.78f, .skid = 0.57f, .bite = 0.55f, .restitution = 0.02f, .launchBuff = 0.32f, .hitBuff = 0.61f },
    { .id = kBallVoidStrike, .name = "Void Strike",  .rarity = "EPIC",   .theme = "Galaxy", .price = 1561.0f,.mass = 0.71f, .radius = 0.80f, .spin = 0.74f, .skid = 0.70f, .bite = 0.72f, .restitution = 0.02f, .launchBuff = 0.42f, .hitBuff = 0.84f },
    { .id = kBallBlackHole,  .name = "Black Hole",   .rarity = "LEGEND", .theme = "Galaxy", .price = 5052.0f,.mass = 0.92f, .radius = 0.97f, .spin = 0.82f, .skid = 0.83f, .bite = 0.91f, .restitution = 0.02f, .launchBuff = 0.50f, .hitBuff = 0.98f },
    { .id = kBallUfoSpin,    .name = "UFO Spin",     .rarity = "RARE",   .theme = "Galaxy", .price = 728.0f, .mass = 0.40f, .radius = 0.59f, .spin = 0.84f, .skid = 0.63f, .bite = 0.47f, .restitution = 0.02f, .launchBuff = 0.34f, .hitBuff = 0.58f },
    { .id = kBallXenonBite,  .name = "Xenon Bite",   .rarity = "EPIC",   .theme = "Galaxy", .price = 1481.0f,.mass = 0.63f, .radius = 0.77f, .spin = 0.72f, .skid = 0.73f, .bite = 0.81f, .restitution = 0.02f, .launchBuff = 0.41f, .hitBuff = 0.83f },

    // ═══════════════════════════════════════════════════════════════
    // 🌿 NATURE THEME
    // ═══════════════════════════════════════════════════════════════
    { .id = kBallForestRoll, .name = "Forest Roll",  .rarity = "COMMON", .theme = "Nature", .price = 45.0f,  .mass = 0.52f, .radius = 0.49f, .spin = 0.65f, .skid = 0.46f, .bite = 0.55f, .restitution = 0.02f, .launchBuff = 0.24f, .hitBuff = 0.48f },
    { .id = kBallVineStrike, .name = "Vine Strike",  .rarity = "COMMON", .theme = "Nature", .price = 69.0f,  .mass = 0.40f, .radius = 0.50f, .spin = 0.65f, .skid = 0.58f, .bite = 0.38f, .restitution = 0.02f, .launchBuff = 0.28f, .hitBuff = 0.44f },
    { .id = kBallThunderOak, .name = "Thunder Oak",  .rarity = "RARE",   .theme = "Nature", .price = 498.0f, .mass = 0.82f, .radius = 0.63f, .spin = 0.60f, .skid = 0.42f, .bite = 0.79f, .restitution = 0.02f, .launchBuff = 0.28f, .hitBuff = 0.68f },
    { .id = kBallTitanRoot,  .name = "Titan Root",   .rarity = "EPIC",   .theme = "Nature", .price = 1374.0f,.mass = 0.91f, .radius = 0.81f, .spin = 0.69f, .skid = 0.51f, .bite = 0.88f, .restitution = 0.02f, .launchBuff = 0.37f, .hitBuff = 0.86f },
    { .id = kBallWorldTree,  .name = "World Tree",   .rarity = "LEGEND", .theme = "Nature", .price = 2697.0f,.mass = 0.84f, .radius = 0.95f, .spin = 0.77f, .skid = 0.69f, .bite = 0.95f, .restitution = 0.02f, .launchBuff = 0.46f, .hitBuff = 0.94f },

    // ═══════════════════════════════════════════════════════════════
    // 🌀 VOID THEME
    // ═══════════════════════════════════════════════════════════════
    { .id = kBallShadowDrift, .name = "Shadow Drift", .rarity = "RARE",   .theme = "Void",   .price = 848.0f, .mass = 0.46f, .radius = 0.57f, .spin = 0.80f, .skid = 0.61f, .bite = 0.50f, .restitution = 0.02f, .launchBuff = 0.35f, .hitBuff = 0.63f },
    { .id = kBallAbyssHook,   .name = "Abyss Hook",   .rarity = "EPIC",   .theme = "Void",   .price = 1869.0f,.mass = 0.69f, .radius = 0.82f, .spin = 0.90f, .skid = 0.68f, .bite = 0.77f, .restitution = 0.02f, .launchBuff = 0.43f, .hitBuff = 0.90f },
    { .id = kBallNullifier,   .name = "Nullifier",    .rarity = "LEGEND", .theme = "Void",   .price = 4683.0f,.mass = 0.88f, .radius = 0.98f, .spin = 0.87f, .skid = 0.76f, .bite = 0.94f, .restitution = 0.02f, .launchBuff = 0.50f, .hitBuff = 1.00f },

    // ═══════════════════════════════════════════════════════════════
    // ⚙️ TECH THEME
    // ═══════════════════════════════════════════════════════════════
    { .id = kBallCircuitRoll, .name = "Circuit Roll", .rarity = "COMMON", .theme = "Tech",   .price = 59.0f,  .mass = 0.57f, .radius = 0.50f, .spin = 0.66f, .skid = 0.48f, .bite = 0.51f, .restitution = 0.02f, .launchBuff = 0.25f, .hitBuff = 0.49f },
    { .id = kBallNeonStrike,  .name = "Neon Strike",  .rarity = "RARE",   .theme = "Tech",   .price = 596.0f, .mass = 0.43f, .radius = 0.61f, .spin = 0.81f, .skid = 0.60f, .bite = 0.49f, .restitution = 0.02f, .launchBuff = 0.34f, .hitBuff = 0.59f },
    { .id = kBallQuantumHook, .name = "Quantum Hook", .rarity = "EPIC",   .theme = "Tech",   .price = 1327.0f,.mass = 0.68f, .radius = 0.79f, .spin = 0.76f, .skid = 0.67f, .bite = 0.73f, .restitution = 0.02f, .launchBuff = 0.40f, .hitBuff = 0.81f },
    { .id = kBallSingularity, .name = "Singularity",  .rarity = "LEGEND", .theme = "Tech",   .price = 3427.0f,.mass = 0.74f, .radius = 0.96f, .spin = 0.93f, .skid = 0.85f, .bite = 0.86f, .restitution = 0.02f, .launchBuff = 0.49f, .hitBuff = 0.96f },

    // ═══════════════════════════════════════════════════════════════
    // 🌊 OCEAN THEME
    // ═══════════════════════════════════════════════════════════════
    { .id = kBallTideRoll,   .name = "Tide Roll",    .rarity = "COMMON", .theme = "Ocean",  .price = 36.0f,  .mass = 0.47f, .radius = 0.49f, .spin = 0.60f, .skid = 0.59f, .bite = 0.40f, .restitution = 0.02f, .launchBuff = 0.25f, .hitBuff = 0.43f },
    { .id = kBallReefStrike, .name = "Reef Strike",  .rarity = "RARE",   .theme = "Ocean",  .price = 470.0f, .mass = 0.73f, .radius = 0.61f, .spin = 0.65f, .skid = 0.66f, .bite = 0.67f, .restitution = 0.02f, .launchBuff = 0.29f, .hitBuff = 0.63f },
    { .id = kBallKrakenBite, .name = "Kraken Bite",  .rarity = "EPIC",   .theme = "Ocean",  .price = 1347.0f,.mass = 0.86f, .radius = 0.78f, .spin = 0.74f, .skid = 0.72f, .bite = 0.82f, .restitution = 0.02f, .launchBuff = 0.38f, .hitBuff = 0.82f },
    { .id = kBallLeviathan,  .name = "Leviathan",    .rarity = "LEGEND", .theme = "Ocean",  .price = 2606.0f,.mass = 0.98f, .radius = 0.97f, .spin = 0.68f, .skid = 0.78f, .bite = 0.97f, .restitution = 0.02f, .launchBuff = 0.46f, .hitBuff = 0.95f },
    
    // ═══════════════════════════════════════════════════════════════
    // ✨ MYSTIC THEME
    // ═══════════════════════════════════════════════════════════════
    { .id = kBallRuneBall,     .name = "Rune Ball",    .rarity = "EPIC",   .theme = "Mystic", .price = 1658.0f,.mass = 0.56f, .radius = 0.83f, .spin = 0.85f, .skid = 0.71f, .bite = 0.63f, .restitution = 0.02f, .launchBuff = 0.44f, .hitBuff = 0.85f },
    { .id = kBallOracleStrike, .name = "Oracle Strike",.rarity = "LEGEND", .theme = "Mystic", .price = 4509.0f,.mass = 0.72f, .radius = 0.97f, .spin = 0.90f, .skid = 0.87f, .bite = 0.89f, .restitution = 0.02f, .launchBuff = 0.50f, .hitBuff = 0.99f }
};

const size_t g_ballCatalogCount = sizeof(g_ballCatalog) / sizeof(g_ballCatalog[0]);

enum BallShopTab
{
    BallShopTab_INVENTORY = 0,
    BallShopTab_SHOP = 1,
};

typedef struct BallShopState
{
    BallShopTab activeTab;
    CatalogItem stock[BALL_SHOP_STOCK_SIZE];
    int stockCount;
    uint64_t stockBucketId;
    uint64_t carouselOwnedMask;
    uint64_t carouselBucketId;
    int refreshMinutes;
    int secondsUntilRefresh;
    bool stockInitialized;
    bool carouselValid;
    BallShopTab carouselTab;
    float transitionTime;
    float transitionDuration;
    float transitionDir;
    bool restockPromptPending;
    int pendingFocusIndex;
    bool carouselRewardAvailable;
    bool rewardCardConsumedThisOpen;
    int removingCardId;
    int removingCardIndex;
    int removingPayout;
    float removingCardElapsed;
    bool removingCardActive;
    bool removingPayoutStarted;
    float rewardCardPulse;
} BallShopState;

static inline void BallShop_Init(BallShopState *state)
{
    if (!state)
        return;
    memset(state, 0, sizeof(BallShopState));
    state->activeTab = BallShopTab_SHOP;
    state->refreshMinutes = BALL_SHOP_DEFAULT_REFRESH_MINUTES;
    state->transitionDuration = 0.18f;
    state->restockPromptPending = false;
    state->pendingFocusIndex = -1;
    state->removingCardId = -2;
    state->removingCardIndex = -1;
}

static inline void BallShop_BeginTransition(BallShopState *state, float dir)
{
    if (!state)
        return;
    state->transitionDuration = 0.18f;
    state->transitionTime = state->transitionDuration;
    state->transitionDir = (dir < 0.0f) ? -1.0f : 1.0f;
}

static inline void BallShop_TickTransition(BallShopState *state, float deltaTime)
{
    if (!state || state->transitionTime <= 0.0f)
        return;
    const float dt = (deltaTime > 0.0f) ? deltaTime : 0.0f;
    state->transitionTime = glm::max(0.0f, state->transitionTime - dt);
}

static inline float BallShop_TransitionNorm(const BallShopState *state)
{
    if (!state || state->transitionDuration <= 0.0f || state->transitionTime <= 0.0f)
        return 0.0f;
    return glm::clamp(state->transitionTime / state->transitionDuration, 0.0f, 1.0f);
}

static inline int BallShop_RarityWeight(const char *rarity)
{
    if (!rarity)
        return 18;
    if (strcmp(rarity, "LEGEND") == 0 || strcmp(rarity, "LEGENDARY") == 0)
        return 1;
    if (strcmp(rarity, "EPIC") == 0)
        return 3;
    if (strcmp(rarity, "RARE") == 0)
        return 7;
    return 18;
}

static inline uint64_t BallShop_BucketIdForEpoch(uint64_t epochSeconds, int refreshMinutes)
{
    const uint64_t secondsPerBucket = (uint64_t)((refreshMinutes > 0) ? refreshMinutes : BALL_SHOP_DEFAULT_REFRESH_MINUTES) * 60ull;
    return (secondsPerBucket > 0ull) ? (epochSeconds / secondsPerBucket) : epochSeconds;
}

static inline int BallShop_SecondsUntilNextRefresh(uint64_t epochSeconds, int refreshMinutes)
{
    const int minutes = (refreshMinutes > 0) ? refreshMinutes : BALL_SHOP_DEFAULT_REFRESH_MINUTES;
    const uint64_t secondsPerBucket = (uint64_t)minutes * 60ull;
    if (secondsPerBucket == 0ull)
        return 0;
    const uint64_t elapsedInBucket = epochSeconds % secondsPerBucket;
    const uint64_t remaining = secondsPerBucket - elapsedInBucket;
    return (int)((remaining == 0ull) ? secondsPerBucket : remaining);
}

static inline uint64_t BallShop_NextRandom(uint64_t *state)
{
    if (!state)
        return 0ull;
    uint64_t x = *state;
    if (x == 0ull)
        x = 0x9E3779B97F4A7C15ull;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *state = x;
    return x * 2685821657736338717ull;
}

static inline uint64_t BallShop_StarterOwnedMask()
{
    return 1ull << 0;
}

// The quote is shared by every copy of a ball during this shop bucket. It
// changes only when the shop renews, even if the player closes the window.
static inline int BallShop_SellPrice(const CatalogItem &item, uint64_t bucketId)
{
    uint64_t seed = (bucketId * 0x9E3779B97F4A7C15ull) ^
                    ((uint64_t)(item.id + 1) * 0xBF58476D1CE4E5B9ull);
    const uint64_t roll = BallShop_NextRandom(&seed) % 10001ull;
    const double multiplier = (2000.0 + (double)roll) / 10000.0; // 20%..120%
    return (int)(ceil((double)item.price * multiplier / 10.0) * 10.0);
}

static inline bool BallShop_CanSell(uint64_t ownedMask, uint64_t destroyedMask,
                                    int equippedBallId, int ballId)
{
    if (ballId < 0 || ballId >= 63 || ballId == equippedBallId)
        return false;
    const uint64_t bit = 1ull << ballId;
    return (ownedMask & bit) != 0ull && (destroyedMask & bit) == 0ull;
}

// One transaction updates ownership and cash together; a repeated click
// cannot pay out again after the ownership bit has been removed.
static inline int BallShop_SellOwnedBall(uint64_t *ownedMask, int *bank, uint64_t destroyedMask,
                                         int equippedBallId, int ballId, uint64_t bucketId)
{
    if (!ownedMask || !bank || !BallShop_CanSell(*ownedMask, destroyedMask, equippedBallId, ballId))
        return 0;
    for (size_t i = 0; i < g_ballCatalogCount; ++i)
    {
        if (g_ballCatalog[i].id != ballId)
            continue;
        const int price = BallShop_SellPrice(g_ballCatalog[i], bucketId);
        *ownedMask &= ~(1ull << ballId);
        *bank += price;
        return price;
    }
    return 0;
}

static inline int BallShop_BuildInventoryItems(
    uint64_t ownedMask,
    int preferredBallId,
    CatalogItem *outItems,
    int maxItems
)
{
    if (!outItems || maxItems <= 0)
        return 0;

    int outCount = 0;
    const uint64_t preferredBit =
        (preferredBallId >= 0 && preferredBallId < 63) ? (1ull << preferredBallId) : 0ull;

    if (preferredBit != 0ull && (ownedMask & preferredBit) != 0ull)
    {
        for (int i = 0; i < (int)g_ballCatalogCount && outCount < maxItems; ++i)
        {
            const CatalogItem &item = g_ballCatalog[i];
            if (item.id == preferredBallId)
            {
                outItems[outCount++] = item;
                break;
            }
        }
    }

    for (int i = 0; i < (int)g_ballCatalogCount && outCount < maxItems; ++i)
    {
        const CatalogItem &item = g_ballCatalog[i];
        if (item.id == preferredBallId)
            continue;
        const bool owned = (item.id >= 0 && item.id < 63) ? (((ownedMask >> item.id) & 1ull) != 0ull) : false;
        if (!owned)
            continue;
        outItems[outCount++] = item;
    }

    return outCount;
}

static inline int BallShop_GenerateStockForBucket(
    uint64_t ownedMask,
    int guaranteedBallId,
    uint64_t bucketId,
    CatalogItem *outItems,
    int maxItems
)
{
    if (!outItems || maxItems <= 0)
        return 0;

    int candidateIds[64];
    int candidateCount = 0;
    int guaranteedCatalogIndex = -1;
    for (int i = 0; i < (int)g_ballCatalogCount; ++i)
    {
        const CatalogItem &item = g_ballCatalog[i];
        const bool owned = (item.id >= 0 && item.id < 63) ? (((ownedMask >> item.id) & 1ull) != 0ull) : false;
        if (item.id == guaranteedBallId && !owned)
            guaranteedCatalogIndex = i;
        if (!owned)
            candidateIds[candidateCount++] = i;
    }

    const int targetCount = (candidateCount < maxItems) ? candidateCount : maxItems;
    uint64_t rngState = bucketId ^ 0xA5A55A5A12345678ull;
    int outCount = 0;
    if (guaranteedCatalogIndex >= 0 && outCount < targetCount)
    {
        outItems[outCount++] = g_ballCatalog[guaranteedCatalogIndex];
        for (int i = 0; i < candidateCount; ++i)
        {
            if (candidateIds[i] == guaranteedCatalogIndex)
            {
                candidateIds[i] = candidateIds[candidateCount - 1];
                candidateCount--;
                break;
            }
        }
    }

    while (outCount < targetCount && candidateCount > 0)
    {
        int totalWeight = 0;
        for (int i = 0; i < candidateCount; ++i)
            totalWeight += BallShop_RarityWeight(g_ballCatalog[candidateIds[i]].rarity);
        if (totalWeight <= 0)
            break;

        const int pick = (int)(BallShop_NextRandom(&rngState) % (uint64_t)totalWeight);
        int cursor = 0;
        int chosenPos = candidateCount - 1;
        for (int i = 0; i < candidateCount; ++i)
        {
            cursor += BallShop_RarityWeight(g_ballCatalog[candidateIds[i]].rarity);
            if (pick < cursor)
            {
                chosenPos = i;
                break;
            }
        }

        outItems[outCount++] = g_ballCatalog[candidateIds[chosenPos]];
        candidateIds[chosenPos] = candidateIds[candidateCount - 1];
        candidateCount--;
    }

    return outCount;
}
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
    int closest3rdBallIdx;
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
    cs->closestBallIdx = -1;
    cs->closest2ndBallIdx = -1;
    cs->closest3rdBallIdx = -1;
}

static inline void Carousel_ClearItems(CarouselState *cs)
{
    if (!cs)
        return;
    cs->scrollOffset = 0.0f;
    cs->velocity = 0.0f;
    cs->isDragging = false;
    cs->isAutoDragging = false;
    cs->isGrabbed = false;
    cs->startingX = 0;
    cs->closestBallIdx = -1;
    cs->closest2ndBallIdx = -1;
    cs->closest3rdBallIdx = -1;
    cs->cardCount = 0;
}

static inline bool Carousel_AddCatalogItem(CarouselState *cs, const CatalogItem *item)
{
    if (!cs || !item || cs->cardCount >= CAROUSEL_MAX_CARDS)
        return false;
    memcpy(&cs->items[cs->cardCount], item, sizeof(CatalogItem));
    cs->cardCount++;
    return true;
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
        cs->closest3rdBallIdx = -1;
        return;
    }

    // Pick the closest, 2nd, and 3rd closest cards by distance to the belt's exact position.
    float exact = cs->scrollOffset / slotWidth;
    struct Candidate { int idx; float dist; };
    Candidate best[3] = { {-1, FLT_MAX}, {-1, FLT_MAX}, {-1, FLT_MAX} };
    for (int i = 0; i < cs->cardCount; i++)
    {
        float slotForI = -(float)i;
        float dist = glm::abs(exact - slotForI);
        Candidate cand{i, dist};
        for (int k = 0; k < 3; k++)
        {
            if (cand.dist < best[k].dist)
            {
                for (int j = 2; j > k; j--) best[j] = best[j - 1];
                best[k] = cand;
                break;
            }
        }
    }
    cs->closestBallIdx = best[0].idx;
    cs->closest2ndBallIdx = best[1].idx;
    cs->closest3rdBallIdx = best[2].idx;
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
            return Carousel_AddCatalogItem(cs, &g_ballCatalog[i]);
        }
    }
    return false;
}

// ─────────────────────────────────────────────────────────────
// b) Populate carousel with a safe starter-only fallback.
// Real shop/inventory contents are rebuilt from ownership + stock state when opened.
// ─────────────────────────────────────────────────────────────
void Carousel_SetupDefaultShop(CarouselState* cs) {
    if (!cs) return;

    Carousel_ClearItems(cs);
    Carousel_AddBall(cs, "Ember Strike");
}
