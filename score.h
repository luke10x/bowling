#pragma once

struct Frame
{
    int roll1;      // 0–10
    int roll2;      // 0–10 (or -1 if not bowled yet)
    int roll3;      // 10th frame only, else -1
    int isStrike;   // 1 if strike
    int isSpare;    // 1 if spare
    int frameScore; // final calculated score for this frame
};
typedef struct
{
    // One frame of bowling
    Frame frames[10];

    int totalScore;
} BowlingScoreboard;

// Returns 1 if frame completed, 0 if still in progress
bool addRoll(BowlingScoreboard *sb, int pins)
{
    // -------------------------------------------------------
    // 1. Locate active frame
    // -------------------------------------------------------
    int f = 0;
    for (; f < 10; f++)
    {
        Frame *fr = &sb->frames[f];

        // 10th frame complete?
        if (f == 9)
        {
            if (fr->roll3 != -1)
                continue;
            break;
        }

        // Normal frames 1–9
        if (fr->isStrike)
            continue; // strike ends frame
        if (fr->roll2 == -1)
            break; // still on roll2
    }

    Frame *fr = &sb->frames[f];
    bool frameComplete = false;

    // -------------------------------------------------------
    // 2. Apply roll to the correct slot
    // -------------------------------------------------------
    if (f < 9)
    {
        // -------- Frames 1–9 --------
        if (fr->roll1 == -1)
        {
            fr->roll1 = pins;
            fr->isStrike = (pins == 10);
            if (fr->isStrike)
            {
                fr->roll2 = 0; // convention
                frameComplete = true;
            }
        }
        else
        {
            fr->roll2 = pins;
            fr->isSpare = (fr->roll1 + fr->roll2 == 10);
            frameComplete = true;
        }
    }
    else
    {
        // -------- Frame 10 --------
        if (fr->roll1 == -1)
        {
            fr->roll1 = pins;
            fr->isStrike = (pins == 10);
            if (fr->isStrike) {
                fr->roll2 = 0; // convention
                frameComplete = true;
            }
        }
        else if (fr->roll2 == -1)
        {
            fr->roll2 = pins;
            fr->isSpare = (!fr->isStrike && fr->roll1 + pins == 10);

            if (!fr->isStrike && !fr->isSpare)
            {
                // No bonus ball
                fr->roll3 = -1;
                frameComplete = true;
            }

            // Note this will not end the game, just instruct it
            // to reset all pins.
            // There is a separate function to check if this is the end
            frameComplete = true;
        }
        else
        {
            // bonus third roll
            fr->roll3 = pins;
            frameComplete = true;
        }
    }

    // -------------------------------------------------------
    // 3. Recompute scores after EVERY roll
    // -------------------------------------------------------

    sb->totalScore = 0;

    for (int i = 0; i < 10; i++)
    {
        Frame *cf = &sb->frames[i];

        // If no first roll, frame not touched yet
        if (cf->roll1 == -1)
        {
            cf->frameScore = -1;
            continue;
        }

        // ----- 10th frame scoring -----
        if (i == 9)
        {
            int s = 0;
            if (cf->roll1 != -1)
                s += cf->roll1;
            if (cf->roll2 != -1)
                s += cf->roll2;
            if (cf->roll3 != -1)
                s += cf->roll3;
            cf->frameScore = s;
            sb->totalScore += s;
            continue;
        }

        // ----- Frames 1–9 -----
        if (cf->isStrike)
        {
            // Strike: 10 + next two rolls
            int bonus = 0;
            int rollsFound = 0;

            // Search next rolls in future frames
            for (int j = i + 1; j < 10 && rollsFound < 2; j++)
            {
                Frame *nf = &sb->frames[j];

                if (nf->roll1 != -1)
                {
                    bonus += nf->roll1;
                    rollsFound++;
                    if (rollsFound == 2)
                        break;
                }

                if (!nf->isStrike && nf->roll2 != -1)
                {
                    bonus += nf->roll2;
                    rollsFound++;
                    if (rollsFound == 2)
                        break;
                }

                // 10th frame: take roll3 if needed
                if (j == 9 && nf->roll3 != -1 && nf->isStrike)
                {
                    bonus += nf->roll3;
                    rollsFound++;
                }
            }

            if (rollsFound == 2)
            {
                cf->frameScore = 10 + bonus;
            }
            else
            {
                cf->frameScore = 0; // incomplete bonus
            }
        }
        else if (cf->isSpare)
        {
            // Spare: 10 + next roll
            int bonus = 0;
            bool ok = false;

            if (i + 1 < 10)
            {
                Frame *nf = &sb->frames[i + 1];

                if (nf->roll1 != -1)
                {
                    bonus = nf->roll1;
                    ok = true;
                }
            }

            cf->frameScore = ok ? (10 + bonus) : 0;
        }
        else
        {
            // Open frame
            if (cf->roll1 != -1 && cf->roll2 != -1)
                cf->frameScore = cf->roll1 + cf->roll2;
            else
                cf->frameScore = 0;
        }

        if (cf->frameScore != -1)
        {
            sb->totalScore += cf->frameScore;
        }
    }

    // -------------------------------------------------------
    // 4. Return 1 if frame is finished (reset pins)
    // -------------------------------------------------------
    return frameComplete;
}

void resetScoreboard(BowlingScoreboard &sb)
{
    sb.totalScore = 0;

    for (int i = 0; i < 10; ++i)
    {
        sb.frames[i].roll1 = -1;
        sb.frames[i].roll2 = -1;
        sb.frames[i].roll3 = -1;
        sb.frames[i].isStrike = 0;
        sb.frames[i].isSpare = 0;
        sb.frames[i].frameScore = -1;
    }
}

bool isGameFinished(const BowlingScoreboard *sb)
{
    // Frames 1–9
    for (int i = 0; i < 9; i++)
    {
        const Frame &f = sb->frames[i];

        if (f.roll1 == -1)
            return false; // not even bowled once
        if (f.isStrike)
            continue; // a strike completes the frame
        if (f.roll2 == -1)
            return false; // second roll missing
    }

    // Frame 10 rules
    const Frame &f10 = sb->frames[9];

    if (f10.roll1 == -1)
        return false;

    if (!f10.isStrike && !f10.isSpare)
    {
        // Normal frame, needs exactly two rolls
        return (f10.roll2 != -1);
    }
    else
    {
        // Strike or spare, third roll allowed/required
        return (f10.roll2 != -1 && f10.roll3 != -1);
    }
}

std::string textScoreboard(const BowlingScoreboard &sb)
{
    std::string out;
    out.reserve(512);

    out += "+-----------------------------------------+\n";
    out += "|           BOWLING SCOREBOARD            |\n";
    out += "+-----------------------------------------+\n";

    // Header row
    out += "|Frame |  R1  |  R2  |  R3  | Frame Score |\n";
    out += "+------+------+------+------+-------------+\n";

    for (int i = 0; i < 10; i++)
    {
        const Frame &f = sb.frames[i];

        // Convert rolls to symbols
        auto rollToStr = [&](int r)
        {
            if (r < 0)
                return std::string(" ");
            if (r == 10)
                return std::string("X"); // Strike
            return std::to_string(r);
        };

        std::string r1 = rollToStr(f.roll1);
        std::string r2 = rollToStr(f.roll2);
        std::string r3 = rollToStr(f.roll3);

        // Spares override R2 display (except 10th frame behaviour)
        if (f.isSpare == 1 && i < 9)
        {
            r2 = "/";
        }

        char buffer[256];
        if (f.frameScore < 0)
        {

            std::snprintf(buffer, sizeof(buffer),
                          "|  %2d  |  %-3s |  %-3s |  %-3s |             |\n",
                          i + 1, r1.c_str(), r2.c_str(), r3.c_str());
        }
        else
        {
            std::snprintf(buffer, sizeof(buffer),
                          "|  %2d  |  %-3s |  %-3s |  %-3s |    %3d      |\n",
                          i + 1, r1.c_str(), r2.c_str(), r3.c_str(), f.frameScore);
        }

        out += buffer;
    }

    out += "+-----------------------------------------+\n";

    {
        char buffer[128];
        std::snprintf(buffer, sizeof(buffer),
                      "| TOTAL SCORE: %3d                        |\n",
                      sb.totalScore);
        out += buffer;
    }

    out += "+-----------------------------------------+\n";

    return out;
}
#include <string>
#include <sstream>
#include <iomanip>

static std::string rollSymbol(int roll, int prev, bool isStrike, bool isSpare, bool secondRoll)
{
    if (roll < 0) return " ";

    if (isStrike && !secondRoll) return "X";
    if (secondRoll && isSpare)   return "/";

    if (roll == 0) return "0";

    return std::to_string(roll);
}

std::string textCompactScoreboard(const BowlingScoreboard *sb)
{
    std::ostringstream out;

    // Top border (9 normal frames + wide frame 10)
    out << "+";
    for (int i = 0; i < 9; i++) out << "---+";
    out << "-----+" << "\n";

    // First row: rolls
    out << "|";
    for (int i = 0; i < 10; i++) {
        const Frame &f = sb->frames[i];

        std::string r1 = rollSymbol(f.roll1, -1, f.isStrike, f.isSpare, false);
        std::string r2 = rollSymbol(f.roll2, f.roll1, f.isStrike, f.isSpare, true);
        std::string r3 = (i == 9 ? rollSymbol(f.roll3, f.roll2, false, false, false) : "-");

        if (i < 9) {
            // Normal frame (2 rolls)
            out << r1 << " " << r2 << "|";
        } else {
            // Frame 10 (3 rolls)
            out << r1 << " " << r2 << " " << r3 << "|";
        }
    }
    out << "\n";

    // Middle border
    out << "+";
    for (int i = 0; i < 9; i++) out << "---+";
    out << "-----+" << "\n";

    // Second row: cumulative score
    out << "|";
    for (int i = 0; i < 10; i++) {
        int sc = sb->frames[i].frameScore;

        if (sc == 0 && sb->frames[i].roll1 == -1) {
            // Empty frame
            if (i < 9) out << "   |";
            else       out << "     |";
        } else {
            if (i < 9)
                out << std::setw(3) << sc << "|";
            else
                out << std::setw(5) << sc << "|";
        }
    }
    out << "\n";

    // Third row: optional "bonus carry" values
    // (You can customise this however you want.)
    out << "|";
    for (int i = 0; i < 10; i++) {
        int bonus = 0;

        const Frame &f = sb->frames[i];
        if (i < 9) {
            if (f.isStrike) {
                bonus = f.frameScore - 10;
            } else if (f.isSpare) {
                bonus = f.frameScore - 10;
            }
        }

        if (bonus <= 0) {
            if (i < 9) out << "   |";
            else       out << "     |";
        } else {
            if (i < 9)
                out << std::setw(3) << bonus << "|";
            else
                out << std::setw(5) << bonus << "|";
        }
    }
    out << "\n";

    // Bottom border
    out << "+";
    for (int i = 0; i < 9; i++) out << "---+";
    out << "-----+";

    return out.str();
}

std::string textCompactScoreboardImproved(const BowlingScoreboard *sb)
{
    std::ostringstream out;

    auto safeRoll = [&](int r){ return r < 0 ? std::string("-") : std::to_string(r); };

    auto rollSymbol = [&](int roll, int prev, bool strike, bool spare, bool isSecond)->std::string {
        if (roll < 0) return "-";
        if (strike && !isSecond) return "X";
        if (isSecond && spare) return "/";
        return std::to_string(roll);
    };

    auto frameIsComplete = [&](int i)->bool {
        const Frame &f = sb->frames[i];
        if (i < 9) {
            if (f.roll1 == -1) return false;
            if (f.isStrike) {
                // need next two rolls
                if (i + 1 >= 10) return false;
                const Frame &n1 = sb->frames[i+1];
                if (n1.roll1 == -1) return false;
                if (n1.isStrike) {
                    if (i + 2 < 10) {
                        if (sb->frames[i+2].roll1 == -1) return false;
                    } else {
                        if (n1.roll2 == -1) return false;
                    }
                } else {
                    if (n1.roll2 == -1) return false;
                }
                return true;
            }
            if (f.roll2 == -1) return false;
            if (f.isSpare) {
                if (i+1 < 10 && sb->frames[i+1].roll1 != -1) return true;
                return false;
            }
            return true; // open frame with 2 rolls
        } else {
            // frame 10
            if (f.roll1 == -1) return false;
            if (f.isStrike || f.isSpare) {
                return f.roll3 != -1;
            } else {
                return f.roll2 != -1;
            }
        }
    };

    // Build cumulative totals (or -1 if unknown)
    int cumulative[10];
    int running = 0;
    for (int i = 0; i < 10; i++) {
        if (!frameIsComplete(i)) {
            cumulative[i] = -1;
        } else {
            running += sb->frames[i].frameScore;
            cumulative[i] = running;
        }
    }

    // -------- TOP BORDER --------
    out << "+";
    for (int i = 0; i < 9; i++) out << "---+";
    out << "-----+\n";

    // -------- FIRST ROW (ROLLS) --------
    out << "|";
    for (int i = 0; i < 10; i++) {
        const Frame &f = sb->frames[i];

        std::string r1 = rollSymbol(f.roll1, -1, f.isStrike, f.isSpare, false);
        std::string r2 = rollSymbol(f.roll2, f.roll1, f.isStrike, f.isSpare, true);
        std::string r3 = (i == 9 ? (
            f.roll3 < 0
                ? "-"
                : rollSymbol(f.roll3, f.roll2, f.roll3 == 10, false, false)
            ) : "-");

        if (i < 9)
            out << r1 << " " << r2 << "|";
        else
            out << r1 << " " << r2 << " " << r3 << "|";
    }
    out << "\n";

    // -------- MIDDLE BORDER --------
    out << "+";
    for (int i = 0; i < 9; i++) out << "---+";
    out << "-----+\n";

    // -------- SECOND ROW (CUMULATIVE TOTALS) --------
    out << "|";
    for (int i = 0; i < 10; i++) {
        if (cumulative[i] < 0) {
            if (i < 9) out << "   |";
            else out << "     |";
        } else {
            if (i < 9)
                out << std::setw(3) << cumulative[i] << "|";
            else
                out << std::setw(5) << cumulative[i] << "|";
        }
    }
    out << "\n";

    // -------- THIRD ROW (BONUS, only when known) --------
    out << "|";
    for (int i = 0; i < 10; i++) {
        const Frame &f = sb->frames[i];

        // no bonus in frame 10
        if (i == 9) { out << "     |"; continue; }

        int bonus = -1;
        if (frameIsComplete(i)) {
            if (f.isStrike) bonus = f.frameScore - 10;
            else if (f.isSpare) bonus = f.frameScore - 10;
        }

        if (bonus < 0)
            out << "   |";
        else
            out << std::setw(3) << bonus << "|";
    }
    out << "\n";

    // -------- BOTTOM BORDER --------
    out << "+";
    for (int i = 0; i < 9; i++) out << "---+";
    out << "-----+";

    return out.str();
}