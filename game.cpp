#include <chrono>
#include <iostream>
#include <thread>
#include <cstdint>

#include "framework/boot.h"

#include "aurora.h"
#include "fpscounter.h"
#include "mod_imgui.h"
#include "mesh.h"
#include "physics/physics.h"
#include "all_assets.h"
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
    uint64_t lastThrowTime = 0;
    TimePoint last = Clock::now();
    ModImgui imgui;

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
    // Enables blending, which allows transparent textures to be rendered properly.
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // Sets the blending function.
    // - `GL_SRC_ALPHA`: Uses the alpha value of the source (texture or color).
    // - `GL_ONE_MINUS_SRC_ALPHA`: Makes the destination color blend with the background based on alpha.
    // This is commonly used for standard transparency effects.
    glEnable(GL_DEPTH_TEST);
    // Enables depth testing, ensuring that objects closer to the camera are drawn in front of those farther away.
    // This prevents objects from rendering incorrectly based on draw order.

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
        float fov = glm::radians(60.0f); // Field of view in radians
        float aspectRatio = (float)ctx->screenWidth / (float)ctx->screenHeight;
        float nearPlane = 0.50f;
        float farPlane = 30.0f;
        usr->perspectiveMat = glm::perspective(fov, aspectRatio, nearPlane, farPlane);
    }

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
        const float l0 = -1.0f + ft * glm::cos(glm::radians(30.0f));
        usr->initialPins[0] = glm::vec3(-0.0f, h, -1.0f);

        const float l1 = -1.0f + ft * glm::cos(glm::radians(30.0f));
        usr->initialPins[1] = glm::vec3(-0.5f * ft, h, l1);
        usr->initialPins[2] = glm::vec3(+0.5f * ft, h, l1);

        const float l2 = -1.0 + 2.0f * ft * glm::cos(glm::radians(30.0f));
        usr->initialPins[3] = glm::vec3(-ft, h, l2);
        usr->initialPins[4] = glm::vec3(-0.0f * ft, h, l2);
        usr->initialPins[5] = glm::vec3(+ft, h, l2);

        const float l3 = -1.0 + 3.0f * ft * glm::cos(glm::radians(30.0f));
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
        usr->ballStart);

    usr->phase = UserContext::Phase::IDLE;
}

void vtx::loop(vtx::VertexContext *ctx)
{
    UserContext *usr = static_cast<UserContext *>(ctx->usrptr);

    volatile uint64_t currentTime = SDL_GetTicks64();
    float deltaTime = (currentTime - usr->lastFrameTime) / 1000.0f;

#ifndef __EMSCRIPTEN__
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

    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        if (e.type == SDL_QUIT)
            ctx->shouldContinue = false;
        usr->imgui.processEvent(&e);
        if (e.type == SDL_KEYDOWN)
        {
            if (
                e.key.keysym.sym == SDLK_F5 || e.key.keysym.sym == SDLK_SPACE)
            {
                usr->phy.physics_reset(
                    usr->initialPins,
                    usr->ballStart);
                usr->phase = UserContext::Phase::IDLE;
            }
        }
        if (usr->phase == UserContext::Phase::IDLE)
        {
            if (e.type == SDL_MOUSEBUTTONDOWN)
            {
                usr->phase = UserContext::Phase::AIM;
                float x = ctx->pixelRatio * static_cast<float>(e.button.x) / ctx->screenWidth;
                float y = ctx->pixelRatio * static_cast<float>(e.button.y) / ctx->screenHeight;
                // Map click coordinates to start of aim point
                usr->aimStart = glm::vec3(
                    0.5f + (-x), // notice x is inverted because we are at the back
                    0.0f,
                    -0.3f);
                usr->aimCurr = usr->aimStart;

                SDL_SetRelativeMouseMode(SDL_TRUE);
            }
        }
        else if (usr->phase == UserContext::Phase::AIM)
        {
            if (e.type == SDL_MOUSEMOTION)
            {
                float x = ctx->pixelRatio * static_cast<float>(e.motion.x) / ctx->screenWidth;
                float y = ctx->pixelRatio * static_cast<float>(e.motion.y) / ctx->screenHeight;

                usr->aimCurr = usr->aimStart + glm::vec3(
                                                   0.5f + (-x), // notice x is inverted because we are at the back
                                                   0.0 + 1.0f * (y - 0.5f) * (y - 0.5f),
                                                   (0.8f + (-y)) * 2.0f);
            }
            if (e.type == SDL_MOUSEBUTTONUP)
            {
                usr->phase = UserContext::Phase::THROW;
                SDL_SetRelativeMouseMode(SDL_FALSE);

                usr->phy.enable_physics_on_ball();
                usr->lastThrowTime = currentTime;
            }
        }
        else if (usr->phase == UserContext::Phase::THROW) {
            if (e.type == SDL_MOUSEBUTTONDOWN)
            {
                if (currentTime > usr->lastThrowTime + 2'000) {
                    usr->phy.physics_reset(
                        usr->initialPins,
                        usr->ballStart);
                    usr->phase = UserContext::Phase::IDLE;
                } else {
                    std::cerr << "peep" << std::endl;
                }
            }
        }

        handle_resize_sdl(ctx, e);
    }


    float TUNE = 200.0f;

    glm::mat4 ballModel;
    /* Put ballmodel */ {
        if (usr->phase == UserContext::Phase::IDLE)
        {
            ballModel = glm::translate(
                glm::mat4(1.0f), glm::vec3(0.0f, 0.2f, -18.0f));
        }
        if (usr->phase == UserContext::Phase::AIM)
        {
            glm::vec3 start = glm::vec3(0.0f, 0.2f, -18.0f);

            glm::vec3 carriedBall = start + usr->aimCurr;
            ballModel = glm::translate(
                glm::mat4(1.0f),
                carriedBall);
            usr->phy.set_manual_ball_position(carriedBall, deltaTime * 1.0f);
        }
        if (usr->phase == UserContext::Phase::THROW)
        {
            ballModel = usr->phy.physics_get_ball_matrix();
        }
    }
    usr->phy.physics_step(deltaTime * 1.0f);

    // if (usr->phase ==) {}
    usr->cameraMat = glm::lookAt(
        glm::vec3(0.0f, 0.8f, glm::clamp(ballModel[3].z - 3.0f, -20.0f, -2.0f)), // eye in before of the ball
        glm::vec3(0.0f, -1.0f, glm::clamp(ballModel[3].z + 4.5f, -12.0f, 2.0f)),   // target after 
        glm::vec3(0.0f, 1.0f, 0.0f)    // up
    );

    /* render */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.1f, 0.2f, 0.1f, 1.0f);
    usr->aurora.renderAurora(deltaTime * TUNE, glm::inverse(usr->cameraMat)); //  * projectionMatrix);

    usr->mainShader.updateLightPos(glm::vec3(3.0f, 3.0f, usr->cameraMat[3].z + 6.0f));
    usr->mainShader.updateDiffuseTexture(usr->everythingTexture);
    usr->mainShader.updateTextureParamsInOneGo(
        glm::vec3(1.0f, 1.0f, 1.0f), // Texture density
        glm::vec2(1.0f, 1.0f),       // Size of one tile compared to full atlas
        glm::vec2(1.0f),             // Atlas region start
        1.0f                         // Atlas region scale compared to entire atlas
    );

    for (int i = 0; i < 10; i++)
    {
        glm::mat4 pinModel = usr->phy.physics_get_pin_matrix(i);
        float halfHeight = 0.19f;
        pinModel = glm::translate(pinModel, glm::vec3(0.0f, -halfHeight, 0.0f));
        usr->mainShader.renderRealMesh(
            usr->pinMesh,
            pinModel,
            usr->cameraMat,
            usr->perspectiveMat);
    }

    usr->mainShader.renderRealMesh(
        usr->ballMesh,
        ballModel,
        usr->cameraMat,
        usr->perspectiveMat);
    usr->mainShader.renderRealMesh(
        usr->laneMesh,
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -.0f, .0f)),
        usr->cameraMat,
        usr->perspectiveMat);

    {
        const glm::vec3 eye = glm::vec3(4.0f);
        const glm::vec3 center = glm::vec3(0.0f);
        const glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

        usr->cameraMat = glm::lookAt(eye, center, up);
    }
    glm::mat4 m = glm::mat4(3.0f);
    usr->fpsCounter.updateFpsCounter(deltaTime);

    usr->imgui.beginImgui();

    ImGui::Begin("Plugin UI");

    ImGui::Text("FPS: %.0f", usr->fpsCounter.fps);
    ImGui::Text("Ball pos: %.3f, %.3f, %.3f",
                ballModel[3].x,
                ballModel[3].y,
                ballModel[3].z);
    if (usr->phase == UserContext::Phase::AIM)
    {
        ImGui::Text("pos left right: %.3f", usr->aimStart.x);
    }

    ImGui::End();

    usr->imgui.endImgui();

    SDL_GL_SwapWindow(ctx->sdlWindow);

    usr->lastFrameTime = currentTime;
}
