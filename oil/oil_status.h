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
};

