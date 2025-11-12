#include "framework/ctx.h"

#include "aurora.h"
#include "fpscounter.h"
#include "mod_imgui.h"

struct UserContext
{
    bool fuckCakez = true;
    Aurora aurora;
    FpsCounter fpsCounter;
    uint64_t lastFrameTime = 0;
    ModImgui imgui;

};
void vtx::hang(vtx::VertexContext *ctx) {

    UserContext *usr = static_cast<UserContext *>(ctx->usrptr);
    usr->imgui.hangImgui(ctx);
}

void vtx::load(vtx::VertexContext *ctx) {
    UserContext *usr = static_cast<UserContext *>(ctx->usrptr);

    usr->imgui.loadImgui(ctx);
    usr->aurora.loadAuroraShader();
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
}

void vtx::loop(vtx::VertexContext *ctx)
{
    UserContext *usr = static_cast<UserContext *>(ctx->usrptr);

    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        if (e.type == SDL_QUIT)
            ctx->shouldContinue = false;
        usr->imgui.processEvent(&e);
    }

    volatile uint64_t currentTime = SDL_GetTicks64();
    float deltaTime = (currentTime - usr->lastFrameTime) / 1000.0f;

        glm::mat4 cameraMatrix = glm::lookAt(
            glm::vec3(0.0f),
            glm::vec3(0.0f, 0.0f, 1.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

    float TUNE = 20.0f;

    /* render */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.1f, 0.2f, 0.1f, 1.0f);
    usr->aurora.renderAurora(deltaTime * TUNE, glm::inverse(cameraMatrix)); //  * projectionMatrix);

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
