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
