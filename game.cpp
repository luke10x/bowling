#include <chrono>
#include <cstdint>
#include <iostream>
#include <stdio.h>
#include <thread>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "framework/boot.h"

#include "all_assets.h"
#include "aurora.h"
#include "circlegest.h"
#include "clayton.h"
#include "fpscounter.h"
#include "hooker.h"
#include "joystick.h"
#include "mesh.h"
#include "mod_imgui.h"
#include "physics/physics.h"
#include "score.h"
#include "transition.h"
#include "window.h"

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
    Phase phase = Phase::IDLE;
    glm::vec3 aimStart;
    glm::vec3 aimCurr;

    bool fuckCakez = true;
    Aurora aurora;
    FpsCounter fpsCounter;
    uint64_t lastFrameTime = 0;
    TimePoint last = Clock::now();
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
    usr->circle.loadCircleShaderProgram();
    usr->clayton.initClayton(ctx->screenWidth, ctx->screenHeight);
}

// Convert array of Vertex to flat float array of positions
// Vertex must have: glm::vec3 position
static std::vector<float> extractPositions(const Vertex *verts, size_t count)
{
    std::vector<float> out;
    out.reserve(count * 3);

    for (size_t i = 0; i < count * 3; ++i)
    {
        out.push_back(verts[i].position.x);
        out.push_back(verts[i].position.y);
        out.push_back(verts[i].position.z);
    }

    return out;
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
    usr->everythingTexture.loadTextureFromFile("assets/files/everything_tex.png");
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

    auto lanePositions = extractPositions(laneMd.vertices, laneMd.vertexCount);

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
        laneMd.indices, laneMd.indexCount, usr->initialPins, usr->ballStart
    );

    usr->phase = UserContext::Phase::IDLE;
    resetScoreboard(usr->board);

    usr->clayton.initClayton(ctx->screenWidth, ctx->screenHeight);

    usr->shouldShowClayDebug = false;
    usr->shouldShowImgui = false;
    Clay_SetDebugModeEnabled(usr->shouldShowClayDebug);

    usr->enjoy.resetJoystick();
    usr->enjoy.initDefaultFlatShaderProgram();
    usr->circle.resetCircle();
    usr->circle.initCircleThing();
}

void vtx::loop(vtx::VertexContext *ctx)
{
    UserContext *usr = static_cast<UserContext *>(ctx->usrptr);

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

    const uint32_t FONT_ID_BODY_24 = 0;

    bool mouseClicked = false;
    float screenRatio = static_cast<float>(ctx->screenWidth) / ctx->screenHeight;
        
    float ropeLength = 1.0f;


    glm::vec2 aimFlatMove = glm::vec2(0.0f);
    bool requestThrowEvent = false;
    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        if (e.type == SDL_QUIT)
            ctx->shouldContinue = false;

        usr->clayton.processClaytonEvent(&e, deltaTime);
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

        if (usr->phase == UserContext::Phase::IDLE)
        {

            usr->aimFlatPos = glm::vec2(0.0f);

            if (e.type == SDL_MOUSEBUTTONDOWN)
            {
                mouseClicked = true;
                usr->phase = UserContext::Phase::AIM;
                float x = ctx->pixelRatio * static_cast<float>(e.button.x) / ctx->screenWidth;
                float y = ctx->pixelRatio * static_cast<float>(e.button.y) / ctx->screenHeight;


                usr->aimFlatPos.x = x;
                usr->aimFlatPos.y = y;

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
        }
        else if (usr->phase == UserContext::Phase::AIM)
        {
            if (e.type == SDL_MOUSEMOTION)
            {
                // I used to have:
                float x = ctx->pixelRatio * static_cast<float>(e.motion.x) / ctx->screenWidth;
                float y = ctx->pixelRatio * static_cast<float>(e.motion.y) / ctx->screenHeight;

                // I want to use this as well
                float x_rel =
                    ctx->pixelRatio * static_cast<float>(e.motion.xrel) / ctx->screenWidth;
                float y_rel =
                    ctx->pixelRatio * static_cast<float>(e.motion.yrel) / ctx->screenHeight;

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
        else if (usr->phase == UserContext::Phase::SWING) {
            // std::cerr << "Waiting for button up while swing" << std::endl;
            if (e.type == SDL_MOUSEMOTION)
            {
                // I used to have:
                float x = ctx->pixelRatio * static_cast<float>(e.motion.x) / ctx->screenWidth;
                float y = ctx->pixelRatio * static_cast<float>(e.motion.y) / ctx->screenHeight;

                // I want to use this as well
                float x_rel =
                    ctx->pixelRatio * static_cast<float>(e.motion.xrel) / ctx->screenWidth;
                float y_rel =
                    ctx->pixelRatio * static_cast<float>(e.motion.yrel) / ctx->screenHeight;

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
            if (e.type == SDL_MOUSEMOTION)
            {
                // I used to have:
                float x = ctx->pixelRatio * static_cast<float>(e.motion.x) / ctx->screenWidth;
                float y = ctx->pixelRatio * static_cast<float>(e.motion.y) / ctx->screenHeight;

                // I want to use this as well
                float x_rel =
                    ctx->pixelRatio * static_cast<float>(e.motion.xrel) / ctx->screenWidth;
                float y_rel =
                    ctx->pixelRatio * static_cast<float>(e.motion.yrel) / ctx->screenHeight;

                usr->aimFlatPos.x = x;
                usr->aimFlatPos.y = y;
                aimFlatMove.x = x_rel;
                aimFlatMove.y = y_rel;
            }
            if (e.type == SDL_MOUSEBUTTONDOWN)
            {
                // if (currentTime > usr->lastThrowTime + 1'000)
                // {
                //     usr->phy.physics_reset(
                //         usr->initialPins,
                //         usr->ballStart,
                //         true);
                //     usr->wereDead = 0;
                //     usr->phase = UserContext::Phase::IDLE;
                // }
                // else
                // {
                //     std::cerr << "peep" << std::endl;
                // }
            }
        }
        // else if (usr->phase == UserContext::Phase::RESULT)
        // {
        //     // int pinsKnockedDown = 3;
        //     // addRoll(&usr->board, pinsKnockedDown);
        //     // computeScore(&usr->board);
        // }

        if (handle_resize_sdl(ctx, e))
        {
            // Recalculate perspective
            float fov = glm::radians(60.0f); // Field of view in radians
            float aspectRatio = (float)ctx->screenWidth / (float)ctx->screenHeight;
            float nearPlane = 0.50f;
            float farPlane = 30.0f;
            usr->perspectiveMat = glm::perspective(fov, aspectRatio, nearPlane, farPlane);
        }
    }

    float TUNE = 200.0f;
    int movePivot = 0;
    if (usr->aimFlatPos.x < 0.1f) {
        movePivot = -1;
    } else 
    if (usr->aimFlatPos.x > 0.9f) {
        movePivot = +1;
    } 

    if (usr->phase == UserContext::Phase::AIM)
    {
        float pivotRail = 0.45f;
        if (movePivot != 0) {
            if (usr->pivotPoint.x >= -pivotRail && usr->pivotPoint.x <= pivotRail) {
                float pivotMoveSpeed = 0.25f;
                usr->pivotPoint.x -= (movePivot * deltaTime * pivotMoveSpeed);
                usr->pivotPoint.x = glm::clamp(usr->pivotPoint.x, -pivotRail, pivotRail);
                usr->phy.change_pivot_point(usr->pivotPoint);
            }
        }
    }

    /* Stuff that updates joystick and spin circle */ {

        if (usr->phase == UserContext::Phase::IDLE) {
            usr->circle.resetCircle();
        }
        int sectors = usr->circle.moveCircle(aimFlatMove, deltaTime);
        if (usr->phase == UserContext::Phase::THROW)
        {
            // std::cerr << "Sectors : " << sectors << std::endl;
            if (sectors > 2) {
                usr->phy.apply_angular_velocity_on_ball(usr->circle.direction * 2.0f);
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

    float yFactor = 0.0f;
    glm::mat4 ballModel;
    /* Put ballmodel */ {
        if (usr->phase == UserContext::Phase::IDLE)
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

            ballModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.2f, -18.0f));

            // Apply translation
            ballModel = glm::translate(ballModel, glm::vec3(0.0f, yOffset, 0.0f));

            // Apply rotation
            ballModel = glm::rotate(ballModel, rotation, glm::vec3(0.0f, 1.0f, 0.0f));

            usr->carriedBall = ballModel[3];
        }

        // float aimProlongation = (screenRatio < 0.0f ? screenRatio : 1.0f);
        if (usr->phase == UserContext::Phase::AIM)
        {
            // Init AIM phase
            if (usr->aimingTime == 0.0f) {

                // usr->phy.set_ball_hanging(usr->pivotPoint, usr->carriedBall);
                // usr->phy.enable_physics_on_ball();
            }
            usr->aimingTime += deltaTime;

            bool aimingLongEnough = usr->aimingTime > 0.6f;
            bool wantsPhysics = usr->trans.wantsPhysics(usr->enjoy.ndc, deltaTime);
            if (wantsPhysics && aimingLongEnough && movePivot == 0) {
                std::cerr << "-> SWING " << usr->trans.mWantsPhysics << std::endl;
                usr->phase = UserContext::Phase::SWING;
                usr->swingingTime = 0.0f;
                usr->highestPoint = -10.0f;
            }

            float pullX = usr->enjoy.ndc.x;
            float pullZ = usr->enjoy.ndc.y;
            // Adds hung
            float pullY = - sqrtf(1.01f * ropeLength * ropeLength - pullX * pullX - pullZ * pullZ);

            usr->desiredBall = glm::vec3(
                    usr->pivotPoint.x + pullX,
                    usr->pivotPoint.y + pullY,
                    usr->pivotPoint.z + pullZ
                );

            glm::quat ySpin = glm::angleAxis(usr->totalSpinAngle, glm::vec3(0.0f, 1.0f, 0));


            // Not really setting spinSpeed here
            // usr->phy.set_spin_speed(usr->spinSpeed);
            // On one way we want ball that moves at max speed
            /* Platrform equalizer: cap ball cary speed */ {

                float gee = 9.8f;
                float undesiredLen = glm::length(usr->undesiredMovement);
                if (undesiredLen > 0.001f)
                {
                    /* first make it stop the move it carried from physics */
                    float newLen = glm::max(0.0f, undesiredLen - gee * deltaTime);
                    usr->undesiredMovement *= newLen / undesiredLen;
                } else {
                    /* them make it return to desired position */
                    float catchupSpeed = 10.0f; // m/s max carry speed

                    glm::vec3 delta = usr->desiredBall - usr->carriedBall;
                    float dist = glm::length(delta);

                    if (dist > 0.0001f)
                    {
                        float maxStep = catchupSpeed * deltaTime;

                        if (maxStep >= dist) {
                            usr->carriedBall = usr->desiredBall;
                        } else {
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
            // Init SWING phase
            if (usr->swingingTime == 0.0f) {
                usr->phy.set_ball_hanging(usr->pivotPoint, usr->carriedBall);
                usr->phy.enable_physics_on_ball();
                usr->undesiredMovement = glm::vec3(0.0f);
            }
            usr->swingingTime += deltaTime;


            float spin = usr->aimFlatPos.x * 20.0f;
            //  std::cerr << "SPIN2 " << spin << std::endl
            // usr->phy.apply_angular_velocity_on_ball(spin);

            // Only first time swinging will enable this
            ballModel = usr->phy.physics_get_ball_matrix();
            glm::vec3 before = usr->carriedBall;
            usr->carriedBall = ballModel[3]; //
            glm::vec3 after = usr->carriedBall;
            glm::vec3 potentiallyUndesiredMovement = after - before; // save how much moved by physics

            glm::vec3 ballPos = ballModel[3];
            bool muchUp = ballPos.y > usr->pivotPoint.y + 0.2f;
            bool muchFwd = ballPos.z > usr->pivotPoint.z + 0.8f;
            bool muchUpFront = muchUp + muchFwd;
            bool physicsLongEnough = usr->swingingTime > 0.4f;
            bool wantsPhysics = usr->trans.wantsPhysics(usr->enjoy.ndc, deltaTime);
            if ((!(requestThrowEvent || usr->bufferedRequestThrow)) && ((!wantsPhysics && physicsLongEnough) || (muchUpFront))) {
                std::cerr << "-> BACK to HOlD " << usr->trans.mWantsPhysics << std::endl;
                usr->aimingTime = 0.0f;
                usr->phase = UserContext::Phase::AIM;
                usr->swingingTime = 0.0f;
                usr->highestPoint = -10.0f;
                usr->carriedBall = ballModel[3];
                usr->undesiredMovement = potentiallyUndesiredMovement;
            }
        }

        if (requestThrowEvent || usr->bufferedRequestThrow) {
            // Do not release if the ball is pulled behind, let it swing at least to pivot point 
            bool safeToRelease = ballModel[3].z > usr->pivotPoint.z;
            if (!safeToRelease ) {
                if (!usr->bufferedRequestThrow) {
                    usr->bufferedRequestThrow = true;

                    if (usr->phase == UserContext::Phase::AIM) 
                    {
                        usr->phy.enable_physics_on_ball(); // Olnly required when throws directly from aim
                    } else {
                    }
                    usr->phase = UserContext::Phase::SWING;
                    usr->swingingTime = 0.0f;
                    usr->highestPoint = -10.0f;
                }
            } else {
                usr->phase = UserContext::Phase::THROW;
                usr->bufferedRequestThrow = false;

                usr->phy.set_ball_free();

                if (usr->phase == UserContext::Phase::AIM) 
                {
                    usr->phy.enable_physics_on_ball(); // Olnly required when throws directly from aim
                } else {
                }
                SDL_SetRelativeMouseMode(SDL_FALSE);

                usr->throwingTime = 0.0f;
                usr->settlingTime = 0.0f;
            }
        }
        if (usr->phase == UserContext::Phase::THROW)
        {
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

                usr->phy.physics_reset(usr->initialPins, usr->ballStart, shouldResetAllPins);

                if (isGameFinished(&usr->board))
                {
                    usr->phase = UserContext::Phase::RESULT;
                }
                else
                {
                    usr->phase = UserContext::Phase::IDLE;
                }
            }
        }
    }
    usr->phy.physics_step(deltaTime * 1.0f);

    usr->lastBallPosition = ballModel[3];

    /* Change lane friction */ {
        float z = usr->lastBallPosition.z;
        constexpr float zStart = -18.3f;
        constexpr float zEnd   = -5.0f;
        constexpr float maxFriction = 0.35f;

        float t = (z - zStart) / (zEnd - zStart);
        t = glm::clamp(t, 0.10f, 1.0f);
        float tq = t * t;


        usr->phy.apply_friction_to_lane(tq * maxFriction);
    }

    usr->cameraMat = glm::lookAt(
        glm::vec3(
            0.0f, 0.8f,
            glm::clamp(
                ballModel[3].z - 3.0f, -21.0f,
                -2.0f
            )
        ), // eye in before of the ball
        glm::vec3(
            0.0f, -1.0f,
            glm::clamp(
                ballModel[3].z + 4.5f, -12.0f,
                2.0f
            )
        ),                          // target after
        glm::vec3(0.0f, 1.0f, 0.0f) // up
    );
    usr->cameraMat[3][0] = usr->pivotPoint.x;

    SDL_GL_GetDrawableSize(ctx->sdlWindow, &ctx->screenWidth, &ctx->screenHeight);

    /* 3D render zone */ {

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE); // Depth write if set

        glClearColor(0.1f, 0.2f, 0.1f, 1.0f);

        usr->aurora.renderAurora(
            deltaTime * TUNE,
            glm::inverse(usr->cameraMat)
        ); //  * projectionMatrix);

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
        }

        usr->mainShader.renderRealMesh(
            usr->ballMesh, ballModel, usr->cameraMat, usr->perspectiveMat
        );
        usr->mainShader.renderRealMesh(
            usr->laneMesh, glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -.0f, .0f)),
            usr->cameraMat, usr->perspectiveMat
        );

        {
            const glm::vec3 eye = glm::vec3(4.0f);
            const glm::vec3 center = glm::vec3(0.0f);
            const glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

            usr->cameraMat = glm::lookAt(eye, center, up);
        }
        glm::mat4 m = glm::mat4(3.0f);
        
        if (usr->phase < UserContext::Phase::THROW) {
            usr->enjoy.renderJoystick(ctx->screenWidth, ctx->screenHeight);
            usr->circle.resetCircle();
        } else if (usr->phase == UserContext::Phase::THROW) {
            usr->circle.renderCircle(ctx->screenWidth, ctx->screenHeight);
        } else {
            usr->enjoy.resetJoystick();
        }
    }

    /* Clay zone */ {
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE); // Clay is simple and never writes to depth buffer

        float portraitWidth = ctx->screenWidth;
        float portraitHeight = ctx->screenHeight;
        float ratio = portraitWidth / portraitHeight;

        float goldenConstant
            // = 9.0f / 16.0f;
            = 480.0f / 720.0f;
        if (ratio > goldenConstant)
        {
            portraitWidth = portraitHeight * goldenConstant;
        }

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

            uint16_t portraitPadding = 10;
            CLAY(
                CLAY_ID("Portrait area"),
                {
                    .layout = {
                        .sizing{
                            .width = CLAY_SIZING_FIXED(portraitWidth),
                            .height = CLAY_SIZING_FIXED(portraitHeight),
                        },
                        .padding = { 0, 0, portraitPadding, 0},
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
                // Scoreboard
                usr->clayton.constructClayScoreboard(&usr->board, portraitWidth - portraitPadding * 2);

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
                            CLAY_ID("PlayButton"),
                            {
                                .layout =
                                    {
                                        .sizing = {CLAY_SIZING_FIXED(200), CLAY_SIZING_FIXED(60)},
                                        .childAlignment =
                                            {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                    },
                                .backgroundColor = {40, 160, 240, 255},
                                .cornerRadius = {12, 12, 12, 12},
                            }
                        )
                        {
                            CLAY_TEXT(
                                CLAY_STRING("PLAY"),
                                CLAY_TEXT_CONFIG({
                                    .textColor = {255, 255, 255, 255},
                                    .fontId = 0,
                                    .fontSize = 28,
                                })
                            );
                        }
                    };
                }

                CLAY_AUTO_ID(
                    {.layout =
                         {
                             .sizing = {.width = CLAY_SIZING_GROW(0)},
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
                        .textColor = {255, 255, 255, 255},
                        .fontId = FONT_ID_BODY_24,
                        .fontSize = 16,
                    };
                    CLAY_TEXT(cs, &fpsElementConfig);
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
                            // .backgroundColor = {255, 255, 255, 100},
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
                            else if (usr->circle.progress > 0)
                            {
                                joystickLabelLen = snprintf(
                                    joystickLabel, sizeof(joystickLabel), "Right +%d",
                                    usr->circle.progress
                                );
                            }
                            else
                            {
                                joystickLabelLen = snprintf(
                                    joystickLabel, sizeof(joystickLabel), "Left +%d",
                                    -usr->circle.progress
                                );
                            }
                            Clay_String cs = {
                                .isStaticallyAllocated = false,
                                .length = joystickLabelLen,
                                .chars = joystickLabel
                            };
                            Clay_TextElementConfig textConfig = {
                                .textColor = {255, 255, 255, 255},
                                .fontId = FONT_ID_BODY_24,
                                .fontSize = 16,
                            };
                            CLAY_TEXT(cs, &textConfig);
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
                    .backgroundColor = {255, 255, 255, 100},
                }
            ){};
        }
        if (mouseClicked && Clay_PointerOver(CLAY_ID("PlayButton")))
        {
            usr->phase = UserContext::Phase::IDLE;
            std::cerr << textScoreboard(usr->board) << std::endl;
            resetScoreboard(usr->board);
        }

        Clay_RenderCommandArray cmds = Clay_EndLayout();

        usr->clayton.renderClayton(
            cmds, ctx->pixelRatio, ctx->screenWidth, ctx->screenHeight, deltaTime
        );

        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
    }

    if (usr->shouldShowImgui)
    {
        usr->imgui.beginImgui();

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
                resetScoreboard(usr->board);
            }
            ImGui::End();
        }

        usr->imgui.endImgui();
    }

    usr->fpsCounter.endFrame();
    SDL_GL_SwapWindow(ctx->sdlWindow);
}
