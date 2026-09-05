#pragma once

#include "framework/gl_header.h"
#include <glm/glm.hpp>
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include <vector>
#include <iostream>
#include <cmath>
#include "framework/boot.h"
#include "framework/gl_util.h"
#include <algorithm>

struct RenderTexture {
    GLuint fbo = 0;
    GLuint colorTexture = 0;
    GLuint depthRenderbuffer = 0; // Optional, but recommended for proper occlusion
    GLsizei width = 256;
    GLsizei height = 256;

    void renderTextureInit(bool withDepth = true, const char *debugName = "RenderTexture") {
        if (colorTexture != 0)
            glDeleteTextures(1, &colorTexture);
        if (depthRenderbuffer != 0)
            glDeleteRenderbuffers(1, &depthRenderbuffer);
        if (fbo != 0)
            glDeleteFramebuffers(1, &fbo);
        colorTexture = 0;
        depthRenderbuffer = 0;
        fbo = 0;

        // 1. Create color texture
        glGenTextures(1, &colorTexture);
        glBindTexture(GL_TEXTURE_2D, colorTexture);
#if defined(__APPLE__) && defined(TARGET_OS_IOS) && TARGET_OS_IOS
        const GLint colorInternalFormat = GL_RGBA;
#else
        const GLint colorInternalFormat = GL_RGBA8;
#endif
        glTexImage2D(GL_TEXTURE_2D, 0, colorInternalFormat, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // 2. Create FBO
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);

        // 3. Optional depth buffer
        if (withDepth) {
            glGenRenderbuffers(1, &depthRenderbuffer);
            glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
#if defined(__APPLE__) && defined(TARGET_OS_IOS) && TARGET_OS_IOS
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
#else
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
#endif
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);
        }

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            printf("%s FBO incomplete: 0x%x (%dx%d depth=%d)\n", debugName, status, width, height, withDepth ? 1 : 0);
        }
        bindDefaultOpenGLFramebuffer();
    }

    void ensureSize(GLsizei newWidth, GLsizei newHeight, bool withDepth = true) {
        newWidth = std::max<GLsizei>(1, newWidth);
        newHeight = std::max<GLsizei>(1, newHeight);
        if (fbo != 0 && colorTexture != 0 && width == newWidth && height == newHeight)
            return;
        width = newWidth;
        height = newHeight;
        renderTextureInit(withDepth);
    }

    // Bind + set viewport for rendering INTO this texture
    void bindForWriting() {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, width, height);
    }

    // Unbind to return to default framebuffer
    void unbind(int screenWidth, int screenHeight) {
        bindDefaultOpenGLFramebuffer();
        // Restore main viewport if needed:
        glViewport(0, 0, screenWidth, screenHeight);
    }
    // Bind texture for SAMPLING (e.g., in UI or in-world shader)
    void bindForReading(GLuint textureUnit = 0) {
        glActiveTexture(GL_TEXTURE0 + textureUnit);
        glBindTexture(GL_TEXTURE_2D, colorTexture);
    }
};
