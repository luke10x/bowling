#pragma once

#include "ads_config.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <ctime>

// Calendar days are local to the player; holidays use absolute Unix time so
// closing the game or crossing midnight cannot shorten the configured holiday.
struct AdPolicyState
{
    float credit = AdsConfig::CreditInitial;
    float creditBucket = 0.0f;
    int lastLoginDay = 0; // YYYYMMDD
    int counterDay = 0;   // YYYYMMDD
    int interstitialsToday = 0;
    int moneyAdsToday = 0;
    int64_t holidayEndsAt = 0;
    uint64_t rewardedShopBucketId = UINT64_MAX;
    int moneyAdsThisRestock = 0;
};

static constexpr int kAdPolicyDailyInterstitialLimit = AdsConfig::InterstitialDailyLimit;
static constexpr int kAdPolicyDailyMoneyAdLimit = AdsConfig::MoneyDailyLimit;
static constexpr int64_t kAdPolicyHolidaySeconds = AdsConfig::HolidayHours * 60 * 60;
static_assert(AdsConfig::MoneyPerRestockLimit >= 0,
              "The rewarded ad limit per restock cannot be negative");
static_assert(AdsConfig::CreditBucketSize > 0.0f,
              "The win credit bucket must have a positive size");
static_assert(AdsConfig::CreditBucketWinBonus >= 0.0f,
              "The win credit bucket bonus cannot be negative");

static inline int AdPolicy_LocalDay(int64_t epochSeconds)
{
    const time_t seconds = static_cast<time_t>(epochSeconds);
    const tm *local = std::localtime(&seconds);
    return local ? (local->tm_year + 1900) * 10000 + (local->tm_mon + 1) * 100 + local->tm_mday : 0;
}

static inline void AdPolicy_RefreshDay(AdPolicyState &state, int today)
{
    if (today > state.counterDay)
    {
        state.counterDay = today;
        state.interstitialsToday = 0;
        state.moneyAdsToday = 0;
    }
}

static inline bool AdPolicy_OnLogin(AdPolicyState &state, int today)
{
    AdPolicy_RefreshDay(state, today);
    if (today <= 0 || today <= state.lastLoginDay)
        return false;
    const bool earned = state.lastLoginDay > 0;
    if (earned)
        state.credit = std::min(AdsConfig::CreditMax, state.credit + AdsConfig::DailyLoginBonus);
    state.lastLoginDay = today;
    return true;
}

static inline bool AdPolicy_OnHoliday(const AdPolicyState &state, int64_t now)
{
    return state.holidayEndsAt > now;
}

// Returns whether this outcome should request an interstitial. The cost is
// charged once, at the outcome; a loss can defer its placement until Retry.
static inline bool AdPolicy_OnOutcome(AdPolicyState &state, bool won, int today, int64_t now)
{
    AdPolicy_RefreshDay(state, today);
    if (AdPolicy_OnHoliday(state, now) ||
        state.interstitialsToday >= kAdPolicyDailyInterstitialLimit)
        return false;
    if (won && state.credit < AdsConfig::CreditBucketSize)
    {
        state.creditBucket += AdsConfig::CreditBucketWinBonus;
        while (state.creditBucket >= AdsConfig::CreditBucketSize)
        {
            state.creditBucket -= AdsConfig::CreditBucketSize;
            state.credit = std::min(AdsConfig::CreditMax,
                                    state.credit + AdsConfig::CreditBucketSize);
        }
    }
    const float cost = won ? AdsConfig::WinCost : AdsConfig::LossCost;
    if (state.credit >= cost)
    {
        state.credit = std::max(0.0f, state.credit - cost);
        return false;
    }
    state.credit = 0.0f;
    return true;
}

// Called only after the platform confirms that an ad actually appeared.
static inline bool AdPolicy_OnInterstitialShown(AdPolicyState &state, int today, int64_t now)
{
    AdPolicy_RefreshDay(state, today);
    if (AdPolicy_OnHoliday(state, now) ||
        state.interstitialsToday >= kAdPolicyDailyInterstitialLimit)
        return false;
    ++state.interstitialsToday;
    if (state.interstitialsToday == kAdPolicyDailyInterstitialLimit)
        state.holidayEndsAt = now + kAdPolicyHolidaySeconds;
    return true;
}

static inline bool AdPolicy_CanOfferMoneyAd(const AdPolicyState &state, uint64_t shopBucketId)
{
    return AdsConfig::MoneyPerRestockLimit > 0 &&
           state.moneyAdsToday < kAdPolicyDailyMoneyAdLimit &&
           (state.rewardedShopBucketId != shopBucketId ||
            state.moneyAdsThisRestock < AdsConfig::MoneyPerRestockLimit);
}

// A verified reward consumes one offer from this restock and one daily money ad.
// Rewarded ads never contribute to the interstitial holiday.
static inline bool AdPolicy_OnMoneyAdViewed(AdPolicyState &state, int today, uint64_t shopBucketId)
{
    AdPolicy_RefreshDay(state, today);
    if (!AdPolicy_CanOfferMoneyAd(state, shopBucketId))
        return false;
    ++state.moneyAdsToday;
    if (state.rewardedShopBucketId != shopBucketId)
        state.moneyAdsThisRestock = 0;
    state.rewardedShopBucketId = shopBucketId;
    ++state.moneyAdsThisRestock;
    return true;
}

static inline void AdPolicy_Format(const AdPolicyState &state, char *out, size_t size)
{
    std::snprintf(out, size, "%.3f,%d,%d,%d,%d,%lld,%llu,%d,%.3f", state.credit,
                  state.lastLoginDay, state.counterDay,
                  state.interstitialsToday, state.moneyAdsToday,
                  static_cast<long long>(state.holidayEndsAt),
                  static_cast<unsigned long long>(state.rewardedShopBucketId),
                  state.moneyAdsThisRestock, state.creditBucket);
}

static inline bool AdPolicy_Parse(AdPolicyState &state, const char *text)
{
    if (!text) return false;
    AdPolicyState parsed;
    long long holiday = 0;
    unsigned long long rewardedBucket = ULLONG_MAX;
    const int fields = std::sscanf(text, "%f,%d,%d,%d,%d,%lld,%llu,%d,%f", &parsed.credit,
                    &parsed.lastLoginDay, &parsed.counterDay,
                    &parsed.interstitialsToday, &parsed.moneyAdsToday, &holiday,
                    &rewardedBucket, &parsed.moneyAdsThisRestock, &parsed.creditBucket);
    if (fields < 6 || !std::isfinite(parsed.credit) ||
        (fields >= 9 && !std::isfinite(parsed.creditBucket)))
        return false;
    parsed.credit = std::max(0.0f, std::min(AdsConfig::CreditMax, parsed.credit));
    parsed.creditBucket = std::max(0.0f, std::min(AdsConfig::CreditBucketSize,
                                                  parsed.creditBucket));
    parsed.interstitialsToday = std::max(0, std::min(kAdPolicyDailyInterstitialLimit, parsed.interstitialsToday));
    parsed.moneyAdsToday = std::max(0, std::min(kAdPolicyDailyMoneyAdLimit, parsed.moneyAdsToday));
    parsed.holidayEndsAt = std::max<int64_t>(0, holiday);
    parsed.rewardedShopBucketId = static_cast<uint64_t>(rewardedBucket);
    // Seven-field saves recorded one reward in the stored bucket. Six-field
    // saves predate the bucket and have no restock rewards to carry forward.
    parsed.moneyAdsThisRestock = parsed.rewardedShopBucketId == UINT64_MAX
        ? 0 : (fields == 7 ? 1 : std::max(0, parsed.moneyAdsThisRestock));
    state = parsed;
    return true;
}
