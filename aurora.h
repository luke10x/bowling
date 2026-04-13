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

struct Aurora
{
    static const char *AURORA_VERTEX_SHADER;
    static const char *AURORA_FRAGMENT_SHADER;

    GLuint auroraVAO;
    GLuint auroraShaderId;
    float time;

    void initAurora()
    {
        this->time = 3.0f;
        this->loadAuroraShader();

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

    void loadAuroraShader() {
        this->auroraShaderId = vtx::createShaderProgram(
            AURORA_VERTEX_SHADER, AURORA_FRAGMENT_SHADER);
    }

    void hangAuroraShader() {
        // Cleanup if needed
    }

    /**
     * @param animationMode: Float parameter controlling which animation style to display
     *   - 0.0 = Original aurora borealis (cloud-like noise)
     *   - 1.0 = Balatro-style hypnotic swirl (geometric flowing patterns) [[23]]
     *   - 2.0 = Starfield/nebula (particle-based cosmic effect)
     *   - Integer values = pure style, float values = weighted mix toward nearest integers
     *   - Example: 0.5 = 50% aurora + 50% Balatro, 1.3 = 70% Balatro + 30% starfield
     */
    void renderAurora(
        float deltaTime,
        const glm::mat4 cameraMatrix,
        float animationMode = 0.0f)  // NEW PARAMETER
    {
        glUseProgram(this->auroraShaderId);

        glm::vec3 forward = glm::normalize(
            glm::vec3(
                cameraMatrix[0][2],
                cameraMatrix[1][2],
                cameraMatrix[2][2]));

        float yaw = atan2(forward.x, forward.z) * 5.0f;
        float pitch = asin((forward.y + 1.0f) * 0.5f);

        glUniform1f(glGetUniformLocation(this->auroraShaderId, "uYaw"), yaw);
        glUniform1f(glGetUniformLocation(this->auroraShaderId, "uPitch"), pitch);

        this->time += 1.0f * deltaTime;
        glUniform1f(glGetUniformLocation(this->auroraShaderId, "uTime"), this->time);
        
        // NEW: Pass animation mode to shader
        glUniform1f(glGetUniformLocation(this->auroraShaderId, "uAnimationMode"), animationMode);

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
        gl_Position = vec4(aPos, 1.0);
    }
    )";

const char *Aurora::AURORA_FRAGMENT_SHADER =
    GLSL_VERSION
    R"(
    precision highp float;
    in vec2 TexCoord;
    out vec4 FragColor;
    uniform float uYaw;
    uniform float uPitch;
    uniform float uTime;
    uniform float uAnimationMode;  // NEW: Controls which animation style to use

    // ========== SHARED UTILITIES ==========
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
        m = m * m; m = m * m;
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

    // Reusable color palette (aurora-inspired, reused across styles)
    vec3 getPaletteColor(float t) {
        // Smooth gradient through aurora colors: deep blue → cyan → green → purple
        vec3 c1 = vec3(0.0, 0.2, 0.5);   // Deep blue
        vec3 c2 = vec3(0.0, 0.8, 0.9);   // Cyan
        vec3 c3 = vec3(0.3, 1.0, 0.4);   // Green
        vec3 c4 = vec3(0.7, 0.3, 1.0);   // Purple
        t = fract(t);  // Wrap around
        if (t < 0.33) return mix(c1, c2, t * 3.0);
        if (t < 0.66) return mix(c2, c3, (t - 0.33) * 3.0);
        return mix(c3, c4, (t - 0.66) * 3.0);
    }

    // ========== STYLE 0: ORIGINAL AURORA (Cloud-like noise) ==========
    vec3 renderAuroraStyle(vec2 uv, float time, float yaw, float pitch) {
        float yawNormalized = yaw / 3.14159;
        float yawSpeedFactor = abs(yawNormalized);
        float yawOffset = yawNormalized * yawSpeedFactor;
        float pitchNormalized = (pitch + 1.5708) / 3.14159;
        float pitchOffset = (pitchNormalized - 0.5) * 2.0 * 2.0;
        float timeOffset = time * 0.0005;
        vec2 coord = uv + vec2(yawOffset, pitchOffset + timeOffset);

        float noise1 = snoise(coord * 3.0) * 0.5;
        float noise2 = snoise(coord * 7.0 + vec2(time * 0.01, 0.0)) * 0.3;
        float noise3 = snoise(coord * 15.0 + vec2(time * 0.02, 0.0)) * 0.2;
        float intensity = clamp(noise1 + noise2 + noise3, 0.0, 1.0);

        vec3 auroraColor1 = vec3(sin(uv.x + uv.y + time * 0.001), 0.2, 0.3);
        vec3 auroraColor2 = vec3(0.9, sin(uv.y + time * 0.0005), 0.5);
        return mix(auroraColor1, auroraColor2, intensity);
    }

    // ========== STYLE 1: BALATRO-STYLE SWIRL (Hypnotic geometric flow) ==========
    // Based on Balatro's distinctive animated background with swirling patterns [[23]][[31]]
    vec3 renderBalatroStyle(vec2 uv, float time, float yaw, float pitch) {
        vec2 p = uv * 2.0 - 1.0;  // Center coordinates
        float aspect = 1.0;  // Adjust if needed for non-square
        
        // Hypnotic swirl using polar coordinates
        float angle = atan(p.y, p.x);
        float radius = length(p);
        
        // Time-based rotation with camera influence
        float swirlSpeed = 0.3 + abs(yaw) * 0.1;
        float swirl = angle + time * swirlSpeed + radius * 5.0;
        
        // Layered flowing bands (Balatro's signature look)
        float band1 = sin(radius * 12.0 - time * 0.5 + swirl * 0.3) * 0.5 + 0.5;
        float band2 = sin(radius * 8.0 + time * 0.3 - swirl * 0.5) * 0.5 + 0.5;
        float band3 = sin((angle + time * 0.2) * 6.0 + radius * 3.0) * 0.5 + 0.5;
        
        // Combine bands with palette colors
        vec3 color = vec3(0.0);
        color += getPaletteColor(band1 + time * 0.01) * band1 * 0.5;
        color += getPaletteColor(band2 + time * 0.015 + 0.3) * band2 * 0.3;
        color += getPaletteColor(band3 + time * 0.008 + 0.6) * band3 * 0.2;
        
        // Add subtle noise for texture (Balatro's pixelated aesthetic)
        float noise = snoise(uv * 20.0 + time * 0.1) * 0.1;
        color += noise;
        
        // Vignette for depth (CRT-style effect common in Balatro)
        float vignette = 1.0 - radius * 0.8;
        vignette = clamp(vignette, 0.3, 1.0);
        color *= vignette;
        
        return color;
    }

    // ========== STYLE 2: STARFIELD/NEBULA (Cosmic particle effect) ==========
    vec3 renderStarfieldStyle(vec2 uv, float time, float yaw, float pitch) {
        vec2 p = uv * 2.0 - 1.0;
        float aspect = 1.0;
        
        // Base deep space background
        vec3 bgColor = vec3(0.02, 0.03, 0.08);
        
        // Twinkling stars using hash-based placement
        vec3 stars = vec3(0.0);
        for (float i = 0.0; i < 50.0; i++) {
            // Deterministic star positions
            vec2 seed = vec2(sin(i * 0.1), cos(i * 0.17));
            vec2 starPos = vec2(
                sin(seed.x * 100.0 + time * 0.01) * 0.9,
                cos(seed.y * 100.0 - time * 0.015) * 0.9 * aspect
            );
            float dist = distance(p, starPos);
            // Twinkle effect
            float twinkle = 0.5 + 0.5 * sin(time * 3.0 + i);
            float brightness = smoothstep(0.02, 0.0, dist) * twinkle;
            stars += vec3(0.8, 0.9, 1.0) * brightness * (0.3 + fract(i * 0.3));
        }
        
        // Subtle nebula clouds using layered noise
        vec2 nebulaUv = uv + vec2(time * 0.0002, time * 0.0001);
        float nebula1 = snoise(nebulaUv * 2.0) * 0.5 + 0.5;
        float nebula2 = snoise(nebulaUv * 4.0 + vec2(time * 0.005, 0.0)) * 0.3;
        float nebulaIntensity = clamp(nebula1 * 0.7 + nebula2 * 0.3, 0.0, 0.4);
        
        // Color nebula with palette
        vec3 nebulaColor = getPaletteColor(nebulaIntensity + time * 0.001) * nebulaIntensity;
        
        // Camera motion affects star parallax
        float parallax = (yaw * 0.1 + pitch * 0.05);
        p += vec2(parallax, -parallax * 0.5);
        
        return bgColor + stars + nebulaColor;
    }

    // ========== MIXING LOGIC ==========
    // Interpolates between animation styles based on uAnimationMode
    // Integer values = pure style, floats = weighted mix toward nearest integers
    vec3 mixAnimationStyles(vec2 uv, float time, float yaw, float pitch, float mode) {
        // Clamp mode to valid range [0, 2]
        mode = clamp(mode, 0.0, 2.0);
        
        // Find bounding integer styles
        float lower = floor(mode);
        float upper = ceil(mode);
        float weight = mode - lower;  // How far toward upper style
        
        // Get the two style results
        vec3 resultLower, resultUpper;
        
        if (lower < 0.5) resultLower = renderAuroraStyle(uv, time, yaw, pitch);
        else if (lower < 1.5) resultLower = renderBalatroStyle(uv, time, yaw, pitch);
        else resultLower = renderStarfieldStyle(uv, time, yaw, pitch);
        
        if (upper < 0.5) resultUpper = renderAuroraStyle(uv, time, yaw, pitch);
        else if (upper < 1.5) resultUpper = renderBalatroStyle(uv, time, yaw, pitch);
        else resultUpper = renderStarfieldStyle(uv, time, yaw, pitch);
        
        // Mix between them (if mode is integer, weight=0, so pure lower style)
        return mix(resultLower, resultUpper, weight);
    }

    void main() {
        // Adjust UVs based on camera orientation (shared across styles)
        vec2 uv = TexCoord;
        
        // Get mixed animation result
        vec3 color = mixAnimationStyles(uv, uTime, uYaw, uPitch, uAnimationMode);
        
        // Adjust opacity based on pitch (shared behavior)
        float pitchNormalized = (uPitch + 1.5708) / 3.14159;
        float opacity = clamp(pitchNormalized, 0.2, 1.0);
        
        FragColor = vec4(color, opacity);
    }
    )";