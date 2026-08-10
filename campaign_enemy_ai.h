#pragma once

#include <stdint.h>
#include <algorithm>
#include <cmath>

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

struct CampaignEnemyAimChoice
{
    bool valid = false;
    bool usedSmartSplitHandling = false;
    uint16_t chosenClusterMask = 0u;
    glm::vec3 target = glm::vec3(0.0f);
};

static constexpr int CAMPAIGN_ENEMY_THROW_EXAMPLE_CAPACITY = 10;

struct CampaignEnemyThrowExample
{
    glm::vec3 movement = glm::vec3(0.0f);
    float spinSpeed = 0.0f;
    int score = 0;
    bool valid = false;
};

struct CampaignEnemyThrowExampleCatalog
{
    CampaignEnemyThrowExample examples[CAMPAIGN_ENEMY_THROW_EXAMPLE_CAPACITY] = {};
    int next = 0;
    int count = 0;
    glm::vec3 pendingMovement = glm::vec3(0.0f);
    float pendingSpinSpeed = 0.0f;
    bool pendingValid = false;
    bool currentDestroyedByRune = false;
};

struct CampaignEnemyProvenThrow
{
    float vx = 0.0f;
    float vy = 0.0f;
    float vz = -10.0f;
    float spinSpeed = 0.0f;
};

inline bool CampaignEnemyAiVec3Finite(const glm::vec3 &v)
{
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

inline bool CampaignEnemyAiSelectProvenFallbackThrow(
    float skill,
    uint32_t seed,
    glm::vec3 &outMovement,
    float &outSpin)
{
    static const CampaignEnemyProvenThrow kThrows[] = {
        { 0.00294958f, 2.01458f, -3.73625f, 0.0f},
        {-0.00178045f, 1.49604f, -4.18036f, 0.0f},
        {-0.00130654f, 2.01458f, -3.73625f, 0.0f},
    };
    const int count = int(sizeof(kThrows) / sizeof(kThrows[0]));
    if (count <= 0)
        return false;

    const float clampedSkill = std::isfinite(skill) ? glm::clamp(skill, 0.0f, 1.0f) : 0.0f;
    const int easyCount = glm::clamp(3 + int(clampedSkill * float(count - 3)), 1, count);
    const int idx = int(seed % uint32_t(easyCount));
    const CampaignEnemyProvenThrow &picked = kThrows[idx];
    outMovement = glm::vec3(picked.vx, picked.vy, picked.vz);
    outSpin = picked.spinSpeed;
    return CampaignEnemyAiVec3Finite(outMovement) && std::isfinite(outSpin);
}

inline void CampaignEnemyThrowCatalogStage(
    CampaignEnemyThrowExampleCatalog &catalog,
    const glm::vec3 &movement,
    float spinSpeed)
{
    if (!CampaignEnemyAiVec3Finite(movement) || !std::isfinite(spinSpeed))
        return;
    const float speed = glm::length(movement);
    if (!std::isfinite(speed) || speed <= 1e-4f)
        return;

    catalog.pendingMovement = movement;
    catalog.pendingSpinSpeed = spinSpeed;
    catalog.pendingValid = true;
    catalog.currentDestroyedByRune = false;
}

inline void CampaignEnemyThrowCatalogDiscardPending(CampaignEnemyThrowExampleCatalog &catalog)
{
    catalog.pendingMovement = glm::vec3(0.0f);
    catalog.pendingSpinSpeed = 0.0f;
    catalog.pendingValid = false;
}

inline void CampaignEnemyThrowCatalogMarkCurrentDestroyedByRune(CampaignEnemyThrowExampleCatalog &catalog)
{
    catalog.currentDestroyedByRune = true;
    CampaignEnemyThrowCatalogDiscardPending(catalog);
}

inline void CampaignEnemyThrowCatalogCommitScored(CampaignEnemyThrowExampleCatalog &catalog, int score)
{
    if (!catalog.pendingValid || catalog.currentDestroyedByRune)
    {
        CampaignEnemyThrowCatalogDiscardPending(catalog);
        catalog.currentDestroyedByRune = false;
        return;
    }

    const int slot = std::clamp(catalog.next, 0, CAMPAIGN_ENEMY_THROW_EXAMPLE_CAPACITY - 1);
    CampaignEnemyThrowExample &example = catalog.examples[slot];
    example.movement = catalog.pendingMovement;
    example.spinSpeed = catalog.pendingSpinSpeed;
    example.score = std::clamp(score, 0, 10);
    example.valid = true;

    catalog.next = (slot + 1) % CAMPAIGN_ENEMY_THROW_EXAMPLE_CAPACITY;
    catalog.count = std::min(catalog.count + 1, CAMPAIGN_ENEMY_THROW_EXAMPLE_CAPACITY);
    CampaignEnemyThrowCatalogDiscardPending(catalog);
    catalog.currentDestroyedByRune = false;
}

inline bool CampaignEnemyThrowCatalogSelect(
    const CampaignEnemyThrowExampleCatalog &catalog,
    int minScore,
    uint32_t seed,
    glm::vec3 &outMovement,
    float &outSpin)
{
    const int count = std::clamp(catalog.count, 0, CAMPAIGN_ENEMY_THROW_EXAMPLE_CAPACITY);
    if (count <= 0)
        return false;

    auto countEligible = [&](int requiredScore)
    {
        int eligible = 0;
        for (int i = 0; i < CAMPAIGN_ENEMY_THROW_EXAMPLE_CAPACITY; ++i)
        {
            const CampaignEnemyThrowExample &example = catalog.examples[i];
            if (example.valid && example.score >= requiredScore && CampaignEnemyAiVec3Finite(example.movement))
                ++eligible;
        }
        return eligible;
    };

    int requiredScore = std::max(0, minScore);
    int eligible = countEligible(requiredScore);
    if (eligible <= 0)
        return false;

    int pick = int(seed % uint32_t(eligible));
    for (int i = 0; i < CAMPAIGN_ENEMY_THROW_EXAMPLE_CAPACITY; ++i)
    {
        const CampaignEnemyThrowExample &example = catalog.examples[i];
        if (!example.valid || example.score < requiredScore || !CampaignEnemyAiVec3Finite(example.movement))
            continue;
        if (pick-- == 0)
        {
            outMovement = example.movement;
            outSpin = std::isfinite(example.spinSpeed) ? example.spinSpeed : 0.0f;
            return true;
        }
    }
    return false;
}

inline bool CampaignEnemyAiHasSmartSplitHandling(float skill)
{
    return std::isfinite(skill) && skill >= 0.5f;
}

inline uint16_t CampaignEnemyAiStandingMaskFromDead(const bool pinDead[10])
{
    uint16_t standingMask = 0u;
    if (!pinDead)
        return standingMask;
    for (int i = 0; i < 10; ++i)
    {
        if (!pinDead[i])
            standingMask |= uint16_t(1u << i);
    }
    return standingMask;
}

inline CampaignEnemyAimChoice CampaignEnemyAiChooseAimTarget(
    uint16_t standingMask, const glm::vec3 pins[10], float skill
)
{
    CampaignEnemyAimChoice out = {};
    if (pins == nullptr || standingMask == 0u)
        return out;

    static constexpr uint16_t kAdj[10] = {
        (1u << 1) | (1u << 2),
        (1u << 0) | (1u << 2) | (1u << 3) | (1u << 4),
        (1u << 0) | (1u << 1) | (1u << 4) | (1u << 5),
        (1u << 1) | (1u << 4) | (1u << 6) | (1u << 7),
        (1u << 1) | (1u << 2) | (1u << 3) | (1u << 5) | (1u << 7) | (1u << 8),
        (1u << 2) | (1u << 4) | (1u << 8) | (1u << 9),
        (1u << 3) | (1u << 7),
        (1u << 3) | (1u << 4) | (1u << 6) | (1u << 8),
        (1u << 4) | (1u << 5) | (1u << 7) | (1u << 9),
        (1u << 5) | (1u << 8),
    };

    struct Cluster
    {
        uint16_t mask = 0u;
        glm::vec3 center = glm::vec3(0.0f);
        int count = 0;
    };

    Cluster clusters[10] = {};
    int clusterCount = 0;
    uint16_t remaining = standingMask;
    while (remaining != 0u && clusterCount < 10)
    {
        int start = 0;
        while (start < 10 && ((remaining >> start) & 1u) == 0u)
            ++start;
        if (start >= 10)
            break;

        uint16_t frontier = uint16_t(1u << start);
        uint16_t clusterMask = 0u;
        remaining &= ~frontier;
        while (frontier != 0u)
        {
            int idx = 0;
            while (idx < 10 && ((frontier >> idx) & 1u) == 0u)
                ++idx;
            if (idx >= 10)
                break;
            const uint16_t bit = uint16_t(1u << idx);
            frontier &= ~bit;
            clusterMask |= bit;
            const uint16_t connected = kAdj[idx] & remaining;
            frontier |= connected;
            remaining &= ~connected;
        }

        Cluster &cluster = clusters[clusterCount++];
        cluster.mask = clusterMask;
        for (int i = 0; i < 10; ++i)
        {
            if (((clusterMask >> i) & 1u) == 0u)
                continue;
            cluster.center += pins[i];
            cluster.count++;
        }
        if (cluster.count > 0)
            cluster.center /= float(cluster.count);
    }

    if (clusterCount <= 0)
        return out;

    int chosenClusterIndex = 0;
    if (clusterCount > 1 && CampaignEnemyAiHasSmartSplitHandling(skill))
    {
        out.usedSmartSplitHandling = true;
        for (int i = 1; i < clusterCount; ++i)
        {
            const Cluster &best = clusters[chosenClusterIndex];
            const Cluster &candidate = clusters[i];
            if (candidate.count > best.count)
            {
                chosenClusterIndex = i;
                continue;
            }
            if (candidate.count < best.count)
                continue;

            const float bestAbsX = std::fabs(best.center.x);
            const float candidateAbsX = std::fabs(candidate.center.x);
            if (candidateAbsX + 1e-4f < bestAbsX)
            {
                chosenClusterIndex = i;
                continue;
            }
            if (std::fabs(candidateAbsX - bestAbsX) <= 1e-4f && candidate.center.z < best.center.z)
                chosenClusterIndex = i;
        }
    }
    else
    {
        glm::vec3 sum(0.0f);
        int count = 0;
        for (int i = 0; i < 10; ++i)
        {
            if (((standingMask >> i) & 1u) == 0u)
                continue;
            sum += pins[i];
            count++;
        }
        if (count <= 0)
            return out;
        out.valid = true;
        out.chosenClusterMask = standingMask;
        out.target = sum / float(count);
        return out;
    }

    out.valid = true;
    out.chosenClusterMask = clusters[chosenClusterIndex].mask;
    out.target = clusters[chosenClusterIndex].center;
    return out;
}

inline float CampaignEnemyAiComputeSpinCorrection(
    const glm::vec3 &ballPos, const glm::vec3 &velocity, const glm::vec3 &target, float skill
)
{
    if (!std::isfinite(skill) || skill <= 0.0f)
        return 0.0f;

    glm::vec2 curDir(velocity.x, velocity.z);
    const float curLen = glm::length(curDir);
    if (!std::isfinite(curLen) || curLen <= 1e-4f)
        return 0.0f;
    curDir /= curLen;

    glm::vec2 desiredDir(target.x - ballPos.x, target.z - ballPos.z);
    const float desiredLen = glm::length(desiredDir);
    if (!std::isfinite(desiredLen) || desiredLen <= 1e-4f)
        return 0.0f;
    desiredDir /= desiredLen;

    const float lateralError = desiredDir.x - curDir.x;
    const float misalignment = glm::clamp(1.0f - glm::dot(curDir, desiredDir), 0.0f, 2.0f);
    const float maxSpin = glm::mix(0.0f, 1.65f, glm::clamp(skill, 0.0f, 1.0f));
    const float spin = lateralError * (1.3f + misalignment * 2.5f) * maxSpin;
    return glm::clamp(spin, -maxSpin, maxSpin);
}

inline bool CampaignEnemyAiShouldCommitNos(
    const glm::vec3 &ballPos, const glm::vec3 &velocity, const glm::vec3 &target, float skill
)
{
    glm::vec2 curDir(velocity.x, velocity.z);
    const float curLen = glm::length(curDir);
    if (!std::isfinite(curLen) || curLen <= 1e-4f)
        return false;
    curDir /= curLen;

    glm::vec2 desiredDir(target.x - ballPos.x, target.z - ballPos.z);
    const float desiredLen = glm::length(desiredDir);
    if (!std::isfinite(desiredLen) || desiredLen <= 1e-4f)
        return false;
    desiredDir /= desiredLen;

    const float alignThreshold = glm::mix(0.985f, 0.93f, glm::clamp(skill, 0.0f, 1.0f));
    return glm::dot(curDir, desiredDir) >= alignThreshold;
}
