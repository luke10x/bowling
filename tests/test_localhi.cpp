#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../3rdparty/json/tests/thirdparty/doctest/doctest.h"

#include "../localhi.h"

// -----------------------------------------------------------------------------
// Helper: advance time by faking timestamps (since we can't mock time())
// We'll directly manipulate entry timestamps for expiry tests.
// -----------------------------------------------------------------------------

TEST_CASE("LocalHi_Init initializes all fields to zero/defaults")
{
    LocalHighscore hi;
    LocalHi_Init(&hi);

    CHECK_EQ(hi.count, 0);
    CHECK_EQ(hi.lastSubmitResult, LOCALHI_SUBMIT_NONE);
    CHECK_EQ(hi.lastSubmittedScore, 0);
    CHECK_EQ(hi.lastSubmittedRank, 0);
    CHECK_EQ(hi.lastSubmittedPercentile, 0.0f);
    CHECK_EQ(hi.percentileTracker.totalAttempts, 0);
    CHECK_EQ(hi.percentileTracker.minScore, LOCALHI_MAX_SCORE);
    CHECK_EQ(hi.percentileTracker.maxScore, 0);
}

TEST_CASE("LocalHi_SubmitScore adds first entry as record")
{
    LocalHighscore hi;
    LocalHi_Init(&hi);

    bool result = LocalHi_SubmitScore(&hi, "Alice", 5, 250);

    CHECK(result);
    CHECK_EQ(hi.lastSubmitResult, LOCALHI_SUBMIT_NEW_RECORD);
    CHECK_EQ(hi.lastSubmittedRank, 1);
    CHECK_EQ(hi.count, 1);
    CHECK_EQ(hi.entries[0].score, 250);
    CHECK(strcmp(hi.entries[0].username, "Alice") == 0);
}

TEST_CASE("LocalHi_SubmitScore maintains descending order")
{
    LocalHighscore hi;
    LocalHi_Init(&hi);

    LocalHi_SubmitScore(&hi, "Bob", 3, 150);
    LocalHi_SubmitScore(&hi, "Alice", 5, 250);
    LocalHi_SubmitScore(&hi, "Charlie", 7, 200);

    CHECK_EQ(hi.count, 3);
    CHECK_EQ(hi.entries[0].score, 250);
    CHECK(strcmp(hi.entries[0].username, "Alice") == 0);
    CHECK_EQ(hi.entries[1].score, 200);
    CHECK(strcmp(hi.entries[1].username, "Charlie") == 0);
    CHECK_EQ(hi.entries[2].score, 150);
    CHECK(strcmp(hi.entries[2].username, "Bob") == 0);
}

TEST_CASE("LocalHi_SubmitScore caps at LOCALHI_MAX_ENTRIES")
{
    LocalHighscore hi;
    LocalHi_Init(&hi);

    // Fill with 6 entries (LOCALHI_MAX_ENTRIES = 6)
    LocalHi_SubmitScore(&hi, "P1", 2, 60);
    LocalHi_SubmitScore(&hi, "P2", 2, 70);
    LocalHi_SubmitScore(&hi, "P3", 2, 80);
    LocalHi_SubmitScore(&hi, "P4", 2, 90);
    LocalHi_SubmitScore(&hi, "P5", 2, 100);
    LocalHi_SubmitScore(&hi, "P6", 2, 110);

    CHECK_EQ(hi.count, LOCALHI_MAX_ENTRIES);

    // Score that beats last entry (110)
    bool result = LocalHi_SubmitScore(&hi, "P7", 2, 120);
    CHECK(result);
    CHECK_EQ(hi.count, LOCALHI_MAX_ENTRIES);
    CHECK_EQ(hi.entries[0].score, 120);
    CHECK_EQ(hi.entries[LOCALHI_MAX_ENTRIES - 1].score, 70); // P1 evicted

    // Score that misses
    bool missed = LocalHi_SubmitScore(&hi, "Low", 3, 50);
    CHECK_FALSE(missed);
    CHECK_EQ(hi.lastSubmitResult, LOCALHI_SUBMIT_MISSED);
    CHECK_EQ(hi.lastSubmittedRank, 0);
    CHECK_EQ(hi.count, LOCALHI_MAX_ENTRIES); // Still 6
}

TEST_CASE("LocalHi_GetRank returns correct rank")
{
    LocalHighscore hi;
    LocalHi_Init(&hi);

    LocalHi_SubmitScore(&hi, "A", 1, 300);
    LocalHi_SubmitScore(&hi, "B", 1, 200);
    LocalHi_SubmitScore(&hi, "C", 1, 100);

    CHECK_EQ(LocalHi_GetRank(&hi, 300), 1);
    CHECK_EQ(LocalHi_GetRank(&hi, 250), 2); // Between A and B
    CHECK_EQ(LocalHi_GetRank(&hi, 200), 2);
    CHECK_EQ(LocalHi_GetRank(&hi, 50), 0);  // Not in top N
}

TEST_CASE("LocalHi_RecordAttempt updates percentile tracker")
{
    LocalHighscore hi;
    LocalHi_Init(&hi);

    LocalHi_RecordAttempt(&hi, 100);
    LocalHi_RecordAttempt(&hi, 200);
    LocalHi_RecordAttempt(&hi, 150);

    CHECK_EQ(hi.percentileTracker.totalAttempts, 3);
    CHECK_EQ(hi.percentileTracker.minScore, 100);
    CHECK_EQ(hi.percentileTracker.maxScore, 200);
    CHECK_EQ(hi.percentileTracker.bucketCounts[100], 1);
    CHECK_EQ(hi.percentileTracker.bucketCounts[150], 1);
    CHECK_EQ(hi.percentileTracker.bucketCounts[200], 1);
}

TEST_CASE("LocalHi_CalculatePercentile returns correct values")
{
    LocalHighscore hi;
    LocalHi_Init(&hi);

    // Record 10 attempts: scores 10, 20, 30, ..., 100
    for (int i = 1; i <= 10; i++) {
        LocalHi_RecordAttempt(&hi, i * 10);
    }

    // Score 50 beats 4 attempts (10, 20, 30, 40) → 4/10 = 40%
    float p50 = LocalHi_CalculatePercentile(&hi, 50);
    CHECK(p50 == doctest::Approx(40.0f).epsilon(0.01f));

    // Score 100 beats 9 attempts → 9/10 = 90%
    float p100 = LocalHi_CalculatePercentile(&hi, 100);
    CHECK(p100 == doctest::Approx(90.0f).epsilon(0.01f));

    // Score 0 beats nothing → 0%
    float p0 = LocalHi_CalculatePercentile(&hi, 0);
    CHECK(p0 == doctest::Approx(0.0f).epsilon(0.01f));
}

TEST_CASE("LocalHi_CalculatePercentile returns 0% when no attempts")
{
    LocalHighscore hi;
    LocalHi_Init(&hi);

    CHECK_EQ(LocalHi_CalculatePercentile(&hi, 100), 0.0f);
}

TEST_CASE("LocalHi lastSubmittedPercentile for new highest score")
{
    LocalHighscore hi;
    LocalHi_Init(&hi);

    // Record 10 baseline attempts with scores 10..100
    for (int i = 1; i <= 10; i++) {
        LocalHi_SubmitScore(&hi, "Player", 6, i * 10);
    }

    // Now submit a score (250) that is higher than all previous attempts (max was 100)
    LocalHi_SubmitScore(&hi, "Champ", 5, 250);

    // 250 beats all 10 previous attempts → 100th percentile
    // (percentile measures how many PRIOR attempts this score beats, not including itself)
    CHECK(hi.lastSubmittedPercentile == doctest::Approx(100.0f).epsilon(0.01f));
    CHECK_EQ(hi.lastSubmitResult, LOCALHI_SUBMIT_NEW_RECORD);
    CHECK_EQ(hi.lastSubmittedRank, 1);
    CHECK_EQ(hi.entries[0].score, 250);
    CHECK(strcmp(hi.entries[0].username, "Champ") == 0);
}

TEST_CASE("LocalHi_SubmitScore records attempt and calculates percentile")
{
    LocalHighscore hi;
    LocalHi_Init(&hi);

    // Record some baseline attempts
    LocalHi_SubmitScore(&hi, "A", 1, 50);
    LocalHi_SubmitScore(&hi, "B", 1, 100);
    LocalHi_SubmitScore(&hi, "C", 1, 150);

    // Now submit a high score (300 beats all 3 prior: 50, 100, 150)
    LocalHi_SubmitScore(&hi, "D", 1, 300);

    // Should beat 3 out of 3 prior attempts → 100%
    CHECK(hi.lastSubmittedPercentile == doctest::Approx(100.0f).epsilon(0.01f));
}

TEST_CASE("LocalHi username truncation for long names")
{
    LocalHighscore hi;
    LocalHi_Init(&hi);

    const char* longName = "ThisNameIsWayTooLongForTheBuffer";
    LocalHi_SubmitScore(&hi, longName, strlen(longName), 200);

    CHECK(hi.entries[0].username[LOCALHI_USERNAME_MAX - 1] == '\0');
    CHECK(strlen(hi.entries[0].username) < LOCALHI_USERNAME_MAX);
}

TEST_CASE("LocalHi score clamping in RecordAttempt")
{
    LocalHighscore hi;
    LocalHi_Init(&hi);

    LocalHi_RecordAttempt(&hi, -10);   // Below min
    LocalHi_RecordAttempt(&hi, 9999);  // Above max

    CHECK_EQ(hi.percentileTracker.bucketCounts[0], 1);
    CHECK_EQ(hi.percentileTracker.bucketCounts[LOCALHI_MAX_SCORE], 1);
    CHECK_EQ(hi.percentileTracker.totalAttempts, 2);
}

TEST_CASE("LocalHi circular queue overflow handling")
{
    LocalHighscore hi;
    LocalHi_Init(&hi);

    // Fill beyond queue size
    for (int i = 0; i < LOCALHI_PERCENTILE_QUEUE_SIZE + 100; i++) {
        LocalHi_RecordAttempt(&hi, i % 361);
    }

    // Queue should never exceed max size
    CHECK_LE(hi.percentileTracker.queueCount, LOCALHI_PERCENTILE_QUEUE_SIZE);
    // Total attempts should reflect evictions
    CHECK_LE(hi.percentileTracker.totalAttempts, LOCALHI_PERCENTILE_QUEUE_SIZE);
}

TEST_CASE("LocalHi expiry cleanup removes old entries")
{
    LocalHighscore hi;
    LocalHi_Init(&hi);

    LocalHi_SubmitScore(&hi, "Fresh", 5, 200);
    LocalHi_SubmitScore(&hi, "Old", 5, 150);

    // Manually age the "Old" entry past expiry
    hi.entries[1].timestamp -= (LOCALHI_EXPIRY_SECONDS + 100);

    // Also age its percentile entry
    LocalHiPercentileTracker* pt = &hi.percentileTracker;
    // Find the entry for "Old" (score 150) and age it
    for (int32_t i = 0; i < pt->queueCount; i++) {
        int32_t idx = (pt->queueWriteIdx - pt->queueCount + i + LOCALHI_PERCENTILE_QUEUE_SIZE) % LOCALHI_PERCENTILE_QUEUE_SIZE;
        if (pt->expiryQueue[idx].score == 150) {
            pt->expiryQueue[idx].timestamp -= (LOCALHI_EXPIRY_SECONDS + 100);
            break;
        }
    }

    LocalHi_CleanExpired(&hi);

    // "Old" should be removed from leaderboard
    CHECK_EQ(hi.count, 1);
    CHECK(strcmp(hi.entries[0].username, "Fresh") == 0);
}
