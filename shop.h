#pragma once

#define CAROUSEL_CARD_WIDTH 280.0f
#define CAROUSEL_CARD_SPACING 20.0f
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

    const CatalogItem *items;
    int cardCount;
} CarouselState;
struct Shop
{
};