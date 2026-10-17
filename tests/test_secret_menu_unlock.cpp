#include "../secret_menu_unlock.h"

#include <cassert>

static bool click(SecretMenuUnlock &gesture, SecretMenuUnlock::Area area,
                  uint64_t downMs, uint64_t upMs, bool touch = false, uint64_t pointer = 1)
{
    gesture.pointerDown(area, touch, pointer, downMs);
    return gesture.pointerUp(area, touch, pointer, upMs);
}

int main()
{
    using Area = SecretMenuUnlock::Area;
    SecretMenuUnlock gesture;
    assert(!gesture.canOpenMinigames(false));
    assert(!gesture.canOpenMinigames(true));
    assert(!click(gesture, Area::Energy, 100, 120));
    assert(!click(gesture, Area::Timer, 600, 620, true, 42));
    assert(click(gesture, Area::Money, 2000, 2120)); // Exactly two seconds after Energy.
    assert(gesture.unlocked);
    assert(!gesture.canOpenMinigames(false));
    assert(gesture.canOpenMinigames(true));

    gesture = {};
    assert(!click(gesture, Area::Energy, 100, 120));
    assert(!click(gesture, Area::Other, 200, 220));
    assert(!click(gesture, Area::Timer, 300, 320));
    assert(!click(gesture, Area::Money, 400, 420));
    assert(!gesture.unlocked);

    gesture = {};
    assert(!click(gesture, Area::Energy, 100, 120));
    assert(!click(gesture, Area::Timer, 700, 720));
    assert(!click(gesture, Area::Money, 2120, 2121));
    assert(!gesture.unlocked);

    gesture = {};
    gesture.pointerDown(Area::Energy, true, 1, 100);
    assert(!gesture.pointerUp(Area::Timer, true, 1, 120)); // Dragged off target.
    assert(!click(gesture, Area::Timer, 200, 220));
    assert(!gesture.unlocked);

    gesture = {};
    gesture.pointerDown(Area::Energy, true, 1, 100);
    assert(!gesture.pointerUp(Area::Energy, true, 2, 120)); // Different finger.
    assert(!click(gesture, Area::Timer, 200, 220));
    assert(!gesture.unlocked);

    gesture = {};
    assert(!click(gesture, Area::Energy, 100, 120));
    gesture.pointerDown(Area::Other, false, 1, 180); // Other click cancels immediately.
    assert(!gesture.pointerUp(Area::Other, false, 1, 200));
    assert(gesture.progress == 0);
}
