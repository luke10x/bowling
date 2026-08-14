#pragma once

#include <cstring>
#include <iostream>

#include "stubs.h"

static inline bool Cheats_Matches3CaseInsensitive(const UserContext *usr, char a, char b, char c)
{
    if (!usr || usr->username_len != 3)
        return false;
    const char ua = usr->username[0];
    const char ub = usr->username[1];
    const char uc = usr->username[2];
    return (ua == a || ua == char(a - 'A' + 'a')) &&
           (ub == b || ub == char(b - 'A' + 'a')) &&
           (uc == c || uc == char(c - 'A' + 'a'));
}

static inline bool Cheats_MatchesCaseInsensitive(const UserContext *usr, const char *code)
{
    if (!usr || !code)
        return false;
    const int len = (int)std::strlen(code);
    if (usr->username_len != len)
        return false;
    for (int i = 0; i < len; ++i)
    {
        char expected = code[i];
        char got = usr->username[i];
        if (expected >= 'A' && expected <= 'Z')
        {
            if (got >= 'a' && got <= 'z')
                got = char(got - 'a' + 'A');
        }
        if (got != expected)
            return false;
    }
    return true;
}

static inline bool Cheats_ShouldShowImgui(const UserContext *usr)
{
    return usr && usr->username_len == 7 && std::memcmp(usr->username, "GUGUCAS", 7) == 0;
}

static inline void Cheats_ApplyEndOfRound(UserContext *usr)
{
    if (!usr)
        return;
    setupStubScoreboardEndOfRound(&usr->board);
    setupStubScoreboardEndOfRound(&usr->enemyBoard);
    usr->enemyBoardInit = true;
    usr->phase = UserContext::Phase::IDLE;
    usr->turnOwner = UserContext::TurnOwner::PLAYER;
    usr->enemyAutoTimer = 0.0f;
    usr->enemyLaunched = false;
    usr->enemyDebugLogged = false;
    usr->enemyTurnSetup = false;
    usr->wereDead = 0;
    UI_ResetBannersForNewRoll(usr, "SECRET_END");
    ResultWindow_ClearPresentation(usr);
    ResultWindow_ResetRoundEarnings(usr);
    PhysicsResetForMode(usr, /*reviveAll=*/true);
    ResetAllElectroBalls(usr);
    if (usr->gameMode == UserContext::GameMode::BOT)
        Bot_RestorePresentationForMainGame(usr, /*resetCameraToPlayerIdle=*/true);
}

static inline void Cheats_ApplyUsernameCommands(UserContext *usr)
{
    if (!usr)
        return;

    std::cerr << "keypad news detect" << usr->username_len << std::endl;

    const bool isSb1 = (usr->username_len == 3 && std::memcmp(usr->username, "SB1", 3) == 0);
    if (isSb1)
    {
        setupStubScoreboardFinal(&usr->board);
        std::cerr << "seted up board stub" << std::endl;
    }

    if (Cheats_Matches3CaseInsensitive(usr, 'E', 'N', 'D'))
    {
        Cheats_ApplyEndOfRound(usr);
        std::cerr << "Secret cheat END applied: both scoreboards ready for frame 10" << std::endl;
    }

    if (Cheats_MatchesCaseInsensitive(usr, "CHEST"))
    {
        usr->cheatChestEveryFrame = true;
        usr->chestSpawnRollMadeThisThrow = false;
        usr->chestSpawnedThisThrow = false;
        usr->chestSpawnPlanned = false;
        std::cerr << "Secret cheat CHEST applied: chest every throw" << std::endl;
    }

    if (Cheats_MatchesCaseInsensitive(usr, "RUNES"))
    {
        for (int i = 0; i < kRuneKindCount; ++i)
            usr->runeCounts[i] = 1;
        RuneFab_MarkNeedsRebuild(usr);
        if (Runes_AreAllowedInCurrentMode(usr))
            RuneFab_RebuildSlots(usr);
        Progress_SaveUnlocksAndBank(usr);
        std::cerr << "Secret cheat RUNES applied: reset to one of each rune" << std::endl;
    }

    // School cheat codes: SC1..SC5 unlock/complete lessons.
    const bool isSc =
        (usr->username_len == 3 && std::memcmp(usr->username, "SC", 2) == 0 &&
         usr->username[2] >= '1' && usr->username[2] <= '5');
    if (isSc)
    {
        const int n = (int)(usr->username[2] - '0'); // 1..5
        for (int i = 0; i < 5; i++)
            usr->school.lessonDone[i] = (i < n);
        usr->school.unlockedLessons = glm::max(usr->school.unlockedLessons, glm::min(n + 1, 5));
        std::cerr << "School cheat: SC" << n << " applied" << std::endl;
    }

    bool isLevelJump = false;
    int jumpLevel = 0;
    if (usr->username_len >= 2 && usr->username[0] == 'L')
    {
        isLevelJump = true;
        for (int i = 1; i < usr->username_len; ++i)
        {
            if (usr->username[i] < '0' || usr->username[i] > '9')
            {
                isLevelJump = false;
                break;
            }
            jumpLevel = jumpLevel * 10 + int(usr->username[i] - '0');
        }
        if (jumpLevel < 1 || jumpLevel > kCampaignLevelCount)
            isLevelJump = false;
    }
    if (isLevelJump)
    {
        usr->campaignLevelIndex = jumpLevel;
        Campaign_SaveCurrentLevel(usr);
        Campaign_ApplyCurrentLevelSetup(usr, /*resetStoryKick=*/true);
        Run_ResetBoardsAndMode(usr, usr->gameMode);
        std::cerr << "Campaign jump: L" << jumpLevel << " applied" << std::endl;
    }
}
