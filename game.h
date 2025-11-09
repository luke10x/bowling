#pragma once

#include <SDL.h>

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <vector>
#include <memory>
#include <map>
#include <iostream>
#include <thread>
#include <chrono>

#include "framework/ctx.h"

#include "aurora.h"

struct UserContext
{
    bool fuckCakez = true;
    Aurora aurora;
    uint32_t lastFrameTime = 0;

    void setupLevel(const int whichLevelNewVal);
};

void UserContext::setupLevel(const int whichLevelNewVal)
{
    // To make refactoring easy
    UserContext *usr = this;
}
