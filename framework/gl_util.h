#pragma once

#include "gl_header.h"

// ****************************
//  1. OpenGL shader subsystem
// ****************************

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

namespace vtx
{
    GLuint createShaderProgram(
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

}

// ****************************
//  2. GLSL Helpers
// ****************************

// ****************************
//  3. OpenGL diagnostic utils
// ****************************

inline GLuint gVtxDefaultFramebuffer = 0;
inline GLuint gVtxDefaultRenderbuffer = 0;

static void setDefaultOpenGLFramebuffer(GLuint framebuffer, GLuint renderbuffer = 0)
{
    gVtxDefaultFramebuffer = framebuffer;
    gVtxDefaultRenderbuffer = renderbuffer;
}

static void bindDefaultOpenGLFramebuffer()
{
    glBindFramebuffer(GL_FRAMEBUFFER, gVtxDefaultFramebuffer);
    if (gVtxDefaultRenderbuffer != 0)
        glBindRenderbuffer(GL_RENDERBUFFER, gVtxDefaultRenderbuffer);
}

static void checkOpenGLError(const char *optionalTag = "")
{
    bool found = false;
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
    {
        GLint framebuffer = 0;
        GLint viewport[4] = {};
        GLenum framebufferStatus = 0;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &framebuffer);
        glGetIntegerv(GL_VIEWPORT, viewport);
        if (err == GL_INVALID_FRAMEBUFFER_OPERATION)
            framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);

        std::cerr << "OpenGL error optionalTag=" << optionalTag << " errorCode=" << err;
        if (err == GL_INVALID_FRAMEBUFFER_OPERATION)
            std::cerr << " framebuffer=" << framebuffer
                      << " framebufferStatus=0x" << std::hex << framebufferStatus << std::dec
                      << " viewport=" << viewport[0] << "," << viewport[1]
                      << "," << viewport[2] << "," << viewport[3];
        std::cerr << std::endl;

        found = true;
    }
    if (found)
    {
        // vtx::exitVortex();
        abort();
    }
}
