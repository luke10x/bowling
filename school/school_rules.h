#pragma once

// school/school_rules.h
// Tiny dependency-free rules for school mode (safe to include in unit tests).

inline bool School_LessonHasPins(int lessonNum)
{
    // Lesson 3 is the coin-only spin/driving lesson. All other lessons keep pins.
    return lessonNum != 3;
}
