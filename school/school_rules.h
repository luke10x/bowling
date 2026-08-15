#pragma once

// school/school_rules.h
// Tiny dependency-free rules for school mode (safe to include in unit tests).

inline bool School_LessonHasPins(int lessonNum)
{
    // Lesson 3 now uses lightweight pin targets instead of moving coins.
    (void)lessonNum;
    return true;
}
