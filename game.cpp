#include <chrono>
#include <iostream>
#include <thread>
#include <cstdint>

#include "framework/boot.h"

#include "aurora.h"
#include "fpscounter.h"
#include "mod_imgui.h"
#include "mesh.h"
#include "physics.h"
#include "all_assets.h"

using Clock = std::chrono::high_resolution_clock;
using TimePoint = std::chrono::time_point<Clock>;
using Seconds = std::chrono::duration<double>;

struct UserContext
{
    bool fuckCakez = true;
    Aurora aurora;
    FpsCounter fpsCounter;
    uint64_t lastFrameTime = 0;
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
};

void vtx::hang(vtx::VertexContext *ctx) {
    UserContext *usr = static_cast<UserContext *>(ctx->usrptr);
    usr->imgui.hangImgui(ctx);
    // TODO I guess it is leaking memory, but I can live with that in dev build
}

void vtx::load(vtx::VertexContext *ctx) {
    UserContext *usr = static_cast<UserContext *>(ctx->usrptr);

    usr->imgui.loadImgui(ctx);
    usr->aurora.loadAuroraShader();
}
// Convert array of Vertex to flat float array of positions
// Vertex must have: glm::vec3 position
static std::vector<float> extractPositions(const Vertex* verts, size_t count)
{
    std::vector<float> out;
    out.reserve(count * 3);

    for (size_t i = 0; i < count*3; ++i)
    {
        out.push_back(verts[i].position.x);
        out.push_back(verts[i].position.y);
        out.push_back(verts[i].position.z);
    }

    return out;
}

glm::vec3 pinStart = glm::vec3(0.0f, 0.35f, -1.0f);
glm::vec3 ballStart = glm::vec3(0.0f, 0.5f, -6.8f);
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
        float nearPlane = 1.50f; 
        float farPlane = 120.0f;
        usr->perspectiveMat = glm::perspective(fov, aspectRatio, nearPlane, farPlane);
    }

    {
        const glm::vec3 eye = glm::vec3(4.0f);
        const glm::vec3 center = glm::vec3(0.0f);
        const glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

        usr->cameraMat = glm::lookAt(eye, center, up);
    }

        // glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.5f, -1.5f)),
        // glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.5f, -1.0f)),
    auto lanePositions = extractPositions(laneMd.vertices, laneMd.vertexCount);

    usr->phy.physics_init(
        lanePositions.data(), // number of floats
        lanePositions.size(), // number of floats
        laneMd.indices,
        laneMd.indexCount,
        pinStart,
        ballStart
    );
}

void vtx::loop(vtx::VertexContext *ctx)
{
    UserContext *usr = static_cast<UserContext *>(ctx->usrptr);

#ifndef __EMSCRIPTEN__
    {
        TimePoint now = Clock::now();
        Seconds dt = now - usr->last;
        const double targetDelta = 1.0 / 300.0;
        if (dt.count() < targetDelta) {
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
        if (e.type == SDL_KEYDOWN) {
            if (e.key.keysym.sym == SDLK_F5)
            {
                usr->phy.physics_reset(
                    pinStart,
                    ballStart
                );
            }
        }
    }

    volatile uint64_t currentTime = SDL_GetTicks64();
    float deltaTime = (currentTime - usr->lastFrameTime) / 1000.0f;

    const double targetDelta = 1.0 / 60.0; // 60 FPS


    glm::mat4 cameraMatrix = glm::lookAt(
        glm::vec3(0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    float TUNE = 200.0f;

    usr->phy.physics_step(deltaTime * 1.0f);

    glm::mat4 ballModel = usr->phy.physics_get_ball_matrix();
    glm::mat4 pinModel  = usr->phy.physics_get_pin_matrix();

    /* render */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.1f, 0.2f, 0.1f, 1.0f);
    usr->aurora.renderAurora(deltaTime * TUNE, glm::inverse(cameraMatrix)); //  * projectionMatrix);

    usr->mainShader.updateDiffuseTexture(usr->everythingTexture);
    usr->mainShader.updateTextureParamsInOneGo(
        glm::vec3(1.0f, 1.0f, 1.0f), // Texture density
        glm::vec2(1.0f, 1.0f),       // Size of one tile compared to full atlas
        glm::vec2(1.0f),             // Atlas region start
        1.0f                         // Atlas region scale compared to entire atlas
    );

    usr->mainShader.renderRealMesh(
        usr->pinMesh,
        pinModel,
        usr->cameraMat,
        usr->perspectiveMat
    );
    usr->mainShader.renderRealMesh(
        usr->ballMesh,
        ballModel,
        usr->cameraMat,
        usr->perspectiveMat
    );
    usr->mainShader.renderRealMesh(
        usr->laneMesh,
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -.0f, .0f)),
        usr->cameraMat,
        usr->perspectiveMat
    );

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
    ImGui::Text("Hot reload is aliveidd !");
    ImGui::End();

    usr->imgui.endImgui();

    SDL_GL_SwapWindow(ctx->sdlWindow);

    usr->lastFrameTime = currentTime;
}
