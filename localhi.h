// =============================================================================
// local_highscore.h — C-Compatible Local Highscore System
// =============================================================================
#pragma once

#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

// -----------------------------------------------------------------------------
// Constants
// -----------------------------------------------------------------------------
static const int LOCALHI_MAX_ENTRIES = 10;
static const int LOCALHI_USERNAME_MAX = 20;
static const int LOCALHI_EXPIRY_SECONDS = 3600;  // 1 hour
static const int LOCALHI_PERCENTILE_BUCKETS = 100;

// -----------------------------------------------------------------------------
// C-Compatible Enum (defined OUTSIDE struct)
// -----------------------------------------------------------------------------
typedef enum {
    LOCALHI_SUBMIT_NONE,
    LOCALHI_SUBMIT_NEW_RECORD,
    LOCALHI_SUBMIT_MISSED
} LocalHiSubmitResult;

// -----------------------------------------------------------------------------
// Data Structures
// -----------------------------------------------------------------------------
typedef struct {
    char username[LOCALHI_USERNAME_MAX];
    int32_t score;
    time_t timestamp;
} LocalHiEntry;

typedef struct {
    int32_t bucketCounts[LOCALHI_PERCENTILE_BUCKETS];
    int32_t totalAttempts;
    int32_t minScore;
    int32_t maxScore;
} LocalHiPercentileTracker;

typedef struct {
    LocalHiEntry entries[LOCALHI_MAX_ENTRIES];
    int32_t count;
    LocalHiPercentileTracker percentileTracker;
    LocalHiSubmitResult lastSubmitResult;
    int32_t lastSubmittedScore;
    int32_t lastSubmittedRank;
    float lastSubmittedPercentile;
} LocalHighscore;

// -----------------------------------------------------------------------------
// Inline Functions
// -----------------------------------------------------------------------------

inline void LocalHi_Init(LocalHighscore* self) {
    if (!self) return;
    memset(self, 0, sizeof(LocalHighscore));
    self->lastSubmitResult = LOCALHI_SUBMIT_NONE;
    self->percentileTracker.minScore = 999999;
}

inline void LocalHi_CleanExpired(LocalHighscore* self) {
    if (!self) return;
    time_t now = time(NULL);
    int32_t writeIdx = 0;
    for (int32_t i = 0; i < self->count; i++) {
        if ((now - self->entries[i].timestamp) < LOCALHI_EXPIRY_SECONDS) {
            if (writeIdx != i) self->entries[writeIdx] = self->entries[i];
            writeIdx++;
        }
    }
    self->count = writeIdx;
}

inline int32_t LocalHi_GetMinutesAgo(time_t timestamp) {
    time_t now = time(NULL);
    int32_t mins = (int32_t)((now - timestamp) / 60);
    return (mins < 1) ? 1 : (mins > 60) ? 60 : mins;
}

inline float LocalHi_CalculatePercentile(LocalHighscore* self, int32_t score) {
    if (!self || self->percentileTracker.totalAttempts == 0) return 50.0f;
    
    int32_t beaten = 0;
    int32_t bucket = score / 10;
    if (bucket < 0) bucket = 0;
    if (bucket >= LOCALHI_PERCENTILE_BUCKETS) bucket = LOCALHI_PERCENTILE_BUCKETS - 1;
    
    for (int32_t i = 0; i < bucket; i++) beaten += self->percentileTracker.bucketCounts[i];
    
    if (self->percentileTracker.bucketCounts[bucket] > 0) {
        int32_t start = bucket * 10;
        float frac = (float)(score - start) / 10.0f;
        beaten += (int32_t)(self->percentileTracker.bucketCounts[bucket] * frac);
    }
    return (float)beaten / (float)self->percentileTracker.totalAttempts * 100.0f;
}

inline void LocalHi_RecordAttempt(LocalHighscore* self, int32_t score) {
    if (!self) return;
    int32_t bucket = score / 10;
    if (bucket < 0) bucket = 0;
    if (bucket >= LOCALHI_PERCENTILE_BUCKETS) bucket = LOCALHI_PERCENTILE_BUCKETS - 1;
    self->percentileTracker.bucketCounts[bucket]++;
    self->percentileTracker.totalAttempts++;
    if (score < self->percentileTracker.minScore) self->percentileTracker.minScore = score;
    if (score > self->percentileTracker.maxScore) self->percentileTracker.maxScore = score;
}

inline bool LocalHi_SubmitScore(LocalHighscore* self, const char* username, int usernameLen, int32_t score) {
    if (!self || !username) return false;
    LocalHi_CleanExpired(self);
    LocalHi_RecordAttempt(self, score);
    self->lastSubmittedScore = score;
    self->lastSubmittedPercentile = LocalHi_CalculatePercentile(self, score);
    
    bool isTop10 = (self->count < LOCALHI_MAX_ENTRIES) || 
                   (score > self->entries[LOCALHI_MAX_ENTRIES - 1].score);
    
    if (isTop10) {
        int32_t pos = self->count;
        for (int32_t i = 0; i < self->count; i++) {
            if (score > self->entries[i].score) { pos = i; break; }
        }
        int32_t newCount = (self->count < LOCALHI_MAX_ENTRIES) ? self->count + 1 : LOCALHI_MAX_ENTRIES;
        for (int32_t i = newCount - 1; i > pos; i--) self->entries[i] = self->entries[i - 1];
        
        strncpy(self->entries[pos].username, username, LOCALHI_USERNAME_MAX - 1);
        self->entries[pos].username[usernameLen] = '\0';
        self->entries[pos].score = score;
        self->entries[pos].timestamp = time(NULL);
        self->count = newCount;
        self->lastSubmitResult = LOCALHI_SUBMIT_NEW_RECORD;
        self->lastSubmittedRank = pos + 1;
    } else {
        self->lastSubmitResult = LOCALHI_SUBMIT_MISSED;
        self->lastSubmittedRank = 0;
    }
    return isTop10;
}

inline int32_t LocalHi_GetRank(LocalHighscore* self, int32_t score) {
    if (!self) return 0;
    LocalHi_CleanExpired(self);
    for (int32_t i = 0; i < self->count; i++) {
        if (score >= self->entries[i].score) return i + 1;
    }
    return 0;
}