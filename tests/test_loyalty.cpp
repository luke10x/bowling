#include "../loyalty.h"

#include <cassert>

int main()
{
    LoyaltyState fresh{20261021, 1, 20261021};
    assert(!Loyalty_OnActivation(fresh, 20261021));
    assert(Loyalty_OnActivation(fresh, 20261022));
    assert(fresh.streak == 2);
    // Returning focus while the window is open can queue the same reward again.
    bool pendingWindow = true;
    assert(Loyalty_ClaimToday(fresh, pendingWindow, 20261022));
    assert(!pendingWindow && fresh.claimedDay == 20261022);
    pendingWindow = true;
    assert(!Loyalty_ClaimToday(fresh, pendingWindow, 20261022));
    assert(!pendingWindow);
    assert(!Loyalty_OnActivation(fresh, 20261022));

    // The DAY cheat clears only today's claim; a later activation offers it again.
    fresh.claimedDay = 0;
    assert(Loyalty_OnActivation(fresh, 20261022));
    assert(fresh.streak == 2);

    assert(Loyalty_OnActivation(fresh, 20261024));
    assert(fresh.streak == 1);
    assert(Loyalty_IsNextDay(20261231, 20270101));
    assert(!Loyalty_IsNextDay(20261231, 20270102));
    assert(!Loyalty_OnActivation(fresh, 20261023)); // rolled-back local clock

    LoyaltyState restored;
    assert(Loyalty_Parse(restored, "20261022,2,20261022"));
    assert(restored.lastSeenDay == 20261022 && restored.streak == 2 &&
           restored.claimedDay == 20261022);
}
