#include "loop.h"

#include <iostream>
#include <thread>
#include <chrono>


#include "glm/glm.hpp"

#ifdef HOT_RELOAD
extern "C" void init(void *ctxptr)
{
    std::cerr << " Entered dynamically loaded INIT " << ctxptr << std::endl;
    vtx::VertexContext *ctx = static_cast<vtx::VertexContext *>(ctxptr);
#else
void vtx::init(vtx::VertexContext *ctx)
{
#endif
    ctx->usrptr = malloc(sizeof(UserContext));
    ctx->usrptr = new UserContext;
    UserContext *usr = static_cast<UserContext *>(ctx->usrptr);
    std::cerr << "Init done" << std::endl;

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

    usr->aurora.initAurora();
}

#ifdef HOT_RELOAD
extern "C" void loop(void *ctxptr)
{
    std::cerr << " Entered dynamically loaded loop " << ctxptr << std::endl;
    vtx::VertexContext *ctx = static_cast<vtx::VertexContext *>(ctxptr);
#else
void vtx::loop(vtx::VertexContext *ctx)
{
#endif
    if (ctx->usrptr == nullptr)
    {
        std::cerr << "User ptr is null" << std::endl;
        exit(1);
    }
    UserContext *usr = static_cast<UserContext *>(ctx->usrptr);
    if (usr == nullptr)
    {
        std::cerr << "User is null" << std::endl;
        exit(1);
    }
    SDL_Event e;

    while (SDL_PollEvent(&e))
    {
        if (e.type == SDL_QUIT)
            ctx->shouldContinue = false;
    }
    // glViewport(0, 0, usr->depthMap.shadowWidth, usr->depthMap.shadowHeight);
    // glBindFrame<buffer(GL_FRAMEBUFFER, usr->depthMap.depthMapFBO);
    // glClear(GL_DEPTH_BUFFER_BIT);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.1f, 0.2f, 0.1f, 1.0f);

    // ⭐️ Opaque phase
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE); // write to depth

    // At start of  framebuffer rendering
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // Depth test always ON for 3D
    glEnable(GL_DEPTH_TEST);

    // restore state after semi tansparent rendering
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);


    volatile uint32_t currentTime = SDL_GetTicks();
    float deltaTime = (currentTime - usr->lastFrameTime) / 1000.0f;

        glm::mat4 cameraMatrix = glm::lookAt(
            glm::vec3(0.0f),
            glm::vec3(0.0f, 0.0f, 1.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        usr->aurora.renderAurora(deltaTime * 0.5f, glm::inverse(cameraMatrix)); //  * projectionMatrix);
    // std::cerr << " Aurora renders " << ctx << std::endl;
        checkOpenGLError();

        SDL_GL_SwapWindow(ctx->sdlWindow);
    // Sleep for 3 seconds
    // std::this_thread::sleep_for(std::chrono::seconds(10));
    usr->lastFrameTime = currentTime;
}

// ****************************
//  2. OpenGL shader subsystem
// ****************************

GLuint vtx::createShaderProgram(
    const char *vertexShaderSource,
    const char *fragmentShaderSource)
{
    GLuint vertexShader =
        compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader =
        compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

static GLuint compileShader(GLenum type, const char *source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);

        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n"
                  << "SHADER SOURCE:\n"
                  << source << "\nSHADER TYPE: "
                  << (type == GL_VERTEX_SHADER ? "Vertex Shader" : "")
                  << (type == GL_FRAGMENT_SHADER ? "Fragment Shader"
                                                 : "")
                  << "\nSHADER COMPILATION ERROR:\n"
                  << infoLog << std::endl;

        // vtx::exitVortex(1);
	    exit(1);
    }
    return shader;
}

