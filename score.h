#pragma once

    struct Frame {
        int roll1;        // 0–10
        int roll2;        // 0–10 (or -1 if not bowled yet)
        int roll3;        // 10th frame only, else -1
        int isStrike;     // 1 if strike
        int isSpare;      // 1 if spare
        int frameScore;   // final calculated score for this frame
    };
typedef struct {
    // One frame of bowling
    Frame frames[10];

    int totalScore;
} BowlingScoreboard;

// Returns 1 if frame completed, 0 if still in progress
int addRoll(BowlingScoreboard *sb, int pins)
{
    int f; // current frame

    // Find current frame
    for (f = 0; f < 10; f++) {
        Frame *fr = &sb->frames[f];
        if (fr->roll3 != -1) continue;   // completed 10th frame
        if (!fr->isStrike && fr->roll2 == -1) break; // still rolling
        if (fr->isStrike && f < 9) break; // strike ends frame (1–9)
    }

    Frame *fr = &sb->frames[f];

    // --- Frame 10 special handling ---
    if (f == 9) {
        if (fr->roll1 == -1) {
            fr->roll1 = pins;
            fr->isStrike = (pins == 10);
            return 0;
        }
        if (fr->roll2 == -1) {
            fr->roll2 = pins;
            fr->isSpare = (!fr->isStrike && fr->roll1 + pins == 10);
            if (fr->isStrike || fr->isSpare)
                return 0; // gets 3rd roll
            fr->roll3 = -1; // no bonus roll
            return 1;
        }
        // 3rd roll
        fr->roll3 = pins;
        return 1;
    }

    // --- Frames 1–9 normal logic ---
    if (fr->roll1 == -1) {
        fr->roll1 = pins;

        if (pins == 10) {
            fr->isStrike = 1;
            fr->roll2 = 0;   // by convention
            return 1;        // frame ends
        }

        return 0;
    }

    // roll2
    fr->roll2 = pins;
    if (fr->roll1 + fr->roll2 == 10)
        fr->isSpare = 1;

    return 1; // frame completed
}

void computeScore(BowlingScoreboard *sb)
{
    sb->totalScore = 0;

    for (int i = 0; i < 10; i++) {
        Frame *f = &sb->frames[i];
        int score = 0;

        if (i < 9) {
            // ------ FRAMES 1–9 ------

            if (f->isStrike) {
                score = 10;

                // Next two rolls:
                if (sb->frames[i+1].isStrike) {
                    score += 10;
                    // next-next roll
                    if (i + 2 < 10)
                        score += sb->frames[i+2].roll1;
                    else
                        score += sb->frames[i+1].roll2; // 10th frame case
                } else {
                    score += sb->frames[i+1].roll1 + sb->frames[i+1].roll2;
                }
            }
            else if (f->isSpare) {
                score = 10 + sb->frames[i+1].roll1;  // 1 bonus roll
            }
            else {
                score = f->roll1 + f->roll2;
            }
        }
        else {
            // ------ FRAME 10 ------
            score = f->roll1;
            if (f->roll2 >= 0) score += f->roll2;
            if (f->roll3 >= 0) score += f->roll3;
        }

        f->frameScore = score;
        sb->totalScore += score;
    }
}