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

static void printShaderVersions()
{
    // Get OpenGL version
    const GLubyte *glVersion = glGetString(GL_VERSION);
    printf("OpenGL Version       : %s\n", glVersion);
    // Get GLSL (shader language) version
    const GLubyte *glslVersion =
        glGetString(GL_SHADING_LANGUAGE_VERSION);
    printf("GLSL (Shader) Version: %s\n", glslVersion);
    // You can also get OpenGL vendor and renderer information
    const GLubyte *vendor = glGetString(GL_VENDOR);
    const GLubyte *renderer = glGetString(GL_RENDERER);
    printf("Vendor               : %s\n", vendor);
    printf("Renderer             : %s\n", renderer);
#if defined(__ANDROID__) || defined(ANDROID)
#else
    GLint maxVertexUniformComponents;
    GLint maxVertexUniformVectors;
    GLint maxUniformBlockSize;
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &maxVertexUniformVectors);
    // glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUniformBlockSize);
    printf("GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB     : %d\n", maxVertexUniformComponents); // * 4 bytes
    printf("GL_MAX_VERTEX_UNIFORM_VECTORS            : %d\n", maxVertexUniformVectors);    // * 4 bytes
    // printf("GL_MAX_UNIFORM_BLOCK_SIZE                : %d\n", maxUniformBlockSize);        // * 4 bytes
#endif
}

static void checkOpenGLError(const char *optionalTag = "")
{
    bool found = false;
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
    {
        std::cerr << "OpenGL error optionalTag=" << optionalTag << " errorCode=" << err
                  << std::endl;

        found = true;
    }
    if (found)
    {
        // vtx::exitVortex();
        abort();
    }
}
