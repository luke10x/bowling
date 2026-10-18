#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <ctime>

// Calendar days are local to the player; holidays use absolute Unix time so
// closing the game or crossing midnight cannot shorten the promised 24 hours.
struct AdPolicyState
{
    float credit = 50.0f;
    int lastLoginDay = 0; // YYYYMMDD
    int counterDay = 0;   // YYYYMMDD
    int interstitialsToday = 0;
    int moneyAdsToday = 0;
    int64_t holidayEndsAt = 0;
};

static constexpr int kAdPolicyDailyInterstitialLimit = 5;
static constexpr int kAdPolicyDailyMoneyAdLimit = 3;
static constexpr int64_t kAdPolicyHolidaySeconds = 24 * 60 * 60;

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
        state.credit = std::min(100.0f, state.credit + 10.0f);
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
    const float cost = won ? 1.0f : 5.0f;
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

// Reserved for future rewarded money ads. They never count toward the holiday.
static inline bool AdPolicy_OnMoneyAdViewed(AdPolicyState &state, int today)
{
    AdPolicy_RefreshDay(state, today);
    if (state.moneyAdsToday >= kAdPolicyDailyMoneyAdLimit)
        return false;
    ++state.moneyAdsToday;
    return true;
}

static inline void AdPolicy_Format(const AdPolicyState &state, char *out, size_t size)
{
    std::snprintf(out, size, "%.3f,%d,%d,%d,%d,%lld", state.credit,
                  state.lastLoginDay, state.counterDay,
                  state.interstitialsToday, state.moneyAdsToday,
                  static_cast<long long>(state.holidayEndsAt));
}

static inline bool AdPolicy_Parse(AdPolicyState &state, const char *text)
{
    if (!text) return false;
    AdPolicyState parsed;
    long long holiday = 0;
    if (std::sscanf(text, "%f,%d,%d,%d,%d,%lld", &parsed.credit,
                    &parsed.lastLoginDay, &parsed.counterDay,
                    &parsed.interstitialsToday, &parsed.moneyAdsToday, &holiday) != 6 ||
        !std::isfinite(parsed.credit))
        return false;
    parsed.credit = std::max(0.0f, std::min(100.0f, parsed.credit));
    parsed.interstitialsToday = std::max(0, std::min(kAdPolicyDailyInterstitialLimit, parsed.interstitialsToday));
    parsed.moneyAdsToday = std::max(0, std::min(kAdPolicyDailyMoneyAdLimit, parsed.moneyAdsToday));
    parsed.holidayEndsAt = std::max<int64_t>(0, holiday);
    state = parsed;
    return true;
}
