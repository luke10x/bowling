#include <chrono>
#include <iostream>
#include <random>
#include <stdint.h>
#include <stdio.h>
#include <string.h> // for memcpy, strcmp
#include <thread>

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
    glm::vec3 pivotPoint;
    glm::vec3 joystick;
    Joystick enjoy;
    glm::vec3 desiredBall;
    glm::vec3 carriedBall;
    glm::vec3 undesiredMovement = glm::vec3(0.0f);
    glm::vec3 swingMovement = glm::vec3(0.0f);
    glm::vec3 swingPreviousFramePoint = glm::vec3(0.0f);
    float swingStallTime = 0.0f;
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
    Clayton_Click replayButton;
    Clayton_Click renameButton;
    Clayton_Click menuButton;
    Clayton_Click soundButton;
    Clayton_Click hiScoreButton;

    // TUNABLET entries
    float speedBoostAtThrow = 2.0f;
    float angularFactor = 0.15f;
    float smashingPower = 10.0f;
    float desiredMass = 7.25f;
    float ballBaseFriction;
    float ballSkidFactor;
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
    static constexpr float CATALOG_BITE_MIN = 0.46f;
    static constexpr float CATALOG_BITE_MAX = 0.63f;
    static constexpr float CATALOG_BUFF_MIN = 0.0f;
    static constexpr float CATALOG_BUFF_MAX = 1.0f;

    // Target physics/gameplay ranges (from your ImGui sliders)
    static constexpr float PHYSICS_MASS_MIN = 2.5f;
    static constexpr float PHYSICS_MASS_MAX = 8.0f;
    static constexpr float PHYSICS_SPIN_MIN = 0.1f;
    static constexpr float PHYSICS_SPIN_MAX = 1.0f;
    static constexpr float PHYSICS_SMASH_MIN = 5.0f;
    static constexpr float PHYSICS_SMASH_MAX = 50.0f;
    static constexpr float PHYSICS_SPEEDBOOST_MIN = 0.5f;
    static constexpr float PHYSICS_SPEEDBOOST_MAX = 5.0f;
    static constexpr float PHYSICS_FRICTION_MIN = 0.0f;
    static constexpr float PHYSICS_FRICTION_MAX = 0.15f;

    // Tunable multipliers for fine-tuning feel
    static constexpr float SPIN_MULTIPLIER = 1.0f;
    static constexpr float BITE_TO_FRICTION_SCALE = 1.0f;
    static constexpr float SKID_TO_ANGULAR_SCALE = 0.8f;
};
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

    // Spin factor: exponential for more dramatic high-end feel
    usr->angularFactor = remapExponential(
                             ball.spin,
                             BallPhysicsMapping::CATALOG_SPIN_MIN,
                             BallPhysicsMapping::CATALOG_SPIN_MAX,
                             BallPhysicsMapping::PHYSICS_SPIN_MIN,
                             BallPhysicsMapping::PHYSICS_SPIN_MAX,
                             1.5f // Slight curve
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

    // Precompute friction curve params from 'bite' and 'skid'
    usr->ballBaseFriction = remapExponential(
                                ball.bite,
                                BallPhysicsMapping::CATALOG_BITE_MIN,
                                BallPhysicsMapping::CATALOG_BITE_MAX,
                                0.05f, // Minimum lane friction
                                0.12f, // Maximum base friction
                                2.0f   // Exponential for sharper bite difference
                            ) *
        BallPhysicsMapping::BITE_TO_FRICTION_SCALE;

    usr->ballSkidFactor = remapClamped(
                              ball.skid,
                              BallPhysicsMapping::CATALOG_SKID_MIN,
                              BallPhysicsMapping::CATALOG_SKID_MAX,
                              0.3f, // Low skid = more grip early
                              0.9f  // High skid = slides longer
                          ) *
        BallPhysicsMapping::SKID_TO_ANGULAR_SCALE;

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
    // === Lane Friction Progression ===
    {
        float z = ballModel[3].z;
        constexpr float zStart = -18.3f;
        constexpr float zEnd = -5.0f;

        // Normalize position along lane
        float t = (z - zStart) / (zEnd - zStart);
        t = glm::clamp(t, 0.0f, 1.0f);

        // Apply skid factor: high skid = slower friction ramp-up
        float frictionProgress = powf(t, usr->ballSkidFactor);

        // Combine base friction (from bite) with progression
        float currentFriction = usr->ballBaseFriction * frictionProgress;
        currentFriction =
            glm::clamp(currentFriction, 0.0f, BallPhysicsMapping::PHYSICS_FRICTION_MAX);

        usr->phy.apply_friction_to_lane(currentFriction);
    }

    // === Spin & Angular Velocity (Only during active throw) ===
    if (usr->phase == UserContext::Phase::THROW)
    {
        if (glm::abs(usr->circles) >= 1)
        {

            float sideDrive =
                -usr->smoothedAngularVelocity * (usr->myBall.spin * usr->myBall.spin) * 0.25f;
            usr->phy.apply_angular_velocity_on_ball(sideDrive);

            float spinContributionToSmash = sideDrive * usr->myBall.hitBuff;
            usr->phy.set_spin_speed(spinContributionToSmash);
        }
    }
    else
    {
        usr->sectors = -1;
    }
}

void vtx::init(vtx::VertexContext *ctx)
{

    ctx->usrptr = new UserContext;
    UserContext *usr = static_cast<UserContext *>(ctx->usrptr);

    usr->ballRenderTex.renderTextureInit();
    usr->ballRenderTex2.renderTextureInit();

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

    usr->clayton.initClayton(ctx->screenWidth, ctx->screenHeight);
    usr->clayton.renderer.imageTextures[1] = usr->ballRenderTex.colorTexture;
    usr->clayton.renderer.imageTextures[2] = usr->ballRenderTex2.colorTexture;

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
    initClaytonClick(&usr->replayButton, "ReplayButton");
    initClaytonClick(&usr->renameButton, "PlaceOfRenameName");
    initClaytonClick(&usr->menuButton, "MenuButton");
    initClaytonClick(&usr->soundButton, "SoundButton");
    initClaytonClick(&usr->hiScoreButton, "HiScoreButton");
    initClaytonClick(&usr->openShopClick, "openShopButton");
    initClaytonClick(&usr->clayton.closeShopClick, "closeShopButton");
    initClaytonClick(&usr->clayton.buyClick, "BuyButtdd");

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

        if (isClaytonClicked(&usr->replayButton, e))
        {
            usr->phase = UserContext::Phase::IDLE;
            usr->enjoy.resetJoystick();
            usr->aimFlatPos = glm::vec2(0.5f, 0.5f);
            usr->aimDownFlatPos = usr->aimFlatPos;
            std::cerr << textScoreboard(usr->board) << std::endl;
            resetScoreboard(&usr->board);
            continue;
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

                    float factor = glm::clamp(deltaTime * smoothingSpeed, 0.0f, 1.0f);

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
        bool muchFwd = ballPos.z > usr->pivotPoint.z + 0.9f;
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

            if (usr->swingingTime > 0.50f && usr->swingStallTime > 0.35f)
            {
                std::cerr << "Dead swing cancel -> IDLE" << std::endl;
                usr->bufferedRequestThrow = false;
                usr->phy.set_ball_free();

                // Must match IDLE_BALL_POS (declared later).
                const glm::vec3 IDLE_BALL_POS = glm::vec3(0.0f, 0.2f, -18.0f);
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
        bool safeToRelease = ballPos.z > usr->pivotPoint.z;
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

                usr->pivotPoint = glm::vec3(0.0f, 1.2f, -18.3f);
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
                usr->phy.set_ball_free();

                SDL_SetRelativeMouseMode(SDL_FALSE);
                usr->throwingTime = 0.0f;
                usr->settlingTime = 0.0f;
                // events
                usr->sound.playSfxBallHitLane();
            }
        }
    }

    glm::vec3 IDLE_BALL_POS = glm::vec3(0.0f, 0.2f, -18.0f);

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

            glm::quat ySpin = glm::angleAxis(usr->totalSpinAngle, glm::vec3(0.0f, 1.0f, 0));

            usr->aimingTime += deltaTime;

            float pullX = usr->enjoy.ndc.x;
            float pullZ = usr->enjoy.ndc.y;
            // Adds hung
            float pullY = -sqrtf(1.01f * ropeLength * ropeLength - pullX * pullX - pullZ * pullZ);

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

            ballModel = glm::translate(glm::mat4(1.0f), usr->carriedBall) * glm::mat4_cast(ySpin);

            usr->phy.set_manual_ball_position(usr->carriedBall, ySpin, deltaTime * 1.0f);
        }
        // usr->phy.enable_physics_on_ball();

        if (usr->phase == UserContext::Phase::SWING)
        {
            usr->swingingTime += deltaTime;

            //  std::cerr << "SPIN2 " << spin << std::endl
            // usr->phy.apply_angular_velocity_on_ball(spin);

            // Only first time swinging will enable this
            ballModel = usr->phy.physics_get_ball_matrix();
            glm::vec3 before = usr->carriedBall;
            usr->carriedBall = ballModel[3]; //
            glm::vec3 after = usr->carriedBall;

            glm::vec3 ballPos = ballModel[3];
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
                }
                else
                {
                    usr->phase = UserContext::Phase::IDLE;
                    usr->enjoy.resetJoystick();
                    usr->aimFlatPos = glm::vec2(0.5f, 0.5f);
                    usr->aimDownFlatPos = usr->aimFlatPos;
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

    usr->cameraMat = glm::lookAt(
        glm::vec3(
            0.0f,
            0.8f,
            glm::clamp(
                ballModel[3].z - 3.0f,
                -21.0f,
                -2.0f
            )
        ), // eye in before of the ball
        glm::vec3(
            0.0f,
            -1.0f,
            glm::clamp(
                ballModel[3].z + 4.5f,
                -12.0f,
                2.0f
            )
        ),                          // target after
        glm::vec3(0.0f, 1.0f, 0.0f) // up
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
                if (usr->phase == UserContext::Phase::RESULT)
                {
                    CLAY(usr->replayButton.clayId, CLAY_THEME_BTN_SUCCESS)
                    {
                        CLAY_TEXT(CLAY_STRING("PLAY"), CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_BUTTON));
                    }
                }
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
	    usr->windowStack.renderWindowStack(
	        &usr->clayton,
	        &usr->keypad,
	        &usr->sound.settings,
	        &usr->adaptiveAudio,
	        &usr->localHi,
	        &usr->carousel,
	        usr->shouldShowShop
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
        ImGui::Text("Derived Physics Values (Live)");
        ImGui::Separator();

        if (ImGui::BeginTable("##physics_vals", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
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
            ImGui::Text("ballSkidFactor");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.2f", usr->ballSkidFactor);

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
            ImGui::Text("currentFriction @ mid-lane");
            {
                constexpr float zStart = -18.3f, zEnd = -5.0f;
                float zMid = (zStart + zEnd) * 0.5f;
                float t = glm::clamp((zMid - zStart) / (zEnd - zStart), 0.0f, 1.0f);
                float progress = powf(t, usr->ballSkidFactor);
                float friction = glm::clamp(
                    usr->ballBaseFriction * progress, 0.0f, BallPhysicsMapping::PHYSICS_FRICTION_MAX
                );
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%.3f", friction);
            }

            ImGui::EndTable();
        }

        if (ImGui::CollapsingHeader("📐 Formula Reference"))
        {
            ImGui::BulletText("angularStrength = angularFactor × smashingPower × 0.02f");
            ImGui::BulletText("spinStrength    = angularFactor × smashingPower × 0.008f");
            ImGui::BulletText("currentFriction = clamp(bite × skid^t, 0, 0.15)");
            ImGui::BulletText("  where t = clamp((z+18.3)/13.3, 0, 1)");
        }

        ImGui::End();
    }
    usr->imgui.endImgui();
}

usr->fpsCounter.endFrame();

SDL_GL_SwapWindow(ctx->sdlWindow);
}
