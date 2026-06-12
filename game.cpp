#include <chrono>
#include <cmath>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <cctype>
#include <ctime>
#include <stdint.h>
#include <stdio.h>
#include <string.h> // for memcpy, strcmp
#include <thread>
#include <utility>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

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
#include "clayton/slider.h"
#include "coins.h"
#include "decal.h"
#include "electroball.h"
#include "fpscounter.h"
#include "hiscore/hiscore_clay.h"
#include "hiscore/localhi.h"
#include "hooker.h"
#include "joystick.h"
#include "dialogbox.h"
#include "mesh.h"
#include "animation/anim_player.h"
#include "mod_imgui.h"
#include "oil/oilmap.h"
#include "particles.h"
#include "houses/houses.h"
#include "bots/bots.h"
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
#include "bowling/pin_delta.h"
#include "school/school.h"
#include "school/school_clay.h"
#include "settings/settings.h"
#include "tracker/tracker.h"
#include "tracker/tracker_clay.h"
#include "tracker/tracker_diagrams.h"
#include "tracker/tracker_oscilloscope.h"

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

struct UserContext;
#ifdef __EMSCRIPTEN__
// Only for emscripten as it breaks hot reload otherwise
static UserContext *g_trackerIoUserContext = nullptr;
#endif
enum class BotAvatar
{
    ANGEL = 0,
    CHERUB = 1,
    SERAPH = 2,
    THRONE = 3,
};

enum class CampaignBiome
{
    NORMAL = 0,
    DESERT = 1,
    ICE = 2,
    NEON = 3,
};

enum class CampaignOpponent
{
    NONE = 0,
    MALACH = 1,
    DOG = 2,
    BEAK = 3,
    COW = 4,
};

enum class CampaignMode
{
    SOLO = 0,
    BOT = 1,
};

enum class PlayerRoute
{
    CAMPAIGN = 0,
    PRACTICE = 1,
    FREESTYLE = 2,
};

enum class SelectorFlowStep
{
    NONE = 0,
    BOT = 1,
    HOUSE = 2,
    BALL = 3,
};

enum class CampaignWinType
{
    SCORE_AT_LEAST = 0,
    BEAT_OPPONENT = 1,
};

struct CampaignLevelConfig
{
    int levelNumber;
    const char *title;
    const char *subtitle;
    CampaignBiome biome;
    CampaignOpponent opponent;
    CampaignMode mode;
    CampaignWinType winType;
    int targetScore;
    float enemySkill;
    int enemyBallId;
    int startStoryId;
    int endStoryId;
    CoinPattern pattern;
    int collectableCount;
    int rewardBank;
    const char *rewardText;
    const char *unlockText;
    int unlockBallId;
    int unlockHouseId;
    CampaignOpponent unlockOpponent;
};

static constexpr CampaignLevelConfig kCampaignLevels[] = {
    {1, "LEVEL 1  FIRST MILESTONE", "Normal biome  Reach 100 to pass", CampaignBiome::NORMAL, CampaignOpponent::NONE, CampaignMode::SOLO, CampaignWinType::SCORE_AT_LEAST, 100, 0.0f, 0, 1, 0, CoinPattern::Static, 7, 20, "20 bank", "Unlock Classic House, Ember Strike, Malach", 0, 0, CampaignOpponent::MALACH},
    {2, "LEVEL 2  MALACH ARRIVES", "Normal biome  Beat Malach", CampaignBiome::NORMAL, CampaignOpponent::MALACH, CampaignMode::BOT, CampaignWinType::BEAT_OPPONENT, 0, 0.86f, 2, 3002, 3102, CoinPattern::SideToSide, 7, 25, "25 bank", "Unlock Dry Fronts and Blaze Hook", 2, 1, CampaignOpponent::NONE},
    {3, "LEVEL 3  DESERT WARNING", "Desert biome  Beat Malach", CampaignBiome::DESERT, CampaignOpponent::MALACH, CampaignMode::BOT, CampaignWinType::BEAT_OPPONENT, 0, 0.88f, 3, 3003, 3103, CoinPattern::SideSweep, 8, 30, "30 bank", "Unlock Long Oil and Glacier Bite", 8, 2, CampaignOpponent::NONE},
    {4, "LEVEL 4  GLASS ICE", "Ice biome  Beat Malach", CampaignBiome::ICE, CampaignOpponent::MALACH, CampaignMode::BOT, CampaignWinType::BEAT_OPPONENT, 0, 0.90f, 8, 3004, 3104, CoinPattern::WaveOrbit, 8, 35, "35 bank", "Unlock Dog and Neon Strike", 26, -1, CampaignOpponent::DOG},
    {5, "LEVEL 5  DOG'S CHALLENGE", "Normal biome  Beat Dog", CampaignBiome::NORMAL, CampaignOpponent::DOG, CampaignMode::BOT, CampaignWinType::BEAT_OPPONENT, 0, 0.92f, 12, 3005, 3105, CoinPattern::TwinOrbit, 8, 40, "40 bank", "Unlock Asym Split and Void Strike", 13, 3, CampaignOpponent::NONE},
    {6, "LEVEL 6  POWER SHOT ALLEY", "Neon biome  Beat Dog", CampaignBiome::NEON, CampaignOpponent::DOG, CampaignMode::BOT, CampaignWinType::BEAT_OPPONENT, 0, 0.94f, 26, 3006, 3106, CoinPattern::RibbonOrbit, 9, 45, "45 bank", "Unlock Quantum Hook", 27, -1, CampaignOpponent::NONE},
    {7, "LEVEL 7  SAND TALK", "Desert biome  Beat Dog", CampaignBiome::DESERT, CampaignOpponent::DOG, CampaignMode::BOT, CampaignWinType::BEAT_OPPONENT, 0, 0.95f, 23, 3007, 3107, CoinPattern::TripleOrbit, 9, 50, "50 bank", "Unlock Beak and Rune Ball", 33, -1, CampaignOpponent::BEAK},
    {8, "LEVEL 8  BEAK IN THE DUNES", "Desert biome  Beat Beak", CampaignBiome::DESERT, CampaignOpponent::BEAK, CampaignMode::BOT, CampaignWinType::BEAT_OPPONENT, 0, 0.96f, 33, 3008, 3108, CoinPattern::StaticDrift, 8, 55, "55 bank", "Unlock Oracle Strike", 34, -1, CampaignOpponent::NONE},
    {9, "LEVEL 9  ICE AUDIENCE", "Ice biome  Beat Beak", CampaignBiome::ICE, CampaignOpponent::BEAK, CampaignMode::BOT, CampaignWinType::BEAT_OPPONENT, 0, 0.97f, 34, 3009, 3109, CoinPattern::WaveOrbit, 9, 60, "60 bank", "Unlock Black Hole", 14, -1, CampaignOpponent::NONE},
    {10, "LEVEL 10  NEON CONFESSION", "Neon biome  Beat Beak", CampaignBiome::NEON, CampaignOpponent::BEAK, CampaignMode::BOT, CampaignWinType::BEAT_OPPONENT, 0, 0.98f, 28, 3010, 3110, CoinPattern::RibbonOrbit, 9, 65, "65 bank", "Unlock Cow and Nullifier", 24, -1, CampaignOpponent::COW},
    {11, "LEVEL 11  WHEELS OF THE CITY", "Neon biome  Beat Cow", CampaignBiome::NEON, CampaignOpponent::COW, CampaignMode::BOT, CampaignWinType::BEAT_OPPONENT, 0, 0.99f, 24, 3011, 3111, CoinPattern::TripleOrbit, 10, 80, "80 bank", "Unlock Singularity", 28, -1, CampaignOpponent::NONE},
};

static constexpr int kCampaignLevelCount = (int)(sizeof(kCampaignLevels) / sizeof(kCampaignLevels[0]));

// ─────────────────────────────────────────────────────────────────────────────
// Angel (animated mesh) — loaded via assman pipeline (mesh + anim blob)
// NOTE: Kept out of UserContext to preserve hot-reload memory layout.
// ─────────────────────────────────────────────────────────────────────────────
static AssetMesh gAngelMesh;
static bool gAngelMeshReady = false;
static AssmanAnimPlayer gAngelAnim;
static bool gAngelAnimReady = false;

static AssetMesh gCherubMesh;
static bool gCherubMeshReady = false;
static AssmanAnimPlayer gCherubAnim;
static bool gCherubAnimReady = false;

static AssetMesh gSeraphMesh;
static bool gSeraphMeshReady = false;
static AssmanAnimPlayer gSeraphAnim;
static bool gSeraphAnimReady = false;

static AssetMesh gThroneMesh;
static bool gThroneMeshReady = false;
static AssmanAnimPlayer gThroneAnim;
static bool gThroneAnimReady = false;

static AssetMesh gGemMesh;
static bool gGemMeshReady = false;

static inline float Angel_ClipDurationSeconds(int clipIndex)
{
    if (!gAngelAnimReady)
        return 0.0f;
    if (clipIndex < 0 || clipIndex >= (int)gAngelAnim.clipPtrs.size())
        return 0.0f;
    const auto *ch = reinterpret_cast<const AssmanAnimClipHeader *>(gAngelAnim.clipPtrs[clipIndex]);
    return ch ? (float)ch->durationSeconds : 0.0f;
}

static inline AssmanAnimPlayer *Bot_Anim(UserContext *usr);
static inline bool Bot_AnimReady(const UserContext *usr);
static inline float Bot_ClipDurationSeconds(const UserContext *usr, int clipIndex);
static inline void Bot_InitIfNeeded(UserContext *usr);
static inline int Bot_ClipThrow(const UserContext *usr);
static inline int Bot_ClipArgument(const UserContext *usr);
static inline void Bot_PlayArgumentIfPossible(UserContext *usr, bool resetTime);
static inline void Bot_PlayThrowIfPossible(UserContext *usr, bool resetTime);

static inline int Anim_FindRightHandBoneIndex(const AssmanAnimPlayer &anim);
static inline int Anim_FindRightHandTipBoneIndex(const AssmanAnimPlayer &anim, int rightHandBone);
static inline glm::mat4 Angel_ComputeModelMatrix(const UserContext *usr);
static inline glm::mat4 Cherub_ComputeModelMatrix(const UserContext *usr);
static inline glm::mat4 Seraph_ComputeModelMatrix(const UserContext *usr);
static inline glm::mat4 Throne_ComputeModelMatrix(const UserContext *usr);
static void Angel_InitIfNeeded(UserContext *usr);
static void Cherub_InitIfNeeded(UserContext *usr);
static void Seraph_InitIfNeeded(UserContext *usr);
static void Throne_InitIfNeeded(UserContext *usr);
static void Gem_InitIfNeeded(UserContext *usr);
static inline void Angel_PlayArgumentIfPossible(UserContext *usr, bool resetTime);
static inline void Angel_PlayThrowIfPossible(UserContext *usr, bool resetTime);
static inline void Angel_Tick(UserContext *usr, float dt);
static inline void PhysicsResetForMode(UserContext *usr, bool reviveAll);
void BallStats_OnBallChange(const CatalogItem *ball, UserContext *usr);

// ─────────────────────────────────────────────────────────────────────────────
// Enemy turn (vs mode)
// ─────────────────────────────────────────────────────────────────────────────
static inline bool IsEnemyTurn(const UserContext *usr);
static inline void Enemy_ComputePins(UserContext *usr, const glm::vec3 initialPins[10]);
static inline glm::vec3 Enemy_IdleBallPos(const UserContext *usr);

// Implemented after UserContext is defined (needs member access).
static inline void Enemy_EnterTurn(UserContext *usr, const glm::vec3 initialPins[10]);
static inline void Player_EnterTurn(UserContext *usr);
static inline bool Enemy_TickAutoThrow(UserContext *usr, float dt);

struct SceneTunables
{
    //float pivotY = 1.30f;
    // float pivotZ = -18.90f;
    float pivotY = 1.11f;
    float pivotZ = -18.30f;
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

static inline void Scene_ComputeCameraEyeTarget(
    const SceneTunables &s, const glm::vec3 &ballPos, glm::vec3 &outEye, glm::vec3 &outTarget
)
{
    float eyeZ = glm::clamp(
        ballPos.z + s.camEyeZFromBall,
        s.camEyeZMin,
        s.camEyeZMax
    );
    float targetZ = glm::clamp(
        ballPos.z + s.camTargetZFromBall,
        s.camTargetZMin,
        s.camTargetZMax
    );
    outEye = glm::vec3(0.0f, s.camEyeY, eyeZ);
    outTarget = glm::vec3(0.0f, s.camTargetY, targetZ);
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
    enum class TurnOwner
    {
        PLAYER = 0,
        ENEMY = 1,
    };
    enum class GameMode
    {
        SOLO,
        SCHOOL,
        TRACKER,
        BOT
    };
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
    GameMode gameMode = GameMode::SOLO;

    // Progression: once school is done (or skipped), start new games in BOT mode.
    bool schoolDone = false;
    int campaignLevelIndex = 1; // 1-based into kCampaignLevels
    int campaignStartStoryLevelShown = 0;
    int pendingCampaignEndStoryId = 0;
    bool pendingCampaignBotResultWindow = false;
    int pendingCampaignBotPlayerScore = 0;
    int pendingCampaignBotEnemyScore = 0;
    bool pendingCampaignBotPlayerWon = false;
    PlayerRoute playerRoute = PlayerRoute::CAMPAIGN;
    TxlLanguage language = TXL_LANG_EN_US;
    SelectorFlowStep selectorFlowStep = SelectorFlowStep::NONE;
    BotAvatar selectedFreestyleAvatar = BotAvatar::ANGEL;
    int selectedHouseId = 0;
    int selectedBallId = 0;
    uint64_t unlockedBallMask = 0;
    uint32_t unlockedHouseMask = 0;
    uint32_t unlockedBotMask = 0;
    bool pendingFreestyleResultWindow = false;
    // Milestone gate: must score >=100 in SOLO before BOT mode can begin.
    bool milestone100Reached = false;
    bool milestone100StoryShown = false;
    // Story: show intro dialog once at the start of a new profile/session.
    bool introStoryShown = false;
    // Greetings modal is shown on first launch and browser resume to focus the canvas.
    bool greetingsSeen = false;
    bool greetingsWindowRequested = false;
    bool greetingsResumeMessageRequested = false;
    int saveGreetingMuteFrames = 0;
    int loadGreetingMuteFrames = 0;
    bool appFocusLost = false;
    bool appInactiveOverlayActive = false;
    bool trackerSongFilePickerActive = false;
    int trackerSongFilePickerFocusGraceFrames = 0;
    // Tracks whether the first SOLO game was actually completed (10 frames).
    bool firstSoloCompleted = false;
    // If true, the player cannot exit school until graduating (used when school is mandatory).
    bool schoolExitLocked = false;
    // When ending a SOLO run, we may enqueue a one-time mode switch (to SCHOOL or BOT)
    // that happens when the player hits "Play Again".
    bool pendingModeChange = false;
    GameMode pendingMode = GameMode::SOLO;
    GameMode trackerReturnMode = GameMode::SOLO;

    // Vs enemy (computer) turn state. Keep all state on UserContext (no file-scope globals).
    TurnOwner turnOwner = TurnOwner::PLAYER;
    BowlingScoreboard enemyBoard;
    bool enemyBoardInit = false;

    glm::vec3 enemyPins[10];
    bool enemyPinsInit = false;

    // Lane Z bounds used for mirroring pins/ball when enemy owns the turn.
    float enemyLaneMinZ = -18.3f;
    float enemyLaneMaxZ = 0.87f;

    glm::vec3 lastPlayerReleaseMovement = glm::vec3(0.0f);
    float lastPlayerReleaseSpinSpeed = 0.0f; // around +Y (Physics::apply_angular_velocity_on_ball)
    bool haveLastPlayerRelease = false;

    float enemyAutoTimer = 0.0f;
    bool enemyLaunched = false;
    bool enemyDebugLogged = false;
    // Tracks whether the current enemy turn has been set up via Enemy_EnterTurn.
    bool enemyTurnSetup = false;
    float enemyRetargetStrength = 0.85f;

    // Angel animation clip indices (loaded from assman anim blob).
    bool angelClipsInit = false;
    int angelClipThrow = -1;    // "BowlingThrow"
    int angelClipArgument = -1; // "BowlingArgument"
    int angelRightHandBone = -1;
    int angelRightHandTipBone = -1;
    bool angelRightHandWarned = false;
    // When Angel turn starts, we play BowlingThrow and launch the ball at this fraction of the clip.
    // (Configurable; default 3/4 as requested.)
    float angelThrowLaunchFrac = 0.75f;
    // Render-only: when the throw clip starts, keep the ball attached to the right hand until this
    // fraction, then blend toward the idle ball position (pre-launch) and finally to physics.
    float angelThrowLeaveHandFrac = 0.05f;
    // Render-only: after launch, how quickly (in seconds) the rendered ball should catch up to the
    // physics ball. Smaller = faster snap.
    float angelThrowCatchupSeconds = 0.20f;
    // Render-only: wait this long after launch before starting physics catchup.
    float angelThrowCatchupDelaySeconds = 0.50f;

    // Render-only enemy ball position smoothing during the Angel throw clip.
    glm::vec3 enemyBallRenderPos = glm::vec3(0.0f);
    bool enemyBallRenderPosValid = false;
    float enemyBallRenderSecondsSinceLaunch = 0.0f;

    BotAvatar botAvatar = BotAvatar::ANGEL;

    // Cherub animation indices (same clip names expected).
    bool cherubClipsInit = false;
    int cherubClipThrow = -1;
    int cherubClipArgument = -1;
    int cherubRightHandBone = -1;
    int cherubRightHandTipBone = -1;
    bool cherubRightHandWarned = false;

    // Seraph animation indices (same clip names expected).
    bool seraphClipsInit = false;
    int seraphClipThrow = -1;
    int seraphClipArgument = -1;
    int seraphRightHandBone = -1;
    int seraphRightHandTipBone = -1;
    bool seraphRightHandWarned = false;

    // Throne animation indices (same clip names expected).
    bool throneClipsInit = false;
    int throneClipThrow = -1;
    int throneClipArgument = -1;
    int throneRightHandBone = -1;
    int throneRightHandTipBone = -1;
    bool throneRightHandWarned = false;

    // Bot avatar render scales (Blender export units vary).
    float angelModelScale = 0.015f;
    // Cherub mesh is exported ~100x larger (vertex units) than Angel.
    // Keep it consistent with Angel's world size by default.
    float cherubModelScale = 0.000143f;
    // Seraph: tuned to match Angel size (slightly bigger by default).
    float seraphModelScale = 0.0145f;
    // Throne is authored ~100x larger than Angel units; start with a large scale.
    float throneModelScale = 0.0145f;
    // Mesh height in "asset units" (pre-world-scale), used to auto-fix scales on hot-reload.
    float angelMeshHeightUnits = 0.0f;
    float cherubMeshHeightUnits = 0.0f;
    float seraphMeshHeightUnits = 0.0f;
    float throneMeshHeightUnits = 0.0f;
    bool botScaleAutofixedOnce = false;
    glm::vec3 aimStart;
    glm::vec3 aimCurr;

    bool fuckCakez = true;
	Aurora aurora;
    ElectroBall electroBall;
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
    AssetMesh gemMesh;

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
    GameSettings settings;
    DialogBox dialog;
    bool firstGameStoryShown = false;
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
    Clayton_Click housesButton;
    Clayton_Click hiScoreButton;
    School school;
    Tracker tracker;
    Tracker trackerLoadScratch;
    std::string trackerChannelSoloPattern;
    std::string trackerSelectionPlaybackPattern;
    bool trackerSelectionPlaybackOverrideActive = false;

	// TUNABLET entries
	// Launch assist is modeled as an *impulse* applied at release:
	// heavier balls get less Δv for the same impulse (Δv = J / m).
	float armImpulseAtThrow = 10.0f;
	float angularFactor = 0.15f;
	float smashingPower = 10.0f;
	float desiredMass = 7.25f;
	float ballBaseFriction = 0.0f;
	float ballSkid = 0.0f;
	float ballSkidStartScale = 1.0f;
	float laneFriction = 0.05f;
	float laneRestitution = 0.01f;
	float lanePushbackStrength = 33.0f;
	// Asymmetric oil cover: per-side fade start/end in meters from lane start (LANE_Z_START).
	// We expose End first then Start in ImGui to match perspective view (pins are "forward").
	float leftOilFadeEndM = 13.3f;
	float rightOilFadeEndM = 13.3f;
	float leftOilFadeStartM = 8.3f;
	float rightOilFadeStartM = 8.3f;
	float laneOilThickness = 1.0f; // 0..1, scales how slippery the oil zone starts
    int laneTextureIdx = 0;
    int pinTextureIdx = 0;

    // Material tuning (Jolt)
    float ballRestitution = 0.02f;
    float pinRestitution = 0.3f;
    float pinFriction = 0.3f;
    float pinMass = 1.53f;
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
			33.0f,
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

    // Launch buff modifier derived from ball mass (lighter balls get more launch buff).
    float lightnessBuff = 1.0f;
    float launchBuffEffective = 0.0f; // myBall.launchBuff * lightnessBuff (clamped)

    CoinLane coinLane;

    float globalTime = 0.0f;
    int clearedCoins = 0; // Track coin pickups for SFX

    glm::vec2 placeOfMoney = glm::vec2(0.0f);
    glm::vec2 placeOfCharge = glm::vec2(0.0f);
    int hudAboveThis = 0;

    CarouselState carousel;
    HouseCarouselState housesCarousel;
    BotCarouselState botsCarousel;

	RenderTexture ballRenderTex;
	RenderTexture ballRenderTex2;
	RenderTexture oilRenderTex;
	RenderTexture trackerDiagramTex;
	RenderTexture trackerOscilloscopeTex;
    std::vector<uint32_t> trackerOscilloscopePixels;
    TrackerDiagramRenderer trackerDiagramRenderer;
	Particles particles;

    CatalogItem myBall;
    CatalogItem imguiBall;
    int sectors;

    // Spin params
    glm::vec2 prevDir = glm::vec2(1.0f, 0.0f);
    float totalAngle = 0.0f;
    float angularVelocity = 0.0f;
    float smoothedAngularVelocity = 0.0f;
    int circles = 0;

    // Celebration overlay (Strike/Spare)
    float strikeSpareFlashTime = 0.0f;
    int strikeSpareKind = 0; // 0=none, 1=strike, 2=spare
    float strikeSpareEarlyAllDownTime = 0.0f;
    bool strikeSpareEarlyDeclared = false;
    int strikeSpareEarlyKind = 0; // 0=none, 1=strike, 2=spare
    float strikeSpareEarlyDeclaredAt = 0.0f; // usr->globalTime when we first showed it early
    int strikeSpareSfxPlayedKind = 0; // 0=none, 1=strike, 2=spare (per throw)

    // Negative banners (gutter / stalled)
    float negativeBannerFlashTime = 0.0f;
    int negativeBannerKind = 0; // 0=none, 1=gutter, 2=stalled
    int negativeBannerSfxPlayedKind = 0; // 0=none, 1=gutter, 2=stalled (per throw)

	    // Neutral banner for a normal scoring roll (no strike/spare, not stalled/gutter)
	    float neutralBannerFlashTime = 0.0f;
	    int neutralBannerPins = 0;

	    // Ball<->lane impact tracking (hot reloadable, game.cpp-only)
	    int laneImpactHitCount = 0;
	    int laneImpactBounceIndex = 0; // 0=1.0, 1=0.5, 2=0.25, ...
	    bool laneImpactHadAirtime = true;
	    float laneImpactCooldownT = 0.0f;
	    bool laneImpactPrevValid = false;
	    glm::vec3 laneImpactPrevPos = glm::vec3(0.0f);
        xfm_voice_id rollingBallVoice = FM_VOICE_INVALID;

	    // Screen shake on ball<->lane impacts
	    float laneImpactShakeTime = 0.0f;
	    float laneImpactShakeDuration = 0.12f;
	    float laneImpactShakeAmp = 0.0f; // meters

        // Screen shake on pin hits (adds together across rapid successive impacts).
        float pinHitShakeTime = 0.0f;
        float pinHitShakeDuration = 0.18f;
        float pinHitShakeAmp = 0.0f; // meters

    // Camera smoothing when returning to IDLE after a throw.
    bool cameraReturnActive = false;
    float cameraReturnT = 0.0f;
    float cameraReturnDuration = 0.20f;
    glm::vec3 cameraEye = glm::vec3(0.0f);
    glm::vec3 cameraTarget = glm::vec3(0.0f);
    glm::vec3 cameraReturnStartEye = glm::vec3(0.0f);
    glm::vec3 cameraReturnStartTarget = glm::vec3(0.0f);
    glm::vec3 cameraReturnEndEye = glm::vec3(0.0f);
    glm::vec3 cameraReturnEndTarget = glm::vec3(0.0f);
};

// ─────────────────────────────────────────────────────────────────────────────
// Angel animation helpers (definitions; need full UserContext)
// ─────────────────────────────────────────────────────────────────────────────
static inline AssmanAnimPlayer *Bot_Anim(UserContext *usr)
{
    if (!usr)
        return nullptr;
    if (usr->botAvatar == BotAvatar::CHERUB)
        return &gCherubAnim;
    if (usr->botAvatar == BotAvatar::SERAPH)
        return &gSeraphAnim;
    if (usr->botAvatar == BotAvatar::THRONE)
        return &gThroneAnim;
    return &gAngelAnim;
}

static inline bool Bot_AnimReady(const UserContext *usr)
{
    if (!usr)
        return false;
    if (usr->botAvatar == BotAvatar::CHERUB)
        return gCherubAnimReady;
    if (usr->botAvatar == BotAvatar::SERAPH)
        return gSeraphAnimReady;
    if (usr->botAvatar == BotAvatar::THRONE)
        return gThroneAnimReady;
    return gAngelAnimReady;
}

static inline float Bot_ClipDurationSeconds(const UserContext *usr, int clipIndex)
{
    if (!usr)
        return 0.0f;
    if (usr->botAvatar == BotAvatar::CHERUB)
    {
        if (!gCherubAnimReady)
            return 0.0f;
        if (clipIndex < 0 || clipIndex >= (int)gCherubAnim.clipPtrs.size())
            return 0.0f;
        const auto *ch = reinterpret_cast<const AssmanAnimClipHeader *>(gCherubAnim.clipPtrs[clipIndex]);
        return ch ? (float)ch->durationSeconds : 0.0f;
    }
    if (usr->botAvatar == BotAvatar::SERAPH)
    {
        if (!gSeraphAnimReady)
            return 0.0f;
        if (clipIndex < 0 || clipIndex >= (int)gSeraphAnim.clipPtrs.size())
            return 0.0f;
        const auto *ch = reinterpret_cast<const AssmanAnimClipHeader *>(gSeraphAnim.clipPtrs[clipIndex]);
        return ch ? (float)ch->durationSeconds : 0.0f;
    }
    if (usr->botAvatar == BotAvatar::THRONE)
    {
        if (!gThroneAnimReady)
            return 0.0f;
        if (clipIndex < 0 || clipIndex >= (int)gThroneAnim.clipPtrs.size())
            return 0.0f;
        const auto *ch = reinterpret_cast<const AssmanAnimClipHeader *>(gThroneAnim.clipPtrs[clipIndex]);
        return ch ? (float)ch->durationSeconds : 0.0f;
    }
    return Angel_ClipDurationSeconds(clipIndex);
}

static inline void Bot_InitIfNeeded(UserContext *usr)
{
    if (!usr)
        return;
    // Keep all avatars loadable at runtime (F7 toggle), and ensure we can compute
    // a robust relative scale even on hot-reload.
    Angel_InitIfNeeded(usr);
    Cherub_InitIfNeeded(usr);
    Seraph_InitIfNeeded(usr);
    Throne_InitIfNeeded(usr);
    Gem_InitIfNeeded(usr);
}

static inline int Bot_ClipThrow(const UserContext *usr)
{
    if (!usr)
        return -1;
    if (usr->botAvatar == BotAvatar::CHERUB)
        return usr->cherubClipThrow;
    if (usr->botAvatar == BotAvatar::SERAPH)
        return usr->seraphClipThrow;
    if (usr->botAvatar == BotAvatar::THRONE)
        return usr->throneClipThrow;
    return usr->angelClipThrow;
}

static inline int Bot_ClipArgument(const UserContext *usr)
{
    if (!usr)
        return -1;
    if (usr->botAvatar == BotAvatar::CHERUB)
        return usr->cherubClipArgument;
    if (usr->botAvatar == BotAvatar::SERAPH)
        return usr->seraphClipArgument;
    if (usr->botAvatar == BotAvatar::THRONE)
        return usr->throneClipArgument;
    return usr->angelClipArgument;
}

static inline void Bot_PlayArgumentIfPossible(UserContext *usr, bool resetTime)
{
    if (!usr || !Bot_AnimReady(usr))
        return;
    AssmanAnimPlayer *anim = Bot_Anim(usr);
    int clip = Bot_ClipArgument(usr);
    if (!anim || clip < 0)
        return;
    anim->setClip(clip, /*resetTime=*/resetTime);
    anim->loop = true;
}

static inline void Bot_PlayThrowIfPossible(UserContext *usr, bool resetTime)
{
    if (!usr || !Bot_AnimReady(usr))
        return;
    AssmanAnimPlayer *anim = Bot_Anim(usr);
    int clip = Bot_ClipThrow(usr);
    if (!anim || clip < 0)
        return;
    anim->setClip(clip, /*resetTime=*/resetTime);
    anim->loop = false;
}

static inline int Anim_FindRightHandBoneIndex(const AssmanAnimPlayer &anim)
{
    if (!anim.anim.header)
        return -1;
    const uint32_t n = anim.anim.header->boneCount;

    // Fast path for the common Mixamo naming.
    for (uint32_t i = 0; i < n; ++i)
    {
        const char *nm = animNameCStr(anim.anim.bones[i].name);
        if (!nm)
            continue;
        std::string s(nm);
        for (char &c : s)
            c = (char)std::tolower((unsigned char)c);
        if (s == "mixamorig:righthand" || s == "righthand")
            return (int)i;
    }

    int best = -1;
    int bestScore = -1;
    for (uint32_t i = 0; i < n; ++i)
    {
        const char *nm = animNameCStr(anim.anim.bones[i].name);
        if (!nm)
            continue;

        std::string s(nm);
        for (char &c : s)
            c = (char)std::tolower((unsigned char)c);

        if (s.find("hand") == std::string::npos)
            continue;

        int score = 0;
        if (s.find("right") != std::string::npos)
            score += 3;
        if (s.find("hand_r") != std::string::npos || s.find("hand.r") != std::string::npos || s.find("r_hand") != std::string::npos)
            score += 4;
        if (s.size() >= 2 && s.rfind(".r") == s.size() - 2)
            score += 2;
        if (s.size() >= 2 && s.rfind("_r") == s.size() - 2)
            score += 2;

        if (score > bestScore || (score == bestScore && (int)i > best))
        {
            bestScore = score;
            best = (int)i;
        }
    }
    return best;
}

static inline int Anim_FindRightHandTipBoneIndex(const AssmanAnimPlayer &anim, int rightHandBone)
{
    if (!anim.anim.header)
        return -1;
    if (rightHandBone < 0 || rightHandBone >= (int)anim.anim.header->boneCount)
        return -1;

    const uint32_t n = anim.anim.header->boneCount;

    auto isDescendantOfHand = [&](uint32_t bone) -> bool {
        int32_t p = (int32_t)bone;
        while (p != ASSMAN_ANIM_NO_PARENT)
        {
            if (p == rightHandBone)
                return true;
            p = anim.anim.bones[p].parentIndex;
        }
        return false;
    };

    // Mark which bones are inside the hand subtree (including the hand itself).
    std::vector<uint8_t> inSubtree(n, 0);
    inSubtree[rightHandBone] = 1;
    for (uint32_t i = 0; i < n; ++i)
    {
        if ((int)i == rightHandBone)
            continue;
        if (isDescendantOfHand(i))
            inSubtree[i] = 1;
    }

    // Compute leaf nodes within the subtree.
    std::vector<uint8_t> hasChild(n, 0);
    for (uint32_t i = 0; i < n; ++i)
    {
        if (!inSubtree[i])
            continue;
        int32_t p = anim.anim.bones[i].parentIndex;
        if (p >= 0 && p < (int32_t)n && inSubtree[p])
            hasChild[p] = 1;
    }

    // Pick the leaf farthest from the hand in the current pose (t=0 pose after load).
    int bestLeaf = -1;
    float bestDist2 = -1.0f;
    if (rightHandBone >= 0 && rightHandBone < (int)anim.globalMatrices.size())
    {
        glm::vec3 handPos = glm::vec3(anim.globalMatrices[rightHandBone][3]);
        for (uint32_t i = 0; i < n; ++i)
        {
            if (!inSubtree[i])
                continue;
            if (hasChild[i])
                continue; // not a leaf
            if ((int)i >= (int)anim.globalMatrices.size())
                continue;
            glm::vec3 p = glm::vec3(anim.globalMatrices[i][3]);
            float d2 = glm::dot(p - handPos, p - handPos);
            if (d2 > bestDist2)
            {
                bestDist2 = d2;
                bestLeaf = (int)i;
            }
        }
    }

    return (bestLeaf >= 0) ? bestLeaf : rightHandBone;
}

static inline glm::mat4 Bot_ComputeModelMatrix_NoScale(const UserContext *usr)
{
    if (!usr)
        return glm::mat4(1.0f);
    const float behindPinsM = 2.0f;
    float zBack = usr->initialPins[9].z + behindPinsM;
    glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.5f, zBack));

    const glm::mat4 rotZUpToYUp = glm::rotate(glm::mat4(1.0f), glm::radians(+90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    const glm::mat4 rotFaceCamera = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    m = m * rotFaceCamera * rotZUpToYUp;
    return m;
}

static inline glm::mat4 Bot_ComputeFacingMatrix_NoTranslate()
{
    const glm::mat4 rotZUpToYUp = glm::rotate(glm::mat4(1.0f), glm::radians(+90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    const glm::mat4 rotFaceCamera = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    return rotFaceCamera * rotZUpToYUp;
}

static inline glm::mat4 BotPreview_ComputeModelMatrix(const UserContext *usr, BotAvatar avatar)
{
    if (!usr)
        return glm::mat4(1.0f);
    float s = usr->angelModelScale;
    if (avatar == BotAvatar::CHERUB)
        s = usr->cherubModelScale;
    else if (avatar == BotAvatar::SERAPH)
        s = usr->seraphModelScale;
    else if (avatar == BotAvatar::THRONE)
        s = usr->throneModelScale;
    return Bot_ComputeFacingMatrix_NoTranslate() * glm::scale(glm::mat4(1.0f), glm::vec3(s));
}

static inline glm::mat4 Bot_ComputeModelMatrix_TranslateOnly(const UserContext *usr)
{
    if (!usr)
        return glm::mat4(1.0f);
    const float behindPinsM = 2.0f;
    float zBack = usr->initialPins[9].z + behindPinsM;
    return glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.5f, zBack));
}

static inline glm::mat4 Angel_ComputeModelMatrix(const UserContext *usr)
{
    const float s = usr ? usr->angelModelScale : 0.017f;
    return Bot_ComputeModelMatrix_NoScale(usr) * glm::scale(glm::mat4(1.0f), glm::vec3(s));
}

static inline glm::mat4 Cherub_ComputeModelMatrix(const UserContext *usr)
{
    const float s = usr ? usr->cherubModelScale : 0.003f;
    return Bot_ComputeModelMatrix_NoScale(usr) * glm::scale(glm::mat4(1.0f), glm::vec3(s));
}

static inline glm::mat4 Seraph_ComputeModelMatrix(const UserContext *usr)
{
    const float s = usr ? usr->seraphModelScale : 0.019f;
    // Seraph needs the same facing/orientation as the other bot avatars.
    return Bot_ComputeModelMatrix_NoScale(usr) * glm::scale(glm::mat4(1.0f), glm::vec3(s));
}

static inline glm::mat4 Throne_ComputeModelMatrix(const UserContext *usr)
{
    const float s = usr ? usr->throneModelScale : 1.5f;
    return Bot_ComputeModelMatrix_NoScale(usr) * glm::scale(glm::mat4(1.0f), glm::vec3(s));
}

static inline bool Angel_ComputeRightHandAttachPosWorld(const UserContext *usr, glm::vec3 &outWorld)
{
    if (!usr)
        return false;

    AssmanAnimPlayer *anim = Bot_Anim(const_cast<UserContext *>(usr));
    if (!anim || !Bot_AnimReady(usr))
        return false;

    int bone = -1;
    if (usr->botAvatar == BotAvatar::CHERUB)
        bone = (usr->cherubRightHandTipBone >= 0) ? usr->cherubRightHandTipBone : usr->cherubRightHandBone;
    else if (usr->botAvatar == BotAvatar::SERAPH)
        bone = (usr->seraphRightHandTipBone >= 0) ? usr->seraphRightHandTipBone : usr->seraphRightHandBone;
    else if (usr->botAvatar == BotAvatar::THRONE)
        bone = (usr->throneRightHandTipBone >= 0) ? usr->throneRightHandTipBone : usr->throneRightHandBone;
    else
        bone = (usr->angelRightHandTipBone >= 0) ? usr->angelRightHandTipBone : usr->angelRightHandBone;
    if (bone < 0 || bone >= (int)anim->globalMatrices.size())
        return false;

    glm::mat4 model = Angel_ComputeModelMatrix(usr);
    if (usr->botAvatar == BotAvatar::CHERUB)
        model = Cherub_ComputeModelMatrix(usr);
    else if (usr->botAvatar == BotAvatar::SERAPH)
        model = Seraph_ComputeModelMatrix(usr);
    else if (usr->botAvatar == BotAvatar::THRONE)
        model = Throne_ComputeModelMatrix(usr);

    glm::vec3 bonePosModel = glm::vec3(anim->globalMatrices[bone][3]);
    glm::vec3 bonePosWorld = glm::vec3(model * glm::vec4(bonePosModel, 1.0f));

    // Direction of the bone: from parent -> bone. Extend by a fixed amount (20cm) toward fingertips.
    glm::vec3 dirWorld = glm::vec3(0.0f, 0.0f, 1.0f);
    int parent = (int)anim->anim.bones[bone].parentIndex;
    if (parent >= 0 && parent < (int)anim->globalMatrices.size())
    {
        glm::vec3 parentPosModel = glm::vec3(anim->globalMatrices[parent][3]);
        glm::vec3 parentPosWorld = glm::vec3(model * glm::vec4(parentPosModel, 1.0f));
        glm::vec3 d = bonePosWorld - parentPosWorld;
        float len2 = glm::dot(d, d);
        if (len2 > 1e-8f)
            dirWorld = d / std::sqrt(len2);
    }

    outWorld = bonePosWorld + dirWorld * 0.20f;
    return true;
}

static inline void Enemy_SeedRenderedBallPosFromHand(UserContext *usr)
{
    if (!usr)
        return;
    if (usr->gameMode != UserContext::GameMode::BOT || !IsEnemyTurn(usr))
        return;
    glm::vec3 p;
    if (Angel_ComputeRightHandAttachPosWorld(usr, p))
    {
        usr->enemyBallRenderPos = p;
        usr->enemyBallRenderPosValid = true;
    }
}

static inline void Angel_PlayArgumentIfPossible(UserContext *usr, bool resetTime)
{
    // Backward-compat wrapper.
    Bot_PlayArgumentIfPossible(usr, resetTime);
}

static inline void Angel_PlayThrowIfPossible(UserContext *usr, bool resetTime)
{
    // Backward-compat wrapper.
    Bot_PlayThrowIfPossible(usr, resetTime);
}

static inline void Angel_Tick(UserContext *usr, float dt)
{
    if (!usr)
        return;
    if (usr->gameMode != UserContext::GameMode::BOT)
        return;

    // Advance *all* bot animations so any bot we render (now or later) animates consistently,
    // not just the currently active gameplay avatar.
    if (gAngelAnimReady) gAngelAnim.tick(dt);
    if (gCherubAnimReady) gCherubAnim.tick(dt);
    if (gSeraphAnimReady) gSeraphAnim.tick(dt);
    if (gThroneAnimReady) gThroneAnim.tick(dt);

    // Still enforce the gameplay avatar state machine: if the active avatar is in a non-looping
    // throw clip and it finished, return it to looping "argumenting".
    if (!Bot_AnimReady(usr))
        return;
    AssmanAnimPlayer *anim = Bot_Anim(usr);
    if (!anim)
        return;
    const int throwClip = Bot_ClipThrow(usr);
    if (!anim->loop && anim->activeClip == throwClip)
    {
        float dur = Bot_ClipDurationSeconds(usr, throwClip);
        if (dur > 0.0f && anim->t >= dur)
            Bot_PlayArgumentIfPossible(usr, /*resetTime=*/true);
    }
}

static inline void Enemy_UpdateRenderedBallPosDuringThrow(UserContext *usr, float dt)
{
    if (!usr)
        return;
    if (usr->gameMode != UserContext::GameMode::BOT || !IsEnemyTurn(usr))
        return;
    if (!Bot_AnimReady(usr) || Bot_ClipThrow(usr) < 0)
        return;
    AssmanAnimPlayer *anim = Bot_Anim(usr);
    const int throwClip = Bot_ClipThrow(usr);
    if (!anim)
        return;
    if (anim->activeClip != throwClip || anim->loop)
        return;

    float dur = Bot_ClipDurationSeconds(usr, throwClip);
    if (dur <= 1e-3f)
        return;

    (void)dur;

    glm::vec3 handWorld = Enemy_IdleBallPos(usr);
    bool haveHand = Angel_ComputeRightHandAttachPosWorld(usr, handWorld);

    if (!usr->enemyBallRenderPosValid)
    {
        usr->enemyBallRenderPos = haveHand ? handWorld : Enemy_IdleBallPos(usr);
        usr->enemyBallRenderPosValid = true;
    }

    // Requirement: follow animation hand 100% until the physics launch happens.
    if (!usr->enemyLaunched)
    {
        usr->enemyBallRenderPos = haveHand ? handWorld : usr->enemyBallRenderPos;
        return;
    }

    usr->enemyBallRenderSecondsSinceLaunch += dt;

    // Post-launch: optional delay before starting physics catchup.
    if (usr->enemyBallRenderSecondsSinceLaunch < glm::max(0.0f, usr->angelThrowCatchupDelaySeconds))
    {
        // Keep following the hand briefly so the throw reads well.
        if (haveHand)
            usr->enemyBallRenderPos = handWorld;
        return;
    }

    // Post-launch: quickly chase the physics ball position (time-based).
    glm::vec3 physPos = glm::vec3(usr->phy.physics_get_ball_matrix()[3]);
    float tau = glm::max(0.01f, usr->angelThrowCatchupSeconds);
    // Exponential smoothing alpha, so we get very close within ~tau.
    float alpha = 1.0f - std::exp(-dt / tau);
    usr->enemyBallRenderPos = glm::mix(usr->enemyBallRenderPos, physPos, glm::clamp(alpha, 0.0f, 1.0f));
}

static inline float Mesh_ComputeHeightUnits(const MeshData &md)
{
    if (!md.vertices || md.vertexCount == 0)
        return 0.0f;
    float minY = md.vertices[0].position.y;
    float maxY = md.vertices[0].position.y;
    for (uint32_t i = 1; i < md.vertexCount; ++i)
    {
        float y = md.vertices[i].position.y;
        minY = (y < minY) ? y : minY;
        maxY = (y > maxY) ? y : maxY;
    }
    return maxY - minY;
}

static inline void Bot_MaybeAutofixAvatarScales(UserContext *usr)
{
    if (!usr)
        return;
    if (usr->botScaleAutofixedOnce)
        return;
    if (usr->angelMeshHeightUnits <= 0.0f || usr->cherubMeshHeightUnits <= 0.0f)
        return;

    const float idealCherubScale =
        usr->angelModelScale * (usr->angelMeshHeightUnits / usr->cherubMeshHeightUnits);

    const bool cherubScaleBad =
        !std::isfinite(usr->cherubModelScale) || usr->cherubModelScale <= 0.0f ||
        // If it's within the same order as Angel, it's almost certainly wrong for Cherub.
        usr->cherubModelScale > usr->angelModelScale * 0.10f ||
        // If it's wildly off the ideal computed ratio, also treat as bad.
        usr->cherubModelScale < idealCherubScale * 0.20f ||
        usr->cherubModelScale > idealCherubScale * 5.00f;

    if (cherubScaleBad && std::isfinite(idealCherubScale) && idealCherubScale > 0.0f)
    {
        usr->cherubModelScale = idealCherubScale;
        usr->botScaleAutofixedOnce = true;
        std::cerr << "[bot-avatar] autofix: angelHeight=" << usr->angelMeshHeightUnits
                  << " cherubHeight=" << usr->cherubMeshHeightUnits
                  << " => cherubModelScale=" << usr->cherubModelScale << "\n";
    }
}

static void Angel_InitIfNeeded(UserContext *usr)
{
    if (!usr)
        return;
    if (!gAngelMeshReady)
    {
        // Placeholder headers ship with len=0 until `make assets` regenerates them.
        if (angel_mesh_data_len < sizeof(MeshDataHeader) + sizeof(Vertex) + sizeof(uint32_t))
            return;

        MeshData angelMd = loadMeshFromBlob(angel_mesh_data, angel_mesh_data_len);
        usr->angelMeshHeightUnits = Mesh_ComputeHeightUnits(angelMd);
        gAngelMesh.sendMeshDataToGpu(&angelMd);
        gAngelMeshReady = true;
    }

    if (!gAngelAnimReady && angel_anim_data_len >= sizeof(AssmanAnimHeader))
    {
        try
        {
            gAngelAnim.loadFromBlob(angel_anim_data, angel_anim_data_len);
            // Default to an "idle" clip (argumenting) once loaded.
            int clip = gAngelAnim.findClipByName("BowlingArgument");
            if (clip < 0)
                clip = 0;
            gAngelAnim.setClip(clip, /*resetTime=*/true);
            gAngelAnim.loop = true;
            gAngelAnimReady = true;

            // Prime pose buffers for any bone queries (t=0 pose).
            (void)gAngelAnim.evaluate();
        }
        catch (...)
        {
            gAngelAnimReady = false;
        }
    }

    // Cache clip indices onto the UserContext (no file-scope gameplay state).
    if (gAngelAnimReady && !usr->angelClipsInit)
    {
        usr->angelClipThrow = gAngelAnim.findClipByName("BowlingThrow");
        usr->angelClipArgument = gAngelAnim.findClipByName("BowlingArgument");
        usr->angelRightHandBone = Anim_FindRightHandBoneIndex(gAngelAnim);
        usr->angelRightHandTipBone = Anim_FindRightHandTipBoneIndex(gAngelAnim, usr->angelRightHandBone);
        usr->angelClipsInit = true;
    }

    if (gAngelAnimReady && usr->angelClipsInit && usr->angelRightHandBone < 0 && !usr->angelRightHandWarned)
    {
        usr->angelRightHandWarned = true;
        std::cerr << "[angel] WARNING: could not find a right-hand bone (ball won't attach to hand during throw)\n";
    }
}

static void Cherub_InitIfNeeded(UserContext *usr)
{
    if (!usr)
        return;
    if (!gCherubMeshReady)
    {
        if (cherub_mesh_data_len < sizeof(MeshDataHeader) + sizeof(Vertex) + sizeof(uint32_t))
            return;
        MeshData md = loadMeshFromBlob(cherub_mesh_data, cherub_mesh_data_len);
        usr->cherubMeshHeightUnits = Mesh_ComputeHeightUnits(md);
        gCherubMesh.sendMeshDataToGpu(&md);
        gCherubMeshReady = true;
    }

    if (!gCherubAnimReady && cherub_anim_data_len >= sizeof(AssmanAnimHeader))
    {
        try
        {
            gCherubAnim.loadFromBlob(cherub_anim_data, cherub_anim_data_len);
            int clip = gCherubAnim.findClipByName("BowlingArgument");
            if (clip < 0)
                clip = 0;
            gCherubAnim.setClip(clip, /*resetTime=*/true);
            gCherubAnim.loop = true;
            gCherubAnimReady = true;
            (void)gCherubAnim.evaluate();
        }
        catch (...)
        {
            gCherubAnimReady = false;
        }
    }

    if (gCherubAnimReady && !usr->cherubClipsInit)
    {
        usr->cherubClipThrow = gCherubAnim.findClipByName("BowlingThrow");
        usr->cherubClipArgument = gCherubAnim.findClipByName("BowlingArgument");
        usr->cherubRightHandBone = Anim_FindRightHandBoneIndex(gCherubAnim);
        usr->cherubRightHandTipBone = Anim_FindRightHandTipBoneIndex(gCherubAnim, usr->cherubRightHandBone);
        usr->cherubClipsInit = true;
    }
    if (gCherubAnimReady && usr->cherubClipsInit && usr->cherubRightHandBone < 0 && !usr->cherubRightHandWarned)
    {
        usr->cherubRightHandWarned = true;
        std::cerr << "[cherub] WARNING: could not find a right-hand bone\n";
    }

    // Hot-reload safety: if the new cherub scale field is stale/uninitialized, fix it.
    Bot_MaybeAutofixAvatarScales(usr);
}

static void Seraph_InitIfNeeded(UserContext *usr)
{
    if (!usr)
        return;
    if (!gSeraphMeshReady)
    {
        if (seraph_mesh_data_len < sizeof(MeshDataHeader) + sizeof(Vertex) + sizeof(uint32_t))
            return;
        MeshData md = loadMeshFromBlob(seraph_mesh_data, seraph_mesh_data_len);
        usr->seraphMeshHeightUnits = Mesh_ComputeHeightUnits(md);
        gSeraphMesh.sendMeshDataToGpu(&md);
        gSeraphMeshReady = true;
    }

    if (!gSeraphAnimReady && seraph_anim_data_len >= sizeof(AssmanAnimHeader))
    {
        try
        {
            gSeraphAnim.loadFromBlob(seraph_anim_data, seraph_anim_data_len);
            int clip = gSeraphAnim.findClipByName("BowlingArgument");
            if (clip < 0)
                clip = 0;
            gSeraphAnim.setClip(clip, /*resetTime=*/true);
            gSeraphAnim.loop = true;
            gSeraphAnimReady = true;
            (void)gSeraphAnim.evaluate();
        }
        catch (...)
        {
            gSeraphAnimReady = false;
        }
    }

    if (gSeraphAnimReady && !usr->seraphClipsInit)
    {
        usr->seraphClipThrow = gSeraphAnim.findClipByName("BowlingThrow");
        usr->seraphClipArgument = gSeraphAnim.findClipByName("BowlingArgument");
        usr->seraphRightHandBone = Anim_FindRightHandBoneIndex(gSeraphAnim);
        usr->seraphRightHandTipBone = Anim_FindRightHandTipBoneIndex(gSeraphAnim, usr->seraphRightHandBone);
        usr->seraphClipsInit = true;
    }
    if (gSeraphAnimReady && usr->seraphClipsInit && usr->seraphRightHandBone < 0 && !usr->seraphRightHandWarned)
    {
        usr->seraphRightHandWarned = true;
        std::cerr << "[seraph] WARNING: could not find a right-hand bone\n";
    }
}

static void Throne_InitIfNeeded(UserContext *usr)
{
    if (!usr)
        return;
    if (!gThroneMeshReady)
    {
        if (throne_mesh_data_len < sizeof(MeshDataHeader) + sizeof(Vertex) + sizeof(uint32_t))
            return;
        MeshData md = loadMeshFromBlob(throne_mesh_data, throne_mesh_data_len);
        usr->throneMeshHeightUnits = Mesh_ComputeHeightUnits(md);
        gThroneMesh.sendMeshDataToGpu(&md);
        gThroneMeshReady = true;
    }

    if (!gThroneAnimReady && throne_anim_data_len >= sizeof(AssmanAnimHeader))
    {
        try
        {
            gThroneAnim.loadFromBlob(throne_anim_data, throne_anim_data_len);
            int clip = gThroneAnim.findClipByName("BowlingArgument");
            if (clip < 0)
                clip = 0;
            gThroneAnim.setClip(clip, /*resetTime=*/true);
            gThroneAnim.loop = true;
            gThroneAnimReady = true;
            (void)gThroneAnim.evaluate();
        }
        catch (...)
        {
            gThroneAnimReady = false;
        }
    }

    if (gThroneAnimReady && !usr->throneClipsInit)
    {
        usr->throneClipThrow = gThroneAnim.findClipByName("BowlingThrow");
        usr->throneClipArgument = gThroneAnim.findClipByName("BowlingArgument");
        usr->throneRightHandBone = Anim_FindRightHandBoneIndex(gThroneAnim);
        usr->throneRightHandTipBone = Anim_FindRightHandTipBoneIndex(gThroneAnim, usr->throneRightHandBone);
        usr->throneClipsInit = true;
    }
    if (gThroneAnimReady && usr->throneClipsInit && usr->throneRightHandBone < 0 && !usr->throneRightHandWarned)
    {
        usr->throneRightHandWarned = true;
        std::cerr << "[throne] WARNING: could not find a right-hand bone\n";
    }
}

static void Gem_InitIfNeeded(UserContext *usr)
{
    if (!usr)
        return;
    if (!gGemMeshReady)
    {
        if (gem_mesh_data_len < sizeof(MeshDataHeader) + sizeof(Vertex) + sizeof(uint32_t))
            return;
        MeshData md = loadMeshFromBlob(gem_mesh_data, gem_mesh_data_len);
        gGemMesh.sendMeshDataToGpu(&md);
        gGemMeshReady = true;
    }
}

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

static inline void BallRollingSfx_Stop(UserContext *usr);
static inline void BallRollingSfx_Start(UserContext *usr);

static inline void LogToIdle(UserContext *usr, const char *reason)
{
    BallRollingSfx_Stop(usr);
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

static inline void BallRollingSfx_Stop(UserContext *usr)
{
    if (!usr || usr->rollingBallVoice == FM_VOICE_INVALID)
        return;
    usr->sound.stopSfx(usr->rollingBallVoice);
    usr->rollingBallVoice = FM_VOICE_INVALID;
}

static inline void BallRollingSfx_Start(UserContext *usr)
{
    if (!usr || usr->rollingBallVoice != FM_VOICE_INVALID)
        return;
    usr->rollingBallVoice = usr->sound.playSfxBallRolling();
}

static inline void UI_ResetBannersForNewRoll(UserContext *usr, const char *reason)
{
    if (!usr)
        return;
    (void)reason;
    usr->strikeSpareKind = 0;
    usr->strikeSpareFlashTime = 0.0f;
    usr->strikeSpareEarlyAllDownTime = 0.0f;
    usr->strikeSpareEarlyDeclared = false;
    usr->strikeSpareEarlyKind = 0;
    usr->strikeSpareEarlyDeclaredAt = 0.0f;
    usr->strikeSpareSfxPlayedKind = 0;

    usr->negativeBannerKind = 0;
    usr->negativeBannerFlashTime = 0.0f;
    usr->negativeBannerSfxPlayedKind = 0;

    usr->neutralBannerFlashTime = 0.0f;
}

static inline void UI_ResetToIdleAndAbsolute(UserContext *usr, float dt, const char *reason)
{
    if (!usr)
        return;
    LogToIdle(usr, reason);
    usr->bufferedRequestThrow = false;
    usr->phy.set_ball_free();
    glm::vec3 idlePos = Scene_IdleBallPos(usr->scene);
    usr->carriedBall = idlePos;
    usr->carriedVel = glm::vec3(0.0f);
    usr->phase = UserContext::Phase::IDLE;
    usr->enjoy.resetJoystick();
    usr->aimFlatPos = glm::vec2(0.5f, 0.5f);
    usr->aimDownFlatPos = usr->aimFlatPos;
    SDL_SetRelativeMouseMode(SDL_FALSE);
    if (!std::isfinite(dt) || dt <= 0.0f)
        dt = 0.0f;
    usr->phy.set_manual_ball_position(idlePos, glm::quat(1.0f, 0, 0, 0), dt);
}

// ─────────────────────────────────────────────────────────────────────────────
// Enemy turn implementation (needs full UserContext definition)
// ─────────────────────────────────────────────────────────────────────────────
static inline bool IsEnemyTurn(const UserContext *usr)
{
    return usr && usr->turnOwner == UserContext::TurnOwner::ENEMY;
}

static inline void Enemy_ComputePins(UserContext *usr, const glm::vec3 initialPins[10])
{
    if (!usr)
        return;
    if (usr->enemyPinsInit)
        return;
    // Swap lane ends by rotating 180° around Y:
    //  x' = -x, z' = laneMinZ + (laneMaxZ - z)
    for (int i = 0; i < 10; ++i)
    {
        usr->enemyPins[i] = initialPins[i];
        usr->enemyPins[i].x = -usr->enemyPins[i].x;
        usr->enemyPins[i].z =
            usr->enemyLaneMinZ + (usr->enemyLaneMaxZ - usr->enemyPins[i].z);
    }
    usr->enemyPinsInit = true;
}

static inline glm::vec3 Enemy_IdleBallPos(const UserContext *usr)
{
    // Ball sits near the enemy end (near laneMaxZ) before the auto-throw.
    // Keep it on-lane but away from the pin deck.
    const float y = 0.30f;
    const float laneMaxZ = usr ? usr->enemyLaneMaxZ : 0.87f;
    const float z = laneMaxZ - 1.8f;
    return glm::vec3(0.0f, y, z);
}

static inline bool Enemy_StandingPinsMidpoint(const UserContext *usr, glm::vec3 &outMidpoint)
{
    if (!usr || !usr->enemyPinsInit)
        return false;

    glm::vec3 sum(0.0f);
    int count = 0;
    for (int i = 0; i < 10; i++)
    {
        if (usr->phy.mPinDead[i])
            continue;
        sum += usr->enemyPins[i];
        count++;
    }

    if (count <= 0)
        return false;

    outMidpoint = sum / (float)count;
    return true;
}

static inline glm::vec3 Enemy_RetargetCopiedThrowToStandingPins(UserContext *usr, glm::vec3 move)
{
    glm::vec3 target;
    if (!Enemy_StandingPinsMidpoint(usr, target))
        return move;

    glm::vec3 start = Enemy_IdleBallPos(usr);
    glm::vec2 copiedDir(move.x, move.z);
    float copiedSpeed = glm::length(copiedDir);
    if (!std::isfinite(copiedSpeed) || copiedSpeed <= 1e-4f)
        return move;

    glm::vec2 targetDir(target.x - start.x, target.z - start.z);
    float targetLen = glm::length(targetDir);
    if (!std::isfinite(targetLen) || targetLen <= 1e-4f)
        return move;

    copiedDir /= copiedSpeed;
    targetDir /= targetLen;

    const float retargetStrength = usr ? glm::clamp(usr->enemyRetargetStrength, 0.0f, 1.0f) : 0.85f;
    glm::vec2 finalDir = glm::mix(copiedDir, targetDir, retargetStrength);
    float finalLen = glm::length(finalDir);
    if (!std::isfinite(finalLen) || finalLen <= 1e-4f)
        finalDir = targetDir;
    else
        finalDir /= finalLen;

    move.x = finalDir.x * copiedSpeed;
    move.z = finalDir.y * copiedSpeed;
    return move;
}

static inline void Enemy_ComputeCameraEyeTargetAtBall(const glm::vec3 &ballPos, glm::vec3 &outEye, glm::vec3 &outTarget)
{
    const float followDist = 4.0f;
    outEye = glm::vec3(0.0f, 0.90f, ballPos.z - followDist);
    outTarget = glm::vec3(0.0f, 0.35f, ballPos.z + 1.0f);
}

static inline void Enemy_EnterTurn(UserContext *usr, const glm::vec3 initialPins[10])
{
    if (!usr)
        return;
    UI_ResetBannersForNewRoll(usr, "ENEMY_ENTER_TURN");
    Bot_InitIfNeeded(usr);
    Enemy_ComputePins(usr, initialPins);
    usr->turnOwner = UserContext::TurnOwner::ENEMY;
    usr->enemyAutoTimer = 0.0f;
    usr->enemyLaunched = false;
    usr->enemyDebugLogged = false;
    usr->enemyTurnSetup = true;
    usr->enemyBallRenderPosValid = false;
    usr->enemyBallRenderSecondsSinceLaunch = 0.0f;

    // Start the Angel bowling throw animation immediately; we will launch the ball
    // at a configurable fraction of this clip.
    if (Bot_AnimReady(usr))
    {
        Bot_PlayThrowIfPossible(usr, /*resetTime=*/true);
        // Ensure pose is evaluated for hand attachment on the first frame.
        (void)Bot_Anim(usr)->evaluate();
    }

    // Seed the render-only ball position from the hand so camera can track it smoothly.
    Enemy_SeedRenderedBallPosFromHand(usr);

    // Smooth camera transition to the enemy view when the enemy turn begins.
    // Use the render-ball Z (hand-attached) instead of the physics idle ball position to avoid flashes.
    {
        usr->cameraReturnActive = true;
        usr->cameraReturnT = 0.0f;
        usr->cameraReturnStartEye = usr->cameraEye;
        usr->cameraReturnStartTarget = usr->cameraTarget;
        glm::vec3 p = usr->enemyBallRenderPosValid ? usr->enemyBallRenderPos : Enemy_IdleBallPos(usr);
        Enemy_ComputeCameraEyeTargetAtBall(p, usr->cameraReturnEndEye, usr->cameraReturnEndTarget);
        usr->cameraReturnDuration = 0.5f;
    }

    // Put pins at player's end (mirrored), and reset ball.
    usr->phy.physics_reset(usr->enemyPins, usr->ballStart, /*reviveAll=*/true);

    glm::vec3 pos = Enemy_IdleBallPos(usr);
    usr->carriedBall = pos;
    usr->carriedVel = glm::vec3(0.0f);
    usr->throwingTime = 0.0f;
    usr->settlingTime = 0.0f;
    usr->aimingTime = 0.0f;
    usr->wereDead = 0;
    usr->strikeSpareSfxPlayedKind = 0;
    usr->negativeBannerSfxPlayedKind = 0;

    // Enemy auto-throw uses THROW loop, but we keep the ball static until `Enemy_TickAutoThrow` fires.
    usr->phase = UserContext::Phase::THROW;
    SDL_SetRelativeMouseMode(SDL_FALSE);
    usr->enjoy.resetJoystick();
    usr->phy.set_ball_free();
    usr->phy.set_manual_ball_position(pos, glm::quat(1.0f, 0, 0, 0), 0.0f);
}

static inline void Player_EnterTurn(UserContext *usr)
{
    if (!usr)
        return;
    UI_ResetBannersForNewRoll(usr, "PLAYER_ENTER_TURN");
    usr->turnOwner = UserContext::TurnOwner::PLAYER;
    usr->enemyTurnSetup = false;
    usr->wereDead = 0;
    // Normal game always uses the standard pin deck.
    usr->phy.physics_reset(usr->initialPins, usr->ballStart, /*reviveAll=*/true);
    UI_ResetToIdleAndAbsolute(usr, 0.0f, "TURN_TO_PLAYER");

    // Smooth camera transition back to player idle (covers non-frame-complete entry paths).
    {
        usr->cameraReturnActive = true;
        usr->cameraReturnT = 0.0f;
        usr->cameraReturnStartEye = usr->cameraEye;
        usr->cameraReturnStartTarget = usr->cameraTarget;
        glm::vec3 idleBallPos = Scene_IdleBallPos(usr->scene);
        Scene_ComputeCameraEyeTarget(usr->scene, idleBallPos, usr->cameraReturnEndEye, usr->cameraReturnEndTarget);
        usr->cameraReturnDuration = 1.0f;
    }
}

static inline void Enemy_EnsureTurnActive(UserContext *usr, float dt)
{
    if (!usr)
        return;
    if (usr->gameMode != UserContext::GameMode::BOT)
        return;
    if (!IsEnemyTurn(usr))
        return;

    // If we ever enter enemy ownership without having run the enemy turn setup,
    // (e.g. hot-reload or a UI flow that only flips the owner), do it now.
    if (!usr->enemyTurnSetup)
    {
        Enemy_EnterTurn(usr, usr->initialPins);
        return;
    }

    // Defensive: certain UI flows reset the phase back to IDLE (or AIM/SWING)
    // while the enemy is the turn owner. Ensure we get back to THROW so
    // `Enemy_TickAutoThrow` runs and the enemy eventually launches.
    if ((usr->phase == UserContext::Phase::IDLE ||
         usr->phase == UserContext::Phase::AIM ||
         usr->phase == UserContext::Phase::SWING) &&
        !usr->enemyLaunched)
    {
        // Make sure the pre-shot animation is playing if we got kicked back to a non-throw phase.
        Bot_InitIfNeeded(usr);
        if (Bot_AnimReady(usr))
            Bot_PlayThrowIfPossible(usr, /*resetTime=*/true);

        glm::vec3 pos = Enemy_IdleBallPos(usr);
        usr->bufferedRequestThrow = false;
        usr->carriedBall = pos;
        usr->carriedVel = glm::vec3(0.0f);
        usr->throwingTime = 0.0f;
        usr->settlingTime = 0.0f;
        usr->aimingTime = 0.0f;
        usr->phase = UserContext::Phase::THROW;
        usr->phy.set_ball_free();
        usr->phy.set_manual_ball_position(pos, glm::quat(1.0f, 0, 0, 0), dt);
        usr->enemyBallRenderPosValid = false;
    }
}

static inline bool Enemy_TickAutoThrow(UserContext *usr, float dt)
{
    if (!usr || !IsEnemyTurn(usr))
        return false;

    // Primary: drive the launch timing from the Angel "BowlingThrow" animation.
    bool shouldLaunch = false;
    const int throwClip = Bot_ClipThrow(usr);
    AssmanAnimPlayer *anim = Bot_Anim(usr);
    if (!usr->enemyLaunched && Bot_AnimReady(usr) && throwClip >= 0 && anim)
    {
        // Ensure the throw clip is active while we're waiting to launch.
        if (anim->activeClip != throwClip || anim->loop)
            Bot_PlayThrowIfPossible(usr, /*resetTime=*/true);

        float dur = Bot_ClipDurationSeconds(usr, throwClip);
        float frac = glm::clamp(usr->angelThrowLaunchFrac, 0.0f, 1.0f);
        if (dur > 0.0f)
            shouldLaunch = (anim->t >= dur * frac);
    }

    // Fallback: time-based auto throw if the animation isn't available.
    if (!usr->enemyLaunched && !shouldLaunch)
    {
        usr->enemyAutoTimer += dt;
        if (!usr->enemyDebugLogged && usr->enemyAutoTimer >= 0.25f)
        {
            usr->enemyDebugLogged = true;
            std::cerr << "[enemy] armed, will launch at t>=1.0s pos=" << Enemy_IdleBallPos(usr).z << "\n";
        }
        if (usr->enemyAutoTimer >= 1.0f)
            shouldLaunch = true;
    }

    if (!usr->enemyLaunched && shouldLaunch)
    {
        glm::vec3 move =
            usr->haveLastPlayerRelease ? usr->lastPlayerReleaseMovement : glm::vec3(0.0f, 0.0f, 8.0f);
        move.x = -move.x;
        move.z = -move.z;
        move = Enemy_RetargetCopiedThrowToStandingPins(usr, move);

        // Switch the ball from kinematic (manual placement) to dynamic before launching.
        // Otherwise SetLinearVelocity won't move it.
        usr->phy.set_ball_free();
        usr->phy.enable_physics_on_ball();

        // Launch slightly upward (enemy "shoots" the ball instead of using a pivot swing).
        // Enemy rolls toward the player end (negative Z).
        move.y = glm::max(move.y, 1.0f);
        usr->phy.set_ball_swing_movement(move);

        float spin = usr->haveLastPlayerRelease ? usr->lastPlayerReleaseSpinSpeed : 0.0f;
        usr->phy.apply_angular_velocity_on_ball(-spin);

        usr->enemyLaunched = true;
        usr->enemyBallRenderSecondsSinceLaunch = 0.0f;
        usr->throwingTime = 0.0f;
        std::cerr << "[enemy] LAUNCH move=(" << move.x << "," << move.y << "," << move.z << ") spin=" << (-spin) << "\n";
        return true;
    }
    return false;
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

static inline const CampaignLevelConfig &Campaign_GetLevelConfig(int levelIndex)
{
    int idx = glm::clamp(levelIndex, 1, kCampaignLevelCount) - 1;
    return kCampaignLevels[idx];
}

static inline const CampaignLevelConfig &Campaign_CurrentLevel(const UserContext *usr)
{
    return Campaign_GetLevelConfig(usr ? usr->campaignLevelIndex : 1);
}

static inline void Campaign_SaveCurrentLevel(UserContext *usr);

static inline bool UnlockMask_HasBall(const UserContext *usr, int ballId)
{
    return usr && ballId >= 0 && ballId < 63 && ((usr->unlockedBallMask >> ballId) & 1ull) != 0ull;
}

static inline bool UnlockMask_HasHouse(const UserContext *usr, int houseId)
{
    return usr && houseId >= 0 && houseId < 31 && ((usr->unlockedHouseMask >> houseId) & 1u) != 0u;
}

static inline bool UnlockMask_HasOpponent(const UserContext *usr, CampaignOpponent opponent)
{
    return usr && opponent != CampaignOpponent::NONE &&
           ((usr->unlockedBotMask >> (int)opponent) & 1u) != 0u;
}

static inline void UnlockMask_AddBall(UserContext *usr, int ballId)
{
    if (!usr || ballId < 0 || ballId >= 63)
        return;
    usr->unlockedBallMask |= (1ull << ballId);
}

static inline void UnlockMask_AddHouse(UserContext *usr, int houseId)
{
    if (!usr || houseId < 0 || houseId >= 31)
        return;
    usr->unlockedHouseMask |= (1u << houseId);
}

static inline void UnlockMask_AddOpponent(UserContext *usr, CampaignOpponent opponent)
{
    if (!usr || opponent == CampaignOpponent::NONE)
        return;
    usr->unlockedBotMask |= (1u << (int)opponent);
}

static inline bool Progress_EnsureStarterUnlocks(UserContext *usr)
{
    if (!usr)
        return false;

    const uint64_t prevBallMask = usr->unlockedBallMask;
    const uint32_t prevHouseMask = usr->unlockedHouseMask;
    const uint32_t prevBotMask = usr->unlockedBotMask;

    UnlockMask_AddBall(usr, 0);
    UnlockMask_AddHouse(usr, 0);
    UnlockMask_AddOpponent(usr, CampaignOpponent::MALACH);

    return usr->unlockedBallMask != prevBallMask ||
           usr->unlockedHouseMask != prevHouseMask ||
           usr->unlockedBotMask != prevBotMask;
}

static inline void Progress_SaveUnlocksAndBank(UserContext *usr)
{
    if (!usr)
        return;
    char buf[64];
    snprintf(buf, sizeof(buf), "%d", (int)std::lround(usr->carousel.bank));
    usr->storage.setChar(Storage::BANK, buf, strlen(buf));
    snprintf(buf, sizeof(buf), "%llu", (unsigned long long)usr->unlockedBallMask);
    usr->storage.setChar(Storage::UNLOCKED_BALLS, buf, strlen(buf));
    snprintf(buf, sizeof(buf), "%u", usr->unlockedHouseMask);
    usr->storage.setChar(Storage::UNLOCKED_HOUSES, buf, strlen(buf));
    snprintf(buf, sizeof(buf), "%u", usr->unlockedBotMask);
    usr->storage.setChar(Storage::UNLOCKED_BOTS, buf, strlen(buf));
}

static inline void Progress_ResetCampaign(UserContext *usr)
{
    if (!usr)
        return;
    usr->campaignLevelIndex = 1;
    usr->campaignStartStoryLevelShown = 0;
    usr->carousel.bank = 20.0f;
    usr->unlockedBallMask = 0;
    usr->unlockedHouseMask = 0;
    usr->unlockedBotMask = 0;
    Progress_EnsureStarterUnlocks(usr);
    usr->firstSoloCompleted = false;
    usr->milestone100Reached = false;
    usr->schoolExitLocked = false;
    Campaign_SaveCurrentLevel(usr);
    Progress_SaveUnlocksAndBank(usr);
}

static inline const HouseCatalogItem *House_FindById(int id)
{
    for (int i = 0; i < g_houseCatalogCount; ++i)
        if (g_houseCatalog[i].id == id)
            return &g_houseCatalog[i];
    return nullptr;
}

static inline const CatalogItem *Ball_FindById(int id)
{
    for (int i = 0; i < g_ballCatalogCount; ++i)
        if (g_ballCatalog[i].id == id)
            return &g_ballCatalog[i];
    return nullptr;
}

static inline void ApplyHouseCatalogToUser(UserContext *usr, const HouseCatalogItem *house)
{
    if (!usr || !house)
        return;
    usr->houseLane.laneFriction = house->laneFriction;
    usr->houseLane.lanePushbackStrength = house->lanePushbackStrength;
    usr->houseLane.laneOilThickness = house->laneOilThickness;
    usr->houseLane.leftOilFadeStartM = house->leftOilFadeStartM;
    usr->houseLane.leftOilFadeEndM = house->leftOilFadeEndM;
    usr->houseLane.rightOilFadeStartM = house->rightOilFadeStartM;
    usr->houseLane.rightOilFadeEndM = house->rightOilFadeEndM;
    usr->houseLane.oilCarrydownPerBallTravelM = house->oilCarrydownPerBallTravelM;
    usr->houseLane.oilThicknessDecayPerBallTravel = house->oilThicknessDecayPerBallTravel;
    usr->laneTextureIdx = house->laneTextureIdx;
    usr->pinTextureIdx = house->pinTextureIdx;
    ApplyHouseLaneParams(usr);
}

static inline BotAvatar Campaign_BotAvatarForOpponent(CampaignOpponent opponent)
{
    switch (opponent)
    {
        case CampaignOpponent::DOG:
            return BotAvatar::CHERUB;
        case CampaignOpponent::BEAK:
            return BotAvatar::SERAPH;
        case CampaignOpponent::COW:
            return BotAvatar::THRONE;
        case CampaignOpponent::MALACH:
        default:
            return BotAvatar::ANGEL;
    }
}

static inline const char *Campaign_OpponentDisplayName(CampaignOpponent opponent)
{
    switch (opponent)
    {
        case CampaignOpponent::MALACH:
            return "Malach";
        case CampaignOpponent::DOG:
            return "Dog";
        case CampaignOpponent::BEAK:
            return "Beak";
        case CampaignOpponent::COW:
            return "Cow";
        default:
            return "Solo";
    }
}

static inline const char *BotAvatar_DisplayName(BotAvatar avatar)
{
    switch (avatar)
    {
        case BotAvatar::CHERUB:
            return "Dog";
        case BotAvatar::SERAPH:
            return "Beak";
        case BotAvatar::THRONE:
            return "Cow";
        case BotAvatar::ANGEL:
        default:
            return "Malach";
    }
}

static inline void Campaign_ApplyBiomePreset(UserContext *usr, CampaignBiome biome)
{
    if (!usr)
        return;

    switch (biome)
    {
        case CampaignBiome::DESERT:
            usr->houseLane = {0.055f, 22.0f, 0.88f, 6.8f, 10.8f, 6.8f, 10.8f, 0.024f, 0.0042f};
            usr->laneTextureIdx = 1;
            usr->pinTextureIdx = 1;
            break;
        case CampaignBiome::ICE:
            usr->houseLane = {0.040f, 18.0f, 0.98f, 9.6f, 14.8f, 9.6f, 14.8f, 0.0f, 0.0016f};
            usr->laneTextureIdx = 2;
            usr->pinTextureIdx = 2;
            break;
        case CampaignBiome::NEON:
            usr->houseLane = {0.050f, 6.0f, 0.80f, 7.0f, 11.7f, 7.0f, 11.7f, 0.032f, 0.0028f};
            usr->laneTextureIdx = 3;
            usr->pinTextureIdx = 3;
            break;
        case CampaignBiome::NORMAL:
        default:
            usr->houseLane = {0.050f, 34.0f, 1.00f, 8.8f, 14.1f, 8.8f, 14.1f, 0.006f, 0.0008f};
            usr->laneTextureIdx = 0;
            usr->pinTextureIdx = 0;
            break;
    }

    ApplyHouseLaneParams(usr);
}

static inline void Campaign_SaveCurrentLevel(UserContext *usr)
{
    if (!usr)
        return;
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", glm::clamp(usr->campaignLevelIndex, 1, kCampaignLevelCount));
    usr->storage.setChar(Storage::LAST_LEVEL, buf, strlen(buf));
}

static inline void Campaign_SetResultWindowLabels(UserContext *usr, bool advanced);

static inline void Campaign_ApplyCurrentLevelSetup(UserContext *usr, bool resetStoryKick)
{
    if (!usr)
        return;

    usr->campaignLevelIndex = glm::clamp(usr->campaignLevelIndex, 1, kCampaignLevelCount);
    const CampaignLevelConfig &cfg = Campaign_CurrentLevel(usr);

    usr->playerRoute = PlayerRoute::CAMPAIGN;
    usr->gameMode = (cfg.mode == CampaignMode::SOLO) ? UserContext::GameMode::SOLO : UserContext::GameMode::BOT;
    usr->botAvatar = Campaign_BotAvatarForOpponent(cfg.opponent);
    usr->enemyRetargetStrength = glm::clamp(cfg.enemySkill, 0.0f, 1.0f);
    usr->pendingCampaignEndStoryId = 0;
    usr->pendingCampaignBotResultWindow = false;
    Campaign_SetResultWindowLabels(usr, /*advanced=*/false);

    Campaign_ApplyBiomePreset(usr, cfg.biome);
    usr->coinLane.initStars(cfg.pattern, cfg.collectableCount);

    if (usr->gameMode == UserContext::GameMode::BOT)
    {
        usr->turnOwner = UserContext::TurnOwner::PLAYER;
        usr->enemyAutoTimer = 0.0f;
        usr->enemyLaunched = false;
        usr->enemyDebugLogged = false;
        usr->enemyTurnSetup = false;
    }

    if (resetStoryKick)
        usr->campaignStartStoryLevelShown = 0;
}

static inline void Campaign_AdvanceIfWon(UserContext *usr, const CampaignLevelConfig &cfg)
{
    if (!usr)
        return;

    usr->carousel.bank += (float)glm::max(0, cfg.rewardBank);
    if (cfg.unlockBallId >= 0)
        UnlockMask_AddBall(usr, cfg.unlockBallId);
    if (cfg.unlockHouseId >= 0)
        UnlockMask_AddHouse(usr, cfg.unlockHouseId);
    if (cfg.unlockOpponent != CampaignOpponent::NONE)
        UnlockMask_AddOpponent(usr, cfg.unlockOpponent);
    if (usr->campaignLevelIndex < kCampaignLevelCount)
        usr->campaignLevelIndex++;
    Campaign_SaveCurrentLevel(usr);
    Progress_SaveUnlocksAndBank(usr);
    usr->campaignStartStoryLevelShown = 0;
}

static inline void Campaign_SetResultWindowLabels(UserContext *usr, bool advanced)
{
    if (!usr)
        return;
    usr->clayton.newGameTitle = advanced ? Txl_Get(usr->language, TXL_NEXT_LEVEL) : Txl_Get(usr->language, TXL_TRY_AGAIN);
    usr->clayton.newGameButtonLabel = usr->clayton.newGameTitle;
}

static inline TxlKey Campaign_TitleKey(int levelNumber)
{
    switch (levelNumber)
    {
        case 1: return TXL_LEVEL1_TITLE;
        case 2: return TXL_LEVEL2_TITLE;
        case 3: return TXL_LEVEL3_TITLE;
        case 4: return TXL_LEVEL4_TITLE;
        case 5: return TXL_LEVEL5_TITLE;
        case 6: return TXL_LEVEL6_TITLE;
        case 7: return TXL_LEVEL7_TITLE;
        case 8: return TXL_LEVEL8_TITLE;
        case 9: return TXL_LEVEL9_TITLE;
        case 10: return TXL_LEVEL10_TITLE;
        case 11: return TXL_LEVEL11_TITLE;
        default: return TXL_LEVEL1_TITLE;
    }
}

static inline TxlKey Campaign_SubtitleKey(int levelNumber)
{
    switch (levelNumber)
    {
        case 1: return TXL_LEVEL1_SUBTITLE;
        case 2: return TXL_LEVEL2_SUBTITLE;
        case 3: return TXL_LEVEL3_SUBTITLE;
        case 4: return TXL_LEVEL4_SUBTITLE;
        case 5: return TXL_LEVEL5_SUBTITLE;
        case 6: return TXL_LEVEL6_SUBTITLE;
        case 7: return TXL_LEVEL7_SUBTITLE;
        case 8: return TXL_LEVEL8_SUBTITLE;
        case 9: return TXL_LEVEL9_SUBTITLE;
        case 10: return TXL_LEVEL10_SUBTITLE;
        case 11: return TXL_LEVEL11_SUBTITLE;
        default: return TXL_LEVEL1_SUBTITLE;
    }
}

static inline void SelectorFlow_Cancel(UserContext *usr)
{
    if (!usr)
        return;
    usr->selectorFlowStep = SelectorFlowStep::NONE;
    usr->clayton.shouldShowBotSelect = false;
    usr->clayton.shouldShowHouses = false;
    usr->shouldShowShop = false;
}

static inline void SelectorFlow_OpenStep(UserContext *usr, SelectorFlowStep step)
{
    if (!usr)
        return;
    usr->selectorFlowStep = step;
    usr->clayton.shouldShowBotSelect = false;
    usr->clayton.shouldShowHouses = false;
    usr->shouldShowShop = false;
    if (step == SelectorFlowStep::BOT)
    {
        usr->clayton.shouldShowBotSelect = true;
        usr->windowStack.windowStackPushBotSelectWindow();
    }
    else if (step == SelectorFlowStep::HOUSE)
    {
        usr->clayton.shouldShowHouses = true;
        usr->windowStack.windowStackPushHousesWindow();
    }
    else if (step == SelectorFlowStep::BALL)
    {
        usr->shouldShowShop = true;
        usr->windowStack.windowStackPushShopWindow();
    }
}

static inline void Run_ResetBoardsAndMode(UserContext *usr, UserContext::GameMode gameMode)
{
    if (!usr)
        return;
    usr->gameMode = gameMode;
    usr->phase = UserContext::Phase::IDLE;
    usr->turnOwner = UserContext::TurnOwner::PLAYER;
    usr->enemyAutoTimer = 0.0f;
    usr->enemyLaunched = false;
    usr->enemyDebugLogged = false;
    usr->enemyTurnSetup = false;
    usr->clayton.shouldShowHiScore = false;
    usr->clayton.shouldShowHiScoreWithLatest = false;
    usr->clayton.shouldShowBotSelect = false;
    usr->clayton.shouldShowHouses = false;
    usr->shouldShowShop = false;
    usr->windowStack.count = 0;
    usr->windowStack.botResultPlayerScore = 0;
    usr->windowStack.botResultAngelScore = 0;
    resetScoreboard(&usr->board);
    if (usr->enemyBoardInit)
        resetScoreboard(&usr->enemyBoard);
    PhysicsResetForMode(usr, /*reviveAll=*/true);
    usr->electroBall.resetCharge();
}

static inline void StartPracticeRun(UserContext *usr)
{
    if (!usr)
        return;
    const HouseCatalogItem *house = House_FindById(usr->selectedHouseId);
    const CatalogItem *ball = Ball_FindById(usr->selectedBallId);
    if (!house || !ball)
        return;
    usr->playerRoute = PlayerRoute::PRACTICE;
    ApplyHouseCatalogToUser(usr, house);
    BallStats_OnBallChange(ball, usr);
    Run_ResetBoardsAndMode(usr, UserContext::GameMode::SOLO);
    Campaign_SetResultWindowLabels(usr, /*advanced=*/false);
    SelectorFlow_Cancel(usr);
}

static inline void StartFreestyleRun(UserContext *usr)
{
    if (!usr)
        return;
    const HouseCatalogItem *house = House_FindById(usr->selectedHouseId);
    const CatalogItem *ball = Ball_FindById(usr->selectedBallId);
    if (!house || !ball)
        return;
    usr->playerRoute = PlayerRoute::FREESTYLE;
    ApplyHouseCatalogToUser(usr, house);
    BallStats_OnBallChange(ball, usr);
    usr->botAvatar = usr->selectedFreestyleAvatar;
    Run_ResetBoardsAndMode(usr, UserContext::GameMode::BOT);
    Campaign_SetResultWindowLabels(usr, /*advanced=*/false);
    SelectorFlow_Cancel(usr);
}

static inline bool Bowling_NeedsFreshRackForNextRoll(const BowlingScoreboard *sb)
{
    if (!sb)
        return false;
    // Find active frame (similar to addRoll's search) — but we care about frame 10 only.
    int f = 0;
    for (; f < 10; ++f)
    {
        const Frame *fr = &sb->frames[f];
        if (f == 9)
        {
            // 10th frame: if roll3 already taken, game is done.
            if (fr->roll3 != -1)
                return false;
            break;
        }
        if (fr->isStrike)
            continue;
        if (fr->roll2 == -1)
            break;
    }
    if (f != 9)
        return false;

    const Frame *fr = &sb->frames[9];
    // Before roll2: if roll1 was a strike, roll2 must start with a fresh rack.
    if (fr->roll1 != -1 && fr->roll2 == -1 && fr->isStrike)
        return true;

    // Before roll3: fresh rack if (spare) OR (strike + strike).
    if (fr->roll2 != -1 && fr->roll3 == -1)
    {
        if (fr->isSpare)
            return true;
        if (fr->isStrike && fr->roll2 == 10)
            return true;
    }

    return false;
}

static inline void School_ApplyNoPinsForLesson3(UserContext *usr)
{
    if (!usr)
        return;
    if (!(usr->gameMode == UserContext::GameMode::SCHOOL && usr->school.selectedLesson == 3))
        return;
    // Mark all pins dead so `physics_reset(..., reviveAll=false)` keeps them out of play,
    // and place them far away so they never collide with the ball/lane.
    glm::vec3 farPins[10];
    for (int i = 0; i < 10; i++)
    {
        usr->phy.mPinDead[i] = true;
        farPins[i] = glm::vec3(1000.0f + (float)i * 2.0f, -1000.0f, 1000.0f);
    }
    usr->phy.physics_reset(farPins, usr->ballStart, /*reviveAll=*/false);
}

static inline void School_ApplyPinModeForSelectedLesson(UserContext *usr)
{
    if (!usr)
        return;
    if (usr->gameMode != UserContext::GameMode::SCHOOL)
        return;
    if (!School_LessonHasPins(usr->school.selectedLesson))
    {
        School_ApplyNoPinsForLesson3(usr);
    }
    else
    {
        // Ensure pins are back and alive for any other lesson.
        for (int i = 0; i < 10; i++)
            usr->phy.mPinDead[i] = false;
        usr->phy.physics_reset(usr->initialPins, usr->ballStart, /*reviveAll=*/true);
    }
}

static inline void PhysicsResetForMode(UserContext *usr, bool reviveAll)
{
    if (!usr)
        return;
    if (usr->gameMode == UserContext::GameMode::SCHOOL && !School_LessonHasPins(usr->school.selectedLesson))
    {
        School_ApplyNoPinsForLesson3(usr);
        return;
    }
    usr->phy.physics_reset(usr->initialPins, usr->ballStart, reviveAll);
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
        // Hot reload: re-register School Clay IDs / slider without resetting progress.
        School_ClayInit(&usr->school, &usr->clayton, usr->desiredMass);

    setupStubScoreboardMax(&usr->board);

	    Carousel_SetupDefaultShop(&usr->carousel);

    // Hot reload: Angel mesh/anim are kept out of UserContext, so initialize here too.
    Angel_InitIfNeeded(usr);
    Gem_InitIfNeeded(usr);
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
    // Arm impulse range (kg*m/s) applied at release along forward movement direction.
    // Bigger range makes mass differences noticeable (Δv = J / m).
    static constexpr float PHYSICS_ARM_IMPULSE_MIN = 6.0f;
    static constexpr float PHYSICS_ARM_IMPULSE_MAX = 16.0f;
    static constexpr float PHYSICS_FRICTION_MIN = 0.0f;
    static constexpr float PHYSICS_FRICTION_MAX = 0.15f;

    // Tunable multipliers for fine-tuning feel
    static constexpr float SPIN_MULTIPLIER = 1.0f;
    static constexpr float BITE_TO_FRICTION_SCALE = 1.0f;

    // Launch buff mass modifier ("lightness buff").
    // Reference point: the default first ball ("Ember Strike") mass mapping.
    static constexpr float LIGHTNESS_REF_MASS_KG = 6.35f;
    static constexpr float LIGHTNESS_MIN_MASS_KG = 0.5f;
    static constexpr float LIGHTNESS_NO_BUFF_OVER_KG = 20.0f;
    // Stronger effect: light balls get noticeably more launch buff, heavy balls noticeably less.
    // Note: we still clamp the final effective launchBuff into [0..1].
    static constexpr float LIGHTNESS_MAX_MULTIPLIER = 1.45f; // up to 1.45x at min mass (keeps light balls less OP)
    static constexpr float LIGHTNESS_LIGHT_EXP_K = 0.45f;    // smaller -> slower ramp for light balls
    static constexpr float LIGHTNESS_HEAVY_EXP_K = 1.0f; //0.35f;   // bigger -> quicker falloff for heavy balls

    // Restitution mass modifier:
    // - At reference mass, modifier is 1.0 (catalog restitution unchanged)
    // - Lighter balls get *more* bouncy (up to RESTITUTION_MAX_MULTIPLIER at 1kg)
    // - Heavier balls lose bounce exponentially, reaching ~0 by 15kg
    static constexpr float RESTITUTION_REF_MASS_KG = LIGHTNESS_REF_MASS_KG;
    static constexpr float RESTITUTION_LIGHT_TARGET_KG = 1.0f;
    static constexpr float RESTITUTION_HEAVY_ZERO_KG = 15.0f;
    static constexpr float RESTITUTION_MAX_MULTIPLIER = 2.0f; // 1kg ball can be up to 2x bouncier
    static constexpr float RESTITUTION_LIGHT_EXP_K = 0.40f;
    static constexpr float RESTITUTION_HEAVY_EXP_K = 0.40f;
};

// School tuning + logic is in `school/school.h`.

static inline float BallStats_LightnessBuff(float massKg)
{
    // 1.0 at reference mass, ramps up for lighter balls, ramps down for heavier balls.
    massKg = glm::max(0.001f, massKg);
    const float ref = BallPhysicsMapping::LIGHTNESS_REF_MASS_KG;
    const float minM = BallPhysicsMapping::LIGHTNESS_MIN_MASS_KG;
    const float noBuffM = BallPhysicsMapping::LIGHTNESS_NO_BUFF_OVER_KG;

    if (massKg <= ref)
    {
        // Exponential ease: ramps quickly as the ball gets light.
        float d = glm::clamp(ref - massKg, 0.0f, ref - minM);
        float denom = 1.0f - expf(-BallPhysicsMapping::LIGHTNESS_LIGHT_EXP_K * glm::max(0.001f, (ref - minM)));
        float t = (denom > 1e-6f) ? (1.0f - expf(-BallPhysicsMapping::LIGHTNESS_LIGHT_EXP_K * d)) / denom : 0.0f; // 0..1
        t = glm::clamp(t, 0.0f, 1.0f);
        return glm::mix(1.0f, BallPhysicsMapping::LIGHTNESS_MAX_MULTIPLIER, t);
    }
    else
    {
        // Exponential falloff: slows the launch buff quickly for heavier balls.
        // Hard clamp to 0 at/over the "no buff" threshold.
        if (massKg >= noBuffM)
            return 0.0f;
        float d = glm::max(0.0f, massKg - ref);
        float v = expf(-BallPhysicsMapping::LIGHTNESS_HEAVY_EXP_K * d); // 1..~0
        return glm::clamp(v, 0.0f, 1.0f);
    }
}

static inline float BallStats_RestitutionMassScale(float massKg)
{
    // 1.0 at reference mass; >1.0 for light balls; ->0 for heavy balls.
    massKg = glm::max(0.001f, massKg);
    const float ref = BallPhysicsMapping::RESTITUTION_REF_MASS_KG;
    const float lightTarget = BallPhysicsMapping::RESTITUTION_LIGHT_TARGET_KG;
    const float heavyZero = BallPhysicsMapping::RESTITUTION_HEAVY_ZERO_KG;

    if (massKg <= ref)
    {
        float d = glm::clamp(ref - massKg, 0.0f, ref - lightTarget);
        float denom = 1.0f - expf(-BallPhysicsMapping::RESTITUTION_LIGHT_EXP_K *
                                  glm::max(0.001f, (ref - lightTarget)));
        float t = (denom > 1e-6f) ? (1.0f - expf(-BallPhysicsMapping::RESTITUTION_LIGHT_EXP_K * d)) / denom : 0.0f;
        t = glm::clamp(t, 0.0f, 1.0f);
        return glm::mix(1.0f, BallPhysicsMapping::RESTITUTION_MAX_MULTIPLIER, t);
    }

    if (massKg >= heavyZero)
        return 0.0f;

    float d = glm::max(0.0f, massKg - ref);
    float v = expf(-BallPhysicsMapping::RESTITUTION_HEAVY_EXP_K * d); // 1..~0
    return glm::clamp(v, 0.0f, 1.0f);
}

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

// School Lesson 4 (Oil) uses its own lane defaults (does not cost money to re-oil).
// Placed here so it can reuse the same catalog->physics mapping constants as the rest of ball/lane tuning.
struct SchoolOilLessonDefaults
{
    float laneFriction = 0.03f;         // more slippery for this lesson
    float lanePushbackStrength = 48.0f; // high pushback power for the lesson
    float laneOilThickness = 1.0f;
    float leftOilFadeStartM = 6.5f;
    float leftOilFadeEndM = 12.8f;
    float rightOilFadeStartM = 6.5f;
    float rightOilFadeEndM = 12.8f;
    float oilCarrydownPerBallTravelM = 0.04f;      // wears/spreads fast (but not instantly)
    float oilThicknessDecayPerBallTravel = 0.016f; // decays fast (aim ~1/3 per roll)
    float ballSkidOverride01 = 0.95f;              // make skid noticeably higher for this lesson only
    float armImpulseMultiplier = 1.12f;            // a little extra "hand launch boost" for this lesson only
    float wearMultiplier = 1.35f;                  // extra wear scaling so it takes a few rolls to unlock re-oil
    float maxThicknessDropPerRoll = 0.35f;         // safety cap: a single roll can't wipe all oil
};

static inline const SchoolOilLessonDefaults &School_OilDefaults()
{
    static SchoolOilLessonDefaults s;
    return s;
}

// Neutral/moderate oiling for all school lessons except Lesson 4 (oil lesson).
struct SchoolNeutralLaneDefaults
{
    float laneFriction = 0.055f;
    float lanePushbackStrength = 22.0f; // moderate, not too much inbound force
    float laneOilThickness = 0.75f;     // "pleasantly oiled"
    float leftOilFadeStartM = 6.8f;
    float leftOilFadeEndM = 12.6f;
    float rightOilFadeStartM = 6.8f;
    float rightOilFadeEndM = 12.6f;
    float oilCarrydownPerBallTravelM = 0.0f;      // no carrydown in school (except lesson 4)
    float oilThicknessDecayPerBallTravel = 0.0f;  // no decay in school (except lesson 4)
};

static inline const SchoolNeutralLaneDefaults &School_NeutralLaneDefaults()
{
    static SchoolNeutralLaneDefaults s;
    return s;
}

static inline void School_ApplyNeutralLaneDefaults(UserContext *usr)
{
    if (!usr)
        return;
    const SchoolNeutralLaneDefaults &d = School_NeutralLaneDefaults();
    usr->laneFriction = d.laneFriction;
    usr->lanePushbackStrength = d.lanePushbackStrength;
    usr->laneOilThickness = d.laneOilThickness;
    usr->leftOilFadeStartM = d.leftOilFadeStartM;
    usr->leftOilFadeEndM = d.leftOilFadeEndM;
    usr->rightOilFadeStartM = d.rightOilFadeStartM;
    usr->rightOilFadeEndM = d.rightOilFadeEndM;
    usr->oilCarrydownPerBallTravelM = d.oilCarrydownPerBallTravelM;
    usr->oilThicknessDecayPerBallTravel = d.oilThicknessDecayPerBallTravel;
    usr->oilWearLeftM = 0.0f;
    usr->oilWearRightM = 0.0f;
    usr->oilWearTotalM = 0.0f;
}

// Lesson 5 (Strike line): keep neutral oiling, but essentially disable inbound pushback.
static inline void School_ApplyStrikeLaneDefaults(UserContext *usr)
{
    if (!usr)
        return;
    School_ApplyNeutralLaneDefaults(usr);
    usr->lanePushbackStrength = 0.8f;
}

static inline void School_ApplyOilLessonDefaults(UserContext *usr)
{
    if (!usr)
        return;
    const SchoolOilLessonDefaults &d = School_OilDefaults();
    usr->laneFriction = d.laneFriction;
    usr->lanePushbackStrength = d.lanePushbackStrength;
    usr->laneOilThickness = d.laneOilThickness;
    usr->leftOilFadeStartM = d.leftOilFadeStartM;
    usr->leftOilFadeEndM = d.leftOilFadeEndM;
    usr->rightOilFadeStartM = d.rightOilFadeStartM;
    usr->rightOilFadeEndM = d.rightOilFadeEndM;
    usr->oilCarrydownPerBallTravelM = d.oilCarrydownPerBallTravelM;
    usr->oilThicknessDecayPerBallTravel = d.oilThicknessDecayPerBallTravel;
    usr->oilWearLeftM = 0.0f;
    usr->oilWearRightM = 0.0f;
    usr->oilWearTotalM = 0.0f;

    // Lesson-only ball tuning: raise skid (slippery early slide) regardless of selected ball.
    // When leaving lesson 4, we re-apply friction params from the catalog to restore normal behavior.
    usr->ballSkid = glm::clamp(glm::max(usr->ballSkid, d.ballSkidOverride01), 0.0f, 1.0f);
    // Equivalent to remapClamped(ballSkid, 0..1, low..high) without depending on remapClamped ordering.
    usr->ballSkidStartScale = glm::mix(
        BallFrictionTuning::SKID_START_SCALE_LOW_SKID,
        BallFrictionTuning::SKID_START_SCALE_HIGH_SKID,
        usr->ballSkid
    );

    // Lesson-only throw boost: slightly increase the forward impulse applied at release.
    // Keep within the same clamp range used by catalog mapping.
    usr->armImpulseAtThrow = glm::clamp(
        usr->armImpulseAtThrow * d.armImpulseMultiplier,
        BallPhysicsMapping::PHYSICS_ARM_IMPULSE_MIN,
        BallPhysicsMapping::PHYSICS_ARM_IMPULSE_MAX
    );
}

static inline bool School_OilLessonCanReoil(const UserContext *usr)
{
    if (!usr)
        return false;
    // Require the lane to wear down first before allowing another re-oil.
    // This is based on overall thickness (0..1). Lower means more wear.
    return usr->laneOilThickness <= 0.45f;
}

// School Lesson 5 strike line side selection (file-scope so it persists across throws without touching UserContext).
static bool g_schoolStrikeAimLeftPocket = true; // true=between pins 1&2, false=between pins 1&3

static constexpr float SCHOOL_STRIKE_SWAP_INTERVAL_S = 10.0f;
static constexpr float SCHOOL_STRIKE_SWAP_DURATION_S = 0.75f;
static constexpr float SCHOOL_STRIKE_SWAP_MAX_START_DELAY_S = 0.5f;
static float g_schoolStrikeSwapElapsed = SCHOOL_STRIKE_SWAP_INTERVAL_S;
static bool g_schoolStrikeSwapInProgress = false;
static bool g_schoolStrikeSwapTargetLeftPocket = true;
static glm::vec3 g_schoolStrikeSwapStartPositions[CoinLane::MAX_COINS] = {};
static glm::vec3 g_schoolStrikeSwapTargetPositions[CoinLane::MAX_COINS] = {};
static float g_schoolStrikeSwapStartDelays[CoinLane::MAX_COINS] = {};

static inline void School_StrikeLessonBuildLayout(
    UserContext *usr,
    bool aimLeftPocket,
    glm::vec3 outPositions[CoinLane::MAX_COINS],
    CoinState outStates[CoinLane::MAX_COINS],
    int *outActiveCount
)
{
    if (!usr || !outPositions || !outStates || !outActiveCount)
        return;

    const int n = 10;
    const float y = 0.20f;

    const glm::vec3 p1 = usr->initialPins[0];
    const glm::vec3 p2 = usr->initialPins[1];
    const glm::vec3 p3 = usr->initialPins[2];
    const glm::vec3 pocket = aimLeftPocket ? (p1 + p2) * 0.5f : (p1 + p3) * 0.5f;

    const float zEnd = pocket.z - 0.20f;
    const float zStart = -16.0f + 0.60f;
    const float bowAmp = aimLeftPocket ? -0.22f : 0.22f;

    *outActiveCount = n;
    for (int i = 0; i < n; i++)
    {
        if (i < 3)
        {
            outStates[i] = CoinState::Dead;
            outPositions[i] = {0.0f, y, zStart};
            continue;
        }

        const float t = (n <= 1) ? 0.0f : (float)i / (float)(n - 1);
        const float z = glm::mix(zStart, zEnd, t);
        const float bow = sinf(glm::pi<float>() * t);
        const float x = glm::mix(0.0f, pocket.x, t) + bowAmp * bow;

        outPositions[i] = {x, y, z};
        outStates[i] = CoinState::Active;
    }
}

static inline void School_StrikeLessonSetupCoins(UserContext *usr, bool aimLeftPocket)
{
    if (!usr)
        return;
    // Lesson 5: gems form a bow that goes away from center, returns, and points into the pocket.
    // `aimLeftPocket=true` -> between pins 1 and 2 (negative X). false -> between pins 1 and 3 (positive X).
    usr->coinLane.visualKind = CollectableVisualKind::Gem;
    usr->coinLane.currentPattern = CoinPattern::Static;
    glm::vec3 positions[CoinLane::MAX_COINS] = {};
    CoinState states[CoinLane::MAX_COINS] = {};
    int activeCount = 0;
    School_StrikeLessonBuildLayout(usr, aimLeftPocket, positions, states, &activeCount);
    usr->coinLane.activeCount = activeCount;
    for (int i = 0; i < activeCount; i++)
    {
        Coin &c = usr->coinLane.coins[i];
        c.basePosition = positions[i];
        c.position = positions[i];
        c.visualKind = CollectableVisualKind::Gem;
        c.anchorIndex = -1;
        c.orbitXRadius = 0.0f;
        c.orbitZRadius = 0.0f;
        c.orbitSpeed = 0.0f;
        c.orbitPhase = 0.0f;
        c.orbitXSign = 1.0f;
        c.orbitZSign = 1.0f;
        c.phaseOffset = (float)i * 0.628f;
        c.rotation = 0.0f;
        c.scale = 1.0f;
        c.state = states[i];
        c.flyTriggered = false;
        c.updateTransform();
    }
    for (int i = activeCount; i < CoinLane::MAX_COINS; i++)
    {
        usr->coinLane.coins[i].state = CoinState::Dead;
        usr->coinLane.coins[i].flyTriggered = false;
        usr->coinLane.coins[i].visualKind = CollectableVisualKind::Coin;
        usr->coinLane.coins[i].anchorIndex = -1;
        usr->coinLane.coins[i].orbitXRadius = 0.0f;
        usr->coinLane.coins[i].orbitZRadius = 0.0f;
        usr->coinLane.coins[i].orbitSpeed = 0.0f;
        usr->coinLane.coins[i].orbitPhase = 0.0f;
        usr->coinLane.coins[i].orbitXSign = 1.0f;
        usr->coinLane.coins[i].orbitZSign = 1.0f;
    }
}

static inline void School_StrikeLessonStartSwap(UserContext *usr)
{
    if (!usr)
        return;
    g_schoolStrikeSwapTargetLeftPocket = !g_schoolStrikeAimLeftPocket;
    CoinState targetStates[CoinLane::MAX_COINS] = {};
    int targetActiveCount = 0;
    School_StrikeLessonBuildLayout(
        usr,
        g_schoolStrikeSwapTargetLeftPocket,
        g_schoolStrikeSwapTargetPositions,
        targetStates,
        &targetActiveCount
    );
    usr->coinLane.activeCount = targetActiveCount;
    float minZ = 0.0f;
    float maxZ = 0.0f;
    bool foundZ = false;
    for (int i = 0; i < CoinLane::MAX_COINS; i++)
    {
        g_schoolStrikeSwapStartPositions[i] = usr->coinLane.coins[i].position;
        if (i < targetActiveCount && targetStates[i] == CoinState::Active)
        {
            const float z = g_schoolStrikeSwapStartPositions[i].z;
            if (!foundZ)
            {
                minZ = maxZ = z;
                foundZ = true;
            }
            else
            {
                minZ = glm::min(minZ, z);
                maxZ = glm::max(maxZ, z);
            }
        }
    }
    for (int i = 0; i < CoinLane::MAX_COINS; i++)
    {
        if (i < targetActiveCount && targetStates[i] == CoinState::Active)
        {
            g_schoolStrikeSwapStartDelays[i] =
                School_StrikeSwapDelayForZ(
                    g_schoolStrikeSwapStartPositions[i].z,
                    minZ,
                    maxZ,
                    SCHOOL_STRIKE_SWAP_MAX_START_DELAY_S
                );
        }
        else
        {
            g_schoolStrikeSwapStartDelays[i] = SCHOOL_STRIKE_SWAP_MAX_START_DELAY_S;
        }
    }
    g_schoolStrikeSwapInProgress = true;
    g_schoolStrikeSwapElapsed = 0.0f;
}

static inline void School_StrikeLessonTickSwap(UserContext *usr, float dt)
{
    if (!usr || usr->gameMode != UserContext::GameMode::SCHOOL || usr->school.selectedLesson != 5)
        return;

    if (usr->coinLane.getRenderableCount() == 0 && !g_schoolStrikeSwapInProgress)
    {
        g_schoolStrikeSwapElapsed = SCHOOL_STRIKE_SWAP_INTERVAL_S;
        return;
    }

    if (g_schoolStrikeSwapInProgress)
    {
        g_schoolStrikeSwapElapsed += dt;
        for (int i = 0; i < usr->coinLane.activeCount; i++)
        {
            Coin &c = usr->coinLane.coins[i];
            float localT = glm::clamp(
                (g_schoolStrikeSwapElapsed - g_schoolStrikeSwapStartDelays[i]) / SCHOOL_STRIKE_SWAP_DURATION_S,
                0.0f,
                1.0f
            );
            float ease = localT * localT * (3.0f - 2.0f * localT);
            c.position = glm::mix(g_schoolStrikeSwapStartPositions[i], g_schoolStrikeSwapTargetPositions[i], ease);
            c.basePosition = c.position;
            c.updateTransform();
        }
        if (g_schoolStrikeSwapElapsed >= SCHOOL_STRIKE_SWAP_MAX_START_DELAY_S + SCHOOL_STRIKE_SWAP_DURATION_S)
        {
            g_schoolStrikeAimLeftPocket = g_schoolStrikeSwapTargetLeftPocket;
            School_StrikeLessonSetupCoins(usr, g_schoolStrikeAimLeftPocket);
            g_schoolStrikeSwapInProgress = false;
            g_schoolStrikeSwapElapsed = SCHOOL_STRIKE_SWAP_INTERVAL_S;
        }
        return;
    }

    g_schoolStrikeSwapElapsed -= dt;
    if (g_schoolStrikeSwapElapsed <= 0.0f)
    {
        School_StrikeLessonStartSwap(usr);
    }
}

// School Lesson 4 completion is triggered after the player closes the Oil window.
static bool g_schoolOilLessonCompletionPending = false;
static bool g_schoolOilStatusWasOpen = false;
// School Lesson 5 strike help: offer a new ball every N failed attempts.
static int g_schoolStrikeFailedAttempts = 0;
static bool g_schoolStrikeHelpPending = false;
static int g_schoolStrikeBallBeforeLesson = -1; // restore when leaving lesson 5
static uint32_t g_schoolStrikeRng = 0x12345678u;
static float g_schoolStrikeLaneRestitutionBase = -1.0f;
static bool g_schoolStrikeLaneRestitutionActive = false;
static float g_schoolStrikeArmImpulseBase = -1.0f;
static bool g_schoolStrikeArmImpulseActive = false;
// Restore normal-game coin lane state after leaving school (lesson 5 uses a special pattern).
static CoinLane g_coinLaneBeforeSchool = {};
static bool g_coinLaneBeforeSchoolValid = false;
static float g_clearedCoinsBeforeSchool = 0.0f;

struct SchoolStrikeDifficultyTuning
{
    // Every failed strike attempt increases lane bounciness by this multiplier to help the player.
    // Restored to baseline after 5 failed attempts (when offering a new ball), and when leaving lesson 5.
    static constexpr float FAIL_RESTITUTION_MUL = 1.15f;
    static constexpr float FAIL_ARM_IMPULSE_MUL = 1.5f;
};

static inline uint32_t School_StrikeRngNext()
{
    // LCG
    g_schoolStrikeRng = g_schoolStrikeRng * 1664525u + 1013904223u;
    return g_schoolStrikeRng;
}

// Screen shake + SFX on ball<->lane impacts (hot-reloadable: game.cpp-only).
// We intentionally do NOT rely on physics contact callbacks so you can tune it live.
struct LaneImpactTuning
{
    // Lane is around y=0; physics code also assumes "near lane" when ball center y <= ~0.15.
    static constexpr float CONTACT_CENTER_Y_MAX = 0.15f;
    // Ball must rise above this to count as "airborne" for the next impact (avoids resting-contact spam).
    static constexpr float AIRBORNE_CENTER_Y_MIN = 0.22f;
    static constexpr float MIN_DOWN_VY = 0.35f;
    static constexpr float COOLDOWN_S = 0.08f;

    // Shake mapping: E = m * c^2 (c = downward speed), then amp = clamp(E*k, 0..max).
    static constexpr float ENERGY_TO_SHAKE = 0.0025f;
    static constexpr float SHAKE_AMP_MAX_M = 0.06f;
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

// School_Enter/SelectLesson/coin setup now live in the module.

// Forward decl: implemented later in the file, but used by School_Exit.
void BallStats_OnBallChange(const CatalogItem *ball, UserContext *usr);

static inline void EnterSchool(UserContext *usr, bool playStory)
{
    if (!usr)
        return;

    // Backup coin lane so school lessons can freely override patterns, and we can restore on exit.
    std::memcpy(&g_coinLaneBeforeSchool, &usr->coinLane, sizeof(CoinLane));
    g_clearedCoinsBeforeSchool = usr->clearedCoins;
    g_coinLaneBeforeSchoolValid = true;
    usr->electroBall.resetCharge();

    usr->school.ballIdBeforeSchool = usr->myBall.id;
    usr->gameMode = UserContext::GameMode::SCHOOL;

    SchoolServices svc = {};
    svc.phy = &usr->phy;
    svc.coinLane = &usr->coinLane;
    svc.dialog = (usr->windowStack.count == 0 && !usr->dialog.active) ? &usr->dialog : nullptr;
    svc.myBall = &usr->myBall;
    svc.ballStatsLightnessBuff = BallStats_LightnessBuff;
    svc.ballStatsRestitutionMassScale = BallStats_RestitutionMassScale;
    svc.remapClamped = remapClamped;
    svc.catalogBuffMin = BallPhysicsMapping::CATALOG_BUFF_MIN;
    svc.catalogBuffMax = BallPhysicsMapping::CATALOG_BUFF_MAX;
    svc.physicsArmImpulseMin = BallPhysicsMapping::PHYSICS_ARM_IMPULSE_MIN;
    svc.physicsArmImpulseMax = BallPhysicsMapping::PHYSICS_ARM_IMPULSE_MAX;

    SchoolRuntimeTuning rt = {};
    rt.desiredMassKg = &usr->desiredMass;
    rt.lightnessBuff = &usr->lightnessBuff;
    rt.launchBuffEffective = &usr->launchBuffEffective;
    rt.armImpulseAtThrow = &usr->armImpulseAtThrow;
    rt.angularFactor = &usr->angularFactor;
    rt.ballSkid = &usr->ballSkid;
    rt.ballSkidStartScale = &usr->ballSkidStartScale;
    rt.ballBaseFriction = &usr->ballBaseFriction;
    rt.laneOilThickness = &usr->laneOilThickness;
    rt.ballRestitution = &usr->ballRestitution;

    School_Enter(&usr->school, svc, rt, /*playStory=*/playStory);
    School_ApplyPinModeForSelectedLesson(usr);
    if (usr->school.selectedLesson == 4)
        School_ApplyOilLessonDefaults(usr);
    else if (usr->school.selectedLesson == 5)
        School_ApplyStrikeLaneDefaults(usr);
    else
        School_ApplyNeutralLaneDefaults(usr);

    // Leave any RESULT flow back to gameplay.
    usr->phase = UserContext::Phase::IDLE;
    usr->clayton.shouldShowHiScore = false;
    usr->clayton.shouldShowHiScoreWithLatest = false;
    resetScoreboard(&usr->board);
    usr->wereDead = 0;
    PhysicsResetForMode(usr, /*reviveAll=*/true);
}

static void School_Exit(UserContext *usr)
{
    // Leaving school:
    // - If all lessons are completed AND the 100-point milestone is reached, graduate into BOT mode.
    // - Otherwise, return to SOLO.
    bool graduated = true;
    for (int i = 0; i < 5; i++)
        graduated = graduated && usr->school.lessonDone[i];

    if (graduated && usr->milestone100Reached)
    {
        usr->schoolDone = true;
        usr->storage.setChar(Storage::SCHOOL_DONE, "1", 1);
        usr->gameMode = UserContext::GameMode::BOT;

        // Reset vs state so the next game starts cleanly.
        usr->turnOwner = UserContext::TurnOwner::PLAYER;
        usr->enemyAutoTimer = 0.0f;
        usr->enemyLaunched = false;
        usr->enemyDebugLogged = false;
        usr->enemyTurnSetup = false;
        resetScoreboard(&usr->enemyBoard);
    }
    else
    {
        usr->gameMode = UserContext::GameMode::SOLO;
    }

    // Once the player graduates school, don't keep exit locked.
    if (graduated)
        usr->schoolExitLocked = false;
    // Restore the ball selection and its catalog-driven mass/stats after leaving school.
    if (usr->school.ballIdBeforeSchool >= 0)
        BallStats_OnBallChange(&g_ballCatalog[usr->school.ballIdBeforeSchool], usr);
    // If we were in strike lesson with boosted restitution, restore baseline.
    if (g_schoolStrikeLaneRestitutionActive && g_schoolStrikeLaneRestitutionBase >= 0.0f)
    {
        usr->laneRestitution = glm::clamp(g_schoolStrikeLaneRestitutionBase, 0.0f, 1.0f);
    }
    g_schoolStrikeLaneRestitutionActive = false;
    if (g_schoolStrikeArmImpulseActive && g_schoolStrikeArmImpulseBase > 0.0f)
    {
        usr->armImpulseAtThrow = glm::clamp(
            g_schoolStrikeArmImpulseBase,
            BallPhysicsMapping::PHYSICS_ARM_IMPULSE_MIN,
            BallPhysicsMapping::PHYSICS_ARM_IMPULSE_MAX
        );
    }
    g_schoolStrikeArmImpulseActive = false;
    g_schoolStrikeBallBeforeLesson = -1;
    g_schoolStrikeFailedAttempts = 0;
    g_schoolStrikeHelpPending = false;
    g_schoolStrikeSwapInProgress = false;
    g_schoolStrikeSwapElapsed = SCHOOL_STRIKE_SWAP_INTERVAL_S;
    g_schoolStrikeSwapTargetLeftPocket = true;

    // Restore coin lane to whatever it was before entering school.
    if (g_coinLaneBeforeSchoolValid)
    {
        std::memcpy(&usr->coinLane, &g_coinLaneBeforeSchool, sizeof(CoinLane));
        usr->clearedCoins = g_clearedCoinsBeforeSchool;
        g_coinLaneBeforeSchoolValid = false;
    }

    resetScoreboard(&usr->board);
    usr->wereDead = 0;
    usr->phase = UserContext::Phase::IDLE;
    PhysicsResetForMode(usr, /*reviveAll=*/true);
}

static inline void Tracker_LoadPatchFromSound(UserContext *usr, int instrument);
static inline void Tracker_LoadUsedPatchesFromSound(UserContext *usr);
static inline void Tracker_EnsureUserSongForEdit(UserContext *usr);
static inline void Tracker_ApplyPatternToSound(UserContext *usr);
static inline void Tracker_ApplyPatchEditsToSound(UserContext *usr);
static inline void Tracker_ApplyRealtimeLfoToSound(UserContext *usr);

static inline void EnterTracker(UserContext *usr)
{
    if (!usr)
        return;

    if (usr->gameMode != UserContext::GameMode::TRACKER &&
        usr->gameMode != UserContext::GameMode::SCHOOL)
    {
        usr->trackerReturnMode = usr->gameMode;
    }
    usr->gameMode = UserContext::GameMode::TRACKER;
    usr->phase = UserContext::Phase::IDLE;
    usr->shouldShowShop = false;
    usr->clayton.shouldShowHiScore = false;
    usr->clayton.shouldShowHiScoreWithLatest = false;
    usr->clayton.shouldShowOilStatus = false;
    usr->clayton.shouldShowHouses = false;
    usr->clayton.shouldShowBotSelect = false;
    usr->clayton.shouldShowSettings = false;
    usr->windowStack.count = 0;
    SDL_SetRelativeMouseMode(SDL_FALSE);
    setTrackerPatternState(
        &usr->tracker,
        usr->sound.currentSongIndex,
        usr->sound.getSongPattern(usr->sound.currentSongIndex),
        usr->sound.getSongName(usr->sound.currentSongIndex)
    );
    usr->tracker.playing =
        (!usr->sound.useWavPlayback && usr->sound.musicModule && usr->sound.musicModule->active_song.active) ||
        (usr->sound.useWavPlayback && usr->sound.wavMusicModule && xfm_wav_song_is_playing(usr->sound.wavMusicModule));
    Tracker_LoadUsedPatchesFromSound(usr);
    Tracker_Open(&usr->tracker);
}

static inline void ExitTracker(UserContext *usr)
{
    if (!usr)
        return;

    Tracker_ApplyPatternToSound(usr);
    Tracker_ApplyPatchEditsToSound(usr);
    Tracker_ApplyRealtimeLfoToSound(usr);
    Tracker_Close(&usr->tracker);
    usr->gameMode = usr->trackerReturnMode;
    usr->phase = UserContext::Phase::IDLE;
}

static inline int Tracker_DefaultTicksPerRowForSong(int songIndex)
{
    return songIndex == 2 ? 8 : 6;
}

static inline void Tracker_SyncCursorFromSound(UserContext *usr)
{
    if (!usr || usr->gameMode != UserContext::GameMode::TRACKER || !usr->tracker.active)
        return;

    if (usr->tracker.songIndex != usr->sound.currentSongIndex &&
        !usr->tracker.patternDirty &&
        !usr->tracker.copyOnWriteRequested)
        setTrackerPatternState(
            &usr->tracker,
            usr->sound.currentSongIndex,
            usr->sound.getSongPattern(usr->sound.currentSongIndex),
            usr->sound.getSongName(usr->sound.currentSongIndex)
        );

    int row = 0;
    int tick = 0;
    int ticksPerRow = Tracker_DefaultTicksPerRowForSong(usr->sound.currentSongIndex);
    if (!usr->sound.useWavPlayback && usr->sound.musicModule)
    {
        xfm_module *m = usr->sound.musicModule;
        const int songId = usr->sound.currentSongIndex;
        if (songId >= 0 && songId < 16 && m->song_present[songId])
        {
            XfmSongPattern &pat = m->song_patterns[songId];
            ticksPerRow = std::max(1, pat.speed);
            row = m->active_song.current_row;
            const int samplesPerTick = std::max(1, pat.samples_per_row / ticksPerRow);
            tick = m->active_song.sample_in_row / samplesPerTick;
        }
    }
    else if (usr->sound.wavMusicModule)
    {
        row = xfm_wav_song_get_row(usr->sound.wavMusicModule);
    }
    setTrackerCursorState(&usr->tracker, Tracker_SongRowForPlaybackRow(&usr->tracker, row), tick, ticksPerRow);
}

static inline TrackerOscilloscopeSnapshot Tracker_BuildOscilloscopeSnapshotFromSound(GameSoundSystem *sound)
{
    TrackerOscilloscopeSnapshot snapshot = {};
    if (!sound)
        return snapshot;

    snapshot.sampleRate = Sound_PreferredAudioSampleRate(*sound);
    uint32_t write = sound->oscilloscopeWriteIndex.load(std::memory_order_acquire);
    uint64_t cursor = sound->oscilloscopeSampleCursor.load(std::memory_order_acquire);
    for (int ch = 0; ch < TRACKER_OSC_CHANNELS; ch++)
    {
        snapshot.channels[ch].ring = sound->oscilloscopeRing[ch];
        snapshot.channels[ch].ringSize = TRACKER_OSC_RING_SIZE;
        snapshot.channels[ch].writeIndex = write;
        snapshot.channels[ch].sampleCursor = cursor;
        snapshot.channels[ch].noteStartSample = sound->oscilloscopeNoteStartSample[ch].load(std::memory_order_relaxed);
        snapshot.channels[ch].fnum = sound->oscilloscopeFnum[ch].load(std::memory_order_relaxed);
        snapshot.channels[ch].block = sound->oscilloscopeBlock[ch].load(std::memory_order_relaxed);
        snapshot.channels[ch].keyOn = sound->oscilloscopeKeyOn[ch].load(std::memory_order_relaxed);
    }
    return snapshot;
}

static inline void Tracker_UpdateOscilloscopeTexture(UserContext *usr)
{
    if (!usr || !usr->tracker.active || !usr->tracker.oscilloscopeVisible)
        return;
    if ((int)usr->trackerOscilloscopePixels.size() != TRACKER_OSC_ATLAS_WIDTH * TRACKER_OSC_ATLAS_HEIGHT)
        usr->trackerOscilloscopePixels.resize(TRACKER_OSC_ATLAS_WIDTH * TRACKER_OSC_ATLAS_HEIGHT);

    TrackerOscilloscopeSnapshot snapshot = Tracker_BuildOscilloscopeSnapshotFromSound(&usr->sound);
    TrackerOscilloscope_DrawAtlas(
        usr->trackerOscilloscopePixels.data(),
        TRACKER_OSC_ATLAS_WIDTH,
        TRACKER_OSC_ATLAS_HEIGHT,
        snapshot
    );

    glBindTexture(GL_TEXTURE_2D, usr->trackerOscilloscopeTex.colorTexture);
    glTexSubImage2D(
        GL_TEXTURE_2D,
        0,
        0,
        0,
        TRACKER_OSC_ATLAS_WIDTH,
        TRACKER_OSC_ATLAS_HEIGHT,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        usr->trackerOscilloscopePixels.data()
    );
}

static inline bool Tracker_ShouldUseSelectionPlaybackOverride(const Tracker *tracker);
static inline int Tracker_LivePlaybackRowFromSongRow(const Tracker *tracker, int songRow, bool selectionOverrideActive);
static inline const char *Tracker_SelectLivePlaybackPattern(
    UserContext *usr,
    bool *outSelectionOverrideActive,
    int *outLoopStartRow,
    int *outLoopEndRow);

static inline void Tracker_ApplyLoopRangeToSound(UserContext *usr)
{
    if (!usr || usr->gameMode != UserContext::GameMode::TRACKER || !usr->tracker.active)
        return;
    if (!usr->tracker.loopRangeDirty)
        return;

    usr->tracker.loopRangeDirty = false;
    if (!usr->sound.useWavPlayback && !usr->sound.audioDisabled && usr->sound.musicModule)
    {
        const bool useSelectionOverride = Tracker_ShouldUseSelectionPlaybackOverride(&usr->tracker);
        if (useSelectionOverride || usr->trackerSelectionPlaybackOverrideActive)
        {
            bool overrideActive = false;
            int loopStart = 0;
            int loopEnd = 0;
            const char *pattern = Tracker_SelectLivePlaybackPattern(usr, &overrideActive, &loopStart, &loopEnd);
            usr->trackerSelectionPlaybackOverrideActive = overrideActive;
            if (!pattern || !pattern[0])
                return;

            const int tickRate = std::max(1, usr->tracker.songTickRate);
            const int ticksPerRow = std::max(1, usr->tracker.songSpeed);
            const int startRow = usr->tracker.loopEnabled ?
                Tracker_LivePlaybackRowFromSongRow(&usr->tracker, usr->tracker.loopStart, overrideActive) :
                Tracker_LivePlaybackRowFromSongRow(&usr->tracker, usr->tracker.playRow, overrideActive);
            SDL_LockAudioDevice(usr->sound.audioDev);
            xfm_module_set_lfo(usr->sound.musicModule, usr->tracker.songLfoEnabled, usr->tracker.songLfoFrequency);
            xfm_song_declare(usr->sound.musicModule, usr->sound.currentSongIndex, pattern, tickRate, ticksPerRow);
            xfm_song_set_loop_range(usr->sound.musicModule, loopStart, loopEnd);
            if (usr->tracker.playing)
            {
                xfm_song_play(usr->sound.musicModule, usr->sound.currentSongIndex, true);
                xfm_song_jump_to_row(usr->sound.musicModule, startRow);
            }
            SDL_UnlockAudioDevice(usr->sound.audioDev);
        }
        else
        {
            usr->trackerSelectionPlaybackOverrideActive = false;
            if (usr->tracker.loopEnabled)
            {
                int loopStart = 0;
                int loopEnd = Tracker_PlaybackRowCount(&usr->tracker) - 1;
                (void)Tracker_PlaybackLoopRangeForSongRange(&usr->tracker, usr->tracker.loopStart, usr->tracker.loopEnd, &loopStart, &loopEnd);
                usr->sound.setMusicLoopRange(loopStart, loopEnd);
            }
            else
                usr->sound.clearMusicLoopRange();
        }
    }
}

static inline void Tracker_LoadPatchFromSound(UserContext *usr, int instrument)
{
    if (!usr || usr->sound.useWavPlayback || usr->sound.audioDisabled || !usr->sound.musicModule)
        return;
    int inst = std::max(0, std::min(255, instrument));
    xfm_module *module = usr->sound.musicModule;
    if (!module->patch_present[inst])
        return;
    usr->tracker.editPatches[inst] = module->patches[inst];
    usr->tracker.editPatchValid[inst] = true;
    usr->tracker.editPatchDirty[inst] = false;
    for (int target = XFM_MACRO_TL1; target < XFM_MACRO_TARGET_COUNT; target++)
    {
        int macroId = module->patch_macros[inst][target];
        if (macroId >= 0 && macroId < XFM_MAX_MACROS && module->macro_present[macroId] &&
            module->macros[macroId].target == target)
        {
            usr->tracker.editMacros[inst][target] = module->macros[macroId];
            usr->tracker.editMacroEnabled[inst][target] = true;
            usr->tracker.editMacroValid[inst][target] = true;
            usr->tracker.editMacroDirty[inst][target] = false;
        }
        else if (!usr->tracker.editMacroDirty[inst][target])
        {
            usr->tracker.editMacroEnabled[inst][target] = false;
        }
    }
}

static inline void Tracker_LoadUsedPatchesFromSound(UserContext *usr)
{
    if (!usr)
        return;
    if (usr->sound.useWavPlayback || usr->sound.audioDisabled || !usr->sound.musicModule || !usr->sound.audioDev)
        return;

    xfm_module *module = usr->sound.musicModule;
    for (int inst = 0; inst < 256; inst++)
    {
        if (!module->patch_present[inst])
            continue;
        Tracker_SetInstrumentAvailable(&usr->tracker, inst);
        Tracker_LoadPatchFromSound(usr, inst);
    }
}

static inline void Tracker_ApplyRealtimeLfoToSound(UserContext *usr)
{
    if (!usr || usr->gameMode != UserContext::GameMode::TRACKER || !usr->tracker.active)
        return;
    if (usr->sound.useWavPlayback || usr->sound.audioDisabled || !usr->sound.musicModule || !usr->sound.audioDev)
        return;

    xfm_module *module = usr->sound.musicModule;
    if (module->lfo_enable == usr->tracker.songLfoEnabled &&
        module->lfo_freq == usr->tracker.songLfoFrequency)
        return;

    SDL_LockAudioDevice(usr->sound.audioDev);
    xfm_module_set_lfo(module, usr->tracker.songLfoEnabled, usr->tracker.songLfoFrequency);
    SDL_UnlockAudioDevice(usr->sound.audioDev);
}

static inline void Tracker_RefreshHeldPreviewPatch(UserContext *usr)
{
    if (!usr || usr->sound.useWavPlayback || usr->sound.audioDisabled || !usr->sound.sfxModule)
        return;
    if (usr->sound.trackerPreviewVoice == FM_VOICE_INVALID)
        return;

    const int inst = std::max(0, std::min(255, usr->tracker.editInstrument));
    if (!usr->tracker.editPatchValid[inst])
        return;

    constexpr int previewInstrument = 0xEB;
    xfm_patch_opn previewPatch = usr->tracker.editPatches[inst];
    const int safeVolume = std::max(0, std::min(127, usr->tracker.editVolume));
    const int tlAdd = ((0x7F - safeVolume) * 127) / 0x7F;
    for (int op = 0; op < 4; op++)
        previewPatch.op[op].TL = (uint8_t)std::min(127, (int)previewPatch.op[op].TL + tlAdd);

    xfm_patch_set(
        usr->sound.sfxModule,
        previewInstrument,
        &previewPatch,
        sizeof(xfm_patch_opn),
        XFM_CHIP_YM3438
    );
    xfm_patch_refresh_live(usr->sound.sfxModule, previewInstrument);
}

static inline void Tracker_ApplyPatchEditsToSound(UserContext *usr)
{
    if (!usr || usr->gameMode != UserContext::GameMode::TRACKER || !usr->tracker.active)
        return;
    Tracker_EnsureUserSongForEdit(usr);
    if (usr->sound.useWavPlayback || usr->sound.audioDisabled || !usr->sound.musicModule)
        return;

    if (usr->sound.trackerNeedsFullPatchSync)
    {
        // After a browser suspend/resume or sound system reinit, modules can lose patches/macros.
        // Force-push everything the tracker already knows about.
        Tracker_MarkAllAvailablePatchesAndMacrosDirty(&usr->tracker);
    }

    bool anyPatchDirty = false;
    bool anyMacroDirty = false;
    for (int inst = 0; inst < 256; inst++)
    {
        if (usr->tracker.editPatchDirty[inst])
        {
            anyPatchDirty = true;
            break;
        }
    }
    for (int inst = 0; inst < 256 && !anyMacroDirty; inst++)
    {
        for (int target = XFM_MACRO_TL1; target < XFM_MACRO_TARGET_COUNT; target++)
        {
            if (usr->tracker.editMacroDirty[inst][target])
            {
                anyMacroDirty = true;
                break;
            }
        }
    }
    if (!anyPatchDirty && !anyMacroDirty)
        return;

    if (usr->sound.audioDev)
        SDL_LockAudioDevice(usr->sound.audioDev);
    bool previewPatchNeedsRefresh = false;
    if (anyPatchDirty)
    {
        for (int inst = 0; inst < 256; inst++)
        {
            if (!usr->tracker.editPatchDirty[inst])
                continue;
            if (!usr->tracker.availableInstruments[inst] || !usr->tracker.editPatchValid[inst])
            {
                usr->sound.musicModule->patch_present[inst] = false;
                usr->tracker.editPatchDirty[inst] = false;
                continue;
            }
            xfm_patch_set(
                usr->sound.musicModule,
                inst,
                &usr->tracker.editPatches[inst],
                sizeof(xfm_patch_opn),
                XFM_CHIP_YM3438
            );
            xfm_patch_refresh_live(usr->sound.musicModule, inst);
            if (inst == std::max(0, std::min(255, usr->tracker.editInstrument)))
                previewPatchNeedsRefresh = true;
            usr->tracker.editPatchDirty[inst] = false;
        }
    }
    if (anyMacroDirty)
    {
        int nextMacroId = 0;
        for (int inst = 0; inst < 256; inst++)
        {
            bool knownInst = false;
            for (int target = XFM_MACRO_TL1; target < XFM_MACRO_TARGET_COUNT; target++)
            {
                if (usr->tracker.editMacroValid[inst][target] || usr->tracker.editMacroDirty[inst][target])
                {
                    knownInst = true;
                    break;
                }
            }
            if (!knownInst)
                continue;

            xfm_patch_macro_clear(usr->sound.musicModule, inst, XFM_MACRO_NONE);
            for (int target = XFM_MACRO_TL1; target < XFM_MACRO_TARGET_COUNT; target++)
            {
                if (!usr->tracker.editMacroEnabled[inst][target] || !usr->tracker.editMacroValid[inst][target])
                    continue;
                if (nextMacroId >= XFM_MAX_MACROS)
                    break;
                XfmMacro macro = usr->tracker.editMacros[inst][target];
                macro.target = (uint8_t)target;
                Tracker_NormalizeMacroUiState(&macro);
                if (macro.length == 0)
                    continue;
                if (xfm_macro_set(usr->sound.musicModule, nextMacroId, &macro) >= 0)
                {
                    xfm_patch_macro_set(usr->sound.musicModule, inst, (uint8_t)target, nextMacroId);
                    nextMacroId++;
                }
            }
            for (int target = XFM_MACRO_TL1; target < XFM_MACRO_TARGET_COUNT; target++)
                usr->tracker.editMacroDirty[inst][target] = false;
        }
    }
    if (previewPatchNeedsRefresh)
        Tracker_RefreshHeldPreviewPatch(usr);
    if (usr->sound.audioDev)
        SDL_UnlockAudioDevice(usr->sound.audioDev);

    usr->sound.trackerNeedsFullPatchSync = false;
}

static inline std::string Tracker_BuildPatternText(const Tracker *tracker)
{
    return Tracker_BuildPartPatternText(tracker);
}

static inline std::string Tracker_BuildPlaybackPatternText(const Tracker *tracker)
{
    return Tracker_BuildFlatPatternText(tracker);
}

static inline bool Tracker_ShouldUseSelectionPlaybackOverride(const Tracker *tracker)
{
    return tracker &&
        tracker->loopEnabled &&
        Tracker_SongRangeTouchesSkippedPart(tracker, tracker->loopStart, tracker->loopEnd);
}

static inline int Tracker_LivePlaybackRowFromSongRow(const Tracker *tracker, int songRow, bool selectionOverrideActive)
{
    if (!tracker || tracker->rowCount <= 0)
        return 0;
    songRow = std::max(0, std::min(tracker->rowCount - 1, songRow));
    if (selectionOverrideActive && tracker->loopEnabled)
        return std::max(0, std::min(tracker->loopEnd - tracker->loopStart, songRow - tracker->loopStart));
    return Tracker_PlaybackRowForSongRow(tracker, songRow);
}

static inline const char *Tracker_SelectLivePlaybackPattern(
    UserContext *usr,
    bool *outSelectionOverrideActive,
    int *outLoopStartRow,
    int *outLoopEndRow)
{
    if (!usr)
        return "";

    const bool wantsChannelSolo = usr->tracker.channelSelectionEnabled;
    const bool useSelectionOverride = Tracker_ShouldUseSelectionPlaybackOverride(&usr->tracker);
    const int playbackRowCount = std::max(1, Tracker_PlaybackRowCount(&usr->tracker));
    if (outSelectionOverrideActive) *outSelectionOverrideActive = useSelectionOverride;

    if (useSelectionOverride)
    {
        usr->trackerSelectionPlaybackPattern = Tracker_BuildSongRangePatternText(
            &usr->tracker,
            usr->tracker.loopStart,
            usr->tracker.loopEnd,
            wantsChannelSolo,
            usr->tracker.channelStart,
            usr->tracker.channelEnd
        );
        const int selectionRows = std::max(1, usr->tracker.loopEnd - usr->tracker.loopStart + 1);
        if (outLoopStartRow) *outLoopStartRow = 0;
        if (outLoopEndRow) *outLoopEndRow = selectionRows - 1;
        return usr->trackerSelectionPlaybackPattern.c_str();
    }

    usr->trackerSelectionPlaybackPattern.clear();
    if (wantsChannelSolo)
    {
        usr->trackerChannelSoloPattern = Tracker_BuildFlatPatternText(
            &usr->tracker,
            true,
            usr->tracker.channelStart,
            usr->tracker.channelEnd
        );
        if (usr->tracker.loopEnabled)
        {
            int loopStart = 0;
            int loopEnd = playbackRowCount - 1;
            (void)Tracker_PlaybackLoopRangeForSongRange(&usr->tracker, usr->tracker.loopStart, usr->tracker.loopEnd, &loopStart, &loopEnd);
            if (outLoopStartRow) *outLoopStartRow = loopStart;
            if (outLoopEndRow) *outLoopEndRow = loopEnd;
        }
        else
        {
            if (outLoopStartRow) *outLoopStartRow = 0;
            if (outLoopEndRow) *outLoopEndRow = playbackRowCount - 1;
        }
        return usr->trackerChannelSoloPattern.c_str();
    }

    usr->trackerChannelSoloPattern = Tracker_BuildPlaybackPatternText(&usr->tracker);
    if (usr->tracker.loopEnabled)
    {
        int loopStart = 0;
        int loopEnd = playbackRowCount - 1;
        (void)Tracker_PlaybackLoopRangeForSongRange(&usr->tracker, usr->tracker.loopStart, usr->tracker.loopEnd, &loopStart, &loopEnd);
        if (outLoopStartRow) *outLoopStartRow = loopStart;
        if (outLoopEndRow) *outLoopEndRow = loopEnd;
    }
    else
    {
        if (outLoopStartRow) *outLoopStartRow = 0;
        if (outLoopEndRow) *outLoopEndRow = playbackRowCount - 1;
    }
    return usr->trackerChannelSoloPattern.c_str();
}

static inline std::string Tracker_DefaultUserSongDisplayName()
{
    time_t t = time(nullptr);
    struct tm tmv {};
#if defined(_WIN32)
    localtime_s(&tmv, &t);
#else
    localtime_r(&t, &tmv);
#endif
    return TrackerSongIO_StemToDisplay(TrackerSongIO_DefaultDateStem(tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday));
}

static inline void Tracker_UpdateSoundSettingsSongNames(UserContext *usr)
{
    if (!usr) return;
    initSoundSettings(&usr->clayton, &usr->sound.settings, &usr->sound);
}

static inline void Tracker_LoadEmptyUserSong(UserContext *usr)
{
    if (!usr) return;

    const std::string uiPattern = "32\nPART 1\n";
    const std::string emptyDisplayName = "Empty Song";

    setTrackerPatternState(
        &usr->trackerLoadScratch,
        TRACKER_USER_SONG_SLOT,
        uiPattern.c_str(),
        emptyDisplayName.c_str()
    );
    std::string playbackPattern = Tracker_BuildPlaybackPatternText(&usr->trackerLoadScratch);

    usr->sound.setUserSong(emptyDisplayName.c_str(), uiPattern.c_str(), playbackPattern.c_str());
    usr->sound.currentSongIndex = TRACKER_USER_SONG_SLOT;

    setTrackerPatternState(
        &usr->tracker,
        TRACKER_USER_SONG_SLOT,
        uiPattern.c_str(),
        usr->sound.userSongName
    );
    Tracker_ClearInstrumentState(&usr->tracker, true);
    Tracker_EnsureDefaultInstrument(&usr->tracker, true);
    Tracker_PrepareClipboardForSong(&usr->tracker);

    if (!usr->sound.useWavPlayback && !usr->sound.audioDisabled && usr->sound.musicModule)
    {
        SDL_LockAudioDevice(usr->sound.audioDev);
        xfm_song_declare(
            usr->sound.musicModule,
            TRACKER_USER_SONG_SLOT,
            playbackPattern.c_str(),
            usr->tracker.songTickRate,
            usr->tracker.songSpeed
        );
        xfm_song_play(usr->sound.musicModule, TRACKER_USER_SONG_SLOT, true);
        SDL_UnlockAudioDevice(usr->sound.audioDev);
    }

    usr->tracker.patternDirty = false;
    usr->tracker.copyOnWriteRequested = false;
    usr->tracker.songLoadEmptyRequested = false;
    Tracker_UpdateSoundSettingsSongNames(usr);
}

static inline void Tracker_EnsureUserSongForEdit(UserContext *usr)
{
    if (!usr || !usr->tracker.copyOnWriteRequested || usr->sound.currentSongIndex == TRACKER_USER_SONG_SLOT)
        return;
    bool loopEnabled = usr->tracker.loopEnabled;
    int loopStart = usr->tracker.loopStart;
    int loopEnd = usr->tracker.loopEnd;
    bool channelSelectionEnabled = usr->tracker.channelSelectionEnabled;
    int channelStart = usr->tracker.channelStart;
    int channelEnd = usr->tracker.channelEnd;
    int songTickRate = usr->tracker.songTickRate;
    int songSpeed = usr->tracker.songSpeed;
    int songRowsPerBeat = usr->tracker.songRowsPerBeat;
    bool songLfoEnabled = usr->tracker.songLfoEnabled;
    int songLfoFrequency = usr->tracker.songLfoFrequency;
    std::string uiPattern = Tracker_BuildPatternText(&usr->tracker);
    std::string pattern = Tracker_BuildPlaybackPatternText(&usr->tracker);
    std::string displayName = Tracker_DefaultUserSongDisplayName();
    usr->sound.setUserSong(displayName.c_str(), uiPattern.c_str(), pattern.c_str());
    usr->sound.currentSongIndex = TRACKER_USER_SONG_SLOT;
    usr->tracker.songIndex = TRACKER_USER_SONG_SLOT;
    std::snprintf(usr->tracker.songDisplayName, sizeof(usr->tracker.songDisplayName), "%s", usr->sound.userSongName);
    usr->tracker.loopEnabled = loopEnabled;
    usr->tracker.loopStart = std::max(0, std::min(loopStart, std::max(0, usr->tracker.rowCount - 1)));
    usr->tracker.loopEnd = std::max(usr->tracker.loopStart, std::min(loopEnd, std::max(0, usr->tracker.rowCount - 1)));
    usr->tracker.channelSelectionEnabled = channelSelectionEnabled;
    usr->tracker.channelStart = std::max(0, std::min(channelStart, TRACKER_CHANNELS - 1));
    usr->tracker.channelEnd = std::max(usr->tracker.channelStart, std::min(channelEnd, TRACKER_CHANNELS - 1));
    usr->tracker.songTickRate = songTickRate;
    usr->tracker.songSpeed = songSpeed;
    usr->tracker.ticksPerRow = songSpeed;
    usr->tracker.songRowsPerBeat = songRowsPerBeat;
    usr->tracker.songLfoEnabled = songLfoEnabled;
    usr->tracker.songLfoFrequency = songLfoFrequency;
    usr->tracker.loopRangeDirty = true;
    Tracker_UpdateSoundSettingsSongNames(usr);
    if (!usr->sound.useWavPlayback && !usr->sound.audioDisabled && usr->sound.musicModule)
    {
        SDL_LockAudioDevice(usr->sound.audioDev);
        xfm_module_set_lfo(usr->sound.musicModule, usr->tracker.songLfoEnabled, usr->tracker.songLfoFrequency);
        xfm_song_declare(
            usr->sound.musicModule,
            TRACKER_USER_SONG_SLOT,
            pattern.c_str(),
            usr->tracker.songTickRate,
            usr->tracker.songSpeed
        );
        xfm_song_play(usr->sound.musicModule, TRACKER_USER_SONG_SLOT, true);
        SDL_UnlockAudioDevice(usr->sound.audioDev);
    }
}

static inline bool Tracker_CommitPatternToUserSong(UserContext *usr)
{
    if (!usr)
        return false;
    if (!usr->tracker.copyOnWriteRequested &&
        !(usr->sound.currentSongIndex == TRACKER_USER_SONG_SLOT && usr->tracker.patternDirty))
        return false;

    bool loopEnabled = usr->tracker.loopEnabled;
    int loopStart = usr->tracker.loopStart;
    int loopEnd = usr->tracker.loopEnd;
    bool channelSelectionEnabled = usr->tracker.channelSelectionEnabled;
    int channelStart = usr->tracker.channelStart;
    int channelEnd = usr->tracker.channelEnd;
    int songTickRate = usr->tracker.songTickRate;
    int songSpeed = usr->tracker.songSpeed;
    int songRowsPerBeat = usr->tracker.songRowsPerBeat;
    bool songLfoEnabled = usr->tracker.songLfoEnabled;
    int songLfoFrequency = usr->tracker.songLfoFrequency;
    bool playing = usr->tracker.playing;
    float scrollY = usr->tracker.scrollY;
    std::string uiPattern = Tracker_BuildPatternText(&usr->tracker);
    std::string pattern = Tracker_BuildPlaybackPatternText(&usr->tracker);
    std::string displayName = usr->sound.currentSongIndex == TRACKER_USER_SONG_SLOT ?
        usr->tracker.songDisplayName : Tracker_DefaultUserSongDisplayName();

    usr->sound.setUserSong(displayName.c_str(), uiPattern.c_str(), pattern.c_str());
    usr->sound.currentSongIndex = TRACKER_USER_SONG_SLOT;
    usr->tracker.songIndex = TRACKER_USER_SONG_SLOT;
    std::snprintf(usr->tracker.songDisplayName, sizeof(usr->tracker.songDisplayName), "%s", usr->sound.userSongName);

    usr->tracker.loopEnabled = loopEnabled;
    usr->tracker.loopStart = std::max(0, std::min(loopStart, std::max(0, usr->tracker.rowCount - 1)));
    usr->tracker.loopEnd = std::max(usr->tracker.loopStart, std::min(loopEnd, std::max(0, usr->tracker.rowCount - 1)));
    usr->tracker.channelSelectionEnabled = channelSelectionEnabled;
    usr->tracker.channelStart = std::max(0, std::min(channelStart, TRACKER_CHANNELS - 1));
    usr->tracker.channelEnd = std::max(usr->tracker.channelStart, std::min(channelEnd, TRACKER_CHANNELS - 1));
    usr->tracker.songTickRate = songTickRate;
    usr->tracker.songSpeed = songSpeed;
    usr->tracker.ticksPerRow = songSpeed;
    usr->tracker.songRowsPerBeat = songRowsPerBeat;
    usr->tracker.songLfoEnabled = songLfoEnabled;
    usr->tracker.songLfoFrequency = songLfoFrequency;
    usr->tracker.playing = playing;
    usr->tracker.scrollY = scrollY;
    usr->tracker.loopRangeDirty = true;
    usr->tracker.copyOnWriteRequested = false;
    Tracker_UpdateSoundSettingsSongNames(usr);
    return true;
}

static inline void Tracker_OpenInstrumentNameKeypadIfRequested(UserContext *usr)
{
    if (!usr || usr->tracker.pendingInstrumentAction == 0)
        return;
    if (usr->tracker.pendingInstrumentKeypadOpen)
    {
        if (!usr->keypad.activated && !usr->keypad.newsDetected)
        {
            usr->tracker.pendingInstrumentAction = 0;
            usr->tracker.pendingInstrumentTarget = -1;
            usr->tracker.pendingInstrumentKeypadOpen = false;
        }
        return;
    }
    const char *title =
        usr->tracker.pendingInstrumentAction == 1 ? "Clone Instrument" :
        usr->tracker.pendingInstrumentAction == 2 ? "Name Instrument" :
        "New Instrument";
    usr->tracker.pendingInstrumentKeypadOpen = true;
    usr->windowStack.windowStackPushKeypadEditor(
        &usr->keypad,
        title,
        usr->tracker.pendingInstrumentName,
        &usr->tracker.pendingInstrumentNameLen,
        false
    );
}

static inline void Tracker_ApplyInstrumentNameKeypadResult(UserContext *usr)
{
    if (!usr || !usr->keypad.newsDetected || usr->tracker.pendingInstrumentAction == 0)
        return;
    int action = usr->tracker.pendingInstrumentAction;
    int inst = std::max(0, std::min(255, usr->tracker.pendingInstrument));
    int target = std::max(0, std::min(255, usr->tracker.pendingInstrumentTarget));
    if (action == 1)
    {
        Tracker_CloneInstrument(
            &usr->tracker,
            inst,
            target,
            usr->tracker.pendingInstrumentName,
            usr->tracker.pendingInstrumentNameLen
        );
    }
    else if (action == 2)
    {
        Tracker_SetInstrumentName(
            &usr->tracker,
            inst,
            usr->tracker.pendingInstrumentName,
            usr->tracker.pendingInstrumentNameLen
        );
        usr->tracker.patternDirty = true;
        usr->tracker.copyOnWriteRequested = true;
    }
    else if (action == 3)
    {
        Tracker_CreateInstrumentFromTemplate(
            &usr->tracker,
            target,
            usr->tracker.pendingInstrumentName,
            usr->tracker.pendingInstrumentNameLen
        );
    }
    usr->tracker.pendingInstrumentAction = 0;
    usr->tracker.pendingInstrumentTarget = -1;
    usr->tracker.pendingInstrumentKeypadOpen = false;
}

static inline void Tracker_OpenSongNameKeypadIfRequested(UserContext *usr)
{
    if (!usr || !usr->tracker.pendingSongNameKeypadOpen)
        return;
    if (usr->tracker.pendingSongNameKeypadActive)
    {
        if (!usr->keypad.activated && !usr->keypad.newsDetected)
        {
            usr->tracker.pendingSongNameKeypadOpen = false;
            usr->tracker.pendingSongNameKeypadActive = false;
        }
        return;
    }
    if (usr->keypad.activated)
        return;
    usr->tracker.pendingSongNameKeypadActive = true;
    usr->windowStack.windowStackPushKeypadEditor(
        &usr->keypad,
        "Song Name",
        usr->tracker.pendingSongName,
        &usr->tracker.pendingSongNameLen,
        false
    );
}

static inline void Tracker_ApplySongNameKeypadResult(UserContext *usr)
{
    if (!usr || !usr->keypad.newsDetected || !usr->tracker.pendingSongNameKeypadOpen)
        return;
    if (usr->tracker.pendingSongNameLen <= 0)
    {
        usr->tracker.pendingSongNameKeypadOpen = false;
        return;
    }

    int tickRate = usr->tracker.songTickRate;
    int speed = usr->tracker.songSpeed;
    int rowsPerBeat = usr->tracker.songRowsPerBeat;
    bool lfoEnabled = usr->tracker.songLfoEnabled;
    int lfoFrequency = usr->tracker.songLfoFrequency;
    bool loopEnabled = usr->tracker.loopEnabled;
    int loopStart = usr->tracker.loopStart;
    int loopEnd = usr->tracker.loopEnd;
    std::string uiPattern = Tracker_BuildPatternText(&usr->tracker);
    std::string pattern = Tracker_BuildPlaybackPatternText(&usr->tracker);
    usr->tracker.pendingSongName[std::min((int32_t)TRACKER_SONG_NAME_CAPACITY - 1, usr->tracker.pendingSongNameLen)] = '\0';

    usr->sound.setUserSong(usr->tracker.pendingSongName, uiPattern.c_str(), pattern.c_str());
    usr->sound.currentSongIndex = TRACKER_USER_SONG_SLOT;
    usr->tracker.songIndex = TRACKER_USER_SONG_SLOT;
    std::snprintf(usr->tracker.songDisplayName, sizeof(usr->tracker.songDisplayName), "%s", usr->sound.userSongName);
    usr->tracker.songTickRate = tickRate;
    usr->tracker.songSpeed = speed;
    usr->tracker.ticksPerRow = speed;
    usr->tracker.songRowsPerBeat = rowsPerBeat;
    usr->tracker.songLfoEnabled = lfoEnabled;
    usr->tracker.songLfoFrequency = lfoFrequency;
    usr->tracker.loopEnabled = loopEnabled;
    usr->tracker.loopStart = std::max(0, std::min(loopStart, std::max(0, usr->tracker.rowCount - 1)));
    usr->tracker.loopEnd = std::max(usr->tracker.loopStart, std::min(loopEnd, std::max(0, usr->tracker.rowCount - 1)));
    usr->tracker.loopRangeDirty = true;
    usr->tracker.patternDirty = true;
    usr->tracker.copyOnWriteRequested = false;
    usr->tracker.pendingSongNameKeypadOpen = false;
    usr->tracker.pendingSongNameKeypadActive = false;
    Tracker_UpdateSoundSettingsSongNames(usr);
}

static inline void Tracker_OpenPartNameKeypadIfRequested(UserContext *usr)
{
    if (!usr || !usr->tracker.pendingPartNameKeypadOpen)
        return;
    if (usr->tracker.pendingPartNameKeypadActive)
    {
        if (!usr->keypad.activated && !usr->keypad.newsDetected)
        {
            usr->tracker.pendingPartAction = 0;
            usr->tracker.pendingPartNameKeypadOpen = false;
            usr->tracker.pendingPartNameKeypadActive = false;
        }
        return;
    }
    if (usr->keypad.activated)
        return;
    usr->tracker.pendingPartNameKeypadActive = true;
    usr->windowStack.windowStackPushKeypadEditor(
        &usr->keypad,
        "Part Name",
        usr->tracker.pendingPartName,
        &usr->tracker.pendingPartNameLen,
        false
    );
}

static inline void Tracker_ApplyPartNameKeypadResult(UserContext *usr)
{
    if (!usr || !usr->keypad.newsDetected || !usr->tracker.pendingPartNameKeypadOpen)
        return;
    int part = usr->tracker.pendingPart;
    if (part >= 0 && part < usr->tracker.partCount && usr->tracker.pendingPartNameLen > 0)
    {
        usr->tracker.pendingPartName[std::min((int32_t)TRACKER_PART_NAME_CAPACITY - 1, usr->tracker.pendingPartNameLen)] = '\0';
        Tracker_SetPartName(&usr->tracker.parts[part], usr->tracker.pendingPartName);
        usr->tracker.patternDirty = true;
        usr->tracker.copyOnWriteRequested = true;
    }
    usr->tracker.pendingPartAction = 0;
    usr->tracker.pendingPart = -1;
    usr->tracker.pendingPartNameKeypadOpen = false;
    usr->tracker.pendingPartNameKeypadActive = false;
}

static inline void Tracker_SaveSongToBrowser(UserContext *usr)
{
    if (!usr) return;
    std::string pattern = Tracker_BuildPatternText(&usr->tracker);
    std::string displayName = usr->tracker.songDisplayName;
    if (displayName.empty())
        displayName = usr->sound.getSongName(usr->sound.currentSongIndex);
    if (displayName.empty())
        displayName = Tracker_DefaultUserSongDisplayName();
    std::string filename = TrackerSongIO_SaveFilenameForDisplay(displayName);
    if (filename.size() <= 2 || filename == ".h")
        filename = TrackerSongIO_SaveFilenameForDisplay(Tracker_DefaultUserSongDisplayName());
    std::string text = TrackerSongIO_BuildFileText(
        displayName,
        pattern,
        Tracker_BuildCustomInstrumentText(&usr->tracker),
        usr->tracker.songTickRate,
        usr->tracker.songSpeed,
        usr->tracker.songRowsPerBeat,
        usr->tracker.songLfoEnabled,
        usr->tracker.songLfoFrequency
    );
#ifdef __EMSCRIPTEN__
    // Browser downloads can briefly blur/hide the page. Reuse the same grace
    // path as the real file picker so returning from a save does not reopen
    // greetings or the inactive overlay.
    usr->trackerSongFilePickerActive = true;
    usr->trackerSongFilePickerFocusGraceFrames = 120;
    usr->saveGreetingMuteFrames = 240;
    usr->appFocusLost = false;
    usr->appInactiveOverlayActive = false;
    usr->greetingsWindowRequested = false;
    usr->greetingsResumeMessageRequested = false;
    EM_ASM({
        let filename = UTF8ToString($0);
        if (!filename || filename === ".h") filename = "SONG.h";
        if (!filename.toLowerCase().endsWith(".h")) filename += ".h";
        const text = UTF8ToString($1);
        const blob = new Blob([text], { type: 'text/x-c++hdr;charset=utf-8' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.setAttribute('download', filename);
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        setTimeout(function () { URL.revokeObjectURL(url); }, 0);
    }, filename.c_str(), text.c_str());
    usr->trackerSongFilePickerActive = false;
    usr->trackerSongFilePickerFocusGraceFrames = 120;
#else
    (void)filename;
    (void)text;
#endif
}

static inline void Tracker_ReportSongLoadFailure(UserContext *usr, const std::string &error)
{
    if (!usr) return;
    std::string fullError = error.empty() ? "invalid tracker file" : error;
    std::string summary = TrackerSongIO_LoadErrorSummary(fullError);
    std::snprintf(
        usr->tracker.songLoadStatus,
        sizeof(usr->tracker.songLoadStatus),
        "%s",
        summary.c_str()
    );
    std::snprintf(
        usr->tracker.songLoadErrorText,
        sizeof(usr->tracker.songLoadErrorText),
        "%s",
        fullError.c_str()
    );
    usr->tracker.songLoadErrorWindowOpen = true;
    usr->tracker.songLoadErrorWindowRequested = false;
    usr->windowStack.windowStackPushTrackerLoadErrorWindow();
}

static inline void Tracker_ClearSongFilePickerState(UserContext *usr)
{
    if (!usr) return;
    usr->trackerSongFilePickerActive = false;
    usr->trackerSongFilePickerFocusGraceFrames = 120;
    usr->appFocusLost = false;
    usr->appInactiveOverlayActive = false;
}

static inline bool Tracker_IsSongFilePickerFocusMuted(UserContext *usr)
{
    return usr && (usr->trackerSongFilePickerActive || usr->trackerSongFilePickerFocusGraceFrames > 0);
}

static inline bool AppInactiveOverlayIsOrphaned(UserContext *usr)
{
    return usr && usr->appInactiveOverlayActive && usr->windowStack.count == 0 && !usr->dialog.active;
}

static inline void AppInactiveOverlayRepairOrClear(UserContext *usr)
{
    if (!AppInactiveOverlayIsOrphaned(usr))
        return;
    if (usr->appFocusLost)
    {
        usr->greetingsResumeMessageRequested = true;
        usr->windowStack.windowStackPushGreetingsWindow(true);
        usr->greetingsWindowRequested = false;
        usr->greetingsResumeMessageRequested = false;
    }
    else
    {
        usr->appInactiveOverlayActive = false;
    }
}

static inline bool AppInactiveOverlayHandleOrphanPointerEvent(UserContext *usr, const SDL_Event &e)
{
    if (!AppInactiveOverlayIsOrphaned(usr))
        return false;
    const bool pointerEvent =
        (e.type == SDL_MOUSEBUTTONDOWN) || (e.type == SDL_MOUSEBUTTONUP) ||
        (e.type == SDL_FINGERDOWN) || (e.type == SDL_FINGERUP);
    if (!pointerEvent)
        return false;
    usr->appFocusLost = false;
    usr->appInactiveOverlayActive = false;
    return true;
}

#ifdef __EMSCRIPTEN__
extern "C" EMSCRIPTEN_KEEPALIVE void Tracker_EmscriptenSongFilePickerClosed()
{
    if (!g_trackerIoUserContext) return;
    Tracker_ClearSongFilePickerState(g_trackerIoUserContext);
}

extern "C" EMSCRIPTEN_KEEPALIVE void Tracker_EmscriptenSongFileLoaded(const char *filename, const char *text)
{

    if (!g_trackerIoUserContext || !filename || !text) return;
    UserContext *usr = g_trackerIoUserContext;
    Tracker_ClearSongFilePickerState(usr);

    TrackerSongLoadResult loaded = TrackerSongIO_ParseFile(filename, text);
    if (!loaded.ok)
    {
        Tracker_ReportSongLoadFailure(usr, loaded.error);
        return;
    }
    std::string instruments;
    (void)TrackerSongIO_ExtractInstrumentText(text, instruments);
    const std::string migratedPattern = loaded.pattern;
    setTrackerPatternState(&usr->trackerLoadScratch, TRACKER_USER_SONG_SLOT, migratedPattern.c_str(), loaded.displayName.c_str());
    const std::string playbackPattern = Tracker_BuildPlaybackPatternText(&usr->trackerLoadScratch);
    usr->sound.setUserSong(loaded.displayName.c_str(), migratedPattern.c_str(), playbackPattern.c_str());
    usr->sound.currentSongIndex = TRACKER_USER_SONG_SLOT;
    setTrackerPatternState(&usr->tracker, TRACKER_USER_SONG_SLOT, migratedPattern.c_str(), usr->sound.userSongName);
    Tracker_ClearInstrumentState(&usr->tracker, true);
    int setting = 0;
    if (TrackerSongIO_ExtractInt(text, "XFM_TRACKER_TICK_RATE", setting))
        usr->tracker.songTickRate = std::max(1, std::min(300, setting));
    if (TrackerSongIO_ExtractInt(text, "XFM_TRACKER_SPEED", setting))
    {
        usr->tracker.songSpeed = std::max(1, std::min(32, setting));
        usr->tracker.ticksPerRow = usr->tracker.songSpeed;
    }
    if (TrackerSongIO_ExtractInt(text, "XFM_TRACKER_ROWS_PER_BEAT", setting))
        usr->tracker.songRowsPerBeat = std::max(1, std::min(32, setting));
    if (TrackerSongIO_ExtractInt(text, "XFM_TRACKER_LFO_ENABLED", setting))
        usr->tracker.songLfoEnabled = setting != 0;
    if (TrackerSongIO_ExtractInt(text, "XFM_TRACKER_LFO_FREQUENCY", setting))
        usr->tracker.songLfoFrequency = std::max(0, std::min(7, setting));
    if (TrackerSongIO_ExtractInstrumentText(text, instruments))
        Tracker_LoadCustomInstrumentText(&usr->tracker, instruments);
    bool referencedInstruments[256] = {};
    TrackerSongIO_MarkReferencedInstruments(migratedPattern, referencedInstruments);
    char missing[96] = {};
    int missingLen = 0;
    for (int inst = 0; inst < 256; inst++)
    {
        if (!referencedInstruments[inst] || usr->tracker.editPatchValid[inst])
            continue;
        int written = std::snprintf(
            missing + missingLen,
            sizeof(missing) - (size_t)missingLen,
            "%s%02X",
            missingLen > 0 ? ", " : "",
            inst
        );
        if (written <= 0)
            break;
        missingLen = std::min((int)sizeof(missing) - 1, missingLen + written);
    }
    Tracker_UpdateSoundSettingsSongNames(usr);
    usr->tracker.patternDirty = true;
    usr->tracker.copyOnWriteRequested = false;
    usr->loadGreetingMuteFrames = 240;
    usr->greetingsResumeMessageRequested = true;
    usr->greetingsWindowRequested = true;
    if (missing[0])
        std::snprintf(
            usr->tracker.songLoadStatus,
            sizeof(usr->tracker.songLoadStatus),
            "Loaded %s; missing instruments: %s",
            loaded.displayName.c_str(),
            missing
        );
    else
        std::snprintf(
            usr->tracker.songLoadStatus,
            sizeof(usr->tracker.songLoadStatus),
            "Loaded %s",
            loaded.displayName.c_str()
        );
    usr->tracker.songLoadErrorText[0] = '\0';
    usr->tracker.songLoadErrorWindowOpen = false;
    Tracker_ApplyPatchEditsToSound(usr);
    Tracker_ApplyRealtimeLfoToSound(usr);
}
#endif

static inline void Tracker_OpenSongLoadDialog(UserContext *usr)
{
    if (!usr) return;
#ifdef __EMSCRIPTEN__
    usr->trackerSongFilePickerActive = true;
    usr->trackerSongFilePickerFocusGraceFrames = 0;
    usr->appFocusLost = false;
    usr->appInactiveOverlayActive = false;
    EM_ASM({
        function notifyPickerClosed() {
            Module.ccall('Tracker_EmscriptenSongFilePickerClosed', null, [], []);
        }

        function makeInput() {
            const input = document.createElement('input');
            input.type = 'file';
            input.accept = '.h,.txt,text/plain,text/x-c++hdr';
            input.addEventListener('cancel', notifyPickerClosed);
            input.onchange = function () {
                const file = input.files && input.files[0];
                if (!file) {
                    notifyPickerClosed();
                    return;
                }
                const reader = new FileReader();
                reader.onload = function () {
                    Module.ccall('Tracker_EmscriptenSongFileLoaded', null, ['string', 'string'], [file.name, String(reader.result || "")]);
                };
                reader.onerror = notifyPickerClosed;
                reader.readAsText(file);
                const overlay = document.getElementById('xfm-tracker-load-overlay');
                if (overlay) overlay.remove();
            };
            return input;
        }

        function showTapFallback() {
            const existing = document.getElementById('xfm-tracker-load-overlay');
            if (existing) existing.remove();

            const overlay = document.createElement('div');
            overlay.id = 'xfm-tracker-load-overlay';
            overlay.style.cssText =
                'position:fixed;left:0;right:0;bottom:0;top:0;' +
                'z-index:2147483647;display:flex;align-items:center;' +
                'justify-content:center;background:rgba(0,0,0,0.35);font-family:sans-serif';

            const panel = document.createElement('div');
            panel.style.cssText =
                'position:relative;width:min(86vw,360px);padding:24px;border-radius:16px;' +
                'background:#241633;color:white;text-align:center;box-shadow:0 8px 30px rgba(0,0,0,0.45)';

            const title = document.createElement('div');
            title.textContent = 'Tap to load song';
            title.style.cssText = 'font-size:20px;margin-bottom:14px;font-weight:700';

            const hint = document.createElement('div');
            hint.textContent = 'Choose a tracker .h file';
            hint.style.cssText = 'font-size:14px;margin-bottom:18px;opacity:0.8';

            const button = document.createElement('div');
            button.textContent = 'LOAD FILE';
            button.style.cssText =
                'position:relative;height:54px;line-height:54px;border-radius:12px;' +
                'background:#4f35df;font-size:18px;font-weight:700;overflow:hidden';

            const input = makeInput();
            input.style.cssText = 'position:absolute;inset:0;width:100%;height:100%;opacity:0;cursor:pointer';
            button.appendChild(input);

            const close = document.createElement('button');
            close.textContent = 'Cancel';
            close.style.cssText = 'margin-top:14px;border:0;background:transparent;color:#ddd;font-size:16px;padding:10px';
            close.onclick = function () {
                notifyPickerClosed();
                overlay.remove();
            };

            panel.appendChild(title);
            panel.appendChild(hint);
            panel.appendChild(button);
            panel.appendChild(close);
            overlay.appendChild(panel);
            document.body.appendChild(overlay);
        }

        const ua = navigator.userAgent || "";
        const isiOS = /iPad|iPhone|iPod/.test(ua) || (navigator.platform === 'MacIntel' && navigator.maxTouchPoints > 1);
        if (isiOS) {
            showTapFallback();
            return;
        }

        const input = makeInput();
        input.style.display = 'none';
        document.body.appendChild(input);
        input.click();
        setTimeout(function () {
            window.addEventListener('focus', function () {
                setTimeout(function () {
                    if (input.parentNode && (!input.files || input.files.length === 0))
                        notifyPickerClosed();
                }, 500);
            }, { once: true });
        }, 0);
        setTimeout(function () {
            if (input.parentNode) {
                if (!input.files || input.files.length === 0)
                    notifyPickerClosed();
                input.parentNode.removeChild(input);
            }
        }, 60000);
    });
#endif
}

static inline void Tracker_ApplyPatternToSound(UserContext *usr)
{
    if (!usr)
        return;
    bool patternDirty = usr->tracker.patternDirty;
    bool songLengthDirty = usr->tracker.songLengthDirty;
    bool committed = Tracker_CommitPatternToUserSong(usr);

    // Channel selection acts as "solo": only selected channels should play.
    // Any change to the channel selection must redeclare the song pattern.
    const bool wantsChannelSolo = usr->gameMode == UserContext::GameMode::TRACKER && usr->tracker.channelSelectionEnabled;
    const bool soloChanged =
        wantsChannelSolo != usr->tracker.channelSoloApplied ||
        (wantsChannelSolo &&
         (usr->tracker.channelStart != usr->tracker.channelSoloAppliedStart ||
          usr->tracker.channelEnd != usr->tracker.channelSoloAppliedEnd));
    if (soloChanged)
    {
        usr->tracker.channelSoloApplied = wantsChannelSolo;
        usr->tracker.channelSoloAppliedStart = usr->tracker.channelStart;
        usr->tracker.channelSoloAppliedEnd = usr->tracker.channelEnd;
        patternDirty = true;
    }

    if (!patternDirty && !committed)
        return;

    usr->tracker.patternDirty = false;
    usr->tracker.songLengthDirty = false;
    if (usr->sound.useWavPlayback || usr->sound.audioDisabled || !usr->sound.musicModule)
        return;

    int songId = usr->sound.currentSongIndex;
    bool selectionOverrideActive = false;
    int loopStartRow = 0;
    int loopEndRow = std::max(0, Tracker_PlaybackRowCount(&usr->tracker) - 1);
    const char *pattern = Tracker_SelectLivePlaybackPattern(
        usr,
        &selectionOverrideActive,
        &loopStartRow,
        &loopEndRow
    );
    usr->trackerSelectionPlaybackOverrideActive = selectionOverrideActive;
    int tickRate = std::max(1, usr->tracker.songTickRate);
    int ticksPerRow = std::max(1, usr->tracker.songSpeed);
    int resumeRow = usr->sound.musicModule->active_song.current_row;
    SDL_LockAudioDevice(usr->sound.audioDev);
    int prevTickRate = 0;
    int prevTicksPerRow = 0;
    if (songId >= 0 && songId < 16 && usr->sound.musicModule->song_present[songId])
    {
        prevTickRate = usr->sound.musicModule->song_patterns[songId].tick_rate;
        prevTicksPerRow = usr->sound.musicModule->song_patterns[songId].speed;
    }
    xfm_module_set_lfo(usr->sound.musicModule, usr->tracker.songLfoEnabled, usr->tracker.songLfoFrequency);
    xfm_song_declare(usr->sound.musicModule, songId, pattern, tickRate, ticksPerRow);
    if (usr->tracker.playing)
    {
        bool songWasActive = usr->sound.musicModule->active_song.active;
        bool songIdChanged = usr->sound.musicModule->active_song.song_id != songId;
        const bool tempoChanged =
            (prevTickRate != 0 && prevTickRate != tickRate) ||
            (prevTicksPerRow != 0 && prevTicksPerRow != ticksPerRow);
        if (songLengthDirty || songIdChanged || !songWasActive || tempoChanged)
            xfm_song_play(usr->sound.musicModule, songId, true);
        xfm_song_set_loop_range(usr->sound.musicModule, loopStartRow, loopEndRow);
        if (!songLengthDirty && (songIdChanged || !songWasActive))
        {
            const int jumpRow = selectionOverrideActive ?
                Tracker_LivePlaybackRowFromSongRow(&usr->tracker, usr->tracker.playRow, true) :
                resumeRow;
            xfm_song_jump_to_row(usr->sound.musicModule, jumpRow);
        }
        if (wantsChannelSolo && usr->sound.musicModule->chip)
        {
            for (int ch = 0; ch < TRACKER_CHANNELS; ch++)
            {
                const bool selected = ch >= usr->tracker.channelStart && ch <= usr->tracker.channelEnd;
                if (!selected)
                {
                    usr->sound.musicModule->chip->hard_mute(ch);
                    usr->sound.musicModule->channel_active[ch] = false;
                }
            }
        }
    }
    else
    {
        usr->sound.musicModule->active_song.active = false;
        for (int ch = 0; ch < 6; ch++)
        {
            if (usr->sound.musicModule->chip)
                usr->sound.musicModule->chip->key_off(ch);
            usr->sound.musicModule->channel_active[ch] = false;
        }
    }
    SDL_UnlockAudioDevice(usr->sound.audioDev);
}

static inline void Tracker_ApplyTransportRequests(UserContext *usr)
{
    if (!usr)
        return;
    if (usr->tracker.musicStartRequested)
    {
        usr->tracker.musicStartRequested = false;
        const bool selectionOverrideActive = Tracker_ShouldUseSelectionPlaybackOverride(&usr->tracker);
        int startSongRow = usr->tracker.loopEnabled ? usr->tracker.loopStart : 0;
        int startRow = Tracker_LivePlaybackRowFromSongRow(&usr->tracker, startSongRow, selectionOverrideActive);
        usr->sound.startMusicAtRow(startRow);
    }
    if (usr->tracker.musicStopRequested)
    {
        usr->tracker.musicStopRequested = false;
        usr->sound.stopMusic();
    }
    if (usr->tracker.musicPlayRequested)
    {
        usr->tracker.musicPlayRequested = false;
        usr->sound.playCurrentMusic(false);
    }
}

static inline void Tracker_PlayRequestedPreview(UserContext *usr)
{
    if (!usr)
        return;
    if (usr->tracker.previewHeldNoteStopRequested)
    {
        usr->tracker.previewHeldNoteStopRequested = false;
        usr->sound.releaseTrackerPreviewNote();
    }
    if (!usr->tracker.previewNoteRequested && !usr->tracker.previewHeldNoteStartRequested)
        return;
    bool held = usr->tracker.previewHeldNoteStartRequested;
    usr->tracker.previewNoteRequested = false;
    usr->tracker.previewHeldNoteStartRequested = false;
    if (usr->sound.useWavPlayback || usr->sound.audioDisabled)
        return;
    int inst = std::max(0, std::min(255, usr->tracker.editInstrument));
    const xfm_patch_opn *patch = usr->tracker.editPatchValid[inst] ? &usr->tracker.editPatches[inst] : nullptr;
    usr->sound.previewTrackerNote(
        usr->tracker.editNote,
        usr->tracker.editOctave,
        inst,
        usr->tracker.editVolume,
        patch,
        usr->tracker.editMacros[inst],
        usr->tracker.editMacroEnabled[inst],
        usr->tracker.editMacroValid[inst],
        held
    );
}

static inline void Sound_HandleBrowserLifecycle(UserContext *usr)
{
    if (!usr)
        return;
#ifdef __EMSCRIPTEN__
    int audioLifecycleState = EM_ASM_INT({
        if (!Module.xfmAudioLifecycleInstalled) {
            Module.xfmAudioLifecycleInstalled = true;
            Module.xfmAudioLifecycleState = document.hidden ? 1 : 0;
            Module.xfmAppRefocusPending = 0;
            const suspend = function () { Module.xfmAudioLifecycleState = 1; };
            const resumeAudio = function () { Module.xfmAudioLifecycleState = 2; };
            const refocus = function () {
                Module.xfmAudioLifecycleState = 2;
                Module.xfmAppRefocusPending = 1;
            };
            document.addEventListener('visibilitychange', function () {
                if (document.hidden) suspend();
                else refocus();
            });
            window.addEventListener('pagehide', suspend);
            window.addEventListener('pageshow', refocus);
            window.addEventListener('focus', refocus);
            window.addEventListener('blur', function () {
                if (document.hidden) suspend();
            });
            window.addEventListener('pointerdown', resumeAudio, true);
            window.addEventListener('touchstart', resumeAudio, true);
        }
        const state = Module.xfmAudioLifecycleState | 0;
        const refocusPending = Module.xfmAppRefocusPending ? 4 : 0;
        Module.xfmAudioLifecycleState = 0;
        Module.xfmAppRefocusPending = 0;
        return state | refocusPending;
    });
    const int audioState = audioLifecycleState & 3;
    const bool appRefocused = (audioLifecycleState & 4) != 0;
    const bool greetingMuteActive = usr->saveGreetingMuteFrames > 0 || usr->loadGreetingMuteFrames > 0;
    if (audioState == 1)
    {
        if (!Tracker_IsSongFilePickerFocusMuted(usr))
        {
            if (greetingMuteActive)
            {
                usr->appFocusLost = false;
                usr->appInactiveOverlayActive = false;
            }
            else
            {
                usr->appInactiveOverlayActive = true;
                usr->greetingsResumeMessageRequested = true;
                usr->greetingsWindowRequested = true;
            }
        }
        usr->sound.suspendForBrowser();
    }
    else if (audioState == 2)
    {
        if (usr->sound.audioStoppedBecauseWindowLeave || usr->sound.browserAudioSuspended || !usr->sound.audioDev)
            usr->sound.resumeFromBrowser(usr->sound.getSongPlaybackPattern(usr->sound.currentSongIndex));
    }
    if (appRefocused)
    {
        if (!Tracker_IsSongFilePickerFocusMuted(usr) && !greetingMuteActive)
        {
            usr->greetingsResumeMessageRequested = true;
            usr->greetingsWindowRequested = true;
        }
    }
#endif
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

    // Mass-derived launch modifier (lighter balls get more buff, heavier balls less).
    usr->lightnessBuff = BallStats_LightnessBuff(usr->desiredMass);
    usr->launchBuffEffective = glm::clamp(ball.launchBuff * usr->lightnessBuff, 0.0f, 1.0f);

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

    // Arm impulse at throw: map from launchBuff
    usr->armImpulseAtThrow = remapClamped(
        usr->launchBuffEffective,
        BallPhysicsMapping::CATALOG_BUFF_MIN,
        BallPhysicsMapping::CATALOG_BUFF_MAX,
        BallPhysicsMapping::PHYSICS_ARM_IMPULSE_MIN,
        BallPhysicsMapping::PHYSICS_ARM_IMPULSE_MAX
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

    // Restitution (bounciness) is catalog-driven per ball, then modified by mass:
    // light balls get bouncier, heavy balls lose bounce.
    {
        float baseRest = glm::clamp(ball.restitution, 0.0f, 1.0f);
        float massScale = BallStats_RestitutionMassScale(usr->desiredMass);
        usr->ballRestitution = glm::clamp(baseRest * massScale, 0.0f, 1.0f);
    }

    // Store radius for any radius-dependent calculations
    // usr->ballRadius = ball.radius;
}

static inline void BallStats_ApplyFrictionOnly(UserContext *usr, const CatalogItem &ball)
{
    if (!usr)
        return;
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
}

static inline void BallStats_ApplyLaunchImpulseOnly(UserContext *usr)
{
    if (!usr)
        return;
    usr->armImpulseAtThrow = remapClamped(
        usr->launchBuffEffective,
        BallPhysicsMapping::CATALOG_BUFF_MIN,
        BallPhysicsMapping::CATALOG_BUFF_MAX,
        BallPhysicsMapping::PHYSICS_ARM_IMPULSE_MIN,
        BallPhysicsMapping::PHYSICS_ARM_IMPULSE_MAX
    );
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
    usr->electroBall.resetCharge();
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
	    usr->phy.apply_restitution_to_lane(glm::clamp(usr->laneRestitution, 0.0f, 1.0f));
	    usr->phy.set_ball_restitution(glm::clamp(usr->ballRestitution, 0.0f, 1.0f));
	    usr->phy.set_pins_restitution(glm::clamp(usr->pinRestitution, 0.0f, 1.0f));
	    usr->phy.set_pins_friction(glm::max(0.0f, usr->pinFriction));
	    usr->phy.set_pins_mass(glm::max(0.05f, usr->pinMass));
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
        const bool pushbackEnabled =
            BallFrictionTuning::PUSHBACK_ENABLED &&
            !(usr->gameMode == UserContext::GameMode::SCHOOL && usr->school.selectedLesson == 5);
        usr->phy.set_lane_pushback_oil_profile(
            BallFrictionTuning::LANE_Z_START,
            zFadeEnd,
            maxStrength,
            BallFrictionTuning::SKID_FADE_EASE_EXP,
            pushbackEnabled
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
			            // Positive angVel (CCW input) => positive spin => hooks toward +X (lane-left in our view).
			            float sideDrive = angVel * (usr->myBall.spin * usr->myBall.spin) * 0.125f;

		            // Bite "drive": if ball is still sliding laterally opposite to where the
		            // current spin input wants to take it, add extra drive to flip direction sooner.
		            glm::vec3 v = usr->phy.get_ball_swing_movement();
		            float vx = std::isfinite(v.x) ? v.x : 0.0f;
		            float bite01 = glm::clamp(usr->myBall.bite, 0.0f, 1.0f);
		            bite01 = bite01 * bite01;
		            if (fabsf(sideDrive) > 1e-6f && fabsf(vx) > 0.02f)
		            {
			                // sideDrive -> y angular velocity. With our curve model, y>0 drifts to +x.
			                float desiredLateralDir = (sideDrive > 0.0f) ? 1.0f : -1.0f; // +1 => +x, -1 => -x
			                float movingDir = (vx > 0.0f) ? 1.0f : -1.0f;
			                bool disagree = (movingDir != desiredLateralDir);
			                if (disagree)
			                {
			                    float drive = bite01 * BallSwingTuning::BITE_DRIVE_FROM_LATERAL_VEL * fabsf(vx);
			                    sideDrive += (sideDrive > 0.0f) ? drive : -drive;
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

#ifdef __EMSCRIPTEN__
    // Only for emscripten as it breaks hot reload otherwise
    g_trackerIoUserContext = usr;
#endif

		usr->ballRenderTex.renderTextureInit();
		usr->ballRenderTex2.renderTextureInit();
		usr->oilRenderTex.renderTextureInit(true);
        usr->trackerDiagramTex.width = 1024;
        usr->trackerDiagramTex.height = 1024;
        usr->trackerDiagramTex.renderTextureInit(false);
        usr->trackerOscilloscopeTex.width = TRACKER_OSC_ATLAS_WIDTH;
        usr->trackerOscilloscopeTex.height = TRACKER_OSC_ATLAS_HEIGHT;
        usr->trackerOscilloscopeTex.renderTextureInit(false);
        usr->trackerOscilloscopePixels.resize(TRACKER_OSC_ATLAS_WIDTH * TRACKER_OSC_ATLAS_HEIGHT);

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
        usr->electroBall.initElectroBall();
	    usr->fpsCounter.initFpsCounter();
	    usr->particles.init();
        usr->particles.setSnowflakeCount(usr->settings.snowflakeCount);

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
    Gem_InitIfNeeded(usr);
    Angel_InitIfNeeded(usr);

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
    if (!usr->enemyBoardInit)
    {
        resetScoreboard(&usr->enemyBoard);
        usr->enemyBoardInit = true;
    }
    usr->turnOwner = UserContext::TurnOwner::PLAYER;

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
    initClaytonClick(&usr->housesButton, "HousesButton");
    initClaytonClick(&usr->hiScoreButton, "HiScoreButton");
    School_Init(&usr->school);
    School_ClayInit(&usr->school, &usr->clayton, usr->desiredMass);
    initTracker(&usr->tracker);
    setTrackerSongState(&usr->trackerLoadScratch, 1);
    initClaytonClick(&usr->openShopClick, "openShopButton");
    initClaytonClick(&usr->clayton.closeShopClick, "closeShopButton");
    initClaytonClick(&usr->clayton.buyClick, "BuyButtdd");
    initClaytonClick(&usr->clayton.oilReoilClick, "oilReoilButton");
    initClaytonClick(&usr->clayton.housesCloseClick, "housesClose");
    initClaytonClick(&usr->clayton.housesSelectClick, "housesSelect");
    initClaytonClick(&usr->clayton.menuCloseClick, "menuClose");
    initClaytonClick(&usr->clayton.menuRenameClick, "menuRename");
    initClaytonClick(&usr->clayton.menuSchoolClick, "menuSchool");
    initClaytonClick(&usr->clayton.menuLanguageClick, "menuLanguage");
    initClaytonClick(&usr->clayton.menuCampaignClick, "menuCampaign");
    initClaytonClick(&usr->clayton.menuPracticeClick, "menuPractice");
    initClaytonClick(&usr->clayton.menuFreestyleClick, "menuFreestyle");
    initClaytonClick(&usr->clayton.menuDeviceShareClick, "menuDeviceShare");
    initClaytonClick(&usr->clayton.menuTrackerClick, "menuTracker");
    initClaytonClick(&usr->clayton.menuSettingsClick, "menuSettings");
    initClaytonClick(&usr->clayton.settingsCloseClick, "settingsClose");
    initClaytonClick(&usr->clayton.settingsResetProgressClick, "settingsResetProgress");
    initClaytonClick(&usr->clayton.languageCloseClick, "languageClose");
    initClaytonClick(&usr->clayton.languageEnglishClick, "languageEnglish");
    initClaytonClick(&usr->clayton.languageChineseClick, "languageChinese");
    initClaytonClick(&usr->clayton.languageLithuanianClick, "languageLithuanian");
    initClaytonClick(&usr->clayton.languageJapaneseClick, "languageJapanese");
    initClaytonClick(&usr->clayton.botSelectCloseClick, "botSelectClose");
    initClaytonClick(&usr->clayton.botSelectSelectClick, "botSelectSelect");
    initClaytonClick(&usr->clayton.greetingsReadyClick, "greetingsReady");
    initClaytonClick(&usr->dialog.optionClicks[0], "StoryOpt0");
    initClaytonClick(&usr->dialog.optionClicks[1], "StoryOpt1");
    initClaytonClick(&usr->dialog.optionClicks[2], "StoryOpt2");
    initClaytonClick(&usr->dialog.optionClicks[3], "StoryOpt3");

    usr->tri.init();
    usr->totalFrames = 0;
    usr->dialog.language = usr->language;
    usr->storage.storageInit("10x", "bowling");
    usr->username_len = usr->storage.getChar(Storage::USERNAME, usr->username, 20);
    {
        char tmp[32] = {};
        size_t n = usr->storage.getChar(Storage::SCHOOL_DONE, tmp, sizeof(tmp));
        usr->schoolDone = (n > 0 && tmp[0] == '1');
        n = usr->storage.getChar(Storage::GREETINGS_SEEN, tmp, sizeof(tmp));
        usr->greetingsSeen = (n > 0 && tmp[0] == '1');
        n = usr->storage.getChar(Storage::LANGUAGE, tmp, sizeof(tmp));
        usr->language = Txl_LanguageFromStorage(n > 0 ? tmp : nullptr);
        usr->clayton.loadFontsForLanguage(usr->language);
        usr->dialog.language = usr->language;
        n = usr->storage.getChar(Storage::LAST_LEVEL, tmp, sizeof(tmp));
        if (n > 0)
            usr->campaignLevelIndex = glm::clamp(atoi(tmp), 1, kCampaignLevelCount);
        else
            usr->campaignLevelIndex = 1;
        n = usr->storage.getChar(Storage::BANK, tmp, sizeof(tmp));
        if (n > 0)
            usr->carousel.bank = (float)atof(tmp);
        n = usr->storage.getChar(Storage::UNLOCKED_BALLS, tmp, sizeof(tmp));
        if (n > 0)
            usr->unlockedBallMask = (uint64_t)strtoull(tmp, nullptr, 10);
        n = usr->storage.getChar(Storage::UNLOCKED_HOUSES, tmp, sizeof(tmp));
        if (n > 0)
            usr->unlockedHouseMask = (uint32_t)strtoul(tmp, nullptr, 10);
        n = usr->storage.getChar(Storage::UNLOCKED_BOTS, tmp, sizeof(tmp));
        if (n > 0)
            usr->unlockedBotMask = (uint32_t)strtoul(tmp, nullptr, 10);
    }

    LocalHi_Init(&usr->localHi);

    usr->coinLane.initStars(getNextCoinPattern(), 7);
    usr->clearedCoins = 0;

    // 🔌 Wire static demo catalog (replace with your real data source later)
    Carousel_Init(&usr->carousel);
    Carousel_SetupDefaultShop(&usr->carousel);
    HouseCarousel_Init(&usr->housesCarousel);
    HouseCarousel_SetupDefault(&usr->housesCarousel);
    BotCarousel_Init(&usr->botsCarousel);
    BotCarousel_SetupDefault(&usr->botsCarousel);
    usr->settings.initSettings(Particles::SNOW_FLAKES, Particles::SNOW_FLAKES);
    {
        char tmp[64] = {};
        size_t n = usr->storage.getChar(Storage::BANK, tmp, sizeof(tmp));
        if (n > 0)
            usr->carousel.bank = (float)atof(tmp);
        n = usr->storage.getChar(Storage::UNLOCKED_BALLS, tmp, sizeof(tmp));
        if (n > 0)
            usr->unlockedBallMask = (uint64_t)strtoull(tmp, nullptr, 10);
        n = usr->storage.getChar(Storage::UNLOCKED_HOUSES, tmp, sizeof(tmp));
        if (n > 0)
            usr->unlockedHouseMask = (uint32_t)strtoul(tmp, nullptr, 10);
        n = usr->storage.getChar(Storage::UNLOCKED_BOTS, tmp, sizeof(tmp));
        if (n > 0)
            usr->unlockedBotMask = (uint32_t)strtoul(tmp, nullptr, 10);
    }
    const bool starterUnlocksUpdated = Progress_EnsureStarterUnlocks(usr);
    BallStats_OnBallChange(&g_ballCatalog[0], usr);
    Campaign_ApplyCurrentLevelSetup(usr, /*resetStoryKick=*/true);
    if (usr->carousel.bank <= 0.0f)
        usr->carousel.bank = 20.0f;
    if (starterUnlocksUpdated)
        Progress_SaveUnlocksAndBank(usr);
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
        usr->sound.initSoundSystem(nullptr);
        initSoundSettings(&usr->clayton, &usr->sound.settings, &usr->sound);

        AdaptiveAudio_Init(&usr->adaptiveAudio, 20.0f); // Threshold

        initClaytonClick(&usr->clayton.useSynthClick, "adaptiveUseSynth");
        initClaytonClick(&usr->clayton.useWavClick, "adaptiveUseWav");
        initClaytonClick(&usr->clayton.disableAudioClick, "adaptiveDisableAudio");
        initClaytonClick(&usr->clayton.oilStatusCloseClick, "oilStatusClose");

        usr->windowStack.windowStackInit();
        usr->greetingsWindowRequested = true;

        shouldHandleResize = true;
        std::cerr << "resize will be forced because it is first ever run" << std::endl;
    }
    Sound_HandleBrowserLifecycle(usr);
    usr->sound.oscilloscopeCaptureEnabled.store(
        usr->gameMode == UserContext::GameMode::TRACKER && usr->tracker.active && usr->tracker.oscilloscopeVisible,
        std::memory_order_relaxed
    );
    if (!usr->trackerSongFilePickerActive && usr->trackerSongFilePickerFocusGraceFrames > 0)
        usr->trackerSongFilePickerFocusGraceFrames--;
    if (usr->saveGreetingMuteFrames > 0)
        usr->saveGreetingMuteFrames--;
    if (usr->loadGreetingMuteFrames > 0)
        usr->loadGreetingMuteFrames--;
    if (usr->greetingsWindowRequested)
    {
        usr->windowStack.windowStackPushGreetingsWindow(usr->greetingsResumeMessageRequested);
        usr->greetingsWindowRequested = false;
        usr->greetingsResumeMessageRequested = false;
    }
    AppInactiveOverlayRepairOrClear(usr);

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
    const float safeDeltaTime = std::isfinite(deltaTime) ? glm::clamp(deltaTime, 0.0f, 0.100f) : (1.0f / 60.0f);
    const float aimSwingStepDt = glm::min(safeDeltaTime, 1.0f / 30.0f);
    const bool lowFpsAimSwingFrame = safeDeltaTime > (1.0f / 28.0f);
    auto vec3Finite = [](const glm::vec3 &v) -> bool {
        return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
    };
    auto stabilizeAimSwingBall = [&](glm::vec3 &ballPos, glm::vec3 &ballVel, const glm::vec3 &fallbackPos) {
        if (!vec3Finite(ballPos))
        {
            ballPos = fallbackPos;
            ballVel = glm::vec3(0.0f);
            return;
        }

        glm::vec3 rope = ballPos - usr->pivotPoint;
        float ropeLenNow = glm::length(rope);
        const float maxRopeLen = 1.08f;
        if (ropeLenNow > maxRopeLen && ropeLenNow > 1e-6f)
        {
            ballPos = usr->pivotPoint + rope * (maxRopeLen / ropeLenNow);
            ballVel *= 0.35f;
        }

        const float minBallY = -0.08f;
        if (ballPos.y < minBallY)
        {
            ballPos.y = minBallY;
            if (ballVel.y < 0.0f)
                ballVel.y = 0.0f;
        }
    };
    Tracker_Tick(&usr->tracker, deltaTime);
    Tracker_SyncCursorFromSound(usr);
    Tracker_ApplyLoopRangeToSound(usr);
    Tracker_ApplyPatternToSound(usr);
    Tracker_ApplyPatchEditsToSound(usr);
    Tracker_ApplyRealtimeLfoToSound(usr);
    const bool trackerOnlyMode =
        usr->gameMode == UserContext::GameMode::TRACKER && usr->tracker.active;
    usr->deltaTimeLoan = deltaTime;
    usr->deltaTimeSum += deltaTime;                   // for some stuff need it in float
    volatile uint64_t currentTime = SDL_GetTicks64(); // For simple stuff, in ms

    const CampaignLevelConfig &campaignLevel = Campaign_CurrentLevel(usr);

    // Start-of-level story: show after the first rendered frame whenever the
    // current chapter defines an intro beat.
    if (campaignLevel.startStoryId != 0 &&
        usr->campaignStartStoryLevelShown != campaignLevel.levelNumber &&
        usr->totalFrames > 1 &&
        usr->windowStack.count == 0 &&
        !usr->dialog.active &&
        usr->gameMode != UserContext::GameMode::SCHOOL &&
        usr->gameMode != UserContext::GameMode::TRACKER)
    {
        usr->campaignStartStoryLevelShown = campaignLevel.levelNumber;
        usr->dialog.open(campaignLevel.startStoryId);
        usr->dialog.dialogAppearDelayLeft = 0.0f;
        usr->dialog.openedThisFrame = true;
    }

    usr->auroraVibe.update(deltaTime);

    // Vs mode: keep enemy turn in a runnable phase even if UI flows/hot-reload
    // reset us back to IDLE while the enemy owns the turn.
    if (!trackerOnlyMode)
        Enemy_EnsureTurnActive(usr, deltaTime);

    // Tick Angel animation in the update step so enemy launch timing can be driven by it.
    if (!trackerOnlyMode && usr->gameMode == UserContext::GameMode::BOT)
    {
        Bot_InitIfNeeded(usr);
        Angel_Tick(usr, deltaTime);
        Enemy_UpdateRenderedBallPosDuringThrow(usr, deltaTime);
    }

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
            if (usr->pendingCampaignEndStoryId != 0 && !usr->dialog.active && usr->windowStack.count == 0)
            {
                usr->dialog.open(usr->pendingCampaignEndStoryId);
                usr->dialog.dialogAppearDelayLeft = 0.0f;
                usr->dialog.openedThisFrame = true;
                usr->pendingCampaignEndStoryId = 0;
            }
            if (usr->pendingCampaignBotResultWindow && !usr->dialog.active && usr->windowStack.count == 0)
            {
                if (usr->playerRoute == PlayerRoute::FREESTYLE)
                {
                    usr->clayton.shouldShowHiScore = true;
                    usr->clayton.shouldShowHiScoreWithLatest = true;
                    usr->windowStack.windowStackPushLocalHiscoreWindow();
                }
                usr->windowStack.windowStackPushBotResultWindow(
                    usr->pendingCampaignBotPlayerScore,
                    usr->pendingCampaignBotEnemyScore,
                    usr->pendingCampaignBotPlayerWon
                );
                usr->pendingCampaignBotResultWindow = false;
            }
            // If a story dialog is active, it owns the end-of-game flow; only show Play Again after it closes.
            // Also: never steal focus from other modals (Oil/Hiscore/etc). Only show Play Again
            // when no other windows are currently open.
            if (!usr->dialog.active && usr->windowStack.count == 0)
                usr->windowStack.windowStackPushNewGameWindow();
            if (usr->clayton.shouldShowHiScore)
            {
                // Put the hi-score window above the play-again window.
                // But never show other windows while a story dialog is running.
                if (!usr->dialog.active)
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
                AdaptiveAudio_ResetExport(&usr->adaptiveAudio);
                usr->sound.hasRuntimeWavBuffers = false;
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
                usr->sound.initSoundSystem(nullptr);
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
                usr->sound.audioDisabled = true;
                usr->sound.shutdown();
                initSoundSettings(&usr->clayton, &usr->sound.settings, &usr->sound);
                adaptiveExportState = ADAPTIVE_EXPORT_IDLE;
            }
            else if (usr->adaptiveAudio.restartUseWav)
            {
                // Step 1: Stop current audio
                printf("[AdaptiveAudio] Stopping current audio before exporting WAVs...\n");
                usr->sound.audioDisabled = false;
                usr->sound.shutdown();
                AdaptiveAudio_ResetExport(&usr->adaptiveAudio);
                usr->sound.hasRuntimeWavBuffers = false;

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
                usr->sound.audioDisabled = false;
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

            usr->sound.initSoundSystem(nullptr);
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
            usr->sound.initSoundSystem(nullptr);
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
        static uint64_t s_ignoreNativeMouseUntil = 0;

        if ((e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP || e.type == SDL_MOUSEMOTION) &&
            ((e.type == SDL_MOUSEMOTION && e.motion.which != SDL_TOUCH_MOUSEID) ||
             (e.type != SDL_MOUSEMOTION && e.button.which != SDL_TOUCH_MOUSEID)) &&
            SDL_GetTicks64() < s_ignoreNativeMouseUntil)
        {
            continue;
        }

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
            s_ignoreNativeMouseUntil = SDL_GetTicks64() + 700;

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
            s_ignoreNativeMouseUntil = SDL_GetTicks64() + 700;

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
            s_ignoreNativeMouseUntil = SDL_GetTicks64() + 700;

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
        if (e.type == SDL_WINDOWEVENT)
        {
    switch (e.window.event)
    {
    case SDL_WINDOWEVENT_FOCUS_LOST:
    case SDL_WINDOWEVENT_HIDDEN:
    case SDL_WINDOWEVENT_MINIMIZED:
        if (!Tracker_IsSongFilePickerFocusMuted(usr))
        {
            usr->appFocusLost = true;
            if (usr->saveGreetingMuteFrames > 0 || usr->loadGreetingMuteFrames > 0)
            {
                usr->appInactiveOverlayActive = false;
            }
            else
                    {
                        usr->appInactiveOverlayActive = true;
                        usr->greetingsResumeMessageRequested = true;
                        usr->windowStack.windowStackPushGreetingsWindow(true);
                        usr->greetingsWindowRequested = false;
                        usr->greetingsResumeMessageRequested = false;
                    }
                }
                usr->sound.suspendForBrowser();
                break;
            case SDL_WINDOWEVENT_FOCUS_GAINED:
            case SDL_WINDOWEVENT_SHOWN:
            case SDL_WINDOWEVENT_RESTORED:
                if (usr->appFocusLost)
            {
                usr->appFocusLost = false;
                if (usr->sound.audioStoppedBecauseWindowLeave || usr->sound.browserAudioSuspended || !usr->sound.audioDev)
                    usr->sound.resumeFromBrowser(usr->sound.getSongPlaybackPattern(usr->sound.currentSongIndex));
                if (!Tracker_IsSongFilePickerFocusMuted(usr) && usr->saveGreetingMuteFrames <= 0 && usr->loadGreetingMuteFrames <= 0)
                {
                    usr->greetingsResumeMessageRequested = true;
                    usr->greetingsWindowRequested = true;
                    usr->windowStack.windowStackPushGreetingsWindow(true);
                        usr->greetingsWindowRequested = false;
                        usr->greetingsResumeMessageRequested = false;
                    }
                }
                break;
            default:
                break;
            }
        }
        if (e.type == SDL_QUIT)
            ctx->shouldContinue = false;

        usr->clayton.processClaytonEvent(&e, deltaTime, pixelRatio);

        bool stolenByClayton = false;
        if (stolenByClayton)
        {
            continue;
        }

        if (usr->gameMode == UserContext::GameMode::TRACKER &&
            usr->tracker.active &&
            Tracker_HandleOscilloscopeEvent(&usr->tracker, &usr->clayton, e))
        {
            continue;
        }

        // Route SDL input to the active (topmost) window only. If consumed, do not let the game
        // or other UI buttons see it.
        //
        // IMPORTANT: touch can generate both SDL_FINGER* and SDL_MOUSE* (SDL_TOUCH_MOUSEID).
        // When a modal Clay window is/was open for this event, we must consume *all* pointer
        // events to prevent click-through (including the extra synthesized event after closing).
        const int prevWindowCount = usr->windowStack.count;
        const bool modalWasOpen = prevWindowCount > 0;
        if (AppInactiveOverlayHandleOrphanPointerEvent(usr, e))
        {
            continue;
        }
	        if (usr->windowStack.processActiveWindowEvent(
	                &usr->clayton,
	                &usr->keypad,
	                &usr->storage,
	                &usr->sound.settings,
	                &usr->adaptiveAudio,
	                &usr->localHi,
	                &usr->carousel,
                    &usr->housesCarousel,
                    &usr->botsCarousel,
                    &usr->tracker,
                    &usr->school.massSlider,
                    &usr->settings,
	                &usr->shouldShowShop,
	                e
	            ))
	        {
                const int newWindowCount = usr->windowStack.count;
                if (usr->gameMode == UserContext::GameMode::SCHOOL && newWindowCount < prevWindowCount)
                {
                    // Closing any modal window in school returns you to a consistent state:
                    // ball at idle start position + absolute mouse mode.
                    UI_ResetToIdleAndAbsolute(usr, (float)deltaTime, "SCHOOL_WINDOW_CLOSED_TO_IDLE");
                }
		            if (usr->windowStack.oilReoilRequested)
		            {
		                usr->windowStack.oilReoilRequested = false;
                        // School lesson 4: re-oil is free and restores the lesson oil defaults.
                        if (usr->gameMode == UserContext::GameMode::SCHOOL && usr->school.selectedLesson == 4)
                        {
                            if (School_OilLessonCanReoil(usr))
                            {
                                School_ApplyOilLessonDefaults(usr);
                                usr->school.spinSafeCoins = glm::clamp(usr->school.spinSafeCoins + 1, 0, 3);
                                usr->sound.playSfxBuy();
                                {
                                    glm::vec3 p = usr->ballStart;
                                    p.y += 0.35f;
                                    usr->particles.burstConfetti(p);
                                }

                                if (usr->school.spinSafeCoins >= 3)
                                {
                                    usr->school.lessonDone[3] = true;
                                    usr->school.unlockedLessons = glm::max(usr->school.unlockedLessons, 5);
                                    // Defer completion story until the player closes the Oil window.
                                    g_schoolOilLessonCompletionPending = true;
                                }
                            }
                        }
		                else if (usr->carousel.bank >= 10.0f)
		                {
		                    usr->carousel.bank -= 10.0f;
		                    ApplyHouseLaneParams(usr);
		                    usr->sound.playSfxBuy();
		                }
		            }
                if (usr->windowStack.housesSelectRequested)
                {
                    usr->windowStack.housesSelectRequested = false;
                    const int idx = usr->housesCarousel.closestHouseIdx;
                    if (idx >= 0 && idx < usr->housesCarousel.cardCount)
                    {
                        const HouseCatalogItem *house = &usr->housesCarousel.items[idx];
                        if (usr->selectorFlowStep != SelectorFlowStep::NONE)
                        {
                            usr->selectedHouseId = house->id;
                            SelectorFlow_OpenStep(usr, SelectorFlowStep::BALL);
                        }
                        else
                        {
                            ApplyHouseCatalogToUser(usr, house);
                        }
                    }
                }
                if (usr->windowStack.housesCloseRequested)
                {
                    usr->windowStack.housesCloseRequested = false;
                    if (usr->selectorFlowStep != SelectorFlowStep::NONE)
                        SelectorFlow_Cancel(usr);
                }
                if (usr->windowStack.botSelectRequested)
                {
                    usr->windowStack.botSelectRequested = false;

                    BotAvatar picked = BotAvatar::ANGEL;
                    switch (usr->windowStack.botSelectedKind)
                    {
                    case 1: picked = BotAvatar::CHERUB; break;
                    case 2: picked = BotAvatar::SERAPH; break;
                    case 3: picked = BotAvatar::THRONE; break;
                    default: picked = BotAvatar::ANGEL; break;
                    }
                    if (usr->selectorFlowStep == SelectorFlowStep::BOT)
                    {
                        usr->selectedFreestyleAvatar = picked;
                        SelectorFlow_OpenStep(usr, SelectorFlowStep::HOUSE);
                    }
                    else
                    {
                        usr->botAvatar = picked;
                        usr->shouldShowShop = false;
                        usr->clayton.shouldShowHiScore = false;
                        usr->clayton.shouldShowHiScoreWithLatest = false;
                        usr->clayton.shouldShowOilStatus = false;
                        usr->clayton.shouldShowHouses = false;
                        usr->clayton.shouldShowBotSelect = false;
                        usr->windowStack.count = 0;
                        Bot_InitIfNeeded(usr);
                        Bot_PlayArgumentIfPossible(usr, /*resetTime=*/true);
                    }
                }
                if (usr->windowStack.botSelectCloseRequested)
                {
                    usr->windowStack.botSelectCloseRequested = false;
                    if (usr->selectorFlowStep != SelectorFlowStep::NONE)
                        SelectorFlow_Cancel(usr);
                }
                if (usr->windowStack.greetingsReadyRequested)
                {
                    usr->windowStack.greetingsReadyRequested = false;
                    usr->greetingsSeen = true;
                    usr->trackerSongFilePickerActive = false;
                    usr->trackerSongFilePickerFocusGraceFrames = 0;
                    usr->appFocusLost = false;
                    usr->appInactiveOverlayActive = false;
                    if (usr->sound.audioStoppedBecauseWindowLeave || usr->sound.browserAudioSuspended || !usr->sound.audioDev)
                        usr->sound.resumeFromBrowser(usr->sound.getSongPlaybackPattern(usr->sound.currentSongIndex));
                    usr->storage.setChar(Storage::GREETINGS_SEEN, "1", 1);
                }
                if (usr->windowStack.menuRenameRequested)
                {
                    usr->windowStack.menuRenameRequested = false;
                    usr->windowStack.windowStackPushKeypadEditor(
                        &usr->keypad, "Enter Username", usr->username, &usr->username_len
                    );
                }
                if (usr->windowStack.menuCampaignRequested)
                {
                    usr->windowStack.menuCampaignRequested = false;
                    SelectorFlow_Cancel(usr);
                    Campaign_ApplyCurrentLevelSetup(usr, /*resetStoryKick=*/true);
                    Run_ResetBoardsAndMode(usr, usr->gameMode);
                }
                if (usr->windowStack.menuSchoolRequested)
                {
                    usr->windowStack.menuSchoolRequested = false;
                    SelectorFlow_Cancel(usr);
                    EnterSchool(usr, /*playStory=*/true);
                }
                if (usr->windowStack.languageEnglishRequested)
                {
                    usr->windowStack.languageEnglishRequested = false;
                    usr->language = TXL_LANG_EN_US;
                    usr->clayton.loadFontsForLanguage(usr->language);
                    usr->dialog.language = usr->language;
                    usr->storage.setChar(
                        Storage::LANGUAGE,
                        Txl_LanguageStorageValue(usr->language),
                        strlen(Txl_LanguageStorageValue(usr->language))
                    );
                    Campaign_SetResultWindowLabels(usr, usr->playerRoute == PlayerRoute::CAMPAIGN &&
                                                            usr->campaignLevelIndex > 1);
                }
                if (usr->windowStack.languageChineseRequested)
                {
                    usr->windowStack.languageChineseRequested = false;
                    usr->language = TXL_LANG_ZH_CN;
                    usr->clayton.loadFontsForLanguage(usr->language);
                    usr->dialog.language = usr->language;
                    usr->storage.setChar(
                        Storage::LANGUAGE,
                        Txl_LanguageStorageValue(usr->language),
                        strlen(Txl_LanguageStorageValue(usr->language))
                    );
                    Campaign_SetResultWindowLabels(usr, usr->playerRoute == PlayerRoute::CAMPAIGN &&
                                                            usr->campaignLevelIndex > 1);
                }
                if (usr->windowStack.languageLithuanianRequested)
                {
                    usr->windowStack.languageLithuanianRequested = false;
                    usr->language = TXL_LANG_LT_LT;
                    usr->clayton.loadFontsForLanguage(usr->language);
                    usr->dialog.language = usr->language;
                    usr->storage.setChar(
                        Storage::LANGUAGE,
                        Txl_LanguageStorageValue(usr->language),
                        strlen(Txl_LanguageStorageValue(usr->language))
                    );
                    Campaign_SetResultWindowLabels(usr, usr->playerRoute == PlayerRoute::CAMPAIGN &&
                                                            usr->campaignLevelIndex > 1);
                }
                if (usr->windowStack.languageJapaneseRequested)
                {
                    usr->windowStack.languageJapaneseRequested = false;
                    usr->language = TXL_LANG_JP_JP;
                    usr->clayton.loadFontsForLanguage(usr->language);
                    usr->dialog.language = usr->language;
                    usr->storage.setChar(
                        Storage::LANGUAGE,
                        Txl_LanguageStorageValue(usr->language),
                        strlen(Txl_LanguageStorageValue(usr->language))
                    );
                    Campaign_SetResultWindowLabels(usr, usr->playerRoute == PlayerRoute::CAMPAIGN &&
                                                            usr->campaignLevelIndex > 1);
                }
                if (usr->windowStack.menuPracticeRequested)
                {
                    usr->windowStack.menuPracticeRequested = false;
                    usr->playerRoute = PlayerRoute::PRACTICE;
                    SelectorFlow_OpenStep(usr, SelectorFlowStep::HOUSE);
                }
                if (usr->windowStack.menuFreestyleRequested)
                {
                    usr->windowStack.menuFreestyleRequested = false;
                    usr->playerRoute = PlayerRoute::FREESTYLE;
                    SelectorFlow_OpenStep(usr, SelectorFlowStep::BOT);
                }
                if (usr->windowStack.menuDeviceShareRequested)
                {
                    usr->windowStack.menuDeviceShareRequested = false;
                }
                if (usr->windowStack.menuTrackerRequested)
                {
                    usr->windowStack.menuTrackerRequested = false;
                    if (!usr->sound.useWavPlayback && !usr->sound.audioDisabled)
                        EnterTracker(usr);
                }
                if (usr->windowStack.settingsResetProgressRequested)
                {
                    usr->windowStack.settingsResetProgressRequested = false;
                    Progress_ResetCampaign(usr);
                    Campaign_ApplyCurrentLevelSetup(usr, /*resetStoryKick=*/true);
                }
                Tracker_ApplyInstrumentNameKeypadResult(usr);
                Tracker_OpenInstrumentNameKeypadIfRequested(usr);
                Tracker_ApplyPartNameKeypadResult(usr);
                Tracker_OpenPartNameKeypadIfRequested(usr);
                Tracker_ApplySongNameKeypadResult(usr);
                Tracker_OpenSongNameKeypadIfRequested(usr);
                Tracker_PlayRequestedPreview(usr);
                if (usr->tracker.instrumentEditorWindowRequested)
                {
                    usr->tracker.instrumentEditorWindowRequested = false;
                    Tracker_LoadPatchFromSound(usr, usr->tracker.editInstrument);
                    usr->windowStack.windowStackPushTrackerInstrumentEditorWindow();
                }
                if (usr->tracker.instrumentColorWindowRequested)
                {
                    usr->tracker.instrumentColorWindowRequested = false;
                    usr->windowStack.windowStackPushTrackerInstrumentColorWindow();
                }
                if (usr->tracker.operatorEditorWindowRequested)
                {
                    usr->tracker.operatorEditorWindowRequested = false;
                    usr->windowStack.windowStackPushTrackerOperatorEditorWindow();
                }
                if (usr->tracker.instrumentsWindowRequested)
                {
                    usr->tracker.instrumentsWindowRequested = false;
                    usr->windowStack.windowStackPushTrackerInstrumentsWindow();
                }
                if (usr->tracker.songSettingsWindowRequested)
                {
                    usr->tracker.songSettingsWindowRequested = false;
                    usr->windowStack.windowStackPushTrackerSongSettingsWindow();
                }
                if (usr->tracker.partEditorWindowRequested)
                {
                    usr->tracker.partEditorWindowRequested = false;
                    usr->windowStack.windowStackPushTrackerPartEditorWindow();
                }
                if (usr->tracker.songSaveConfirmWindowRequested)
                {
                    usr->tracker.songSaveConfirmWindowRequested = false;
                    usr->windowStack.windowStackPushTrackerSaveConfirmWindow();
                }
                if (usr->tracker.songSaveRequested)
                {
                    usr->tracker.songSaveRequested = false;
                    Tracker_SaveSongToBrowser(usr);
                }
		            continue;
	        }

        if (usr->gameMode == UserContext::GameMode::TRACKER)
        {
            if (usr->tracker.active && Tracker_HandleEvent(&usr->tracker, &usr->clayton, e))
            {
                if (usr->tracker.editorWindowRequested)
                {
                    usr->tracker.editorWindowRequested = false;
                    usr->windowStack.windowStackPushTrackerEditorWindow();
                }
                if (usr->tracker.instrumentEditorWindowRequested)
                {
                    usr->tracker.instrumentEditorWindowRequested = false;
                    Tracker_LoadPatchFromSound(usr, usr->tracker.editInstrument);
                    usr->windowStack.windowStackPushTrackerInstrumentEditorWindow();
                }
                if (usr->tracker.instrumentColorWindowRequested)
                {
                    usr->tracker.instrumentColorWindowRequested = false;
                    usr->windowStack.windowStackPushTrackerInstrumentColorWindow();
                }
                if (usr->tracker.operatorEditorWindowRequested)
                {
                    usr->tracker.operatorEditorWindowRequested = false;
                    usr->windowStack.windowStackPushTrackerOperatorEditorWindow();
                }
                if (usr->tracker.instrumentsWindowRequested)
                {
                    usr->tracker.instrumentsWindowRequested = false;
                    usr->windowStack.windowStackPushTrackerInstrumentsWindow();
                }
                if (usr->tracker.songSettingsWindowRequested)
                {
                    usr->tracker.songSettingsWindowRequested = false;
                    usr->windowStack.windowStackPushTrackerSongSettingsWindow();
                }
                if (usr->tracker.partEditorWindowRequested)
                {
                    usr->tracker.partEditorWindowRequested = false;
                    usr->windowStack.windowStackPushTrackerPartEditorWindow();
                }
                if (usr->tracker.songSaveConfirmWindowRequested)
                {
                    usr->tracker.songSaveConfirmWindowRequested = false;
                    usr->windowStack.windowStackPushTrackerSaveConfirmWindow();
                }
                if (usr->tracker.songSaveRequested)
                {
                    usr->tracker.songSaveRequested = false;
                    Tracker_SaveSongToBrowser(usr);
                }
                if (usr->tracker.songLoadEmptyRequested)
                {
                    usr->tracker.songSettingsWindowOpen = false;
                    usr->tracker.songSettingsWindowRequested = false;
                    Tracker_LoadEmptyUserSong(usr);
                }
                if (usr->tracker.songLoadRequested)
                {
                    usr->tracker.songLoadRequested = false;
                    Tracker_OpenSongLoadDialog(usr);
                }
                Tracker_ApplyTransportRequests(usr);
                Tracker_PlayRequestedPreview(usr);
                Tracker_OpenInstrumentNameKeypadIfRequested(usr);
                Tracker_OpenPartNameKeypadIfRequested(usr);
                Tracker_OpenSongNameKeypadIfRequested(usr);
                if (!usr->tracker.active)
                    ExitTracker(usr);
            }
            continue;
        }

            // Story dialog is shown only when there are no other modal windows on the stack.
            // While it is active, it must consume pointer events and block gameplay/UI openers.
            if (usr->windowStack.count == 0 && usr->dialog.active)
            {
                // When the dialog is waiting for a choice, ensure we are in normal mouse mode
                // (no relative mouse capture), so the player can click options reliably.
                if (usr->dialog.waitingChoice)
                {
                    SDL_SetRelativeMouseMode(SDL_FALSE);
                }

                if (usr->dialog.processEvent(&usr->clayton, e))
                {
                    if (usr->dialog.closeRequested)
                    {
                        usr->dialog.finalizeClose();
                    }
                    continue;
                }

                const bool isPointerEvent =
                    (e.type == SDL_MOUSEBUTTONDOWN) || (e.type == SDL_MOUSEBUTTONUP) ||
                    (e.type == SDL_MOUSEMOTION) || (e.type == SDL_MOUSEWHEEL) ||
                    (e.type == SDL_FINGERDOWN) || (e.type == SDL_FINGERUP) || (e.type == SDL_FINGERMOTION);
                if (isPointerEvent)
                {
                    continue;
                }
            }

            // Any visible Clay window should be modal: never let pointer events click-through into gameplay.
            // This prevents close-button mouse-down from triggering a throw (close buttons fire on mouse-up).
            // Also covers touch-to-mouse synthesized events that may arrive after the window closes.
            if (modalWasOpen)
            {
                const bool isPointerEvent =
                    (e.type == SDL_MOUSEBUTTONDOWN) || (e.type == SDL_MOUSEBUTTONUP) ||
                    (e.type == SDL_MOUSEMOTION) || (e.type == SDL_MOUSEWHEEL) ||
                    (e.type == SDL_FINGERDOWN) || (e.type == SDL_FINGERUP) || (e.type == SDL_FINGERMOTION);
                if (isPointerEvent)
                {
                    continue;
                }
            }
        // Enemy turn is fully automated: block gameplay inputs and HUD openers.
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_F7)
        {
            usr->botAvatar = (usr->botAvatar == BotAvatar::ANGEL)
                                 ? BotAvatar::CHERUB
                                 : (usr->botAvatar == BotAvatar::CHERUB) ? BotAvatar::SERAPH
                                 : (usr->botAvatar == BotAvatar::SERAPH) ? BotAvatar::THRONE
                                                                        : BotAvatar::ANGEL;
            // Force re-seed of hand-attached render ball and restart idle animation.
            usr->enemyBallRenderPosValid = false;
            Bot_InitIfNeeded(usr);
            Bot_PlayArgumentIfPossible(usr, /*resetTime=*/true);
        }
        if (e.type == SDL_KEYDOWN && (e.key.keysym.sym == SDLK_F8 || e.key.keysym.sym == SDLK_F9))
        {
            float *scale =
                (usr->botAvatar == BotAvatar::CHERUB) ? &usr->cherubModelScale :
                (usr->botAvatar == BotAvatar::SERAPH) ? &usr->seraphModelScale :
                (usr->botAvatar == BotAvatar::THRONE) ? &usr->throneModelScale :
                                                       &usr->angelModelScale;
            const float mul = (e.key.keysym.sym == SDLK_F8) ? 1.10f : (1.0f / 1.10f);
            *scale = glm::clamp((*scale) * mul, 0.001f, 0.2f);
            std::cerr << "[bot-avatar] " << ((usr->botAvatar == BotAvatar::CHERUB) ? "cherub" :
                                             (usr->botAvatar == BotAvatar::SERAPH) ? "seraph" :
                                             (usr->botAvatar == BotAvatar::THRONE) ? "throne" : "angel")
                      << " scale=" << *scale << "\n";
        }
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_F10)
        {
            usr->sound.playSfxGlassBreak();
        }
        if (usr->gameMode == UserContext::GameMode::BOT && IsEnemyTurn(usr))
        {
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
                PhysicsResetForMode(usr, /*reviveAll=*/true);
                LogToIdle(usr, "SPACE_RESET");
                usr->phase = UserContext::Phase::IDLE;
                usr->wereDead = 0;
                usr->electroBall.resetCharge();
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

        // While a story dialog is running, it is fully modal: do not allow any other window openers.
        if (usr->dialog.active)
        {
            // We already routed/consumed dialog-specific events above (when no other windows are open).
            // Here we just prevent openers from reacting to the same click/touch.
            const bool isPointerEvent =
                (e.type == SDL_MOUSEBUTTONDOWN) || (e.type == SDL_MOUSEBUTTONUP) ||
                (e.type == SDL_MOUSEMOTION) || (e.type == SDL_MOUSEWHEEL) ||
                (e.type == SDL_FINGERDOWN) || (e.type == SDL_FINGERUP) || (e.type == SDL_FINGERMOTION);
            if (isPointerEvent)
            {
                continue;
            }
        }

        if (isClaytonClicked(&usr->renameButton, e))
        {
            // The top-left slot is now the charge meter instead of the rename button.
            // Username editing remains available from the menu.
            continue;
        }
        if (isClaytonClicked(&usr->menuButton, e))
        {
            if (usr->gameMode != UserContext::GameMode::SCHOOL)
                usr->windowStack.windowStackPushMenuWindow();
            continue;
        }
        if (isClaytonClicked(&usr->soundButton, e))
        {
            if (usr->gameMode == UserContext::GameMode::SCHOOL) continue;
            usr->sound.showSoundSettings();
            usr->windowStack.windowStackPushSoundSettingsWindow();
            continue;
        }
        if (usr->gameMode == UserContext::GameMode::SCHOOL)
        {
            // Mass editor opener: only in Lesson 2 (Mass).
            // Implement click tracking without adding new persistent fields.
            static bool s_massOpenDown = false;
            static bool s_oilOpenDown = false;
            const bool isDownEv = (e.type == SDL_MOUSEBUTTONDOWN) || (e.type == SDL_FINGERDOWN);
            const bool isUpEv = (e.type == SDL_MOUSEBUTTONUP) || (e.type == SDL_FINGERUP);
            const bool isMoveEv = (e.type == SDL_MOUSEMOTION) || (e.type == SDL_FINGERMOTION);
            if (usr->school.selectedLesson == 2)
            {
                const Clay_ElementId massBtn = CLAY_ID("SchoolMassEditorOpen");
                const bool over = Clay_PointerOver(massBtn);
                if (s_massOpenDown)
                {
                    if (isMoveEv && !over)
                        s_massOpenDown = false;
                    if (isUpEv)
                    {
                        s_massOpenDown = false;
                        if (over)
                        {
                            usr->windowStack.windowStackPushMassEditorWindow();
                            continue;
                        }
                    }
                }
                else
                {
                    if (isDownEv && over)
                    {
                        s_massOpenDown = true;
                        continue;
                    }
                }
            }

            // Oil lesson opener: only in Lesson 4 (Oil).
            if (usr->school.selectedLesson == 4)
            {
                const Clay_ElementId oilBtn = CLAY_ID("SchoolOilWindowOpen");
                const bool over = Clay_PointerOver(oilBtn);
                if (s_oilOpenDown)
                {
                    if (isMoveEv && !over)
                        s_oilOpenDown = false;
                    if (isUpEv)
                    {
                        s_oilOpenDown = false;
                        if (over)
                        {
                            usr->clayton.shouldShowHouses = false;
                            usr->clayton.shouldShowOilStatus = true;
                            usr->windowStack.windowStackPushOilStatusWindow();
                            continue;
                        }
                    }
                }
                else
                {
                    if (isDownEv && over)
                    {
                        s_oilOpenDown = true;
                        continue;
                    }
                }
            }

            int desiredLesson = 0;
            bool exitRequested = false;
            bool massChanged = false;
            float newMassKg = 0.0f;

            if (School_ClayHandleEvent(
                    &usr->school,
                    e,
                    /*currentGameModeIsSchool=*/1,
                    &desiredLesson,
                    &exitRequested,
                    &massChanged,
                    &newMassKg
                ))
            {
                if (exitRequested)
                {
                    School_Exit(usr);
                    continue;
                }

                // Mass slider is now edited via the Mass Editor window (WindowKind_MassEditor),
                // so `massChanged` is not used here anymore.

                if (desiredLesson != 0)
                {
                    const int prevLesson = usr->school.selectedLesson;
                    SchoolServices svc = {};
                    svc.phy = &usr->phy;
                    svc.coinLane = &usr->coinLane;
                    svc.dialog = (usr->windowStack.count == 0 && !usr->dialog.active) ? &usr->dialog : nullptr;
                    svc.myBall = &usr->myBall;
                    svc.ballStatsLightnessBuff = BallStats_LightnessBuff;
                    svc.ballStatsRestitutionMassScale = BallStats_RestitutionMassScale;
                    svc.remapClamped = remapClamped;
                    svc.catalogBuffMin = BallPhysicsMapping::CATALOG_BUFF_MIN;
                    svc.catalogBuffMax = BallPhysicsMapping::CATALOG_BUFF_MAX;
                    svc.physicsArmImpulseMin = BallPhysicsMapping::PHYSICS_ARM_IMPULSE_MIN;
                    svc.physicsArmImpulseMax = BallPhysicsMapping::PHYSICS_ARM_IMPULSE_MAX;

                    SchoolRuntimeTuning rt = {};
                    rt.desiredMassKg = &usr->desiredMass;
                    rt.lightnessBuff = &usr->lightnessBuff;
                    rt.launchBuffEffective = &usr->launchBuffEffective;
                    rt.armImpulseAtThrow = &usr->armImpulseAtThrow;
                    rt.angularFactor = &usr->angularFactor;
                    rt.ballSkid = &usr->ballSkid;
                    rt.ballSkidStartScale = &usr->ballSkidStartScale;
                    rt.ballBaseFriction = &usr->ballBaseFriction;
                    rt.laneOilThickness = &usr->laneOilThickness;
                    rt.ballRestitution = &usr->ballRestitution;

                    School_SelectLesson(&usr->school, svc, rt, desiredLesson, /*playStory=*/true);
                    usr->electroBall.resetCharge();
                    School_ApplyPinModeForSelectedLesson(usr);
                    if (usr->school.selectedLesson == 4)
                        School_ApplyOilLessonDefaults(usr);
                    else if (usr->school.selectedLesson == 5)
                    {
                        g_schoolStrikeBallBeforeLesson = usr->myBall.id;
                        g_schoolStrikeLaneRestitutionBase = glm::clamp(usr->laneRestitution, 0.0f, 1.0f);
                        g_schoolStrikeLaneRestitutionActive = true;
                        g_schoolStrikeArmImpulseBase = usr->armImpulseAtThrow;
                        g_schoolStrikeArmImpulseActive = true;
                        School_ApplyStrikeLaneDefaults(usr);
                    }
                    else
                        School_ApplyNeutralLaneDefaults(usr);
                    if (usr->school.selectedLesson == 5)
                    {
                        g_schoolStrikeAimLeftPocket = true;
                        School_StrikeLessonSetupCoins(usr, g_schoolStrikeAimLeftPocket);
                        g_schoolStrikeSwapInProgress = false;
                        g_schoolStrikeSwapElapsed = SCHOOL_STRIKE_SWAP_INTERVAL_S;
                    }
                    else if (prevLesson == 4)
                    {
                        BallStats_ApplyFrictionOnly(usr, usr->myBall);
                        BallStats_ApplyLaunchImpulseOnly(usr);
                    }
                    else if (prevLesson == 5 && g_schoolStrikeBallBeforeLesson >= 0)
                    {
                        BallStats_OnBallChange(&g_ballCatalog[g_schoolStrikeBallBeforeLesson], usr);
                        g_schoolStrikeBallBeforeLesson = -1;
                        g_schoolStrikeFailedAttempts = 0;
                        g_schoolStrikeHelpPending = false;
                        g_schoolStrikeSwapInProgress = false;
                        g_schoolStrikeSwapElapsed = SCHOOL_STRIKE_SWAP_INTERVAL_S;
                        g_schoolStrikeSwapTargetLeftPocket = true;
                        if (g_schoolStrikeLaneRestitutionActive && g_schoolStrikeLaneRestitutionBase >= 0.0f)
                        {
                            usr->laneRestitution = glm::clamp(g_schoolStrikeLaneRestitutionBase, 0.0f, 1.0f);
                        }
                        g_schoolStrikeLaneRestitutionActive = false;
                        if (g_schoolStrikeArmImpulseActive && g_schoolStrikeArmImpulseBase > 0.0f)
                        {
                            usr->armImpulseAtThrow = glm::clamp(
                                g_schoolStrikeArmImpulseBase,
                                BallPhysicsMapping::PHYSICS_ARM_IMPULSE_MIN,
                                BallPhysicsMapping::PHYSICS_ARM_IMPULSE_MAX
                            );
                        }
                        g_schoolStrikeArmImpulseActive = false;
                    }
                    // Switching lessons in school always returns to idle start position + absolute mouse.
                    UI_ResetToIdleAndAbsolute(usr, (float)deltaTime, "SCHOOL_SWITCH_LESSON_TO_IDLE");
                    continue;
                }

                continue;
            }
        }
        if (isClaytonClicked(&usr->oilButton, e))
        {
            if (usr->gameMode == UserContext::GameMode::SCHOOL) continue;
            usr->clayton.shouldShowHouses = false;
	            usr->clayton.shouldShowOilStatus = true;
	            usr->windowStack.windowStackPushOilStatusWindow();
	            continue;
	        }
        if (isClaytonClicked(&usr->hiScoreButton, e))
        {
            if (usr->gameMode == UserContext::GameMode::SCHOOL || usr->playerRoute != PlayerRoute::FREESTYLE) continue;
            usr->clayton.shouldShowHiScore = true;
            usr->clayton.shouldShowHiScoreWithLatest = false;
            usr->windowStack.windowStackPushLocalHiscoreWindow();
            continue;
        }

        if (isClaytonClicked(&usr->openShopClick, e))
        {
            if (usr->gameMode == UserContext::GameMode::SCHOOL) continue;
            // Opening shop is a modal UX; reset to a consistent idle state (like school window closes).
            UI_ResetToIdleAndAbsolute(usr, (float)deltaTime, "SHOP_OPEN_TO_IDLE");
            usr->shouldShowShop = true;
            SDL_SetRelativeMouseMode(SDL_FALSE);
            usr->windowStack.windowStackPushShopWindow();
            continue;
        }

        // HUD window-open buttons are Clay UI; prevent pointer-down click-through into gameplay
        // (Clay clicks fire on mouse-up, but gameplay reacts on mouse-down).
        {
            const bool isPointerEvent =
                (e.type == SDL_MOUSEBUTTONDOWN) || (e.type == SDL_MOUSEBUTTONUP) ||
                (e.type == SDL_MOUSEMOTION) || (e.type == SDL_MOUSEWHEEL) ||
                (e.type == SDL_FINGERDOWN) || (e.type == SDL_FINGERUP) || (e.type == SDL_FINGERMOTION);
            if (isPointerEvent)
            {
                const bool overHudButton =
                    Clay_PointerOver(usr->renameButton.clayId) ||
                    Clay_PointerOver(usr->menuButton.clayId) ||
                    Clay_PointerOver(usr->soundButton.clayId) ||
                    Clay_PointerOver(usr->oilButton.clayId) ||
                    Clay_PointerOver(usr->housesButton.clayId) ||
                    Clay_PointerOver(usr->hiScoreButton.clayId) ||
                    Clay_PointerOver(usr->openShopClick.clayId);
                if (overHudButton)
                {
                    continue;
                }
            }
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
                auto applyTouchAimMargin = [](float &inOut01)
                {
                    // Virtual joystick margin: treat a thin border near screen edges as "already at the edge".
                    // This makes it easier to reach max pullback/swing without dragging to the physical edge.
                    constexpr float kMargin = 0.03f; // 3% on each side
                    float v = glm::clamp(inOut01, 0.0f, 1.0f);
                    v = glm::clamp(v, kMargin, 1.0f - kMargin);
                    inOut01 = (v - kMargin) / (1.0f - 2.0f * kMargin);
                };
                if (e.button.which == SDL_TOUCH_MOUSEID)
                {
                    applyTouchAimMargin(x);
                    applyTouchAimMargin(y);
                }

                // Default behavior: start AIM from the click/touch point.
                // School Lesson 3 wants to start from neutral so the pullback meter begins at 0.
                if (usr->gameMode == UserContext::GameMode::SCHOOL && usr->school.selectedLesson == 1)
                {
                    usr->aimFlatPos = glm::vec2(0.5f, 0.5f);
                }
                else
                {
                    usr->aimFlatPos.x = x;
                    usr->aimFlatPos.y = y;
                }
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
                    auto applyTouchAimMargin = [](float &inOut01)
                    {
                        constexpr float kMargin = 0.03f;
                        float v = glm::clamp(inOut01, 0.0f, 1.0f);
                        v = glm::clamp(v, kMargin, 1.0f - kMargin);
                        inOut01 = (v - kMargin) / (1.0f - 2.0f * kMargin);
                    };
                    if (e.motion.which == SDL_TOUCH_MOUSEID)
                    {
                        applyTouchAimMargin(x);
                        applyTouchAimMargin(y);
                    }
                    usr->aimFlatPos.x = x;
                    usr->aimFlatPos.y = y;
                }

                // SDL coordinates: y increases downward. Track the maximum downward delta.
                float downDelta = usr->aimFlatPos.y - usr->aimDownFlatPos.y;

                // School Lesson 3: the very first touch/mouse-move can jump the absolute position,
                // which would instantly give a non-zero pullback bar. Re-base the "start" point
                // until the player actually drags downward meaningfully.
                if (usr->gameMode == UserContext::GameMode::SCHOOL && usr->school.selectedLesson == 1)
                {
                    const float kPullStartThreshold = 0.02f; // ~2% of screen in [0..1]
                    if (downDelta < kPullStartThreshold)
                    {
                        usr->aimDownFlatPos = usr->aimFlatPos;
                        downDelta = 0.0f;
                    }
                }
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
                    auto applyTouchAimMargin = [](float &inOut01)
                    {
                        constexpr float kMargin = 0.03f;
                        float v = glm::clamp(inOut01, 0.0f, 1.0f);
                        v = glm::clamp(v, kMargin, 1.0f - kMargin);
                        inOut01 = (v - kMargin) / (1.0f - 2.0f * kMargin);
                    };
                    if (e.motion.which == SDL_TOUCH_MOUSEID)
                    {
                        applyTouchAimMargin(x);
                        applyTouchAimMargin(y);
                    }
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

                if (e.motion.which == SDL_TOUCH_MOUSEID)
                {
                    auto applyTouchAimMargin = [](float &inOut01)
                    {
                        constexpr float kMargin = 0.03f;
                        float v = glm::clamp(inOut01, 0.0f, 1.0f);
                        v = glm::clamp(v, kMargin, 1.0f - kMargin);
                        inOut01 = (v - kMargin) / (1.0f - 2.0f * kMargin);
                    };
                    applyTouchAimMargin(x);
                    applyTouchAimMargin(y);
                }
                usr->aimFlatPos.x = x;
                usr->aimFlatPos.y = y;
                spinMove.x = x_rel;
                spinMove.y = y_rel;
            }
        }
    }

    // School Lesson 4 completion: show completion story after the player closes the Oil window.
    // This must be per-frame (not tied to SDL events), otherwise the dialog may never open
    // if the player stops moving/clicking right after completion.
    if (usr->gameMode == UserContext::GameMode::SCHOOL && usr->school.selectedLesson == 4)
    {
        const bool oilOpenNow = usr->clayton.shouldShowOilStatus;
        const bool oilJustClosed = (g_schoolOilStatusWasOpen && !oilOpenNow);
        g_schoolOilStatusWasOpen = oilOpenNow;

        // Only fire once the Oil window is actually closed.
        if (g_schoolOilLessonCompletionPending && oilJustClosed)
        {
            if (usr->windowStack.count == 0 && !usr->dialog.active)
            {
                g_schoolOilLessonCompletionPending = false;
                usr->dialog.open(1060);
            }
        }
    }
    else
    {
        // Leaving lesson 4 cancels any pending completion prompt.
        g_schoolOilLessonCompletionPending = false;
        g_schoolOilStatusWasOpen = false;
    }

    // School Lesson 5: automatically swap the gem lane every 10 seconds with a 1 second slide.
    if (usr->gameMode == UserContext::GameMode::SCHOOL && usr->school.selectedLesson == 5)
    {
        School_StrikeLessonTickSwap(usr, deltaTime);
    }

    // School Lesson 5: ensure the coin guide line is visible as soon as the lesson is entered (IDLE),
    // and respawn it if the player collected them.
    if (usr->gameMode == UserContext::GameMode::SCHOOL &&
        usr->school.selectedLesson == 5 &&
        usr->phase == UserContext::Phase::IDLE)
    {
        bool anyActive = false;
        for (int i = 0; i < usr->coinLane.activeCount; i++)
        {
            if (usr->coinLane.coins[i].state == CoinState::Active)
            {
                anyActive = true;
                break;
            }
        }
        if (!anyActive)
        {
            School_StrikeLessonSetupCoins(usr, g_schoolStrikeAimLeftPocket);
            g_schoolStrikeSwapInProgress = false;
            g_schoolStrikeSwapElapsed = SCHOOL_STRIKE_SWAP_INTERVAL_S;
        }
    }

    // School Lesson 5: show helper-ball prompt every 5 failed attempts (once we're not in another modal).
    if (usr->gameMode == UserContext::GameMode::SCHOOL &&
        usr->school.selectedLesson == 5 &&
        g_schoolStrikeHelpPending &&
        usr->windowStack.count == 0 &&
        !usr->dialog.active)
    {
        g_schoolStrikeHelpPending = false;
        usr->dialog.open(1080);
    }

    // Story dialog events (emitted once when a storyline node finishes typing, or when an option triggers).
    // Kept here (after SDL polling) so a dialog can emit an event and the game reacts on the next tick.
	    {
	        const int32_t storyEvent = usr->dialog.consumeEvent();
		        if (storyEvent != EVENT_NONE)
		        {
                    SchoolServices schoolSvc = {};
                    schoolSvc.phy = &usr->phy;
                    schoolSvc.coinLane = &usr->coinLane;
                    schoolSvc.dialog = (usr->windowStack.count == 0 && !usr->dialog.active) ? &usr->dialog : nullptr;
                    schoolSvc.myBall = &usr->myBall;
                    schoolSvc.ballStatsLightnessBuff = BallStats_LightnessBuff;
                    schoolSvc.ballStatsRestitutionMassScale = BallStats_RestitutionMassScale;
                    schoolSvc.remapClamped = remapClamped;
                    schoolSvc.catalogBuffMin = BallPhysicsMapping::CATALOG_BUFF_MIN;
                    schoolSvc.catalogBuffMax = BallPhysicsMapping::CATALOG_BUFF_MAX;
                    schoolSvc.physicsArmImpulseMin = BallPhysicsMapping::PHYSICS_ARM_IMPULSE_MIN;
                    schoolSvc.physicsArmImpulseMax = BallPhysicsMapping::PHYSICS_ARM_IMPULSE_MAX;

                    SchoolRuntimeTuning schoolRt = {};
                    schoolRt.desiredMassKg = &usr->desiredMass;
                    schoolRt.lightnessBuff = &usr->lightnessBuff;
                    schoolRt.launchBuffEffective = &usr->launchBuffEffective;
                    schoolRt.armImpulseAtThrow = &usr->armImpulseAtThrow;
                    schoolRt.angularFactor = &usr->angularFactor;
                    schoolRt.ballSkid = &usr->ballSkid;
                    schoolRt.ballSkidStartScale = &usr->ballSkidStartScale;
                    schoolRt.ballBaseFriction = &usr->ballBaseFriction;
                    schoolRt.laneOilThickness = &usr->laneOilThickness;
                    schoolRt.ballRestitution = &usr->ballRestitution;

		            if (storyEvent == EVENT_GO_TO_SCHOOL)
		            {
                        // Tutorial is optional before the first SOLO game is completed.
                        // Mandatory school is enabled only when we explicitly set `schoolExitLocked=true`
                        // (failed first milestone).
                        if (!usr->firstSoloCompleted)
                        {
                            usr->schoolExitLocked = false;
                        }
                        EnterSchool(usr, /*playStory=*/true);
		            }
	                    else if (storyEvent == EVENT_GO_TO_BOT)
	                    {
	                        if (usr->milestone100Reached)
	                        {
	                            usr->schoolDone = true;
	                            usr->storage.setChar(Storage::SCHOOL_DONE, "1", 1);
	                            usr->gameMode = UserContext::GameMode::BOT;
	
	                            // Reset vs state for a clean start.
	                            usr->turnOwner = UserContext::TurnOwner::PLAYER;
	                            usr->enemyAutoTimer = 0.0f;
	                            usr->enemyLaunched = false;
	                            usr->enemyDebugLogged = false;
	                            usr->enemyTurnSetup = false;
	                            resetScoreboard(&usr->enemyBoard);
	                            resetScoreboard(&usr->board);
	                            usr->wereDead = 0;
	                            usr->phase = UserContext::Phase::IDLE;
	                            PhysicsResetForMode(usr, /*reviveAll=*/true);
	                        }
	                        else
	                        {
	                            // Safety: BOT mode is locked until the player hits the 100-point milestone.
	                            // Ignore the request.
	                        }
	                    }
	                else if (storyEvent == EVENT_SCHOOL_SELECT_LESSON2)
	                {
	                    usr->school.unlockedLessons = glm::max(usr->school.unlockedLessons, 2);
	                    usr->school.lessonDone[0] = true;
	                    School_SelectLesson(&usr->school, schoolSvc, schoolRt, 2, /*playStory=*/true);
	                    usr->electroBall.resetCharge();
	                    School_ApplyPinModeForSelectedLesson(usr);
                        School_ApplyNeutralLaneDefaults(usr);
	                }
	                else if (storyEvent == EVENT_SCHOOL_PRACTICE_MASS_MORE)
	                {
	                    // Deprecated by EVENT_SCHOOL_EXIT (kept for compatibility if referenced by old story data).
	                    usr->school.unlockedLessons = glm::max(usr->school.unlockedLessons, 3);
	                    usr->school.lessonDone[1] = true;
	                }
	                else if (storyEvent == EVENT_SCHOOL_SELECT_LESSON3)
	                {
	                    usr->school.unlockedLessons = glm::max(usr->school.unlockedLessons, 3);
	                    usr->school.lessonDone[1] = true;
	                    School_SelectLesson(&usr->school, schoolSvc, schoolRt, 3, /*playStory=*/true);
	                    usr->electroBall.resetCharge();
	                    School_ApplyPinModeForSelectedLesson(usr);
                        School_ApplyNeutralLaneDefaults(usr);
	                }
                    else if (storyEvent == EVENT_SCHOOL_SELECT_LESSON4)
                    {
                        usr->school.unlockedLessons = glm::max(usr->school.unlockedLessons, 4);
                        usr->school.lessonDone[2] = true;
                        School_SelectLesson(&usr->school, schoolSvc, schoolRt, 4, /*playStory=*/true);
                        usr->electroBall.resetCharge();
                        School_ApplyPinModeForSelectedLesson(usr);
                        School_ApplyOilLessonDefaults(usr);
                    }
	                else if (storyEvent == EVENT_SCHOOL_PRACTICE_SPIN_MORE)
	                {
	                    usr->school.unlockedLessons = glm::max(usr->school.unlockedLessons, 3);
	                    usr->school.lessonDone[2] = true;
	                    School_SelectLesson(&usr->school, schoolSvc, schoolRt, 3, /*playStory=*/false);
	                    usr->electroBall.resetCharge();
	                    School_ApplyPinModeForSelectedLesson(usr);
	                }
		                else if (storyEvent == EVENT_SCHOOL_EXIT)
		                {
	                        if (usr->schoolExitLocked && !usr->milestone100Reached)
	                        {
	                            usr->school.exitConfirmPending = false;
	                            usr->dialog.open(30);
	                        }
	                        else
	                        {
		                        // If school isn't fully completed, show a short reminder before leaving.
		                        bool anyUncompleted = false;
		                        for (int i = 0; i < 5; i++)
		                            anyUncompleted |= !usr->school.lessonDone[i];
		
		                        if (!usr->school.exitConfirmPending && anyUncompleted)
		                        {
		                            usr->school.exitConfirmPending = true;
		                            usr->dialog.open(1030);
		                        }
		                        else
		                        {
		                            usr->school.exitConfirmPending = false;
		                            School_Exit(usr);
		                        }
	                        }
		                }
                    else if (storyEvent == EVENT_SCHOOL_SELECT_LESSON5)
                    {
                        const int prevLesson = usr->school.selectedLesson;
                        usr->school.unlockedLessons = glm::max(usr->school.unlockedLessons, 5);
                        usr->school.lessonDone[3] = true;
                        School_SelectLesson(&usr->school, schoolSvc, schoolRt, 5, /*playStory=*/true);
                        usr->electroBall.resetCharge();
                        School_ApplyPinModeForSelectedLesson(usr);
                        School_ApplyStrikeLaneDefaults(usr);
                        if (prevLesson == 4)
                        {
                            BallStats_ApplyFrictionOnly(usr, usr->myBall);
                            BallStats_ApplyLaunchImpulseOnly(usr);
                        }
                        g_schoolStrikeBallBeforeLesson = usr->myBall.id;
                        g_schoolStrikeFailedAttempts = 0;
                        g_schoolStrikeHelpPending = false;
                        g_schoolStrikeLaneRestitutionBase = glm::clamp(usr->laneRestitution, 0.0f, 1.0f);
                        g_schoolStrikeLaneRestitutionActive = true;
                        g_schoolStrikeArmImpulseBase = usr->armImpulseAtThrow;
                        g_schoolStrikeArmImpulseActive = true;
                        // Default strike line points into pocket between pins 1 and 2.
                        g_schoolStrikeAimLeftPocket = true;
                        School_StrikeLessonSetupCoins(usr, g_schoolStrikeAimLeftPocket);
                        g_schoolStrikeSwapInProgress = false;
                        g_schoolStrikeSwapElapsed = SCHOOL_STRIKE_SWAP_INTERVAL_S;
                    }
                    else if (storyEvent == EVENT_SCHOOL_STRIKE_HELP_ACCEPT)
                    {
                        // Restore difficulty ramp to baseline when rotating the ball offer.
                        if (g_schoolStrikeLaneRestitutionActive && g_schoolStrikeLaneRestitutionBase >= 0.0f)
                            usr->laneRestitution = glm::clamp(g_schoolStrikeLaneRestitutionBase, 0.0f, 1.0f);
                        if (g_schoolStrikeArmImpulseActive && g_schoolStrikeArmImpulseBase > 0.0f)
                            usr->armImpulseAtThrow = glm::clamp(
                                g_schoolStrikeArmImpulseBase,
                                BallPhysicsMapping::PHYSICS_ARM_IMPULSE_MIN,
                                BallPhysicsMapping::PHYSICS_ARM_IMPULSE_MAX
                            );
                        // Give the player a random ball from the catalog, lesson-only.
                        if (g_ballCatalogCount > 0)
                        {
                            const uint32_t r = School_StrikeRngNext();
                            const int idx = (int)(r % (uint32_t)g_ballCatalogCount);
                            BallStats_OnBallChange(&g_ballCatalog[idx], usr);
                        }
                    }
                    else if (storyEvent == EVENT_SCHOOL_STRIKE_HELP_DECLINE)
                    {
                        // Restore difficulty ramp to baseline when declining the offer.
                        if (g_schoolStrikeLaneRestitutionActive && g_schoolStrikeLaneRestitutionBase >= 0.0f)
                            usr->laneRestitution = glm::clamp(g_schoolStrikeLaneRestitutionBase, 0.0f, 1.0f);
                        if (g_schoolStrikeArmImpulseActive && g_schoolStrikeArmImpulseBase > 0.0f)
                            usr->armImpulseAtThrow = glm::clamp(
                                g_schoolStrikeArmImpulseBase,
                                BallPhysicsMapping::PHYSICS_ARM_IMPULSE_MIN,
                                BallPhysicsMapping::PHYSICS_ARM_IMPULSE_MAX
                            );
                    }
	        }
	    }

    // School Mass lesson: apply slider value to physics every frame.
    // (The slider is edited in the Mass Editor window, not inline in the school panel.)
    if (usr->gameMode == UserContext::GameMode::SCHOOL && usr->school.selectedLesson == 2)
    {
        float m = glm::clamp(
            usr->school.massSlider.value, SchoolMassTuning::MASS_MIN_KG, SchoolMassTuning::MASS_MAX_KG
        );
        if (fabsf(m - usr->desiredMass) > 1e-4f)
        {
            usr->desiredMass = m;
            usr->phy.set_ball_mass(usr->desiredMass);
            usr->lightnessBuff = BallStats_LightnessBuff(usr->desiredMass);
            usr->launchBuffEffective = glm::clamp(usr->myBall.launchBuff * usr->lightnessBuff, 0.0f, 1.0f);
            usr->armImpulseAtThrow = remapClamped(
                usr->launchBuffEffective,
                BallPhysicsMapping::CATALOG_BUFF_MIN,
                BallPhysicsMapping::CATALOG_BUFF_MAX,
                BallPhysicsMapping::PHYSICS_ARM_IMPULSE_MIN,
                BallPhysicsMapping::PHYSICS_ARM_IMPULSE_MAX
            );
            usr->ballRestitution = glm::clamp(
                glm::clamp(usr->myBall.restitution, 0.0f, 1.0f) * BallStats_RestitutionMassScale(usr->desiredMass),
                0.0f,
                1.0f
            );
        }
    }

    if (shouldHandleResize)
    {
        // Recalculate perspective
        float fov = glm::radians(60.0f); // Field of view in radians
        float aspectRatio = (float)ctx->screenWidth / (float)ctx->screenHeight;
        float nearPlane = aspectRatio < 0.60f ? 0.35f : 0.50f;
        float farPlane = aspectRatio < 0.60f ? 40.0f : 35.0f;
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
                usr->pivotPoint.x -= (movePivot * safeDeltaTime * pivotMoveSpeed);
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

	                    // Joystick spin input direction swap: invert the perceived CW/CCW.
	                    // (Physics spin/hook directions are correct; this only changes control mapping.)
	                    float angleDelta = -atan2f(cross, dot);

                    float speedScale = 0.05f;
                    float weight = glm::clamp(speed * speedScale, 0.0f, 1.0f);
                    angleDelta *= weight;

                    usr->totalAngle += angleDelta;
	                    usr->angularVelocity = angleDelta / glm::max(aimSwingStepDt, 1e-4f);

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
            usr->enjoy.moveJoystickTo(usr->aimFlatPos, safeDeltaTime);
        }
        if (usr->phase == UserContext::Phase::SWING)
        {
            usr->enjoy.moveJoystickTo(usr->aimFlatPos, safeDeltaTime);
        }
    }

    if (usr->shouldShowShop && usr->windowStack.shopBuyRequested)
    {
        usr->windowStack.shopBuyRequested = false;
        if (usr->selectorFlowStep == SelectorFlowStep::BALL)
        {
            const int idx = usr->carousel.closestBallIdx;
            if (idx >= 0 && idx < usr->carousel.cardCount)
            {
                usr->selectedBallId = usr->carousel.items[idx].id;
                if (usr->playerRoute == PlayerRoute::FREESTYLE)
                    StartFreestyleRun(usr);
                else
                    StartPracticeRun(usr);
            }
        }
        else
        {
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
            Progress_SaveUnlocksAndBank(usr);
            usr->windowStack.shopPointerDown = false;
            usr->sound.playSfxBuy();
            std::cerr << "Item bought" << std::endl;
        }
    }
    if (usr->windowStack.shopCloseRequested)
    {
        usr->windowStack.shopCloseRequested = false;
        if (usr->selectorFlowStep != SelectorFlowStep::NONE)
            SelectorFlow_Cancel(usr);
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
	        PhysicsResetForMode(usr, /*reviveAll=*/true);
            usr->electroBall.resetCharge();
	        std::cerr << textScoreboard(usr->board) << std::endl;
	        resetScoreboard(&usr->board);
            if (usr->enemyBoardInit)
                resetScoreboard(&usr->enemyBoard);
            usr->turnOwner = UserContext::TurnOwner::PLAYER;

            if (usr->pendingModeChange)
            {
                const UserContext::GameMode next = usr->pendingMode;
                usr->pendingModeChange = false;
                usr->pendingMode = usr->gameMode;

                if (next == UserContext::GameMode::SCHOOL)
                {
                    EnterSchool(usr, /*playStory=*/true);
                }
                else
                {
                    usr->gameMode = next;
                    if (usr->gameMode == UserContext::GameMode::BOT)
                    {
                        usr->turnOwner = UserContext::TurnOwner::PLAYER;
                        usr->enemyAutoTimer = 0.0f;
                        usr->enemyLaunched = false;
                        usr->enemyDebugLogged = false;
                        usr->enemyTurnSetup = false;
                        resetScoreboard(&usr->enemyBoard);
                    }
                }
            }
            else
            {
                Campaign_ApplyCurrentLevelSetup(usr, /*resetStoryKick=*/false);
            }
	        // When leaving RESULT, we generally want relative mode restored by phase logic next frame.
	    }

	    if (usr->keypad.newsDetected)
	    {
            if (usr->tracker.pendingPartNameKeypadOpen)
            {
                Tracker_ApplyPartNameKeypadResult(usr);
                usr->keypad.newsDetected = false;
            }
            else if (usr->tracker.pendingSongNameKeypadOpen)
            {
                Tracker_ApplySongNameKeypadResult(usr);
                usr->keypad.newsDetected = false;
            }
            else if (usr->tracker.pendingInstrumentAction != 0)
            {
                Tracker_ApplyInstrumentNameKeypadResult(usr);
                usr->keypad.newsDetected = false;
            }
            else
            {
	        std::cerr << "keypad news detect" << usr->username_len << std::endl;
	        usr->keypad.newsDetected = false;
	        bool isSb1 = (usr->username_len == 3 && memcmp(usr->username, "SB1", 3) == 0);
	        if (isSb1)
	        {
	            setupStubScoreboardFinal(&usr->board);
	            std::cerr << "seted up board stub" << std::endl;
	        }

            // School cheat codes: SC1..SC5 unlock/complete lessons.
            bool isSc =
                (usr->username_len == 3 && memcmp(usr->username, "SC", 2) == 0 &&
                 usr->username[2] >= '1' && usr->username[2] <= '5');
	            if (isSc)
	            {
	                int n = (int)(usr->username[2] - '0'); // 1..5
	                for (int i = 0; i < 5; i++)
	                    usr->school.lessonDone[i] = (i < n);
	                usr->school.unlockedLessons = glm::max(usr->school.unlockedLessons, glm::min(n + 1, 5));
	                std::cerr << "School cheat: SC" << n << " applied" << std::endl;
	            }
            }
	    }

    // Check for some more if any phases need to transition
    if (usr->phase == UserContext::Phase::AIM)
    {
        bool aimingLongEnough = usr->aimingTime > (lowFpsAimSwingFrame ? 0.45f : 0.6f);
        bool wantsPhysics = usr->trans.wantsPhysics(usr->enjoy.ndc, safeDeltaTime);
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
		            Scene_ComputeReleaseOffsetZ(usr->scene, ropeLen, usr->launchBuffEffective);
		        float releasePlaneZ = usr->pivotPoint.z + usr->scene.releaseOffsetZ;
		        bool muchFwd = ballPos.z > releasePlaneZ + 0.9f;
		        bool muchUpFront = muchUp + muchFwd;
	        bool physicsLongEnough = usr->swingingTime > (lowFpsAimSwingFrame ? 0.30f : 0.4f);
	        bool physicsWayTooLong = usr->swingingTime > (lowFpsAimSwingFrame ? 1.8f : 1.4f);
        bool wantsPhysics = usr->trans.wantsPhysics(usr->enjoy.ndc, safeDeltaTime);
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
            float speed = (aimSwingStepDt > 1e-6f) ? (moved / aimSwingStepDt) : 0.0f;
            const float kStallSpeed = lowFpsAimSwingFrame ? 0.02f : 0.03f; // m/s
            if (speed < kStallSpeed)
            {
                usr->swingStallTime += safeDeltaTime;
            }
            else
            {
                usr->swingStallTime = 0.0f;
            }

            // Forgiveness: if swing stalls for long enough, cancel without counting a throw.
            // User request: stall > 1s => return to AIM (not IDLE).
            if (usr->swingingTime > 0.50f && usr->swingStallTime > (lowFpsAimSwingFrame ? 1.35f : 1.0f))
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
                    usr->carriedBall, glm::quat(1.0f, 0, 0, 0), aimSwingStepDt
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
                    userTriesToThrow = false;
                    wantsPhysics = false;
                    muchUpFront = false;
                    muchUp = false;
                    muchFwd = false;
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
			            Scene_ComputeReleaseOffsetZ(usr->scene, ropeLen, usr->launchBuffEffective);
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

                // Reset joystick input so School Lesson 3 pullback meter starts empty.
                usr->aimFlatPos = glm::vec2(0.5f, 0.5f);
                usr->aimDownFlatPos = usr->aimFlatPos;
                usr->enjoy.resetJoystick();
	            }
            if (phaseTrans == UserContext::PhaseTrans::TRANS_AIM_TO_SWING)
            {
                usr->phase = UserContext::Phase::SWING;
                std::cerr << "AIM -> SWING " << std::endl;

                if (usr->gameMode == UserContext::GameMode::SCHOOL && usr->school.selectedLesson == 1)
                {
                    usr->school.aimQualifiedThisThrow = usr->school.aimPullEnough && usr->school.aimCenteredEnough;
                }

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
		                if (usr->gameMode == UserContext::GameMode::SCHOOL && usr->school.selectedLesson == 1)
		                {
		                    usr->school.aimQualifiedThisThrow = usr->school.aimPullEnough && usr->school.aimCenteredEnough;
		                }
		                usr->strikeSpareSfxPlayedKind = 0;
		                usr->negativeBannerSfxPlayedKind = 0;
			                usr->negativeBannerKind = 0;
			                usr->negativeBannerFlashTime = 0.0f;
			                usr->neutralBannerFlashTime = 0.0f;
			                usr->neutralBannerPins = 0;
			                usr->laneImpactHitCount = 0;
			                usr->laneImpactBounceIndex = 0;
			                usr->laneImpactHadAirtime = true;
			                usr->laneImpactCooldownT = 0.0f;
			                usr->laneImpactPrevValid = false;
			                usr->laneImpactShakeTime = 0.0f;
			                usr->laneImpactShakeAmp = 0.0f;
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

                        // Base assist speed in m/s; keep it modest since we also add
                        // a mass-dependent impulse at release on the first THROW frame.
                        float speedBoostScale = remapClamped(
                            usr->armImpulseAtThrow,
                            BallPhysicsMapping::PHYSICS_ARM_IMPULSE_MIN,
                            BallPhysicsMapping::PHYSICS_ARM_IMPULSE_MAX,
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

	                // Lane impact SFX/shake is driven in game.cpp for hot reload.
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
		                usr->strikeSpareSfxPlayedKind = 0;
		                usr->negativeBannerSfxPlayedKind = 0;
			                usr->negativeBannerKind = 0;
			                usr->negativeBannerFlashTime = 0.0f;
			                usr->neutralBannerFlashTime = 0.0f;
			                usr->neutralBannerPins = 0;
			                usr->laneImpactHitCount = 0;
			                usr->laneImpactBounceIndex = 0;
			                usr->laneImpactHadAirtime = true;
			                usr->laneImpactCooldownT = 0.0f;
			                usr->laneImpactPrevValid = false;
			                usr->laneImpactShakeTime = 0.0f;
			                usr->laneImpactShakeAmp = 0.0f;
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
                // Initialize oil-wear integration baseline so the first THROW frame doesn't see a huge jump.
                usr->lastBallPosition = usr->carriedBall;
                // events
                // Lane impact SFX is now driven by actual ball<->lane contacts in physics.
            }
        }
    }

    glm::vec3 IDLE_BALL_POS = Scene_IdleBallPos(usr->scene);

    float yFactor = 0.0f;
    glm::mat4 ballModel;
    int decalIndex = 0;
    if (!trackerOnlyMode)
    {
	    /* Put ballmodel */ {
	        if (usr->phase == UserContext::Phase::IDLE)
	        {
	            usr->numberOfBallsHit = 0;

                // School lesson 3 (Spin): smooth return-to-start between levels (no idle jiggle/rotation).
                if (usr->gameMode == UserContext::GameMode::SCHOOL &&
                    usr->school.selectedLesson == 3 &&
                    usr->school.returnToStartActive)
                {
                    float dt = (float)deltaTime;
                    if (!std::isfinite(dt) || dt <= 0.0f)
                        dt = 0.0f;
                    usr->school.returnToStartT += (usr->school.returnToStartDuration > 1e-6f)
                                                     ? (dt / usr->school.returnToStartDuration)
                                                     : 1.0f;
                    float t01 = glm::clamp(usr->school.returnToStartT, 0.0f, 1.0f);
                    float ease = t01 * t01 * (3.0f - 2.0f * t01);
                    glm::vec3 pos = glm::mix(usr->school.returnFromBallPos, IDLE_BALL_POS, ease);

                    ballModel = glm::translate(glm::mat4(1.0f), pos);
                    usr->carriedBall = ballModel[3];
                    usr->phy.set_manual_ball_position(pos, glm::quat(1.0f, 0, 0, 0), dt);

                    if (t01 >= 1.0f - 1e-4f)
                    {
                        // Finalize the level completion now that we're back at start.
                        usr->school.returnToStartActive = false;
                        usr->school.returnToStartT = 0.0f;
                        usr->school.celebrateKind = 0;
                        usr->school.spinLevelJustCompleted = false;

                        const int per = SchoolSpinTuning::COINS_PER_LEVEL;
                        const int totalNeeded = SchoolSpinTuning::TOTAL_REQUIRED;
                        usr->school.spinSafeCoins = glm::min(usr->school.spinSafeCoins + per, totalNeeded);
                        usr->school.spinCollectedInLevel = 0;
                        usr->school.spinLevel = glm::min(usr->school.spinLevel + 1, SchoolSpinTuning::LEVELS);

                        if (usr->school.spinSafeCoins >= totalNeeded)
                        {
                            usr->school.spinTestCompleted = true;
                            usr->school.lessonDone[2] = true;
                            usr->school.unlockedLessons = glm::max(usr->school.unlockedLessons, 4);
                            if (usr->windowStack.count == 0 && !usr->dialog.active)
                                usr->dialog.open(1020);
                        }
                        else
                        {
                            SchoolServices svc = {};
                            svc.coinLane = &usr->coinLane;
                            SchoolSpin_InitCoinsForLevel(&usr->school, svc, usr->school.spinLevel);
                        }

                        // Ensure pins stay off-lane in lesson 3 and ball is reset cleanly.
                        PhysicsResetForMode(usr, /*reviveAll=*/true);
                    }

                    // Skip normal IDLE behavior.
                }
                else
                {
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

                if (usr->gameMode != UserContext::GameMode::SCHOOL)
                {
	                if (usr->coinLane.autoRespawnIfNeeded(getNextCoinPattern(), 7, deltaTime))
	                {
	                    usr->clearedCoins = 0; // Reset counter for new set of coins
	                }
                }
                else if (usr->gameMode == UserContext::GameMode::SCHOOL &&
                         usr->school.selectedLesson == 3)
                {
                    // Lesson 3 manages its own coin pattern (no auto-respawn).
                    if (usr->coinLane.getActiveCount() == 0)
                    {
                        SchoolServices svc = {};
                        svc.coinLane = &usr->coinLane;
                        SchoolSpin_InitCoinsForLevel(&usr->school, svc, usr->school.spinLevel);
                    }
                }
            usr->catchupSpeed = glm::vec3(0.0f);
            usr->catchupDirection = glm::vec3(0.0f);
	            usr->carriedVel = glm::vec3(0.0f);
                }
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
	                usr->releaseSpinFromRot = angularVelocityFromDelta(deltaRot, aimSwingStepDt);
	            }
	            usr->prevBallRotForRelease = ballRot;
	            usr->hasPrevBallRotForRelease = true;

	            usr->aimingTime += safeDeltaTime;

	            float pullX = usr->enjoy.ndc.x;
	            float pullZ = usr->enjoy.ndc.y;
	            // Adds hung
	            // Keep inside rope sphere to avoid NaNs.
                // Also: School Lesson 3 (Aim/pullback) uses normalized "how far back" the joystick is.
                // In this coordinate system, negative Z means "pulled back behind pivot".
	            {
	                float r2 = 1.01f * ropeLength * ropeLength;
	                float maxZ2 = r2 - pullX * pullX;
	                maxZ2 = glm::max(0.0f, maxZ2);
	                float maxAbsZ = sqrtf(maxZ2);
	                pullZ = glm::clamp(pullZ, -maxAbsZ, maxAbsZ);

                    if (usr->gameMode == UserContext::GameMode::SCHOOL && usr->school.selectedLesson == 1)
                    {
                        // Lesson 3: use the *joystick visual displacement* (distance between big and
                        // small joystick centers) as input. This guarantees:
                        // - centered knob -> 0.0
                        // - knob at rim   -> 1.0
                        //
                        // `renderNdc` is the visual knob position clamped to the unit circle.
                        // On screen, pulling down produces renderNdc.y < 0.
                        float pullBack01 = glm::clamp(-usr->enjoy.renderNdc.y, 0.0f, 1.0f);
                        usr->school.aimPull01 = pullBack01;
                        usr->school.aimPullEnough = (pullBack01 >= SchoolAimTuning::PULL_ENOUGH_THRESHOLD);
                        usr->school.aimCenteredEnough =
                            (fabsf(usr->enjoy.ndc.x) <= SchoolAimTuning::CENTER_X_MAX_ABS);
                    }
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
                usr->carriedVel += acceleration * aimSwingStepDt;
                usr->carriedBall += usr->carriedVel * aimSwingStepDt;
                // }
            } /* hand moving carried ball end */

            stabilizeAimSwingBall(usr->carriedBall, usr->carriedVel, usr->desiredBall);

            {
                // Pullback depth relative to pivot (positive when ball is behind pivot).
                float pullbackMeters = usr->pivotPoint.z - usr->carriedBall.z;
                usr->aimMaxPullbackMeters = glm::max(usr->aimMaxPullbackMeters, pullbackMeters);
            }

            // Track whether the user ever actually pulled back on the joystick (ndc.y < 0).
            usr->aimMinNdcY = glm::min(usr->aimMinNdcY, usr->enjoy.ndc.y);

	            ballModel = glm::translate(glm::mat4(1.0f), usr->carriedBall) * glm::mat4_cast(ballRot);
	
	            usr->phy.set_manual_ball_position(usr->carriedBall, ballRot, aimSwingStepDt);
	        }
        // usr->phy.enable_physics_on_ball();

	        if (usr->phase == UserContext::Phase::SWING)
	        {
                bool swingSafetyResetToAim = false;
	            usr->swingingTime += safeDeltaTime;

            //  std::cerr << "SPIN2 " << spin << std::endl
            // usr->phy.apply_angular_velocity_on_ball(spin);

	            // Physics controls position; we control rotation so it stays aligned with the rope.
	            ballModel = usr->phy.physics_get_ball_matrix();
	            glm::vec3 before = usr->carriedBall;
	            usr->carriedBall = ballModel[3]; //
	            glm::vec3 after = usr->carriedBall;

                const float maxSwingRopeLen = ropeLength * 1.35f;
	                if (!vec3Finite(usr->carriedBall) ||
	                    glm::length(usr->carriedBall - usr->pivotPoint) > maxSwingRopeLen ||
	                    usr->carriedBall.y < -0.20f)
	                {
                    usr->bufferedRequestThrow = false;
                    usr->carriedVel = glm::vec3(0.0f);
                    stabilizeAimSwingBall(usr->carriedBall, usr->carriedVel, usr->desiredBall);
                    usr->phy.set_ball_free();
                    usr->phy.set_manual_ball_position(
                        usr->carriedBall, glm::quat(1.0f, 0, 0, 0), aimSwingStepDt
                    );
                    usr->phase = UserContext::Phase::AIM;
                    usr->aimingTime = 0.0f;
                    usr->swingingTime = 0.0f;
	                    usr->highestPoint = -10.0f;
	                    usr->swingStallTime = 0.0f;
	                    usr->aimDownFlatPos = usr->aimFlatPos;
	                    SDL_SetRelativeMouseMode(SDL_TRUE);
	                    ballModel = glm::translate(glm::mat4(1.0f), usr->carriedBall);
                        swingSafetyResetToAim = true;
	                }

                    if (!swingSafetyResetToAim)
                    {
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
		                    usr->releaseSpinFromRot = angularVelocityFromDelta(deltaRot, aimSwingStepDt);
		                }
		                usr->prevBallRotForRelease = ballRot;
		                usr->hasPrevBallRotForRelease = true;
                    }
		        }

	        if (usr->phase == UserContext::Phase::THROW)
	        {
                // Enemy turn: before auto-launch, keep the ball static and prevent "throw complete" logic.
                if (usr->gameMode == UserContext::GameMode::BOT && IsEnemyTurn(usr) && !usr->enemyLaunched)
                {
                    const bool launchedNow = Enemy_TickAutoThrow(usr, (float)deltaTime);
                    if (launchedNow)
                    {
                        // New roll just started (enemy launched). Reset pin-hit impact counter so SFX
                        // can trigger on the first impacts of this roll.
                        usr->numberOfBallsHit = 0;
                    }
                    if (!launchedNow && !usr->enemyLaunched)
                    {
                        glm::vec3 pos = Enemy_IdleBallPos(usr);
                        usr->phy.set_manual_ball_position(pos, glm::quat(1.0f, 0, 0, 0), (float)deltaTime);
                        ballModel = glm::translate(glm::mat4(1.0f), pos);
                        usr->throwingTime = 0.0f;
                        usr->settlingTime = 0.0f;
                        usr->lastBallPosition = pos;
                    }
                }
                else
                {
	                if (usr->throwingTime == 0.0f)
	                {
                        // New roll just started (player or already-launched enemy). Reset pin-hit impact
                        // counter so SFX triggers correctly for each roll (including 2nd/3rd roll in 10th).
                        usr->numberOfBallsHit = 0;
                if (usr->auroraVibe.value >= 4.0f)
                {
                    usr->auroraVibe.value += 4.0f;
                }
                float start = usr->auroraVibe.value;
                usr->auroraVibe.start(start, start + 1.0f, 1.5f);
                glm::vec3 movement = usr->phy.get_ball_swing_movement();

                // Arm assist is an impulse at the moment of release (mass-dependent).
                // This makes heavier balls slower for the same launch buff.
                const float m = glm::max(0.10f, usr->desiredMass);
                float baseSpeed = glm::length(movement);
                glm::vec3 dir = (baseSpeed > 1e-6f) ? (movement / baseSpeed) : glm::vec3(0.0f, 0.0f, 1.0f);
                float dv = usr->armImpulseAtThrow / m;

	                // Keep it sane: don't let impulse dominate if base speed is already high.
	                dv = glm::min(dv, 12.0f);

	                movement = movement + dir * dv;
                // Lesson 3: cap launch speed.
                if (usr->gameMode == UserContext::GameMode::SCHOOL &&
                    usr->school.selectedLesson == 3)
                {
                    float sp = glm::length(movement);
                    float cap = SchoolSpinTuning::LAUNCH_SPEED_CAP;
                    if (sp > cap && sp > 1e-6f)
                        movement *= (cap / sp);
                }
	                usr->phy.set_ball_swing_movement(movement);

                // Capture player release params so enemy can mirror the shot on its turn.
                if (!IsEnemyTurn(usr))
                {
                    usr->lastPlayerReleaseMovement = movement;
                    // Store current smoothed spin (around Y) as "release spin speed".
                    usr->lastPlayerReleaseSpinSpeed = usr->smoothedAngularVelocity;
                    usr->haveLastPlayerRelease = true;
                }
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
                    // NOTE: In vs mode, enemy throws travel toward -Z, which would look like a
                    // "backwards throw" to this logic. Skip this forgiveness during enemy turns.
		            if (!forgivenThrow && !(usr->gameMode == UserContext::GameMode::BOT && IsEnemyTurn(usr)))
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
	                            // School Lesson 3: if the attempt ended via forgiveness (fell off / glitch),
	                            // annul this round and respawn the 3 coins for the current level.
	                            if (usr->gameMode == UserContext::GameMode::SCHOOL &&
	                                usr->school.selectedLesson == 3)
	                            {
	                                usr->school.spinCollectedInLevel = 0;
                                    SchoolServices svc = {};
                                    svc.coinLane = &usr->coinLane;
	                                SchoolSpin_InitCoinsForLevel(&usr->school, svc, usr->school.spinLevel);
	                            }
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

				                float throwTimeoutS = 10.0f;
				                if (usr->gameMode == UserContext::GameMode::SCHOOL &&
				                    usr->school.selectedLesson == 3)
				                {
				                    throwTimeoutS = SchoolSpinTuning::THROW_TIMEOUT_S;
				                }
			                bool waitToSettle = usr->settlingTime < 3.0f && usr->throwingTime < throwTimeoutS;
			                bool timedOutThrow = !waitToSettle;
		                int state = usr->phy.checkThrowComplete(
		                    waitToSettle ? 0.1f : 100.0f, // Technically it will still wait to
		                                                  // settle if speed is very high
		                    -0.1f                         // floorLevel
		                );

		                int actualNumberOfBallsHit = usr->phy.get_number_of_impacts();
		                if (actualNumberOfBallsHit > usr->numberOfBallsHit)
		                {
                            BallRollingSfx_Stop(usr);
		                    usr->sound.playSfxBallHitPins();
                            // Pin-hit screenshake: accumulate for clusters of impacts, ease out.
                            {
                                const float add = 0.0012f;
                                usr->pinHitShakeAmp = glm::clamp(usr->pinHitShakeAmp + add, 0.0f, 0.012f);
                                usr->pinHitShakeTime = usr->pinHitShakeDuration;
                            }
                            usr->electroBall.triggerPinFlash();
		                    usr->numberOfBallsHit += 1;
		                }
				                    if (state != -1) // if got actuall score
				                    {
				                        // If we timed out but the ball is still on the lane, show STALLED.
				                        // This can happen because timeout uses a very large stillThreshold to force completion.
				                        if (timedOutThrow &&
			                        usr->negativeBannerFlashTime <= 0.0f &&
			                        std::isfinite(ballModel[3].y) && ballModel[3].y > -0.05f)
		                    {
		                        usr->negativeBannerKind = 2;
		                        usr->negativeBannerFlashTime = 1.25f;
		                        if (usr->negativeBannerSfxPlayedKind != 2)
		                        {
                                    BallRollingSfx_Stop(usr);
		                            usr->sound.playSfxBallTimeout();
		                            usr->negativeBannerSfxPlayedKind = 2;
		                        }
		                    }

		                    // If the roll hit no pins, treat as a "GUTTER BALL" (user-facing wording).
		                    // This uses the same pin-delta logic as scoring.
		                    int knockedThisRoll = Bowling_ComputeKnockedThisRoll(state, usr->wereDead);
		                    if (!timedOutThrow && knockedThisRoll <= 0 && usr->negativeBannerFlashTime <= 0.0f)
		                    {
		                        usr->negativeBannerKind = 1;
		                        usr->negativeBannerFlashTime = 1.25f;
		                        if (usr->negativeBannerSfxPlayedKind != 1)
		                        {
                                    BallRollingSfx_Stop(usr);
		                            usr->sound.playSfxBallInGutter();
		                            usr->negativeBannerSfxPlayedKind = 1;
		                        }
		                    }

		                    // Apply per-throw oil wear once per completed roll.
		                    // - Carrydown extends oil fade start/end forward
		                    // - Thickness decays based on total travel
		                    {
                                const bool isSchoolOilLesson =
                                    (usr->gameMode == UserContext::GameMode::SCHOOL && usr->school.selectedLesson == 4);
                                const float wearMul = isSchoolOilLesson ? School_OilDefaults().wearMultiplier : 1.0f;

		                        auto ApplyCarrydownSide = [&](float &fadeStartM, float &fadeEndM, float wearM)
		                        {
		                            float s = fadeStartM;
		                            float e = fadeEndM;
		                            if (s > e)
		                                std::swap(s, e);
		                            float carryStart = usr->oilCarrydownPerBallTravelM * wearM * wearMul;
		                            float ratio = (s > 1e-3f) ? (e / s) : 1.0f;
		                            float carryEnd = carryStart * ratio;
		                            fadeStartM = glm::clamp(s + carryStart, 0.0f, 18.3f);
		                            fadeEndM = glm::clamp(e + carryEnd, 0.0f, 18.3f);
		                            if (fadeStartM > fadeEndM)
		                                std::swap(fadeStartM, fadeEndM);
		                        };

		                        ApplyCarrydownSide(usr->leftOilFadeStartM, usr->leftOilFadeEndM, usr->oilWearLeftM);
		                        ApplyCarrydownSide(usr->rightOilFadeStartM, usr->rightOilFadeEndM, usr->oilWearRightM);

		                        float thicknessDrop = usr->oilThicknessDecayPerBallTravel * usr->oilWearTotalM * wearMul;
		                        if (std::isfinite(thicknessDrop))
                                {
                                    if (isSchoolOilLesson)
                                        thicknessDrop = glm::min(thicknessDrop, School_OilDefaults().maxThicknessDropPerRoll);
		                            usr->laneOilThickness = glm::clamp(usr->laneOilThickness - thicknessDrop, 0.0f, 1.0f);
                                }

		                        usr->oilWearLeftM = 0.0f;
		                        usr->oilWearRightM = 0.0f;
		                        usr->oilWearTotalM = 0.0f;
		                    }

		                    // Count rolls inside school lessons.
		                    if (usr->gameMode == UserContext::GameMode::SCHOOL)
		                        usr->school.lessonRolls += 1;

		                    // Capture strike/spare flags pre-roll so we can trigger celebration once.
                            BowlingScoreboard *activeSb =
                                (usr->gameMode == UserContext::GameMode::BOT && IsEnemyTurn(usr))
                                    ? &usr->enemyBoard
                                    : &usr->board;
		                    int preStrike[10];
		                    int preSpare[10];
		                    for (int i = 0; i < 10; i++)
		                    {
		                        preStrike[i] = activeSb->frames[i].isStrike;
		                        preSpare[i] = activeSb->frames[i].isSpare;
		                    }
		                    bool frameCompleted = false;
		                    if (usr->gameMode == UserContext::GameMode::BOT ||
                                usr->gameMode == UserContext::GameMode::SOLO)
		                        frameCompleted = addRoll(activeSb, knockedThisRoll);
		                    else
		                    {
		                        // School: practice resets the rack every throw, and lessons unlock by doing.
		                        frameCompleted = true;
		                        if (usr->school.selectedLesson == 2)
		                        {
                                    // Lesson 2 "Mass test": hit pins with a LIGHT ball and with a HEAVY ball.
                                    // Harass the player into using the ends:
                                    // - If mid mass: warn every throw (doesn't count).
                                    // - If one side is passed: remind every throw to switch to the other side.
                                    {
	                                        const int need = SchoolMassTuning::REQUIRED_HITS_EACH;
	                                        const bool lightPassed = usr->school.massLightHits >= need;
	                                        const bool heavyPassed = usr->school.massHeavyHits >= need;
	                                        const float m = usr->desiredMass;
	                                        const bool isLight = (m <= SchoolMassTuning::LIGHT_TEST_MAX_KG);
	                                        const bool isHeavy = (m >= SchoolMassTuning::HEAVY_TEST_MIN_KG);

                                        if (usr->windowStack.count == 0 && !usr->dialog.active)
                                        {
                                            if ((lightPassed && !heavyPassed) && !isHeavy)
                                            {
                                                usr->dialog.open(1013); // go heavy
                                            }
                                            else if ((heavyPassed && !lightPassed) && !isLight)
                                            {
                                                usr->dialog.open(1014); // go light
                                            }
                                            else if (!lightPassed && !heavyPassed && !isLight && !isHeavy)
                                            {
                                                usr->dialog.open(1012); // use ends
                                            }
                                        }
                                    }

	                                    // We only count throws that actually knock down at least one pin.
	                                    if (knockedThisRoll > 0)
	                                    {
	                                        const int need = SchoolMassTuning::REQUIRED_HITS_EACH;
	                                        const bool lightWasPassed = usr->school.massLightHits >= need;
	                                        const bool heavyWasPassed = usr->school.massHeavyHits >= need;

	                                        const float m = usr->desiredMass;
	                                        const bool isLight = (m <= SchoolMassTuning::LIGHT_TEST_MAX_KG);
	                                        const bool isHeavy = (m >= SchoolMassTuning::HEAVY_TEST_MIN_KG);

                                        if (!isLight && !isHeavy)
                                        {
                                            // Middle mass: does not count. Show a hint once.
                                            // (handled above: harass every throw)
                                        }
	                                        else
	                                        {
	                                            if (isLight)
	                                                usr->school.massLightHits++;
	                                            if (isHeavy)
	                                                usr->school.massHeavyHits++;

		                                            usr->school.massLightHits = glm::clamp(usr->school.massLightHits, 0, need);
		                                            usr->school.massHeavyHits = glm::clamp(usr->school.massHeavyHits, 0, need);

		                                            const bool lightNowPassed = usr->school.massLightHits >= need;
		                                            const bool heavyNowPassed = usr->school.massHeavyHits >= need;

                                                    // Lesson 1: celebrate when the player completes either side for the first time.
                                                    if (usr->school.celebratePauseT <= 0.0f)
                                                    {
                                                        if (!lightWasPassed && lightNowPassed)
                                                        {
                                                            usr->school.celebrateKind = 1;
                                                            usr->school.celebratePauseT = 0.5f;
                                                            usr->sound.playSfxWin();
                                                            glm::vec3 p = usr->initialPins[0];
                                                            p.y += 0.35f;
                                                            usr->particles.burstConfetti(p);
                                                        }
                                                        else if (!heavyWasPassed && heavyNowPassed)
                                                        {
                                                            usr->school.celebrateKind = 2;
                                                            usr->school.celebratePauseT = 0.5f;
                                                            usr->sound.playSfxWin();
                                                            glm::vec3 p = usr->initialPins[0];
                                                            p.y += 0.35f;
                                                            usr->particles.burstConfetti(p);
                                                        }
                                                    }

		                                            // If the player finished one side first, prompt them to switch.
		                                            if (!usr->school.massSwapHintShown &&
		                                                usr->windowStack.count == 0 && !usr->dialog.active)
		                                            {
	                                                if (!lightWasPassed && lightNowPassed && !heavyNowPassed)
	                                                {
	                                                    usr->dialog.open(1013); // go heavy
	                                                    usr->school.massSwapHintShown = true;
	                                                }
	                                                else if (!heavyWasPassed && heavyNowPassed && !lightNowPassed)
	                                                {
	                                                    usr->dialog.open(1014); // go light
	                                                    usr->school.massSwapHintShown = true;
	                                                }
	                                            }
	                                        }
	                                    }

	                                    const int need = SchoolMassTuning::REQUIRED_HITS_EACH;
	                                    const bool passed = (usr->school.massLightHits >= need) && (usr->school.massHeavyHits >= need);
	                                    if (passed && !usr->school.massTestCompleted)
	                                    {
	                                        usr->school.massTestCompleted = true;
	                                        usr->school.lessonDone[1] = true;
	                                        usr->school.unlockedLessons = glm::max(usr->school.unlockedLessons, 3);
	                                        // Show a short story immediately (modal, no windows).
			                                if (usr->windowStack.count == 0 && !usr->dialog.active)
			                                    usr->dialog.open(1010);
			                            }
			                        }
	                                else if (usr->school.selectedLesson == 3)
	                                {
	                                    // Lesson 3 ends immediately when all coins in the level are collected.
	                                    // Failure (didn't collect all coins) will be handled by the end-of-run timeout
	                                    // or by re-entering the lesson; we don't do per-throw settle logic here anymore.
	                                }
	                                else if (usr->school.selectedLesson == 4)
	                                {
	                                    // Lesson 4 (Oil): completion is driven by successful re-oils (3x),
	                                    // not by throws directly. Still track rolls for UX, but no auto-pass here.
	                                }
                                    else if (usr->school.selectedLesson == 5)
                                    {
                                        // Lesson 5 (Strike): pass when a strike is scored (all 10 pins down in one roll).
                                        if (!usr->school.lessonDone[4] && knockedThisRoll >= 10)
                                        {
                                            usr->school.lessonDone[4] = true;
                                            usr->school.unlockedLessons = 5;
                                            glm::vec3 p = usr->initialPins[0];
                                            p.y += 0.35f;
                                            usr->particles.burstConfetti(p);
                                            usr->sound.playSfxWin();
                                            // Always show graduation message (even if it was already passed before);
                                            // keep it modal to offer "Practice more" vs "Back to game".
                                            if (usr->windowStack.count == 0 && !usr->dialog.active)
                                                usr->dialog.open(1072);
                                        }
                                        else
                                        {
                                            // Failed attempt; every 5 failed attempts offer a helper ball.
                                            g_schoolStrikeFailedAttempts++;
                                            // Make the lane a bit bouncier to help the player (lesson-only).
                                            if (g_schoolStrikeLaneRestitutionActive)
                                            {
                                                usr->laneRestitution = glm::clamp(
                                                    usr->laneRestitution * SchoolStrikeDifficultyTuning::FAIL_RESTITUTION_MUL,
                                                    0.0f,
                                                    1.0f
                                                );
                                            }
                                            // Make the throw a bit stronger too (lesson-only).
                                            if (g_schoolStrikeArmImpulseActive && g_schoolStrikeArmImpulseBase > 0.0f)
                                            {
                                                usr->armImpulseAtThrow = glm::clamp(
                                                    usr->armImpulseAtThrow * SchoolStrikeDifficultyTuning::FAIL_ARM_IMPULSE_MUL,
                                                    BallPhysicsMapping::PHYSICS_ARM_IMPULSE_MIN,
                                                    BallPhysicsMapping::PHYSICS_ARM_IMPULSE_MAX
                                                );
                                            }
                                            if ((g_schoolStrikeFailedAttempts % 5) == 0)
                                            {
                                                // Before offering a new ball, restore the lane to original restitution.
                                                if (g_schoolStrikeLaneRestitutionActive && g_schoolStrikeLaneRestitutionBase >= 0.0f)
                                                {
                                                    usr->laneRestitution = glm::clamp(g_schoolStrikeLaneRestitutionBase, 0.0f, 1.0f);
                                                }
                                                // Also restore hand strength to baseline.
                                                if (g_schoolStrikeArmImpulseActive && g_schoolStrikeArmImpulseBase > 0.0f)
                                                {
                                                    usr->armImpulseAtThrow = glm::clamp(
                                                        g_schoolStrikeArmImpulseBase,
                                                        BallPhysicsMapping::PHYSICS_ARM_IMPULSE_MIN,
                                                        BallPhysicsMapping::PHYSICS_ARM_IMPULSE_MAX
                                                    );
                                                }
                                                g_schoolStrikeHelpPending = true;
                                            }
                                        }
                                    }
                                    else if (usr->school.selectedLesson == 1)
                                    {
                                        // Lesson 1 (Aim lesson): qualify in AIM (pull back enough + centered),
                                        // then score a point if you hit any pins this throw.
                                        int prevPts = usr->school.aimLessonPoints;
                                        if (usr->school.aimQualifiedThisThrow && knockedThisRoll > 0)
                                        {
                                            const int need = SchoolAimTuning::REQUIRED_POINTS;
                                            usr->school.aimLessonPoints =
                                                glm::clamp(usr->school.aimLessonPoints + 1, 0, need);
                                        }
                                        if (usr->school.aimLessonPoints > prevPts)
                                        {
                                            glm::vec3 p = usr->initialPins[0];
                                            p.y += 0.35f;
                                            usr->particles.burstConfetti(p);
                                        }

                                        const int need = SchoolAimTuning::REQUIRED_POINTS;
                                        if (!usr->school.aimLessonCompleted &&
                                            usr->school.aimLessonPoints >= need)
                                        {
                                            usr->school.aimLessonCompleted = true;
                                            usr->school.lessonDone[0] = true;
                                            usr->school.unlockedLessons = glm::max(usr->school.unlockedLessons, 2);
                                            if (usr->windowStack.count == 0 && !usr->dialog.active)
                                                usr->dialog.open(1040);
                                        }

                                        // Always clear the qualification flag after the throw resolves so it can't
                                        // double-score across resets/phase transitions.
                                        usr->school.aimQualifiedThisThrow = false;
                                    }
		                    }

		                    // Neutral banner: normal roll scored some pins (not strike/spare, not negative).
		                    // Show it during the camera return to IDLE.
		                    if (!timedOutThrow &&
		                        usr->negativeBannerFlashTime <= 0.0f &&
		                        usr->strikeSpareFlashTime <= 0.0f &&
		                        knockedThisRoll > 0 &&
		                        knockedThisRoll < 10)
		                    {
		                        usr->neutralBannerPins = knockedThisRoll;
		                        usr->neutralBannerFlashTime = 0.85f;
		                        usr->sound.playSfxNeutralRoll();
		                    }

			                    // Trigger/refresh strike/spare overlay if a flag flipped 0 -> 1.
			                    // (If we showed an early STRIKE/SPARE during THROW, this will correct it
			                    // to the final settled result and log how much earlier the message started.)
			                    if (!(usr->gameMode == UserContext::GameMode::SCHOOL && usr->school.selectedLesson == 3))
			                    {
			                        bool newStrike = false;
			                        bool newSpare = false;
		                        for (int i = 0; i < 10; i++)
		                        {
		                            // IMPORTANT: compare against the same scoreboard we updated this roll
		                            // (player vs Angel). Using usr->board here causes false positives
		                            // during Angel turns (e.g. player's earlier strikes appear "new").
		                            newStrike |= (activeSb->frames[i].isStrike && !preStrike[i]);
		                            newSpare |= (activeSb->frames[i].isSpare && !preSpare[i]);
		                        }
		                        if (newStrike)
		                        {
		                            // Positive result overrides neutral.
		                            usr->neutralBannerFlashTime = 0.0f;
		                            usr->strikeSpareKind = 1;
		                            usr->strikeSpareFlashTime = glm::max(usr->strikeSpareFlashTime, 1.25f);
		                            if (usr->strikeSpareSfxPlayedKind != 1)
		                            {
		                                usr->sound.playSfxStrike();
		                                usr->strikeSpareSfxPlayedKind = 1;
		                            }
		                            if (usr->strikeSpareEarlyDeclared)
		                            {
		                                float earlierBy = usr->globalTime - usr->strikeSpareEarlyDeclaredAt;
		                                std::cerr << "[celebrate] FINAL=STRIKE earlyKind=" << usr->strikeSpareEarlyKind
		                                          << " earlierBy=" << earlierBy << "s\n";
		                            }
		                        }
		                        else if (newSpare)
		                        {
		                            // Positive result overrides neutral.
		                            usr->neutralBannerFlashTime = 0.0f;
		                            usr->strikeSpareKind = 2;
		                            usr->strikeSpareFlashTime = glm::max(usr->strikeSpareFlashTime, 1.25f);
		                            if (usr->strikeSpareSfxPlayedKind != 2)
		                            {
		                                usr->sound.playSfxSpare();
		                                usr->strikeSpareSfxPlayedKind = 2;
		                            }
		                            if (usr->strikeSpareEarlyDeclared)
		                            {
		                                float earlierBy = usr->globalTime - usr->strikeSpareEarlyDeclaredAt;
		                                std::cerr << "[celebrate] FINAL=SPARE earlyKind=" << usr->strikeSpareEarlyKind
		                                          << " earlierBy=" << earlierBy << "s\n";
		                            }
		                        }
		                    }

		                    Bowling_AdvanceWereDead(state, &usr->wereDead);

		                    bool shouldResetAllPins = false;
		                    if (frameCompleted)
		                    {
		                        shouldResetAllPins = true;
		                    }
                            // 10th-frame bonus: even if the frame isn't completed yet, we may need to
                            // put up a fresh rack for the next roll (strike/spare cases).
                            if (!shouldResetAllPins &&
                                (usr->gameMode == UserContext::GameMode::BOT || usr->gameMode == UserContext::GameMode::SOLO))
                            {
                                if (Bowling_NeedsFreshRackForNextRoll(activeSb))
                                    shouldResetAllPins = true;
                            }
                            // If we reset the rack (new frame OR 10th-frame bonus), the next roll's
                            // pin-delta should be computed from a fresh rack.
                            if (shouldResetAllPins)
                                Bowling_OnRackReset(&usr->wereDead);

		                    // BOT edge case: if we are about to yield the turn to Angel, don't
		                    // "snap back" camera/ball to the player's idle position even for a frame.
                            const bool willSwitchToAngel =
                                (usr->gameMode == UserContext::GameMode::BOT &&
                                 frameCompleted &&
                                 !IsEnemyTurn(usr) &&
                                 !isGameFinished(&usr->enemyBoard));

                            // BOT edge case: if the Angel is continuing within the same frame (2nd/3rd roll),
                            // we must return camera/ball to the Angel side (not the player's idle view).
                            const bool willContinueEnemyRoll =
                                (usr->gameMode == UserContext::GameMode::BOT &&
                                 !frameCompleted &&
                                 IsEnemyTurn(usr) &&
                                 !isGameFinished(&usr->enemyBoard));

		                    // Start a fast camera return to IDLE instead of instantly jumping.
		                    // (We still reset the ball/pins immediately; only camera is smoothed.)
		                    if (willSwitchToAngel)
                            {
                                // Yielding to Angel: Enemy_EnterTurn will seed the render-ball position
                                // (hand-attached) and start the eased camera transition from there.
                                usr->cameraReturnActive = false;
                                usr->cameraReturnT = 0.0f;
                            }
                            else if (willContinueEnemyRoll)
                            {
                                // Angel stays the turn owner: re-arm the pre-shot state and smooth camera
                                // back to the hand-attached render ball position.
                                usr->enemyAutoTimer = 0.0f;
                                usr->enemyLaunched = false;
                                usr->enemyDebugLogged = false;
                                usr->enemyBallRenderSecondsSinceLaunch = 0.0f;
                                usr->enemyBallRenderPosValid = false;
                                Bot_InitIfNeeded(usr);
                                if (Bot_AnimReady(usr))
                                {
                                    Bot_PlayThrowIfPossible(usr, /*resetTime=*/true);
                                    (void)Bot_Anim(usr)->evaluate();
                                }
                                Enemy_SeedRenderedBallPosFromHand(usr);

                                usr->cameraReturnActive = true;
                                usr->cameraReturnT = 0.0f;
                                usr->cameraReturnStartEye = usr->cameraEye;
                                usr->cameraReturnStartTarget = usr->cameraTarget;
                                glm::vec3 p = usr->enemyBallRenderPosValid ? usr->enemyBallRenderPos : Enemy_IdleBallPos(usr);
                                Enemy_ComputeCameraEyeTargetAtBall(p, usr->cameraReturnEndEye, usr->cameraReturnEndTarget);
                                usr->cameraReturnDuration = 0.5f;
                            }
                            else
                            {
                                usr->cameraReturnActive = true;
                                usr->cameraReturnT = 0.0f;
                                usr->cameraReturnStartEye = usr->cameraEye;
                                usr->cameraReturnStartTarget = usr->cameraTarget;
                                glm::vec3 idleBallPos = Scene_IdleBallPos(usr->scene);
                                Scene_ComputeCameraEyeTarget(
                                    usr->scene, idleBallPos, usr->cameraReturnEndEye, usr->cameraReturnEndTarget
                                );
                                usr->cameraReturnDuration = 1.0f;
                            }

		                    // camera must be moved when physics reset, to avoid one frame showing reset another
		                    // moving camera already luckily, camera will be following the ball later in the
		                    // frame
		                    ballModel[3] = glm::vec4(
                                (willSwitchToAngel || willContinueEnemyRoll || (usr->gameMode == UserContext::GameMode::BOT && IsEnemyTurn(usr)))
                                    ? Enemy_IdleBallPos(usr)
                                    : IDLE_BALL_POS,
                                1.0f
                            );
                            // BOT mode: pins are at different lane end on enemy turn.
                            if (usr->gameMode == UserContext::GameMode::BOT && IsEnemyTurn(usr))
                            {
                                Enemy_ComputePins(usr, usr->initialPins);
                                usr->phy.physics_reset(usr->enemyPins, usr->ballStart, /*reviveAll=*/shouldResetAllPins);
                            }
                            else if (willSwitchToAngel)
                            {
                                // Avoid resetting to player-side pins/ball only to immediately switch to Angel turn.
                                // Enemy_EnterTurn will reset pins + ball for the Angel side.
                            }
                            else
                            {
		                        PhysicsResetForMode(usr, /*reviveAll=*/shouldResetAllPins);
                            }

                            // BOT mode: after a completed frame, yield turn to the other side.
                            if (usr->gameMode == UserContext::GameMode::BOT && frameCompleted)
                            {
                                const bool playerDone = isGameFinished(&usr->board);
                                const bool enemyDone = isGameFinished(&usr->enemyBoard);
                                if (IsEnemyTurn(usr))
                                {
                                    if (!playerDone)
                                        Player_EnterTurn(usr);
                                }
                                else
                                {
                                    if (!enemyDone)
                                        Enemy_EnterTurn(usr, usr->initialPins);
                                }
                            }

				                    if (usr->gameMode == UserContext::GameMode::BOT &&
				                        isGameFinished(&usr->board) &&
                                        isGameFinished(&usr->enemyBoard))
				                    {
				                        // Final outcome SFX (win/lose) vs Angel.
                                        // Tie counts as a loss (player must strictly beat Angel).
                                        const bool playerWins = (usr->board.totalScore > usr->enemyBoard.totalScore);
                                        const CampaignLevelConfig &cfg = Campaign_CurrentLevel(usr);
				                        if (playerWins)
				                        {
				                            usr->sound.playSfxWin();
				                            // Victory confetti at the pin deck.
				                            glm::vec3 p = usr->initialPins[0];
				                            p.y += 0.35f;
				                            usr->particles.burstConfetti(p);
				                        }
				                        else
				                            usr->sound.playSfxLose();

                                        usr->electroBall.resetCharge();
				                        usr->phase = UserContext::Phase::RESULT;
                                        usr->windowStack.windowStackPushNewGameWindow();
                                        if (usr->playerRoute == PlayerRoute::FREESTYLE)
                                        {
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
                                        }

                                        if (usr->playerRoute == PlayerRoute::CAMPAIGN && playerWins)
                                        {
                                            Campaign_AdvanceIfWon(usr, cfg);
                                            Campaign_SetResultWindowLabels(usr, /*advanced=*/true);
                                        }
                                        else if (usr->playerRoute == PlayerRoute::CAMPAIGN)
                                        {
                                            Campaign_SetResultWindowLabels(usr, /*advanced=*/false);
                                        }
                                        else
                                        {
                                            Campaign_SetResultWindowLabels(usr, /*advanced=*/playerWins);
                                        }
                                        if (usr->playerRoute == PlayerRoute::CAMPAIGN && playerWins && cfg.endStoryId != 0)
                                            usr->pendingCampaignEndStoryId = cfg.endStoryId;
                                        usr->pendingCampaignBotResultWindow = true;
                                        usr->pendingCampaignBotPlayerScore = usr->board.totalScore;
                                        usr->pendingCampaignBotEnemyScore = usr->enemyBoard.totalScore;
                                        usr->pendingCampaignBotPlayerWon = playerWins;
		                    }
                            else if (usr->gameMode == UserContext::GameMode::SOLO &&
                                     isGameFinished(&usr->board))
                            {
                                usr->electroBall.resetCharge();
                                usr->phase = UserContext::Phase::RESULT;
                                if (usr->playerRoute == PlayerRoute::CAMPAIGN)
                                {
                                    const CampaignLevelConfig &cfg = Campaign_CurrentLevel(usr);
                                    const bool passed = usr->board.totalScore >= cfg.targetScore;
                                    if (cfg.levelNumber == 1)
                                    {
                                        usr->firstSoloCompleted = true;
                                        if (passed)
                                        {
                                            usr->milestone100Reached = true;
                                            usr->schoolExitLocked = false;
                                        }
                                    }
                                    if (passed)
                                    {
                                        usr->sound.playSfxWin();
                                        glm::vec3 p = usr->initialPins[0];
                                        p.y += 0.35f;
                                        usr->particles.burstConfetti(p);
                                        Campaign_SetResultWindowLabels(usr, /*advanced=*/true);
                                    }
                                    else
                                    {
                                        Campaign_SetResultWindowLabels(usr, /*advanced=*/false);
                                    }
                                    if (passed)
                                        Campaign_AdvanceIfWon(usr, cfg);
                                    if (passed && cfg.endStoryId != 0)
                                        usr->pendingCampaignEndStoryId = cfg.endStoryId;
                                    if (!passed && cfg.levelNumber == 1)
                                        usr->pendingCampaignEndStoryId = 10;
                                }
                                else
                                {
                                    Campaign_SetResultWindowLabels(usr, /*advanced=*/false);
                                }
                            }
			                    else
			                    {
	                                    // School Lesson 3: end the attempt when throw completes (stalled / timeout / fall-off handled),
	                                    // and if the player didn't collect all 3 coins, annul this round and respawn coins.
	                                    if (usr->gameMode == UserContext::GameMode::SCHOOL &&
	                                        usr->school.selectedLesson == 3)
	                                    {
	                                        const int per = SchoolSpinTuning::COINS_PER_LEVEL;
	                                        if (usr->school.spinCollectedInLevel < per)
	                                        {
	                                            usr->school.spinCollectedInLevel = 0;
                                                SchoolServices svc = {};
                                                svc.coinLane = &usr->coinLane;
	                                            SchoolSpin_InitCoinsForLevel(&usr->school, svc, usr->school.spinLevel);
	                                        }
	                                    }
                                        // School Lesson 5: always respawn the coin line when returning to IDLE,
                                        // so the player sees the guide line at the start of every throw.
                                        if (usr->gameMode == UserContext::GameMode::SCHOOL &&
                                            usr->school.selectedLesson == 5)
                                        {
                                            School_StrikeLessonSetupCoins(usr, g_schoolStrikeAimLeftPocket);
                                            g_schoolStrikeSwapInProgress = false;
                                            g_schoolStrikeSwapElapsed = SCHOOL_STRIKE_SWAP_INTERVAL_S;
                                        }

                                        // Every completed throw drains a little of the current charge.
                                        usr->electroBall.consumeChargeAfterThrow();

                                    // BOT mode: if it's still the enemy's turn (i.e. frame not completed),
                                    // immediately re-arm the auto-throw for the next roll instead of going IDLE.
                                    if (usr->gameMode == UserContext::GameMode::BOT && IsEnemyTurn(usr))
                                    {
                                        LogToIdle(usr, "ENEMY_ROLL_DONE_REARM");
                                        UI_ResetBannersForNewRoll(usr, "ENEMY_REARM_ROLL");
                                        usr->enemyAutoTimer = 0.0f;
                                        usr->enemyLaunched = false;
                                        usr->enemyDebugLogged = false;

                                        glm::vec3 pos = Enemy_IdleBallPos(usr);
                                        usr->bufferedRequestThrow = false;
                                        usr->carriedBall = pos;
                                        usr->carriedVel = glm::vec3(0.0f);
                                        usr->throwingTime = 0.0f;
                                        usr->settlingTime = 0.0f;
                                        usr->aimingTime = 0.0f;
                                        usr->phase = UserContext::Phase::THROW;
                                        usr->enjoy.resetJoystick();
                                        usr->aimFlatPos = glm::vec2(0.5f, 0.5f);
                                        usr->aimDownFlatPos = usr->aimFlatPos;
                                        usr->phy.set_ball_free();
                                        usr->phy.set_manual_ball_position(pos, glm::quat(1.0f, 0, 0, 0), deltaTime);
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
		                else
		                {
		                    // Negative banner: STALLED when the throw exceeds the time budget.
		                    // (Doesn't end the throw early; just informs the player.)
				                    float totalThrowTime = usr->throwingTime + usr->settlingTime;
				                    float stalledBannerAtS = 10.0f;
				                    if (usr->gameMode == UserContext::GameMode::SCHOOL &&
				                        usr->school.selectedLesson == 3)
				                    {
				                        stalledBannerAtS = SchoolSpinTuning::STALLED_BANNER_AT_S;
				                    }
	                    if (usr->negativeBannerFlashTime <= 0.0f && totalThrowTime > stalledBannerAtS)
			                    {
			                        usr->negativeBannerKind = 2;
			                        usr->negativeBannerFlashTime = 1.25f;
			                        if (usr->negativeBannerSfxPlayedKind != 2)
		                        {
                                    BallRollingSfx_Stop(usr);
		                            usr->sound.playSfxBallTimeout();
		                            usr->negativeBannerSfxPlayedKind = 2;
		                        }
		                    }

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
        physicsInterval = lowFpsAimSwingFrame ? 0.080f : 0.050f; // Loosen cost on slow devices
    }
    if (usr->phase == UserContext::Phase::SWING)
    {
        physicsInterval = lowFpsAimSwingFrame ? 0.010f : 0.005f; // More forgiving under slow WASM/Android
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

	    // Ball<->lane impacts (SFX + screenshake).
	    // Done in game.cpp (not physics) so you can hot-reload tuning & behavior.
	    if (usr->phase == UserContext::Phase::THROW || usr->phase == UserContext::Phase::RESULT)
	    {
	        float dt = (float)deltaTime;
	        glm::vec3 pos = glm::vec3(usr->phy.physics_get_ball_matrix()[3]);

	        if (!usr->laneImpactPrevValid || dt <= 1e-6f || !std::isfinite(pos.y))
	        {
	            usr->laneImpactPrevPos = pos;
	            usr->laneImpactPrevValid = true;
	        }
	        else
	        {
	            usr->laneImpactCooldownT = glm::max(0.0f, usr->laneImpactCooldownT - dt);

	            float vy = (pos.y - usr->laneImpactPrevPos.y) / dt;
	            float downV = glm::max(0.0f, -vy);

	            // Require airtime: the ball must rise above a threshold between impacts.
	            if (pos.y > LaneImpactTuning::AIRBORNE_CENTER_Y_MIN)
	                usr->laneImpactHadAirtime = true;

	            bool nearLane = pos.y <= LaneImpactTuning::CONTACT_CENTER_Y_MAX;
	            bool meaningful = downV >= LaneImpactTuning::MIN_DOWN_VY;

	            if (usr->laneImpactHadAirtime && usr->laneImpactCooldownT <= 0.0f && nearLane && meaningful)
	            {
	                usr->laneImpactHitCount += 1;
	                usr->sound.playSfxBallHitLane();
                    if (usr->phase == UserContext::Phase::THROW)
                        BallRollingSfx_Start(usr);

	                // Shake strength: E = m * c^2 (c=downward speed). Then attenuate per bounce:
	                // 1.0, 0.5, 0.25, ... within the same throw.
	                float m = glm::max(0.001f, usr->desiredMass);
	                float E = m * downV * downV;
	                if (!std::isfinite(E))
	                    E = 0.0f;

	                float amp = glm::clamp(
	                    E * LaneImpactTuning::ENERGY_TO_SHAKE, 0.0f, LaneImpactTuning::SHAKE_AMP_MAX_M
	                );
	                float bounceMul = powf(0.5f, (float)usr->laneImpactBounceIndex);
	                usr->laneImpactBounceIndex += 1;
	                amp *= bounceMul;

	                usr->laneImpactShakeAmp = glm::max(usr->laneImpactShakeAmp, amp);
	                usr->laneImpactShakeTime = usr->laneImpactShakeDuration;

	                usr->laneImpactHadAirtime = false;
	                usr->laneImpactCooldownT = LaneImpactTuning::COOLDOWN_S;
	            }

	            usr->laneImpactPrevPos = pos;
	        }
	    }
	    else
	    {
	        usr->laneImpactPrevValid = false;
	        usr->laneImpactHadAirtime = true;
	        usr->laneImpactCooldownT = 0.0f;
	    }
        if (usr->phase != UserContext::Phase::THROW)
            BallRollingSfx_Stop(usr);

	    Carousel_Update(&usr->carousel, deltaTime);

    // Early strike/spare detection (without ending the throw early).
    // We show STRIKE/SPARE as soon as all pins are down, but we still let physics settle
    // and end the THROW/RESULT flow using the original completion logic.
    if (!(usr->gameMode == UserContext::GameMode::SCHOOL && usr->school.selectedLesson == 3) &&
        usr->phase == UserContext::Phase::THROW && usr->strikeSpareFlashTime <= 0.0f)
    {
        int down = usr->phy.estimatePinsDown(-0.1f);
        if (down >= 10)
            usr->strikeSpareEarlyAllDownTime += (float)deltaTime;
        else
            usr->strikeSpareEarlyAllDownTime = 0.0f;

        // Only show early celebration once the rack has stayed "all down" for a short time,
        // to avoid false positives where a pin bounces back up.
        if (usr->strikeSpareEarlyAllDownTime >= 0.20f)
        {
            usr->strikeSpareKind = (usr->wereDead == 0) ? 1 : 2;
            usr->strikeSpareFlashTime = 1.25f;
            usr->strikeSpareEarlyDeclared = true;
            usr->strikeSpareEarlyKind = usr->strikeSpareKind;
            usr->strikeSpareEarlyDeclaredAt = usr->globalTime;
            std::cerr << "[celebrate] EARLY=" << (usr->strikeSpareKind == 1 ? "STRIKE" : "SPARE")
                      << " t=" << usr->globalTime << "s\n";
            if (usr->strikeSpareKind == 1 && usr->strikeSpareSfxPlayedKind != 1)
            {
                usr->sound.playSfxStrike();
                usr->strikeSpareSfxPlayedKind = 1;
            }
            else if (usr->strikeSpareKind == 2 && usr->strikeSpareSfxPlayedKind != 2)
            {
                usr->sound.playSfxSpare();
                usr->strikeSpareSfxPlayedKind = 2;
            }
            usr->strikeSpareEarlyAllDownTime = 0.0f;
        }
    }
    else
    {
        usr->strikeSpareEarlyAllDownTime = 0.0f;
        if (usr->phase != UserContext::Phase::THROW)
        {
            usr->strikeSpareEarlyDeclared = false;
            usr->strikeSpareEarlyKind = 0;
            usr->strikeSpareEarlyDeclaredAt = 0.0f;
        }
    }

    BallStats_EveryFrame(usr, ballModel);

		    glm::vec3 desiredEye, desiredTarget;
            if (usr->gameMode == UserContext::GameMode::BOT && IsEnemyTurn(usr))
            {
                // Enemy turn camera:
                // Look from the *player* side (same side as normal play), so the enemy ball
                // rolls toward the camera instead of the camera moving "backwards".
                glm::vec3 ballPos = glm::vec3(ballModel[3]);
                // Always use the render-ball Z during the Angel throw clip; it starts hand-attached
                // and then smoothly chases the physics ball, avoiding any idle-pos flash/jump.
                const int throwClip = Bot_ClipThrow(usr);
                AssmanAnimPlayer *anim = Bot_Anim(usr);
                if (usr->enemyBallRenderPosValid && Bot_AnimReady(usr) && throwClip >= 0 && anim &&
                    anim->activeClip == throwClip && !anim->loop)
                {
                    ballPos.z = usr->enemyBallRenderPos.z;
                }
                Enemy_ComputeCameraEyeTargetAtBall(ballPos, desiredEye, desiredTarget);
            }
            else
            {
		        Scene_ComputeCameraEyeTarget(usr->scene, glm::vec3(ballModel[3]), desiredEye, desiredTarget);
            }

		    // If we just finished a throw, smoothly return camera to the IDLE view.
		    glm::vec3 eye = desiredEye;
		    glm::vec3 target = desiredTarget;
		    if (usr->cameraReturnActive)
		    {
		        const float returnDuration = glm::max(1e-6f, usr->cameraReturnDuration);
		        usr->cameraReturnT += (deltaTime / returnDuration);
		        float t = glm::clamp(usr->cameraReturnT, 0.0f, 1.0f);
		        // Easing: ease-out cubic (quickly starts returning, then smoothly settles).
		        float inv = 1.0f - t;
		        float ease = 1.0f - inv * inv * inv;
		        eye = glm::mix(usr->cameraReturnStartEye, usr->cameraReturnEndEye, ease);
		        target = glm::mix(usr->cameraReturnStartTarget, usr->cameraReturnEndTarget, ease);
		        if (t >= 1.0f)
		        {
		            usr->cameraReturnActive = false;
		            usr->cameraReturnT = 0.0f;
		        }
		    }

		    // Screen shake: subtle down then up (applied after camera return blend).
		    if (usr->laneImpactShakeTime > 0.0f && usr->laneImpactShakeDuration > 1e-6f)
		    {
		        float t = 1.0f - glm::clamp(usr->laneImpactShakeTime / usr->laneImpactShakeDuration, 0.0f, 1.0f);
		        // 0 -> down -> 0
		        float s = sinf(t * 3.1415926f);
		        float yOff = -usr->laneImpactShakeAmp * s;
		        eye.y += yOff;
		        target.y += yOff;
		    }

            // Screen shake: pin hits (smaller, accumulative, and eased out).
            if (usr->pinHitShakeTime > 0.0f && usr->pinHitShakeDuration > 1e-6f)
            {
                float t = 1.0f - glm::clamp(usr->pinHitShakeTime / usr->pinHitShakeDuration, 0.0f, 1.0f);
                float s = sinf(t * 3.1415926f);
                float fade = 1.0f - t;
                float yOff = -usr->pinHitShakeAmp * s * fade;
                eye.y += yOff;
                target.y += yOff;
            }

		    usr->cameraEye = eye;
		    usr->cameraTarget = target;
		    usr->cameraMat = glm::lookAt(
		        eye,
		        target,
		        glm::vec3(0.0f, 1.0f, 0.0f)
		    );
		    usr->cameraMat[3][0] = usr->pivotPoint.x;

    SDL_GL_GetDrawableSize(ctx->sdlWindow, &ctx->screenWidth, &ctx->screenHeight);

    const bool showElectroBall =
        ctx->screenWidth > 0 &&
        ctx->screenHeight > 0 &&
        usr->phase != UserContext::Phase::FINAL_RESULT &&
        !(usr->gameMode == UserContext::GameMode::BOT && IsEnemyTurn(usr));
    usr->electroBall.updateElectroBall((float)deltaTime, glm::vec3(ballModel[3]), showElectroBall);

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
    }
    else
    {
        BallRollingSfx_Stop(usr);
        usr->electroBall.updateElectroBall((float)deltaTime, glm::vec3(0.0f), false);
        usr->laneImpactPrevValid = false;
        usr->laneImpactHadAirtime = true;
        usr->laneImpactCooldownT = 0.0f;
        usr->enjoy.resetJoystick();
        usr->circle.resetCircle();
    }

	    // ===== [NEW] PRE-PASS: Render ball to texture for UI =====

    // (Do this AFTER ballModel is computed, BEFORE any rendering)
    if (!trackerOnlyMode)
	    {
        ZONE("RENDER TO TEXTURE FRAMEBUFFER")
        {
        float step = 1.0f / 16.0f;
        usr->mainShader.updateDiffuseTexture(usr->everythingTexture);

        // ── Icon camera: closer + simple ──
	        const glm::mat4 iconView = glm::lookAt(
	            glm::vec3(0.0f, 0.45f, 0.60f), // eye (closer for wider 16:6 preview)
	            glm::vec3(0.0f, 0.0f, 0.0f),  // center
	            glm::vec3(
	                0.0f, -1.0f, 0.0f
	            ) // up, normally it is possitive Y, but this is tocompensate Y flip
	        );
	    // NOTE: The preview render textures are sampled into UI rectangles that may not be square
	    // (e.g. Shop uses a 16:6 preview). Because the render texture is square (256x256), Clay will
	    // stretch it horizontally when drawn into a wide rect, making the ball look "flat on Y".
	    // Compensate by rendering with the same wide aspect so the rendered ball is narrower, and
	    // the UI stretch brings it back to round.
	    const float shopPreviewAspect = 16.0f / 6.0f;
	    const glm::mat4 iconProj = glm::perspective(glm::radians(30.0f), shopPreviewAspect, 0.1f, 50.0f);

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
	        // When Houses window is open, reuse the two "ball preview" render textures to render lane previews instead.
	        if (usr->clayton.shouldShowBotSelect)
	        {
                usr->mainShader.updateLightPos(
                    glm::vec3(2.0f, 3.0f, -2.0f) // fixed front-top-right for consistent icon lighting
                );
	            // Ensure preview slot 3 points at the 3rd texture.
	            usr->clayton.renderer.imageTextures[3] = usr->oilRenderTex.colorTexture;

	            Bot_InitIfNeeded(usr);
	            // Ensure all bot anims advance even when not the currently active gameplay avatar.
	            // In BOT mode they already advance via `Angel_Tick`, so avoid double-ticking.
	            if (usr->gameMode != UserContext::GameMode::BOT)
	            {
	                if (gAngelAnimReady) gAngelAnim.tick((float)deltaTime);
	                if (gCherubAnimReady) gCherubAnim.tick((float)deltaTime);
	                if (gSeraphAnimReady) gSeraphAnim.tick((float)deltaTime);
	                if (gThroneAnimReady) gThroneAnim.tick((float)deltaTime);
	            }

	            // "Catalog" camera: ~4m away, looking at the avatar in idle pose.
	            const glm::mat4 botPrevView = glm::lookAt(
	                glm::vec3(0.0f, 0.65f, -4.0f), // eye (4m away, slightly higher)
	                glm::vec3(0.0f, 1.85f, 0.0f), // center (aim at upper torso/head)
	                glm::vec3(0.0f, 1.0f, 0.0f)   // up
	            );
	            const glm::mat4 botPrevProj = glm::perspective(glm::radians(30.0f), 1.0f, 0.1f, 80.0f);

	            auto renderBotPreview = [&](RenderTexture &rt, int botIdx)
	            {
	                if (botIdx < 0 || botIdx >= usr->botsCarousel.cardCount)
	                    return;
	                const BotCatalogItem *bot = &usr->botsCarousel.items[botIdx];

	                BotAvatar avatar = BotAvatar::ANGEL;
	                if (bot->kind == BotCatalogAvatar_CHERUB)
	                    avatar = BotAvatar::CHERUB;
	                else if (bot->kind == BotCatalogAvatar_SERAPH)
	                    avatar = BotAvatar::SERAPH;
	                else if (bot->kind == BotCatalogAvatar_THRONE)
	                    avatar = BotAvatar::THRONE;

	                AssetMesh *mesh = &gAngelMesh;
	                bool meshReady = gAngelMeshReady;
	                AssmanAnimPlayer *anim = &gAngelAnim;
	                bool animReady = gAngelAnimReady;
	                int idleClip = usr->angelClipArgument;
	                if (avatar == BotAvatar::CHERUB)
	                {
	                    mesh = &gCherubMesh;
	                    meshReady = gCherubMeshReady;
	                    anim = &gCherubAnim;
	                    animReady = gCherubAnimReady;
	                    idleClip = usr->cherubClipArgument;
	                }
	                else if (avatar == BotAvatar::SERAPH)
	                {
	                    mesh = &gSeraphMesh;
	                    meshReady = gSeraphMeshReady;
	                    anim = &gSeraphAnim;
	                    animReady = gSeraphAnimReady;
	                    idleClip = usr->seraphClipArgument;
	                }
	                else if (avatar == BotAvatar::THRONE)
	                {
	                    mesh = &gThroneMesh;
	                    meshReady = gThroneMeshReady;
	                    anim = &gThroneAnim;
	                    animReady = gThroneAnimReady;
	                    idleClip = usr->throneClipArgument;
	                }

	                if (!meshReady || !animReady || !mesh || !anim)
	                    return;

	                rt.bindForWriting();
	                glClearColor(0, 0, 0, 1);
	                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	                // Background (avoid double-advancing aurora).
	                glDisable(GL_DEPTH_TEST);
	                glDepthMask(GL_FALSE);
	                usr->aurora.renderAurora(0.0f, glm::inverse(botPrevView), usr->auroraVibe.value);
	                glUseProgram(usr->mainShader.id);
	                glEnable(GL_DEPTH_TEST);
	                glDepthMask(GL_TRUE);

	                // Force looping "Argument" clip for catalog look, but don't reset every frame
	                // (so the mugshot actually animates).
	                if (idleClip >= 0 && anim->activeClip != idleClip)
	                {
	                    anim->setClip(idleClip, /*resetTime=*/true);
	                }
	                anim->loop = true;

	                const std::vector<glm::mat4> &bones = anim->evaluate();
	                if (!bones.empty())
	                    usr->mainShader.updateBoneTransformData(bones);

	                usr->mainShader.updateDiffuseTexture(usr->everythingTexture);
	                usr->mainShader.updateTextureParamsInOneGo(
	                    glm::vec3(1.0f),
	                    glm::vec2(1.0f),
	                    glm::vec2(1.0f),
	                    1.0f
	                );

	                glm::mat4 model = BotPreview_ComputeModelMatrix(usr, avatar);
	                usr->mainShader.renderRealMesh(*mesh, model, botPrevView, botPrevProj);

	                rt.unbind(ctx->screenWidth * ctx->pixelRatio, ctx->screenHeight * ctx->pixelRatio);
	            };

	            renderBotPreview(usr->ballRenderTex, usr->botsCarousel.closestBotIdx);
	            renderBotPreview(usr->ballRenderTex2, usr->botsCarousel.closest2ndBotIdx);
	            renderBotPreview(usr->oilRenderTex, usr->botsCarousel.closest3rdBotIdx);
	        }
	        else if (usr->clayton.shouldShowHouses)
	        {
	            // While Houses is open, texture slot 3 is used as the 3rd preview image.
	            usr->clayton.renderer.imageTextures[3] = usr->oilRenderTex.colorTexture;
		            const glm::mat4 lanePrevView = glm::lookAt(
		                // Close-up camera aimed at the pin deck (about 2–3m away).
		                glm::vec3(0.0f, 0.80f, -2.0f),
		                glm::vec3(0.0f, 0.25f, 0.60f),
		                glm::vec3(0.0f, 1.0f, 0.0f)
		            );
            const glm::mat4 lanePrevProj = glm::perspective(glm::radians(35.0f), 1.0f, 0.1f, 80.0f);

	            auto renderLanePreview = [&](RenderTexture &rt, int houseIdx)
	            {
	                if (houseIdx < 0 || houseIdx >= usr->housesCarousel.cardCount)
	                    return;
	            const HouseCatalogItem *house = &usr->housesCarousel.items[houseIdx];
	
	                rt.bindForWriting();
	                glClearColor(0, 0, 0, 1);
	                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	                // Aurora background (same as main scene), rendered into the preview FBO.
	                // Use 0 deltaTime to avoid double-advancing animation when the window is open.
	                glDisable(GL_DEPTH_TEST);
	                glDepthMask(GL_FALSE);
	                usr->aurora.renderAurora(
	                    0.0f,
	                    glm::inverse(lanePrevView),
	                    usr->auroraVibe.value
	                );
	
	                // Aurora uses its own shader program; switch back to main shader before setting uniforms / drawing meshes.
	                glUseProgram(usr->mainShader.id);
	
	                glEnable(GL_DEPTH_TEST);
	                glDepthMask(GL_TRUE);
	
	                // Lane texture for this house.
	                // The lane mesh UVs are already authored in 1/8 steps inside the atlas (u is in the lane column).
	                // So selecting a different lane background is just a V offset by N * (1/8).
	                {
                    float cell = 1.0f / 8.0f;
                    int idx = glm::clamp(house->laneTextureIdx, 0, 3); // 0=default, 1..3 = next cells down
                    // IMPORTANT: atlasStart override only kicks in when u_atlasStart.* is non-zero.
                    // We keep x at 1.0 (neutral due to REPEAT wrap).
                    // For lane backgrounds, our atlas rows are authored from TOP->BOTTOM in the PNG,
                    // but UV v=0 is the BOTTOM. So selecting house variants is an *upward* shift in V.
                    usr->mainShader.updateTextureParamsInOneGo(
                        glm::vec3(1.0f),
                        glm::vec2(1.0f),
                        glm::vec2(1.0f, 1.0f + (float)idx * cell),
                        1.0f
                    );
                }

	                // Nudge lane slightly forward so the full rack sits on the lane in the close-up preview.
	                const float lanePreviewZOffsetM = 0.61f; // ~2ft
	                usr->mainShader.renderRealMesh(
	                    usr->laneMesh,
	                    glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, lanePreviewZOffsetM)),
	                    lanePrevView,
	                    lanePrevProj
	                );
	
	                // Pin preview: show the full rack (10 pins) with the house's pin variant.
	                {
	                    const float cell = 1.0f / 8.0f;
	                    const int idx = glm::clamp(house->pinTextureIdx, 0, 3);
	                    usr->mainShader.updateTextureParamsInOneGo(
	                        glm::vec3(1.0f),
	                        glm::vec2(1.0f),
	                        glm::vec2(1.0f, 1.0f + (float)idx * cell),
	                        1.0f
	                    );
	
	                    const float halfHeight = 0.19f;
	                    for (int i = 0; i < 10; i++)
	                    {
	                        glm::mat4 pinModel = glm::translate(glm::mat4(1.0f), usr->initialPins[i]);
	                        pinModel = glm::translate(pinModel, glm::vec3(0.0f, -halfHeight, 0.0f));
	                        usr->mainShader.renderRealMesh(
	                            usr->pinMesh,
	                            pinModel,
	                            lanePrevView,
	                            lanePrevProj
	                        );
	                    }
	                }
	                rt.unbind(ctx->screenWidth * ctx->pixelRatio, ctx->screenHeight * ctx->pixelRatio);
	            };
	
	            renderLanePreview(usr->ballRenderTex, usr->housesCarousel.closestHouseIdx);
	            renderLanePreview(usr->ballRenderTex2, usr->housesCarousel.closest2ndHouseIdx);
	            renderLanePreview(usr->oilRenderTex, usr->housesCarousel.closest3rdHouseIdx);
	
	            // Restore default atlas.
	            usr->mainShader.updateTextureParamsInOneGo(
	                glm::vec3(1.0f),
	                glm::vec2(1.0f),
	                glm::vec2(1.0f),
	                1.0f
	            );
	        }
	        else if (usr->carousel.closestBallIdx != -1)
	        {
	            usr->clayton.renderer.imageTextures[3] = usr->oilRenderTex.colorTexture;
	            usr->ballRenderTex.bindForWriting();
	            glClearColor(0, 0, 0, 1);
	            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	            // Aurora background in preview (avoid advancing aurora time twice per frame).
	            glDisable(GL_DEPTH_TEST);
	            glDepthMask(GL_FALSE);
	            {
	                float savedTime = usr->aurora.time;
	                usr->aurora.renderAurora(usr->deltaTimeLoan * TUNE, glm::inverse(iconView), usr->auroraVibe.value);
	                usr->aurora.time = savedTime;
	            }
	            // Aurora uses its own shader program; switch back to main shader before setting uniforms / drawing meshes.
	            glUseProgram(usr->mainShader.id);
	
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
		        if (!usr->clayton.shouldShowHouses && usr->carousel.closest2ndBallIdx != -1)
		        {
	            usr->ballRenderTex2.bindForWriting();
	            glClearColor(0, 0, 0, 1);
	            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	            // Aurora background in preview (avoid advancing aurora time twice per frame).
	            glDisable(GL_DEPTH_TEST);
	            glDepthMask(GL_FALSE);
	            {
	                float savedTime = usr->aurora.time;
	                usr->aurora.renderAurora(usr->deltaTimeLoan * TUNE, glm::inverse(iconView), usr->auroraVibe.value);
	                usr->aurora.time = savedTime;
	            }
	            // Aurora uses its own shader program; switch back to main shader before setting uniforms / drawing meshes.
	            glUseProgram(usr->mainShader.id);
	
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
	
		        // 3rd closest ball preview (reuse oilRenderTex slot 3 when Oil Status isn't visible).
		        if (!usr->clayton.shouldShowHouses && !usr->clayton.shouldShowOilStatus && usr->carousel.closest3rdBallIdx != -1)
		        {
		            usr->oilRenderTex.bindForWriting();
		            glClearColor(0, 0, 0, 1);
		            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
		            // Aurora background in preview (avoid advancing aurora time twice per frame).
		            glDisable(GL_DEPTH_TEST);
		            glDepthMask(GL_FALSE);
		            {
		                float savedTime = usr->aurora.time;
		                usr->aurora.renderAurora(usr->deltaTimeLoan * TUNE, glm::inverse(iconView), usr->auroraVibe.value);
		                usr->aurora.time = savedTime;
		            }
		            glUseProgram(usr->mainShader.id);
		            glEnable(GL_DEPTH_TEST);
		            glDepthMask(GL_TRUE);
	
		            int ballId = usr->carousel.items[usr->carousel.closest3rdBallIdx].id;
		            float stepx = 1.0f + step * 2.0f * (float)(ballId / 16);
		            float stepy = 1.0f + step * (float)(ballId % 16);
		            usr->mainShader.updateTextureParamsInOneGo(
		                glm::vec3(1.0f, 1.0f, 1.0f),
		                glm::vec2(1.0f, 1.0f),
		                glm::vec2(stepx, stepy),
		                1.0f
		            );
		            usr->mainShader.renderRealMesh(usr->ballMesh, iconModel, iconView, iconProj);
		            checkOpenGLError("icon-ball-3");
	
	                usr->oilRenderTex.unbind(
	                    ctx->screenWidth * ctx->pixelRatio, ctx->screenHeight * ctx->pixelRatio
	                );
		        }

		        // Oil preview (only when Oil Status window is visible).
		        if (usr->clayton.shouldShowOilStatus)
		        {
		            // Ensure slot 3 points at the oil map when Oil Status is open.
		            usr->clayton.renderer.imageTextures[3] = usr->oilRenderTex.colorTexture;
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
                        usr->houseLane.leftOilFadeStartM,
                        usr->houseLane.leftOilFadeEndM,
		                usr->laneOilThickness,
		                glm::clamp(usr->lanePushbackStrength / 50.0f, 0.0f, 1.0f)
		            );

	            usr->oilRenderTex.unbind(
	                ctx->screenWidth * ctx->pixelRatio, ctx->screenHeight * ctx->pixelRatio
	            );
	        }
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

        usr->globalTime += deltaTime;
        if (!trackerOnlyMode)
        {
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
	        // Pins: UV-shifted pin texture per selected house.
	        // Base pin tile is authored in atlas column 0 at (col=0,row=7) in an 8x8 conceptual grid,
	        // and variants are stacked downwards in the atlas; selection uses the same 1/8 V stepping
	        // as lane variants.
	        {
	            const float cell = 1.0f / 8.0f;
	            const int idx = glm::clamp(usr->pinTextureIdx, 0, 3);
	            usr->mainShader.updateTextureParamsInOneGo(
	                glm::vec3(1.0f),
	                glm::vec2(1.0f),
	                glm::vec2(1.0f, 1.0f + (float)idx * cell),
	                1.0f
	            );
	        }
		        if (!(usr->gameMode == UserContext::GameMode::SCHOOL &&
	                      usr->school.selectedLesson == 3))
		        {
		            for (int i = 0; i < 10; i++)
		            {
		                glm::mat4 pinModel = usr->phy.physics_get_pin_matrix(i);
		                float halfHeight = 0.19f;
		                pinModel = glm::translate(pinModel, glm::vec3(0.0f, -halfHeight, 0.0f));
		                usr->mainShader.renderRealMesh(
		                    usr->pinMesh, pinModel, usr->cameraMat, usr->perspectiveMat
		                );
		                checkOpenGLError("stare");
		            }
		        }

        // BOT avatar (Angel / Cherub) — only shown in BOT mode.
        glm::vec3 botRightHandWorld = glm::vec3(0.0f);
        bool haveBotRightHandWorld = false;
        bool botThrowClipActive = false;
        float botThrowNormT = 0.0f;
        if (usr->gameMode == UserContext::GameMode::BOT)
        {
            Bot_InitIfNeeded(usr);
            AssetMesh *mesh = &gAngelMesh;
            bool meshReady = gAngelMeshReady;
            if (usr->botAvatar == BotAvatar::CHERUB)
            {
                mesh = &gCherubMesh;
                meshReady = gCherubMeshReady;
            }
            else if (usr->botAvatar == BotAvatar::SERAPH)
            {
                mesh = &gSeraphMesh;
                meshReady = gSeraphMeshReady;
            }
            else if (usr->botAvatar == BotAvatar::THRONE)
            {
                mesh = &gThroneMesh;
                meshReady = gThroneMeshReady;
            }
            AssmanAnimPlayer *anim = Bot_Anim(usr);
            const bool animReady = Bot_AnimReady(usr);
            if (meshReady)
            {
                // Default atlas params (UVs authored in the atlas space).
                usr->mainShader.updateTextureParamsInOneGo(
                    glm::vec3(1.0f),
                    glm::vec2(1.0f),
                    glm::vec2(1.0f),
                    1.0f
                );

                glm::mat4 botModel(1.0f);
                if (animReady && anim)
                {
                    const std::vector<glm::mat4> &bones = anim->evaluate();
                    if (!bones.empty())
                        usr->mainShader.updateBoneTransformData(bones);
                }

                botModel = Angel_ComputeModelMatrix(usr);
                if (usr->botAvatar == BotAvatar::CHERUB)
                    botModel = Cherub_ComputeModelMatrix(usr);
                else if (usr->botAvatar == BotAvatar::SERAPH)
                    botModel = Seraph_ComputeModelMatrix(usr);
                else if (usr->botAvatar == BotAvatar::THRONE)
                    botModel = Throne_ComputeModelMatrix(usr);

                // Compute right-hand attachment world position for syncing the (render-only) enemy ball.
                if (Angel_ComputeRightHandAttachPosWorld(usr, botRightHandWorld))
                {
                    haveBotRightHandWorld = true;
                }

                // Cache whether we're in the non-looping throw clip, and its normalized time.
                const int throwClip = Bot_ClipThrow(usr);
                if (animReady && anim && throwClip >= 0 && anim->activeClip == throwClip && !anim->loop)
                {
                    float dur = Bot_ClipDurationSeconds(usr, throwClip);
                    if (dur > 0.0f)
                    {
                        botThrowClipActive = true;
                        botThrowNormT = glm::clamp(anim->t / dur, 0.0f, 1.0f);
                    }
                }
                usr->mainShader.renderRealMesh(
                    *mesh, botModel, usr->cameraMat, usr->perspectiveMat
                );
            }
        }

        /*
         * Mostly for decals other bodies are not even see-through
         */
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);

        float step = 1.0f / 16.0f;
        int ballId = usr->myBall.id;
        if (usr->gameMode == UserContext::GameMode::BOT && IsEnemyTurn(usr))
        {
            // Enemy uses a different ball texture variant for readability.
            ballId = (ballId + 7) % 32;
        }
        float stepx = 1.0f + step * 2.0f * (float)(ballId / 16);
        float stepy = 1.0f + step * (float)(ballId % 16);
        usr->mainShader.updateTextureParamsInOneGo(
            glm::vec3(1.0f, 1.0f, 1.0f), // Texture density
            glm::vec2(1.0f, 1.0f),       // Size of one tile compared to full atlas
            glm::vec2(stepx, stepy),     // Atlas region start
            1.0f                         // Atlas region scale compared to entire atlas
        );

        // Lane texture depends on selected house.
        // The lane mesh UVs are already authored in 1/8 steps inside the atlas (u is in the lane column),
        // so selection is just a V offset by N * (1/8).
        {
            float cell = 1.0f / 8.0f;
            int idx = glm::clamp(usr->laneTextureIdx, 0, 3); // 0=default, 1..3 = next cells down
            usr->mainShader.updateTextureParamsInOneGo(
                glm::vec3(1.0f),
                glm::vec2(1.0f),
                glm::vec2(1.0f, 1.0f + (float)idx * cell),
                1.0f
            );
        }
        usr->mainShader.renderRealMesh(
            usr->laneMesh,
            glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -.0f, .0f)),
            usr->cameraMat,
            usr->perspectiveMat
        );
        // Restore default atlas for any later draws.
        usr->mainShader.updateTextureParamsInOneGo(
            glm::vec3(1.0f, 1.0f, 1.0f),
            glm::vec2(1.0f, 1.0f),
            glm::vec2(1.0f),
            1.0f
        );

        // BOT mode: while Angel's throw clip is active, render enemy ball using the update-driven
        // smoothed render position (hand -> idle -> physics catch-up).
        if (usr->gameMode == UserContext::GameMode::BOT &&
            IsEnemyTurn(usr) &&
            botThrowClipActive &&
            usr->enemyBallRenderPosValid)
        {
            ballModel[3] = glm::vec4(usr->enemyBallRenderPos, 1.0f);
        }

        usr->mainShader.renderRealMesh(
            usr->ballMesh, ballModel, usr->cameraMat, usr->perspectiveMat
        );
        usr->electroBall.renderElectroBallSurface(
            usr->ballMesh,
            ballModel,
            usr->cameraMat,
            usr->perspectiveMat
        );
        usr->electroBall.renderElectroBallShell(
            usr->ballMesh,
            ballModel,
            usr->cameraMat,
            usr->perspectiveMat
        );
        // restore defaults
        usr->mainShader.updateTextureParamsInOneGo(
            glm::vec3(1.0f, 1.0f, 1.0f), // Texture density
            glm::vec2(1.0f, 1.0f),       // Size of one tile compared to full atlas
            glm::vec2(1.0f),             // Atlas region start
            1.0f                         // Atlas region scale compared to entire atlas
        );

        // Particles - rendered in 3D space after opaque geometry.
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_TRUE);
        glDisable(GL_CULL_FACE);
        const float snowSpinDeltaRadians = usr->phy.get_ball_angular_velocity().y * (float)deltaTime;
        usr->particles.setSnowflakeCount(usr->settings.snowflakeCount);
        usr->particles.drawSnow((float)deltaTime, snowSpinDeltaRadians, usr->cameraMat, usr->perspectiveMat);
        usr->particles.draw((float)deltaTime, usr->cameraMat, usr->perspectiveMat);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_TRUE);

        if (usr->strikeSpareFlashTime > 0.0f)
            usr->strikeSpareFlashTime = glm::max(0.0f, usr->strikeSpareFlashTime - (float)deltaTime);
        if (usr->negativeBannerFlashTime > 0.0f)
            usr->negativeBannerFlashTime = glm::max(0.0f, usr->negativeBannerFlashTime - (float)deltaTime);
        if (usr->neutralBannerFlashTime > 0.0f)
            usr->neutralBannerFlashTime = glm::max(0.0f, usr->neutralBannerFlashTime - (float)deltaTime);
        if (usr->laneImpactShakeTime > 0.0f)
            usr->laneImpactShakeTime = glm::max(0.0f, usr->laneImpactShakeTime - (float)deltaTime);
	        if (usr->pinHitShakeTime > 0.0f)
	            usr->pinHitShakeTime = glm::max(0.0f, usr->pinHitShakeTime - (float)deltaTime);

            // School module tick (celebration pauses, etc).
            if (usr->gameMode == UserContext::GameMode::SCHOOL)
                School_Tick(&usr->school, (float)deltaTime);

        // coin_update.cpp — Call this once per frame from your main update loop
        // Assumes: usr->coinLane, usr->globalTime, deltaTime, ctx->screenWidth/Height, etc.

        // 1. Update coin physics/collision FIRST using previous frame position (sets Collected state)
        usr->coinLane.updateStars(usr->lastBallPosition, ballModel[3], usr->globalTime, deltaTime);

                // 2. Update all flying coin animations
                float earned = usr->coinLane.updateFlyAnimations(deltaTime);
                if (usr->gameMode != UserContext::GameMode::SCHOOL)
                    usr->carousel.bank += earned;

        // 3. Cleanup finished fly animations (free slots for new coins)
        usr->coinLane.cleanupFinishedFlyAnimations();

        // 4. Detect newly collected coins and spawn fly animations
	        const auto &coins = usr->coinLane.getCoins(); // ✅ Keep this line
            // Don't clear the completion flag while we're in the level-complete celebration flow.
            // We clear it explicitly after we advance to the next level.
            if (!(usr->gameMode == UserContext::GameMode::SCHOOL && usr->school.celebrateKind == 3))
		        usr->school.spinLevelJustCompleted = false;
        for (int i = 0; i < usr->coinLane.getActiveCount(); ++i)
        {
            const Coin &coin = coins[i];

            // ✅ Simplified condition
		            if (coin.state == CoinState::Collected && !coin.flyTriggered)
		            {
		                // School lesson 3: count coin pickups toward the spin/drive test.
		                if (usr->gameMode == UserContext::GameMode::SCHOOL &&
		                    usr->school.selectedLesson == 3)
		                {
                            // Ignore pickups while returning to start.
                            if (usr->school.returnToStartActive)
                            {
                                usr->coinLane.markFlyTriggered(i);
                                continue;
                            }
		                    usr->school.spinCollectedInLevel =
		                        glm::min(usr->school.spinCollectedInLevel + 1, SchoolSpinTuning::COINS_PER_LEVEL);
		                    if (usr->school.spinCollectedInLevel >= SchoolSpinTuning::COINS_PER_LEVEL)
                            {
		                        usr->school.spinLevelJustCompleted = true;
                                // Fireworks immediately when the last coin is picked.
                                if (usr->school.celebrateKind != 3)
                                {
                                    usr->school.celebrateKind = 3;
                                    usr->school.celebratePauseT = 0.5f;
                                    usr->sound.playSfxWin();
                                    glm::vec3 p = usr->initialPins[0];
                                    p.y += 0.35f;
                                    usr->particles.burstConfetti(p);
		                        }
		                    }
		                }

                if (coin.visualKind == CollectableVisualKind::Gem)
                {
                    usr->electroBall.addGemCharge();
                }

                // In School, coins are just targets for tests: don't spawn fly-to-HUD animations
                // (HUD target isn't visible anyway). Just mark as triggered so it doesn't retrigger.
                if (usr->gameMode == UserContext::GameMode::SCHOOL)
                {
                    usr->coinLane.markFlyTriggered(i);
                    usr->sound.playSfxCoinPickup();
                    continue;
                }

                glm::vec4 viewport(
                    0.0f,
                    0.0f,
                    static_cast<float>(ctx->screenWidth),
                    static_cast<float>(ctx->screenHeight)
                );
                glm::vec3 screenPos =
                    glm::project(coin.position, usr->cameraMat, usr->perspectiveMat, viewport);
                glm::vec2 hudTarget = coin.visualKind == CollectableVisualKind::Gem
                    ? (usr->placeOfCharge + glm::vec2(30.0f, 20.0f))
                    : (usr->placeOfMoney + glm::vec2(30.0f, 30.0f));

                if (usr->coinLane.spawnFlyAnimation(
                        glm::vec2(screenPos.x, screenPos.y),
                        hudTarget,
                        coin.visualKind
                    ))
                {
                    usr->coinLane.markFlyTriggered(i); // ✅ Mark via helper method
                    usr->sound.playSfxCoinPickup();
                }
            }
        }

        if (usr->coinLane.redistributeIfAllGemsCollected())
        {
            // Keep the pickup effect visible, but immediately deploy a fresh wave
            // once every gem in the current deployment has been consumed.
        }

        // Store for next frame after all coin logic consumed prev->cur.
        usr->lastBallPosition = ballModel[3];

		        // Lesson 3: when all coins for the level are collected, pause for 0.5s, then
                // smoothly return to start and advance to next level.
		        if (usr->gameMode == UserContext::GameMode::SCHOOL &&
		            usr->school.selectedLesson == 3 &&
		            usr->school.spinLevelJustCompleted)
		        {
                    if (!School_IsPaused(&usr->school) && !usr->school.returnToStartActive)
                    {
                        // Start smooth return-to-start after the pause.
                        usr->school.returnToStartActive = true;
                        usr->school.returnToStartT = 0.0f;
                        usr->school.returnToStartDtLoan = 0.0f;
                        usr->school.returnFromBallPos = glm::vec3(ballModel[3]);
                        usr->phase = UserContext::Phase::IDLE;

                        // Clear any leftover strike/spare banner from prior normal play.
                        usr->strikeSpareKind = 0;
                        usr->strikeSpareFlashTime = 0.0f;
                        usr->negativeBannerKind = 0;
                        usr->negativeBannerFlashTime = 0.0f;
                        usr->neutralBannerFlashTime = 0.0f;

                        // Also start camera smoothing back to IDLE target.
                        usr->cameraReturnActive = true;
                        usr->cameraReturnT = 0.0f;
                        usr->cameraReturnStartEye = usr->cameraEye;
                        usr->cameraReturnStartTarget = usr->cameraTarget;
                        glm::vec3 idleBallPos = Scene_IdleBallPos(usr->scene);
                        Scene_ComputeCameraEyeTarget(
                            usr->scene, idleBallPos, usr->cameraReturnEndEye, usr->cameraReturnEndTarget
                        );
                    }
		        }
        // 5. Add coin to bank if
        // usr->coinLane.getNewlyCollected =

        // Render 3D collectables in perspective view
        AssetMesh *coinCollectableMesh = &usr->starMesh;
        AssetMesh *gemCollectableMesh = gGemMeshReady ? &gGemMesh : &usr->starMesh;

        for (int i = 0; i < usr->coinLane.getActiveCount(); i++)
        {
            const Coin &coin = usr->coinLane.getCoins()[i];
            const bool useGemCollectable = coin.visualKind == CollectableVisualKind::Gem && gGemMeshReady;
            AssetMesh *collectableMesh = useGemCollectable ? gemCollectableMesh : coinCollectableMesh;
            const float collectableWorldScale = useGemCollectable ? 0.34f : 0.25f;

            // Skip rendering in 3D if this coin is currently flying to HUD as 2D sprite
            // Condition: collected + fly animation spawned + still in early implosion (visual
            // overlap window)
            if (coin.state == CoinState::Collected && coin.flyTriggered)
            {
                continue;
            }

            if (coin.isRenderable())
            {
                glm::mat4 model = glm::scale(coin.transform, glm::vec3(collectableWorldScale));
                if (useGemCollectable)
                    model = glm::rotate(model, 0.35f, glm::vec3(1.0f, 0.0f, 0.0f));
                usr->mainShader.renderRealMesh(*collectableMesh, model, usr->cameraMat, usr->perspectiveMat);
            }
        }
        renderFlyingCollectables(
            &usr->mainShader,
            coinCollectableMesh,
            gemCollectableMesh,
            &usr->everythingTexture,
            &usr->coinLane,
            (float)ctx->screenWidth,
            (float)ctx->screenHeight,
            true, // vary
            usr->hudAboveThis,
            3.0f
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

        if (usr->gameMode == UserContext::GameMode::TRACKER && usr->tracker.active)
        {
            usr->clayton.renderer.imageTextures[2] = usr->trackerOscilloscopeTex.colorTexture;
            usr->clayton.renderer.imageTextures[3] = usr->trackerDiagramTex.colorTexture;
            Tracker_UpdateOscilloscopeTexture(usr);
            usr->trackerDiagramRenderer.render(
                usr->trackerDiagramTex,
                Tracker_EditablePatch(&usr->tracker),
                ctx->screenWidth,
                ctx->screenHeight,
                ctx->pixelRatio
            );
        }
        else
        {
            usr->clayton.renderer.imageTextures[2] = usr->ballRenderTex2.colorTexture;
        }

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
            (usr->windowStack.count > 0 || usr->dialog.active || usr->appInactiveOverlayActive)
                ? (Clay_Color){255, 255, 255, 0}
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

                if (usr->gameMode != UserContext::GameMode::TRACKER)
                {
                CLAY(CLAY_ID("NotchArounds1"), CLAY_THEME_TOP_BAR)
                {
                    // std::cerr << "renameID: " << usr->renameButton.clayId.stringId.chars <<
                    // std::endl;
                }
                }
                uint16_t contentPadding = usr->gameMode == UserContext::GameMode::TRACKER ? 0 : portraitPadding;
                CLAY(
                    CLAY_ID("Content body1"),
                    {.layout = {
                         .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                         .padding =
                             {contentPadding, contentPadding, contentPadding, contentPadding},
                         .layoutDirection = CLAY_TOP_TO_BOTTOM,
                     }}
                )
                {

                    if (usr->gameMode != UserContext::GameMode::TRACKER)
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
                        if (usr->gameMode != UserContext::GameMode::TRACKER)
                        {
                            const float charge01 = usr->electroBall.getVisualCharge01();
                            const int chargePct = glm::clamp((int)std::lround(charge01 * 100.0f), 0, 100);
                            const bool charged = charge01 > 0.001f;
                            Clay_ElementDeclaration chargeHud = CLAY_THEME_BTN_HUD;
                            chargeHud.layout = {
                                .sizing = {CLAY_SIZING_FIXED(180), CLAY_SIZING_FIT()},
                                .padding = {8, 8, 8, 8},
                                .childGap = 6,
                                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                            };
                            chargeHud.backgroundColor = charged ? (Clay_Color){22, 26, 42, 220} : (Clay_Color){28, 28, 28, 190};
                            chargeHud.cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG};
                            chargeHud.border = {
                                .color = charged ? (Clay_Color){80, 205, 255, 180} : CLAY_COLOR_BORDER,
                                .width = CLAY_BORDER_ALL(1),
                            };

                            CLAY(usr->renameButton.clayId, chargeHud)
                            {
                                ClayArena *arena = &usr->clayton.clayArena;
                                Clay_TextElementConfig titleCfg = CLAY_THEME_TEXT_BUTTON;
                                titleCfg.fontSize = CLAY_FONT_SIZE_SM;
                                titleCfg.textColor = charged ? (Clay_Color){235, 248, 255, 255} : (Clay_Color){190, 190, 200, 220};
                                Clay_String chargeLabel = ClayArena_FormatString(
                                    arena,
                                    Txl_Get(usr->language, TXL_BALL_CHARGE_FMT),
                                    chargePct
                                );
                                CLAY_TEXT(chargeLabel, CLAY_TEXT_CONFIG(titleCfg));
                                CLAY(
                                    CLAY_ID("BallChargeOuter"),
                                    {
                                        .layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(12)}},
                                        .backgroundColor = {0, 0, 0, 120},
                                        .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
                                    }
                                )
                                {
                                    Clay_Color fill = charged ? (Clay_Color){86, 205, 255, 225} : (Clay_Color){86, 86, 96, 180};
                                    if (chargePct >= 100)
                                        fill = (Clay_Color){180, 245, 255, 240};
                                    CLAY(
                                        CLAY_ID("BallChargeInner"),
                                        {
                                            .layout = {.sizing = {CLAY_SIZING_PERCENT(charge01), CLAY_SIZING_GROW()}},
                                            .backgroundColor = fill,
                                            .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
                                        }
                                    )
                                    {
                                    }
                                }
                            }
                            if (usr->gameMode != UserContext::GameMode::SCHOOL)
                            {
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
                                    (void)snprintf(
                                        bankAmountBuf, sizeof(bankAmountBuf), "$ %d", usr->carousel.bank
                                    );
                                    Clay_String bankAmount = ClayArena_AllocString(arena, bankAmountBuf);
                                    CLAY_TEXT(bankAmount, CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_BUTTON));
                                }
                            }
                        }
                    }
                    }

                    // Scoreboard / tracker / school panel
                    if (usr->gameMode == UserContext::GameMode::TRACKER)
                    {
                        Tracker_BuildHud(&usr->tracker, &usr->clayton);
                    }
                    else if (usr->gameMode != UserContext::GameMode::SCHOOL)
                    {
                        if (usr->gameMode == UserContext::GameMode::BOT && IsEnemyTurn(usr))
                        {
                            const char *opponentName =
                                (usr->playerRoute == PlayerRoute::CAMPAIGN)
                                    ? Campaign_OpponentDisplayName(Campaign_CurrentLevel(usr).opponent)
                                    : BotAvatar_DisplayName(usr->botAvatar);
                            ClayArena *arena = &usr->clayton.clayArena;
                            Clay_String turnLabel = ClayArena_FormatString(arena, "%s TURN", opponentName);
                            CLAY(
                                CLAY_ID("EnemyTurnBanner"),
                                {
                                    .layout = {
                                        .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                                        .padding = {6, 6, 6, 6},
                                        .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
                                    },
                                }
                            )
                            {
                                CLAY_TEXT(turnLabel, CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_BUTTON));
                            }
                        }
                        if (usr->gameMode == UserContext::GameMode::BOT)
                        {
                            // BOT mode: show both scoreboards (You + Angel), highlight active turn.
                            const bool enemyTurn = IsEnemyTurn(usr);
                            const char *opponentNameSrc =
                                (usr->playerRoute == PlayerRoute::CAMPAIGN)
                                    ? Campaign_OpponentDisplayName(Campaign_CurrentLevel(usr).opponent)
                                    : BotAvatar_DisplayName(usr->botAvatar);
                            char angelName[20] = {};
                            snprintf(angelName, sizeof(angelName), "%s", opponentNameSrc);
                            int32_t angelLen = (int32_t)strlen(angelName);
                            usr->clayton.constructClayScoreboardStyled(
                                &usr->board,
                                scoreBoardWidth,
                                usr->username,
                                &usr->username_len,
                                /*isActiveTurn=*/!enemyTurn,
                                /*boardKey=*/0
                            );
                            usr->clayton.constructClayScoreboardStyled(
                                &usr->enemyBoard,
                                scoreBoardWidth,
                                angelName,
                                &angelLen,
                                /*isActiveTurn=*/enemyTurn,
                                /*boardKey=*/1
                            );
                        }
                        else
                        {
                            // SOLO mode: only the player scoreboard.
                            usr->clayton.constructClayScoreboardStyled(
                                &usr->board,
                                scoreBoardWidth,
                                usr->username,
                                &usr->username_len,
                                /*isActiveTurn=*/true,
                                /*boardKey=*/0
                            );
                        }

                    }
                    else
                    {
                        // School mode panel (no scoring; replaces HUD action bar).
                        School_ClayBuildPanel(&usr->school, &usr->clayton, portraitPadding);
                    }

                        if (usr->gameMode == UserContext::GameMode::SCHOOL)
                        {
                            const bool showAimIndicator =
                                (usr->phase == UserContext::Phase::AIM) || (usr->phase == UserContext::Phase::SWING);
                            float oilRemain01 = 0.0f;
                            int oilReoilCount = 0;
                            int oilReoilNeeded = 0;
                            bool oilCanReoilNow = false;
                            if (usr->gameMode == UserContext::GameMode::SCHOOL && usr->school.selectedLesson == 4)
                            {
                                const SchoolOilLessonDefaults &d = School_OilDefaults();
                                const float denom = glm::max(1e-6f, d.laneOilThickness);
                                oilRemain01 = glm::clamp(usr->laneOilThickness / denom, 0.0f, 1.0f);
                                oilReoilCount = usr->school.spinSafeCoins;
                                oilReoilNeeded = 3;
                                oilCanReoilNow = School_OilLessonCanReoil(usr);
                            }
                            School_ClayBuildHud(
                                &usr->school,
                                &usr->clayton,
                                usr->enjoy.ndc.x,
                                showAimIndicator,
                                oilRemain01,
                                oilReoilCount,
                                oilReoilNeeded,
                                oilCanReoilNow
                            );
                        }

	                    if (usr->gameMode != UserContext::GameMode::TRACKER &&
                            usr->gameMode != UserContext::GameMode::SCHOOL)
	                    {
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
                                         .layoutDirection = CLAY_LEFT_TO_RIGHT,
	                                 }}
                                )
                            {

	                        CLAY(usr->menuButton.clayId, CLAY_THEME_BTN_HUD)
	                        {
	                            CLAY_TEXT(
	                                usr->clayton.txl(TXL_MENU), CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_BUTTON)
	                            );
	                        }
                /*
        CLAY(CLAY_ID("NotchArounds2"), CLAY_THEME_TOP_BAR)
                */

                // SOUND button next to MENU
	                    CLAY(usr->soundButton.clayId, CLAY_THEME_BTN_HUD)
	                    {
	                        CLAY_TEXT(usr->clayton.txl(TXL_SOUND), CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_BUTTON));
	                    }

                    CLAY(usr->oilButton.clayId, CLAY_THEME_BTN_HUD)
                    {
                        CLAY_TEXT(usr->clayton.txl(TXL_OIL), CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_BUTTON));
                    }

                    if (usr->playerRoute == PlayerRoute::FREESTYLE)
                    {
                        CLAY(usr->hiScoreButton.clayId, CLAY_THEME_BTN_HUD)
                        {
                            CLAY_TEXT(usr->clayton.txl(TXL_HI_SCORE), CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_BUTTON));
                        }
                    }

                CLAY(usr->openShopClick.clayId, CLAY_THEME_BTN_HUD)
                {
                    CLAY_TEXT(usr->clayton.txl(TXL_SHOP), CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_BUTTON));
                }
                        };

                        {
                            ClayArena *arena = &usr->clayton.clayArena;
                            Clay_String levelTitle = {};
                            Clay_String levelSubtitle = {};
                            if (usr->playerRoute == PlayerRoute::CAMPAIGN)
                            {
                                const CampaignLevelConfig &cfg = Campaign_CurrentLevel(usr);
                                levelTitle = usr->clayton.txl(Campaign_TitleKey(cfg.levelNumber));
                                levelSubtitle = usr->clayton.txl(Campaign_SubtitleKey(cfg.levelNumber));
                            }
                            else if (usr->playerRoute == PlayerRoute::PRACTICE)
                            {
                                levelTitle = usr->clayton.txl(TXL_PRACTICE_TITLE);
                                levelSubtitle = usr->clayton.txl(TXL_PRACTICE_SUBTITLE);
                            }
                            else
                            {
                                levelTitle = usr->clayton.txl(TXL_FREESTYLE_TITLE);
                                levelSubtitle = usr->clayton.txl(TXL_FREESTYLE_SUBTITLE);
                            }
                            Clay_TextElementConfig levelTitleCfg = CLAY_THEME_TEXT_BUTTON;
                            levelTitleCfg.fontSize = CLAY_FONT_SIZE_SM;
                            levelTitleCfg.textColor = (Clay_Color){230, 236, 248, 255};
                            Clay_TextElementConfig levelSubtitleCfg = CLAY_THEME_TEXT_BUTTON;
                            levelSubtitleCfg.fontSize = CLAY_FONT_SIZE_SM - 2;
                            levelSubtitleCfg.textColor = (Clay_Color){175, 186, 205, 230};

                            CLAY(
                                CLAY_ID("CampaignLevelTitle"),
                                {
                                    .layout = {
                                        .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                                        .padding = {4, 2, 4, 8},
                                        .childGap = 2,
                                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                                        .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
                                    },
                                }
                            )
                            {
                                CLAY_TEXT(levelTitle, CLAY_TEXT_CONFIG(levelTitleCfg));
                                CLAY_TEXT(levelSubtitle, CLAY_TEXT_CONFIG(levelSubtitleCfg));
                            }
                        }
                    }

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
	                            {.element = CLAY_ATTACH_POINT_CENTER_CENTER,
	                             .parent = CLAY_ATTACH_POINT_CENTER_TOP},
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

        // Overlay banners — constrained to the 9:16 portrait area.
        // Negative banners take priority over strike/spare when active.
        bool showNegative = usr->negativeBannerFlashTime > 0.0f && (usr->negativeBannerKind == 1 || usr->negativeBannerKind == 2);
        bool showPositive = usr->strikeSpareFlashTime > 0.0f && (usr->strikeSpareKind == 1 || usr->strikeSpareKind == 2);

        // Neutral banner (e.g. "<N> PINS") disabled for now — keeping the code around for later reuse.
        // bool showNeutral = usr->neutralBannerFlashTime > 0.0f && usr->neutralBannerPins > 0;
        bool showNeutral = false;

        if (showNegative || showPositive || showNeutral)
        {
            const float duration = 1.25f;
            float pulse = 0.5f + 0.5f * sinf(usr->globalTime * 12.0f);

            float textA = glm::clamp(120.0f + 135.0f * pulse, 0.0f, 255.0f);
            float bgA = glm::clamp(70.0f + 90.0f * pulse, 0.0f, 200.0f);
            float outlineA = glm::clamp(80.0f + 80.0f * pulse, 0.0f, 255.0f);

            const char *label = nullptr;
            Clay_Color bg = {0.0f, 0.0f, 0.0f, bgA};
            Clay_Color outline = {255.0f, 255.0f, 255.0f, outlineA};
            Clay_Color text = {255.0f, 200.0f + 55.0f * pulse, 0.0f, textA};
            if (showNegative)
            {
                label = (usr->negativeBannerKind == 1) ? "MISSED" : "STALLED";
                bg = {140.0f, 0.0f, 0.0f, bgA};
                outline = {255.0f, 80.0f, 80.0f, outlineA};
                text = {255.0f, 255.0f, 255.0f, textA};
            }
            else if (showPositive)
            {
                label = (usr->strikeSpareKind == 1) ? "STRIKE" : "SPARE";
            }
            // else
            // {
            //     // Neutral roll banner: "<N> PINS"
            //     char pinsBuf[32];
            //     snprintf(pinsBuf, sizeof(pinsBuf), "%d PINS", usr->neutralBannerPins);
            //     ClayArena *tmpArena = &usr->clayton.clayArena;
            //     Clay_String tmp = ClayArena_AllocString(tmpArena, pinsBuf);
            //     label = tmp.chars; // stable for this frame
            //     bg = {20.0f, 30.0f, 70.0f, bgA};
            //     outline = {140.0f, 170.0f, 255.0f, outlineA};
            //     text = {255.0f, 255.0f, 255.0f, textA};
            // }

            // Slightly above center inside the portrait box.
            Clay_Vector2 overlayOffset = {0, -portraitHeight * 0.08f};
            CLAY(
                CLAY_ID("StrikeSpareOverlay"),
                {
                    .layout = {
                        .sizing = {CLAY_SIZING_PERCENT(0.78f), CLAY_SIZING_FIT()},
                        .padding = {18, 26, 18, 26},
                        .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
	                    },
                    .backgroundColor = bg,
                    .cornerRadius = {CLAY_RADIUS_XL, CLAY_RADIUS_XL, CLAY_RADIUS_XL, CLAY_RADIUS_XL},
                    .floating = {
                        .offset = overlayOffset,
                        .zIndex = 50,
                        .attachPoints = {.element = CLAY_ATTACH_POINT_CENTER_CENTER,
                                         .parent = CLAY_ATTACH_POINT_CENTER_CENTER},
                        .attachTo = CLAY_ATTACH_TO_PARENT,
                    },
	                    .border = {.color = outline, .width = CLAY_BORDER_ALL(2)},
                }
            )
            {
                Clay_TextElementConfig txtCfg = {
                    .textColor = text,
                    .fontId = CLAY_FONT_NOTO,
                    .fontSize = 54,
                };
                ClayArena *bannerArena = &usr->clayton.clayArena;
                Clay_String bannerStr = ClayArena_AllocString(bannerArena, label);
                CLAY_TEXT(
                    bannerStr,
                    CLAY_TEXT_CONFIG(txtCfg)
                );
            }

            (void)duration;
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

    if (usr->appInactiveOverlayActive)
    {
        CLAY(
            CLAY_ID("AppInactiveOverlay"),
            {
                .layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()}},
                .backgroundColor = {20, 10, 40, 150},
                .floating = {
                    .offset = {0},
                    .zIndex = 96,
                    .attachPoints = {CLAY_ATTACH_POINT_CENTER_CENTER, CLAY_ATTACH_POINT_CENTER_CENTER},
                    .attachTo = CLAY_ATTACH_TO_PARENT,
                },
            }
        )
        {
        }
    }

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
        oilStatus.reoilCost = 10.0f;

        // School Lesson 4: use lesson oil defaults as the "house" baseline, and make re-oil free.
        if (usr->gameMode == UserContext::GameMode::SCHOOL && usr->school.selectedLesson == 4)
        {
            const SchoolOilLessonDefaults &d = School_OilDefaults();
            oilStatus.laneFriction = d.laneFriction;
            oilStatus.lanePushbackStrength = d.lanePushbackStrength;
            oilStatus.houseOilThickness = d.laneOilThickness;
            oilStatus.leftOilFadeStartM = d.leftOilFadeStartM;
            oilStatus.leftOilFadeEndM = d.leftOilFadeEndM;
            oilStatus.rightOilFadeStartM = d.rightOilFadeStartM;
            oilStatus.rightOilFadeEndM = d.rightOilFadeEndM;
            oilStatus.oilCarrydownPerBallTravelM = d.oilCarrydownPerBallTravelM;
            oilStatus.oilThicknessDecayPerBallTravel = d.oilThicknessDecayPerBallTravel;
            oilStatus.reoilCost = 0.0f;

            oilStatus.reoilEnabled = School_OilLessonCanReoil(usr);
            oilStatus.reoilDisabledLabel =
                oilStatus.reoilEnabled ? nullptr : Txl_Get(usr->language, TXL_WEAR_IT_DOWN);
            oilStatus.lessonReoilCount = usr->school.spinSafeCoins;
            oilStatus.lessonReoilNeeded = 3;
        }

        usr->clayton.botsActionLabel = (usr->selectorFlowStep == SelectorFlowStep::BOT) ? Txl_Get(usr->language, TXL_SELECT_ANGEL) : Txl_Get(usr->language, TXL_SELECT_BOT);
        usr->clayton.housesActionLabel = (usr->selectorFlowStep == SelectorFlowStep::HOUSE) ? Txl_Get(usr->language, TXL_SELECT_HOUSE) : Txl_Get(usr->language, TXL_SWITCH_HOUSE);
        usr->clayton.shopActionLabel = (usr->selectorFlowStep == SelectorFlowStep::BALL) ? Txl_Get(usr->language, TXL_SELECT_BALL) : Txl_Get(usr->language, TXL_BUY_NOW);
        usr->clayton.botsActionEnabled = true;
        usr->clayton.housesActionEnabled = true;
        usr->clayton.shopActionEnabled = true;
        if (usr->selectorFlowStep == SelectorFlowStep::BOT)
        {
            const int idx = usr->botsCarousel.closestBotIdx;
            if (idx >= 0 && idx < usr->botsCarousel.cardCount)
            {
                CampaignOpponent opp = CampaignOpponent::MALACH;
                switch (usr->botsCarousel.items[idx].kind)
                {
                    case BotCatalogAvatar_CHERUB: opp = CampaignOpponent::DOG; break;
                    case BotCatalogAvatar_SERAPH: opp = CampaignOpponent::BEAK; break;
                    case BotCatalogAvatar_THRONE: opp = CampaignOpponent::COW; break;
                    default: opp = CampaignOpponent::MALACH; break;
                }
                usr->clayton.botsActionEnabled = UnlockMask_HasOpponent(usr, opp);
            }
        }
        if (usr->selectorFlowStep == SelectorFlowStep::HOUSE)
        {
            const int idx = usr->housesCarousel.closestHouseIdx;
            if (idx >= 0 && idx < usr->housesCarousel.cardCount)
                usr->clayton.housesActionEnabled = UnlockMask_HasHouse(usr, usr->housesCarousel.items[idx].id);
        }
        if (usr->selectorFlowStep == SelectorFlowStep::BALL)
        {
            const int idx = usr->carousel.closestBallIdx;
            if (idx >= 0 && idx < usr->carousel.cardCount)
                usr->clayton.shopActionEnabled = UnlockMask_HasBall(usr, usr->carousel.items[idx].id);
        }

	    usr->windowStack.renderWindowStack(
	        &usr->clayton,
	        &usr->keypad,
	        &usr->sound.settings,
	        &usr->adaptiveAudio,
	        &usr->localHi,
	        &usr->carousel,
            &usr->housesCarousel,
            &usr->botsCarousel,
            &usr->tracker,
            &usr->school.massSlider,
            &usr->settings,
	        usr->shouldShowShop,
            !usr->sound.useWavPlayback && !usr->sound.audioDisabled,
            &oilStatus,
            (float)deltaTime
	    );

        if (usr->gameMode == UserContext::GameMode::TRACKER && usr->tracker.active)
        {
            Tracker_BuildOscilloscopeOverlay(&usr->tracker, &usr->clayton);
        }

        // Story dialog renders only when no other modal windows are present.
        // Typing animation advances only when actually rendered.
        if (usr->windowStack.count == 0 && usr->dialog.active)
        {
            // Dialog is modal. When it shows clickable options, force normal mouse mode so
            // desktop users can actually click them (relative mouse capture breaks this).
            if (usr->dialog.waitingChoice)
            {
                SDL_SetRelativeMouseMode(SDL_FALSE);
            }

            usr->dialog.update((float)deltaTime);
            // Per-character typewriter SFX (non-whitespace only).
            // Clamp per frame to avoid audio overload on slow frames.
            {
                int ticks = usr->dialog.consumeTypedNonWhitespaceCount();
                // Cap: no more than ~15 ticks/sec.
                static float typeTickCooldown = 0.0f;
                typeTickCooldown = glm::max(0.0f, typeTickCooldown - (float)deltaTime);

                if (ticks > 0 && typeTickCooldown <= 0.0f)
                {
                    usr->sound.playSfxTypewriter();
                    // If we typed a lot in one frame (hitch), add a second tick with longer cooldown.
                    typeTickCooldown = (ticks >= 6) ? 0.12f : 0.07f;
                }
            }
            usr->dialog.render(&usr->clayton);
            // Debug (hot-reload friendly): prove typing is producing non-whitespace chars.
            // Uncomment if needed:
            // std::cerr << "[dialog] typedNonWs=" << usr->dialog.peekTypedNonWhitespaceCount() << "\n";
            if (usr->dialog.closeRequested)
            {
                usr->dialog.finalizeClose();
            }
        }

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
if (usr->gameMode != UserContext::GameMode::SCHOOL)
{
    Clay_ElementId menuAndShopRow = CLAY_ID("MenuAndShopRow");
    Clay_BoundingBox hudBottom = Clay_GetElementData(menuAndShopRow).boundingBox;
    usr->hudAboveThis = ctx->screenHeight - (hudBottom.y + hudBottom.height);
    Clay_ElementId id = CLAY_ID("PlaceOfMoney");
    Clay_BoundingBox box = Clay_GetElementData(id).boundingBox;
    usr->placeOfMoney = glm::vec2(
        box.x + (box.width - CoinFlyConfig::PIXEL_SIZE) * 0.125f,
        ctx->screenHeight - (box.height * 0.5f + box.y) - 20.0f
    );
    Clay_BoundingBox chargeBox = Clay_GetElementData(usr->renameButton.clayId).boundingBox;
    usr->placeOfCharge = glm::vec2(
        chargeBox.x + chargeBox.width * 0.5f - CoinFlyConfig::PIXEL_SIZE * 0.5f,
        ctx->screenHeight - (chargeBox.y + chargeBox.height * 0.5f) - CoinFlyConfig::PIXEL_SIZE * 0.25f
    );
}
// === PASS 3: Flying Collectables (Ortho Overlay) ===

AssetMesh *coinCollectableMeshForHUD = &usr->starMesh;
AssetMesh *gemCollectableMeshForHUD = gGemMeshReady ? &gGemMesh : &usr->starMesh;

glUseProgram(usr->mainShader.id);
renderFlyingCollectables(
    &usr->mainShader,
    coinCollectableMeshForHUD,
    gemCollectableMeshForHUD,
    &usr->everythingTexture,
    &usr->coinLane,
    (float)ctx->screenWidth,
    (float)ctx->screenHeight,
    false, // vary
    usr->hudAboveThis,
    3.0f
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
        // Restitution (bounciness)
        {
            bool changed =
                ImGui::SliderFloat("Restitution", &usr->imguiBall.restitution, 0.0f, 0.6f, "%.3f");
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
			            ImGui::SliderFloat("Lane Restitution", &usr->laneRestitution, 0.0f, 0.5f, "%.3f");
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
		        ImGui::Text("Jolt Materials (Live)");
		        ImGui::Separator();
		        {
		            ImGui::SliderFloat("Pins Restitution", &usr->pinRestitution, 0.0f, 0.8f, "%.3f");
		            ImGui::SliderFloat("Pins Friction", &usr->pinFriction, 0.0f, 1.0f, "%.3f");
		            ImGui::SliderFloat("Pins Mass (kg)", &usr->pinMass, 0.2f, 3.0f, "%.2f");
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
	                usr->laneRestitution = 0.01f;
	                usr->pinRestitution = 0.3f;
	                usr->pinFriction = 0.3f;
	                usr->pinMass = 1.53f;
	                usr->ballRestitution = glm::clamp(g_ballCatalog[usr->myBall.id].restitution, 0.0f, 1.0f);
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
            ImGui::Text("armImpulseAtThrow (kg*m/s)");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.2f", usr->armImpulseAtThrow);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("lightnessBuff");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.2f", usr->lightnessBuff);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("launchBuffEffective");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.2f", usr->launchBuffEffective);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("ballRestitution");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.3f", usr->ballRestitution);

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
