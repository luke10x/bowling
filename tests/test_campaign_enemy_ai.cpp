#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../3rdparty/json/tests/thirdparty/doctest/doctest.h"

#include "../campaign_enemy_ai.h"

TEST_CASE("Enemy split handling level 0 keeps aiming at the whole leave")
{
    glm::vec3 pins[10] = {};
    pins[3] = glm::vec3(-2.0f, 0.0f, -12.0f);
    pins[6] = glm::vec3(-1.2f, 0.0f, -13.0f);
    pins[5] = glm::vec3(2.0f, 0.0f, -12.0f);

    const uint16_t standingMask = uint16_t((1u << 3) | (1u << 5) | (1u << 6));
    const CampaignEnemyAimChoice choice = CampaignEnemyAiChooseAimTarget(standingMask, pins, 0.0f);

    REQUIRE(choice.valid);
    CHECK(choice.usedSmartSplitHandling == false);
    CHECK(choice.target.x == doctest::Approx((-2.0f - 1.2f + 2.0f) / 3.0f));
}

TEST_CASE("Enemy split handling level 1 chooses the bigger cluster")
{
    glm::vec3 pins[10] = {};
    pins[3] = glm::vec3(-2.0f, 0.0f, -12.0f);
    pins[6] = glm::vec3(-1.2f, 0.0f, -13.0f);
    pins[5] = glm::vec3(2.0f, 0.0f, -12.0f);

    const uint16_t standingMask = uint16_t((1u << 3) | (1u << 5) | (1u << 6));
    const CampaignEnemyAimChoice choice = CampaignEnemyAiChooseAimTarget(standingMask, pins, 1.0f);

    REQUIRE(choice.valid);
    CHECK(choice.usedSmartSplitHandling == true);
    CHECK(choice.chosenClusterMask == uint16_t((1u << 3) | (1u << 6)));
    CHECK(choice.target.x == doctest::Approx((-2.0f - 1.2f) / 2.0f));
}

TEST_CASE("Enemy split handling tiebreak prefers the cluster closer to the middle")
{
    glm::vec3 pins[10] = {};
    pins[3] = glm::vec3(-2.5f, 0.0f, -12.0f);
    pins[5] = glm::vec3(0.6f, 0.0f, -12.0f);

    const uint16_t standingMask = uint16_t((1u << 3) | (1u << 5));
    const CampaignEnemyAimChoice choice = CampaignEnemyAiChooseAimTarget(standingMask, pins, 1.0f);

    REQUIRE(choice.valid);
    CHECK(choice.usedSmartSplitHandling == true);
    CHECK(choice.chosenClusterMask == uint16_t(1u << 5));
    CHECK(choice.target.x == doctest::Approx(0.6f));
}

TEST_CASE("Enemy spin correction steers toward the target only when skilled")
{
    const glm::vec3 ballPos(0.0f, 0.0f, 0.0f);
    const glm::vec3 vel(0.0f, 0.0f, -10.0f);
    const glm::vec3 target(1.0f, 0.0f, -8.0f);

    CHECK(CampaignEnemyAiComputeSpinCorrection(ballPos, vel, target, 0.0f) == doctest::Approx(0.0f));
    CHECK(CampaignEnemyAiComputeSpinCorrection(ballPos, vel, target, 1.0f) > 0.0f);
    CHECK(CampaignEnemyAiShouldCommitNos(ballPos, vel, glm::vec3(0.0f, 0.0f, -8.0f), 1.0f) == true);
}

TEST_CASE("Enemy precision curve gives elite skill a stronger aim boost")
{
    CHECK(CampaignEnemyAiEffectivePrecision(0.32f) == doctest::Approx(0.32f));
    CHECK(CampaignEnemyAiEffectivePrecision(0.74f) > 0.74f);
    CHECK(CampaignEnemyAiEffectivePrecision(0.98f) > 0.99f);

    const glm::vec3 ballPos(0.0f, 0.0f, 0.0f);
    const glm::vec3 vel(0.4f, 0.0f, -10.0f);
    const glm::vec3 target(1.4f, 0.0f, -8.0f);
    CHECK(std::fabs(CampaignEnemyAiComputeSpinCorrection(ballPos, vel, target, 0.98f)) >
          std::fabs(CampaignEnemyAiComputeSpinCorrection(ballPos, vel, target, 0.74f)));
}

TEST_CASE("Enemy throw catalog keeps only the last ten scored examples")
{
    CampaignEnemyThrowExampleCatalog catalog = {};
    for (int i = 0; i < 12; ++i)
    {
        CampaignEnemyThrowCatalogStage(catalog, glm::vec3(float(i + 1), 0.0f, 8.0f), 0.25f * float(i));
        CampaignEnemyThrowCatalogCommitScored(catalog, i % 10);
    }

    CHECK(catalog.count == CAMPAIGN_ENEMY_THROW_EXAMPLE_CAPACITY);
    CHECK(catalog.next == 2);
    CHECK(catalog.examples[0].movement.x == doctest::Approx(11.0f));
    CHECK(catalog.examples[1].movement.x == doctest::Approx(12.0f));
}

TEST_CASE("Enemy throw catalog filters weak examples when possible")
{
    CampaignEnemyThrowExampleCatalog catalog = {};
    CampaignEnemyThrowCatalogStage(catalog, glm::vec3(1.0f, 0.0f, 8.0f), 0.1f);
    CampaignEnemyThrowCatalogCommitScored(catalog, 0);
    CampaignEnemyThrowCatalogStage(catalog, glm::vec3(2.0f, 0.0f, 8.0f), 0.2f);
    CampaignEnemyThrowCatalogCommitScored(catalog, 3);

    glm::vec3 movement(0.0f);
    float spin = 0.0f;
    REQUIRE(CampaignEnemyThrowCatalogSelect(catalog, 1, 0u, movement, spin));
    CHECK(movement.x == doctest::Approx(2.0f));
    CHECK(spin == doctest::Approx(0.2f));
}

TEST_CASE("Enemy throw catalog rejects weak examples when a minimum score is required")
{
    CampaignEnemyThrowExampleCatalog catalog = {};
    CampaignEnemyThrowCatalogStage(catalog, glm::vec3(1.0f, 0.0f, 8.0f), 0.1f);
    CampaignEnemyThrowCatalogCommitScored(catalog, 0);

    glm::vec3 movement(0.0f);
    float spin = 0.0f;
    CHECK(CampaignEnemyThrowCatalogSelect(catalog, 1, 0u, movement, spin) == false);
}

TEST_CASE("Enemy throw catalog rejects examples that do not move down the lane")
{
    CampaignEnemyThrowExampleCatalog catalog = {};
    CampaignEnemyThrowCatalogStage(catalog, glm::vec3(2.0f, 0.0f, -8.0f), 0.1f);
    CampaignEnemyThrowCatalogCommitScored(catalog, 10);
    CampaignEnemyThrowCatalogStage(catalog, glm::vec3(0.5f, 0.0f, 0.5f), 0.1f);
    CampaignEnemyThrowCatalogCommitScored(catalog, 10);

    glm::vec3 movement(0.0f);
    float spin = 0.0f;
    CHECK(CampaignEnemyThrowCatalogSelect(catalog, 0, 0u, movement, spin) == false);
}

TEST_CASE("Enemy throw catalog rejects examples with implausible vertical launch")
{
    CampaignEnemyThrowExampleCatalog catalog = {};
    CampaignEnemyThrowCatalogStage(catalog, glm::vec3(0.0f, 8.0f, 8.0f), 0.1f);
    CampaignEnemyThrowCatalogCommitScored(catalog, 10);

    glm::vec3 movement(0.0f);
    float spin = 0.0f;
    CHECK(CampaignEnemyThrowCatalogSelect(catalog, 0, 0u, movement, spin) == false);
}

TEST_CASE("Enemy proven fallback throws are finite and roll toward enemy pins")
{
    for (uint32_t seed = 0; seed < 16; ++seed)
    {
        glm::vec3 movement(0.0f);
        float spin = 0.0f;
        REQUIRE(CampaignEnemyAiSelectProvenFallbackThrow(1.0f, seed, movement, spin));
        CHECK(CampaignEnemyAiVec3Finite(movement));
        CHECK(CampaignEnemyThrowMovementUsableForLane(movement, -1.0f));
        CHECK(std::isfinite(spin));
        CHECK(movement.y >= 0.0f);
        CHECK(movement.z < 0.0f);
    }
}

TEST_CASE("Enemy throw catalog excludes rune-destroyed pending examples")
{
    CampaignEnemyThrowExampleCatalog catalog = {};
    CampaignEnemyThrowCatalogStage(catalog, glm::vec3(1.0f, 0.0f, 8.0f), 0.1f);
    CampaignEnemyThrowCatalogMarkCurrentDestroyedByRune(catalog);
    CampaignEnemyThrowCatalogCommitScored(catalog, 10);

    glm::vec3 movement(0.0f);
    float spin = 0.0f;
    CHECK(CampaignEnemyThrowCatalogSelect(catalog, 0, 0u, movement, spin) == false);
}
