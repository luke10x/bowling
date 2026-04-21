#pragma once

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
    const char *name;
    const char *rarity; // "COMMON", "RARE", "EPIC", "LEGENDARY"
    float price;
    float mass, spin, skid, bite; // stats 0.0–1.0
    Clay_String imagePlaceholder; // or your actual image handle
    // Add more fields as needed (id, unlock condition, etc.)
} CatalogItem;

typedef struct
{
    // my new imple
    bool isGrabbed = false;
    int startingX = 0;
    // Motion state
    float scrollOffset;
    float velocity;
    bool isDragging;
    bool isAutoDragging;

    // Pointer tracking
    float pointerDownTime;
    float pointerDownX;
    float pointerCurrentX;
    float totalMovement;
    int pressedCardAbsoluteIndex; // -1 = none
    int pressedCardRelativePos;   // -1=left neighbour, 0=center, +1=right neighbour

    // Animation state (shared for autodrag + snap)
    int animTargetIndex;
    float animStartOffset;
    float animStartTime;
    float animTargetOffset;
    float animDuration;

    // Layout cache (set during render)
    float containerX, containerY, containerWidth, containerHeight;
    float minOffset, maxOffset;


    CatalogItem items[4];
    int cardCount;
} CarouselState;

void Carousel_Init(CarouselState *cs)
{
    memset(cs, 0, sizeof(CarouselState));
    cs->pressedCardAbsoluteIndex = -1;
    cs->animTargetIndex = -1;

   // Populate array manually
    cs->items[0] = (CatalogItem){"Strike Master", "RARE", 100.0f, 0.8f, 0.6f, 0.3f, 0.9f, CLAY_STRING("⚾")};
    cs->items[1] = (CatalogItem){"Spin Doctor",   "EPIC", 150.0f, 0.5f, 0.95f, 0.7f, 0.6f, CLAY_STRING("🌀")};
    cs->items[2] = (CatalogItem){"Pin Crusher",   "COMMON", 200.0f, 0.95f, 0.3f, 0.8f, 0.4f, CLAY_STRING("📌")};
    cs->items[3] = (CatalogItem){"Golden Strike", "LEGENDARY", 500.0f, 0.7f, 0.8f, 0.5f, 1.0f, CLAY_STRING("👑")};

    cs->cardCount = 4;
}


static inline float
Carousel_CenterOffsetForIndex(int idx, float cardW, float spacing, float containerW)
{
    float pitch = cardW + spacing;
    float cardCenterWorld = idx * pitch + cardW * 0.5f;
    return -(cardCenterWorld - containerW * 0.5f);
}

static inline int
Carousel_NearestIndex(float offset, float cardW, float spacing, float containerW, int count)
{
    float pitch = cardW + spacing;
    float centerWorld = containerW * 0.5f - offset;
    float idxF = (centerWorld - cardW * 0.5f) / pitch;
    int idx = (int)(idxF + 0.5f);
    return idx < 0 ? 0 : (idx >= count ? count - 1 : idx);
}

static inline float EaseOutQuad(float t)
{
    return t * (2.0f - t);
}
static inline float Clamp(float v, float mn, float mx)
{
    return v < mn ? mn : (v > mx ? mx : v);
}

void Carousel_Update(CarouselState *cs, float currentTime, float deltaTime, int cardCount)
{
    // cs->cardCount = cardCount;

    // Compute scroll bounds
    // float pitch = CAROUSEL_CARD_WIDTH + CAROUSEL_CARD_SPACING;
    // float contentW = cardCount * pitch - CAROUSEL_CARD_SPACING;
    // cs->minOffset = -(contentW - cs->containerWidth);
    // cs->maxOffset = 0.0f;
    // if (cs->minOffset > cs->maxOffset)
    //     cs->minOffset = cs->maxOffset = 0.0f;

    // // Handle animation (AUTODRAG or SNAP)
    // if (cs->isAutoDragging && cs->animTargetIndex >= 0)
    // {
    //     float elapsed = currentTime - cs->animStartTime;
    //     float t = elapsed / cs->animDuration;

    //     if (t >= 1.0f)
    //     {
    //         cs->scrollOffset = cs->animTargetOffset;
    //         cs->isAutoDragging = false;
    //         cs->animTargetIndex = -1;
    //         cs->velocity = 0.0f;
    //     }
    //     else
    //     {
    //         float eased = EaseOutQuad(Clamp(t, 0.0f, 1.0f));
    //         cs->scrollOffset =
    //             cs->animStartOffset + (cs->animTargetOffset - cs->animStartOffset) * eased;
    //     }
    // }
    // else if (cs->isDragging)
    // {
    //     cs->velocity *= 0.9f; // subtle damping while dragging
    // }
    // else
    // {
    //     cs->velocity *= 0.8f; // idle damping
    //     if (fabsf(cs->velocity) < 0.1f)
    //         cs->velocity = 0.0f;
    // }

    // cs->scrollOffset = Clamp(cs->scrollOffset, cs->minOffset, cs->maxOffset);
}

struct Shop
{
};