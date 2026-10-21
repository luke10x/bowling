#pragma once

#include <algorithm>
#include <cstdio>
#include <ctime>

struct LoyaltyState
{
    int lastSeenDay = 0; // local YYYYMMDD
    int streak = 0;
    int claimedDay = 0;
};

static constexpr int kLoyaltyDailyReward = 100;

static inline bool Loyalty_IsNextDay(int previous, int today)
{
    if (previous <= 0 || today <= previous) return false;
    tm date = {};
    date.tm_year = previous / 10000 - 1900;
    date.tm_mon = (previous / 100) % 100 - 1;
    date.tm_mday = previous % 100 + 1;
    date.tm_hour = 12; // midday avoids local daylight-saving midnight changes
    const time_t next = std::mktime(&date);
    if (next == time_t(-1)) return false;
    const tm *local = std::localtime(&next);
    return local && (local->tm_year + 1900) * 10000 + (local->tm_mon + 1) * 100 + local->tm_mday == today;
}

// Returns true when this activation should offer the unclaimed daily reward.
static inline bool Loyalty_OnActivation(LoyaltyState &state, int today)
{
    if (today <= 0 || today < state.lastSeenDay) return false;
    if (today > state.lastSeenDay)
    {
        state.streak = Loyalty_IsNextDay(state.lastSeenDay, today)
            ? std::max(1, state.streak + 1) : 1;
        state.lastSeenDay = today;
    }
    return state.claimedDay != today;
}

static inline bool Loyalty_Parse(LoyaltyState &state, const char *text)
{
    LoyaltyState parsed;
    if (!text || std::sscanf(text, "%d,%d,%d", &parsed.lastSeenDay,
                             &parsed.streak, &parsed.claimedDay) != 3)
        return false;
    parsed.streak = std::max(1, parsed.streak);
    state = parsed;
    return true;
}
