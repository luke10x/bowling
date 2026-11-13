#pragma once

// Player-only assistance after consecutive completed throws without a mark.
// The first value applies on the fifth ordinary throw, or the third throw in
// the strike lesson. Subsequent misses advance one entry at a time.
struct LoserBufTuning
{
    static constexpr int NORMAL_START_THROW = 5;
    static constexpr int STRIKE_LESSON_START_THROW = 3;
    static constexpr int SPIN_LESSON_START_THROW = 5;
    static constexpr float HIT_MULTIPLIER_AT_MAX = 2.0f;
    static constexpr float LAUNCH_MULTIPLIER_AT_MAX = 1.5f;
    static constexpr float VALUES[] = {
        0.10f, 0.10f, 0.20f, 0.30f, 0.40f, 0.45f, 0.50f,
        0.55f, 0.60f, 0.65f, 0.67f, 0.70f, 0.72f, 0.74f,
        0.76f, 0.78f, 0.80f
    };
    static constexpr float SLOW_STEP = 0.01f;
    static constexpr float SLOW_STEP_CEILING = 0.90f;
    static constexpr float FINAL_STEP = 0.05f;
};

static inline float LoserBuf_ValueForMisses(int misses, bool strikeLesson)
{
    const int start = strikeLesson
        ? LoserBufTuning::STRIKE_LESSON_START_THROW
        : LoserBufTuning::NORMAL_START_THROW;
    if (misses < start)
        return 0.0f;
    int index = misses - start;
    constexpr int count = sizeof(LoserBufTuning::VALUES) / sizeof(float);
    if (index < count)
        return LoserBufTuning::VALUES[index];
    index -= count;
    const int slowSteps = static_cast<int>((LoserBufTuning::SLOW_STEP_CEILING -
        LoserBufTuning::VALUES[count - 1]) / LoserBufTuning::SLOW_STEP + 0.5f);
    if (index < slowSteps)
        return LoserBufTuning::VALUES[count - 1] + (index + 1) * LoserBufTuning::SLOW_STEP;
    index -= slowSteps;
    const float value = LoserBufTuning::SLOW_STEP_CEILING +
        (index + 1) * LoserBufTuning::FINAL_STEP;
    return value < 1.0f ? value : 1.0f;
}

static inline float LoserBuf_LaunchMultiplier(float value)
{
    return 1.0f + value * (LoserBufTuning::LAUNCH_MULTIPLIER_AT_MAX - 1.0f);
}

static inline float LoserBuf_HitMultiplier(float value)
{
    return 1.0f + value * (LoserBufTuning::HIT_MULTIPLIER_AT_MAX - 1.0f);
}

static inline float SpinLoserBuf_ValueForRolls(int rolls)
{
    return LoserBuf_ValueForMisses(
        rolls - LoserBufTuning::SPIN_LESSON_START_THROW + LoserBufTuning::NORMAL_START_THROW,
        false
    );
}

static inline float SpinLoserBuf_EffectiveStat(float base, float value)
{
    if (base < 0.0f) base = 0.0f;
    if (base > 1.0f) base = 1.0f;
    if (value < 0.0f) value = 0.0f;
    if (value > 1.0f) value = 1.0f;
    return base + (1.0f - base) * value;
}
