#include <chrono>
#include <cmath>
#include <iostream>
#include <random>
#include <stdint.h>
#include <stdio.h>
#include <string.h> // for memcpy, strcmp
#include <thread>
#include <utility>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "framework/boot.h"

#include "all_assets.h"
#include "aurora.h"
#include "circlegest.h"
#include "clayton/claytheme.h"
#include "clayton/clayton.h"
#include "clayton/clayton_click.h"
#include "clayton/keypad.h"
#include "clayton/shop_clay.h"
#include "clayton/win_stack.h"
#include "coins.h"
#include "decal.h"
#include "fpscounter.h"
#include "hiscore/hiscore_clay.h"
#include "hiscore/localhi.h"
#include "hooker.h"
#include "joystick.h"
#include "mesh.h"
#include "mod_imgui.h"
#include "oil/oilmap.h"
#include "ortho3d.h"
#include "physics/physics.h"
#include "rendertexture.h"
#include "score.h"
#include "shop.h"
#include "shop/flying_coins_helper.h"
#include "sounds/adaptive_audio.h"
#include "sounds/adaptive_clay.h"
#include "sounds/sound_clay.h"
#include "sounds/sounds.h"
#include "storage.h"
#include "stubs.h"
#include "transition.h"
#include "tritest.h"
#include "tween.h"
#include "window.h"

#define ZONE(x) ;

#ifndef ASSET_PATH
#if defined(__ANDROID__) || defined(ANDROID)
#define ASSET_PATH "files/"
#elif TARGET_OS_IPHONE
#define ASSET_PATH "" // iOS
#elif TARGET_OS_OSX
#define ASSET_PATH "assets/files/" // macOS
#else
#define ASSET_PATH "assets/files/"
#endif
#endif

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

using Clock = std::chrono::high_resolution_clock;
using TimePoint = std::chrono::time_point<Clock>;
using Seconds = std::chrono::duration<double>;

struct SceneTunables
{
    float pivotY = 1.30f;
    float pivotZ = -18.90f;
    // Release plane offset is derived:
    // offset = ropeLen * releaseOffsetFracMax * (1 - releaseBuff^2)
    float releaseOffsetFracMax = 0.15f;
    float releaseOffsetZ = 0.0f; // derived, for debug/UI

    // Where the ball sits in IDLE (and where dead-swing forgiveness snaps it back to).
    // Defined relative to pivot so moving the pivot doesn't break the early-game flow.
    float idleBallY = 0.3f;
    float idleBallOffsetZFromPivot = 1.8f;

    float camEyeY = 0.9f;
    float camTargetY = -1.0f;
    float camEyeZFromBall = -1.9f;
    float camTargetZFromBall = 4.2f;

    // Camera Z clamps (world-space). Lane is around z=-18 (player end) toward z=0 (pins end).
    float camEyeZMin = -22.0f;
    float camEyeZMax = -3.0f;
    float camTargetZMin = -13.0f;
    float camTargetZMax = 2.0f;
};

static inline glm::vec3 Scene_IdleBallPos(const SceneTunables &s)
{
    return glm::vec3(0.0f, s.idleBallY, s.pivotZ + s.idleBallOffsetZFromPivot);
}

static inline float Scene_ComputeReleaseOffsetZ(const SceneTunables &s, float ropeLen, float releaseBuff01)
{
    float buff = glm::clamp(releaseBuff01, 0.0f, 1.0f);
    // Inverted mapping with cutoff:
    // - buff <= 0.0 => max offset
    // - buff >= 0.5 => zero offset (release as soon as pivot Z is crossed)
    float inv = glm::clamp((0.5f - buff) / 0.5f, 0.0f, 1.0f);
    float invSq = inv * inv;
    float frac = glm::clamp(s.releaseOffsetFracMax, 0.0f, 0.5f);
    float maxOffset = glm::max(0.0f, ropeLen) * frac;
    return maxOffset * invSq;
}

static inline SceneTunables SceneTunables_Default()
{
    return SceneTunables{};
}

struct UserContext
{
    enum class Phase
    {
        IDLE,
        AIM,
        SWING,
        THROW,
        RESULT,
        FINAL_RESULT,
        MENU
    };

    enum class PhaseTrans
    {
        TRANS_NONE,
        TRANS_IDLE_TO_AIM,
        TRANS_AIM_TO_SWING,
        TRANS_AIM_TO_THROW,
        TRANS_SWING_TO_AIM,
        TRANS_SWING_TO_THROW,
    };

    Phase phase = Phase::IDLE;
    glm::vec3 aimStart;
    glm::vec3 aimCurr;

    bool fuckCakez = true;
	Aurora aurora;
	OilMap oilMap;
    Tween<float> auroraVibe;
    FpsCounter fpsCounter;
    uint64_t lastFrameTime = 0;
    TimePoint last = Clock::now();
    uint64_t totalFrames = 0;
    ModImgui imgui;

    float throwingTime;
    float settlingTime;
    float aimingTime;

    ShaderProgram mainShader;
    Texture everythingTexture;
    SimpleShaderProgram simpleShader;

    AssetMesh ballMesh;
    AssetMesh laneMesh;
    AssetMesh pinMesh;
    AssetMesh starMesh;

    glm::mat4 cameraMat;
    glm::mat4 perspectiveMat;
    glm::mat4 orthographicMat;

    Physics phy;

    glm::vec3 initialPins[10];
    glm::vec3 ballStart;

    float launchSpeed;
    float endSpeed;
    glm::vec3 lastBallPosition;
    glm::vec2 aimFlatPos;
    glm::vec2 asd;
    // Used to detect "tap" releases that should cancel instead of throwing.
    glm::vec2 aimDownFlatPos = glm::vec2(0.0f);
    glm::vec3 catchupSpeed;
    glm::vec3 catchupDirection;
    glm::vec3 carriedVel;
    // Track how far back the ball ever got during AIM; used for "kids throw" detection.
    float aimMaxPullbackMeters = 0.0f;
    // Track minimum joystick Y during AIM; if the user never pulled back (down), we can
    // still treat some releases as "kids throws" even if the ball drifted backward.
    float aimMinNdcY = 0.0f;
    // Track the maximum "downward" delta (SDL y increases downward) during AIM relative to the
    // initial touch point. Used to detect upward-only/sideways swipes robustly (noise-tolerant).
    float aimMaxDownDeltaNdc = 0.0f;
    float totalSpinAngle;
    float spinSpeed;
    SpinTracker st;
    SceneTunables scene = SceneTunables_Default();
    glm::vec3 pivotPoint = glm::vec3(0.0f, 1.15f, -18.60f);
    glm::vec3 joystick;
    Joystick enjoy;
    glm::vec3 desiredBall;
    glm::vec3 carriedBall;
    glm::vec3 undesiredMovement = glm::vec3(0.0f);
	glm::vec3 swingMovement = glm::vec3(0.0f);
	glm::vec3 swingPreviousFramePoint = glm::vec3(0.0f);
	float swingStallTime = 0.0f;
	// Forgiveness tracking for THROW (cancels that should not count as a roll).
	bool throwEverAboveLane = false;
	bool debugForgiveness = false;
	float swingingTime;
	float highestPoint;

    BowlingScoreboard board;
    int wereDead;
    Clayton clayton;
    WindowStack windowStack;
    bool shouldShowClayDebug;
    bool shouldShowImgui;

    Transition trans;
    Circle circle;
    bool bufferedRequestThrow = false;
    float deltaTimeLoan = 0.0f;
    float deltaTimeSum = 0.0f;
    DecalBatch decalBatch;

    char username[20];
    int32_t username_len;
    Keypad keypad;
    Clayton_Click renameButton;
    Clayton_Click menuButton;
    Clayton_Click soundButton;
    Clayton_Click oilButton;
    Clayton_Click hiScoreButton;

	// TUNABLET entries
	float speedBoostAtThrow = 2.0f;
	float angularFactor = 0.15f;
	float smashingPower = 10.0f;
	float desiredMass = 7.25f;
	float ballBaseFriction = 0.0f;
	float ballSkid = 0.0f;
	float ballSkidStartScale = 1.0f;
	float laneFriction = 0.05f;
	float lanePushbackStrength = 15.0f;
	// Asymmetric oil cover: per-side fade start/end in meters from lane start (LANE_Z_START).
	// We expose End first then Start in ImGui to match perspective view (pins are "forward").
	float leftOilFadeEndM = 13.3f;
	float rightOilFadeEndM = 13.3f;
	float leftOilFadeStartM = 8.3f;
	float rightOilFadeStartM = 8.3f;
	float laneOilThickness = 1.0f; // 0..1, scales how slippery the oil zone starts
	// Oil wear/carrydown (per throw).
	float oilWearLeftM = 0.0f;
	float oilWearRightM = 0.0f;
	float oilWearTotalM = 0.0f;
		// Tunables: how much carrydown (meters) per meter travelled; and how much oil thickness decays per meter.
		float oilCarrydownPerBallTravelM = 0.01f;
		float oilThicknessDecayPerBallTravel = 0.001f;

		// House defaults (initial lane state for the current "house").
		struct HouseLaneParams
		{
			float laneFriction;
			float lanePushbackStrength;
			float laneOilThickness;
			float leftOilFadeStartM;
			float leftOilFadeEndM;
			float rightOilFadeStartM;
			float rightOilFadeEndM;
			float oilCarrydownPerBallTravelM;
			float oilThicknessDecayPerBallTravel;
		};
		HouseLaneParams houseLane = {
			0.05f,
			15.0f,
			1.0f,
			8.3f,
			13.3f,
			8.3f,
			13.3f,
			0.01f,
			0.001f,
		};
	glm::vec3 releaseOrbitAngularVel = glm::vec3(0.0f);
	glm::vec3 orbitPrevDir = glm::vec3(0.0f);
	bool orbitHasPrev = false;
	glm::vec3 prevBallPosForRelease = glm::vec3(0.0f);
	bool hasPrevBallPosForRelease = false;
	glm::quat prevBallRotForRelease = glm::quat(1.0f, 0, 0, 0);
	bool hasPrevBallRotForRelease = false;
	glm::vec3 releaseSpinFromRot = glm::vec3(0.0f);
	bool isMouseDownInThrow;
	bool lastPointerWasTouch = false;
	int touchRelDx = 0;
	int touchRelDy = 0;

    MiniTriangle tri;
    Storage storage;

    GameSoundSystem sound;
    AdaptiveAudioSystem adaptiveAudio;
    LocalHighscore localHi;
    bool wasMutedForMonitoring;

    int wavExportWaitFrames = 0;
    const char *wavExportSongPattern = nullptr;
    uint32_t wavExportResumeTime = 0; // SDL_GetTicks64() when to resume

    // Click handlers
    // Clayton_Click buyClicks[];

    // Shop Clicks
    Clayton_Click openShopClick;

    bool shouldShowShop = false;

    int numberOfBallsHit;

    CoinLane coinLane;

    float globalTime = 0.0f;
    int clearedCoins = 0; // Track coin pickups for SFX

    glm::vec2 placeOfMoney = glm::vec2(0.0f);
    int hudAboveThis = 0;

    CarouselState carousel;

	RenderTexture ballRenderTex;
	RenderTexture ballRenderTex2;
	RenderTexture oilRenderTex;

    CatalogItem myBall;
    CatalogItem imguiBall;
    int sectors;

    // Spin params
    glm::vec2 prevDir = glm::vec2(1.0f, 0.0f);
    float totalAngle = 0.0f;
    float angularVelocity = 0.0f;
    float smoothedAngularVelocity = 0.0f;
    int circles = 0;
};

static inline const char *PhaseName(UserContext::Phase p)
{
    switch (p)
    {
    case UserContext::Phase::IDLE: return "IDLE";
    case UserContext::Phase::AIM: return "AIM";
    case UserContext::Phase::SWING: return "SWING";
    case UserContext::Phase::THROW: return "THROW";
    case UserContext::Phase::RESULT: return "RESULT";
    case UserContext::Phase::FINAL_RESULT: return "FINAL_RESULT";
    case UserContext::Phase::MENU: return "MENU";
    }
    return "?";
}

static inline void LogToIdle(UserContext *usr, const char *reason)
{
    const glm::vec3 ball = usr->carriedBall;
    const glm::vec3 pivot = usr->pivotPoint;
    float releasePlaneZ = pivot.z + usr->scene.releaseOffsetZ;
    glm::vec3 idlePos = Scene_IdleBallPos(usr->scene);
    std::cerr << "[IDLE-RESET] reason=" << reason << " from=" << PhaseName(usr->phase)
              << " ball=(" << ball.x << "," << ball.y << "," << ball.z << ")"
              << " pivot=(" << pivot.x << "," << pivot.y << "," << pivot.z << ")"
              << " releasePlaneZ=" << releasePlaneZ
              << " idlePos=(" << idlePos.x << "," << idlePos.y << "," << idlePos.z << ")"
              << " aimingTime=" << usr->aimingTime
              << " swingTime=" << usr->swingingTime
              << " stallTime=" << usr->swingStallTime
              << std::endl;
}

static inline void ApplyHouseLaneParams(UserContext *usr)
{
	if (!usr)
		return;
	usr->laneFriction = usr->houseLane.laneFriction;
	usr->lanePushbackStrength = usr->houseLane.lanePushbackStrength;
	usr->laneOilThickness = usr->houseLane.laneOilThickness;
	usr->leftOilFadeStartM = usr->houseLane.leftOilFadeStartM;
	usr->leftOilFadeEndM = usr->houseLane.leftOilFadeEndM;
	usr->rightOilFadeStartM = usr->houseLane.rightOilFadeStartM;
	usr->rightOilFadeEndM = usr->houseLane.rightOilFadeEndM;
	usr->oilCarrydownPerBallTravelM = usr->houseLane.oilCarrydownPerBallTravelM;
	usr->oilThicknessDecayPerBallTravel = usr->houseLane.oilThicknessDecayPerBallTravel;

	usr->oilWearLeftM = 0.0f;
	usr->oilWearRightM = 0.0f;
	usr->oilWearTotalM = 0.0f;
}

void vtx::hang(vtx::VertexContext *ctx)
{
    UserContext *usr = static_cast<UserContext *>(ctx->usrptr);
    usr->imgui.hangImgui(ctx);
    // TODO I guess it is leaking memory, but I can live with that in dev build
}

void vtx::load(vtx::VertexContext *ctx)
{
    UserContext *usr = static_cast<UserContext *>(ctx->usrptr);

    usr->imgui.loadImgui(ctx);
    usr->aurora.loadAuroraShader();
    usr->auroraVibe.value = 0.0f;
    usr->circle.loadCircleShaderProgram();
    usr->clayton.initClayton(ctx->screenWidth, ctx->screenHeight);
    usr->windowStack.windowStackInit();
    usr->decalBatch.loadDecalBatchShader();

    setupStubScoreboardMax(&usr->board);

    Carousel_SetupDefaultShop(&usr->carousel);
}

// todo this is shit  but ok for now
// ball_stats.cpp
// ball_stats_config.h
struct BallPhysicsMapping
{
    // Catalog value ranges (from your g_ballCatalog)
    static constexpr float CATALOG_MASS_MIN = 0.1f;
    static constexpr float CATALOG_MASS_MAX = 0.60f;
    static constexpr float CATALOG_SPIN_MIN = 0.0f;
    static constexpr float CATALOG_SPIN_MAX = 1.0f;
    static constexpr float CATALOG_SKID_MIN = 0.0f;
    static constexpr float CATALOG_SKID_MAX = 1.0f;
    static constexpr float CATALOG_BITE_MIN = 0.0f;
    static constexpr float CATALOG_BITE_MAX = 1.0f;
    static constexpr float CATALOG_BUFF_MIN = 0.0f;
    static constexpr float CATALOG_BUFF_MAX = 1.0f;

    // Target physics/gameplay ranges (from your ImGui sliders)
    static constexpr float PHYSICS_MASS_MIN = 2.5f;
    static constexpr float PHYSICS_MASS_MAX = 8.0f;
    static constexpr float PHYSICS_SPIN_MIN = 0.1f;
    static constexpr float PHYSICS_SPIN_MAX = 0.75f;
    static constexpr float PHYSICS_SMASH_MIN = 5.0f;
    static constexpr float PHYSICS_SMASH_MAX = 50.0f;
    static constexpr float PHYSICS_SPEEDBOOST_MIN = 0.5f;
    static constexpr float PHYSICS_SPEEDBOOST_MAX = 5.0f;
    static constexpr float PHYSICS_FRICTION_MIN = 0.0f;
    static constexpr float PHYSICS_FRICTION_MAX = 0.15f;

    // Tunable multipliers for fine-tuning feel
    static constexpr float SPIN_MULTIPLIER = 1.0f;
    static constexpr float BITE_TO_FRICTION_SCALE = 1.0f;
};

// Central place to tune how skid/bite turn into ball friction.
struct BallFrictionTuning
{
    // Lane distance used for friction progression.
    static constexpr float LANE_Z_START = -18.3f;
    static constexpr float LANE_Z_END = -5.0f;

    // Base friction coming from bite (higher bite => higher friction).
    // Note: this is applied to the BALL body (lane friction stays constant).
    static constexpr float BALL_FRICTION_MIN = 0.0f;
    static constexpr float BALL_FRICTION_MAX = 0.60f;
    static constexpr float BITE_EXPONENT = 2.0f;

    // Skid controls how long the ball "slides" before full bite friction applies.
    // Skid stays fully slippery until SKID_FADE_START_Z, then smoothly fades out,
    // and at SKID_FADE_END_Z skid has *no* effect (friction is only from bite).
    static constexpr float SKID_FADE_EASE_EXP = 2.5f; // >1 keeps it slippery longer, then ramps late

    // At the start of the lane (t=0), we still allow some friction.
    // Larger skid => smaller start scale => less early friction.
    static constexpr float SKID_START_SCALE_LOW_SKID = 0.55f;
    static constexpr float SKID_START_SCALE_HIGH_SKID = 0.0f;

    // Extra skid effect when the ball is far from the lane center (x != 0).
    // This helps you "see" skid when throwing wide lines.
    static constexpr float LANE_HALF_WIDTH_M = (41.857f * 0.0254f) * 0.5f;
    static constexpr float SKID_EDGE_X_START = 0.12f; // start applying near outside boards
    static constexpr float SKID_EDGE_X_END = 0.48f;   // near gutter
    static constexpr float SKID_EDGE_MULT_AT_EDGE = 0.05f; // really low friction at edge

    // For asymmetric oil zones: outside of this margin from lane edge, only that side's oil applies.
    static constexpr float OIL_BLEND_GUTTER_MARGIN_M = 0.06f;

    static constexpr bool PUSHBACK_ENABLED = true;
};

inline float smoothstep(float edge0, float edge1, float x)
{
    float t = glm::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

static inline glm::quat quatFromToSafe(glm::vec3 from, glm::vec3 to)
{
    float fromLen2 = glm::dot(from, from);
    float toLen2 = glm::dot(to, to);
    if (fromLen2 < 1e-12f || toLen2 < 1e-12f)
        return glm::quat(1.0f, 0, 0, 0);

    from *= glm::inversesqrt(fromLen2);
    to *= glm::inversesqrt(toLen2);

    float d = glm::clamp(glm::dot(from, to), -1.0f, 1.0f);
    if (d > 0.9999f)
        return glm::quat(1.0f, 0, 0, 0);

    if (d < -0.9999f)
    {
        // 180 degree turn; pick an arbitrary orthogonal axis.
        glm::vec3 axis = glm::cross(from, glm::vec3(1.0f, 0.0f, 0.0f));
        if (glm::dot(axis, axis) < 1e-6f)
            axis = glm::cross(from, glm::vec3(0.0f, 0.0f, 1.0f));
        axis = glm::normalize(axis);
        return glm::angleAxis(glm::pi<float>(), axis);
    }

    glm::vec3 axis = glm::cross(from, to);
    float s = sqrtf((1.0f + d) * 2.0f);
    float invS = 1.0f / s;
    return glm::normalize(glm::quat(s * 0.5f, axis.x * invS, axis.y * invS, axis.z * invS));
}

static inline glm::vec3 angularVelocityFromDelta(glm::quat deltaRot, float dt)
{
    if (dt <= 1e-6f)
        return glm::vec3(0.0f);
    {
        glm::vec3 v(deltaRot.x, deltaRot.y, deltaRot.z);
        if (glm::dot(v, v) < 1e-10f)
            return glm::vec3(0.0f);
    }

    float w = glm::clamp(deltaRot.w, -1.0f, 1.0f);
    float angle = 2.0f * acosf(w);
    glm::vec3 axis(deltaRot.x, deltaRot.y, deltaRot.z);
    float axisLen = glm::length(axis);
    if (axisLen < 1e-8f || angle < 1e-6f)
        return glm::vec3(0.0f);
    axis /= axisLen;
    return axis * (angle / dt);
}

static inline float softCapTanh(float x, float cap)
{
    if (cap <= 1e-6f)
        return x;
    return cap * tanhf(x / cap);
}

struct BallSwingTuning
{
    // How much of the pendulum/orbital angular velocity becomes initial spin at release.
    static constexpr float RELEASE_ORBIT_SPIN_SCALE = 0.20f;
    static constexpr float RELEASE_ORBIT_SPIN_MAX = 20.0f; // rad/s

    // Soft cap for swipe-derived angular velocity (prevents crazy spins on low FPS / fast swipes).
    static constexpr float INPUT_ANGVEL_SOFTCAP = 4.0f;

    // Bite adds extra "drive" when you are trying to reverse hook direction while the ball is
    // still moving laterally the other way (vx). This is applied in THROW when spin input
    // and current vx disagree.
    static constexpr float BITE_DRIVE_FROM_LATERAL_VEL = 5.0f;
};

// (SceneTuning lives near the top of the file as the single source of truth.)
// utils/math_helpers.h or similar
inline float remapClamped(float value, float inMin, float inMax, float outMin, float outMax)
{
    float t = (value - inMin) / (inMax - inMin); // No clamp → allows extrapolation
    return glm::mix(outMin, outMax, t);          // ← glm::mix
}

// Optional: exponential remap for non-linear feel (great for spin/bite)
inline float remapExponential(
    float value, float inMin, float inMax, float outMin, float outMax, float exponent = 2.0f
)
{
    float t = glm::clamp((value - inMin) / (inMax - inMin), 0.0f, 1.0f);
    t = powf(t, exponent); // Curve the interpolation
    return outMin + t * (outMax - outMin);
}

// Log-like remap: fast early growth, smooth approach to ceiling.
inline float remapLogarithmic(float value, float inMin, float inMax, float outMin, float outMax, float k = 9.0f)
{
    float t = glm::clamp((value - inMin) / (inMax - inMin), 0.0f, 1.0f);
    k = glm::max(k, 0.0001f);
    float lt = log1pf(k * t) / log1pf(k);
    return outMin + lt * (outMax - outMin);
}

void BallStats_ApplyCatalog(UserContext *usr, const CatalogItem &ball)
{
    // Mass: linear remap
    usr->desiredMass = remapClamped(
        ball.mass,
        BallPhysicsMapping::CATALOG_MASS_MIN,
        BallPhysicsMapping::CATALOG_MASS_MAX,
        BallPhysicsMapping::PHYSICS_MASS_MIN,
        BallPhysicsMapping::PHYSICS_MASS_MAX
    );
    usr->phy.set_ball_mass(usr->desiredMass);

    // Spin factor: logarithmic for strong low-end effect and a smooth cap near the ceiling.
    usr->angularFactor = remapLogarithmic(
                             ball.spin,
                             BallPhysicsMapping::CATALOG_SPIN_MIN,
                             BallPhysicsMapping::CATALOG_SPIN_MAX,
                             BallPhysicsMapping::PHYSICS_SPIN_MIN,
                             BallPhysicsMapping::PHYSICS_SPIN_MAX,
                             12.0f
                         ) *
        BallPhysicsMapping::SPIN_MULTIPLIER;

    // Smashing power: map from hitBuff/launchBuff average
    float buffAvg = (ball.launchBuff + ball.hitBuff) * 0.5f;
    usr->smashingPower = remapClamped(
        buffAvg,
        BallPhysicsMapping::CATALOG_BUFF_MIN,
        BallPhysicsMapping::CATALOG_BUFF_MAX,
        BallPhysicsMapping::PHYSICS_SMASH_MIN,
        BallPhysicsMapping::PHYSICS_SMASH_MAX
    );

    // Speed boost at throw: map from launchBuff
    usr->speedBoostAtThrow = remapClamped(
        ball.launchBuff,
        BallPhysicsMapping::CATALOG_BUFF_MIN,
        BallPhysicsMapping::CATALOG_BUFF_MAX,
        BallPhysicsMapping::PHYSICS_SPEEDBOOST_MIN,
        BallPhysicsMapping::PHYSICS_SPEEDBOOST_MAX
    );

    // Release timing is derived at runtime from rope length + launchBuff.

    // Precompute friction curve params from 'bite' and 'skid'
    usr->ballSkid = glm::clamp(ball.skid, 0.0f, 1.0f);

    usr->ballBaseFriction = remapExponential(
                                ball.bite,
                                BallPhysicsMapping::CATALOG_BITE_MIN,
                                BallPhysicsMapping::CATALOG_BITE_MAX,
                                BallFrictionTuning::BALL_FRICTION_MIN,
                                BallFrictionTuning::BALL_FRICTION_MAX,
                                BallFrictionTuning::BITE_EXPONENT
                            ) *
        BallPhysicsMapping::BITE_TO_FRICTION_SCALE;

    usr->ballSkidStartScale = remapClamped(
        usr->ballSkid,
        BallPhysicsMapping::CATALOG_SKID_MIN,
        BallPhysicsMapping::CATALOG_SKID_MAX,
        BallFrictionTuning::SKID_START_SCALE_LOW_SKID,
        BallFrictionTuning::SKID_START_SCALE_HIGH_SKID
    );

    // Store radius for any radius-dependent calculations
    // usr->ballRadius = ball.radius;
}
void BallStats_OnBallChange(const CatalogItem *ball, UserContext *usr)
{

    static_assert(
        std::is_trivially_copyable_v<CatalogItem>,
        "CatalogItem must be trivially copyable for memcpy"
    );

    std::memcpy(&usr->myBall, ball, sizeof(CatalogItem));
    std::memcpy(&usr->imguiBall, ball, sizeof(CatalogItem));

    BallStats_ApplyCatalog(usr, *ball);
}
void BallStats_EveryFrame(UserContext *usr, glm::mat4 ballModel)
{
    // === Ball Friction Progression (skid → early slide, bite → max friction) ===
    {
        float z = ballModel[3].z;
        float x = ballModel[3].x;
        // Asymmetric oil fade ranges: blend left/right based on ball X.
        float leftStartM = usr->leftOilFadeStartM;
        float leftEndM = usr->leftOilFadeEndM;
        float rightStartM = usr->rightOilFadeStartM;
        float rightEndM = usr->rightOilFadeEndM;
        if (leftStartM > leftEndM)
            std::swap(leftStartM, leftEndM);
        if (rightStartM > rightEndM)
            std::swap(rightStartM, rightEndM);

        float oilBlendX = BallFrictionTuning::LANE_HALF_WIDTH_M - BallFrictionTuning::OIL_BLEND_GUTTER_MARGIN_M;
        oilBlendX = glm::max(0.01f, oilBlendX);
        // Note: our world X is mirrored vs "lane left/right" in camera perspective.
        // Treat +X as LEFT and -X as RIGHT for oil asymmetry.
        float oilSideT = glm::clamp(((-x) + oilBlendX) / (2.0f * oilBlendX), 0.0f, 1.0f); // 0=left, 1=right

        float oilStartM = glm::mix(leftStartM, rightStartM, oilSideT);
        float oilEndM = glm::mix(leftEndM, rightEndM, oilSideT);

        const float zFadeStart = BallFrictionTuning::LANE_Z_START + oilStartM;
        const float zFadeEnd = BallFrictionTuning::LANE_Z_START + oilEndM;

        // Skid stays fully slippery at the beginning, then smoothly fades out;
        // at zFadeEnd skid has no effect and friction is only from bite.
        float denom = (zFadeEnd - zFadeStart);
        float fadeT = (denom > 1e-6f) ? ((z - zFadeStart) / denom) : 1.0f;
        if (!std::isfinite(fadeT))
            fadeT = 1.0f;
        fadeT = glm::clamp(fadeT, 0.0f, 1.0f);
        float skidRamp = smoothstep(0.0f, 1.0f, fadeT);
        skidRamp = powf(skidRamp, BallFrictionTuning::SKID_FADE_EASE_EXP);

        // Oil thickness scales how slippery the oil zone starts:
        // - thickness=1: fully use skidStartScale (most slippery)
        // - thickness=0: ignore skidStartScale (no extra slipperiness from oil)
        float oilT = glm::clamp(usr->laneOilThickness, 0.0f, 1.0f);
        float startScale = glm::mix(1.0f, usr->ballSkidStartScale, oilT);

        float skidMultiplier = glm::mix(startScale, 1.0f, skidRamp);

        // When the ball is far from the center, exaggerate skid early so it is visually obvious.
        // This only applies inside the skid zone; by the end (skidRamp->1), effect becomes 0.
        float absX = glm::abs(x);
        float edgeT = smoothstep(
            BallFrictionTuning::SKID_EDGE_X_START, BallFrictionTuning::SKID_EDGE_X_END, absX
        );
        float edgeMult = glm::mix(1.0f, BallFrictionTuning::SKID_EDGE_MULT_AT_EDGE, edgeT);
        float skidZoneStrength = (1.0f - skidRamp);
        skidMultiplier *= glm::mix(1.0f, edgeMult, skidZoneStrength);

        float currentFriction = usr->ballBaseFriction * skidMultiplier;
        currentFriction = glm::clamp(currentFriction, 0.0f, BallFrictionTuning::BALL_FRICTION_MAX);

        usr->phy.set_ball_friction(currentFriction);
    }

    // Lane friction + pushback are lane-level tunables (shown in ImGui).
    usr->phy.apply_friction_to_lane(glm::max(0.0f, usr->laneFriction));
    {
        float x = ballModel[3].x;
        float leftStartM = usr->leftOilFadeStartM;
        float leftEndM = usr->leftOilFadeEndM;
        float rightStartM = usr->rightOilFadeStartM;
        float rightEndM = usr->rightOilFadeEndM;
        if (leftStartM > leftEndM)
            std::swap(leftStartM, leftEndM);
        if (rightStartM > rightEndM)
            std::swap(rightStartM, rightEndM);

        float oilBlendX = BallFrictionTuning::LANE_HALF_WIDTH_M - BallFrictionTuning::OIL_BLEND_GUTTER_MARGIN_M;
        oilBlendX = glm::max(0.01f, oilBlendX);
        float oilSideT = glm::clamp(((-x) + oilBlendX) / (2.0f * oilBlendX), 0.0f, 1.0f);
        float oilEndM = glm::mix(leftEndM, rightEndM, oilSideT);
        float zFadeEnd = BallFrictionTuning::LANE_Z_START + oilEndM;

        // Pushback follows oil concentration: strong at oil start, fades out as oil wears out.
        float maxStrength = glm::max(0.0f, usr->lanePushbackStrength) * glm::clamp(usr->laneOilThickness, 0.0f, 1.0f);
        usr->phy.set_lane_pushback_oil_profile(
            BallFrictionTuning::LANE_Z_START,
            zFadeEnd,
            maxStrength,
            BallFrictionTuning::SKID_FADE_EASE_EXP,
            BallFrictionTuning::PUSHBACK_ENABLED
        );
    }

    // === Spin & Angular Velocity (Only during active throw) ===
		    if (usr->phase == UserContext::Phase::THROW)
		    {
		        if (glm::abs(usr->circles) >= 1)
		        {
		            static bool s_warnedNonFiniteAngVel = false;
		            float rawAngVel = usr->smoothedAngularVelocity;
		            if (!std::isfinite(rawAngVel))
		            {
		                if (!s_warnedNonFiniteAngVel)
		                {
		                    std::cerr << "[throw] Non-finite smoothedAngularVelocity: " << usr->smoothedAngularVelocity
		                              << " (forcing 0)\n";
		                    s_warnedNonFiniteAngVel = true;
		                }
		                rawAngVel = 0.0f;
		            }

		            float angVel = softCapTanh(rawAngVel, BallSwingTuning::INPUT_ANGVEL_SOFTCAP);

		            // Base spin from input (yaw angular velocity).
		            float sideDrive = -angVel * (usr->myBall.spin * usr->myBall.spin) * 0.125f;

		            // Bite "drive": if ball is still sliding laterally opposite to where the
		            // current spin input wants to take it, add extra drive to flip direction sooner.
		            glm::vec3 v = usr->phy.get_ball_swing_movement();
		            float vx = std::isfinite(v.x) ? v.x : 0.0f;
		            float bite01 = glm::clamp(usr->myBall.bite, 0.0f, 1.0f);
		            bite01 = bite01 * bite01;
		            if (fabsf(sideDrive) > 1e-6f && fabsf(vx) > 0.02f)
		            {
		                // sideDrive -> y angular velocity. y>0 drifts to -x, y<0 drifts to +x.
		                float desiredLateralDir = (sideDrive < 0.0f) ? 1.0f : -1.0f; // +1 => +x, -1 => -x
		                float movingDir = (vx > 0.0f) ? 1.0f : -1.0f;
		                bool disagree = (movingDir != desiredLateralDir);
		                if (disagree)
		                {
		                    float drive = bite01 * BallSwingTuning::BITE_DRIVE_FROM_LATERAL_VEL * fabsf(vx);
		                    sideDrive += (sideDrive < 0.0f) ? -drive : drive;
		                }
		            }

		            if (!std::isfinite(sideDrive))
		            {
		                if (!s_warnedNonFiniteAngVel)
		                {
		                    std::cerr << "[throw] Non-finite sideDrive (forcing 0). angVel=" << angVel
		                              << " spin=" << usr->myBall.spin << "\n";
		                    s_warnedNonFiniteAngVel = true;
		                }
		                sideDrive = 0.0f;
		            }
		            usr->phy.apply_angular_velocity_on_ball(sideDrive);
		
		            float spinContributionToSmash = sideDrive * usr->myBall.hitBuff;
		            if (!std::isfinite(spinContributionToSmash))
		            {
		                if (!s_warnedNonFiniteAngVel)
		                {
		                    std::cerr << "[throw] Non-finite spinContributionToSmash (forcing 0). sideDrive="
		                              << sideDrive << " hitBuff=" << usr->myBall.hitBuff << "\n";
		                    s_warnedNonFiniteAngVel = true;
		                }
		                spinContributionToSmash = 0.0f;
		            }
		            usr->phy.set_spin_speed(spinContributionToSmash);
		        }
		    }
    else
    {
        usr->sectors = -1;
    }

	// Track orbital angular velocity during AIM/SWING so release can impart a bit of initial roll.
	// This approximates the "pendulum" rotation turning into spin at release.
		{
			glm::vec3 ballPos = usr->carriedBall;
			glm::vec3 r = ballPos - usr->pivotPoint;
			float rLen = glm::length(r);
			if (rLen > 1e-4f && usr->deltaTimeLoan > 1e-6f)
			{
				glm::vec3 dir = r / rLen;
				if (usr->orbitHasPrev)
				{
					glm::vec3 axis = glm::cross(usr->orbitPrevDir, dir);
					float axisLen = glm::length(axis);
					float d = glm::clamp(glm::dot(usr->orbitPrevDir, dir), -1.0f, 1.0f);
					float angle = acosf(d);
					if (axisLen > 1e-6f && angle > 1e-6f)
					{
						axis /= axisLen;
						usr->releaseOrbitAngularVel = axis * (angle / usr->deltaTimeLoan);
						if (!std::isfinite(usr->releaseOrbitAngularVel.x) ||
						    !std::isfinite(usr->releaseOrbitAngularVel.y) ||
						    !std::isfinite(usr->releaseOrbitAngularVel.z))
						{
							usr->releaseOrbitAngularVel = glm::vec3(0.0f);
						}
					}
				}
				usr->orbitPrevDir = dir;
				usr->orbitHasPrev = true;
			}
		else
		{
			usr->orbitHasPrev = false;
			usr->releaseOrbitAngularVel = glm::vec3(0.0f);
		}
	}
}

void vtx::init(vtx::VertexContext *ctx)
{

    ctx->usrptr = new UserContext;
    UserContext *usr = static_cast<UserContext *>(ctx->usrptr);

		usr->ballRenderTex.renderTextureInit();
		usr->ballRenderTex2.renderTextureInit();
		usr->oilRenderTex.renderTextureInit(false);

    usr->imgui.loadImgui(ctx);

    glEnable(GL_BLEND);
    // Enables blending, which allows transparent textures to be rendered
    // properly.
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // Sets the blending function.
    // - `GL_SRC_ALPHA`: Uses the alpha value of the source (texture or color).
    // - `GL_ONE_MINUS_SRC_ALPHA`: Makes the destination color blend with the
    // background based on alpha. This is commonly used for standard transparency
    // effects.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    // Enables depth testing, ensuring that objects closer to the camera are drawn
    // in front of those farther away. This prevents objects from rendering
    // incorrectly based on draw order.

    printShaderVersions();
    checkOpenGLError("INIT_GAME_TAG");

    usr->aurora.initAurora();
    usr->fpsCounter.initFpsCounter();

    usr->mainShader.initDefaultShaderProgram();
    usr->simpleShader.initSimpleShaderProgram();
    usr->everythingTexture.loadTextureFromFile(ASSET_PATH "everything_tex.png");
    MeshData ballMd = loadMeshFromBlob(ball_mesh_data, ball_mesh_data_len);
    usr->ballMesh.sendMeshDataToGpu(&ballMd);
    MeshData laneMd = loadMeshFromBlob(lane_mesh_data, lane_mesh_data_len);
    usr->laneMesh.sendMeshDataToGpu(&laneMd);
    MeshData pinMd = loadMeshFromBlob(pin_mesh_data, pin_mesh_data_len);
    usr->pinMesh.sendMeshDataToGpu(&pinMd);
    MeshData starMd = loadMeshFromBlob(star_mesh_data, star_mesh_data_len);
    usr->starMesh.sendMeshDataToGpu(&starMd);

    {
        const glm::vec3 eye = glm::vec3(4.0f);
        const glm::vec3 center = glm::vec3(0.0f);
        const glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

        usr->cameraMat = glm::lookAt(eye, center, up);
    }

    auto lanePositions = extractPositions(&laneMd);

    {
        const float h = 0.35f;
        const float ft = 0.305f;
        const float offset = 0.87f - 3.0f * ft;
        const float l0 = offset - 0.0 * ft * glm::cos(glm::radians(30.0f));
        usr->initialPins[0] = glm::vec3(-0.0f, h, l0);

        const float l1 = offset + 1.0 * ft * glm::cos(glm::radians(30.0f));
        usr->initialPins[1] = glm::vec3(-0.5f * ft, h, l1);
        usr->initialPins[2] = glm::vec3(+0.5f * ft, h, l1);

        const float l2 = offset + 2.0f * ft * glm::cos(glm::radians(30.0f));
        usr->initialPins[3] = glm::vec3(-ft, h, l2);
        usr->initialPins[4] = glm::vec3(-0.0f * ft, h, l2);
        usr->initialPins[5] = glm::vec3(+ft, h, l2);

        const float l3 = offset + 3.0f * ft * glm::cos(glm::radians(30.0f));
        usr->initialPins[6] = glm::vec3(-1.5f * ft, h, l3);
        usr->initialPins[7] = glm::vec3(-0.5f * ft, h, l3);
        usr->initialPins[8] = glm::vec3(+0.5f * ft, h, l3);
        usr->initialPins[9] = glm::vec3(+1.5f * ft, h, l3);
    }

    usr->ballStart = glm::vec3(0.0f, 4.0f, -8.0f);

    usr->phy.physics_init(
        lanePositions.data(), // number of floats
        lanePositions.size(), // number of floats
        laneMd.indices,
        laneMd.indexCount,
        usr->initialPins,
        usr->ballStart
    );

	usr->phase = UserContext::Phase::IDLE;
	resetScoreboard(&usr->board);

		ApplyHouseLaneParams(usr);

	usr->clayton.initClayton(ctx->screenWidth, ctx->screenHeight);
	usr->clayton.renderer.imageTextures[1] = usr->ballRenderTex.colorTexture;
	usr->clayton.renderer.imageTextures[2] = usr->ballRenderTex2.colorTexture;
	usr->clayton.renderer.imageTextures[3] = usr->oilRenderTex.colorTexture;

    usr->shouldShowClayDebug = false;
    usr->shouldShowImgui = false;
    Clay_SetDebugModeEnabled(usr->shouldShowClayDebug);

    usr->enjoy.resetJoystick();
    usr->enjoy.initDefaultFlatShaderProgram();
    usr->circle.resetCircle();
    usr->circle.initCircleThing();
    usr->decalBatch.initDecalBatch();

    usr->username_len = snprintf(usr->username, sizeof(usr->username), "Anonymous");
    initKeypad(&usr->keypad, usr->username, &usr->username_len);
    initClaytonClick(&usr->clayton.playAgainClick, "ReplayButton");
    initClaytonClick(&usr->renameButton, "PlaceOfRenameName");
    initClaytonClick(&usr->menuButton, "MenuButton");
    initClaytonClick(&usr->soundButton, "SoundButton");
    initClaytonClick(&usr->oilButton, "OilButton");
    initClaytonClick(&usr->hiScoreButton, "HiScoreButton");
    initClaytonClick(&usr->openShopClick, "openShopButton");
    initClaytonClick(&usr->clayton.closeShopClick, "closeShopButton");
    initClaytonClick(&usr->clayton.buyClick, "BuyButtdd");
    initClaytonClick(&usr->clayton.oilReoilClick, "oilReoilButton");

    usr->tri.init();
    usr->totalFrames = 0;
    usr->storage.storageInit("10x", "bowling");
    usr->username_len = usr->storage.getChar(Storage::USERNAME, usr->username, 20);

    LocalHi_Init(&usr->localHi);

    usr->coinLane.initStars(getNextCoinPattern(), 7);
    usr->clearedCoins = 0;

    // 🔌 Wire static demo catalog (replace with your real data source later)
    Carousel_Init(&usr->carousel);
    Carousel_SetupDefaultShop(&usr->carousel);
    BallStats_OnBallChange(&g_ballCatalog[0], usr);
    usr->carousel.bank = 20.0f;
}

void vtx::loop(vtx::VertexContext *ctx)
{
    UserContext *usr = static_cast<UserContext *>(ctx->usrptr);

    usr->totalFrames += 1;

    // Update async sound system restart state machine (if in progress)
    usr->sound.updateRestart();

    // SDL_SetRelativeMouseMode(SDL_FALSE);
    bool shouldHandleResize = false;
    if (usr->totalFrames == 1)
    {
        usr->sound.initSoundSystem(SONG_01);
        initSoundSettings(&usr->clayton, &usr->sound.settings, &usr->sound);

        AdaptiveAudio_Init(&usr->adaptiveAudio, 20.0f); // Threshold

        initClaytonClick(&usr->clayton.useSynthClick, "adaptiveUseSynth");
        initClaytonClick(&usr->clayton.useWavClick, "adaptiveUseWav");
        initClaytonClick(&usr->clayton.disableAudioClick, "adaptiveDisableAudio");
        initClaytonClick(&usr->clayton.oilStatusCloseClick, "oilStatusClose");

        usr->windowStack.windowStackInit();

        shouldHandleResize = true;
        std::cerr << "resize will be forced because it is first ever run" << std::endl;
    }

    // usr->phase= UserContext::Phase::THROW;
#ifndef __EMSCRIPTEN__
    if (true)
    {
        TimePoint now = Clock::now();
        Seconds dt = now - usr->last;
        const double targetDelta = 1.0 / 20.0;
        if (dt.count() < targetDelta)
        {
            double sleepTime = targetDelta - dt.count();
            std::this_thread::sleep_for(Seconds(sleepTime) - Seconds(0.001f));
            return;
        }
        usr->last = now;
    }
#endif

    float deltaTime = (float)usr->fpsCounter.startFrame();
    usr->deltaTimeLoan = deltaTime;
    usr->deltaTimeSum += deltaTime;                   // for some stuff need it in float
    volatile uint64_t currentTime = SDL_GetTicks64(); // For simple stuff, in ms

    usr->auroraVibe.update(deltaTime);

    /* Step of adaptive audio loading - must be before rendering */ {

        // Update adaptive audio system
        AdaptiveAudioState prevState = usr->adaptiveAudio.state;
        AdaptiveAudio_Update(&usr->adaptiveAudio, deltaTime, usr->fpsCounter.fps);
        AdaptiveAudioState newState = usr->adaptiveAudio.state;
        // Ensure the adaptive audio modal/progress is on the window stack whenever active.
        // (Do not rely only on state transitions; hot-reload and some flows can miss the edge.)
        if (newState == ADAPTIVE_DECIDING || newState == ADAPTIVE_EXPORTING)
        {
            usr->windowStack.windowStackPushAdaptiveAudioWindow();
        }

        // End-of-run UI: when RESULT is active we want a modal "play again" window available.
        // Keep it on the stack so hot-reload / missed transition edges don't make it disappear.
        if (usr->phase == UserContext::Phase::RESULT)
        {
            usr->windowStack.windowStackPushNewGameWindow();
            if (usr->clayton.shouldShowHiScore)
            {
                // Put the hi-score window above the play-again window.
                usr->windowStack.windowStackPushLocalHiscoreWindow();
            }
        }

        // Handle volume muting during startup monitoring:
        // - MONITORING: mute sound (inaudible while measuring FPS)
        // - -> SYNTH transition: unmute (FPS is good)
        // - -> DECIDING transition: keep muted (show modal, user decides)
        usr->wasMutedForMonitoring = false;
        if (newState == ADAPTIVE_MONITORING)
        {
            if (!usr->wasMutedForMonitoring)
            {
                // First frame of monitoring - mute sound
                usr->sound.musicVolume = 0.0f;
                usr->sound.sfxVolume = 0.0f;
                if (usr->sound.musicModule)
                    xfm_module_set_volume(usr->sound.musicModule, 0.0f);
                if (usr->sound.sfxModule)
                    xfm_module_set_volume(usr->sound.sfxModule, 0.0f);
                if (usr->sound.wavMusicModule)
                    xfm_wav_module_set_volume(usr->sound.wavMusicModule, 0.0f);
                if (usr->sound.wavSfxModule)
                    xfm_wav_module_set_volume(usr->sound.wavSfxModule, 0.0f);
                usr->wasMutedForMonitoring = true;
            }
        }
        else if (newState == ADAPTIVE_SYNTH && prevState == ADAPTIVE_MONITORING)
        {
            // FPS is good - restore volume
            usr->sound.musicVolume = 0.5f;
            usr->sound.sfxVolume = 1.0f;
            if (usr->sound.musicModule)
                xfm_module_set_volume(usr->sound.musicModule, 0.5f);
            if (usr->sound.sfxModule)
                xfm_module_set_volume(usr->sound.sfxModule, 1.0f);
            usr->wasMutedForMonitoring = false;
        }
        else if (newState == ADAPTIVE_DECIDING && prevState == ADAPTIVE_MONITORING)
        {
            // FPS is low - keep muted, modal will let user decide
            usr->wasMutedForMonitoring = false; // Reset so next monitoring cycle can mute again
        }

        // Check if sound settings triggered WAV export (user selected WAV quality in sound
        // settings) This handles the case where user skipped the slow start modal but later chooses
        // WAV CRITICAL: On Emscripten we MUST close the audio device completely to stop the
        // callback, then reopen it after export. Just pausing is NOT enough. Two-phase approach:
        // Phase 1 does export and returns to loop, Phase 2 completes init after 2s delay
        static enum {
            WAV_EXPORT_IDLE,
            WAV_EXPORT_PHASE1_CLOSE,     // Close audio device
            WAV_EXPORT_PHASE1_WAIT1,     // Wait for callback to stop
            WAV_EXPORT_PHASE1_SHUTDOWN,  // Destroy old modules
            WAV_EXPORT_PHASE1_EXPORT,    // Start export
            WAV_EXPORT_PHASE1_EXPORTING, // Actually exporting (yieldable)
            WAV_EXPORT_PHASE1_DONE,      // Return to loop, will resume after delay
            WAV_EXPORT_PHASE2_RESUME,    // Resume after delay
            WAV_EXPORT_PHASE2_INIT,      // Initialize new audio (reopens device)
        } wavExportState = WAV_EXPORT_IDLE;

        usr->wavExportWaitFrames = 0;
        usr->wavExportSongPattern = nullptr;
        usr->wavExportResumeTime = 0; // SDL_GetTicks64() when to resume

        if (usr->sound.settings.needsWavExport)
        {
            usr->sound.settings.needsWavExport = false;
            printf("[SoundSettings] Triggering WAV export from sound settings...\n");

            // Determine which song to load after export
            switch (usr->sound.currentSongIndex)
            {
            case 1:
                usr->wavExportSongPattern = SONG_01;
                break;
            case 2:
                usr->wavExportSongPattern = SONG_02;
                break;
            case 3:
                usr->wavExportSongPattern = SONG_03;
                break;
            case 4:
                usr->wavExportSongPattern = SONG_04;
                break;
            default:
                usr->wavExportSongPattern = SONG_01;
                break;
            }

            // PHASE 1: Close audio device and export (runs immediately)
            printf("[SoundSettings] Phase 1/2: Closing audio device...\n");
            if (usr->sound.audioDev)
            {
                SDL_CloseAudioDevice(usr->sound.audioDev);
                usr->sound.audioDev = 0;
            }

            // Set UI loading indicator
            usr->sound.settings.wavExportInProgress = true;
            snprintf(
                usr->sound.settings.wavExportStatus,
                sizeof(usr->sound.settings.wavExportStatus),
                "Closing audio device..."
            );

            wavExportState = WAV_EXPORT_PHASE1_WAIT1;
            usr->wavExportWaitFrames = 10;
        }

        else if (wavExportState == WAV_EXPORT_PHASE1_WAIT1)
        {
            usr->wavExportWaitFrames--;
            if (usr->wavExportWaitFrames <= 0)
            {
                printf("[SoundSettings] Phase 1/2: Destroying old modules...\n");
                snprintf(
                    usr->sound.settings.wavExportStatus,
                    sizeof(usr->sound.settings.wavExportStatus),
                    "Shutting down audio..."
                );
                usr->sound.audioShutdownInProgress.store(true);
                usr->sound.shutdown();
                wavExportState = WAV_EXPORT_PHASE1_EXPORT;
            }
        }

        else if (wavExportState == WAV_EXPORT_PHASE1_EXPORT)
        {
            // Export WAVs - yieldable, call every frame until done
            printf(
                "[SoundSettings] Phase 1/2: Exporting WAVs at %d Hz...\n", usr->sound.sampleRate
            );
            usr->sound.settings.wavExportInProgress = true;
            wavExportState = WAV_EXPORT_PHASE1_EXPORTING;
        }

        else if (wavExportState == WAV_EXPORT_PHASE1_EXPORTING)
        {
            // Call yieldable export every frame until done
            bool exportDone = AdaptiveAudio_ExportWAV(&usr->adaptiveAudio, usr->sound.sampleRate);

            // Update UI status from export progress
            snprintf(
                usr->sound.settings.wavExportStatus,
                sizeof(usr->sound.settings.wavExportStatus),
                "%s",
                usr->adaptiveAudio.exportStatus
            );

            if (exportDone)
            {
                if (usr->adaptiveAudio.state == ADAPTIVE_WAV)
                {
                    usr->sound.useWavPlayback = true;
                    usr->sound.setRuntimeWavBuffers(
                        usr->adaptiveAudio.songBuffers,
                        usr->adaptiveAudio.songBufferSizes,
                        usr->adaptiveAudio.sfxBuffers,
                        usr->adaptiveAudio.sfxBufferSizes
                    );
                }
                else
                {
                    printf("[SoundSettings] WAV export failed, falling back to synth mode\n");
                    usr->sound.useWavPlayback = false;
                }

                // Set resume time: 2 seconds from now
                usr->wavExportResumeTime = SDL_GetTicks64() + 2000;
                wavExportState = WAV_EXPORT_PHASE1_DONE;
                snprintf(
                    usr->sound.settings.wavExportStatus,
                    sizeof(usr->sound.settings.wavExportStatus),
                    "Export complete! Starting audio in 2 seconds..."
                );
                printf(
                    "[SoundSettings] Phase 1/2: Export done, returning to loop. Will resume in "
                    "2s...\n"
                );
            }
        }

        else if (wavExportState == WAV_EXPORT_PHASE1_DONE)
        {
            // Just return to loop - will check resume time each frame
            uint32_t now = SDL_GetTicks64();
            if (now >= usr->wavExportResumeTime)
            {
                wavExportState = WAV_EXPORT_PHASE2_RESUME;
            }
        }

        else if (wavExportState == WAV_EXPORT_PHASE2_RESUME)
        {
            // PHASE 2: Resume after delay, initialize new audio
            printf(
                "[SoundSettings] Phase 2/2: Resuming, initializing %s audio...\n",
                usr->sound.useWavPlayback ? "WAV" : "synth"
            );
            snprintf(
                usr->sound.settings.wavExportStatus,
                sizeof(usr->sound.settings.wavExportStatus),
                "Starting %s audio...",
                usr->sound.useWavPlayback ? "WAV" : "synth"
            );
            if (usr->sound.useWavPlayback)
            {
                usr->sound.initSoundSystem(usr->wavExportSongPattern);
                initSoundSettings(&usr->clayton, &usr->sound.settings, &usr->sound);
            }
            else
            {
                usr->sound.initSoundSystem(SONG_01);
                initSoundSettings(&usr->clayton, &usr->sound.settings, &usr->sound);
            }

            // Clear shutdown flag and loading indicator - audio is ready
            usr->sound.audioShutdownInProgress.store(false);
            usr->sound.settings.wavExportInProgress = false;
            usr->sound.settings.wavExportStatus[0] = '\0';
            wavExportState = WAV_EXPORT_IDLE;
            printf("[SoundSettings] Phase 2/2: WAV mode initialized, audio ready\n");
        }
        // remember all elseif so that i does not just simply cascade to the next phase without
        // screen update!

        // Check if restart was requested (from slow start adaptive audio modal)
        // Uses yieldable export - call AdaptiveAudio_ExportWAV every frame until done
        static enum {
            ADAPTIVE_EXPORT_IDLE,
            ADAPTIVE_EXPORT_STOP_AUDIO, // Stop current audio
            ADAPTIVE_EXPORTING,         // Actually exporting (yieldable)
            ADAPTIVE_EXPORT_INIT_WAV,   // Initialize WAV mode
            ADAPTIVE_EXPORT_INIT_SYNTH, // Initialize synth mode
        } adaptiveExportState = ADAPTIVE_EXPORT_IDLE;

        if (usr->adaptiveAudio.restartRequested && adaptiveExportState == ADAPTIVE_EXPORT_IDLE)
        {
            usr->adaptiveAudio.restartRequested = false;
            // Restore volume before restart (was muted during monitoring)
            usr->sound.musicVolume = 0.5f;
            usr->sound.sfxVolume = 1.0f;
            adaptiveExportState = ADAPTIVE_EXPORT_STOP_AUDIO;
        }

        else if (adaptiveExportState == ADAPTIVE_EXPORT_STOP_AUDIO)
        {
            if (usr->adaptiveAudio.audioDisabled)
            {
                printf("[AdaptiveAudio] Disabling audio...\n");
                usr->sound.shutdown();
                adaptiveExportState = ADAPTIVE_EXPORT_IDLE;
            }
            else if (usr->adaptiveAudio.restartUseWav)
            {
                // Step 1: Stop current audio
                printf("[AdaptiveAudio] Stopping current audio before exporting WAVs...\n");
                usr->sound.shutdown();

                // Hide the slow start modal, show WAV export loading indicator instead
                usr->adaptiveAudio.showModal = false;
                usr->sound.settings.wavExportInProgress = true;
                snprintf(
                    usr->sound.settings.wavExportStatus,
                    sizeof(usr->sound.settings.wavExportStatus),
                    "Exporting WAVs..."
                );

                adaptiveExportState = ADAPTIVE_EXPORTING;
            }
            else
            {
                // Restart with synth mode
                printf("[AdaptiveAudio] Restarting with synth mode...\n");
                usr->sound.useWavPlayback = false;
                usr->sound.restartSoundSystem();
                adaptiveExportState = ADAPTIVE_EXPORT_IDLE;
            }
        }

        else if (adaptiveExportState == ADAPTIVE_EXPORTING)
        {
            // Yieldable export - call every frame until done
            bool exportDone = AdaptiveAudio_ExportWAV(&usr->adaptiveAudio, usr->sound.sampleRate);

            // Update UI status from export progress
            snprintf(
                usr->sound.settings.wavExportStatus,
                sizeof(usr->sound.settings.wavExportStatus),
                "%s",
                usr->adaptiveAudio.exportStatus
            );

            if (exportDone)
            {
                if (usr->adaptiveAudio.state == ADAPTIVE_WAV)
                {
                    printf("[AdaptiveAudio] WAV export complete, starting WAV mode...\n");
                    usr->sound.useWavPlayback = true;

                    // Pass exported buffers to sound system
                    usr->sound.setRuntimeWavBuffers(
                        usr->adaptiveAudio.songBuffers,
                        usr->adaptiveAudio.songBufferSizes,
                        usr->adaptiveAudio.sfxBuffers,
                        usr->adaptiveAudio.sfxBufferSizes
                    );

                    snprintf(
                        usr->sound.settings.wavExportStatus,
                        sizeof(usr->sound.settings.wavExportStatus),
                        "Starting WAV audio..."
                    );
                    adaptiveExportState = ADAPTIVE_EXPORT_INIT_WAV;
                }
                else
                {
                    printf("[AdaptiveAudio] WAV export failed, falling back to synth mode\n");
                    usr->sound.useWavPlayback = false;
                    adaptiveExportState = ADAPTIVE_EXPORT_INIT_SYNTH;
                }
            }
        }

        else if (adaptiveExportState == ADAPTIVE_EXPORT_INIT_WAV)
        {
            // Restore volume before init (was muted during monitoring)
            usr->sound.musicVolume = 0.5f;
            usr->sound.sfxVolume = 1.0f;
            // Initialize with WAV mode

            usr->sound.initSoundSystem(SONG_01);
            initSoundSettings(&usr->clayton, &usr->sound.settings, &usr->sound);

            usr->sound.settings.wavExportInProgress = false;
            usr->sound.settings.wavExportStatus[0] = '\0';
            adaptiveExportState = ADAPTIVE_EXPORT_IDLE;
        }

        else if (adaptiveExportState == ADAPTIVE_EXPORT_INIT_SYNTH)
        {
            // Restore volume before init (was muted during monitoring)
            usr->sound.musicVolume = 0.5f;
            usr->sound.sfxVolume = 1.0f;
            // Initialize with synth mode
            usr->sound.initSoundSystem(SONG_01);
            initSoundSettings(&usr->clayton, &usr->sound.settings, &usr->sound);

            usr->sound.settings.wavExportInProgress = false;
            usr->sound.settings.wavExportStatus[0] = '\0';
            adaptiveExportState = ADAPTIVE_EXPORT_IDLE;
        }
    }
    const uint32_t FONT_ID_BODY_24 = 0;

    bool mouseClicked = false;
    float screenRatio = static_cast<float>(ctx->screenWidth) / ctx->screenHeight;

    float ropeLength = 1.0f;

    glm::vec2 spinMove = glm::vec2(0.0f);
    bool requestThrowEvent = false;
    SDL_Event e;
    usr->touchRelDx = 0;
    usr->touchRelDy = 0;

    // Central policy for relative mouse mode.
    // - Any active window (or RESULT/PLAY button) forces absolute mode.
    // - When returning back to AIM/SWING after closing windows, we must restore relative mode,
    //   otherwise desktop aiming feels "stuck".
    // - THROW uses relative mode only while the pointer is held down (see isMouseDownInThrow).
    //
    // IMPORTANT: Shop dragging must not depend on SDL relative mouse mode (touch uses injected
    // mouse events and relative mode can break/swallow motion deltas).
    {
        const bool windowsActive = usr->windowStack.count > 0;
        const bool forceAbsolute = windowsActive || usr->phase == UserContext::Phase::RESULT;
#if defined(__EMSCRIPTEN__) || TARGET_OS_IOS || TARGET_IPHONE_SIMULATOR
        // Touch-first platforms: never enable relative mode.
        //
        // Why:
        // - Our touch input path synthesizes SDL mouse events (SDL_TOUCH_MOUSEID) so all input
        //   routes through the same code paths (Clay UI + gameplay).
        // - Toggling relative mode can "flush" pending mouse motion internally and/or change how
        //   SDL reports motion deltas (see SDL_SetRelativeMouseMode remarks).
        // - After opening/closing any modal window we often switch relative mode off->on, and that
        //   transition can effectively drop the first few motion deltas. On touch this feels like
        //   aiming/spin becoming "stuck" after returning from a window.
        // - For touch we want absolute pointer semantics and we already have per-event xrel/yrel
        //   from our injected mouse motions when we need deltas (shop swipe, spin).
        (void)forceAbsolute;
        SDL_SetRelativeMouseMode(SDL_FALSE);
#else
        // Desktop policy: relative mode is used for AIM/SWING (and THROW while held) because it
        // gives continuous deltas without edge clamping and feels better with a real mouse.
        if (forceAbsolute)
        {
            // Any window shown => absolute mode so UI clicking works and cursor isn't grabbed.
            SDL_SetRelativeMouseMode(SDL_FALSE);
        }
        else
        {
            const bool wantsRelative =
                ((usr->phase == UserContext::Phase::AIM) || (usr->phase == UserContext::Phase::SWING)) ||
                (usr->phase == UserContext::Phase::THROW && usr->isMouseDownInThrow);
            SDL_SetRelativeMouseMode(wantsRelative ? SDL_TRUE : SDL_FALSE);
        }
#endif
    }

    UserContext::PhaseTrans phaseTrans = UserContext::PhaseTrans::TRANS_NONE;
    Clay_Vector2 scrollDelta = {};
    while (SDL_PollEvent(&e))
    {
#if defined(__EMSCRIPTEN__) || TARGET_OS_IOS || TARGET_IPHONE_SIMULATOR
        // On high-DPI targets (mobile web, iOS), ctx->screenWidth/Height are the GL drawable size.
        // SDL mouse coordinates are in window pixels, and later code normalizes using:
        //   pixelRatio * mouse_x / ctx->screenWidth
        // So when we synthesize mouse events from touch, we must inject window-pixel coords.
        int winW = ctx->screenWidth;
        int winH = ctx->screenHeight;
        if (ctx->pixelRatio > 0.0f)
        {
            winW = static_cast<int>(static_cast<float>(ctx->screenWidth) / ctx->pixelRatio);
            winH = static_cast<int>(static_cast<float>(ctx->screenHeight) / ctx->pixelRatio);
        }

        // Track one "primary" finger so we can synthesize mouse xrel/yrel for swipe-based UI
        // (e.g. shop carousel) on touch devices.
        static bool s_touchActive = false;
        static SDL_FingerID s_touchFingerId = 0;
        static int s_lastTouchX = 0;
        static int s_lastTouchY = 0;

        switch (e.type)
        {
        case SDL_MOUSEWHEEL:
        {
            scrollDelta.x = e.wheel.x;
            scrollDelta.y = e.wheel.y;
            break;
        }
        case SDL_FINGERDOWN:
        {
            // Convert normalized touch position to window pixels
            int x = (int)(e.tfinger.x * winW);
            int y = (int)(e.tfinger.y * winH);

            s_touchActive = true;
            s_touchFingerId = e.tfinger.fingerId;
            s_lastTouchX = x;
            s_lastTouchY = y;

            SDL_Event mouse;
            mouse.type = SDL_MOUSEBUTTONDOWN;
            mouse.button.button = SDL_BUTTON_LEFT;
            mouse.button.state = SDL_PRESSED;
            mouse.button.which = SDL_TOUCH_MOUSEID;
            mouse.button.x = x;
            mouse.button.y = y;
            SDL_PushEvent(&mouse); // inject as mouse event
            continue;
        }
        case SDL_FINGERUP:
        {
            int x = (int)(e.tfinger.x * winW);
            int y = (int)(e.tfinger.y * winH);

            if (s_touchActive && e.tfinger.fingerId == s_touchFingerId)
            {
                s_touchActive = false;
            }

            SDL_Event mouse;
            mouse.type = SDL_MOUSEBUTTONUP;
            mouse.button.button = SDL_BUTTON_LEFT;
            mouse.button.state = SDL_RELEASED;
            mouse.button.which = SDL_TOUCH_MOUSEID;
            mouse.button.x = x;
            mouse.button.y = y;
            SDL_PushEvent(&mouse);
            continue;
        }
        case SDL_FINGERMOTION:
        {
            int x = (int)(e.tfinger.x * winW);
            int y = (int)(e.tfinger.y * winH);

            // Only synthesize mouse movement for the active finger.
            int xrel = 0;
            int yrel = 0;
            if (!s_touchActive)
            {
                s_touchActive = true;
                s_touchFingerId = e.tfinger.fingerId;
                s_lastTouchX = x;
                s_lastTouchY = y;
            }
            if (e.tfinger.fingerId == s_touchFingerId)
            {
                xrel = x - s_lastTouchX;
                yrel = y - s_lastTouchY;
                s_lastTouchX = x;
                s_lastTouchY = y;
            }

            SDL_Event mouse;
            mouse.type = SDL_MOUSEMOTION;
            mouse.motion.state = SDL_BUTTON_LMASK; // left button held
            mouse.motion.which = SDL_TOUCH_MOUSEID;
            mouse.motion.x = x;
            mouse.motion.y = y;
            mouse.motion.xrel = xrel;
            mouse.motion.yrel = yrel;
            SDL_PushEvent(&mouse);
            continue;
        }
        }
#endif

        // Track last pointer source so we don't enable relative mouse mode on touch devices.
        // NOTE: SDL_TOUCH_MOUSEID events are our "touch -> mouse" bridge. These are the only
        // events that carry touch deltas (xrel/yrel) in a way compatible with the rest of the code.
        if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP)
        {
            usr->lastPointerWasTouch = (e.button.which == SDL_TOUCH_MOUSEID);
        }
        else if (e.type == SDL_MOUSEMOTION)
        {
            usr->lastPointerWasTouch = (e.motion.which == SDL_TOUCH_MOUSEID);
            if (usr->lastPointerWasTouch)
            {
                // Accumulate touch deltas for any code that historically used
                // SDL_GetRelativeMouseState() (which requires relative mode).
                usr->touchRelDx += e.motion.xrel;
                usr->touchRelDy += e.motion.yrel;
            }
        }

        usr->imgui.processEvent(&e, ctx);

        float pixelRatio = ctx->pixelRatio;
#if TARGET_OS_MAC
        if (e.type == SDL_MOUSEBUTTONUP || e.type == SDL_MOUSEBUTTONDOWN ||
            e.type == SDL_MOUSEMOTION)
        {
            // Because of the previous hack for Mac
            // (edit: what previous hack)
            // never scale to pixel ratio
            pixelRatio = 1.0f;
        }
#endif

        if (handle_resize_sdl(ctx, e))
        {
            shouldHandleResize = true;
        }
        if (e.type == SDL_QUIT)
            ctx->shouldContinue = false;

        usr->clayton.processClaytonEvent(&e, deltaTime, pixelRatio);

        bool stolenByClayton = false;
        if (stolenByClayton)
        {
            continue;
        }

        // Route SDL input to the active (topmost) window only. If consumed, do not let the game
        // or other UI buttons see it.
	        if (usr->windowStack.processActiveWindowEvent(
	                &usr->clayton,
	                &usr->keypad,
	                &usr->storage,
	                &usr->sound.settings,
	                &usr->adaptiveAudio,
	                &usr->localHi,
	                &usr->carousel,
	                &usr->shouldShowShop,
	                e
	            ))
	        {
	            if (usr->windowStack.oilReoilRequested)
	            {
	                usr->windowStack.oilReoilRequested = false;
	                if (usr->carousel.bank >= 10.0f)
	                {
	                    usr->carousel.bank -= 10.0f;
	                    ApplyHouseLaneParams(usr);
	                }
	            }
	            continue;
	        }
        if (e.type == SDL_KEYDOWN)
        {
            if (e.key.keysym.sym == SDLK_F5)
            {
                usr->shouldShowClayDebug = !usr->shouldShowClayDebug;
                Clay_SetDebugModeEnabled(usr->shouldShowClayDebug);
            }
            if (e.key.keysym.sym == SDLK_F6)
            {
                usr->shouldShowImgui = !usr->shouldShowImgui;
            }
            if (e.key.keysym.sym == SDLK_SPACE)
            {
                usr->phy.physics_reset(usr->initialPins, usr->ballStart, true);
                LogToIdle(usr, "SPACE_RESET");
                usr->phase = UserContext::Phase::IDLE;
                usr->wereDead = 0;
                usr->enjoy.resetJoystick();
                usr->aimFlatPos = glm::vec2(0.5f, 0.5f);
                usr->aimDownFlatPos = usr->aimFlatPos;
            }
        }
        if (e.type == SDL_MOUSEBUTTONDOWN)
        {
            mouseClicked = true; // will see this later
        }
        if (e.type == SDL_FINGERDOWN)
        {
            mouseClicked = true; // will see this later
        }

        if (isClaytonClicked(&usr->renameButton, e))
        {
            usr->windowStack.windowStackPushKeypadEditor(
                &usr->keypad, "Enter Username", usr->username, &usr->username_len
            );
            continue;
        }
        if (isClaytonClicked(&usr->menuButton, e))
        {
            usr->windowStack.windowStackPushKeypadEditor(
                &usr->keypad, "Enter Username", usr->username, &usr->username_len
            );
            continue;
        }
        if (isClaytonClicked(&usr->soundButton, e))
        {
            usr->sound.showSoundSettings();
            usr->windowStack.windowStackPushSoundSettingsWindow();
            continue;
        }
        if (isClaytonClicked(&usr->oilButton, e))
        {
            usr->clayton.shouldShowOilStatus = true;
            usr->windowStack.windowStackPushOilStatusWindow();
            continue;
        }
        if (isClaytonClicked(&usr->hiScoreButton, e))
        {
            usr->clayton.shouldShowHiScore = true;
            usr->clayton.shouldShowHiScoreWithLatest = false;
            usr->windowStack.windowStackPushLocalHiscoreWindow();
            continue;
        }

        if (isClaytonClicked(&usr->openShopClick, e))
        {
            usr->shouldShowShop = true;
            SDL_SetRelativeMouseMode(SDL_FALSE);
            usr->windowStack.windowStackPushShopWindow();
            continue;
        }

        if (usr->phase == UserContext::Phase::IDLE)
        {

            if (usr->isMouseDownInThrow == false)
            {
                SDL_SetRelativeMouseMode(SDL_FALSE);
                usr->isMouseDownInThrow = false;
            }

            usr->aimFlatPos = glm::vec2(0.0f);

            if (e.type == SDL_MOUSEBUTTONDOWN)
            {
                float x = pixelRatio * static_cast<float>(e.button.x) / ctx->screenWidth;
                float y = pixelRatio * static_cast<float>(e.button.y) / ctx->screenHeight;

                usr->aimFlatPos.x = x;
                usr->aimFlatPos.y = y;
                usr->asd = usr->aimFlatPos;
                usr->aimDownFlatPos = usr->aimFlatPos;

                phaseTrans = UserContext::PhaseTrans::TRANS_IDLE_TO_AIM;
            }
        }
        else if (usr->phase == UserContext::Phase::AIM)
        {
            if (e.type == SDL_MOUSEMOTION)
            {
                // I used to have:
                float x = pixelRatio * static_cast<float>(e.motion.x) / ctx->screenWidth;
                float y = pixelRatio * static_cast<float>(e.motion.y) / ctx->screenHeight;

                // When relative mouse mode is enabled (desktop), use xrel/yrel to accumulate
                // aimFlatPos with a higher gain so the joystick traverses its full range with
                // less physical mouse travel. For touch (injected mouse events), xrel/yrel will
                // be 0 and we fall back to absolute x/y.
                float x_rel = pixelRatio * static_cast<float>(e.motion.xrel) / ctx->screenWidth;
                float y_rel = pixelRatio * static_cast<float>(e.motion.yrel) / ctx->screenHeight;
                const float kAimRelativeGain = 3.0f;
                // Only use relative deltas when SDL relative mouse mode is actually enabled.
                // Touch-injected mouse events on web provide xrel/yrel for UI swipes, but for
                // aiming we want absolute positioning so the full joystick range is reachable.
                if (e.motion.which != SDL_TOUCH_MOUSEID &&
                    SDL_GetRelativeMouseMode() == SDL_TRUE && (x_rel != 0.0f || y_rel != 0.0f))
                {
                    usr->aimFlatPos += glm::vec2(x_rel, y_rel) * kAimRelativeGain;
                    usr->aimFlatPos.x = glm::clamp(usr->aimFlatPos.x, 0.0f, 1.0f);
                    usr->aimFlatPos.y = glm::clamp(usr->aimFlatPos.y, 0.0f, 1.0f);
                }
                else
                {
                    usr->aimFlatPos.x = x;
                    usr->aimFlatPos.y = y;
                }

                // SDL coordinates: y increases downward. Track the maximum downward delta.
                float downDelta = usr->aimFlatPos.y - usr->aimDownFlatPos.y;
                usr->aimMaxDownDeltaNdc = glm::max(usr->aimMaxDownDeltaNdc, downDelta);
            }
            if (e.type == SDL_MOUSEBUTTONUP)
            {
                // std::cerr << "let it go because of button up" << std::endl;
                const float kTapGraceSeconds = 0.40f;
                const float kTapGraceMoveNdc = 0.040f; // ~4% of screen in normalized [0..1] coords
                const float kNoMoveForgiveNdc = 0.012f; // effectively "no drag": forgive regardless of hold time
                const glm::vec2 d = usr->aimFlatPos - usr->aimDownFlatPos;
                const float moved = glm::length(d);
                const bool isTap =
                    (moved < kNoMoveForgiveNdc) ||
                    ((usr->aimingTime < kTapGraceSeconds) && (moved < kTapGraceMoveNdc));
                if (isTap)
                {
                    // Treat as "cancel": don't consume the ball / don't penalize.
                    LogToIdle(usr, "AIM_TAP_CANCEL");
                    usr->phase = UserContext::Phase::IDLE;
                    usr->bufferedRequestThrow = false;
                    usr->aimingTime = 0.0f;
                    usr->enjoy.resetJoystick();
                    SDL_SetRelativeMouseMode(SDL_FALSE);
                }
                else
                {
                    requestThrowEvent = true;
                }
            }
        }
        else if (usr->phase == UserContext::Phase::SWING)
        {
            // std::cerr << "Waiting for button up while swing" << std::endl;
            if (e.type == SDL_MOUSEMOTION)
            {
                // I used to have:
                float x = pixelRatio * static_cast<float>(e.motion.x) / ctx->screenWidth;
                float y = pixelRatio * static_cast<float>(e.motion.y) / ctx->screenHeight;

                // I want to use this as well
                float x_rel = pixelRatio * static_cast<float>(e.motion.xrel) / ctx->screenWidth;
                float y_rel = pixelRatio * static_cast<float>(e.motion.yrel) / ctx->screenHeight;

                const float kSwingRelativeGain = 3.0f;
                if (e.motion.which != SDL_TOUCH_MOUSEID &&
                    SDL_GetRelativeMouseMode() == SDL_TRUE && (x_rel != 0.0f || y_rel != 0.0f))
                {
                    usr->aimFlatPos += glm::vec2(x_rel, y_rel) * kSwingRelativeGain;
                    usr->aimFlatPos.x = glm::clamp(usr->aimFlatPos.x, 0.0f, 1.0f);
                    usr->aimFlatPos.y = glm::clamp(usr->aimFlatPos.y, 0.0f, 1.0f);
                }
                else
                {
                    usr->aimFlatPos.x = x;
                    usr->aimFlatPos.y = y;
                }
            }
            if (e.type == SDL_MOUSEBUTTONUP)
            {
                // std::cerr << "let it go because of button up" << std::endl;

                requestThrowEvent = true;
            }
        }
        else if (usr->phase == UserContext::Phase::THROW)
        {
            if (e.type == SDL_MOUSEBUTTONDOWN)
            {
                if (e.button.which != SDL_TOUCH_MOUSEID)
                {
                    SDL_SetRelativeMouseMode(SDL_TRUE);
                }
                usr->isMouseDownInThrow = true;
            }
            if (e.type == SDL_MOUSEBUTTONUP)
            {
                SDL_SetRelativeMouseMode(SDL_FALSE);
                usr->isMouseDownInThrow = false;
            }
            if (e.type == SDL_MOUSEMOTION)
            {
                // I used to have:
                float x = pixelRatio * static_cast<float>(e.motion.x) / ctx->screenWidth;
                float y = pixelRatio * static_cast<float>(e.motion.y) / ctx->screenHeight;

                // I want to use this as well
                float x_rel = pixelRatio * static_cast<float>(e.motion.xrel) / ctx->screenWidth;
                float y_rel = pixelRatio * static_cast<float>(e.motion.yrel) / ctx->screenHeight;

                usr->aimFlatPos.x = x;
                usr->aimFlatPos.y = y;
                spinMove.x = x_rel;
                spinMove.y = y_rel;
            }
        }
    }
    if (shouldHandleResize)
    {
        // Recalculate perspective
        float fov = glm::radians(60.0f); // Field of view in radians
        float aspectRatio = (float)ctx->screenWidth / (float)ctx->screenHeight;
        float nearPlane = 0.50f;
        float farPlane = 30.0f;
        usr->perspectiveMat = glm::perspective(fov, aspectRatio, nearPlane, farPlane);

        usr->imgui.loadImgui(ctx);
    }

    float TUNE = 200.0f;
    int movePivot = 0;
    if (usr->aimFlatPos.x < 0.1f)
    {
        movePivot = -1;
    }
    else if (usr->aimFlatPos.x > 0.9f)
    {
        movePivot = +1;
    }

    if (usr->phase == UserContext::Phase::AIM)
    {
        float pivotRail = 0.40f;
        if (movePivot != 0)
        {
            if (usr->pivotPoint.x >= -pivotRail && usr->pivotPoint.x <= pivotRail)
            {
                float pivotMoveSpeed = 0.5f;
                usr->pivotPoint.x -= (movePivot * deltaTime * pivotMoveSpeed);
                usr->pivotPoint.x = glm::clamp(usr->pivotPoint.x, -pivotRail, pivotRail);
                usr->phy.change_pivot_point(usr->pivotPoint);
            }
        }
    }

    /* Stuff that depends on ball stats, and affects every frame */ {
        // Moved to BallStats_EveryFrame
    }
    /* Stuff that updates joystick and spin circle */ {

        if (usr->phase == UserContext::Phase::IDLE)
        {
            usr->circle.resetCircle();
            usr->totalAngle = 0.0f;
            usr->angularVelocity = 0.0f;
            usr->smoothedAngularVelocity = 0.0f;

            // just to reset it
            usr->circles = 0;

            // IDLE should always start from a centered control state.
            usr->enjoy.resetJoystick();
            usr->aimFlatPos = glm::vec2(0.5f, 0.5f);
            usr->aimDownFlatPos = usr->aimFlatPos;

            // Hot-reload resilience: if scene params look uninitialized, restore defaults.
            if (usr->scene.pivotZ > -5.0f)
            {
                usr->scene = SceneTunables_Default();
            }

            // Keep pivot stable in IDLE so camera clamps don't jump.
            usr->pivotPoint = glm::vec3(0.0f, usr->scene.pivotY, usr->scene.pivotZ);
            usr->phy.change_pivot_point(usr->pivotPoint);
        }
        // usr->sectors = usr->circle.moveCircle(spinMove, deltaTime);
        if (usr->phase == UserContext::Phase::THROW)
        {

            /* update spin */ {
                // Prefer per-event deltas (spinMove) because they work for both:
                // - touch-injected mouse motion (xrel/yrel provided by our injector)
                // - relative mouse mode on desktop
                //
                // Why not rely on SDL_GetRelativeMouseState():
                // - On touch-first builds we keep relative mouse mode OFF, so SDL's relative
                //   accumulator often stays at 0 even though we are receiving motion events.
                // - After toggling relative mode (e.g. when showing/hiding windows), SDL may flush
                //   pending motion, causing a "no spin" frame right after returning.
                int dx = (int)spinMove.x;
                int dy = (int)spinMove.y;
                if (dx == 0 && dy == 0)
                {
                    dx = usr->touchRelDx;
                    dy = usr->touchRelDy;
                }
                if (dx == 0 && dy == 0)
                {
                    SDL_GetRelativeMouseState(&dx, &dy);
                }

                glm::vec2 v(dx, dy);

                float speed = glm::length(v);
                float minSpeed = 0.5f;

                if (speed > minSpeed)
                {

                    glm::vec2 dir = v / speed;

                    float cross = usr->prevDir.x * dir.y - usr->prevDir.y * dir.x;
                    float dot = usr->prevDir.x * dir.x + usr->prevDir.y * dir.y;

                    float angleDelta = -atan2f(cross, dot);

                    float speedScale = 0.05f;
                    float weight = glm::clamp(speed * speedScale, 0.0f, 1.0f);
                    angleDelta *= weight;

                    usr->totalAngle += angleDelta;
                    usr->angularVelocity = angleDelta / deltaTime;

                    const float FULL_TURN = glm::two_pi<float>();

                    if (usr->totalAngle >= FULL_TURN)
                    {
                        usr->totalAngle -= FULL_TURN;
                        usr->circles++;
                    }
                    else if (usr->totalAngle <= -FULL_TURN)
                    {
                        usr->totalAngle += FULL_TURN;
                        usr->circles--;
                    }

                    usr->prevDir = dir;

	                    // Smoothing
	                    float smoothingSpeed = 10.0f; // higher = snappier, lower = smoother

	                    float safeDt = std::isfinite(deltaTime) ? deltaTime : 0.0f;
	                    float factor = glm::clamp(safeDt * smoothingSpeed, 0.0f, 1.0f);

	                    usr->smoothedAngularVelocity +=

	                        (usr->angularVelocity - usr->smoothedAngularVelocity) * factor;

                    // Feed in to legacy shit
                    float absoluteAngle = usr->circles * FULL_TURN + usr->totalAngle;
                    float wrapped = fmodf(absoluteAngle, glm::two_pi<float>());
                    float sectorSize = glm::two_pi<float>() / Circle::SECTOR_COUNT;
                    int sector = int(wrapped / sectorSize);
                    if (sector > 0)
                    {
                        sector = sector + 1;
                    }

                    usr->circle.updateSector(-sector);
                }
            }
        }

        if (usr->phase == UserContext::Phase::AIM)
        {
            usr->enjoy.moveJoystickTo(usr->aimFlatPos, deltaTime);
        }
        if (usr->phase == UserContext::Phase::SWING)
        {
            usr->enjoy.moveJoystickTo(usr->aimFlatPos, deltaTime);
        }
    }

    if (usr->shouldShowShop && usr->windowStack.shopBuyRequested)
    {
        usr->windowStack.shopBuyRequested = false;
        CatalogItem temp;
        std::memcpy(&temp, &usr->myBall, sizeof(CatalogItem));
        std::memcpy(
            &usr->myBall,
            &usr->carousel.items[usr->carousel.closestBallIdx],
            sizeof(CatalogItem)
        );
        std::memcpy(&usr->carousel.items[usr->carousel.closestBallIdx], &temp, sizeof(CatalogItem));
        BallStats_ApplyCatalog(usr, usr->myBall);
        usr->shouldShowShop = false;
        usr->carousel.bank -= usr->myBall.price;
        usr->windowStack.shopPointerDown = false;
        std::cerr << "Item bought" << std::endl;
    }

    if (usr->windowStack.playAgainRequested)
    {
        usr->windowStack.playAgainRequested = false;
        LogToIdle(usr, "PLAY_AGAIN");
        usr->phase = UserContext::Phase::IDLE;
        usr->clayton.shouldShowHiScore = false;
        usr->clayton.shouldShowHiScoreWithLatest = false;
        usr->enjoy.resetJoystick();
        usr->aimFlatPos = glm::vec2(0.5f, 0.5f);
        usr->aimDownFlatPos = usr->aimFlatPos;
        usr->wereDead = 0;
        usr->phy.physics_reset(usr->initialPins, usr->ballStart, true);
        std::cerr << textScoreboard(usr->board) << std::endl;
        resetScoreboard(&usr->board);
        // When leaving RESULT, we generally want relative mode restored by phase logic next frame.
    }

    if (usr->keypad.newsDetected)
    {
        std::cerr << "keypad news detect" << usr->username_len << std::endl;
        usr->keypad.newsDetected = false;
        bool isSb1 = (usr->username_len == 3 && memcmp(usr->username, "SB1", 3) == 0);
        if (isSb1)
        {
            setupStubScoreboardFinal(&usr->board);
            std::cerr << "seted up board stub" << std::endl;
        }
    }

    // Check for some more if any phases need to transition
    if (usr->phase == UserContext::Phase::AIM)
    {
        bool aimingLongEnough = usr->aimingTime > 0.6f;
        bool wantsPhysics = usr->trans.wantsPhysics(usr->enjoy.ndc, deltaTime);
        if (wantsPhysics && aimingLongEnough && movePivot == 0)
        {
            phaseTrans = UserContext::PhaseTrans::TRANS_AIM_TO_SWING;
        }
    }

		    if (usr->phase == UserContext::Phase::SWING)
		    {
		        glm::vec3 ballPos = usr->carriedBall;
		        bool muchUp = ballPos.y > usr->pivotPoint.y + 0.2f;
		        float ropeLen = glm::length(ballPos - usr->pivotPoint);
		        usr->scene.releaseOffsetZ =
		            Scene_ComputeReleaseOffsetZ(usr->scene, ropeLen, usr->myBall.launchBuff);
		        float releasePlaneZ = usr->pivotPoint.z + usr->scene.releaseOffsetZ;
		        bool muchFwd = ballPos.z > releasePlaneZ + 0.9f;
		        bool muchUpFront = muchUp + muchFwd;
	        bool physicsLongEnough = usr->swingingTime > 0.4f;
	        bool physicsWayTooLong = usr->swingingTime > 1.4f;
        bool wantsPhysics = usr->trans.wantsPhysics(usr->enjoy.ndc, deltaTime);
        bool userTriesToThrow = requestThrowEvent || usr->bufferedRequestThrow;

        // If SWING gets stuck (ball position not changing), cancel the attempt.
        // We use position deltas instead of velocity because Jolt can report small velocities even
        // when the constraint is effectively wedged. This should also cancel "buffered throw"
        // cases (long-press release near pivot) so we don't hang forever in SWING.
	        if (true)
	        {
	            glm::vec3 prev = usr->swingPreviousFramePoint;
	            usr->prevBallPosForRelease = prev;
	            usr->hasPrevBallPosForRelease = true;
	            usr->swingPreviousFramePoint = usr->carriedBall;

            float moved = glm::length(usr->carriedBall - prev);
            float speed = (deltaTime > 1e-6f) ? (moved / deltaTime) : 0.0f;
            const float kStallSpeed = 0.03f; // m/s
            if (speed < kStallSpeed)
            {
                usr->swingStallTime += deltaTime;
            }
            else
            {
                usr->swingStallTime = 0.0f;
            }

            // Forgiveness: if swing stalls for long enough, cancel without counting a throw.
            // User request: stall > 1s => return to AIM (not IDLE).
            if (usr->swingingTime > 0.50f && usr->swingStallTime > 1.0f)
            {
                if (usr->debugForgiveness)
                {
                    std::cerr << "Swing stalled -> forgive to AIM" << std::endl;
                    std::cerr << "  moved=" << moved << " speed=" << speed << " stallSpeed=" << kStallSpeed
                              << " swingTime=" << usr->swingingTime << " stallTime=" << usr->swingStallTime
                              << " muchUp=" << muchUp << " muchFwd=" << muchFwd << " wantsPhysics=" << wantsPhysics
                              << " userTriesToThrow=" << userTriesToThrow << std::endl;
                }

                usr->bufferedRequestThrow = false;
                usr->phy.set_ball_free();

                // Keep ball where it is, but switch back to kinematic control for AIM.
                usr->carriedVel = glm::vec3(0.0f);
                usr->phy.set_manual_ball_position(
                    usr->carriedBall, glm::quat(1.0f, 0, 0, 0), deltaTime
                );

                usr->phase = UserContext::Phase::AIM;
                usr->aimingTime = 0.0f;
                usr->swingingTime = 0.0f;
                usr->highestPoint = -10.0f;
                usr->swingStallTime = 0.0f;
                usr->aimDownFlatPos = usr->aimFlatPos;
                SDL_SetRelativeMouseMode(SDL_TRUE);

                // Prevent any other transitions this frame.
                phaseTrans = UserContext::PhaseTrans::TRANS_NONE;
                requestThrowEvent = false;
                usr->bufferedRequestThrow = false;
                goto swing_checks_done;
            }
        }

        if ((!userTriesToThrow) &&
            ((!wantsPhysics && physicsLongEnough) || (muchUpFront) ||
             usr->carriedBall.y > usr->pivotPoint.y)) // super complicated trans function
        {
            phaseTrans = UserContext::PhaseTrans::TRANS_SWING_TO_AIM;
        }
swing_checks_done:
        ;
    }

	    if (requestThrowEvent || usr->bufferedRequestThrow)
	    {
	        // Do not release if the ball is pulled behind, let it swing at least to pivot point

		        glm::vec3 ballPos = usr->carriedBall;
			        float ropeLen = glm::length(ballPos - usr->pivotPoint);
			        usr->scene.releaseOffsetZ =
			            Scene_ComputeReleaseOffsetZ(usr->scene, ropeLen, usr->myBall.launchBuff);
			        float releasePlaneZ = usr->pivotPoint.z + usr->scene.releaseOffsetZ;

			        // FPS-independent release: if we crossed the release plane this frame,
			        // interpolate the moment of crossing and compute orbital spin there.
			        if (usr->phase == UserContext::Phase::SWING && usr->hasPrevBallPosForRelease)
			        {
			            glm::vec3 prevPos = usr->prevBallPosForRelease;
			            float z0 = prevPos.z;
			            float z1 = ballPos.z;
			            if (z0 <= releasePlaneZ && z1 > releasePlaneZ)
			            {
			                float denom = (z1 - z0);
			                float a = (denom > 1e-6f) ? ((releasePlaneZ - z0) / denom) : 1.0f;
			                a = glm::clamp(a, 0.0f, 1.0f);
			                glm::vec3 crossPos = glm::mix(prevPos, ballPos, a);
			                glm::vec3 r = crossPos - usr->pivotPoint;
			                float rLen = glm::length(r);
			                if (rLen > 1e-4f)
			                {
			                    glm::vec3 dir = r / rLen;
			                    if (usr->orbitHasPrev)
			                    {
			                        glm::vec3 axis = glm::cross(usr->orbitPrevDir, dir);
			                        float axisLen = glm::length(axis);
			                        float d = glm::clamp(glm::dot(usr->orbitPrevDir, dir), -1.0f, 1.0f);
			                        float angle = acosf(d);
			                        if (axisLen > 1e-6f && angle > 1e-6f && usr->deltaTimeLoan > 1e-6f)
			                        {
			                            axis /= axisLen;
			                            usr->releaseOrbitAngularVel = axis * (angle / usr->deltaTimeLoan);
			                        }
			                    }
			                }
			            }
			        }

			        bool safeToRelease = ballPos.z > releasePlaneZ;
		        if (!safeToRelease)
		        {
	            if (!usr->bufferedRequestThrow)
	            {
                usr->bufferedRequestThrow = true;
                if (usr->phase == UserContext::Phase::AIM)
                {
                    phaseTrans = UserContext::PhaseTrans::TRANS_AIM_TO_SWING;
                }
            }
        }
        else
        {
            if (usr->phase == UserContext::Phase::AIM)
            {
                phaseTrans = UserContext::PhaseTrans::TRANS_AIM_TO_THROW;
            }
            if (usr->phase == UserContext::Phase::SWING)
            {
                phaseTrans = UserContext::PhaseTrans::TRANS_SWING_TO_THROW;
            }

            usr->bufferedRequestThrow = false;
        }
    }
    // Transition phases and apply side effects
    ZONE("Apply Phase Transitions")
    {
        if (phaseTrans != UserContext::PhaseTrans::TRANS_NONE)
        {
	            if (phaseTrans == UserContext::PhaseTrans::TRANS_IDLE_TO_AIM)
	            {
	                usr->phase = UserContext::Phase::AIM;
	                std::cerr << "IDLE -> AIM" << std::endl;
	
	                usr->pivotPoint = glm::vec3(0.0f, usr->scene.pivotY, usr->scene.pivotZ);
	                usr->phy.change_pivot_point(usr->pivotPoint);

                usr->joystick = glm::vec3(0.0f);
                usr->aimStart = glm::vec3(0.0f);
                usr->aimingTime = 0.0f;
                usr->aimDownFlatPos = usr->aimFlatPos;
                usr->aimMaxPullbackMeters = 0.0f;
                usr->aimMinNdcY = 0.0f;
                usr->aimMaxDownDeltaNdc = 0.0f;

                SDL_SetRelativeMouseMode(SDL_TRUE);

                usr->launchSpeed = 0.0f;
                usr->endSpeed = 0.0f;

                usr->spinSpeed = 0.0f;
                usr->totalSpinAngle = 0.0f;

                usr->st.lastPos = glm::vec2(0.0f);
                usr->st.lastVel = glm::vec2(0.0f);
                usr->st.spinSpeed = 0.0f;
                usr->st.curveAccum = 0.0f;
            }
            if (phaseTrans == UserContext::PhaseTrans::TRANS_AIM_TO_SWING)
            {
                usr->phase = UserContext::Phase::SWING;
                std::cerr << "AIM -> SWING " << std::endl;

                usr->phy.set_ball_swing_movement(glm::vec3(0.0f));
                usr->phy.set_ball_hanging(usr->pivotPoint, usr->carriedBall);
                usr->phy.enable_physics_on_ball();

                usr->undesiredMovement = glm::vec3(0.0f);
                usr->swingingTime = 0.0f;
                usr->highestPoint = -10.0f;
                usr->swingPreviousFramePoint = usr->carriedBall;
                usr->swingStallTime = 0.0f;
            }
		            if (phaseTrans == UserContext::PhaseTrans::TRANS_AIM_TO_THROW)
		            {
		                usr->phase = UserContext::Phase::THROW;
		                std::cerr << "AIM -> THROW" << std::endl;
		                usr->throwEverAboveLane = false;
		                usr->oilWearLeftM = 0.0f;
		                usr->oilWearRightM = 0.0f;
		                usr->oilWearTotalM = 0.0f;
		                usr->throwEverAboveLane = false;
		
		                // Convert a bit of the swing/orbital motion into initial spin at release.
		                glm::vec3 w = usr->releaseOrbitAngularVel * BallSwingTuning::RELEASE_ORBIT_SPIN_SCALE;
		                float wLen = glm::length(w);
	                if (wLen > BallSwingTuning::RELEASE_ORBIT_SPIN_MAX)
	                {
	                    w *= (BallSwingTuning::RELEASE_ORBIT_SPIN_MAX / wLen);
	                }
	                usr->phy.set_pending_release_angular_velocity(w + usr->releaseSpinFromRot);

	                usr->phy.set_ball_free();
	                usr->phy.enable_physics_on_ball();

                // Assist for "pick + push forward" users (often kids): if they release quickly
                // while pushing forward, guarantee a small minimum forward speed so the ball
                // actually rolls instead of dropping out with near-zero velocity.
                {
                    const float kAssistMaxAimingSeconds = 0.45f;
                    const float kAssistMinForwardIntent = 0.25f; // joystick ndc.y in [0..1]
                    float forwardIntent = glm::clamp(usr->enjoy.ndc.y, 0.0f, 1.0f);
                    // Kids throw detection is based only on 2D pointer movement:
                    // if the swipe never moves down the screen by a meaningful amount, treat it as a kids throw.
                    const float kDownMoveThreshold = 0.020f; // ~2% of screen; avoids noise disqualifying kids throws
                    bool upwardOnlySwipe = usr->aimMaxDownDeltaNdc < kDownMoveThreshold;
                    bool looksLikeKidsThrow = upwardOnlySwipe;
                    if (usr->aimingTime < kAssistMaxAimingSeconds &&
                        forwardIntent > kAssistMinForwardIntent && looksLikeKidsThrow)
                    {
                        std::cerr << "Kids throw detected (no downward screen movement)" << std::endl;
                        glm::vec3 vel = usr->phy.get_ball_swing_movement();

                        // Base assist speed in m/s; keep it modest since speedBoostAtThrow
                        // will scale it on the first THROW frame.
                        float speedBoostScale = remapClamped(
                            usr->speedBoostAtThrow,
                            BallPhysicsMapping::PHYSICS_SPEEDBOOST_MIN,
                            BallPhysicsMapping::PHYSICS_SPEEDBOOST_MAX,
                            0.85f,
                            1.15f
                        );
                        float minForwardSpeed = (0.8f + 1.6f * forwardIntent) * speedBoostScale;

                        if (vel.z < minForwardSpeed)
                        {
                            vel.z = minForwardSpeed;
                        }

                        // Help alignment: damp sideways velocity so the ball tends to go down-lane.
                        // Keep some X so it still feels responsive (not a rail).
                        float alignStrength = glm::clamp((forwardIntent - kAssistMinForwardIntent) / 0.75f, 0.0f, 1.0f);
                        vel.x = glm::mix(vel.x, 0.0f, 0.65f * alignStrength);

                        usr->phy.set_ball_swing_movement(vel);
                    }
                }

                SDL_SetRelativeMouseMode(SDL_FALSE);
                usr->throwingTime = 0.0f;
                usr->settlingTime = 0.0f;
                // GameSoundSystem sound;

                usr->sound.playSfxBallHitLane();
            }
            if (phaseTrans == UserContext::PhaseTrans::TRANS_SWING_TO_AIM)
            {
                usr->phase = UserContext::Phase::AIM;
                std::cerr << "SWING -> AIM" << std::endl;

                usr->aimingTime = 0.0f;
                usr->swingingTime = 0.0f;
                usr->highestPoint = -10.0f;
            }
		            if (phaseTrans == UserContext::PhaseTrans::TRANS_SWING_TO_THROW)
		            {
		                std::cerr << "SWING -> THROW" << std::endl;
		
		                usr->phase = UserContext::Phase::THROW;
		                usr->throwEverAboveLane = false;
		                usr->oilWearLeftM = 0.0f;
		                usr->oilWearRightM = 0.0f;
		                usr->oilWearTotalM = 0.0f;
		
		                usr->phy.set_ball_free();

	                // Ball is already dynamic in SWING; inject a bit of initial spin on release.
	                glm::vec3 w = usr->releaseOrbitAngularVel * BallSwingTuning::RELEASE_ORBIT_SPIN_SCALE;
	                float wLen = glm::length(w);
	                if (wLen > BallSwingTuning::RELEASE_ORBIT_SPIN_MAX)
	                {
	                    w *= (BallSwingTuning::RELEASE_ORBIT_SPIN_MAX / wLen);
	                }
	                usr->phy.add_ball_angular_velocity(w + usr->releaseSpinFromRot);
	
	                SDL_SetRelativeMouseMode(SDL_FALSE);
	                usr->throwingTime = 0.0f;
                usr->settlingTime = 0.0f;
                // events
                usr->sound.playSfxBallHitLane();
            }
        }
    }

    glm::vec3 IDLE_BALL_POS = Scene_IdleBallPos(usr->scene);

    float yFactor = 0.0f;
    glm::mat4 ballModel;
    /* Put ballmodel */ {
        if (usr->phase == UserContext::Phase::IDLE)
        {
            usr->numberOfBallsHit = 0;

            const float t = static_cast<float>(currentTime) / 1000.0f;
            // Vertical jiggle: amplitude 0.15 m (15 cm), frequency arbitrary (1 Hz
            // here)
            const float amplitude = 0.10f;
            const float frequency = 1.0f;
            const float yOffset = amplitude * sinf(t * frequency * glm::two_pi<float>());

            // Rotations (in radians): slow globe-like spin
            const float idleSpinSpeed = glm::radians(45.0f); // 45° per second
            const float rotation = t * idleSpinSpeed;

            ballModel = glm::translate(glm::mat4(1.0f), IDLE_BALL_POS);

            // Apply translation
            ballModel = glm::translate(ballModel, glm::vec3(0.0f, yOffset, 0.0f));

            // Apply rotation
            ballModel = glm::rotate(ballModel, rotation, glm::vec3(0.0f, 1.0f, 0.0f));

            usr->carriedBall = ballModel[3];

            if (usr->coinLane.autoRespawnIfNeeded(getNextCoinPattern(), 7, deltaTime))
            {
                usr->clearedCoins = 0; // Reset counter for new set of coins
            }
            usr->catchupSpeed = glm::vec3(0.0f);
            usr->catchupDirection = glm::vec3(0.0f);
            usr->carriedVel = glm::vec3(0.0f);
        }

	        if (usr->phase == UserContext::Phase::AIM)
	        {
	
	            // Align ball local +Y axis to point toward pivot (rope direction).
	            glm::vec3 ropeDir = usr->pivotPoint - usr->carriedBall;
	            if (glm::dot(ropeDir, ropeDir) < 1e-8f)
	            {
	                ropeDir = glm::vec3(0.0f, 1.0f, 0.0f);
	            }
	            ropeDir = glm::normalize(ropeDir);
	            glm::quat ropeAlign = quatFromToSafe(glm::vec3(0.0f, 1.0f, 0.0f), ropeDir);

	            // Spin around the rope axis so alignment is preserved.
	            glm::quat ropeSpin = glm::angleAxis(usr->totalSpinAngle, ropeDir);
	            glm::quat ballRot = ropeSpin * ropeAlign;

	            // Track per-frame rotation change so release spin is FPS-independent.
	            if (usr->hasPrevBallRotForRelease)
	            {
	                glm::quat deltaRot = ballRot * glm::inverse(usr->prevBallRotForRelease);
	                usr->releaseSpinFromRot = angularVelocityFromDelta(deltaRot, usr->deltaTimeLoan);
	            }
	            usr->prevBallRotForRelease = ballRot;
	            usr->hasPrevBallRotForRelease = true;

	            usr->aimingTime += deltaTime;

	            float pullX = usr->enjoy.ndc.x;
	            float pullZ = usr->enjoy.ndc.y;
	            // Adds hung
	            // Keep inside rope sphere to avoid NaNs.
	            {
	                float r2 = 1.01f * ropeLength * ropeLength;
	                float maxZ2 = r2 - pullX * pullX;
	                maxZ2 = glm::max(0.0f, maxZ2);
	                float maxAbsZ = sqrtf(maxZ2);
	                pullZ = glm::clamp(pullZ, -maxAbsZ, maxAbsZ);
	            }
	            float pullY2 = 1.01f * ropeLength * ropeLength - pullX * pullX - pullZ * pullZ;
	            pullY2 = glm::max(0.0f, pullY2);
	            float pullY = -sqrtf(pullY2);

            usr->desiredBall = glm::vec3(
                usr->pivotPoint.x + pullX,
                usr->pivotPoint.y + pullY + 0.1f, // + 0.25f,
                usr->pivotPoint.z + pullZ
            );

            /* Hand moving carried ball */ {
                // F = m * a  →  a = F / m
                glm::vec3 handPos = usr->desiredBall;
                glm::vec3 velXZ = glm::vec3(usr->carriedVel.x, 0.0f, usr->carriedVel.z);

                float speedXZ = glm::length(velXZ);
                if (speedXZ > 0.0001f)
                {
                    glm::vec3 dirXZ = velXZ / speedXZ;

                    // How much we trust momentum vs hand
                    // --- Tunable parameters ---
                    const float momentumGainPerSpeed =
                        0.3f; // how quickly momentum influence grows with speed
                    const float minMomentumInfluence = 0.0f; // minimum influence (usually 0)
                    const float maxMomentumInfluence =
                        0.9f; // maximum influence (how stubborn it can get)
                    // --- Compute influence ---
                    float rawMomentumInfluence = speedXZ * momentumGainPerSpeed;
                    float alignStrength = glm::clamp(
                        rawMomentumInfluence, minMomentumInfluence, maxMomentumInfluence
                    );

                    // Project desired offset onto movement direction
                    glm::vec3 toHand = handPos - usr->carriedBall;

                    float forwardAmount = glm::dot(toHand, dirXZ);
                    glm::vec3 forwardComponent = forwardAmount * dirXZ;

                    // Blend: more speed → more forward bias
                    glm::vec3 biasedOffset = glm::mix(toHand, forwardComponent, alignStrength);

                    handPos = usr->carriedBall + biasedOffset;
                }

                // Tunable parameters
                const float stiffness = 80.0f; // spring strength
                const float damping = 12.0f;   // velocity damping
                const glm::vec3 gravity(0.0f, -9.81f, 0.0f);

                // --- SPRING FORCE (Hooke’s law style) ---
                glm::vec3 displacement = handPos - usr->carriedBall;
                glm::vec3 springForce = stiffness * displacement;

                // /* not too sure if it add musch stisfactory value 9abe on moment when ball goes
                // back on cancelled throw)*/{
                //     // --- SPEED-BASED XZ STEERING RESISTANCE ---
                //     glm::vec3 velXZ = glm::vec3(usr->carriedVel.x, 0.0f, usr->carriedVel.z);
                //     float speedXZ = glm::length(velXZ);

                //     float steerFactor = 1.0f / (1.0f + speedXZ * 0.125f);

                //     // Only weaken steering on XZ plane
                //     springForce.x *= steerFactor;
                //     springForce.z *= steerFactor;
                // }

                // --- DAMPING FORCE ---
                glm::vec3 dampingForce = -damping * usr->carriedVel;

                // --- GRAVITY FORCE ---
                glm::vec3 gravityForce = usr->myBall.mass * gravity;

                // --- TOTAL FORCE ---
                glm::vec3 totalForce = springForce + dampingForce + gravityForce;

                // --- ACCELERATION ---
                glm::vec3 acceleration = totalForce / usr->myBall.mass;

                // --- INTEGRATION ---
                usr->carriedVel += acceleration * deltaTime;
                usr->carriedBall += usr->carriedVel * deltaTime;
                // }
            } /* hand moving carried ball end */

            {
                // Pullback depth relative to pivot (positive when ball is behind pivot).
                float pullbackMeters = usr->pivotPoint.z - usr->carriedBall.z;
                usr->aimMaxPullbackMeters = glm::max(usr->aimMaxPullbackMeters, pullbackMeters);
            }

            // Track whether the user ever actually pulled back on the joystick (ndc.y < 0).
            usr->aimMinNdcY = glm::min(usr->aimMinNdcY, usr->enjoy.ndc.y);

	            ballModel = glm::translate(glm::mat4(1.0f), usr->carriedBall) * glm::mat4_cast(ballRot);
	
	            usr->phy.set_manual_ball_position(usr->carriedBall, ballRot, deltaTime * 1.0f);
	        }
        // usr->phy.enable_physics_on_ball();

	        if (usr->phase == UserContext::Phase::SWING)
	        {
	            usr->swingingTime += deltaTime;

            //  std::cerr << "SPIN2 " << spin << std::endl
            // usr->phy.apply_angular_velocity_on_ball(spin);

	            // Physics controls position; we control rotation so it stays aligned with the rope.
	            ballModel = usr->phy.physics_get_ball_matrix();
	            glm::vec3 before = usr->carriedBall;
	            usr->carriedBall = ballModel[3]; //
	            glm::vec3 after = usr->carriedBall;

	            glm::vec3 ballPos = ballModel[3];
	            glm::vec3 ropeDir = usr->pivotPoint - ballPos;
	            if (glm::dot(ropeDir, ropeDir) < 1e-8f)
	                ropeDir = glm::vec3(0.0f, 1.0f, 0.0f);
	            ropeDir = glm::normalize(ropeDir);
	            glm::quat ropeAlign = quatFromToSafe(glm::vec3(0.0f, 1.0f, 0.0f), ropeDir);
	            glm::quat ropeSpin = glm::angleAxis(usr->totalSpinAngle, ropeDir);
	            glm::quat ballRot = ropeSpin * ropeAlign;

	            usr->phy.set_ball_rotation(ballRot);

	            if (usr->hasPrevBallRotForRelease)
	            {
	                glm::quat deltaRot = ballRot * glm::inverse(usr->prevBallRotForRelease);
	                usr->releaseSpinFromRot = angularVelocityFromDelta(deltaRot, usr->deltaTimeLoan);
	            }
	            usr->prevBallRotForRelease = ballRot;
	            usr->hasPrevBallRotForRelease = true;
	        }

	        if (usr->phase == UserContext::Phase::THROW)
	        {
	            if (usr->throwingTime == 0.0f)
	            {
                if (usr->auroraVibe.value >= 4.0f)
                {
                    usr->auroraVibe.value += 4.0f;
                }
                float start = usr->auroraVibe.value;
                usr->auroraVibe.start(start, start + 1.0f, 1.5f);
                glm::vec3 movement = usr->phy.get_ball_swing_movement();
                movement *= usr->speedBoostAtThrow; // TUNABLET speed boost on throw
                usr->phy.set_ball_swing_movement(movement);
            }
		            // Take ball position back from physics
		            ballModel = usr->phy.physics_get_ball_matrix();

		            // Accumulate oil wear every frame based on forward travel this frame (meters).
		            // We split the wear between left/right using the same x->side blend as oil effect.
		            {
		                glm::vec3 prev = usr->lastBallPosition;
		                glm::vec3 cur = glm::vec3(ballModel[3]);
		                glm::vec3 d = cur - prev;
		                float travelM = std::isfinite(d.z) ? glm::max(0.0f, d.z) : 0.0f; // forward only
		                if (travelM > 0.0f && std::isfinite(cur.x) && std::isfinite(prev.x))
		                {
		                    float midX = 0.5f * (cur.x + prev.x);
		                    float oilBlendX = BallFrictionTuning::LANE_HALF_WIDTH_M - BallFrictionTuning::OIL_BLEND_GUTTER_MARGIN_M;
		                    oilBlendX = glm::max(0.01f, oilBlendX);
		                    float oilSideT = glm::clamp(((-midX) + oilBlendX) / (2.0f * oilBlendX), 0.0f, 1.0f); // 0=left, 1=right
		                    float leftShare = 1.0f - oilSideT;
		                    float rightShare = oilSideT;
		                    usr->oilWearLeftM += travelM * leftShare;
		                    usr->oilWearRightM += travelM * rightShare;
		                    usr->oilWearTotalM += travelM;
		                }
		            }

	            // Track if the ball's center has been above the lane since THROW started.
	            // Used to forgive glitched starts where the ball spawns under the lane.
	            if (std::isfinite(ballModel[3].y) && ballModel[3].y > 0.02f)
	                usr->throwEverAboveLane = true;

		            bool forgivenThrow = false;

		            auto ForgiveToIdleNoScore = [&](const char *reason)
		            {
		                if (usr->debugForgiveness)
		                    std::cerr << "[forgive] " << reason << " -> IDLE" << std::endl;
		                LogToIdle(usr, reason);
		                usr->bufferedRequestThrow = false;
		                usr->phy.set_ball_free();
		                const glm::vec3 IDLE_BALL_POS = Scene_IdleBallPos(usr->scene);
		                usr->carriedBall = IDLE_BALL_POS;
		                usr->carriedVel = glm::vec3(0.0f);
		                usr->phy.set_manual_ball_position(
		                    IDLE_BALL_POS, glm::quat(1.0f, 0, 0, 0), deltaTime
		                );
		                usr->phase = UserContext::Phase::IDLE;
		                usr->enjoy.resetJoystick();
		                usr->aimFlatPos = glm::vec2(0.5f, 0.5f);
		                usr->aimDownFlatPos = usr->aimFlatPos;
		                SDL_SetRelativeMouseMode(SDL_FALSE);
		                forgivenThrow = true;
		            };

		            // Forgiveness 1: under-lane glitch at start (must not count as a throw).
		            if (!usr->throwEverAboveLane && std::isfinite(ballModel[3].y) && ballModel[3].y < -0.10f)
		            {
		                ForgiveToIdleNoScore("THROW_UNDER_LANE_CANCEL");
		            }

		            // Forgiveness 2: backwards throw early (must not count as a throw).
		            if (!forgivenThrow)
		            {
		                glm::vec3 v = usr->phy.get_ball_swing_movement();
		                float totalThrowTime = usr->throwingTime + usr->settlingTime;
		                if (std::isfinite(v.z) && v.z < -0.20f && totalThrowTime < 1.0f)
		                {
		                    if (usr->debugForgiveness)
		                        std::cerr << "  vz=" << v.z << " t=" << totalThrowTime << std::endl;
		                    ForgiveToIdleNoScore("THROW_BACKWARDS_CANCEL");
		                }
		            }

		            if (forgivenThrow)
		            {
		                // Skip all scoring / completion logic.
		            }
		            else
		            {
		                // Throw time
		                if (ballModel[3].z < -2.5f && deltaTime > glm::epsilon<float>())
		                {
		                    usr->endSpeed =
		                        glm::length(glm::vec3(ballModel[3]) - usr->lastBallPosition) / deltaTime;
		                }
		                // Settling time
		                if (usr->phy.is_settling_started())
		                {
		                    usr->settlingTime += deltaTime;
		                }
		                else
		                {
		                    usr->throwingTime += deltaTime;
		                }

		                bool waitToSettle = usr->settlingTime < 3.0f && usr->throwingTime < 10.0f;
		                int state = usr->phy.checkThrowComplete(
		                    waitToSettle ? 0.1f : 100.0f, // Technically it will still wait to
		                                                  // settle if speed is very high
		                    -0.1f                         // floorLevel
		                );

		                int actualNumberOfBallsHit = usr->phy.get_number_of_impacts();
		                if (actualNumberOfBallsHit > usr->numberOfBallsHit)
		                {
		                    usr->sound.playSfxBallHitPins();
		                    usr->numberOfBallsHit += 1;
		                }
		                if (state != -1) // if got actuall score
		                {
		                    // Apply per-throw oil wear once per completed roll.
		                    // - Carrydown extends oil fade start/end forward
		                    // - Thickness decays based on total travel
		                    {
		                        auto ApplyCarrydownSide = [&](float &fadeStartM, float &fadeEndM, float wearM)
		                        {
		                            float s = fadeStartM;
		                            float e = fadeEndM;
		                            if (s > e)
		                                std::swap(s, e);
		                            float carryStart = usr->oilCarrydownPerBallTravelM * wearM;
		                            float ratio = (s > 1e-3f) ? (e / s) : 1.0f;
		                            float carryEnd = carryStart * ratio;
		                            fadeStartM = glm::clamp(s + carryStart, 0.0f, 18.3f);
		                            fadeEndM = glm::clamp(e + carryEnd, 0.0f, 18.3f);
		                            if (fadeStartM > fadeEndM)
		                                std::swap(fadeStartM, fadeEndM);
		                        };

		                        ApplyCarrydownSide(usr->leftOilFadeStartM, usr->leftOilFadeEndM, usr->oilWearLeftM);
		                        ApplyCarrydownSide(usr->rightOilFadeStartM, usr->rightOilFadeEndM, usr->oilWearRightM);

		                        float thicknessDrop = usr->oilThicknessDecayPerBallTravel * usr->oilWearTotalM;
		                        if (std::isfinite(thicknessDrop))
		                            usr->laneOilThickness = glm::clamp(usr->laneOilThickness - thicknessDrop, 0.0f, 1.0f);

		                        usr->oilWearLeftM = 0.0f;
		                        usr->oilWearRightM = 0.0f;
		                        usr->oilWearTotalM = 0.0f;
		                    }

		                    bool frameCompleted = addRoll(&usr->board, state - usr->wereDead);

		                    usr->wereDead += state;

		                    bool shouldResetAllPins = false;
		                    if (frameCompleted)
		                    {
		                        shouldResetAllPins = true;
		                        usr->wereDead = 0;
		                    }

		                    // camera must be moved when physics reset, to avoid one frame showing reset another
		                    // moving camera already luckily, camera will be following the ball later in the
		                    // frame
		                    ballModel[3] = glm::vec4(IDLE_BALL_POS, 1.0f);
		                    usr->phy.physics_reset(usr->initialPins, usr->ballStart, shouldResetAllPins);

		                    if (isGameFinished(&usr->board))
		                    {
		                        usr->phase = UserContext::Phase::RESULT;
		                        usr->windowStack.windowStackPushNewGameWindow();
		                        // Player submits a score
		                        char safeUsername[20];
		                        memcpy(safeUsername, usr->username, 20);
		                        safeUsername[20 - 1] = '\0';

		                        bool madeIt = LocalHi_SubmitScore(
		                            &usr->localHi, usr->username, usr->username_len, usr->board.totalScore
		                        );

		                        if (madeIt)
		                        {
		                            printf(
		                                "🎉 New record %d! Rank #%d\n",
		                                usr->localHi.lastSubmittedScore,
		                                usr->localHi.lastSubmittedRank
		                            );
		                        }
		                        else
		                        {
		                            printf(
		                                "You scored %d (%.1fth percentile)\n",
		                                usr->localHi.lastSubmittedScore,
		                                usr->localHi.lastSubmittedPercentile
		                            );
		                        }

		                        usr->clayton.shouldShowHiScore = true;
		                        usr->clayton.shouldShowHiScoreWithLatest = true;
		                        usr->windowStack.windowStackPushLocalHiscoreWindow();
		                    }
		                    else
		                    {
		                        LogToIdle(usr, "THROW_DONE_TO_IDLE");
		                        usr->phase = UserContext::Phase::IDLE;
		                        usr->enjoy.resetJoystick();
		                        usr->aimFlatPos = glm::vec2(0.5f, 0.5f);
		                        usr->aimDownFlatPos = usr->aimFlatPos;
		                    }
		                }
		            }
	        }
        else if (usr->phase == UserContext::Phase::RESULT)
        {
            ballModel = usr->phy.physics_get_ball_matrix();
        }
        else if (usr->phase == UserContext::Phase::FINAL_RESULT)
        {
        }
    }

    float physicsInterval = 0.500f; // Default physics is 2 times a second
    if (usr->phase == UserContext::Phase::IDLE)
    {
        physicsInterval = 0.020f; // make sure they don't fall through when restocked
    }
    if (usr->phase == UserContext::Phase::AIM)
    {
        physicsInterval = 0.050f; // Aiming does not require too frequent
    }
    if (usr->phase == UserContext::Phase::SWING)
    {
        physicsInterval = 0.005f; // Swing most intense because of the launch time
    }
    if (usr->phase == UserContext::Phase::THROW)
    {
        if (ballModel[3].z > -1.0f)
        {
            physicsInterval = 0.005f; // throw most intense before the end
        }
        else
        {
            physicsInterval = 0.015f; // Otherwise moderate
            // Note it requires more spin if not frequent enough
        }
    }
    if (usr->phase == UserContext::Phase::RESULT)
    {
        physicsInterval = 0.005f; //
                                  // Swing most intense because of the launch time
    }
    usr->phy.physics_step(deltaTime * 1.0f, physicsInterval);

    Carousel_Update(&usr->carousel, deltaTime);

    BallStats_EveryFrame(usr, ballModel);

		    float eyeZ = glm::clamp(
		        ballModel[3].z + usr->scene.camEyeZFromBall,
		        usr->scene.camEyeZMin,
		        usr->scene.camEyeZMax
		    );
		    float targetZ = glm::clamp(
		        ballModel[3].z + usr->scene.camTargetZFromBall,
		        usr->scene.camTargetZMin,
		        usr->scene.camTargetZMax
		    );
		    usr->cameraMat = glm::lookAt(
		        glm::vec3(0.0f, usr->scene.camEyeY, eyeZ),
		        glm::vec3(0.0f, usr->scene.camTargetY, targetZ),
		        glm::vec3(0.0f, 1.0f, 0.0f)
		    );
		    usr->cameraMat[3][0] = usr->pivotPoint.x;

    SDL_GL_GetDrawableSize(ctx->sdlWindow, &ctx->screenWidth, &ctx->screenHeight);

    int decalIndex = 0;
    for (int i = 0; i < 7; i++)
    {
        Decal &dot = usr->decalBatch.decals[decalIndex];
        dot.enabled.x = 1;
        dot.transform = glm::translate(
                            glm::mat4(1.0f),
                            glm::vec3(
                                (i - 3.0f) * 0.13295f, // Every 13.295cm (= 1.0636m / 8)
                                0.001f,                // at 1mm over lane
                                -(18.3f - 1.83f)       // 6ft from us
                            )
                        ) *
            glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0)) *
            glm::scale(glm::mat4(1.0f), glm::vec3(0.125f * 0.5f));
        // Atlas UVs (top-left quarter, for example)
        dot.uvStart = glm::vec2(0.875f, 0.875f);
        dot.uvEnd = glm::vec2(1.0f, 1.0f);
        decalIndex += 1;
    }
    for (int i = 0; i < 7; i++)
    {
        Decal &dot = usr->decalBatch.decals[decalIndex];
        dot.enabled.x = 1;
        dot.transform =
            glm::translate(
                glm::mat4(1.0f),
                glm::vec3(
                    (i - 3.0f) * 0.13295f, // Every 13.295cm (= 1.0636m / 8)
                    0.001f,                // at 1mm over lane
                    -(18.3f - 3.6576 - ((1.0f - glm::abs(i - 3.0f)) * 0.305f)) // 12' 16' from us
                )
            ) *
            glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0)) *
            glm::scale(glm::mat4(1.0f), glm::vec3(0.125f));
        // Atlas UVs (top-left quarter, for example)
        dot.uvStart = glm::vec2(0.875f - 0.125f, 1.0f);
        dot.uvEnd = glm::vec2(0.875, 0.875f);
        decalIndex += 1;
    }
    if (usr->phase == UserContext::Phase::AIM)
    {
        glm::vec3 a = usr->pivotPoint;
        glm::vec3 b = usr->carriedBall;

        Decal &line = usr->decalBatch.decals[decalIndex];

        if (a.z < b.z + 0.15f)
        { // half ball
            line.enabled.x = 0;
            goto END_LINE;
        }

        // --- Ground direction (XZ) ---
        glm::vec2 aXZ(a.x, a.z);
        glm::vec2 bXZ(b.x, b.z);

        glm::vec2 dirXZ = bXZ - aXZ;
        float dirLen = glm::length(dirXZ);
        if (dirLen < 0.0001f)
        {
            std::cerr << "dirlen " << dirLen << std::endl;
            line.enabled.x = 0;
            goto END_LINE;
        }
        line.enabled.x = 1;

        dirXZ /= dirLen;

        float throwGroundAngle = std::atan2(dirXZ.x, dirXZ.y);

        // --- Lane geometry (metres) ---
        constexpr float FT = 0.3048f;
        constexpr float IN = 0.0254f;

        // float zCentre = (-12.0f + 1.0f) * FT;              // centre arrows
        // float zSide   = (-16.0f + 1.0f) * FT;              // side arrows
        // float halfWidthAtSide = (41.875f * IN) * 0.5f;

        // --- Fixed arrow boundary points ---
        float sideSign = (b.x >= 0.0f) ? 1.0f : -1.0f;
        glm::vec3 p1(sideSign * -0.5f * 41.857f * IN - a.x, 0.0f, (-60.0f + 12.0f + 8.0f) * FT);

        glm::vec3 p2(0.0f + a.x * 1.0f, 0.0f, (-60.0f + 16.0f + 8.0f) * FT);

        // --- Directions ---
        glm::vec3 d1 = p2 - p1; // boundary line direction
        glm::vec3 d2 = b - a;   // throw line direction

        // Solve intersection in XZ
        float denom = d1.x * d2.z - d1.z * d2.x;

        float length = 10.0f;
        if (std::abs(denom) < 1e-6f)
        {
            // Lines are parallel or coincident → no single intersection
        }
        else
        {
            glm::vec3 diff = a - p1;

            float t = (diff.x * d2.z - diff.z * d2.x) / denom;

            glm::vec3 intersection = p1 + t * d1;
            intersection.y = 0.0f;

            length = glm::length(intersection - a);
            // intersection is the crossing point
        }
        float midX = a.x;
        float midZ = a.z;
        // --- Build decal ---
        line.enabled.x = 1;

        line.transform = glm::translate(glm::mat4(1.0f), glm::vec3(midX, 0.001f, midZ)) *
            glm::rotate(glm::mat4(1.0f), throwGroundAngle, glm::vec3(0, 1, 0)) *
            glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0)) *
            glm::scale(glm::mat4(1.0f), glm::vec3(0.05f, length, 1.0f));

        float offset = std::fmod(usr->aimingTime, length);
        line.uvStart = glm::vec2(0.25f, 0.0f);
        line.uvEnd = glm::vec2(0.25f + 0.125f, 1.0f * length + offset);
        decalIndex += 1;
    }
    else
    {
        Decal &line = usr->decalBatch.decals[decalIndex];
        line.enabled.x = 1;
    }
END_LINE:
    if (1 == 2)
    {
        Decal &test = usr->decalBatch.decals[decalIndex];
        test.enabled = glm::ivec4(1);
        test.transform = glm::lookAt(
            glm::vec3(0.0f, 0.0f, -18.4f), glm::vec3(0.0f, 0.0f, -5.0f), glm::vec3(0, 1, 0)
        );

        test.uvStart = glm::vec2(0.0f, 0.0f);
        test.uvEnd = glm::vec2(1.0f, 1.0f);

        decalIndex += 1;
    }

	    // ===== [NEW] PRE-PASS: Render ball to texture for UI =====

    // (Do this AFTER ballModel is computed, BEFORE any rendering)
	    ZONE("RENDER TO TEXTURE FRAMEBUFFER")
	    {
        float step = 1.0f / 16.0f;
        usr->mainShader.updateDiffuseTexture(usr->everythingTexture);

        // ── Icon camera: closer + simple ──
        const glm::mat4 iconView = glm::lookAt(
            glm::vec3(0.0f, 0.60f, 1.0f), // eye
            glm::vec3(0.0f, 0.0f, 0.0f),  // center
            glm::vec3(
                0.0f, -1.0f, 0.0f
            ) // up, normally it is possitive Y, but this is tocompensate Y flip
        );
        const glm::mat4 iconProj = glm::perspective(glm::radians(30.0f), 1.0f, 0.1f, 50.0f);

        // ── Animated model: spin + gentle bob ──
        float t = usr->globalTime;
        glm::mat4 iconModel = glm::translate(
            glm::mat4(1.0f), glm::vec3(0.0f, glm::sin(t * 4.0f) * 0.05f, 0.0f)
        );                                                                         // subtle bob
        iconModel = glm::rotate(iconModel, t * 2.0f, glm::vec3(0.0f, 1.0f, 0.0f)); // smooth spin
        // ── Bind FBO ──

        usr->mainShader.updateLightPos(
            glm::vec3(2.0f, 3.0f, 2.0f) // fixed front-top-right for consistent icon lighting
        );
        if (usr->carousel.closestBallIdx != -1)
        {
            usr->ballRenderTex.bindForWriting();
            glClearColor(0, 0, 0, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);
            glDepthMask(GL_TRUE);
            // ── Render ──
            int ballId = usr->carousel.items[usr->carousel.closestBallIdx].id;
            float stepx = 1.0f + step * 2.0f * (float)(ballId / 16);
            float stepy = 1.0f + step * (float)(ballId % 16);
            usr->mainShader.updateTextureParamsInOneGo(
                glm::vec3(1.0f, 1.0f, 1.0f), // Texture density
                glm::vec2(1.0f, 1.0f),       // Size of one tile compared to full atlas
                glm::vec2(stepx, stepy),     // Atlas region start
                1.0f                         // Atlas region scale compared to entire atlas
            );
            usr->mainShader.renderRealMesh(usr->ballMesh, iconModel, iconView, iconProj);
            checkOpenGLError("icon-ball");

            // ── Restore ──
            usr->ballRenderTex.unbind(
                ctx->screenWidth * ctx->pixelRatio, ctx->screenHeight * ctx->pixelRatio
            );
        }
	        if (usr->carousel.closest2ndBallIdx != -1)
	        {
            usr->ballRenderTex2.bindForWriting();
            glClearColor(0, 0, 0, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);
            glDepthMask(GL_TRUE);
            // ── Render ──
            int ballId = usr->carousel.items[usr->carousel.closest2ndBallIdx].id;
            float stepx = 1.0f + step * 2.0f * (float)(ballId / 16);
            float stepy = 1.0f + step * (float)(ballId % 16);
            usr->mainShader.updateTextureParamsInOneGo(
                glm::vec3(1.0f, 1.0f, 1.0f), // Texture density
                glm::vec2(1.0f, 1.0f),       // Size of one tile compared to full atlas
                glm::vec2(stepx, stepy),     // Atlas region start
                1.0f                         // Atlas region scale compared to entire atlas
            );
            usr->mainShader.renderRealMesh(usr->ballMesh, iconModel, iconView, iconProj);
            checkOpenGLError("icon-ball");

            // ── Restore ──
            usr->ballRenderTex2.unbind(
                ctx->screenWidth * ctx->pixelRatio, ctx->screenHeight * ctx->pixelRatio
            );
	        }

	        // Oil preview (only when Oil Status window is visible).
	        if (usr->clayton.shouldShowOilStatus)
	        {
	            usr->oilRenderTex.bindForWriting();
	            glDisable(GL_DEPTH_TEST);
	            glClearColor(0, 0, 0, 0);
	            glClear(GL_COLOR_BUFFER_BIT);

		            usr->oilMap.render(
		                18.3f,
		                usr->leftOilFadeStartM,
		                usr->leftOilFadeEndM,
		                usr->rightOilFadeStartM,
		                usr->rightOilFadeEndM,
		                usr->laneOilThickness,
		                glm::clamp(usr->lanePushbackStrength / 50.0f, 0.0f, 1.0f)
		            );

	            usr->oilRenderTex.unbind(
	                ctx->screenWidth * ctx->pixelRatio, ctx->screenHeight * ctx->pixelRatio
	            );
	        }
	    }

    ZONE("3D render")
    {

        // ===== [END NEW PRE-PASS] =====
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE); // Depth write if set

        glClearColor(0.1f, 0.2f, 0.1f, 1.0f);

        usr->auroraVibe.update(deltaTime);
        usr->aurora.renderAurora(
            deltaTime * TUNE,
            glm::inverse(usr->cameraMat),
            usr->auroraVibe.value
        ); //  * projectionMatrix);

        // usr->tri.render(usr->everythingTexture.id);

        usr->mainShader.updateLightPos(
            glm::vec3(3.0f, 3.0f, glm::clamp(usr->cameraMat[3].z + 6.0f, -100.0f, -7.0f))
        );
        usr->mainShader.updateDiffuseTexture(usr->everythingTexture);
        usr->mainShader.updateTextureParamsInOneGo(
            glm::vec3(1.0f, 1.0f, 1.0f), // Texture density
            glm::vec2(1.0f, 1.0f),       // Size of one tile compared to full atlas
            glm::vec2(1.0f),             // Atlas region start
            1.0f                         // Atlas region scale compared to entire atlas
        );

        // TODO optimize to instanced render
        for (int i = 0; i < 10; i++)
        {
            if (usr->phy.mPinDead[i])
            {
                // continue;
            }
            glm::mat4 pinModel = usr->phy.physics_get_pin_matrix(i);
            float halfHeight = 0.19f;
            pinModel = glm::translate(pinModel, glm::vec3(0.0f, -halfHeight, 0.0f));
            usr->mainShader.renderRealMesh(
                usr->pinMesh, pinModel, usr->cameraMat, usr->perspectiveMat
            );
            checkOpenGLError("stare");
        }

        /*
         * Mostly for decals other bodies are not even see-through
         */
        glEnable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);

        float step = 1.0f / 16.0f;
        int ballId = usr->myBall.id;
        float stepx = 1.0f + step * 2.0f * (float)(ballId / 16);
        float stepy = 1.0f + step * (float)(ballId % 16);
        usr->mainShader.updateTextureParamsInOneGo(
            glm::vec3(1.0f, 1.0f, 1.0f), // Texture density
            glm::vec2(1.0f, 1.0f),       // Size of one tile compared to full atlas
            glm::vec2(stepx, stepy),     // Atlas region start
            1.0f                         // Atlas region scale compared to entire atlas
        );
        usr->mainShader.renderRealMesh(
            usr->ballMesh, ballModel, usr->cameraMat, usr->perspectiveMat
        );
        // restore defaults
        usr->mainShader.updateTextureParamsInOneGo(
            glm::vec3(1.0f, 1.0f, 1.0f), // Texture density
            glm::vec2(1.0f, 1.0f),       // Size of one tile compared to full atlas
            glm::vec2(1.0f),             // Atlas region start
            1.0f                         // Atlas region scale compared to entire atlas
        );

        usr->mainShader.renderRealMesh(
            usr->laneMesh,
            glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -.0f, .0f)),
            usr->cameraMat,
            usr->perspectiveMat
        );

        usr->globalTime += deltaTime;

        // coin_update.cpp — Call this once per frame from your main update loop
        // Assumes: usr->coinLane, usr->globalTime, deltaTime, ctx->screenWidth/Height, etc.

        usr->lastBallPosition = ballModel[3];

        // 1. Update coin physics/collision FIRST (sets Collected state)
        usr->coinLane.updateStars(usr->lastBallPosition, ballModel[3], usr->globalTime, deltaTime);

        // 2. Update all flying coin animations
        usr->carousel.bank += usr->coinLane.updateFlyAnimations(deltaTime);

        // 3. Cleanup finished fly animations (free slots for new coins)
        usr->coinLane.cleanupFinishedFlyAnimations();

        // 4. Detect newly collected coins and spawn fly animations
        const auto &coins = usr->coinLane.getCoins(); // ✅ Keep this line
        for (int i = 0; i < usr->coinLane.getActiveCount(); ++i)
        {
            const Coin &coin = coins[i];

            // ✅ Simplified condition
            if (coin.state == CoinState::Collected && !coin.flyTriggered)
            {

                glm::vec4 viewport(
                    0.0f,
                    0.0f,
                    static_cast<float>(ctx->screenWidth),
                    static_cast<float>(ctx->screenHeight)
                );
                glm::vec3 screenPos =
                    glm::project(coin.position, usr->cameraMat, usr->perspectiveMat, viewport);
                glm::vec2 hudTarget = usr->placeOfMoney + glm::vec2(30.0f, 30.0f);

                if (usr->coinLane.spawnFlyAnimation(glm::vec2(screenPos.x, screenPos.y), hudTarget))
                {
                    usr->coinLane.markFlyTriggered(i); // ✅ Mark via helper method
                    usr->sound.playSfxCoinPickup();
                }
            }
        }
        // 5. Add coin to bank if
        // usr->coinLane.getNewlyCollected =

        // Render 3D coins in perspective view
        for (int i = 0; i < usr->coinLane.getActiveCount(); i++)
        {
            const Coin &coin = usr->coinLane.getCoins()[i];

            // Skip rendering in 3D if this coin is currently flying to HUD as 2D sprite
            // Condition: collected + fly animation spawned + still in early implosion (visual
            // overlap window)
            if (coin.state == CoinState::Collected && coin.flyTriggered)
            {
                continue;
            }

            if (coin.isRenderable())
            {
                usr->mainShader.renderRealMesh(
                    usr->starMesh,
                    glm::scale(coin.transform, glm::vec3(0.25f)),
                    usr->cameraMat,
                    usr->perspectiveMat
                );
            }
        }
        renderFlyingCoins(
            &usr->mainShader,
            &usr->starMesh,
            &usr->everythingTexture,
            &usr->coinLane,
            (float)ctx->screenWidth,
            (float)ctx->screenHeight,
            true, // vary
            usr->hudAboveThis
        );
        usr->decalBatch.renderDecals(
            usr->everythingTexture.id, // Atlas for all decals
            usr->cameraMat,            // view to world
            usr->perspectiveMat,       // projection
            decalIndex                 // how many decals added this frame
        );

        // #ifndef __EMSCRIPTEN__
        //         glDisable(GL_BLEND); // I think i need it but it breaks mac angle build
        // #endif
        // glDisable(GL_DEPTH_TEST);

        // {
        //     const glm::vec3 eye = glm::vec3(4.0f);
        //     const glm::vec3 center = glm::vec3(0.0f);
        //     const glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

        //     usr->cameraMat = glm::lookAt(eye, center, up);
        // }
        glm::mat4 m = glm::mat4(3.0f);

        if (usr->phase < UserContext::Phase::SWING)
        {
            usr->enjoy.renderJoystick(ctx->screenWidth, ctx->screenHeight);
            usr->circle.resetCircle();
        }
        else if (usr->phase == UserContext::Phase::SWING)
        {
            usr->enjoy.renderJoystick(ctx->screenWidth, ctx->screenHeight);
        }
        else if (usr->phase == UserContext::Phase::THROW)
        {
            usr->circle.renderCircle(ctx->screenWidth, ctx->screenHeight);
        }
        else
        {
            usr->enjoy.resetJoystick();
        }
    }

    ZONE("clay")
    {
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE); // Clay is simple and never writes to depth buffer

        ClayArena_Reset(&usr->clayton.clayArena);

        float portraitWidth = ctx->screenWidth;
        float portraitHeight = ctx->screenHeight;
        float ratio = portraitWidth / portraitHeight;

        float goldenConstant
            // = 9.0f / 16.0f;
            = 480.0f / 720.0f;
        int downsizeWidth = 600;

        // Reconfigure based on screen size
        uint16_t portraitPadding;
        ZONE("portraitPadding")
        {

            if (portraitWidth < downsizeWidth)
            {
                portraitPadding = 5;
                usr->clayton.smallFontCfg.fontSize = 16;
            }
            else
            {
                portraitPadding = 10;
                usr->clayton.smallFontCfg.fontSize = 32;
            }
        }

        if (ratio > goldenConstant)
        {
            // No side spacers that cover sky are visible because we very portraity
            portraitWidth = portraitHeight * goldenConstant;
        }

        int scoreBoardWidth = portraitWidth - portraitPadding * 2;

        Clay_Color buttonColor = {40, 160, 240, 255};
        char joystickLabel[200];

        // Clay_SetLayoutDimensions((Clay_Dimensions){(float)ctx->screenWidth,
        // (float)ctx->screenHeight * ctx->pixelRatio});
        //     Clay_UpdateScrollContainers(
        //     true,
        //     (Clay_Vector2){scrollDelta.x, scrollDelta.y},
        //     deltaTime);
        Clay_BeginLayout();

        // When any modal/window is present, the window-stack overlay dims the whole screen.
        // The side spacers should become fully transparent so the overlay is the only tint.
        Clay_Color sideSpacerBg =
            (usr->windowStack.count > 0) ? (Clay_Color){255, 255, 255, 0}
                                         : (Clay_Color){255, 255, 255, 100};

        CLAY(
            CLAY_ID("Root"),
            {
                .layout = {
                    .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                    .padding = {0, 0, 0, 0},
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                },

            }
        )
        {
            CLAY(
                CLAY_ID("Left spacer"),
                {
                    .layout =
                        {
                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                        },
                    .backgroundColor = sideSpacerBg,
                }
            ){};

            CLAY(
                CLAY_ID("Portrait area"),
                {
                    .layout = {
                        .sizing{
                            .width = CLAY_SIZING_FIXED(portraitWidth),
                            .height = CLAY_SIZING_FIXED(portraitHeight),
                        },
                        .childAlignment =
                            {
                                .x = CLAY_ALIGN_X_CENTER,
                                .y = CLAY_ALIGN_Y_CENTER,
                            },
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    },
                }
            )

            {

                CLAY(CLAY_ID("NotchArounds1"), CLAY_THEME_TOP_BAR)
                {
                    // std::cerr << "renameID: " << usr->renameButton.clayId.stringId.chars <<
                    // std::endl;
                }
                CLAY(
                    CLAY_ID("Content body1"),
                    {.layout = {
                         .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                         .padding =
                             {portraitPadding, portraitPadding, portraitPadding, portraitPadding},
                         .layoutDirection = CLAY_TOP_TO_BOTTOM,
                     }}
                )
                {

                    CLAY(
                        CLAY_ID("NameAndMoneyRow"),
                        {.layout =
                             {
                                 .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                                 .padding = {.top = 0, .bottom = portraitPadding},
                                 .childGap = 10,
                                 .childAlignment =
                                     {
                                         .x = CLAY_ALIGN_X_CENTER,
                                         .y = CLAY_ALIGN_Y_CENTER,
                                     },
                             }}
                    )
                    {
                        CLAY(usr->renameButton.clayId, CLAY_THEME_BTN_HUD)
                        {
                            Clay_String cs = Clay_String{
                                .isStaticallyAllocated = false,
                                .length = usr->username_len,
                                .chars = usr->username,
                            };
                            CLAY_TEXT(cs, CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_BUTTON));
                        }
                        CLAY(
                            CLAY_ID("PlaceOfNotchSpacer"),
                            {
                                .layout = {
                                    .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                                    .padding = {10, 10, 10, 10},
                                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                                },
                            }
                        )
                        {
                        }
                        CLAY(CLAY_ID("PlaceOfMoney"), CLAY_THEME_BTN_HUD)
                        {

                            ClayArena *arena = &usr->clayton.clayArena; // ← Embedded arena
                            char bankAmountBuf[64];
                            int len = snprintf(
                                bankAmountBuf, sizeof(bankAmountBuf), "$ %d", usr->carousel.bank
                            );
                            Clay_String bankAmount = ClayArena_AllocString(arena, bankAmountBuf);
                            CLAY_TEXT(bankAmount, CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_BUTTON));
                        }
                    }

                    // Scoreboard
                    usr->clayton.constructClayScoreboard(
                        &usr->board, scoreBoardWidth, usr->username, &usr->username_len
                    );

                    CLAY(
                        CLAY_ID("MenuAndShopRow"),
                        {.layout =
                             {
                                 .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                                 .padding = {.top = portraitPadding, .bottom = portraitPadding},
                                 .childGap = portraitPadding,
                                 .childAlignment =
                                     {
                                         .x = CLAY_ALIGN_X_CENTER,
                                         .y = CLAY_ALIGN_Y_CENTER,
                                     },
                             }}
                    )
                    {

                        CLAY(
                            usr->menuButton.clayId, CLAY_THEME_BTN_HUD
                        )
                        {
                            CLAY_TEXT(CLAY_STRING("MENU"), CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_BUTTON));
                        }
                /*
        CLAY(CLAY_ID("NotchArounds2"), CLAY_THEME_TOP_BAR)
                */

                // SOUND button next to MENU
	                    CLAY(usr->soundButton.clayId, CLAY_THEME_BTN_HUD)
	                    {
	                        CLAY_TEXT(CLAY_STRING("SOUND"), CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_BUTTON));
	                    }

	                    CLAY(usr->oilButton.clayId, CLAY_THEME_BTN_HUD)
	                    {
	                        CLAY_TEXT(CLAY_STRING("OIL"), CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_BUTTON));
	                    }

	                CLAY(
	                    CLAY_ID("Menu and Shop Bar Grower"),
                    {
                        .layout = {
                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},

                        },

                    }
                )
                {
                }

                CLAY(usr->hiScoreButton.clayId, CLAY_THEME_BTN_HUD)
                {
                    CLAY_TEXT(CLAY_STRING("HI-SCORE"), CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_BUTTON));
                }

                CLAY(usr->openShopClick.clayId, CLAY_THEME_BTN_HUD)
                {
                    CLAY_TEXT(CLAY_STRING("SHOP"), CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_BUTTON));
                }
            };

            CLAY(
                CLAY_ID("Content Grower"),
                {
                    .layout = {
                        .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},

                    },
                }
            )
            {
            }
        };
        CLAY_AUTO_ID(
            {.layout =
                 {
                     .sizing = {.width = CLAY_SIZING_GROW(0)},
                     .padding = {10, 10, 3, 3},
                     .childGap = 12,
                     .layoutDirection = CLAY_LEFT_TO_RIGHT,
                 },
             .backgroundColor = {0, 0, 0, 100}}

        )
        {
            ClayArena *arena = &usr->clayton.clayArena;
            Clay_String cs = {
                .isStaticallyAllocated = false,
                .length = (int32_t)usr->fpsCounter.fpsTextLen,
                .chars = usr->fpsCounter.fpsText
            };
            Clay_TextElementConfig fpsElementConfig = {
                .textColor = CLAY_COLOR_TEXT_PRIMARY,
                .fontId = CLAY_FONT_NOTO,
                .fontSize = usr->clayton.smallFontCfg.fontSize,
            };
            CLAY_TEXT(cs, CLAY_TEXT_CONFIG(fpsElementConfig));

            const char *phaseName = "UNKNOWN";
            switch (usr->phase)
            {
            case UserContext::Phase::IDLE:
                phaseName = "IDLE";
                break;
            case UserContext::Phase::AIM:
                phaseName = "AIM";
                break;
            case UserContext::Phase::SWING:
                phaseName = "SWING";
                break;
            case UserContext::Phase::THROW:
                phaseName = "THROW";
                break;
            case UserContext::Phase::RESULT:
                phaseName = "RESULT";
                break;
            case UserContext::Phase::FINAL_RESULT:
                phaseName = "FINAL_RESULT";
                break;
            case UserContext::Phase::MENU:
                phaseName = "MENU";
                break;
            }

            Clay_String phaseStr = ClayArena_FormatString(arena, "Phase: %s", phaseName);
            CLAY_TEXT(phaseStr, CLAY_TEXT_CONFIG(fpsElementConfig));
        }

        if (usr->phase == UserContext::Phase::THROW)
        {

            unsigned short halfTextH = 12;
            Clay_Vector2 joystickOffset = {0, ctx->screenHeight * 0.75f}; // 1/4 bellow centre
            CLAY(
                CLAY_ID("FloatingOverJoystickContainer"),
                {
                    .layout =
                        {
                            .sizing =
                                {.width = CLAY_SIZING_PERCENT(0.5),
                                 .height = CLAY_SIZING_PERCENT(0.125)},
                            .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
                        },
                    .floating = {
                        .offset = joystickOffset,
                        .zIndex = 1,
                        .attachPoints =
                            {CLAY_ATTACH_POINT_CENTER_CENTER, CLAY_ATTACH_POINT_CENTER_TOP},
                        .attachTo = CLAY_ATTACH_TO_PARENT,
                    },
                }
            )
            {
                CLAY(
                    CLAY_ID("FloatingOverJoystickTextWrapper"), // wrap it in order to center
                    {
                        .layout =
                            {
                                .sizing = {.width = CLAY_SIZING_FIT(), .height = CLAY_SIZING_FIT()},
                                .padding = {10, 10, 10, 10},
                            },
                        .backgroundColor = {255, 1, 2, 100},
                    }
                )
                {
                    int joystickLabelLen;
                    if (glm::abs(usr->circles) < 1)
                    {
                        joystickLabelLen =
                            snprintf(joystickLabel, sizeof(joystickLabel), "Spin\nto Hook");
                    }
                    else if (usr->totalAngle > 0)
                    {
                        joystickLabelLen = snprintf(
                            joystickLabel,
                            sizeof(joystickLabel),
                            "Left %.2f",
                            -usr->smoothedAngularVelocity
                        );
                    }
                    else
                    {
                        joystickLabelLen = snprintf(
                            joystickLabel,
                            sizeof(joystickLabel),
                            "Right %.2f",
                            -usr->smoothedAngularVelocity
                        );
                    }
                    Clay_String cs = {
                        .isStaticallyAllocated = false,
                        .length = joystickLabelLen,
                        .chars = joystickLabel
                    };
                    CLAY_TEXT(
                        cs,
                        CLAY_TEXT_CONFIG({
                            .textColor = {255, 255, 255, 255},
                            .fontId = CLAY_FONT_NOTO,
                            .fontSize = 16,
                        })
                    );
                }
            }
        }

    };
    CLAY(
        CLAY_ID("Right spacer"),
        {
            .layout =
                {
                    .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                },
            .backgroundColor = sideSpacerBg,
        }
    ){};

    // Render window stack as floating layers attached to Root so the dim overlay covers the entire
    // screen (including the left/right spacers).
        OilStatusUI oilStatus = {};
        oilStatus.laneFriction = usr->laneFriction;
        oilStatus.lanePushbackStrength = usr->lanePushbackStrength;
        oilStatus.houseOilThickness = usr->houseLane.laneOilThickness;
        oilStatus.currentOilThickness = usr->laneOilThickness;
        oilStatus.leftOilFadeStartM = usr->leftOilFadeStartM;
        oilStatus.leftOilFadeEndM = usr->leftOilFadeEndM;
        oilStatus.rightOilFadeStartM = usr->rightOilFadeStartM;
        oilStatus.rightOilFadeEndM = usr->rightOilFadeEndM;
        oilStatus.oilWearLeftM = usr->oilWearLeftM;
        oilStatus.oilWearRightM = usr->oilWearRightM;
        oilStatus.oilWearTotalM = usr->oilWearTotalM;
        oilStatus.oilCarrydownPerBallTravelM = usr->oilCarrydownPerBallTravelM;
        oilStatus.oilThicknessDecayPerBallTravel = usr->oilThicknessDecayPerBallTravel;

        oilStatus.estCarryStartLeftM = usr->oilCarrydownPerBallTravelM * usr->oilWearLeftM;
        oilStatus.estCarryStartRightM = usr->oilCarrydownPerBallTravelM * usr->oilWearRightM;
        oilStatus.estThicknessDrop = usr->oilThicknessDecayPerBallTravel * usr->oilWearTotalM;

	    usr->windowStack.renderWindowStack(
	        &usr->clayton,
	        &usr->keypad,
	        &usr->sound.settings,
	        &usr->adaptiveAudio,
	        &usr->localHi,
	        &usr->carousel,
	        usr->shouldShowShop,
            &oilStatus
	    );
	}

	Clay_RenderCommandArray cmds = Clay_EndLayout();

// int mouseX = 0;
// int mouseY = 0;
// Uint32 mouseState = SDL_GetMouseState(&mouseX, &mouseY);
// Clay_Vector2 mousePosition = (Clay_Vector2){(float)mouseX, (float)mouseY};
// Clay_SetPointerState(mousePosition, mouseState & SDL_BUTTON(1));

Clay_UpdateScrollContainers(true, (Clay_Vector2){scrollDelta.x, scrollDelta.y}, deltaTime);

// SDL_GL_GetDrawableSize(ctx->sdlWindow, ctx->screenWidth * ctx->pixelRatio,
// ctx->screenHeight*ctx->pixelRatio); glViewport(0, 0, ctx->screenWidth, ctx->screenHeight);

// glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
// glDisable(GL_DEPTH_TEST);
// glDepthMask(GL_FALSE); // Clay renderer is simple and never writes to depth buffer

usr->clayton.renderClayton(cmds, ctx->screenWidth, ctx->screenHeight, deltaTime);

// === DEBUG: Before rendering coins ===
// debugCoinRenderState("BEFORE_COIN_RENDER");
Clay_ElementId menuAndShopRow = CLAY_ID("MenuAndShopRow");
Clay_BoundingBox hudBottom = Clay_GetElementData(menuAndShopRow).boundingBox;
usr->hudAboveThis = ctx->screenHeight - (hudBottom.y + hudBottom.height);
Clay_ElementId id = CLAY_ID("PlaceOfMoney");
Clay_BoundingBox box = Clay_GetElementData(id).boundingBox;
usr->placeOfMoney = glm::vec2(
    box.x + (box.width - CoinFlyConfig::PIXEL_SIZE) * 0.125f,
    ctx->screenHeight - (box.height * 0.5f + box.y) - 20.0f
);
// === PASS 3: Flying Coins (Ortho Overlay) ===

glUseProgram(usr->mainShader.id);
renderFlyingCoins(
    &usr->mainShader,
    &usr->starMesh,
    &usr->everythingTexture,
    &usr->coinLane,
    (float)ctx->screenWidth,
    (float)ctx->screenHeight,
    false, // vary
    usr->hudAboveThis
);

// Restore state
glEnable(GL_DEPTH_TEST);
glDepthMask(GL_TRUE);
}

bool isGugucas = (usr->username_len == 7 && memcmp(usr->username, "GUGUCAS", 7) == 0);
usr->shouldShowImgui = isGugucas;
if (usr->shouldShowImgui)
{
    usr->imgui.beginImgui();

    /* ball stats editor */ {
        if (!usr->shouldShowImgui)
            return;

        ImGui::Begin("🎳 Ball Tuner (Live)");

        // ── HELPER: Inline status label ────────────────────────────────
        auto DrawStatus = [](float val, float min, float max)
        {
            ImGui::SameLine();
            if (val < min)
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "[less]");
            else if (val > max)
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "[more]");
            else
                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "[in range]");
        };

        // ── HELPER: Push changes to physics immediately ────────────────
        auto ApplyLive = [&]()
        {
            usr->myBall = usr->imguiBall;
            BallStats_ApplyCatalog(usr, usr->myBall);
        };

        ImGui::Text("Catalog Properties (0.0–1.0)");
        ImGui::Separator();

        // Mass
        {
            bool changed = ImGui::SliderFloat("Mass", &usr->imguiBall.mass, 0.0f, 1.0f, "%.3f");
            DrawStatus(
                usr->imguiBall.mass,
                BallPhysicsMapping::CATALOG_MASS_MIN,
                BallPhysicsMapping::CATALOG_MASS_MAX
            );
            if (changed)
                ApplyLive();
        }
        // Spin
        {
            bool changed = ImGui::SliderFloat("Spin", &usr->imguiBall.spin, 0.0f, 1.0f, "%.3f");
            DrawStatus(
                usr->imguiBall.spin,
                BallPhysicsMapping::CATALOG_SPIN_MIN,
                BallPhysicsMapping::CATALOG_SPIN_MAX
            );
            if (changed)
                ApplyLive();
        }
        // Skid
        {
            bool changed = ImGui::SliderFloat("Skid", &usr->imguiBall.skid, 0.0f, 1.0f, "%.3f");
            DrawStatus(
                usr->imguiBall.skid,
                BallPhysicsMapping::CATALOG_SKID_MIN,
                BallPhysicsMapping::CATALOG_SKID_MAX
            );
            if (changed)
                ApplyLive();
        }
        // Bite
        {
            bool changed = ImGui::SliderFloat("Bite", &usr->imguiBall.bite, 0.0f, 1.0f, "%.3f");
            DrawStatus(
                usr->imguiBall.bite,
                BallPhysicsMapping::CATALOG_BITE_MIN,
                BallPhysicsMapping::CATALOG_BITE_MAX
            );
            if (changed)
                ApplyLive();
        }
        // LaunchBuff
        {
            bool changed =
                ImGui::SliderFloat("LaunchBuff", &usr->imguiBall.launchBuff, 0.0f, 1.0f, "%.3f");
            DrawStatus(
                usr->imguiBall.launchBuff,
                BallPhysicsMapping::CATALOG_BUFF_MIN,
                BallPhysicsMapping::CATALOG_BUFF_MAX
            );
            if (changed)
                ApplyLive();
        }
        // HitBuff
        {
            bool changed =
                ImGui::SliderFloat("HitBuff", &usr->imguiBall.hitBuff, 0.0f, 1.0f, "%.3f");
            DrawStatus(
                usr->imguiBall.hitBuff,
                BallPhysicsMapping::CATALOG_BUFF_MIN,
                BallPhysicsMapping::CATALOG_BUFF_MAX
            );
            if (changed)
                ApplyLive();
        }

	        ImGui::Spacing();
	        if (ImGui::Button("↺ Reset to Original", ImVec2(-1, 0)))
	        {
	            usr->imguiBall = g_ballCatalog[usr->myBall.id];
	            ApplyLive();
	        }

	        ImGui::Spacing();
		        ImGui::Text("Lane Tuning (Live)");
		        ImGui::Separator();
		        {
		            ImGui::SliderFloat("Lane Friction", &usr->laneFriction, 0.0f, 0.20f, "%.3f");
		            ImGui::SliderFloat("Oil Thickness", &usr->laneOilThickness, 0.0f, 1.0f, "%.2f");
		            ImGui::SliderFloat("Oil Thickness Decay / m", &usr->oilThicknessDecayPerBallTravel, 0.0f, 0.05f, "%.4f");
		            ImGui::SliderFloat("Carrydown / m", &usr->oilCarrydownPerBallTravelM, 0.0f, 0.15f, "%.3f");
		            ImGui::SliderFloat("Pushback Strength", &usr->lanePushbackStrength, 0.0f, 50.0f, "%.1f");

		            float oilEnds[2] = { usr->leftOilFadeEndM, usr->rightOilFadeEndM };
		            float oilStarts[2] = { usr->leftOilFadeStartM, usr->rightOilFadeStartM };
		            if (ImGui::SliderFloat2("Oil Fade End (LE/RE m)", oilEnds, 0.0f, 18.3f, "%.2f"))
		            {
		                usr->leftOilFadeEndM = oilEnds[0];
		                usr->rightOilFadeEndM = oilEnds[1];
		            }
		            if (ImGui::SliderFloat2("Oil Fade Start (LS/RS m)", oilStarts, 0.0f, 18.3f, "%.2f"))
		            {
		                usr->leftOilFadeStartM = oilStarts[0];
		                usr->rightOilFadeStartM = oilStarts[1];
		            }
		            // Keep per-side ordering sane.
		            if (usr->leftOilFadeStartM > usr->leftOilFadeEndM)
		                std::swap(usr->leftOilFadeStartM, usr->leftOilFadeEndM);
		            if (usr->rightOilFadeStartM > usr->rightOilFadeEndM)
		                std::swap(usr->rightOilFadeStartM, usr->rightOilFadeEndM);

			
		            // Debug readout at x=0 (both sides equal weight).
		            float midStart = 0.5f * (usr->leftOilFadeStartM + usr->rightOilFadeStartM);
		            float midEnd = 0.5f * (usr->leftOilFadeEndM + usr->rightOilFadeEndM);
		            float zFadeStart = BallFrictionTuning::LANE_Z_START + midStart;
		            float zFadeEnd = BallFrictionTuning::LANE_Z_START + midEnd;
		            ImGui::Text("skidFade z @x=0: %.2f .. %.2f", zFadeStart, zFadeEnd);
		            float oilT = glm::clamp(usr->laneOilThickness, 0.0f, 1.0f);
		            float startScale = glm::mix(1.0f, usr->ballSkidStartScale, oilT);
		            ImGui::Text("oil startScale: %.2f", startScale);
		            ImGui::Text("wear L/R/Total (m): %.2f / %.2f / %.2f", usr->oilWearLeftM, usr->oilWearRightM, usr->oilWearTotalM);
		        }

	        ImGui::Spacing();
		        ImGui::Text("Scene Tuning (Live)");
		        ImGui::Separator();
		        {
		            bool changed = false;
		            ImGui::Checkbox("Debug Forgiveness", &usr->debugForgiveness);
		            changed |= ImGui::SliderFloat("Pivot Y", &usr->scene.pivotY, 0.6f, 2.0f, "%.2f");
		            changed |= ImGui::SliderFloat("Pivot Z", &usr->scene.pivotZ, -22.0f, -16.0f, "%.2f");
	            changed |= ImGui::SliderFloat("Release Offset MaxFrac", &usr->scene.releaseOffsetFracMax, 0.0f, 0.50f, "%.2f");
	            changed |= ImGui::SliderFloat("Idle Ball Y", &usr->scene.idleBallY, 0.05f, 0.8f, "%.2f");
	            changed |= ImGui::SliderFloat("Idle Ball ZOffset", &usr->scene.idleBallOffsetZFromPivot, 0.5f, 4.0f, "%.2f");
	            ImGui::SliderFloat("Cam Eye Y", &usr->scene.camEyeY, 0.2f, 2.0f, "%.2f");
	            ImGui::SliderFloat("Cam Eye ZFromBall", &usr->scene.camEyeZFromBall, -6.0f, 1.0f, "%.2f");
	            ImGui::SliderFloat("Cam Target ZFromBall", &usr->scene.camTargetZFromBall, 0.0f, 8.0f, "%.2f");
	            if (changed && (usr->phase == UserContext::Phase::AIM || usr->phase == UserContext::Phase::SWING))
	            {
	                usr->pivotPoint.y = usr->scene.pivotY;
	                usr->pivotPoint.z = usr->scene.pivotZ;
	                usr->phy.change_pivot_point(usr->pivotPoint);
	            }
	            ImGui::Text("releaseOffsetZ (derived): %.2f", usr->scene.releaseOffsetZ);
	            ImGui::Text("releasePlaneZ: %.2f", usr->pivotPoint.z + usr->scene.releaseOffsetZ);
	            ImGui::Text("idleBallZ: %.2f", usr->scene.pivotZ + usr->scene.idleBallOffsetZFromPivot);
	            if (ImGui::Button("Reset Scene Defaults", ImVec2(-1, 0)))
	            {
	                usr->scene = SceneTunables_Default();
	            }
	        }

	        ImGui::Spacing();
	        ImGui::Text("Derived Physics Values (Live)");
	        ImGui::Separator();

	        if (ImGui::BeginTable("##physics_vals", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
	        {
		            auto PreviewFrictionAtXZ = [&](float x, float z) -> float
		            {
		                float leftStartM = usr->leftOilFadeStartM;
		                float leftEndM = usr->leftOilFadeEndM;
		                float rightStartM = usr->rightOilFadeStartM;
		                float rightEndM = usr->rightOilFadeEndM;
		                if (leftStartM > leftEndM)
		                    std::swap(leftStartM, leftEndM);
		                if (rightStartM > rightEndM)
		                    std::swap(rightStartM, rightEndM);

		                float oilBlendX = BallFrictionTuning::LANE_HALF_WIDTH_M - BallFrictionTuning::OIL_BLEND_GUTTER_MARGIN_M;
		                oilBlendX = glm::max(0.01f, oilBlendX);
		                float oilSideT = glm::clamp(((-x) + oilBlendX) / (2.0f * oilBlendX), 0.0f, 1.0f);
		                float oilStartM = glm::mix(leftStartM, rightStartM, oilSideT);
		                float oilEndM = glm::mix(leftEndM, rightEndM, oilSideT);

		                float zFadeStart = BallFrictionTuning::LANE_Z_START + oilStartM;
		                float zFadeEnd = BallFrictionTuning::LANE_Z_START + oilEndM;
		                float denom = (zFadeEnd - zFadeStart);
		                float fadeT = (denom > 1e-6f) ? ((z - zFadeStart) / denom) : 1.0f;
		                if (!std::isfinite(fadeT))
		                    fadeT = 1.0f;
		                fadeT = glm::clamp(fadeT, 0.0f, 1.0f);
		                float ramp = smoothstep(0.0f, 1.0f, fadeT);
		                ramp = powf(ramp, BallFrictionTuning::SKID_FADE_EASE_EXP);
		                float oilT = glm::clamp(usr->laneOilThickness, 0.0f, 1.0f);
		                float startScale = glm::mix(1.0f, usr->ballSkidStartScale, oilT);
	                float mult = glm::mix(startScale, 1.0f, ramp);
	                float edgeT = smoothstep(
	                    BallFrictionTuning::SKID_EDGE_X_START,
	                    BallFrictionTuning::SKID_EDGE_X_END,
	                    glm::abs(x)
	                );
	                float edgeMult = glm::mix(1.0f, BallFrictionTuning::SKID_EDGE_MULT_AT_EDGE, edgeT);
	                float skidZoneStrength = (1.0f - ramp);
	                mult *= glm::mix(1.0f, edgeMult, skidZoneStrength);
	                return glm::clamp(
	                    usr->ballBaseFriction * mult, 0.0f, BallFrictionTuning::BALL_FRICTION_MAX
	                );
	            };

            ImGui::TableSetupColumn("Parameter", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("desiredMass");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.2f", usr->desiredMass);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("angularFactor");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.3f", usr->angularFactor);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("smashingPower");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.1f", usr->smashingPower);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("speedBoostAtThrow");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.2f", usr->speedBoostAtThrow);

	            ImGui::TableNextRow();
	            ImGui::TableSetColumnIndex(0);
	            ImGui::Text("ballBaseFriction");
	            ImGui::TableSetColumnIndex(1);
	            ImGui::Text("%.3f", usr->ballBaseFriction);

	            ImGui::TableNextRow();
	            ImGui::TableSetColumnIndex(0);
	            ImGui::Text("ballSkid");
	            ImGui::TableSetColumnIndex(1);
	            ImGui::Text("%.2f", usr->ballSkid);

	            ImGui::TableNextRow();
	            ImGui::TableSetColumnIndex(0);
	            ImGui::Text("ballSkidStartScale");
	            ImGui::TableSetColumnIndex(1);
	            ImGui::Text("%.2f", usr->ballSkidStartScale);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("angularStrength");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text(
                "%.3f (angFac×smash×0.02)", usr->angularFactor * usr->smashingPower * 0.02f
            );

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("spinStrength");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text(
                "%.3f (angFac×smash×0.008)", usr->angularFactor * usr->smashingPower * 0.008f
            );

	            ImGui::TableNextRow();
	            ImGui::TableSetColumnIndex(0);
	            ImGui::Text("currentFriction @ start(z=-18.3)");
	            ImGui::TableSetColumnIndex(1);
	            ImGui::Text("%.3f", PreviewFrictionAtXZ(0.0f, BallFrictionTuning::LANE_Z_START));

	            ImGui::TableNextRow();
	            ImGui::TableSetColumnIndex(0);
	            ImGui::Text("currentFriction @ start edge");
	            ImGui::TableSetColumnIndex(1);
	            ImGui::Text(
	                "%.3f",
	                PreviewFrictionAtXZ(
	                    BallFrictionTuning::LANE_HALF_WIDTH_M * 0.9f, BallFrictionTuning::LANE_Z_START
	                )
	            );

	            ImGui::TableNextRow();
	            ImGui::TableSetColumnIndex(0);
		            ImGui::Text("currentFriction @ skidFadeStart");
		            ImGui::TableSetColumnIndex(1);
		            ImGui::Text(
		                "%.3f",
		                PreviewFrictionAtXZ(
		                    0.0f,
		                    BallFrictionTuning::LANE_Z_START + 0.5f * (usr->leftOilFadeStartM + usr->rightOilFadeStartM)
		                )
		            );

	            ImGui::TableNextRow();
	            ImGui::TableSetColumnIndex(0);
	            ImGui::Text("currentFriction @ end(z=-5)");
	            ImGui::TableSetColumnIndex(1);
	            ImGui::Text("%.3f", PreviewFrictionAtXZ(0.0f, BallFrictionTuning::LANE_Z_END));

	            ImGui::EndTable();
	        }

		        if (ImGui::CollapsingHeader("📐 Formula Reference"))
		        {
		            ImGui::BulletText("angularStrength = angularFactor × smashingPower × 0.02f");
		            ImGui::BulletText("spinStrength    = angularFactor × smashingPower × 0.008f");
		            ImGui::BulletText("oilBlendX = LANE_HALF_WIDTH_M - OIL_BLEND_GUTTER_MARGIN_M");
		            ImGui::BulletText("oilSideT = clamp(((-x) + oilBlendX)/(2*oilBlendX), 0..1)  // 0=left, 1=right");
		            ImGui::BulletText("oilStartM = lerp(leftOilFadeStartM..rightOilFadeStartM, oilSideT)");
		            ImGui::BulletText("oilEndM   = lerp(leftOilFadeEndM..rightOilFadeEndM, oilSideT)");
		            ImGui::BulletText("zFadeStart = LANE_Z_START + oilStartM");
		            ImGui::BulletText("zFadeEnd   = LANE_Z_START + oilEndM");
		            ImGui::BulletText("fadeT = clamp((z - zFadeStart)/max(zFadeEnd-zFadeStart, eps), 0..1)");
		            ImGui::BulletText("skidRamp = pow(smoothstep(0..1, fadeT), skidFadeEaseExp)");
		            ImGui::BulletText("oilT = clamp(oilThickness, 0..1)");
		            ImGui::BulletText("startScale = lerp(1..ballSkidStartScale, oilT)");
		            ImGui::BulletText("skidMult = lerp(startScale..1, skidRamp)");
		            ImGui::BulletText("edgeT = smoothstep(SKID_EDGE_X_START..SKID_EDGE_X_END, abs(x))");
		            ImGui::BulletText("edgeMult = lerp(1..SKID_EDGE_MULT_AT_EDGE, edgeT)");
		            ImGui::BulletText("skidMult *= lerp(1..edgeMult, (1 - skidRamp))");
		            ImGui::BulletText("currentFriction = clamp(ballBaseFriction × skidMult, 0..BALL_FRICTION_MAX)");
		            ImGui::BulletText("THROW bite drive: if vx disagrees with input spin, sideDrive += sign(sideDrive)×(bite^2)×BITE_DRIVE_FROM_LATERAL_VEL×abs(vx)");
		        }

        ImGui::End();
    }
    usr->imgui.endImgui();
}

usr->fpsCounter.endFrame();

SDL_GL_SwapWindow(ctx->sdlWindow);
}
