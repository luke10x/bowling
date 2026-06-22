#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <random>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>

struct BlockConfiguration
{
    const char *name;
    int fragmentCount;
    float jitter;
    float breakSpeed;
    float thickness;
    float totalMass;
    float friction;
    float restitution;
    glm::vec2 atlasStart;
    glm::vec2 tileSize;
    float atlasScale;
    glm::vec3 textureScaling;
    bool anchorToWorldWhenIntact;
    bool usesTransparency;
};

struct FracturedBlockFragmentGeometry
{
    std::vector<glm::vec2> frontFace;
    glm::vec3 localOffset = glm::vec3(0.0f);
};

struct FracturedBlockSettings
{
    glm::vec3 center = glm::vec3(0.0f, 0.25f, -8.715f);
    float width = 1.06f;
    float height = 0.5f;
    float thickness = 0.1f;
    int fragmentCount = 8;
    float jitter = 0.18f;
    float breakSpeed = 8.5f;
    float totalMass = 6.0f;
    float friction = 0.6f;
    float restitution = 0.32f;
    bool anchorToWorldWhenIntact = false;
    uint32_t randomSeed = 1;
    int variantIndex = 0;
};

inline const std::array<BlockConfiguration, 4> &Block_GetBlockConfigurations()
{
    constexpr float c = 1.0f / 8.0f;
    static const std::array<BlockConfiguration, 4> kConfigs = {{
        {"wood",      7, 0.18f, 3.0f, 0.07f, 2.0f, 0.45f, 0.24f, glm::vec2(1.0f + 3.0f * c, 1.0f + 4.0f * c), glm::vec2(c, c), c, glm::vec3(8.0f, 8.0f, 8.0f), false, false},
        {"brick",    10, 0.14f, 4.0f, 0.10f, 4.0f, 0.75f, 0.18f, glm::vec2(1.0f + 3.0f * c, 1.0f + 5.0f * c), glm::vec2(c, c), c, glm::vec3(8.0f, 8.0f, 8.0f), false, false},
        {"concrete", 14, 0.10f, 5.2f, 0.14f, 6.5f, 0.95f, 0.10f, glm::vec2(1.0f + 3.0f * c, 1.0f + 6.0f * c), glm::vec2(c, c), c, glm::vec3(8.0f, 8.0f, 8.0f), false, false},
        {"glass",    12, 0.24f, 5.8f, 0.01f, 1.0f, 0.15f, 0.55f, glm::vec2(1.0f + 3.0f * c, 1.0f + 7.0f * c), glm::vec2(c, c), c, glm::vec3(8.0f, 8.0f, 8.0f), true, true},
    }};
    return kConfigs;
}

inline FracturedBlockSettings Block_MakeCenteredPlacementSettings(int configIndex, uint32_t randomSeed)
{
    const auto &configs = Block_GetBlockConfigurations();
    const int wrappedIndex = ((configIndex % int(configs.size())) + int(configs.size())) % int(configs.size());
    const BlockConfiguration &config = configs[size_t(wrappedIndex)];
    FracturedBlockSettings settings;
    settings.center = glm::vec3(0.0f, 0.25f, (-18.3f + 0.87f) * 0.5f);
    settings.width = 1.06f;
    settings.height = 0.5f;
    settings.thickness = config.thickness;
    settings.fragmentCount = config.fragmentCount;
    settings.jitter = config.jitter;
    settings.breakSpeed = config.breakSpeed;
    settings.totalMass = config.totalMass;
    settings.friction = config.friction;
    settings.restitution = config.restitution;
    settings.anchorToWorldWhenIntact = config.anchorToWorldWhenIntact;
    settings.randomSeed = randomSeed;
    settings.variantIndex = wrappedIndex;
    return settings;
}

inline float Block_PolygonSignedArea(const std::vector<glm::vec2> &poly)
{
    float twiceArea = 0.0f;
    for (size_t i = 0; i < poly.size(); ++i)
    {
        const glm::vec2 &a = poly[i];
        const glm::vec2 &b = poly[(i + 1) % poly.size()];
        twiceArea += a.x * b.y - b.x * a.y;
    }
    return 0.5f * twiceArea;
}

inline glm::vec2 Block_PolygonCentroid(const std::vector<glm::vec2> &poly)
{
    const float area = Block_PolygonSignedArea(poly);
    if (std::abs(area) < 1.0e-6f)
    {
        glm::vec2 sum(0.0f);
        for (const glm::vec2 &p : poly)
            sum += p;
        return poly.empty() ? glm::vec2(0.0f) : sum / float(poly.size());
    }

    glm::vec2 centroid(0.0f);
    for (size_t i = 0; i < poly.size(); ++i)
    {
        const glm::vec2 &a = poly[i];
        const glm::vec2 &b = poly[(i + 1) % poly.size()];
        const float cross = a.x * b.y - b.x * a.y;
        centroid += (a + b) * cross;
    }
    return centroid / (6.0f * area);
}

inline std::vector<glm::vec2> Block_ClipPolygonToHalfPlane(
    const std::vector<glm::vec2> &poly,
    const glm::vec2 &planePoint,
    const glm::vec2 &planeNormal
)
{
    std::vector<glm::vec2> out;
    if (poly.empty())
        return out;

    auto signedDistance = [&](const glm::vec2 &p) -> float
    {
        return glm::dot(p - planePoint, planeNormal);
    };

    const float eps = 1.0e-5f;
    for (size_t i = 0; i < poly.size(); ++i)
    {
        const glm::vec2 current = poly[i];
        const glm::vec2 next = poly[(i + 1) % poly.size()];
        const float d0 = signedDistance(current);
        const float d1 = signedDistance(next);
        const bool inside0 = d0 <= eps;
        const bool inside1 = d1 <= eps;

        if (inside0 && inside1)
        {
            out.push_back(next);
        }
        else if (inside0 && !inside1)
        {
            const float t = d0 / (d0 - d1);
            out.push_back(current + t * (next - current));
        }
        else if (!inside0 && inside1)
        {
            const float t = d0 / (d0 - d1);
            out.push_back(current + t * (next - current));
            out.push_back(next);
        }
    }

    return out;
}

inline std::vector<glm::vec2> Block_GenerateVoronoiSites(
    int fragmentCount, float width, float height, float jitter, uint32_t seed
)
{
    std::mt19937 rng(seed == 0 ? 1u : seed);
    const float aspect = (height > 1.0e-5f) ? (width / height) : 1.0f;
    const int cols = std::max(1, int(std::ceil(std::sqrt(float(fragmentCount) * aspect))));
    const int rows = std::max(1, int(std::ceil(float(fragmentCount) / float(cols))));
    const float cellW = width / float(cols);
    const float cellH = height / float(rows);
    const float jitter01 = glm::clamp(jitter, 0.0f, 1.0f);
    const float jitterX = 0.5f * cellW * jitter01;
    const float jitterY = 0.5f * cellH * jitter01;
    std::uniform_real_distribution<float> unit(-1.0f, 1.0f);

    std::vector<glm::vec2> sites;
    sites.reserve(fragmentCount);
    for (int i = 0; i < fragmentCount; ++i)
    {
        const int col = i % cols;
        const int row = i / cols;
        glm::vec2 site(
            -0.5f * width + (float(col) + 0.5f) * cellW,
            -0.5f * height + (float(row) + 0.5f) * cellH
        );
        site.x += unit(rng) * jitterX;
        site.y += unit(rng) * jitterY;
        site.x = glm::clamp(site.x, -0.5f * width, 0.5f * width);
        site.y = glm::clamp(site.y, -0.5f * height, 0.5f * height);
        sites.push_back(site);
    }
    return sites;
}

inline std::vector<FracturedBlockFragmentGeometry> Block_GenerateVoronoiFragments(
    const FracturedBlockSettings &settings
)
{
    const int fragmentCount = std::clamp(settings.fragmentCount, 5, 15);
    const float width = std::max(0.05f, settings.width);
    const float height = std::max(0.05f, settings.height);
    const std::vector<glm::vec2> sites =
        Block_GenerateVoronoiSites(fragmentCount, width, height, settings.jitter, settings.randomSeed);

    const std::vector<glm::vec2> bounds = {
        glm::vec2(-0.5f * width, -0.5f * height),
        glm::vec2(0.5f * width, -0.5f * height),
        glm::vec2(0.5f * width, 0.5f * height),
        glm::vec2(-0.5f * width, 0.5f * height),
    };

    std::vector<FracturedBlockFragmentGeometry> fragments;
    fragments.reserve(sites.size());

    for (size_t i = 0; i < sites.size(); ++i)
    {
        std::vector<glm::vec2> cell = bounds;
        for (size_t j = 0; j < sites.size() && cell.size() >= 3; ++j)
        {
            if (i == j)
                continue;
            const glm::vec2 mid = 0.5f * (sites[i] + sites[j]);
            const glm::vec2 normal = sites[j] - sites[i];
            if (glm::length2(normal) < 1.0e-8f)
                continue;
            cell = Block_ClipPolygonToHalfPlane(cell, mid, normal);
        }

        const float area = std::abs(Block_PolygonSignedArea(cell));
        if (cell.size() < 3 || area <= 1.0e-5f)
            continue;

        if (Block_PolygonSignedArea(cell) < 0.0f)
            std::reverse(cell.begin(), cell.end());

        const glm::vec2 centroid = Block_PolygonCentroid(cell);
        FracturedBlockFragmentGeometry geom;
        geom.frontFace.reserve(cell.size());
        for (const glm::vec2 &p : cell)
            geom.frontFace.push_back(p - centroid);
        geom.localOffset = glm::vec3(centroid.x, centroid.y, 0.0f);
        fragments.push_back(std::move(geom));
    }

    return fragments;
}
