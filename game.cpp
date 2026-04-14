#include <chrono>
#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <thread>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "framework/boot.h"

#include "all_assets.h"
#include "aurora.h"
#include "circlegest.h"
#include "clayton/clayarena.h"
#include "clayton/clayton.h"
#include "clayton/clayton_click.h"
#include "clayton/keypad.h"
#include "clayton/claytheme.h"
#include "decal.h"
#include "fpscounter.h"
#include "hooker.h"
#include "joystick.h"
#include "localhi.h"
#include "mesh.h"
#include "mod_imgui.h"
#include "physics/physics.h"
#include "score.h"
#include "sounds/sounds.h"
#include "sounds/adaptive_audio.h"
#include "storage.h"
#include "stubs.h"
#include "tween.h"
#include "transition.h"
#include "tritest.h"
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

    AssetMesh ballMesh;
    AssetMesh laneMesh;
    AssetMesh pinMesh;

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
    float swingingTime;
    float highestPoint;

    BowlingScoreboard board;
    int wereDead;
    Clayton clayton;
    bool shouldShowClayDebug;
    bool shouldShowImgui;

    Transition trans;
    Circle circle;
    bool bufferedRequestThrow = false;
    float deltaTimeLoan = 0.0f;

    DecalBatch decalBatch;

    char username[20];
    int32_t username_len;
    Keypad keypad;
    Clayton_Click replayButton;
    Clayton_Click renameButton;
    Clayton_Click menuButton;
    Clayton_Click soundButton;
    Clayton_Click hiScoreButton;

    bool shouldShowHiScore = false;
    bool shouldShowHiScoreWithLatest = false;

    // TUNABLET entries
    float speedBoostAtThrow = 2.0f;
    float angularFactor = 0.15f;
    float smashingPower = 10.0f;
    float desiredMass = 7.25f;
    bool isMouseDownInThrow;

    MiniTriangle tri;
    Storage storage;

    GameSoundSystem sound;
    AdaptiveAudioSystem adaptiveAudio;
    LocalHighscore localHi;

    // Click handlers
    Clayton_Click musicVolClicks[5];    // 5 volume buttons for music
    Clayton_Click sfxVolClicks[5];      // 5 volume buttons for SFX
    Clayton_Click qualityClicks[3];     // 3 quality buttons
    Clayton_Click prevSongClick;
    Clayton_Click nextSongClick;
    Clayton_Click closeClick;
    Clayton_Click hiScoreCloseClick;

    // For adaptive audion controls
    Clayton_Click useSynthClick;
    Clayton_Click useWavClick;
    Clayton_Click disableAudioClick;

    int numberOfBallsHit;

    ClayArena clayArena;

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
    usr->decalBatch.loadDecalBatchShader();

    setupStubScoreboardMax(&usr->board);
}

void vtx::init(vtx::VertexContext *ctx)
{
    ctx->usrptr = new UserContext;
    UserContext *usr = static_cast<UserContext *>(ctx->usrptr);

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
    usr->everythingTexture.loadTextureFromFile(ASSET_PATH "everything_tex.png");
    MeshData ballMd = loadMeshFromBlob(ball_mesh_data, ball_mesh_data_len);
    usr->ballMesh.sendMeshDataToGpu(&ballMd);
    MeshData laneMd = loadMeshFromBlob(lane_mesh_data, lane_mesh_data_len);
    usr->laneMesh.sendMeshDataToGpu(&laneMd);
    MeshData pinMd = loadMeshFromBlob(pin_mesh_data, pin_mesh_data_len);
    usr->pinMesh.sendMeshDataToGpu(&pinMd);

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
    initClaytonClick(&usr->renameButton, "PlaceOfName");
    initClaytonClick(&usr->menuButton, "MenuButton");
    initClaytonClick(&usr->soundButton, "SoundButton");
    initClaytonClick(&usr->hiScoreButton, "HiScoreButton");

    usr->tri.init();
    usr->totalFrames = 0;
    usr->storage.storageInit("10x", "bowling");
    usr->username_len = usr->storage.getChar(Storage::USERNAME, usr->username, 20);


    LocalHi_Init(&usr->localHi);
}

inline void initSoundSettings(UserContext* usr, SoundSettings* self, GameSoundSystem* soundSystem)
{
    self->soundSystem = soundSystem;
    // self->activated = false;

    // Initialize from sound system - read ACTUAL current values
    self->musicVolume = soundSystem->musicVolume;
    self->sfxVolume = soundSystem->sfxVolume;
    
    // Determine current quality mode from sound system state
    if (soundSystem->useWavPlayback) {
        self->quality = SoundSettings::QUALITY_WAV;
    // } else if (soundSystem->sampleRate == 11025) {
    //     self->quality = SoundSettings::QUALITY_LOFI;
    } else {
        self->quality = SoundSettings::QUALITY_HIFI;
    }
    
    printf("[SoundSettings] Initialized: musicVol=%.2f, sfxVol=%.2f, quality=%d\n",
           self->musicVolume, self->sfxVolume, (int)self->quality);

    // Volume labels
    strcpy(self->musicVolLabels[0], "0%");
    strcpy(self->musicVolLabels[1], "25%");
    strcpy(self->musicVolLabels[2], "50%");
    strcpy(self->musicVolLabels[3], "75%");
    strcpy(self->musicVolLabels[4], "100%");

    memcpy(self->sfxVolLabels, self->musicVolLabels, sizeof(self->sfxVolLabels));

    // Quality labels
    strcpy(self->qualityLabels[0], "Cached");
    // strcpy(self->qualityLabels[1], "LoFi 11025");
    strcpy(self->qualityLabels[1], "Synth");

    // Initialize clicks
    const char* volIds[] = { "musicVol0", "musicVol1", "musicVol2", "musicVol3", "musicVol4" };
    for (int i = 0; i < 5; i++) {
        initClaytonClick(&usr->musicVolClicks[i], volIds[i]);
    }

    const char* sfxIds[] = { "sfxVol0", "sfxVol1", "sfxVol2", "sfxVol3", "sfxVol4" };
    for (int i = 0; i < 5; i++) {
        initClaytonClick(&usr->sfxVolClicks[i], sfxIds[i]);
    }

    const char* qualIds[] = {
         "qualWav", 
        // "qualLofi",
         "qualHifi", 
        };
    for (int i = 0; i < 2; i++) {
        initClaytonClick(&usr->qualityClicks[i], qualIds[i]);
    }

    initClaytonClick(&usr->nextSongClick, "nextSongClick");
    initClaytonClick(&usr->prevSongClick, "prevSongClick");
    initClaytonClick(&usr->closeClick, "soundSettingsClose");
    initClaytonClick(&usr->hiScoreCloseClick, "hiScoreCloseClose");
    
    // Song names - fun random names for each track
    strcpy(self->songNames[1], "1. Bowling Strike");
    strcpy(self->songNames[2], "2. Gutter Groove");
    strcpy(self->songNames[3], "3. Pin Crusher");
    strcpy(self->songNames[4], "4. Alley Cat");
    
    // Set initial song name
    strcpy(self->currentSongName, self->songNames[self->soundSystem->currentSongIndex]);

    // Initialize WAV export flag
    self->needsWavExport = false;
    self->wavExportInProgress = false;
    self->wavExportStatus[0] = '\0';
}

inline void applySoundSettings(SoundSettings* self)
{
    if (!self->soundSystem) return;

    // Apply volume to modules immediately (no restart needed)
    // Volume changes do NOT affect quality setting
    if (self->soundSystem->musicModule) {
        xfm_module_set_volume(self->soundSystem->musicModule, self->musicVolume);
    }
    if (self->soundSystem->sfxModule) {
        xfm_module_set_volume(self->soundSystem->sfxModule, self->sfxVolume);
    }
    // WAV volume control
    if (self->soundSystem->wavMusicModule) {
        printf("[SoundVolume] WAV music volume: %.2f\n", self->musicVolume);
        xfm_wav_module_set_volume(self->soundSystem->wavMusicModule, self->musicVolume);
    }
    if (self->soundSystem->wavSfxModule) {
        printf("[SoundVolume] WAV SFX volume: %.2f\n", self->sfxVolume);
        xfm_wav_module_set_volume(self->soundSystem->wavSfxModule, self->sfxVolume);
    }

    // Check current mode BEFORE applying new setting
    bool wasWav = self->soundSystem->useWavPlayback;
    int wasSampleRate = self->soundSystem->sampleRate;
    
    // Apply new quality setting
    bool wantsWav = false;
    int wantsSampleRate = 44100;
    
    switch (self->quality) {
        case SoundSettings::QUALITY_HIFI:
            wantsWav = false;
            wantsSampleRate = 44100;
            self->soundSystem->sampleRate = 44100;
            printf("[SoundSettings] Quality requested: HiFi 44100 (synth)\n");
            break;
        // case SoundSettings::QUALITY_LOFI:
        //     wantsWav = false;
        //     wantsSampleRate = 11025;
        //     self->soundSystem->sampleRate = 11025;
        //     printf("[SoundSettings] Quality requested: LoFi 11025 (synth)\n");
        //     break;
        case SoundSettings::QUALITY_WAV:
            wantsWav = true;
            wantsSampleRate = 11025;  // WAV always uses 44100
            wantsSampleRate = 44100;  // WAV always uses 44100
            self->soundSystem->sampleRate = 11025;
            self->soundSystem->sampleRate = 44100;
            printf("[SoundSettings] Quality requested: WAV (pre-rendered)\n");
            break;
    }
    
    // Check if mode actually changed (WAV flag OR sample rate)
    bool modeChanged = (wantsWav != wasWav) || (wantsSampleRate != wasSampleRate);

    if (modeChanged) {
        printf("[SoundSettings] Mode CHANGED (WAV=%d→%d, Rate=%d→%d) - scheduling restart...\n",
               wasWav, wantsWav, wasSampleRate, wantsSampleRate);

        // Apply new mode immediately (will take effect after restart)
        self->soundSystem->useWavPlayback = wantsWav;

        // If switching to WAV but buffers aren't loaded, trigger export first
        if (wantsWav && !self->soundSystem->hasRuntimeWavBuffers) {
            printf("[SoundSettings] WAV selected but buffers not loaded - triggering export...\n");
            self->needsWavExport = true;
            // Don't restart yet - export will trigger restart when done
            return;
        }

        // Get current song pattern for restart
        const char* songPattern = SONG_01;
        switch (self->soundSystem->currentSongIndex) {
            case 1: songPattern = SONG_01; break;
            case 2: songPattern = SONG_02; break;
            case 3: songPattern = SONG_03; break;
            case 4: songPattern = SONG_04; break;
        }

        self->soundSystem->startRestart(songPattern);
    } else {
        printf("[SoundSettings] Mode unchanged (no restart needed)\n");
    }
}

inline bool processSoundSettingsEvent(UserContext* usr, SoundSettings* self, SDL_Event event)
{
    if (!self->activated) {
        return false;
    }

    bool mouseDown = event.type == SDL_MOUSEBUTTONDOWN;
    bool mouseUp = event.type == SDL_MOUSEBUTTONUP;

    if (!mouseDown && !mouseUp) {
        return false;
    }

    bool handled = false;

    // Music volume buttons
    for (int i = 0; i < 5; i++) {
        if (isClaytonClicked(&usr->musicVolClicks[i], event)) {
            self->musicVolume = i * 0.25f;
            applySoundSettings(self);
            handled = true;
        }
    }

    // // SFX volume buttons
    // for (int i = 0; i < 5; i++) {
    //     if (isClaytonClicked(&self->sfxVolClicks[i], event)) {
    //         self->sfxVolume = i * 0.25f;
    //         applySoundSettings(self);
    //         handled = true;
    //     }
    // }

    // Quality buttons
    for (int i = 0; i < 3; i++) {
        if (isClaytonClicked(&usr->qualityClicks[i], event)) {
            self->quality = (SoundSettings::Quality)i;
            applySoundSettings(self);
            handled = true;
        }
    }

    // Next song button
    if (isClaytonClicked(&usr->nextSongClick, event)) {
        if (self->soundSystem) {
            self->soundSystem->nextSong();
        }
        handled = true;
    }
    
    // Previous song button
    if (isClaytonClicked(&usr->prevSongClick, event)) {
        if (self->soundSystem) {
            self->soundSystem->previousSong();
        }
        handled = true;
    }

    // Close button
    if (isClaytonClicked(&usr->closeClick, event)) {
        self->activated = false;
        return true;
    }

    // If pointer is over the panel, consume the event (even if not on a button)
    // This prevents click-through to the game
    if (Clay_PointerOver(CLAY_ID("SoundSettingsContainer"))) {
        return true;
    }

    return handled;
}

// =============================================================================
// High Score Panel — Clay UI Builder
// =============================================================================

// =============================================================================
// High Score Panel — Clay UI Builder (Arena-Backed Strings)
// =============================================================================
// inline void buildHiScoreClay(UserContext* usr, LocalHighscore* self) {
//     ClayArena* arena = &usr->clayArena;
    
//     CLAY(CLAY_ID("TestPanel"), CLAY_THEME_PANEL) {
//         CLAY_TEXT(CLAY_STRING("TEST STATIC"), CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_TITLE));
        
//         Clay_String dyn = ClayArena_FormatString(arena, "TEST DYNAMIC: %d", 9999);
//         CLAY_TEXT(dyn, CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_BODY));
        
//         if (self) {
//             Clay_String score = ClayArena_FormatString(arena, "Score: %d", self->lastSubmittedScore);
//             CLAY_TEXT(score, CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_LARGE));
//         }
//     }
// }
// =============================================================================
// buildHiScoreClay — Clay UI Builder (Embedded Arena, C-Compatible)
// =============================================================================
inline void buildHiScoreClay(UserContext* usr, LocalHighscore* self) {
    if (!usr || !self) return;
    ClayArena* arena = &usr->clayArena;  // ← Embedded arena
    
    // Theme font configs
    Clay_TextElementConfig labelCfg = CLAY_THEME_TEXT_LABEL;
    Clay_TextElementConfig buttonCfg = CLAY_THEME_TEXT_BUTTON;
    Clay_TextElementConfig titleCfg = CLAY_THEME_TEXT_TITLE;
    Clay_TextElementConfig scoreCfg = CLAY_THEME_TEXT_LARGE;
    
    CLAY(CLAY_ID("HiScoreContainer"), CLAY_THEME_OVERLAY) {
        CLAY(CLAY_ID("HiScoreWindow"), CLAY_THEME_PANEL) {
            
            // Title bar
            CLAY(CLAY_ID("HiScoreTitle"), {
                .layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                          .padding = {0,0,5,0}, .childGap = 10,
                          .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                          .layoutDirection = CLAY_LEFT_TO_RIGHT}
            }) {
                CLAY_TEXT(CLAY_STRING("🏆 Top Scores"), CLAY_TEXT_CONFIG(titleCfg));
                CLAY(CLAY_ID("TitleDivider"), {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1)}}}){};
                CLAY(usr->hiScoreCloseClick.clayId, CLAY_THEME_BTN_DANGER) {
                    CLAY_TEXT(CLAY_STRING("x"), CLAY_TEXT_CONFIG(buttonCfg));
                }
            }

            if (usr->shouldShowHiScoreWithLatest == true)
            {

                // Feedback section — simplified text-only percentile
                if (self->lastSubmitResult != LOCALHI_SUBMIT_NONE)
                {
                    Clay_Color feedbackBg = (self->lastSubmitResult == LOCALHI_SUBMIT_NEW_RECORD)
                        ? CLAY_COLOR_BTN_SUCCESS
                        : CLAY_COLOR_BTN_DISABLED;
                    Clay_String feedbackTitle;
                    char feedbackBuf[64];
                    if (self->lastSubmitResult == LOCALHI_SUBMIT_NEW_RECORD) {
                        int len = snprintf(feedbackBuf, sizeof(feedbackBuf), "Your score is in top %d", self->lastSubmittedRank);
                        feedbackTitle = ClayArena_AllocString(arena, feedbackBuf);
                    } else {
                        feedbackTitle = CLAY_STRING("Good Run!");
                    }

                    CLAY(
                        CLAY_ID("Feedback"),
                        {.layout =
                             {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                              .padding = {12, 12, 12, 12},
                              .childGap = 8,
                              .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                              .layoutDirection = CLAY_TOP_TO_BOTTOM},
                         .backgroundColor = feedbackBg,
                         .cornerRadius = {
                             CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG
                         }}
                    )
                    {
                        CLAY_TEXT(feedbackTitle, CLAY_TEXT_CONFIG(buttonCfg));

                        // Score display
                        Clay_String scoreStr =
                            ClayArena_FormatString(arena, "%d points", self->lastSubmittedScore);
                        CLAY_TEXT(scoreStr, CLAY_TEXT_CONFIG(scoreCfg));

                        // Simple percentile label: "Your score is higher than X% of all recent
                        // runs"
                        Clay_String pctLabel = ClayArena_FormatString(
                            arena,
                            "Your score is higher than %.0f%% of all recent runs",
                            self->lastSubmittedPercentile
                        );
                        CLAY_TEXT(pctLabel, CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_BODY));
                    }
                }
            }

            // Leaderboard
            CLAY(CLAY_ID("LBSection"), CLAY_THEME_SECTION) {
                CLAY_TEXT(CLAY_STRING("Leaderboard (Last Hour)"), CLAY_TEXT_CONFIG(labelCfg));
                
                // Header
                CLAY(CLAY_ID("LBHeader"), {
                    .layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                              .padding = {5,5,5,5}, .childGap = 10, .layoutDirection = CLAY_LEFT_TO_RIGHT},
                    .border = {.color = CLAY_COLOR_DIVIDER, .width = {.top = 1, .bottom = 1}}
                }) {
                    CLAY(CLAY_ID("HRank"), {.layout = {.sizing = {CLAY_SIZING_FIXED(40), CLAY_SIZING_FIT()}}})
                        { CLAY_TEXT(CLAY_STRING("#"), CLAY_TEXT_CONFIG(labelCfg)); }
                    CLAY(CLAY_ID("HName"), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()}}})
                        { CLAY_TEXT(CLAY_STRING("Player"), CLAY_TEXT_CONFIG(labelCfg)); }
                    CLAY(CLAY_ID("HScore"), {.layout = {.sizing = {CLAY_SIZING_FIXED(80), CLAY_SIZING_FIT()},
                                                        .childAlignment = {CLAY_ALIGN_X_RIGHT, CLAY_ALIGN_Y_CENTER}}})
                        { CLAY_TEXT(CLAY_STRING("Score"), CLAY_TEXT_CONFIG(labelCfg)); }
                    CLAY(CLAY_ID("HTime"), {.layout = {.sizing = {CLAY_SIZING_FIXED(60), CLAY_SIZING_FIT()},
                                                       .childAlignment = {CLAY_ALIGN_X_RIGHT, CLAY_ALIGN_Y_CENTER}}})
                        { CLAY_TEXT(CLAY_STRING("Age"), CLAY_TEXT_CONFIG(labelCfg)); }
                }
                
                // Entries
                LocalHi_CleanExpired(self);
                for (int32_t i = 0; i < self->count; i++) {
                    LocalHiEntry* e = &self->entries[i];
                    bool isUser = (self->lastSubmitResult == LOCALHI_SUBMIT_NEW_RECORD && self->lastSubmittedRank == i + 1);
                    Clay_Color rowBg = isUser ? (Clay_Color){90,70,140,255} : CLAY_COLOR_PANEL_SECTION;
                    
                    CLAY(CLAY_IDI("LBRow", i), {
                        .layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                                  .padding = {8,8,8,8}, .childGap = 10, .layoutDirection = CLAY_LEFT_TO_RIGHT},
                        .backgroundColor = rowBg,
                        .cornerRadius = {CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD}
                    }) {
                        // Rank
                        CLAY(CLAY_IDI("RRank", i), {.layout = {.sizing = {CLAY_SIZING_FIXED(40), CLAY_SIZING_FIT()},
                                                               .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}}}) {
                            Clay_String rs = ClayArena_FormatString(arena, "%d", i+1);
                            Clay_Color rc = (i==0)?(Clay_Color){255,215,0,255}:(i==1)?(Clay_Color){192,192,192,255}
                                                :(i==2)?(Clay_Color){205,127,50,255}:CLAY_COLOR_TEXT_SECONDARY;
                            Clay_TextElementConfig rcf = {.textColor=rc, .fontId=CLAY_FONT_NOTO, .fontSize=CLAY_FONT_SIZE_SM};
                            CLAY_TEXT(rs, CLAY_TEXT_CONFIG(rcf));
                        }
                        // Username
                        CLAY(CLAY_IDI("RName", i), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                                                               .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER}}}) {
                            Clay_String ns = ClayArena_AllocString(arena, e->username);
                            Clay_Color nc = isUser ? CLAY_COLOR_BTN_ACTIVE : CLAY_COLOR_TEXT_PRIMARY;
                            Clay_TextElementConfig ncf = {.textColor=nc, .fontId=CLAY_FONT_NOTO, .fontSize=CLAY_FONT_SIZE_SM};
                            CLAY_TEXT(ns, CLAY_TEXT_CONFIG(ncf));
                        }
                        // Score
                        CLAY(CLAY_IDI("RScore", i), {.layout = {.sizing = {CLAY_SIZING_FIXED(80), CLAY_SIZING_FIT()},
                                                                .childAlignment = {CLAY_ALIGN_X_RIGHT, CLAY_ALIGN_Y_CENTER}}}) {
                            Clay_String ss = ClayArena_FormatString(arena, "%d", e->score);
                            Clay_Color sc = isUser ? CLAY_COLOR_BTN_SUCCESS : CLAY_COLOR_TEXT_PRIMARY;
                            Clay_TextElementConfig scf = {.textColor=sc, .fontId=CLAY_FONT_NOTO, .fontSize=CLAY_FONT_SIZE_SM};
                            CLAY_TEXT(ss, CLAY_TEXT_CONFIG(scf));
                        }
                        // Time
                        CLAY(CLAY_IDI("RTime", i), {.layout = {.sizing = {CLAY_SIZING_FIXED(60), CLAY_SIZING_FIT()},
                                                               .childAlignment = {CLAY_ALIGN_X_RIGHT, CLAY_ALIGN_Y_CENTER}}}) {
                            int32_t m = LocalHi_GetMinutesAgo(e->timestamp);
                            Clay_String ts = ClayArena_FormatString(arena, "%dm", m);
                            CLAY_TEXT(ts, CLAY_TEXT_CONFIG(labelCfg));
                        }
                    }
                }
                
                // Empty state
                if (self->count == 0) {
                    CLAY(CLAY_ID("LBEmpty"), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(60)},
                                                         .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}}}) {
                        CLAY_TEXT(CLAY_STRING("No scores yet — be the first! 🎮"), CLAY_TEXT_CONFIG(labelCfg));
                    }
                }
            }
            
            // Stats footer
            CLAY(CLAY_ID("LBStats"), {
                .layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                          .padding = {10,10,10,10}, .childGap = 15,
                          .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                          .layoutDirection = CLAY_LEFT_TO_RIGHT}
            }) {
                Clay_String att = ClayArena_FormatString(arena, "Attempts: %d", self->percentileTracker.totalAttempts);
                CLAY_TEXT(att, CLAY_TEXT_CONFIG(labelCfg));
                if (self->percentileTracker.totalAttempts > 0) {
                    Clay_String rng = ClayArena_FormatString(arena, "Range: %d–%d", 
                                                            self->percentileTracker.minScore,
                                                            self->percentileTracker.maxScore);
                    CLAY_TEXT(rng, CLAY_TEXT_CONFIG(labelCfg));
                }
            }
        }
    }
}
// inline void buildHiScoreClay2(UserContext* usr, LocalHighscore* self)
// {
//     if (!self) return;
    
//     ClayArena* arena = &usr->clayArena;  // Shortcut

//     // Font configs - use theme
//     Clay_TextElementConfig labelFontCfg = CLAY_THEME_TEXT_LABEL;
//     Clay_TextElementConfig buttonFontCfg = CLAY_THEME_TEXT_BUTTON;
//     Clay_TextElementConfig titleFontCfg = CLAY_THEME_TEXT_TITLE;
//     Clay_TextElementConfig scoreFontCfg = CLAY_THEME_TEXT_LARGE;
//     Clay_TextElementConfig rankFontCfg = {
//         .textColor = CLAY_COLOR_TEXT_PRIMARY,
//         .fontId = CLAY_FONT_NOTO,
//         .fontSize = CLAY_FONT_SIZE_SM,
//     };

//     // Main container
//     CLAY(
//         CLAY_ID("HiScoreContainer"),
//         CLAY_THEME_OVERLAY
//     ) {
//         CLAY(
//             CLAY_ID("HiScoreWindow"),
//             CLAY_THEME_PANEL
//         ) {
//             // ========== TITLE BAR ==========
//             CLAY(
//                 CLAY_ID("HiScoreTitleBar"),
//                 {
//                     .layout = {
//                         .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
//                         .padding = {0, 0, 5, 0},
//                         .childGap = 10,
//                         .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
//                         .layoutDirection = CLAY_LEFT_TO_RIGHT,
//                     },
//                 }
//             ) {
//                 CLAY_TEXT(CLAY_STRING("🏆 Top Scores"), CLAY_TEXT_CONFIG(titleFontCfg));

//                 CLAY(
//                     CLAY_ID("HiScoreTitleDivider"),
//                     { .layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1)}}}
//                 ){};

//                 CLAY(
//                     usr->hiScoreCloseClick.clayId,
//                     CLAY_THEME_BTN_DANGER
//                 ) {
//                     CLAY_TEXT(CLAY_STRING("x"), CLAY_TEXT_CONFIG(buttonFontCfg));
//                 }
//             }

//             // ========== LAST SUBMISSION FEEDBACK ==========
//             if (self->lastSubmitResult != LocalHighscore::SUBMIT_NONE) {
//                 Clay_Color feedbackBg = (self->lastSubmitResult == LocalHighscore::SUBMIT_NEW_RECORD) 
//                     ? CLAY_COLOR_BTN_SUCCESS 
//                     : CLAY_COLOR_BTN_DISABLED;
                
//                 Clay_String feedbackMsg = (self->lastSubmitResult == LocalHighscore::SUBMIT_NEW_RECORD)
//                     ? CLAY_STRING("🎉 New Record!")
//                     : CLAY_STRING("💪 Keep Trying!");

//                 CLAY(
//                     CLAY_ID("HiScoreFeedback"),
//                     {
//                         .layout = {
//                             .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
//                             .padding = {12, 12, 12, 12},
//                             .childGap = 8,
//                             .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
//                             .layoutDirection = CLAY_TOP_TO_BOTTOM,
//                         },
//                         .backgroundColor = feedbackBg,
//                         .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
//                     }
//                 ) {
//                     CLAY_TEXT(feedbackMsg, CLAY_TEXT_CONFIG(buttonFontCfg));
                    
//                     CLAY(
//                         CLAY_ID("HiScoreFeedbackStats"),
//                         {
//                             .layout = {
//                                 .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
//                                 .childGap = 20,
//                                 .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
//                                 .layoutDirection = CLAY_LEFT_TO_RIGHT,
//                             },
//                         }
//                     ) {
//                         // Score display
//                         CLAY(
//                             CLAY_ID("HiScoreFeedbackScore"),
//                             { .layout = { .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER} } }
//                         ) {
//                             Clay_String scoreStr = ClayArena_FormatString(arena, "%d", self->lastSubmittedScore);
//                             std::cerr << "score:: " << self->lastSubmittedScore << std::endl;
//                             CLAY_TEXT(scoreStr, CLAY_TEXT_CONFIG(scoreFontCfg));
//                             CLAY_TEXT(CLAY_STRING("pts"), CLAY_TEXT_CONFIG(labelFontCfg));
//                         }

//                         // Percentile with progress bar
//                         CLAY(
//                             CLAY_ID("HiScoreFeedbackPercentile"),
//                             {
//                                 .layout = {
//                                     .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
//                                     .childGap = 5,
//                                     .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
//                                     .layoutDirection = CLAY_TOP_TO_BOTTOM,
//                                 },
//                             }
//                         ) {
//                             Clay_String pctStr = ClayArena_FormatString(arena, "%.0f%%", self->lastSubmittedPercentile);
//                             CLAY_TEXT(pctStr, CLAY_TEXT_CONFIG(labelFontCfg));
                            
//                             CLAY(
//                                 CLAY_ID("HiScorePctBarBg"),
//                                 CLAY_THEME_PROGRESS_BAR_BG
//                             ) {
//                                 CLAY(
//                                     CLAY_ID("HiScorePctBarFill"),
//                                     CLAY_THEME_PROGRESS_BAR_FILL(self->lastSubmittedPercentile / 100.0f)
//                                 ) {};
//                             }
//                         }
//                     }
//                 }
//             }

//             // ========== TOP 10 LIST SECTION ==========
//             CLAY(
//                 CLAY_ID("HiScoreListSection"),
//                 CLAY_THEME_SECTION
//             ) {
//                 CLAY_TEXT(CLAY_STRING("Leaderboard (Last Hour)"), CLAY_TEXT_CONFIG(labelFontCfg));

//                 // Header row
//                 CLAY(
//                     CLAY_ID("HiScoreHeader"),
//                     {
//                         .layout = {
//                             .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
//                             .padding = {5, 5, 5, 5},
//                             .childGap = 10,
//                             .layoutDirection = CLAY_LEFT_TO_RIGHT,
//                         },
//                         .border = {
//                             .color = CLAY_COLOR_DIVIDER,
//                             .width = {.top = 1, .right = 0, .bottom = 1, .left = 0},
//                         },
//                     }
//                 ) {
//                     CLAY(CLAY_ID("HiScoreHeaderRank"), { .layout = {.sizing = {CLAY_SIZING_FIXED(40), CLAY_SIZING_FIT()}} }) {
//                         CLAY_TEXT(CLAY_STRING("#"), CLAY_TEXT_CONFIG(rankFontCfg));
//                     }
//                     CLAY(CLAY_ID("HiScoreHeaderName"), { .layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()}} }) {
//                         CLAY_TEXT(CLAY_STRING("Player"), CLAY_TEXT_CONFIG(rankFontCfg));
//                     }
//                     CLAY(CLAY_ID("HiScoreHeaderScore"), { .layout = {.sizing = {CLAY_SIZING_FIXED(80), CLAY_SIZING_FIT()}, .childAlignment = {CLAY_ALIGN_X_RIGHT, CLAY_ALIGN_Y_CENTER}} }) {
//                         CLAY_TEXT(CLAY_STRING("Score"), CLAY_TEXT_CONFIG(rankFontCfg));
//                     }
//                     CLAY(CLAY_ID("HiScoreHeaderTime"), { .layout = {.sizing = {CLAY_SIZING_FIXED(60), CLAY_SIZING_FIT()}, .childAlignment = {CLAY_ALIGN_X_RIGHT, CLAY_ALIGN_Y_CENTER}} }) {
//                         CLAY_TEXT(CLAY_STRING("Age"), CLAY_TEXT_CONFIG(rankFontCfg));
//                     }
//                 }

//                 // Entries list
//                 LocalHi_CleanExpired(self);
                
//                 for (int32_t i = 0; i < self->count; i++) {
//                     LocalHiEntry* entry = &self->entries[i];
//                     bool isUserEntry = (self->lastSubmitResult == LocalHighscore::SUBMIT_NEW_RECORD && 
//                                        self->lastSubmittedRank == i + 1);
                    
//                     Clay_Color rowBg = isUserEntry 
//                         ? (Clay_Color){90, 70, 140, 255}
//                         : CLAY_COLOR_PANEL_SECTION;

//                     CLAY(
//                         CLAY_IDI("HiScoreRow", i),
//                         {
//                             .layout = {
//                                 .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
//                                 .padding = {8, 8, 8, 8},
//                                 .childGap = 10,
//                                 .layoutDirection = CLAY_LEFT_TO_RIGHT,
//                             },
//                             .backgroundColor = rowBg,
//                             .cornerRadius = {CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD},
//                         }
//                     ) {
//                         // Rank badge
//                         CLAY(
//                             CLAY_IDI("HiScoreRank", i),
//                             { .layout = {.sizing = {CLAY_SIZING_FIXED(40), CLAY_SIZING_FIT()}, .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}} }
//                         ) {
//                             Clay_String rankStr = ClayArena_FormatString(arena, "%d", i + 1);
//                             Clay_Color rankColor = (i == 0) ? (Clay_Color){255, 215, 0, 255}
//                                                 : (i == 1) ? (Clay_Color){192, 192, 192, 255}
//                                                 : (i == 2) ? (Clay_Color){205, 127, 50, 255}
//                                                 : CLAY_COLOR_TEXT_SECONDARY;
//                             Clay_TextElementConfig rankCfg = {
//                                 .textColor = rankColor,
//                                 .fontId = CLAY_FONT_NOTO,
//                                 .fontSize = CLAY_FONT_SIZE_SM,
//                             };
//                             CLAY_TEXT(rankStr, CLAY_TEXT_CONFIG(rankCfg));
//                         }

//                         // Username
//                         CLAY(
//                             CLAY_IDI("HiScoreName", i),
//                             { .layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()}, .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER}} }
//                         ) {
//                             Clay_String nameStr = ClayArena_AllocString(arena, entry->username);
//                             Clay_Color nameColor = isUserEntry ? CLAY_COLOR_BTN_ACTIVE : CLAY_COLOR_TEXT_PRIMARY;
//                             Clay_TextElementConfig nameCfg = {
//                                 .textColor = nameColor,
//                                 .fontId = CLAY_FONT_NOTO,
//                                 .fontSize = CLAY_FONT_SIZE_SM,
//                             };
//                             CLAY_TEXT(nameStr, CLAY_TEXT_CONFIG(nameCfg));
//                         }

//                         // Score
//                         CLAY(
//                             CLAY_IDI("HiScoreScore", i),
//                             { .layout = {.sizing = {CLAY_SIZING_FIXED(80), CLAY_SIZING_FIT()}, .childAlignment = {CLAY_ALIGN_X_RIGHT, CLAY_ALIGN_Y_CENTER}} }
//                         ) {
//                             Clay_String scoreStr = ClayArena_FormatString(arena, "%d", entry->score);
//                             Clay_Color scoreColor = isUserEntry ? CLAY_COLOR_BTN_SUCCESS : CLAY_COLOR_TEXT_PRIMARY;
//                             Clay_TextElementConfig scoreCfg = {
//                                 .textColor = scoreColor,
//                                 .fontId = CLAY_FONT_NOTO,
//                                 .fontSize = CLAY_FONT_SIZE_SM,
//                             };
//                             CLAY_TEXT(scoreStr, CLAY_TEXT_CONFIG(scoreCfg));
//                         }

//                         // Time ago
//                         CLAY(
//                             CLAY_IDI("HiScoreTime", i),
//                             { .layout = {.sizing = {CLAY_SIZING_FIXED(60), CLAY_SIZING_FIT()}, .childAlignment = {CLAY_ALIGN_X_RIGHT, CLAY_ALIGN_Y_CENTER}} }
//                         ) {
//                             int32_t mins = LocalHi_GetMinutesAgo(entry->timestamp);
//                             Clay_String timeStr = ClayArena_FormatString(arena, "%dm", mins);
//                             CLAY_TEXT(timeStr, CLAY_TEXT_CONFIG(labelFontCfg));
//                         }
//                     }
//                 }

//                 // Empty state
//                 if (self->count == 0) {
//                     CLAY(
//                         CLAY_ID("HiScoreEmpty"),
//                         { .layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(60)}, .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}} }
//                     ) {
//                         CLAY_TEXT(CLAY_STRING("No scores yet — be the first! 🎮"), CLAY_TEXT_CONFIG(labelFontCfg));
//                     }
//                 }
//             }

//             // ========== STATS FOOTER ==========
//             CLAY(
//                 CLAY_ID("HiScoreStatsFooter"),
//                 {
//                     .layout = {
//                         .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
//                         .padding = {10, 10, 10, 10},
//                         .childGap = 15,
//                         .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
//                         .layoutDirection = CLAY_LEFT_TO_RIGHT,
//                     },
//                 }
//             ) {
//                 CLAY(
//                     CLAY_ID("HiScoreStatAttempts"),
//                     { .layout = {.childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}} }
//                 ) {
//                     Clay_String attemptsStr = ClayArena_FormatString(arena, "Attempts: %d", 
//                                                                     self->percentileTracker.totalAttempts);
//                     CLAY_TEXT(attemptsStr, CLAY_TEXT_CONFIG(labelFontCfg));
//                 }

//                 if (self->percentileTracker.totalAttempts > 0) {
//                     CLAY(
//                         CLAY_ID("HiScoreStatRange"),
//                         { .layout = {.childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}} }
//                     ) {
//                         Clay_String rangeStr = ClayArena_FormatString(arena, "Range: %d–%d", 
//                                                                      self->percentileTracker.minScore,
//                                                                      self->percentileTracker.maxScore);
//                         CLAY_TEXT(rangeStr, CLAY_TEXT_CONFIG(labelFontCfg));
//                     }
//                 }
//             }
//         }
//     }
// }

inline void buildSoundSettingsClay(UserContext* usr, SoundSettings* self)
{
    if (!self->activated) {
        return;
    }

    // Font configs - use theme
    Clay_TextElementConfig labelFontCfg = CLAY_THEME_TEXT_LABEL;
    Clay_TextElementConfig buttonFontCfg = CLAY_THEME_TEXT_BUTTON;
    Clay_TextElementConfig titleFontCfg = CLAY_THEME_TEXT_TITLE;

    // Main container
    CLAY(
        CLAY_ID("SoundSettingsContainer"),
        {
            .layout = {
                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                .padding = {0, 0, 0, 0},
                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
            },
        }
    ) {
        // Settings panel window
        CLAY(
            CLAY_ID("SoundSettingsWindow"),
            {
                .layout = {
                    .sizing = {CLAY_SIZING_PERCENT(0.8f), CLAY_SIZING_FIT()},
                    .padding = {20, 20, 20, 20},
                    .childGap = 15,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
                .backgroundColor = CLAY_COLOR_PANEL_BG,
                .cornerRadius = {CLAY_RADIUS_XL, CLAY_RADIUS_XL, CLAY_RADIUS_XL, CLAY_RADIUS_XL},
            }
        ) {
            // Title bar
            CLAY(
                CLAY_ID("SoundSettingsTitle"),
                {
                    .layout = {
                        .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                        .padding = {0, 0, 10, 0},
                        .childGap = 10,
                        .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT,
                    },
                }
            ) {
                CLAY_TEXT(CLAY_STRING("Sound Settings"), CLAY_TEXT_CONFIG(titleFontCfg));

                /* -------- DIVIDER -------- */
                CLAY(
                    CLAY_ID("SoundSettingsTitleDivider"),
                    {
                        .layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1)}},
                    }
                ){};

                // Close button (right side)
                CLAY(
                    usr->closeClick.clayId,
                    CLAY_THEME_BTN_DANGER
                ) {
                    CLAY_TEXT(CLAY_STRING("X"), CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_BUTTON));
                }
            }

            // Quality Section OR Restart Progress (mutually exclusive)
            if (self->soundSystem && self->soundSystem->restartProgress > 0.0f && self->soundSystem->restartProgress < 1.0f) {
                // Show progress indicator instead of quality buttons during restart
                CLAY(
                    CLAY_ID("RestartProgressSection"),
                    {
                        .layout = {
                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                            .padding = {10, 10, 10, 10},
                            .childGap = 10,
                            .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        },
                        .backgroundColor = {80, 60, 40, 255},
                        .cornerRadius = {10, 10, 10, 10},
                    }
                ) {
                    Clay_TextElementConfig progressFontCfg = {
                        .textColor = {255, 255, 100, 255},
                        .fontId = 0,
                        .fontSize = (uint16_t)18,
                    };
                    CLAY_TEXT(CLAY_STRING("Changing quality..."), CLAY_TEXT_CONFIG(progressFontCfg));

                    // Progress bar background
                    CLAY(
                        CLAY_ID("ProgressBarBg"),
                        {
                            .layout = {
                                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(20)},
                            },
                            .backgroundColor = {40, 40, 40, 255},
                            .cornerRadius = {5, 5, 5, 5},
                        }
                    ) {
                        // Progress bar fill
                        float progress = self->soundSystem->restartProgress;
                        Clay_Color progressColor;
                        if (progress < 0.5f) {
                            progressColor = {200, 200, 50, 255};  // Yellow
                        } else if (progress < 0.8f) {
                            progressColor = {200, 150, 50, 255};  // Orange
                        } else {
                            progressColor = {50, 200, 50, 255};   // Green
                        }
                        
                        CLAY(
                            CLAY_ID("ProgressBarFill"),
                            {
                                .layout = {
                                    .sizing = {CLAY_SIZING_PERCENT(progress), CLAY_SIZING_GROW()},
                                },
                                .backgroundColor = progressColor,
                                .cornerRadius = {5, 5, 5, 5},
                            }
                        ) {};
                    }

                    // Progress percentage text
                    char progressText[20];
                    int progressLen = snprintf(progressText, sizeof(progressText), "%d%%", 
                                               (int)(self->soundSystem->restartProgress * 100));
                    Clay_String progressStr = {
                        .isStaticallyAllocated = false,
                        .length = progressLen,
                        .chars = progressText,
                    };
                    CLAY_TEXT(progressStr, CLAY_TEXT_CONFIG(progressFontCfg));
                }
            } else {
                // Show quality buttons when not restarting
                CLAY(
                    CLAY_ID("QualitySection"),
                    {
                        .layout = {
                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                            .padding = {10, 10, 10, 10},
                            .childGap = 10,
                            .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        },
                        .backgroundColor = {60, 60, 80, 255},
                        .cornerRadius = {10, 10, 10, 10},
                    }
                ) {
                    CLAY_TEXT(CLAY_STRING("Audio Mode"), CLAY_TEXT_CONFIG(labelFontCfg));

                    // Quality buttons row
                    CLAY(
                        CLAY_ID("QualityRow"),
                        {
                            .layout = {
                                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                                .childGap = 8,
                                .layoutDirection = CLAY_LEFT_TO_RIGHT,
                            },
                        }
                    ) {
                        for (int i = 0; i < 2; i++) {
                            Clay_Color btnColor = (self->quality == i) ?
                                Clay_Color{100, 200, 100, 255} : Clay_Color{80, 80, 120, 255};

                            CLAY(
                                usr->qualityClicks[i].clayId,
                                {
                                    .layout = {
                                        .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(50)},
                                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                    },
                                    .backgroundColor = btnColor,
                                    .cornerRadius = {8, 8, 8, 8},
                                    .border = {
                                        .color = {150, 150, 200, 255},
                                        .width = CLAY_BORDER_ALL(2),
                                    },
                                }
                            ) {
                                Clay_String label = {
                                    .isStaticallyAllocated = false,
                                    .length = (int)strlen(self->qualityLabels[i]),
                                    .chars = self->qualityLabels[i],
                                };
                                CLAY_TEXT(label, CLAY_TEXT_CONFIG(buttonFontCfg));
                            }
                        }
                    }
                }
            }

            // Music Volume Section
            CLAY(
                CLAY_ID("MusicVolSection"),
                {
                    .layout = {
                        .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                        .padding = {10, 10, 10, 10},
                        .childGap = 10,
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    },
                    .backgroundColor = {60, 60, 80, 255},
                    .cornerRadius = {10, 10, 10, 10},
                }
            ) {
                CLAY_TEXT(CLAY_STRING("Music Volume"), CLAY_TEXT_CONFIG(labelFontCfg));

                // Volume buttons row
                CLAY(
                    CLAY_ID("MusicVolRow"),
                    {
                        .layout = {
                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                            .childGap = 8,
                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        },
                    }
                ) {
                    // Find which button should be highlighted (closest to current volume)
                    int selectedButton = -1;
                    float minDiff = 1.0f;
                    for (int i = 0; i < 5; i++) {
                        float targetVol = i * 0.25f;
                        float diff = fabsf(self->musicVolume - targetVol);
                        if (diff < minDiff) {
                            minDiff = diff;
                            selectedButton = i;
                        }
                    }
                    
                    for (int i = 0; i < 5; i++) {
                        Clay_Color btnColor = (i == selectedButton) ?
                            Clay_Color{100, 200, 100, 255} : Clay_Color{80, 80, 120, 255};

                        CLAY(
                            usr->musicVolClicks[i].clayId,
                            {
                                .layout = {
                                    .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(50)},
                                    .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                },
                                .backgroundColor = btnColor,
                                .cornerRadius = {8, 8, 8, 8},
                                .border = {
                                    .color = {150, 150, 200, 255},
                                    .width = CLAY_BORDER_ALL(2),
                                },
                            }
                        ) {
                            Clay_String label = {
                                .isStaticallyAllocated = false,
                                .length = (int)strlen(self->musicVolLabels[i]),
                                .chars = self->musicVolLabels[i],
                            };
                            CLAY_TEXT(label, CLAY_TEXT_CONFIG(buttonFontCfg));
                        }
                    }
                }
            }

            // SFX Volume Section
            // CLAY(
            //     CLAY_ID("SfxVolSection"),
            //     {
            //         .layout = {
            //             .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
            //             .padding = {10, 10, 10, 10},
            //             .childGap = 10,
            //             .layoutDirection = CLAY_TOP_TO_BOTTOM,
            //         },
            //         .backgroundColor = {60, 60, 80, 255},
            //         .cornerRadius = {10, 10, 10, 10},
            //     }
            // ) {
            //     CLAY_TEXT(CLAY_STRING("SFX Volume"), CLAY_TEXT_CONFIG(labelFontCfg));

            //     // Volume buttons row
            //     CLAY(
            //         CLAY_ID("SfxVolRow"),
            //         {
            //             .layout = {
            //                 .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
            //                 .childGap = 8,
            //                 .layoutDirection = CLAY_LEFT_TO_RIGHT,
            //             },
            //         }
            //     ) {
            //         // Find which button should be highlighted (closest to current volume)
            //         int selectedButton = -1;
            //         float minDiff = 1.0f;
            //         for (int i = 0; i < 5; i++) {
            //             float targetVol = i * 0.25f;
            //             float diff = fabsf(self->sfxVolume - targetVol);
            //             if (diff < minDiff) {
            //                 minDiff = diff;
            //                 selectedButton = i;
            //             }
            //         }
                    
            //         for (int i = 0; i < 5; i++) {
            //             Clay_Color btnColor = (i == selectedButton) ?
            //                 Clay_Color{100, 200, 100, 255} : Clay_Color{80, 80, 120, 255};

            //             CLAY(
            //                 self->sfxVolClicks[i].clayId,
            //                 {
            //                     .layout = {
            //                         .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(50)},
            //                         .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
            //                     },
            //                     .backgroundColor = btnColor,
            //                     .cornerRadius = {8, 8, 8, 8},
            //                     .border = {
            //                         .color = {150, 150, 200, 255},
            //                         .width = CLAY_BORDER_ALL(2),
            //                     },
            //                 }
            //             ) {
            //                 Clay_String label = {
            //                     .isStaticallyAllocated = false,
            //                     .length = (int)strlen(self->sfxVolLabels[i]),
            //                     .chars = self->sfxVolLabels[i],
            //                 };
            //                 CLAY_TEXT(label, CLAY_TEXT_CONFIG(buttonFontCfg));
            //             }
            //         }
            //     }
            // }

            CLAY(
                CLAY_ID("SongSection"),
                {
                    .layout = {
                        .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                        .padding = {10, 10, 10, 10},
                        .childGap = 10,
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    },
                    .backgroundColor = {60, 60, 80, 255},
                    .cornerRadius = {10, 10, 10, 10},
                }
            ) {
                CLAY_TEXT(CLAY_STRING("Song"), CLAY_TEXT_CONFIG(labelFontCfg));

            // Action buttons row
            CLAY(
                CLAY_ID("ActionRow"),
                {
                    .layout = {
                        .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                        .childGap = 10,
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT,
                    },
                }
            ) {
                // Previous Song button (left side)
                CLAY(
                    usr->prevSongClick.clayId,
                    {
                        .layout = {
                            .sizing = {CLAY_SIZING_FIXED(60), CLAY_SIZING_FIXED(60)},
                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                        },
                        .backgroundColor = {50, 100, 200, 255},
                        .cornerRadius = {10, 10, 10, 10},
                        .border = {
                            .color = {150, 150, 200, 255},
                            .width = CLAY_BORDER_ALL(2),
                        },
                    }
                ) {
                    CLAY_TEXT(CLAY_STRING("◀"), CLAY_TEXT_CONFIG(buttonFontCfg));
                }
                
                // Song name display (center)
                CLAY(
                    CLAY_ID("SongNameDisplay"),
                    {
                        .layout = {
                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(60)},
                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                        },
                        .backgroundColor = {30, 30, 50, 255},
                        .cornerRadius = {10, 10, 10, 10},
                        .border = {
                            .color = {100, 100, 150, 255},
                            .width = CLAY_BORDER_ALL(1),
                        },
                    }
                ) {
                    Clay_String songName = {
                        .isStaticallyAllocated = false,
                        .length = (int)strlen(self->currentSongName),
                        .chars = self->currentSongName,
                    };
                    Clay_TextElementConfig songNameCfg = {
                        .textColor = {200, 200, 255, 255},
                        .fontId = CLAY_FONT_NOTO,
                        .fontSize = CLAY_FONT_SIZE_SM,
                    };
                    CLAY_TEXT(songName, CLAY_TEXT_CONFIG(songNameCfg));
                }
                
                // Next Song button (right side)
                CLAY(
                    usr->nextSongClick.clayId,
                    {
                        .layout = {
                            .sizing = {CLAY_SIZING_FIXED(60), CLAY_SIZING_FIXED(60)},
                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                        },
                        .backgroundColor = {50, 100, 200, 255},
                        .cornerRadius = {10, 10, 10, 10},
                        .border = {
                            .color = {150, 150, 200, 255},
                            .width = CLAY_BORDER_ALL(2),
                        },
                    }
                ) {
                    CLAY_TEXT(CLAY_STRING("▶"), CLAY_TEXT_CONFIG(buttonFontCfg));
                }
            }
            }
        }
    }
}

// Render WAV export loading indicator (called from game loop during export)
inline void buildWavExportLoadingIndicator(SoundSettings* self, int exportProgress, float exportedSeconds, float exportTotalSeconds, int sampleRate)
{
    if (!self->wavExportInProgress) {
        return;
    }

    Clay_TextElementConfig titleFontCfg = CLAY_THEME_TEXT_TITLE;
    Clay_TextElementConfig bodyFontCfg = CLAY_THEME_TEXT_BODY;

    // Full-screen overlay
    CLAY(
        CLAY_ID("WavExportOverlay"),
        {
            .layout = {
                .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)},
                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
            },
            .backgroundColor = {0, 0, 0, 0},
        }
    ) {
        // Modal window
        CLAY(
            CLAY_ID("WavExportModal"),
            {
                .layout = {
                    .sizing = {CLAY_SIZING_PERCENT(0.7f), CLAY_SIZING_FIT(0)},
                    .padding = {30, 30, 30, 30},
                    .childGap = 20,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
                .backgroundColor = {40, 40, 60, 255},
                .cornerRadius = {15, 15, 15, 15},
            }
        ) {
            CLAY_TEXT(CLAY_STRING("Caching Audio..."), CLAY_TEXT_CONFIG(titleFontCfg));

            // Status text
            Clay_String statusStr = {
                .isStaticallyAllocated = false,
                .length = (int)strlen(self->wavExportStatus),
                .chars = self->wavExportStatus,
            };
            if (statusStr.length > 0) {
                CLAY_TEXT(statusStr, CLAY_TEXT_CONFIG(bodyFontCfg));
            } else {
                CLAY_TEXT(CLAY_STRING("Preparing audio..."), CLAY_TEXT_CONFIG(bodyFontCfg));
            }

            // Progress bar background
            CLAY(
                CLAY_ID("WavExportProgressBg"),
                {
                    .layout = {
                        .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(30)},
                    },
                    .backgroundColor = {40, 40, 40, 255},
                    .cornerRadius = {5, 5, 5, 5},
                }
            ) {
                // Progress bar fill - clamp to 0.0-1.0 range
                float progress = exportProgress / 100.0f;
                if (progress < 0.0f) progress = 0.0f;
                if (progress > 1.0f) progress = 1.0f;
                CLAY(
                    CLAY_ID("WavExportProgressFill"),
                    {
                        .layout = {
                            .sizing = {CLAY_SIZING_PERCENT(progress), CLAY_SIZING_GROW(0)},
                        },
                        .backgroundColor = {50, 200, 50, 255},
                        .cornerRadius = {5, 5, 5, 5},
                    }
                ) {};
            }

            // Progress percentage text with time info
            char progressText[128];
            if (exportTotalSeconds > 0) {
                snprintf(progressText, sizeof(progressText), "Progress: %d%% (%.1fs exported / %.1fs total)",
                         exportProgress, exportedSeconds, exportTotalSeconds);
            } else {
                snprintf(progressText, sizeof(progressText), "Progress: %d%%", exportProgress);
            }
            Clay_String progressStr = {
                .isStaticallyAllocated = false,
                .length = (int)strlen(progressText),
                .chars = progressText,
            };
            CLAY_TEXT(progressStr, CLAY_TEXT_CONFIG(bodyFontCfg));

            // Animated loading dots
            uint32_t tick = SDL_GetTicks64() / 500;  // Change every 500ms
            char dots[5];
            int dotCount = tick % 4;
            for (int i = 0; i < dotCount; i++) dots[i] = '.';
            dots[dotCount] = '\0';

            char loadingText[64];
            snprintf(loadingText, sizeof(loadingText), "Please wait%s", dots);
            Clay_String loadingStr = {
                .isStaticallyAllocated = false,
                .length = (int)strlen(loadingText),
                .chars = loadingText,
            };
            CLAY_TEXT(loadingStr, CLAY_TEXT_CONFIG(bodyFontCfg));
        }
    }
}

void AdaptiveAudio_RenderUI(UserContext* usr, AdaptiveAudioSystem* self)
{
    if (self->state != ADAPTIVE_DECIDING && self->state != ADAPTIVE_EXPORTING) {
        return;
    }

    // Use theme text configs
    Clay_TextElementConfig titleFontCfg = CLAY_THEME_TEXT_TITLE;
    Clay_TextElementConfig bodyFontCfg = CLAY_THEME_TEXT_BODY;
    Clay_TextElementConfig buttonFontCfg = CLAY_THEME_TEXT_BUTTON;
    
    // Full-screen overlay
    CLAY(
        CLAY_ID("AdaptiveOverlay"),
        CLAY_THEME_OVERLAY
    ) {
        // Modal window
        CLAY(
            CLAY_ID("AdaptiveModal"),
            {
                .layout = {
                    .sizing = {CLAY_SIZING_PERCENT(0.7f), CLAY_SIZING_FIT()},
                    .padding = {30, 30, 30, 30},
                    .childGap = 20,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
                .backgroundColor = CLAY_COLOR_PANEL_BG,
                .cornerRadius = {CLAY_RADIUS_XL, CLAY_RADIUS_XL, CLAY_RADIUS_XL, CLAY_RADIUS_XL},
            }
        ) {
            if (self->state == ADAPTIVE_DECIDING) {
                // Show options
                Clay_String fpsStr = {
                    .isStaticallyAllocated = false,
                    .length = (int)strlen(self->fpsMessage),
                    .chars = self->fpsMessage,
                };
                CLAY_TEXT(CLAY_STRING("Low Performance Detected"), CLAY_TEXT_CONFIG(titleFontCfg));
                CLAY_TEXT(fpsStr, CLAY_TEXT_CONFIG(bodyFontCfg));
                CLAY_TEXT(CLAY_STRING("Please choose an option:"), CLAY_TEXT_CONFIG(bodyFontCfg));
                
                // Buttons row
                CLAY(
                    CLAY_ID("AdaptiveButtons"),
                    {
                        .layout = {
                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                            .childGap = 15,
                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        },
                    }
                ) {
                    // Use Synth button
                    CLAY(
                        usr->useSynthClick.clayId,
                        CLAY_THEME_BTN_PRIMARY
                    ) {
                        CLAY_TEXT(CLAY_STRING("Use Synth"), CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_BUTTON));
                    }

                    // Use Cached button
                    CLAY(
                        usr->useWavClick.clayId,
                        CLAY_THEME_BTN_SUCCESS
                    ) {
                        CLAY_TEXT(CLAY_STRING("Use Cached"), CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_BUTTON));
                    }

                    // Disable Audio button
                    CLAY(
                        usr->disableAudioClick.clayId,
                        {
                            .layout = {
                                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(60)},
                                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                            },
                            .backgroundColor = CLAY_COLOR_BTN_DANGER,
                            .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
                        }
                    ) {
                        CLAY_TEXT(CLAY_STRING("Disable"), CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_BUTTON));
                    }
                }
                
                // Explanation text
                CLAY_TEXT(CLAY_STRING("Synth: Real-time OPN chip synthesis (no preload, more CPU)"),
                          CLAY_TEXT_CONFIG(bodyFontCfg));
                CLAY_TEXT(CLAY_STRING("Cached: Pre-generated audio blobs (needs caching, lighter on CPU)"),
                          CLAY_TEXT_CONFIG(bodyFontCfg));
            } else if (self->state == ADAPTIVE_EXPORTING) {
                // Show progress
                CLAY_TEXT(CLAY_STRING("Caching Audio..."), CLAY_TEXT_CONFIG(titleFontCfg));
                
                // Status text
                Clay_String statusStr = {
                    .isStaticallyAllocated = false,
                    .length = (int)strlen(self->exportStatus),
                    .chars = self->exportStatus,
                };
                CLAY_TEXT(statusStr, CLAY_TEXT_CONFIG(bodyFontCfg));
                
                // Progress bar background
                CLAY(
                    CLAY_ID("AdaptiveProgressBg"),
                    CLAY_THEME_PROGRESS_BAR_BG
                ) {
                    // Progress bar fill
                    float progress = self->exportProgress / 100.0f;
                    CLAY(
                        CLAY_ID("AdaptiveProgressFill"),
                        {
                            .layout = {
                                .sizing = {CLAY_SIZING_PERCENT(progress), CLAY_SIZING_GROW()},
                            },
                            .backgroundColor = CLAY_COLOR_PROGRESS_FILL,
                            .cornerRadius = {CLAY_RADIUS_SM, CLAY_RADIUS_SM, CLAY_RADIUS_SM, CLAY_RADIUS_SM},
                        }
                    ) {};
                }
                
                // Progress percentage text
                char progressText[128];
                int len = snprintf(progressText, sizeof(progressText), "Progress: %d%% (%.1fs / %.1fs)",
                                   self->exportProgress, self->exportedSeconds, self->exportTotalSeconds);
                Clay_String progressStr = {
                    .isStaticallyAllocated = false,
                    .length = len,
                    .chars = progressText,
                };
                CLAY_TEXT(progressStr, CLAY_TEXT_CONFIG(bodyFontCfg));
            }
        }
    }
}

bool AdaptiveAudio_ProcessEvent2(UserContext* usr, AdaptiveAudioSystem* self, SDL_Event event)
{
    if (self->state != ADAPTIVE_DECIDING) {
        return false;
    }
    
    bool mouseDown = event.type == SDL_MOUSEBUTTONDOWN;
    bool mouseUp = event.type == SDL_MOUSEBUTTONUP;
    
    if (!mouseDown && !mouseUp) {
        return false;
    }
    
    if (isClaytonClicked(&usr->useSynthClick, event)) {
        self->state = ADAPTIVE_RESTARTING;
        self->useWavMode = false;
        self->restartRequested = true;
        self->restartUseWav = false;
        self->showModal = false;
        printf("[AdaptiveAudio] User chose Synth mode - will restart sound system\n");
        return true;
    }
    
    if (isClaytonClicked(&usr->useWavClick, event)) {
        self->state = ADAPTIVE_RESTARTING;
        self->useWavMode = true;
        self->restartRequested = true;
        self->restartUseWav = true;
        self->showModal = false;

        printf("[AdaptiveAudio] User chose WAV mode - will restart sound system\n");
        return true;
    }
    
    if (isClaytonClicked(&usr->disableAudioClick, event)) {
        self->state = ADAPTIVE_DISABLED;
        self->audioDisabled = true;
        self->showModal = false;
        printf("[AdaptiveAudio] User disabled audio\n");
        return true;
    }
    
    // Consume events over the modal
    if (Clay_PointerOver(CLAY_ID("AdaptiveOverlay"))) {
        return true;
    }
    
    return false;
}
void vtx::loop(vtx::VertexContext *ctx)
{
    UserContext *usr = static_cast<UserContext *>(ctx->usrptr);

    usr->totalFrames += 1;
    
    // Update async sound system restart state machine (if in progress)
    usr->sound.updateRestart();
    
    bool shouldHandleResize = false;
    if (usr->totalFrames == 1)
    {
        usr->sound.initSoundSystem(SONG_01);
        initSoundSettings(usr, &usr->sound.settings, &usr->sound);

        AdaptiveAudio_Init(&usr->adaptiveAudio, 20.0f);  // Threshold

        initClaytonClick(&usr->useSynthClick, "adaptiveUseSynth");
        initClaytonClick(&usr->useWavClick, "adaptiveUseWav");
        initClaytonClick(&usr->disableAudioClick, "adaptiveDisableAudio");
    
        shouldHandleResize = true;
        std::cerr << "resize will be forced because it is first ever run" << std::endl;
    }

    // usr->phase= UserContext::Phase::THROW;
#ifndef __EMSCRIPTEN__
    if (true)
    {
        TimePoint now = Clock::now();
        Seconds dt = now - usr->last;
        const double targetDelta = 1.0 / 60.0;
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

    volatile uint64_t currentTime = SDL_GetTicks64(); // For simple stuff, in ms

    usr->auroraVibe.update(deltaTime);

    /* Step of adaptive audio loading - must be before rendering */ {

        // Update adaptive audio system
        AdaptiveAudioState prevState = usr->adaptiveAudio.state;
        AdaptiveAudio_Update(&usr->adaptiveAudio, deltaTime, usr->fpsCounter.fps);
        AdaptiveAudioState newState = usr->adaptiveAudio.state;

        // Handle volume muting during startup monitoring:
        // - MONITORING: mute sound (inaudible while measuring FPS)
        // - -> SYNTH transition: unmute (FPS is good)
        // - -> DECIDING transition: keep muted (show modal, user decides)
        static bool wasMutedForMonitoring = false;
        if (newState == ADAPTIVE_MONITORING) {
            if (!wasMutedForMonitoring) {
                // First frame of monitoring - mute sound
                usr->sound.musicVolume = 0.0f;
                usr->sound.sfxVolume = 0.0f;
                if (usr->sound.musicModule) xfm_module_set_volume(usr->sound.musicModule, 0.0f);
                if (usr->sound.sfxModule) xfm_module_set_volume(usr->sound.sfxModule, 0.0f);
                if (usr->sound.wavMusicModule) xfm_wav_module_set_volume(usr->sound.wavMusicModule, 0.0f);
                if (usr->sound.wavSfxModule) xfm_wav_module_set_volume(usr->sound.wavSfxModule, 0.0f);
                wasMutedForMonitoring = true;
            }
        } else if (newState == ADAPTIVE_SYNTH && prevState == ADAPTIVE_MONITORING) {
            // FPS is good - restore volume
            usr->sound.musicVolume = 0.5f;
            usr->sound.sfxVolume = 1.0f;
            if (usr->sound.musicModule) xfm_module_set_volume(usr->sound.musicModule, 0.5f);
            if (usr->sound.sfxModule) xfm_module_set_volume(usr->sound.sfxModule, 1.0f);
            wasMutedForMonitoring = false;
        } else if (newState == ADAPTIVE_DECIDING && prevState == ADAPTIVE_MONITORING) {
            // FPS is low - keep muted, modal will let user decide
            wasMutedForMonitoring = false;  // Reset so next monitoring cycle can mute again
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
        static int wavExportWaitFrames = 0;
        static const char *wavExportSongPattern = nullptr;
        static uint32_t wavExportResumeTime = 0; // SDL_GetTicks64() when to resume

        if (usr->sound.settings.needsWavExport)
        {
            usr->sound.settings.needsWavExport = false;
            printf("[SoundSettings] Triggering WAV export from sound settings...\n");

            // Determine which song to load after export
            switch (usr->sound.currentSongIndex)
            {
            case 1:
                wavExportSongPattern = SONG_01;
                break;
            case 2:
                wavExportSongPattern = SONG_02;
                break;
            case 3:
                wavExportSongPattern = SONG_03;
                break;
            case 4:
                wavExportSongPattern = SONG_04;
                break;
            default:
                wavExportSongPattern = SONG_01;
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
            wavExportWaitFrames = 10;
        }

        else if (wavExportState == WAV_EXPORT_PHASE1_WAIT1)
        {
            wavExportWaitFrames--;
            if (wavExportWaitFrames <= 0)
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
                wavExportResumeTime = SDL_GetTicks64() + 2000;
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
            if (now >= wavExportResumeTime)
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
                usr->sound.initSoundSystem(wavExportSongPattern);
                initSoundSettings(usr, &usr->sound.settings, &usr->sound);
            }
            else
            {
                usr->sound.initSoundSystem(SONG_01);
                initSoundSettings(usr, &usr->sound.settings, &usr->sound);
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
            initSoundSettings(usr, &usr->sound.settings, &usr->sound);

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
            initSoundSettings(usr, &usr->sound.settings, &usr->sound);

            usr->sound.settings.wavExportInProgress = false;
            usr->sound.settings.wavExportStatus[0] = '\0';
            adaptiveExportState = ADAPTIVE_EXPORT_IDLE;
        }
    }
    const uint32_t FONT_ID_BODY_24 = 0;

    bool mouseClicked = false;
    float screenRatio = static_cast<float>(ctx->screenWidth) / ctx->screenHeight;

    float ropeLength = 1.0f;

    glm::vec2 aimFlatMove = glm::vec2(0.0f);
    bool requestThrowEvent = false;
    SDL_Event e;

    UserContext::PhaseTrans phaseTrans = UserContext::PhaseTrans::TRANS_NONE;
    while (SDL_PollEvent(&e))
    {
#if TARGET_OS_IOS || TARGET_IPHONE_SIMULATOR
        switch (e.type)
        {
        case SDL_FINGERDOWN:
        {
            // Convert normalized touch position to window pixels
            int x = (int)(e.tfinger.x * ctx->screenWidth);
            int y = (int)(e.tfinger.y * ctx->screenHeight);

            SDL_Event mouse;
            mouse.type = SDL_MOUSEBUTTONDOWN;
            mouse.button.button = SDL_BUTTON_LEFT;
            mouse.button.state = SDL_PRESSED;
            mouse.button.x = x;
            mouse.button.y = y;
            SDL_PushEvent(&mouse); // inject as mouse event
            continue;
        }
        case SDL_FINGERUP:
        {
            int x = (int)(e.tfinger.x * ctx->screenWidth);
            int y = (int)(e.tfinger.y * ctx->screenHeight);

            SDL_Event mouse;
            mouse.type = SDL_MOUSEBUTTONUP;
            mouse.button.button = SDL_BUTTON_LEFT;
            mouse.button.state = SDL_RELEASED;
            mouse.button.x = x;
            mouse.button.y = y;
            SDL_PushEvent(&mouse);
            continue;
        }
        case SDL_FINGERMOTION:
        {
            int x = (int)(e.tfinger.x * ctx->screenWidth);
            int y = (int)(e.tfinger.y * ctx->screenHeight);

            SDL_Event mouse;
            mouse.type = SDL_MOUSEMOTION;
            mouse.motion.state = SDL_BUTTON_LMASK; // left button held
            mouse.motion.x = x;
            mouse.motion.y = y;
            SDL_PushEvent(&mouse);
            continue;
        }
        }
#endif

        float pixelRatio = ctx->pixelRatio;
#if TARGET_OS_MAC
        if (e.type == SDL_MOUSEBUTTONUP || e.type == SDL_MOUSEBUTTONDOWN ||
            e.type == SDL_MOUSEMOTION)
        {
            // Because of the previous hack for Mac
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
        usr->imgui.processEvent(&e);
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
            std::cerr << textScoreboard(usr->board) << std::endl;
            resetScoreboard(&usr->board);
            continue;
        }

        if (isClaytonClicked(&usr->renameButton, e))
        {
            usr->keypad.activated = true;
            uploadKeypadText(&usr->keypad);
            continue;
        }
        if (isClaytonClicked(&usr->menuButton, e))
        {
            usr->keypad.activated = true;
            uploadKeypadText(&usr->keypad);
            continue;
        }
        if (isClaytonClicked(&usr->soundButton, e))
        {
            usr->sound.showSoundSettings();
            continue;
        }
        if (isClaytonClicked(&usr->hiScoreButton, e))
        {
            usr->shouldShowHiScore = true;
            usr->shouldShowHiScoreWithLatest = false;
            continue;
        }
        // Skip other button clicks only if sound settings is not active
        // I want to understand what the logic
        if (!usr->sound.settings.activated && 
            (usr->renameButton.isDown || usr->replayButton.isDown || usr->menuButton.isDown || usr->soundButton.isDown || usr->hiScoreButton.isDown))
        {
            // ignore other event f button click started
            continue;
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

        bool isStolenBySoundSettings = processSoundSettingsEvent(usr, &usr->sound.settings, e);
        bool isStolenByAdaptiveAudio = false;

        if (isClaytonClicked(&usr->hiScoreCloseClick, e)) {
            usr->shouldShowHiScore = false;
            usr->shouldShowHiScoreWithLatest = false;
            continue;
        }

        // Those events from Low Performance detected window
        AdaptiveAudio_ProcessEvent2(usr, &usr->adaptiveAudio, e);
        bool isStolenByKeypad = processKeypadEvent(&usr->keypad, e, &usr->storage);
        if (isStolenByKeypad 
            || isStolenBySoundSettings 
            || isStolenByAdaptiveAudio)
        {
            continue;
        }
        if (isStolenByAdaptiveAudio && usr->adaptiveAudio.state == AdaptiveAudioState::ADAPTIVE_DECIDING)  {
            // We need to render if stolen
            usr->sound.settings.wavExportInProgress = true ;
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

                // I want to use this as well
                float x_rel = pixelRatio * static_cast<float>(e.motion.xrel) / ctx->screenWidth;
                float y_rel = pixelRatio * static_cast<float>(e.motion.yrel) / ctx->screenHeight;

                usr->aimFlatPos.x = x;
                usr->aimFlatPos.y = y;
                aimFlatMove.x = x_rel;
                aimFlatMove.y = y_rel;
            }
            if (e.type == SDL_MOUSEBUTTONUP)
            {
                // std::cerr << "let it go because of button up" << std::endl;

                requestThrowEvent = true;
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

                usr->aimFlatPos.x = x;
                usr->aimFlatPos.y = y;
                aimFlatMove.x = x_rel;
                aimFlatMove.y = y_rel;

                // That was unsuccesfull experiment trying to avoid acceleration
                // But maybe i will try again later
                // can you please add something here for mapping xrel and yrel so that
                // it matches the same scale usr->aimFlatPos.x += x_rel;
                // usr->aimFlatPos.y += y_rel; usr->aimFlatPos.x =
                // glm::clamp(usr->aimFlatPos.x, -1.0f, 1.0f); usr->aimFlatPos.y =
                // glm::clamp(usr->aimFlatPos.y, -1.0f, 1.0f);
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
                SDL_SetRelativeMouseMode(SDL_TRUE);
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
                aimFlatMove.x = x_rel;
                aimFlatMove.y = y_rel;
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
                float pivotMoveSpeed = 0.25f;
                usr->pivotPoint.x -= (movePivot * deltaTime * pivotMoveSpeed);
                usr->pivotPoint.x = glm::clamp(usr->pivotPoint.x, -pivotRail, pivotRail);
                usr->phy.change_pivot_point(usr->pivotPoint);
            }
        }
    }

    /* Stuff that updates joystick and spin circle */ {

        if (usr->phase == UserContext::Phase::IDLE)
        {
            usr->circle.resetCircle();
        }
        int sectors = usr->circle.moveCircle(aimFlatMove, deltaTime);
        if (usr->phase == UserContext::Phase::THROW || usr->phase == UserContext::Phase::SWING)
        {
            // std::cerr << "Sectors : " << sectors << std::endl;
            if (sectors > 2)
            {
                // TUNABLET: ANGULAR
                usr->phy.apply_angular_velocity_on_ball(usr->circle.direction * usr->angularFactor);
                usr->phy.set_spin_speed(usr->circle.direction * 0.05f * usr->smashingPower);
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

        if ((!userTriesToThrow) &&
            ((!wantsPhysics && physicsLongEnough) || (muchUpFront) ||
             usr->carriedBall.y > usr->pivotPoint.y)) // super complicated trans function
        {
            phaseTrans = UserContext::PhaseTrans::TRANS_SWING_TO_AIM;
        }
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
            }
            if (phaseTrans == UserContext::PhaseTrans::TRANS_AIM_TO_THROW)
            {
                usr->phase = UserContext::Phase::THROW;
                std::cerr << "AIM -> THROW" << std::endl;

                usr->phy.set_ball_free();
                usr->phy.enable_physics_on_ball();

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
        }

        if (usr->phase == UserContext::Phase::AIM)
        {

            usr->aimingTime += deltaTime;

            float pullX = usr->enjoy.ndc.x;
            float pullZ = usr->enjoy.ndc.y;
            // Adds hung
            float pullY = -sqrtf(1.01f * ropeLength * ropeLength - pullX * pullX - pullZ * pullZ);

            usr->desiredBall = glm::vec3(
                usr->pivotPoint.x + pullX, usr->pivotPoint.y + pullY, usr->pivotPoint.z + pullZ
            );

            glm::quat ySpin = glm::angleAxis(usr->totalSpinAngle, glm::vec3(0.0f, 1.0f, 0));

            /* Platrform equalizer: cap ball cary speed */ {

                float gee = 9.8f;
                float undesiredLen = glm::length(usr->undesiredMovement);
                if (undesiredLen > 0.001f)
                {
                    /* first make it stop the move it carried from physics */
                    float newLen = glm::max(0.0f, undesiredLen - gee * deltaTime);
                    usr->undesiredMovement *= newLen / undesiredLen;
                }
                else
                {
                    /* them make it return to desired position */
                    float catchupSpeed = 10.0f; // m/s max carry speed

                    glm::vec3 delta = usr->desiredBall - usr->carriedBall;
                    float dist = glm::length(delta);

                    if (dist > 0.0001f)
                    {
                        float maxStep = catchupSpeed * deltaTime;

                        if (maxStep >= dist)
                        {
                            usr->carriedBall = usr->desiredBall;
                        }
                        else
                        {
                            usr->carriedBall += delta * (maxStep / dist);
                        }
                    }
                }
            }

            ballModel = glm::translate(glm::mat4(1.0f), usr->carriedBall) * glm::mat4_cast(ySpin);

            usr->phy.set_manual_ball_position(usr->carriedBall, ySpin, deltaTime * 1.0f);
        }
        // usr->phy.enable_physics_on_ball();

        if (usr->phase == UserContext::Phase::SWING)
        {
            usr->swingingTime += deltaTime;

            float spin = usr->aimFlatPos.x * 20.0f;
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
                if (usr->auroraVibe.value >= 4.0f) {
                    usr->auroraVibe.value += 4.0f;
                }
                float start = usr->auroraVibe.value;
                usr->auroraVibe.start(start, start +1.0f, 1.5f);
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
            if (actualNumberOfBallsHit > usr->numberOfBallsHit) {
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

                    bool madeIt = LocalHi_SubmitScore(&usr->localHi , usr->username, usr->username_len, usr->board.totalScore);

                    if (madeIt) {
                        printf("🎉 New record %d! Rank #%d\n", usr->localHi.lastSubmittedScore, usr->localHi.lastSubmittedRank);
                    } else {
                        printf("You scored %d (%.1fth percentile)\n", 
                            usr->localHi.lastSubmittedScore, usr->localHi.lastSubmittedPercentile);
                    }

                    usr->shouldShowHiScore = true;
                    usr->shouldShowHiScoreWithLatest = true;
                }
                else
                {
                    usr->phase = UserContext::Phase::IDLE;
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
    usr->lastBallPosition = ballModel[3];

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
        if (usr->lastBallPosition.z > -1.0f)
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

    /* Gradually increase lane friction */ {
        float z = usr->lastBallPosition.z;
        constexpr float zStart = -18.3f;
        constexpr float zEnd = -5.0f;
        constexpr float maxFriction = 0.15f;

        float t = (z - zStart) / (zEnd - zStart);
        t = glm::clamp(t, 0.10f, 1.0f);
        float tq = t * t;

        usr->phy.apply_friction_to_lane(tq * maxFriction);
    }

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

    ZONE("3D render")
    {

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE); // Depth write if set

        glClearColor(0.1f, 0.2f, 0.1f, 1.0f);

        usr->auroraVibe.update(deltaTime);
            // usr->auroraVibe.value = 4.0f;
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

        usr->mainShader.renderRealMesh(
            usr->ballMesh, ballModel, usr->cameraMat, usr->perspectiveMat
        );
        usr->mainShader.renderRealMesh(
            usr->laneMesh,
            glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -.0f, .0f)),
            usr->cameraMat,
            usr->perspectiveMat
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

        ClayArena_Reset(&usr->clayArena);

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
        Clay_BeginLayout();

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
                    .backgroundColor = {255, 255, 255, 100},
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

                CLAY(
                    CLAY_ID("NotchArounds"),
                    CLAY_THEME_TOP_BAR
                )
                {
                    CLAY(
                        usr->renameButton.clayId,
                        CLAY_THEME_BTN_HUD
                    )
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
                    CLAY(
                        CLAY_ID("PlaceOfMoney"),
                        CLAY_THEME_BTN_HUD
                    )
                    {
                        CLAY_TEXT(CLAY_STRING("$ 20"), CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_BUTTON));
                    }
                }
                CLAY(
                    CLAY_ID("Content body"),
                    {.layout = {
                         .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                         .padding =
                             {portraitPadding, portraitPadding, portraitPadding, portraitPadding},
                         .layoutDirection = CLAY_TOP_TO_BOTTOM,
                     }}
                )
                {

                    // Scoreboard
                    usr->clayton.constructClayScoreboard(
                        &usr->board, scoreBoardWidth, usr->username, &usr->username_len
                    );

                    CLAY(
                        CLAY_ID("MenuAndShopRow"),
                        {.layout = {
                             .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                             .padding = {.top = portraitPadding, .bottom = portraitPadding},
                             .childGap = 10,
                             .childAlignment = {
                                 .x = CLAY_ALIGN_X_CENTER,
                                 .y = CLAY_ALIGN_Y_CENTER,
                             },
                         }}
                    )
                    {

                        CLAY(
                            usr->menuButton.clayId,
                            CLAY_THEME_BTN_HUD
                        )
                        {
                            CLAY_TEXT(CLAY_STRING("MENU"), CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_BUTTON));
                        }

                        // SOUND button next to MENU
                        CLAY(
                            usr->soundButton.clayId,
                            CLAY_THEME_BTN_HUD
                        )
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

                        CLAY(
                            usr->hiScoreButton.clayId,
                            CLAY_THEME_BTN_HUD
                        )
                        {
                            CLAY_TEXT(CLAY_STRING("HI-SCORE"), CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_BUTTON));
                        }

                        CLAY(
                            CLAY_ID("ShopButton"),
                            CLAY_THEME_BTN_HUD
                        )
                        {
                            CLAY_TEXT(CLAY_STRING("SHOP6"), CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_BUTTON));
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
                            CLAY(
                                usr->replayButton.clayId,
                                CLAY_THEME_BTN_SUCCESS
                            )
                            {
                                CLAY_TEXT(
                                    CLAY_STRING("PLAY"),
                                    CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_BUTTON)
                                );
                            }
                        }
                    }
                };
                CLAY_AUTO_ID(
                    {.layout =
                         {
                             .sizing = {.width = CLAY_SIZING_GROW(0)},
                             .padding = {10, 10, 3, 3},
                         },
                     .backgroundColor = {0, 0, 0, 100}}

                )
                {
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
                }

                if (usr->phase == UserContext::Phase::THROW)
                {

                    unsigned short halfTextH = 12;
                    Clay_Vector2 joystickOffset = {
                        0, ctx->screenHeight * 0.75f
                    }; // 1/4 bellow centre
                    CLAY(
                        CLAY_ID("FloatingOverJoystickContainer"),
                        {
                            .layout =
                                {
                                    .sizing =
                                        {.width = CLAY_SIZING_PERCENT(0.5),
                                         .height = CLAY_SIZING_PERCENT(0.125)},
                                    .childAlignment =
                                        {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
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
                            CLAY_ID(
                                "FloatingOverJoystickTextWrapper"
                            ), // wrap it in order to center
                            {
                                .layout =
                                    {
                                        .sizing =
                                            {.width = CLAY_SIZING_FIT(),
                                             .height = CLAY_SIZING_FIT()},
                                        .padding = {10, 10, 10, 10},
                                    },
                                .backgroundColor = {255, 1, 2, 100},
                            }
                        )
                        {
                            int joystickLabelLen;
                            if (usr->circle.progress == 0)
                            {
                                joystickLabelLen =
                                    snprintf(joystickLabel, sizeof(joystickLabel), "Spin\nto Hook");
                            }
                            else if (usr->circle.direction > 0)
                            {
                                joystickLabelLen = snprintf(
                                    joystickLabel,
                                    sizeof(joystickLabel),
                                    "Right %d",
                                    usr->circle.progress
                                );
                            }
                            else
                            {
                                joystickLabelLen = snprintf(
                                    joystickLabel,
                                    sizeof(joystickLabel),
                                    "Left %d",
                                    usr->circle.progress
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
                                    .fontId = 0,
                                    .fontSize = 16,
                                })
                            );
                        }
                    }
                }

                if (usr->keypad.activated)
                {
                    CLAY(
                        CLAY_ID("FloatinAndCoveringPortraitZone"),
                        {
                            .layout =
                                {
                                    .sizing =
                                        {.width = CLAY_SIZING_GROW(), .height = CLAY_SIZING_GROW()},
                                    .childAlignment =
                                        {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
                                },
                            .backgroundColor = {0, 0, 0, 100},
                            .floating = {
                                .offset = {0},
                                .zIndex = 1,
                                .attachPoints =
                                    {CLAY_ATTACH_POINT_CENTER_CENTER,
                                     CLAY_ATTACH_POINT_CENTER_CENTER},
                                .attachTo = CLAY_ATTACH_TO_PARENT,
                            },
                        }
                    )
                    {
                        buildKeypadClay(&usr->keypad);
                    }
                }

                // Sound settings panel (separate from keypad)
                if (usr->sound.settings.activated && !usr->sound.settings.wavExportInProgress)
                {
                    CLAY(
                        CLAY_ID("FloatinAndCoveringPortraitZone"),
                        {
                            .layout =
                                {
                                    .sizing =
                                        {.width = CLAY_SIZING_GROW(), .height = CLAY_SIZING_GROW()},
                                    .childAlignment =
                                        {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
                                },
                            .backgroundColor = {0, 0, 0, 100},
                            .floating = {
                                .offset = {0},
                                .zIndex = 2,
                                .attachPoints =
                                    {CLAY_ATTACH_POINT_CENTER_CENTER,
                                     CLAY_ATTACH_POINT_CENTER_CENTER},
                                .attachTo = CLAY_ATTACH_TO_PARENT,
                            },
                        }
                    )
                    {
                        buildSoundSettingsClay(usr, &usr->sound.settings);
                    }
                }


                if (usr->shouldShowHiScore == true) {

                    CLAY(
                        CLAY_ID("FloatinAndCoveringPortraitZone"),
                        {
                            .layout =
                                {
                                    .sizing =
                                        {.width = CLAY_SIZING_GROW(), .height = CLAY_SIZING_GROW()},
                                    .childAlignment =
                                        {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
                                },
                            .backgroundColor = {0, 0, 0, 100},
                            .floating = {
                                .offset = {0},
                                .zIndex = 2,
                                .attachPoints =
                                    {CLAY_ATTACH_POINT_CENTER_CENTER,
                                     CLAY_ATTACH_POINT_CENTER_CENTER},
                                .attachTo = CLAY_ATTACH_TO_PARENT,
                            },
                        }
                    )
                    {
                        buildHiScoreClay(usr, &usr->localHi);
                    }
                }
 
                // Render adaptive audio modal
                if (
                    // usr->sound.settings.wavExportInProgress == true ||
                    usr->adaptiveAudio.showModal || 
                    usr->adaptiveAudio.state == ADAPTIVE_EXPORTING) {
                    CLAY(
                        CLAY_ID("AdaptiveAudioContainer"),
                        {
                            .layout =
                                {
                                    .sizing =
                                        {.width = CLAY_SIZING_GROW(), .height = CLAY_SIZING_GROW()},
                                    .childAlignment =
                                        {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
                                },
                            .backgroundColor = {0, 0, 0, 0}, // Transparent
                            .floating = {
                                .offset = {0},
                                .zIndex = 3,
                                .attachPoints =
                                    {CLAY_ATTACH_POINT_CENTER_CENTER,
                                     CLAY_ATTACH_POINT_CENTER_CENTER},
                                .attachTo = CLAY_ATTACH_TO_PARENT,
                            },
                        }
                    )
                    {
                        AdaptiveAudio_RenderUI(usr, &usr->adaptiveAudio);
                    }
                }

                // Render WAV export loading indicator
                if (usr->sound.settings.wavExportInProgress) {
                    CLAY(
                        CLAY_ID("WavExportContainer"),
                        {
                            .layout =
                                {
                                    .sizing =
                                        {.width = CLAY_SIZING_GROW(), .height = CLAY_SIZING_GROW()},
                                    .childAlignment =
                                        {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
                                },
                            .backgroundColor = {0, 0, 0, 0}, // Transparent
                            .floating = {
                                .offset = {0},
                                .zIndex = 4,  // Above other modals
                                .attachPoints =
                                    {CLAY_ATTACH_POINT_CENTER_CENTER,
                                     CLAY_ATTACH_POINT_CENTER_CENTER},
                                .attachTo = CLAY_ATTACH_TO_PARENT,
                            },
                        }
                    )
                    {
                        // buildWavExportLoadingIndicator(&usr->sound.settings, 
                        //     usr->adaptiveAudio.exportProgress, 
                        //     usr->adaptiveAudio.exportedSeconds, 
                        //     usr->adaptiveAudio.exportTotalSeconds,
                        //     usr->sound.sampleRate);
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
                    .backgroundColor = {255, 255, 255, 100},
                }
            ){};
        }

        Clay_RenderCommandArray cmds = Clay_EndLayout();

        usr->clayton.renderClayton(cmds, ctx->screenWidth, ctx->screenHeight, deltaTime);

        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
    }

    bool isGugucas = (usr->username_len == 7 && memcmp(usr->username, "GUGUCAS", 7) == 0);
    usr->shouldShowImgui = isGugucas;
    if (usr->shouldShowImgui)
    {
        usr->imgui.beginImgui();

        ImGui::Begin("Stygavimui");

        // check TUNABLET
        ImGui::SliderFloat("Rankos Jega", &usr->speedBoostAtThrow, 0.5f, 5.0f);
        ImGui::SliderFloat("Trenksmas", &usr->smashingPower, 5.0f, 50.0f);
        ImGui::SliderFloat("Sukimas+", &usr->angularFactor, 0.1f, 1.0f);
        ImGui::SliderFloat("Mase", &usr->desiredMass, 1.0f, 20.0f);
        if (ImGui::Button("Keisti mase"))
        {
            usr->phy.set_ball_mass(usr->desiredMass);
        }

        ImGui::End(); // Stygavimui end

        ImGui::Begin("Jerunda");
        ImGui::Text(
            "FPS: %.0f (%.0dx%.0d)", usr->fpsCounter.fps, ctx->screenWidth, ctx->screenHeight
        );
        ImGui::Text("yFacotr: %.3f", yFactor);
        ImGui::Text("Rolling time: %.3f", usr->throwingTime);
        ImGui::Text("Settling time: %.3f", usr->settlingTime);

        ImGui::Text("Spin speed: %.3f", usr->spinSpeed);
        // ImGui::Text("Launch speed: %.3f", usr->launchSpeed);
        ImGui::Text("End speed: %.3f", usr->endSpeed);

        if (usr->phase == UserContext::Phase::AIM)
        {
            ImGui::Text("pos left right: %.3f", usr->aimStart.x);
        }
        ImGui::End(); // Jerunda end

        if (usr->phase != UserContext::Phase::RESULT)
        {
            ImGui::SetNextWindowCollapsed(true, ImGuiCond_Once);
            ImGui::Begin("Score details");
            ImGui::Text("%s", textScoreboard(usr->board).c_str());
            ImGui::End();

            ImGui::Begin("Score");
            ImGui::Text("%s", textCompactScoreboardImproved(&usr->board).c_str());
            ImGui::End();
        }

        if (usr->phase == UserContext::Phase::RESULT)
        {
            ImGui::Begin("Score Final");
            ImGui::Text("%s", textCompactScoreboardImproved(&usr->board).c_str());
            ImGui::Text("%s", textScoreboard(usr->board).c_str());
            if (ImGui::Button("\n Restart \n"))
            {
                usr->phase = UserContext::Phase::IDLE;
                std::cerr << textScoreboard(usr->board) << std::endl;
                resetScoreboard(&usr->board);
            }
            ImGui::End();
        }

        usr->imgui.endImgui();
    }

    usr->fpsCounter.endFrame();
    

    SDL_GL_SwapWindow(ctx->sdlWindow);
}
