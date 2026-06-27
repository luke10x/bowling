#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../3rdparty/json/tests/thirdparty/doctest/doctest.h"

#include "../shop.h"
#include "../ui_pause_policy.h"

TEST_CASE("Ball shop bucket math is configurable by refresh minutes")
{
    CHECK(BallShop_BucketIdForEpoch(0, 30) == 0);
    CHECK(BallShop_BucketIdForEpoch(1799, 30) == 0);
    CHECK(BallShop_BucketIdForEpoch(1800, 30) == 1);

    CHECK(BallShop_BucketIdForEpoch(299, 5) == 0);
    CHECK(BallShop_BucketIdForEpoch(300, 5) == 1);
    CHECK(BallShop_SecondsUntilNextRefresh(299, 5) == 1);
    CHECK(BallShop_SecondsUntilNextRefresh(300, 5) == 300);
}

TEST_CASE("Ball shop stock generation is stable within a bucket and capped to five unique balls")
{
    CatalogItem stockA[BALL_SHOP_STOCK_SIZE] = {};
    CatalogItem stockB[BALL_SHOP_STOCK_SIZE] = {};

    const int countA = BallShop_GenerateStockForBucket(0ull, 42ull, stockA, BALL_SHOP_STOCK_SIZE);
    const int countB = BallShop_GenerateStockForBucket(0ull, 42ull, stockB, BALL_SHOP_STOCK_SIZE);

    CHECK(countA == BALL_SHOP_STOCK_SIZE);
    CHECK(countB == BALL_SHOP_STOCK_SIZE);

    for (int i = 0; i < countA; ++i)
    {
        CHECK(stockA[i].id == stockB[i].id);
        for (int j = i + 1; j < countA; ++j)
            CHECK(stockA[i].id != stockA[j].id);
    }
}

TEST_CASE("Ball shop stock generation excludes owned balls and can become empty")
{
    CatalogItem stock[BALL_SHOP_STOCK_SIZE] = {};

    uint64_t ownedMask = 0ull;
    ownedMask |= (1ull << 0);
    ownedMask |= (1ull << 1);
    ownedMask |= (1ull << 2);

    const int count = BallShop_GenerateStockForBucket(ownedMask, 7ull, stock, BALL_SHOP_STOCK_SIZE);
    CHECK(count == BALL_SHOP_STOCK_SIZE);
    for (int i = 0; i < count; ++i)
    {
        CHECK(stock[i].id != 0);
        CHECK(stock[i].id != 1);
        CHECK(stock[i].id != 2);
    }

    uint64_t allOwnedMask = 0ull;
    for (int i = 0; i < (int)g_ballCatalogCount; ++i)
        allOwnedMask |= (1ull << g_ballCatalog[i].id);

    CHECK(BallShop_GenerateStockForBucket(allOwnedMask, 7ull, stock, BALL_SHOP_STOCK_SIZE) == 0);
}

TEST_CASE("Modal pause begin triggers only on first window-open edge")
{
    CHECK(UiModalPauseShouldBegin(false, 0) == false);
    CHECK(UiModalPauseShouldBegin(false, 1) == true);
    CHECK(UiModalPauseShouldBegin(false, 3) == true);
    CHECK(UiModalPauseShouldBegin(true, 1) == false);
    CHECK(UiModalPauseShouldBegin(true, 2) == false);
    CHECK(UiModalPauseShouldBegin(true, 0) == false);
}
