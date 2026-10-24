#pragma once

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>

struct CampaignSequelGame
{
    int score = 0;
    int opponentScore = 0;
    int64_t finishedAt = 0;
    unsigned char settings = 7; // N, B, R in bits 0, 1, 2.
};

struct CampaignSequelList
{
    CampaignSequelGame games[10] = {};
    int count = 0;
};

struct CampaignSequelHistory
{
    CampaignSequelList recent;
    CampaignSequelList top[8];
};

inline unsigned char CampaignSequel_Settings(bool nos, bool blocks, bool runes)
{
    return (nos ? 1 : 0) | (blocks ? 2 : 0) | (runes ? 4 : 0);
}

inline void CampaignSequel_Record(CampaignSequelHistory &history, CampaignSequelGame game)
{
    game.settings &= 7;
    for (int i = std::min(history.recent.count, 9); i > 0; --i)
        history.recent.games[i] = history.recent.games[i - 1];
    history.recent.games[0] = game;
    history.recent.count = std::min(history.recent.count + 1, 10);

    CampaignSequelList &top = history.top[game.settings];
    int place = 0;
    while (place < top.count && top.games[place].score >= game.score)
        ++place;
    if (place >= 5)
        return;
    for (int i = std::min(top.count, 4); i > place; --i)
        top.games[i] = top.games[i - 1];
    top.games[place] = game;
    top.count = std::min(top.count + 1, 5);
}

inline CampaignSequelList CampaignSequel_Top(const CampaignSequelHistory &history, bool currentOnly, unsigned char settings)
{
    if (currentOnly)
        return history.top[settings & 7];
    CampaignSequelList result;
    for (const CampaignSequelList &list : history.top)
        for (int i = 0; i < list.count; ++i)
        {
            const CampaignSequelGame game = list.games[i];
            int place = 0;
            while (place < result.count && result.games[place].score >= game.score)
                ++place;
            if (place >= 5)
                continue;
            for (int j = std::min(result.count, 4); j > place; --j)
                result.games[j] = result.games[j - 1];
            result.games[place] = game;
            result.count = std::min(result.count + 1, 5);
        }
    return result;
}

inline std::string CampaignSequel_Encode(const CampaignSequelList &list)
{
    std::string out;
    for (int i = 0; i < list.count; ++i)
    {
        char item[96];
        const CampaignSequelGame &game = list.games[i];
        std::snprintf(item, sizeof(item), "%s%d,%d,%lld,%u", i ? ";" : "", game.score,
                      game.opponentScore, (long long)game.finishedAt, (unsigned)game.settings);
        out += item;
    }
    return out;
}

inline CampaignSequelList CampaignSequel_Decode(const std::string &encoded, int limit = 5)
{
    CampaignSequelList list;
    size_t pos = 0;
    while (pos < encoded.size() && list.count < std::min(limit, 10))
    {
        size_t end = encoded.find(';', pos);
        const std::string item = encoded.substr(pos, end == std::string::npos ? end : end - pos);
        CampaignSequelGame game;
        long long timestamp = 0;
        unsigned settings = 0;
        if (std::sscanf(item.c_str(), "%d,%d,%lld,%u", &game.score,
                        &game.opponentScore, &timestamp, &settings) != 4 ||
            game.score < 0 || game.score > 100000 || game.opponentScore < 0 ||
            game.opponentScore > 100000 || settings > 7 || timestamp < 0)
            break;
        game.finishedAt = timestamp;
        game.settings = (unsigned char)settings;
        list.games[list.count++] = game;
        if (end == std::string::npos)
            break;
        pos = end + 1;
    }
    return list;
}

inline void CampaignSequel_ChartRange(const CampaignSequelList &recent, int &floor, int &ceiling)
{
    if (!recent.count)
    {
        floor = 0;
        ceiling = 100;
        return;
    }
    int lo = recent.games[0].score, hi = lo;
    for (int i = 1; i < recent.count; ++i)
    {
        lo = std::min(lo, recent.games[i].score);
        hi = std::max(hi, recent.games[i].score);
    }
    const int padding = std::max(5, (hi - lo + 4) / 5);
    floor = std::max(0, ((lo - padding) / 10) * 10);
    ceiling = ((hi + padding + 9) / 10) * 10;
    if (ceiling <= floor)
        ceiling = floor + 10;
}
