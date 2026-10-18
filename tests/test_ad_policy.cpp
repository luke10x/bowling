#include "ad_policy.h"
#include <cassert>
#include <cstring>

int main()
{
    constexpr int day1 = 20261018;
    constexpr int day2 = 20261019;
    constexpr int64_t start = 1'800'000'000;
    AdPolicyState state;
    assert(state.credit == 50.0f);
    assert(AdPolicy_OnLogin(state, day1));
    assert(state.credit == 50.0f);
    assert(!AdPolicy_OnOutcome(state, true, day1, start));
    assert(state.credit == 49.0f);
    assert(!AdPolicy_OnOutcome(state, false, day1, start));
    assert(state.credit == 44.0f);

    state.credit = 2.0f;
    assert(AdPolicy_OnOutcome(state, false, day1, start));
    assert(state.credit == 0.0f);
    for (int i = 1; i <= kAdPolicyDailyInterstitialLimit; ++i)
    {
        assert(AdPolicy_OnInterstitialShown(state, day1, start + i));
        assert(state.interstitialsToday == i);
    }
    assert(state.holidayEndsAt == start + 5 + kAdPolicyHolidaySeconds);
    assert(AdPolicy_OnHoliday(state, start + 5));
    assert(!AdPolicy_OnOutcome(state, false, day1, start + 6));
    assert(!AdPolicy_OnInterstitialShown(state, day1, start + 6));

    // The daily login bonus still applies during the real-time holiday.
    assert(AdPolicy_OnLogin(state, day2));
    assert(state.credit == 10.0f);
    assert(state.interstitialsToday == 0);
    assert(!AdPolicy_OnOutcome(state, true, day2, start + 6));
    assert(state.credit == 10.0f);
    assert(!AdPolicy_OnLogin(state, day2));
    assert(!AdPolicy_OnHoliday(state, state.holidayEndsAt));

    for (int i = 0; i < kAdPolicyDailyMoneyAdLimit; ++i)
        assert(AdPolicy_OnMoneyAdViewed(state, day2));
    assert(!AdPolicy_OnMoneyAdViewed(state, day2));
    assert(state.interstitialsToday == 0);

    char saved[128];
    AdPolicy_Format(state, saved, sizeof(saved));
    AdPolicyState loaded;
    assert(AdPolicy_Parse(loaded, saved));
    assert(loaded.credit == state.credit);
    assert(loaded.holidayEndsAt == state.holidayEndsAt);
    assert(loaded.moneyAdsToday == kAdPolicyDailyMoneyAdLimit);
    assert(!AdPolicy_Parse(loaded, "corrupt"));
    return 0;
}
