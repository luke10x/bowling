// =============================================================================
// local_highscore.h — C-Compatible Local Highscore System
// With per-score percentile buckets + TTL expiration
// =============================================================================
#pragma once

#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

// -----------------------------------------------------------------------------
// Constants
// -----------------------------------------------------------------------------
static const int LOCALHI_MAX_ENTRIES = 6;           // Top-N leaderboard size
static const int LOCALHI_USERNAME_MAX = 20;         // Max username length
static const int LOCALHI_EXPIRY_SECONDS = 3600;     // 1 hour TTL
static const int LOCALHI_MAX_SCORE = 360;           // Bowling max score
static const int LOCALHI_PERCENTILE_QUEUE_SIZE = 4096; // Max attempts to track

// -----------------------------------------------------------------------------
// C-Compatible Enum
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

// Expiry queue entry for percentile tracking
typedef struct {
    time_t timestamp;
    int32_t score;  // 0–360
} LocalHiExpiryEntry;

typedef struct {
    // Circular queue for TTL expiration
    LocalHiExpiryEntry expiryQueue[LOCALHI_PERCENTILE_QUEUE_SIZE];
    int32_t queueWriteIdx;  // Next write position (circular)
    int32_t queueCount;     // Number of valid entries in queue
    
    // One bucket per possible score value (0–360)
    int32_t bucketCounts[LOCALHI_MAX_SCORE + 1];  // bucket[score] = count
    int32_t totalAttempts;
    
    // Track min/max for UI display
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
    
    LocalHiPercentileTracker* pt = &self->percentileTracker;
    pt->totalAttempts = 0;
    pt->queueWriteIdx = 0;
    pt->queueCount = 0;
    memset(pt->bucketCounts, 0, sizeof(pt->bucketCounts));
    pt->minScore = LOCALHI_MAX_SCORE;
    pt->maxScore = 0;
}

// Optional helper: Recalculate exact min/max after cleanup
inline void LocalHi_RecalcMinMax(LocalHiPercentileTracker* pt) {
    pt->minScore = LOCALHI_MAX_SCORE;
    pt->maxScore = 0;
    for (int32_t s = 0; s <= LOCALHI_MAX_SCORE; s++) {
        if (pt->bucketCounts[s] > 0) {
            if (s < pt->minScore) pt->minScore = s;
            if (s > pt->maxScore) pt->maxScore = s;
        }
    }
    if (pt->totalAttempts == 0) {
        pt->minScore = LOCALHI_MAX_SCORE;
        pt->maxScore = 0;
    }
}

inline void LocalHi_CleanExpired(LocalHighscore* self) {
    if (!self) return;
    time_t now = time(NULL);
    
    // Clean leaderboard entries
    int32_t writeIdx = 0;
    for (int32_t i = 0; i < self->count; i++) {
        if ((now - self->entries[i].timestamp) < LOCALHI_EXPIRY_SECONDS) {
            if (writeIdx != i) self->entries[writeIdx] = self->entries[i];
            writeIdx++;
        }
    }
    self->count = writeIdx;
    
    // Clean percentile tracker queue (TTL expiration)
    LocalHiPercentileTracker* pt = &self->percentileTracker;
    
    // Process from oldest to newest, removing expired entries
    while (pt->queueCount > 0) {
        // Calculate index of oldest entry
        int32_t oldestIdx = (pt->queueWriteIdx - pt->queueCount + LOCALHI_PERCENTILE_QUEUE_SIZE) % LOCALHI_PERCENTILE_QUEUE_SIZE;
        LocalHiExpiryEntry* oldest = &pt->expiryQueue[oldestIdx];
        
        // Check if expired
        if ((now - oldest->timestamp) >= LOCALHI_EXPIRY_SECONDS) {
            // Decrement bucket and total
            if (oldest->score >= 0 && oldest->score <= LOCALHI_MAX_SCORE) {
                pt->bucketCounts[oldest->score]--;
                if (pt->bucketCounts[oldest->score] < 0) pt->bucketCounts[oldest->score] = 0;
            }
            pt->totalAttempts--;
            if (pt->totalAttempts < 0) pt->totalAttempts = 0;
            
            // Remove oldest by shifting logical window forward
            pt->queueCount--;
        } else {
            // Oldest not expired → nothing else is (queue is chronological)
            break;
        }
    }
    
    // Recalculate exact min/max after cleanup
    LocalHi_RecalcMinMax(pt);
}

inline int32_t LocalHi_GetMinutesAgo(time_t timestamp) {
    time_t now = time(NULL);
    int32_t mins = (int32_t)((now - timestamp) / 60);
    return (mins < 1) ? 1 : (mins > 60) ? 60 : mins;
}

inline float LocalHi_CalculatePercentile(LocalHighscore* self, int32_t score) {
    if (!self) return 0.0f;
    LocalHiPercentileTracker* pt = &self->percentileTracker;

    if (pt->totalAttempts == 0) return 0.0f; // No prior attempts to compare against
    
    // Clamp score to valid range
    if (score < 0) score = 0;
    if (score > LOCALHI_MAX_SCORE) score = LOCALHI_MAX_SCORE;
    
    // Count attempts with score STRICTLY LESS than target
    int32_t beatenCount = 0;
    for (int32_t s = 0; s < score; s++) {
        beatenCount += pt->bucketCounts[s];
    }
    
    // Percentile = % of attempts this score beats
    return (float)beatenCount / (float)pt->totalAttempts * 100.0f;
}

inline void LocalHi_RecordAttempt(LocalHighscore* self, int32_t score) {
    if (!self) return;
    LocalHiPercentileTracker* pt = &self->percentileTracker;
    
    // Clamp score
    if (score < 0) score = 0;
    if (score > LOCALHI_MAX_SCORE) score = LOCALHI_MAX_SCORE;
    
    // Handle circular queue overflow: remove oldest if full
    if (pt->queueCount >= LOCALHI_PERCENTILE_QUEUE_SIZE) {
        int32_t oldestIdx = (pt->queueWriteIdx - pt->queueCount + LOCALHI_PERCENTILE_QUEUE_SIZE) % LOCALHI_PERCENTILE_QUEUE_SIZE;
        LocalHiExpiryEntry* oldest = &pt->expiryQueue[oldestIdx];
        
        // Decrement its bucket
        if (oldest->score >= 0 && oldest->score <= LOCALHI_MAX_SCORE) {
            pt->bucketCounts[oldest->score]--;
            if (pt->bucketCounts[oldest->score] < 0) pt->bucketCounts[oldest->score] = 0;
        }
        pt->totalAttempts--;
        if (pt->totalAttempts < 0) pt->totalAttempts = 0;
        // Note: min/max become "soft" bounds after eviction; RecalcMinMax fixes if needed
    }
    
    // Add new entry at write position
    int32_t writeIdx = pt->queueWriteIdx % LOCALHI_PERCENTILE_QUEUE_SIZE;
    pt->expiryQueue[writeIdx].timestamp = time(NULL);
    pt->expiryQueue[writeIdx].score = score;
    pt->queueWriteIdx++;
    
    // Only increment queueCount if we didn't overflow
    if (pt->queueCount < LOCALHI_PERCENTILE_QUEUE_SIZE) {
        pt->queueCount++;
    }
    
    // Update bucket and total
    pt->bucketCounts[score]++;
    pt->totalAttempts++;
    
    // Update min/max
    if (score < pt->minScore) pt->minScore = score;
    if (score > pt->maxScore) pt->maxScore = score;
}

inline bool LocalHi_SubmitScore(LocalHighscore* self, const char* username, int usernameLen, int32_t score) {
    if (!self || !username) return false;

    LocalHi_CleanExpired(self);

    // Calculate percentile BEFORE recording this attempt,
    // so it measures how many PRIOR attempts this score beats
    self->lastSubmittedScore = score;
    self->lastSubmittedPercentile = LocalHi_CalculatePercentile(self, score);

    LocalHi_RecordAttempt(self, score);
    
    bool isTopN = (self->count < LOCALHI_MAX_ENTRIES) || 
                  (score > self->entries[LOCALHI_MAX_ENTRIES - 1].score);
    
    if (isTopN) {
        int32_t pos = self->count;
        for (int32_t i = 0; i < self->count; i++) {
            if (score > self->entries[i].score) { pos = i; break; }
        }
        int32_t newCount = (self->count < LOCALHI_MAX_ENTRIES) ? self->count + 1 : LOCALHI_MAX_ENTRIES;
        for (int32_t i = newCount - 1; i > pos; i--) self->entries[i] = self->entries[i - 1];
        
        // Safe string copy with explicit null-termination
        int copyLen = (usernameLen < LOCALHI_USERNAME_MAX - 1) ? usernameLen : LOCALHI_USERNAME_MAX - 1;
        memcpy(self->entries[pos].username, username, copyLen);
        self->entries[pos].username[copyLen] = '\0';
        
        self->entries[pos].score = score;
        self->entries[pos].timestamp = time(NULL);
        self->count = newCount;
        self->lastSubmitResult = LOCALHI_SUBMIT_NEW_RECORD;
        self->lastSubmittedRank = pos + 1;
    } else {
        self->lastSubmitResult = LOCALHI_SUBMIT_MISSED;
        self->lastSubmittedRank = 0;
    }
    return isTopN;
}

inline int32_t LocalHi_GetRank(LocalHighscore* self, int32_t score) {
    if (!self) return 0;
    LocalHi_CleanExpired(self);
    for (int32_t i = 0; i < self->count; i++) {
        if (score >= self->entries[i].score) return i + 1;
    }
    return 0;
}