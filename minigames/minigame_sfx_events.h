#pragma once

#include <array>
#include <cstdint>

#include <glm/glm.hpp>

enum class MiniGameSfxEvent : uint8_t
{
    FIGHT_START = 0,
    ANGEL_DIED = 1,
    ENEMY_DIED = 2,
    BOSS_SPAWNED = 3,
    POWER_UPGRADE_CONSUMED = 4,
    POWER_UPGRADE_MISSED = 5,
};

template <int Capacity>
struct MiniGameSfxEventQueue
{
    std::array<MiniGameSfxEvent, Capacity> events{};
    int count = 0;
    int dropped = 0;

    void clear()
    {
        count = 0;
        dropped = 0;
    }

    void push(MiniGameSfxEvent event)
    {
        if (count < Capacity)
        {
            events[count++] = event;
            return;
        }
        ++dropped;
    }
};

enum class MiniGameParticleEventKind : uint8_t
{
    FIGHT_CONTACT = 0,
    UPGRADE_HIT = 1,
    UPGRADE_CONSUMED = 2,
    ANGEL_DIED = 3,
    ENEMY_DIED = 4,
    POWER_UPGRADE_CONSUMED = 5,
    BOSS_SMASH = 6,
};

struct MiniGameParticleEvent
{
    MiniGameParticleEventKind kind = MiniGameParticleEventKind::FIGHT_CONTACT;
    glm::vec2 pos = glm::vec2(0.0f);
    glm::vec2 dir = glm::vec2(0.0f, 1.0f);
    float intensity = 0.5f;
};

template <int Capacity>
struct MiniGameParticleEventQueue
{
    std::array<MiniGameParticleEvent, Capacity> events{};
    int count = 0;
    int dropped = 0;

    void clear()
    {
        count = 0;
        dropped = 0;
    }

    void push(MiniGameParticleEvent event)
    {
        if (count < Capacity)
        {
            events[count++] = event;
            return;
        }
        ++dropped;
    }
};
