#pragma once

#include "framework/glhead.h"

#include <SDL.h>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include <vector>
#include <iostream>

#include "framework/ctx.h"

struct Aurora
{
    static const char *AURORA_VERTEX_SHADER;
    static const char *AURORA_FRAGMENT_SHADER;

    GLuint auroraVAO;
    GLuint auroraShaderId;
    float time;

    void initAurora()
    {
        // Set time to a point where it needs to regenerate
        this->time = 3.0f;

        this->auroraShaderId = vtx::createShaderProgram(
            AURORA_VERTEX_SHADER, AURORA_FRAGMENT_SHADER);

        const GLfloat fullscreenQuadVertices[] = {
            -1.0f, -1.0f, 1.0f, 0.0f, 0.0f,
            1.0f, -1.0f, 0.998f, 1.0f, 0.0f,
            -1.0f, 1.0f, 0.998f, 0.0f, 1.0f,
            1.0f, 1.0f, 0.998f, 1.0f, 1.0f};

        const GLuint fullscreenQuadIndices[] = {
            0, 1, 2,
            1, 3, 2};

        GLuint vbo, ebo;
        glGenVertexArrays(1, &this->auroraVAO);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(this->auroraVAO);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(fullscreenQuadVertices), fullscreenQuadVertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(fullscreenQuadIndices), fullscreenQuadIndices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void *)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void *)(3 * sizeof(GLfloat)));

        glBindVertexArray(0);
        

        checkOpenGLError();
    }

    void renderAurora(
        float deltaTime,
        const glm::mat4 cameraMatrix)
    {
        glUseProgram(this->auroraShaderId);

        // Extract the forward vector from the view matrix
        glm::vec3 forward = glm::normalize(
            glm::vec3(
                cameraMatrix[0][2],
                cameraMatrix[1][2],
                cameraMatrix[2][2]));

        // Calculate yaw and pitch angles
        float yaw = atan2(forward.x, forward.z) * 5.0f; // Yaw affects x-axis
        float pitch = asin((forward.y + 1.0f) * 0.5f);  // Pitch affects y-axis

        // Pass yaw and pitch as uniforms
        glUniform1f(glGetUniformLocation(this->auroraShaderId, "uYaw"), yaw);
        glUniform1f(glGetUniformLocation(this->auroraShaderId, "uPitch"), pitch);

        this->time += deltaTime;

        glUniform1f(glGetUniformLocation(
                        this->auroraShaderId,
                        "uTime"),
                    this->time);

        glBindVertexArray(this->auroraVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
};

const char *Aurora::AURORA_VERTEX_SHADER =
    GLSL_VERSION
    R"(
    precision highp float;

    layout(location = 0) in vec3 aPos;
    layout(location = 1) in vec2 aTexCoord;

    out vec2 TexCoord;

    void main() {

        TexCoord = aTexCoord;
        gl_Position = vec4(aPos, 1.0); // Fullscreen quad in NDC space
    }
    )";

const char *Aurora::AURORA_FRAGMENT_SHADER =
    GLSL_VERSION
    R"(
    precision highp float;

    in vec2 TexCoord;

    out vec4 FragColor;

    uniform float uYaw;   // Yaw angle in radians (range: [-π, π])
    uniform float uPitch; // Pitch angle in radians (range: [-π/2, π/2])
    uniform float uTime;  // Time for animation

    // Simplex noise function
    vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
    vec2 mod289(vec2 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
    vec3 permute(vec3 x) { return mod289(((x * 34.0) + 1.0) * x); }
    float snoise(vec2 v) {
        const vec4 C = vec4(0.211324865405187, 0.366025403784439, -0.577350269189626, 0.024390243902439);
        vec2 i = floor(v + dot(v, C.yy));
        vec2 x0 = v - i + dot(i, C.xx);
        vec2 i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
        vec4 x12 = x0.xyxy + C.xxzz;
        x12.xy -= i1;
        i = mod289(i);
        vec3 p = permute(permute(i.y + vec3(0.0, i1.y, 1.0)) + i.x + vec3(0.0, i1.x, 1.0));
        vec3 m = max(0.5 - vec3(dot(x0, x0), dot(x12.xy, x12.xy), dot(x12.zw, x12.zw)), 0.0);
        m = m * m;
        m = m * m;
        vec3 x = 2.0 * fract(p * C.www) - 1.0;
        vec3 h = abs(x) - 0.5;
        vec3 ox = floor(x + 0.5);
        vec3 a0 = x - ox;
        m *= 1.79284291400159 - 0.85373472095314 * (a0 * a0 + h * h);
        vec3 g;
        g.x = a0.x * x0.x + h.x * x0.y;
        g.yz = a0.yz * x12.xz + h.yz * x12.yw;
        return 130.0 * dot(m, g);
    }

    void main() {
        // Adjust texture coordinates based on yaw
        float yawNormalized = uYaw / 3.14159; // Normalize yaw to [-1, 1]
        float yawSpeedFactor = abs(yawNormalized); // Speed increases with |yaw|
        float yawOffset = yawNormalized * yawSpeedFactor; // Faster movement for larger |yaw|

        // Adjust texture coordinates based on pitch and time
        float pitchNormalized = (uPitch + 1.5708) / 3.14159; // Normalize pitch to [0, 1]
        float pitchSpeedFactor = 2.0; // Speed increases with this factor
        float pitchOffset = (pitchNormalized - 0.5) * 2.0 * pitchSpeedFactor; // Map pitch to [-1, 1]
        float timeOffset = uTime * 0.005; // Time-based scrolling

        vec2 uv = TexCoord + vec2(yawOffset, pitchOffset + timeOffset); // Move texture based on yaw, pitch, and time

        // Generate noise for the aurora effect
        float noise1 = snoise(uv * 3.0) * 0.5;
        float noise2 = snoise(uv * 7.0 + vec2(uTime * 0.01, 0.0)) * 0.3;
        float noise3 = snoise(uv * 15.0 + vec2(uTime * 0.02, 0.0)) * 0.2;
        float intensity = clamp(noise1 + noise2 + noise3, 0.0, 1.0);

        // Aurora color blending
        vec3 auroraColor1 = vec3(0.0, 0.2, 0.6); // Blue
        vec3 auroraColor2 = vec3(0.1, 0.8, 0.3); // Green
        vec3 auroraColor = mix(auroraColor1, auroraColor2, intensity);

        // Adjust opacity based on pitch
        pitchNormalized = (uPitch + 1.5708) / 3.14159; // Normalize pitch to [0, 1]
        float opacity = pitchNormalized;

        // Combine color and opacity
        FragColor = vec4(auroraColor, opacity);
    }
    )";
