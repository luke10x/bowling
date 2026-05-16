#pragma once

// school/school.h
// School mode state + logic (no dependency on UserContext).
//
// Pattern:
// - UserContext owns `School school;`
// - game.cpp calls School_* functions and passes only the dependencies School needs.
// - No module includes/knows `UserContext`.

#include <stdint.h>
#include <string.h>
#include <cmath>
#include <glm/vec3.hpp>

#include "school_rules.h"

#include "../coins.h"
#include "../storyline.h"
#include "../clayton/slider.h"
#include "../clayton/clayton_click.h"

struct Physics;
struct Clayton;
struct DialogBox;
struct CatalogItem;
// (Clayton UI widgets are included above.)

struct SchoolMassTuning
{
    static constexpr float MASS_MIN_KG = 2.7f;
    static constexpr float MASS_MAX_KG = 7.8f;
    static constexpr float TWO_LBS_KG = 0.90718474f;
    static constexpr float LIGHT_TEST_MAX_KG = MASS_MIN_KG + TWO_LBS_KG;
    static constexpr float HEAVY_TEST_MIN_KG = MASS_MAX_KG - TWO_LBS_KG;
    static constexpr int REQUIRED_HITS_EACH = 3;
};

struct SchoolSpinTuning
{
    static constexpr int LEVELS = 2;
    static constexpr int COINS_PER_LEVEL = 3;
    static constexpr int TOTAL_REQUIRED = LEVELS * COINS_PER_LEVEL; // 9

    static constexpr float COIN_Y = 0.20f;
    static constexpr float Z_LAST = -0.05f;
    static constexpr float Z_STEP = 7.0f;
    static constexpr float Z0 = Z_LAST - (COINS_PER_LEVEL - 1) * Z_STEP;

    static constexpr float AMP_LVL1 = 0.15f;
    static constexpr float AMP_LVL2 = 0.20f;
    static constexpr float AMP_LVL3 = 0.25;

    static constexpr float LAUNCH_SPEED_CAP = 4.5f;
    static constexpr float THROW_TIMEOUT_S = 15.0f;
    static constexpr float STALLED_BANNER_AT_S = 15.0f;
};

struct SchoolAimTuning
{
    static constexpr int REQUIRED_POINTS = 4;
    static constexpr float PULL_ENOUGH_THRESHOLD = 0.92f;
    static constexpr float CENTER_X_MAX_ABS = 0.12f; // stricter: must be near center to qualify
};

inline float SchoolSpin_AmpForLevel(int level)
{
    if (level <= 1)
        return SchoolSpinTuning::AMP_LVL1;
    if (level == 2)
        return SchoolSpinTuning::AMP_LVL2;
    return SchoolSpinTuning::AMP_LVL3;
}

// Values School is allowed to modify in game state (passed from game.cpp).
struct SchoolRuntimeTuning
{
    // Ball / lane knobs the lesson presets may modify.
    float *desiredMassKg = nullptr;
    float *lightnessBuff = nullptr;
    float *launchBuffEffective = nullptr;
    float *armImpulseAtThrow = nullptr;
    float *angularFactor = nullptr;
    float *ballSkid = nullptr;
    float *ballSkidStartScale = nullptr;
    float *ballBaseFriction = nullptr;
    float *laneOilThickness = nullptr;
    float *ballRestitution = nullptr;
};

struct SchoolServices
{
    Physics *phy = nullptr;
    CoinLane *coinLane = nullptr;
    DialogBox *dialog = nullptr;
    const CatalogItem *myBall = nullptr; // read-only

    // Callbacks implemented in game.cpp (kept out of module to avoid circular deps)
    float (*ballStatsLightnessBuff)(float massKg) = nullptr;
    float (*ballStatsRestitutionMassScale)(float massKg) = nullptr;
    float (*remapClamped)(float value, float inMin, float inMax, float outMin, float outMax) = nullptr;

    // Mapping constants (owned by game.cpp; passed in).
    float catalogBuffMin = 0.0f;
    float catalogBuffMax = 1.0f;
    float physicsArmImpulseMin = 6.0f;
    float physicsArmImpulseMax = 16.0f;
};

struct School
{
    // Progress / selection
    int lesson = 1;             // 1..5 (entry target)
    bool lessonDone[5] = {false, false, false, false, false};
    int unlockedLessons = 1;    // 1..5
    int selectedLesson = 1;     // 1..5
    int lastSelectedLesson = 1; // remember last chosen lesson
    int lessonRolls = 0;

    // UI widgets (owned by School; built by school_clay.h)
    Clayton_Click exitButton;
    Clayton_Click lessonButtons[5];
    Clayton_Slider massSlider;

    // Lesson 1 (mass)
    int massLightHits = 0;
    int massHeavyHits = 0;
    bool massTestCompleted = false;
    bool massMidHintShown = false;
    bool massSwapHintShown = false;

    // Lesson 2 (spin/coins)
    int spinLevel = 1;
    int spinSafeCoins = 0;
    int spinCollectedInLevel = 0;
    bool spinTestCompleted = false;
    bool spinLevelJustCompleted = false;
    float celebratePauseT = 0.0f;
    int celebrateKind = 0; // 0=none, 1=lesson1_light, 2=lesson1_heavy, 3=lesson2_level
    bool returnToStartActive = false;
    float returnToStartT = 0.0f;
    float returnToStartDuration = 0.35f;
    float returnToStartDtLoan = 0.0f;
    glm::vec3 returnFromBallPos = {0.0f, 0.0f, 0.0f};

    // Lesson 3 (aim/pullback)
    int aimLessonPoints = 0;
    bool aimLessonCompleted = false;
    float aimPull01 = 0.0f;
    bool aimPullEnough = false;
    bool aimCenteredEnough = true;
    bool aimQualifiedThisThrow = false;

    // Restore selection after leaving School
    int ballIdBeforeSchool = -1;

    bool exitConfirmPending = false;
};

inline void School_Init(School *self)
{
    if (!self)
        return;
    // Ensure sane defaults even if memory was uninitialized (hot reload / platform quirks).
    if (self->unlockedLessons < 1)
        self->unlockedLessons = 1;
    if (self->unlockedLessons > 5)
        self->unlockedLessons = 5;
    if (self->selectedLesson < 1 || self->selectedLesson > 5)
        self->selectedLesson = 1;
}

// Returns true if the school celebration pause is active.
inline bool School_IsPaused(const School *self)
{
    return self && self->celebratePauseT > 0.0f;
}

inline void School_Tick(School *self, float dt)
{
    if (!self)
        return;
    if (!std::isfinite(dt) || dt <= 0.0f)
        return;
    if (self->celebratePauseT > 0.0f)
    {
        self->celebratePauseT -= dt;
        if (self->celebratePauseT <= 0.0f)
        {
            self->celebratePauseT = 0.0f;
            // Note: we intentionally do NOT clear `celebrateKind` here.
            // The caller should clear it after handling the end of the pause, so
            // we can reliably detect "pause ended" transitions without racey
            // same-frame state resets.
        }
    }
}

inline int School_FindFirstUncompletedLesson(const School *self)
{
    for (int i = 0; i < 5; i++)
        if (!self->lessonDone[i])
            return i + 1;
    return 1;
}

inline void SchoolSpin_InitCoinsForLevel(School *self, const SchoolServices &svc, int level)
{
    if (!self || !svc.coinLane)
        return;
    level = (level < 1) ? 1 : (level > SchoolSpinTuning::LEVELS ? SchoolSpinTuning::LEVELS : level);
    svc.coinLane->currentPattern = CoinPattern::Static;
    svc.coinLane->activeCount = SchoolSpinTuning::COINS_PER_LEVEL;
    const float amp = SchoolSpin_AmpForLevel(level);
    const float halfW = CoinLane::LANE_WIDTH * 0.5f - CoinLane::GUTTER_MARGIN;

    for (int i = 0; i < svc.coinLane->activeCount; i++)
    {
        Coin &c = svc.coinLane->coins[i];
        const float z = SchoolSpinTuning::Z0 + (float)i * SchoolSpinTuning::Z_STEP;
        float x = ((i % 2) == 0) ? -amp : amp;
        if (x < -halfW)
            x = -halfW;
        if (x > halfW)
            x = halfW;

        c.basePosition = {x, SchoolSpinTuning::COIN_Y, z};
        c.position = c.basePosition;
        c.phaseOffset = (float)i * 0.628f;
        c.rotation = 0.0f;
        c.scale = 1.0f;
        c.state = CoinState::Active;
        c.flyTriggered = false;
        c.updateTransform();
    }
    for (int i = svc.coinLane->activeCount; i < CoinLane::MAX_COINS; i++)
    {
        svc.coinLane->coins[i].state = CoinState::Dead;
        svc.coinLane->coins[i].flyTriggered = false;
    }
    svc.coinLane->emptyTimer = 0.0f;
}

inline void School_ClearCoins(const SchoolServices &svc)
{
    if (!svc.coinLane)
        return;
    svc.coinLane->activeCount = 0;
    for (int i = 0; i < CoinLane::MAX_COINS; i++)
    {
        svc.coinLane->coins[i].state = CoinState::Dead;
        svc.coinLane->coins[i].flyTriggered = false;
    }
    svc.coinLane->emptyTimer = 0.0f;
}

inline void School_ApplyLesson3SpinPreset(
    School *self, const SchoolServices &svc, const SchoolRuntimeTuning &rt)
{
    (void)self;
    if (!svc.phy || !svc.myBall || !rt.desiredMassKg || !rt.angularFactor || !rt.ballSkid ||
        !rt.ballSkidStartScale || !rt.ballBaseFriction || !rt.laneOilThickness ||
        !rt.ballRestitution || !rt.lightnessBuff || !rt.launchBuffEffective || !rt.armImpulseAtThrow ||
        !svc.ballStatsLightnessBuff || !svc.ballStatsRestitutionMassScale || !svc.remapClamped)
        return;

    // Keep current desiredMass, but clamp to lesson range.
    float mass = *rt.desiredMassKg;
    if (mass < SchoolMassTuning::MASS_MIN_KG)
        mass = SchoolMassTuning::MASS_MIN_KG;
    if (mass > SchoolMassTuning::MASS_MAX_KG)
        mass = SchoolMassTuning::MASS_MAX_KG;
    *rt.desiredMassKg = mass;
    svc.phy->set_ball_mass(mass);

    // Mass-derived launch modifier.
    *rt.lightnessBuff = svc.ballStatsLightnessBuff(mass);
    float eff = svc.myBall->launchBuff * (*rt.lightnessBuff);
    if (eff < 0.0f)
        eff = 0.0f;
    if (eff > 1.0f)
        eff = 1.0f;
    *rt.launchBuffEffective = eff;
    *rt.armImpulseAtThrow = svc.remapClamped(
        eff, svc.catalogBuffMin, svc.catalogBuffMax, svc.physicsArmImpulseMin, svc.physicsArmImpulseMax
    );

    // Stronger spin.
    *rt.angularFactor = 0.72f;

    // Low skid, higher bite-derived friction feel.
    *rt.ballSkid = 0.0f;
    *rt.ballSkidStartScale = 0.55f;
    // Almost max "bite" for this lesson so the ball reacts strongly when you reverse spin.
    *rt.ballBaseFriction = 0.59f;
    *rt.laneOilThickness = 0.25f;

    // Restitution mass scaling.
    float r = svc.myBall->restitution;
    if (r < 0.0f)
        r = 0.0f;
    if (r > 1.0f)
        r = 1.0f;
    r *= svc.ballStatsRestitutionMassScale(mass);
    if (r < 0.0f)
        r = 0.0f;
    if (r > 1.0f)
        r = 1.0f;
    *rt.ballRestitution = r;
}

inline void School_SelectLesson(
    School *self, const SchoolServices &svc, const SchoolRuntimeTuning &rt, int lessonNum, bool playStory)
{
    if (!self)
        return;
    if (lessonNum < 1)
        lessonNum = 1;
    if (lessonNum > 5)
        lessonNum = 5;

    self->selectedLesson = lessonNum;
    self->lastSelectedLesson = lessonNum;
    self->lessonRolls = 0;

    // Keep the mass slider + physics mass consistent whenever entering school/lessons.
    if (rt.desiredMassKg)
    {
        float m = *rt.desiredMassKg;
        if (m < SchoolMassTuning::MASS_MIN_KG)
            m = SchoolMassTuning::MASS_MIN_KG;
        if (m > SchoolMassTuning::MASS_MAX_KG)
            m = SchoolMassTuning::MASS_MAX_KG;
        *rt.desiredMassKg = m;
        ClaytonSlider_SetValue(&self->massSlider, m);
        if (svc.phy)
            svc.phy->set_ball_mass(m);
    }

    // Replay behavior: selecting a lesson always resets its session state.
    // New lesson order:
    // 1 = Aim (pullback)
    // 2 = Mass
    // 3 = Spin (coins only, no pins)
    if (lessonNum == 1)
    {
        School_ClearCoins(svc);
        self->aimLessonPoints = 0;
        self->aimLessonCompleted = false;
        self->aimPull01 = 0.0f;
        self->aimPullEnough = false;
        self->aimCenteredEnough = true;
        self->aimQualifiedThisThrow = false;
    }
    if (lessonNum == 2)
    {
        School_ClearCoins(svc);
        self->massLightHits = 0;
        self->massHeavyHits = 0;
        self->massTestCompleted = false;
        self->massMidHintShown = false;
        self->massSwapHintShown = false;
    }
    if (lessonNum == 3)
    {
        self->spinLevel = 1;
        self->spinSafeCoins = 0;
        self->spinCollectedInLevel = 0;
        self->spinTestCompleted = false;
        School_ApplyLesson3SpinPreset(self, svc, rt);
        SchoolSpin_InitCoinsForLevel(self, svc, self->spinLevel);
    }
    if (lessonNum == 4)
    {
        // Oil lesson: no special per-lesson state yet; lane tuning is applied by game.cpp
        // (so we don't depend on UserContext / lane structs here).
        School_ClearCoins(svc);
        // Reuse coin-progress ints to track oil-lesson progress (no new fields; keeps hot-reload memory layout).
        self->spinSafeCoins = 0;        // counts successful re-oils
        self->spinCollectedInLevel = 0; // unused in this lesson
        self->spinLevelJustCompleted = false; // reused as "pending completion story"
    }
    if (lessonNum == 5)
    {
        // Strike lesson: coins are placed by game.cpp (needs lane/pin positions), just clear state here.
        School_ClearCoins(svc);
    }

    if (playStory && svc.dialog)
    {
        if (lessonNum == 1)
            svc.dialog->open(1032);
        if (lessonNum == 2)
            svc.dialog->open(1000);
        if (lessonNum == 3)
            svc.dialog->open(1022);
        if (lessonNum == 4)
            svc.dialog->open(1052);
        if (lessonNum == 5)
            svc.dialog->open(1070);
    }
}

inline void School_Enter(School *self, const SchoolServices &svc, const SchoolRuntimeTuning &rt, bool playStory)
{
    if (!self)
        return;

    const int firstUncompleted = School_FindFirstUncompletedLesson(self);
    int targetLesson = firstUncompleted;

    // Prefer last selected lesson if it is still relevant (uncompleted and unlocked).
    if (self->lastSelectedLesson >= 1 && self->lastSelectedLesson <= 5)
    {
        const int li = self->lastSelectedLesson - 1;
        const bool unlocked = self->lastSelectedLesson <= self->unlockedLessons;
        if (unlocked && li >= 0 && li < 5 && !self->lessonDone[li])
            targetLesson = self->lastSelectedLesson;
    }
    self->lesson = targetLesson;

    // Ensure unlocked lessons cover all completed ones.
    int maxUnlocked = 1;
    for (int i = 0; i < 5; i++)
        if (self->lessonDone[i])
            if (maxUnlocked < i + 2)
                maxUnlocked = i + 2;
    if (maxUnlocked > 5)
        maxUnlocked = 5;
    if (self->unlockedLessons < maxUnlocked)
        self->unlockedLessons = maxUnlocked;

    // Reset some transient hint state.
    self->massMidHintShown = false;
    self->massSwapHintShown = false;

    School_SelectLesson(self, svc, rt, targetLesson, playStory);
}
