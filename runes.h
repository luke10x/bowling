#pragma once

#include <cstdint>

enum class RuneStage : uint8_t
{
    Offense = 1u << 0,
    Defense = 1u << 1,
    Both = Offense | Defense,
};

struct RuneAvailabilityConfig
{
    int runeIndex;
    RuneStage enabledStages;
};

static constexpr RuneAvailabilityConfig kRuneAvailabilityByKind[] = {
    {0, RuneStage::Offense}, // Boom
    {1, RuneStage::Defense}, // Bolt
    {2, RuneStage::Defense}, // Freeze
    {3, RuneStage::Offense}, // Skull
    {4, RuneStage::Defense}, // Guard Pins
};

static inline bool RuneStage_Allows(RuneStage allowedStages, RuneStage currentStage)
{
    return (((uint8_t)allowedStages) & ((uint8_t)currentStage)) != 0u;
}

static inline RuneStage Rune_EnabledStagesForIndex(int runeIndex)
{
    for (const RuneAvailabilityConfig &cfg : kRuneAvailabilityByKind)
    {
        if (cfg.runeIndex == runeIndex)
            return cfg.enabledStages;
    }
    return RuneStage::Offense;
}

static inline bool Rune_IsEnabledForStage(int runeIndex, RuneStage currentStage)
{
    return RuneStage_Allows(Rune_EnabledStagesForIndex(runeIndex), currentStage);
}
