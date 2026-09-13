#pragma once

#include <cstring>
#include <iostream>

#include "stubs.h"

static inline bool Cheats_TextMatches3CaseInsensitive(const char *text, int32_t textLen, char a, char b, char c)
{
    if (!text || textLen != 3)
        return false;
    const char ua = text[0];
    const char ub = text[1];
    const char uc = text[2];
    return (ua == a || ua == char(a - 'A' + 'a')) &&
           (ub == b || ub == char(b - 'A' + 'a')) &&
           (uc == c || uc == char(c - 'A' + 'a'));
}

static inline bool Cheats_TextMatchesCaseInsensitive(const char *text, int32_t textLen, const char *code)
{
    if (!text || !code)
        return false;
    const int len = (int)std::strlen(code);
    if (textLen != len)
        return false;
    for (int i = 0; i < len; ++i)
    {
        char expected = code[i];
        char got = text[i];
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

static inline bool Cheats_Matches3CaseInsensitive(const UserContext *usr, char a, char b, char c)
{
    return usr && Cheats_TextMatches3CaseInsensitive(usr->username, usr->username_len, a, b, c);
}

static inline bool Cheats_MatchesCaseInsensitive(const UserContext *usr, const char *code)
{
    return usr && Cheats_TextMatchesCaseInsensitive(usr->username, usr->username_len, code);
}

static inline bool Cheats_ShouldShowImgui(const UserContext *usr)
{
    return usr && (usr->cheatImguiUnlocked || Cheats_MatchesCaseInsensitive(usr, "GUGUCAS"));
}

static inline bool Cheats_ShouldUnlockMinigames(const UserContext *usr)
{
    return usr && (usr->cheatMinigamesUnlocked || Cheats_MatchesCaseInsensitive(usr, "GAMES"));
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

static inline bool Cheats_ApplyCode(UserContext *usr, const char *code, int32_t codeLen)
{
    if (!usr)
        return false;

    std::cerr << "keypad news detect" << codeLen << std::endl;

    bool activated = false;

    const bool isSb1 = Cheats_TextMatchesCaseInsensitive(code, codeLen, "SB1");
    if (isSb1)
    {
        setupStubScoreboardFinal(&usr->board);
        activated = true;
        std::cerr << "seted up board stub" << std::endl;
    }

    if (Cheats_TextMatches3CaseInsensitive(code, codeLen, 'E', 'N', 'D'))
    {
        Cheats_ApplyEndOfRound(usr);
        activated = true;
        std::cerr << "Secret cheat END applied: both scoreboards ready for frame 10" << std::endl;
    }

    if (Cheats_TextMatchesCaseInsensitive(code, codeLen, "CHEST"))
    {
        usr->cheatChestEveryFrame = true;
        usr->chestSpawnRollMadeThisThrow = false;
        usr->chestSpawnedThisThrow = false;
        usr->chestSpawnPlanned = false;
        activated = true;
        std::cerr << "Secret cheat CHEST applied: chest every throw" << std::endl;
    }

    if (Cheats_TextMatchesCaseInsensitive(code, codeLen, "RUNES"))
    {
        for (int i = 0; i < kRuneKindCount; ++i)
            usr->runeCounts[i] = 1;
        RuneFab_MarkNeedsRebuild(usr);
        if (Runes_AreAllowedInCurrentMode(usr))
            RuneFab_RebuildSlots(usr);
        Progress_SaveUnlocksAndBank(usr);
        activated = true;
        std::cerr << "Secret cheat RUNES applied: reset to one of each rune" << std::endl;
    }

    if (Cheats_TextMatchesCaseInsensitive(code, codeLen, "GUGUCAS"))
    {
        usr->cheatImguiUnlocked = true;
        activated = true;
        std::cerr << "Secret cheat GUGUCAS applied: imgui enabled" << std::endl;
    }

    if (Cheats_TextMatchesCaseInsensitive(code, codeLen, "GAMES"))
    {
        usr->cheatMinigamesUnlocked = true;
        activated = true;
        std::cerr << "Secret cheat GAMES applied: minigames unlocked" << std::endl;
    }

    // School cheat codes: SC1..SC5 unlock/complete lessons.
    const bool isSc =
        (code && codeLen == 3 &&
         (code[0] == 'S' || code[0] == 's') &&
         (code[1] == 'C' || code[1] == 'c') &&
         code[2] >= '1' && code[2] <= '5');
    if (isSc)
    {
        const int n = (int)(code[2] - '0'); // 1..5
        for (int i = 0; i < 5; i++)
            usr->school.lessonDone[i] = (i < n);
        usr->school.unlockedLessons = glm::max(usr->school.unlockedLessons, glm::min(n + 1, 5));
        activated = true;
        std::cerr << "School cheat: SC" << n << " applied" << std::endl;
    }

    bool isLevelJump = false;
    int jumpLevel = 0;
    if (code && codeLen >= 2 && (code[0] == 'L' || code[0] == 'l'))
    {
        isLevelJump = true;
        for (int i = 1; i < codeLen; ++i)
        {
            if (code[i] < '0' || code[i] > '9')
            {
                isLevelJump = false;
                break;
            }
            jumpLevel = jumpLevel * 10 + int(code[i] - '0');
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
        activated = true;
        std::cerr << "Campaign jump: L" << jumpLevel << " applied" << std::endl;
    }

    return activated;
}

static inline bool Cheats_ApplyUsernameCommands(UserContext *usr)
{
    if (!usr)
        return false;
    return Cheats_ApplyCode(usr, usr->username, usr->username_len);
}
