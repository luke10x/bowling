#include "ad_policy.h"
#include <cassert>
#include <cstring>

int main()
{
    constexpr int day1 = 20261018;
    constexpr int day2 = 20261019;
    constexpr int64_t start = 1'800'000'000;
    AdPolicyState state;
    assert(state.credit == AdsConfig::CreditInitial);
    assert(AdPolicy_OnLogin(state, day1));
    assert(state.credit == AdsConfig::CreditInitial);
    assert(!AdPolicy_OnOutcome(state, true, day1, start));
    assert(state.credit == AdsConfig::CreditInitial - AdsConfig::WinCost);
    state.credit = AdsConfig::CreditMax;
    assert(!AdPolicy_OnOutcome(state, false, day1, start));
    assert(state.credit == AdsConfig::CreditMax - AdsConfig::LossCost);

    state.credit = 2.0f;
    assert(AdPolicy_OnOutcome(state, false, day1, start));
    assert(state.credit == 0.0f);
    for (int i = 1; i <= kAdPolicyDailyInterstitialLimit; ++i)
    {
        assert(AdPolicy_OnInterstitialShown(state, day1, start + i));
        assert(state.interstitialsToday == i);
    }
    assert(state.holidayEndsAt == start + kAdPolicyDailyInterstitialLimit + kAdPolicyHolidaySeconds);
    assert(AdPolicy_OnHoliday(state, start + kAdPolicyDailyInterstitialLimit));
    assert(!AdPolicy_OnOutcome(state, false, day1, start + 6));
    assert(!AdPolicy_OnInterstitialShown(state, day1, start + 6));

    // The daily login bonus still applies during the real-time holiday.
    assert(AdPolicy_OnLogin(state, day2));
    assert(state.credit == AdsConfig::DailyLoginBonus);
    assert(state.interstitialsToday == 0);
    assert(!AdPolicy_OnOutcome(state, true, day2, start + 6));
    assert(state.credit == AdsConfig::DailyLoginBonus);
    assert(!AdPolicy_OnLogin(state, day2));
    assert(!AdPolicy_OnHoliday(state, state.holidayEndsAt));

    AdPolicyState winBucket;
    winBucket.credit = 0.0f;
    assert(AdPolicy_OnOutcome(winBucket, true, day1, start));
    assert(winBucket.credit == 0.0f &&
           winBucket.creditBucket == AdsConfig::CreditBucketWinBonus);
    // The second low-credit win fills the reserve before its outcome charge.
    assert(!AdPolicy_OnOutcome(winBucket, true, day1, start));
    assert(winBucket.credit == AdsConfig::CreditBucketSize - AdsConfig::WinCost);
    assert(winBucket.creditBucket == 0.0f);
    winBucket.credit = AdsConfig::CreditBucketSize;
    assert(!AdPolicy_OnOutcome(winBucket, true, day1, start));
    assert(winBucket.credit == AdsConfig::CreditBucketSize - AdsConfig::WinCost);
    assert(winBucket.creditBucket == 0.0f);
    winBucket.credit = AdsConfig::CreditBucketSize + 1.0f;
    assert(!AdPolicy_OnOutcome(winBucket, true, day1, start));
    assert(winBucket.creditBucket == 0.0f);
    winBucket.credit = 0.0f;
    assert(AdPolicy_OnOutcome(winBucket, true, day1, start));
    winBucket.credit = AdsConfig::CreditBucketSize + 1.0f;
    assert(!AdPolicy_OnOutcome(winBucket, true, day1, start));
    assert(winBucket.creditBucket == AdsConfig::CreditBucketWinBonus);
    winBucket.credit = AdsConfig::CreditBucketSize - 1.0f;
    assert(!AdPolicy_OnOutcome(winBucket, true, day1, start));
    assert(winBucket.credit == 2.0f * AdsConfig::CreditBucketSize - 2.0f);
    assert(winBucket.creditBucket == 0.0f);
    winBucket.credit = 0.0f;
    winBucket.holidayEndsAt = start + 60;
    assert(!AdPolicy_OnOutcome(winBucket, true, day1, start));
    assert(winBucket.credit == 0.0f && winBucket.creditBucket == 0.0f);
    winBucket.holidayEndsAt = 0;
    winBucket.interstitialsToday = kAdPolicyDailyInterstitialLimit;
    assert(!AdPolicy_OnOutcome(winBucket, true, day1, start));
    assert(winBucket.creditBucket == 0.0f);
    winBucket.interstitialsToday = 0;
    assert(AdPolicy_OnOutcome(winBucket, true, day1, start));
    char bucketSave[128];
    AdPolicy_Format(winBucket, bucketSave, sizeof(bucketSave));
    AdPolicyState restoredBucket;
    assert(AdPolicy_Parse(restoredBucket, bucketSave));
    assert(restoredBucket.creditBucket == AdsConfig::CreditBucketWinBonus);

    for (int i = 0; i < kAdPolicyDailyMoneyAdLimit; ++i)
    {
        const uint64_t bucket = 200 + i;
        assert(AdPolicy_CanOfferMoneyAd(state, bucket));
        assert(AdPolicy_OnMoneyAdViewed(state, day2, bucket));
        assert(state.moneyAdsThisRestock == 1);
    }
    assert(!AdPolicy_OnMoneyAdViewed(state, day2, 203));
    assert(state.interstitialsToday == 0);

    char saved[128];
    AdPolicy_Format(state, saved, sizeof(saved));
    AdPolicyState loaded;
    assert(AdPolicy_Parse(loaded, saved));
    assert(loaded.credit == state.credit);
    assert(loaded.holidayEndsAt == state.holidayEndsAt);
    assert(loaded.moneyAdsToday == kAdPolicyDailyMoneyAdLimit);
    assert(loaded.rewardedShopBucketId == 200 + kAdPolicyDailyMoneyAdLimit - 1);
    assert(loaded.moneyAdsThisRestock == 1);
    assert(loaded.creditBucket == 0.0f);
    // Seven-field saves spent one reward in their recorded bucket.
    assert(AdPolicy_Parse(loaded, "50.000,20261018,20261018,0,1,0,123"));
    assert(loaded.rewardedShopBucketId == 123);
    assert(loaded.moneyAdsThisRestock == 1);
    // Old save files have no bucket field and remain readable.
    assert(AdPolicy_Parse(loaded, "50.000,20261018,20261018,0,0,0"));
    assert(loaded.rewardedShopBucketId == UINT64_MAX);
    assert(loaded.moneyAdsThisRestock == 0);
    assert(loaded.creditBucket == 0.0f);

    // The restock allowance persists through saves and midnight, while the
    // daily allowance resets. A new restock starts its own count.
    AdPolicyState rewardState;
    constexpr uint64_t rewardBucket = 987;
    for (int i = 0; i < AdsConfig::MoneyPerRestockLimit; ++i)
    {
        const int day = day2 + i;
        AdPolicy_RefreshDay(rewardState, day);
        assert(AdPolicy_CanOfferMoneyAd(rewardState, rewardBucket));
        assert(AdPolicy_OnMoneyAdViewed(rewardState, day, rewardBucket));
        assert(rewardState.moneyAdsThisRestock == i + 1);
        AdPolicy_Format(rewardState, saved, sizeof(saved));
        assert(AdPolicy_Parse(loaded, saved));
        rewardState = loaded;
    }
    assert(!AdPolicy_CanOfferMoneyAd(rewardState, rewardBucket));
    assert(!AdPolicy_OnMoneyAdViewed(rewardState, day2 + AdsConfig::MoneyPerRestockLimit, rewardBucket));
    AdPolicy_RefreshDay(rewardState, day2 + AdsConfig::MoneyPerRestockLimit);
    assert(AdPolicy_OnMoneyAdViewed(rewardState, day2 + AdsConfig::MoneyPerRestockLimit, rewardBucket + 1));
    assert(rewardState.moneyAdsThisRestock == 1);
    assert(!AdPolicy_Parse(loaded, "corrupt"));
    return 0;
}
