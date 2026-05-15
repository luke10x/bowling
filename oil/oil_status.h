#pragma once

// Lightweight data for the Oil Status window UI.
// Defined outside game.cpp so WindowStack can accept it without including UserContext.
struct OilStatusUI
{
    float laneFriction = 0.0f;
    float lanePushbackStrength = 0.0f;

    float houseOilThickness = 0.0f;
    float currentOilThickness = 0.0f;

    float leftOilFadeStartM = 0.0f;
    float leftOilFadeEndM = 0.0f;
    float rightOilFadeStartM = 0.0f;
    float rightOilFadeEndM = 0.0f;

    float oilWearLeftM = 0.0f;
    float oilWearRightM = 0.0f;
    float oilWearTotalM = 0.0f;

    float oilCarrydownPerBallTravelM = 0.0f;
    float oilThicknessDecayPerBallTravel = 0.0f;

    float estCarryStartLeftM = 0.0f;
    float estCarryStartRightM = 0.0f;
    float estThicknessDrop = 0.0f;

    // Re-oil pricing/UI (normal game uses $10; school oil lesson uses free re-oil).
    float reoilCost = 10.0f;

    // Optional lesson-mode gating for RE-OIL.
    // If `reoilEnabled` is false, the RE-OIL button is disabled and `reoilDisabledLabel`
    // is shown (if provided).
    bool reoilEnabled = true;
    const char *reoilDisabledLabel = nullptr;

    // Optional lesson progress shown in the window (e.g. "Re-oils: 1/3").
    int lessonReoilCount = 0;
    int lessonReoilNeeded = 0;
};
