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

// ========== CONFIGURATION: EASY TO EXTEND ==========
#define NUM_ANIMATION_STYLES 4  // Update this when adding new styles
// Styles:
// 0 = Original Aurora (cloud noise)
// 1 = Balatro Swirl (hypnotic geometric) - FIXED: no seal artifact
// 2 = Starfield/Nebula (cosmic particles)
// 3 = Aurora Waves (flowing horizontal bands) [NEW]
// ================================================

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

        const GLuint fullscreenQuadIndices[] = {0, 1, 2, 1, 3, 2};

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
        this->auroraShaderId = vtx::createShaderProgram(AURORA_VERTEX_SHADER, AURORA_FRAGMENT_SHADER);
    }

    void hangAuroraShader() { /* Cleanup if needed */ }

    /**
     * @param animationMode: Float controlling animation style with wrapping
     *   - Integer values: pure style (0,1,2,3)
     *   - Float values: weighted mix between nearest styles
     *   - Wrapping: mode % NUM_ANIMATION_STYLES (e.g., 4.0→0.0, 5.3→1.3)
     *   - Examples:
     *     0.0 = pure aurora | 0.5 = 50% aurora + 50% Balatro
     *     3.8 = 20% waves + 80% aurora (wraps from 3→0)
     *     5.0 = pure aurora (5 % 4 = 1? No: 5.0 → 1.0, but we want 4.0→0.0)
     *     Actually: floor(5.0) % 4 = 1, but we want 4.0 to wrap to 0.0
     *     Correction: We wrap the INTEGER part only: 
     *       styleIndex = int(floor(mode)) % NUM_STYLES
     *       fractional = mode - floor(mode)
     *       Then mix styleIndex and (styleIndex+1)%NUM_STYLES using fractional
     */
    void renderAurora(float deltaTime, const glm::mat4 cameraMatrix, float animationMode = 0.0f)
    {
        glUseProgram(this->auroraShaderId);

        glm::vec3 forward = glm::normalize(glm::vec3(
            cameraMatrix[0][2], cameraMatrix[1][2], cameraMatrix[2][2]));
        float yaw = atan2(forward.x, forward.z) * 5.0f;
        float pitch = asin((forward.y + 1.0f) * 0.5f);

        glUniform1f(glGetUniformLocation(this->auroraShaderId, "uYaw"), yaw);
        glUniform1f(glGetUniformLocation(this->auroraShaderId, "uPitch"), pitch);

        this->time += 1.0f * deltaTime;
        glUniform1f(glGetUniformLocation(this->auroraShaderId, "uTime"), this->time);
        glUniform1f(glGetUniformLocation(this->auroraShaderId, "uAnimationMode"), animationMode);
        // Pass num styles for wrapping logic in shader
        glUniform1f(glGetUniformLocation(this->auroraShaderId, "uNumStyles"), float(NUM_ANIMATION_STYLES));

        glBindVertexArray(this->auroraVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
};

const char *Aurora::AURORA_VERTEX_SHADER = GLSL_VERSION R"(
    precision highp float;
    layout(location = 0) in vec3 aPos;
    layout(location = 1) in vec2 aTexCoord;
    out vec2 TexCoord;
    void main() {
        TexCoord = aTexCoord;
        gl_Position = vec4(aPos, 1.0);
    }
)";

const char *Aurora::AURORA_FRAGMENT_SHADER = GLSL_VERSION R"(
    precision highp float;
    in vec2 TexCoord;
    out vec4 FragColor;
    uniform float uYaw, uPitch, uTime, uAnimationMode, uNumStyles;

    // ========== SHARED UTILITIES ==========
    vec3 mod289(vec3 x) { return x - floor(x * (1.0/289.0)) * 289.0; }
    vec2 mod289(vec2 x) { return x - floor(x * (1.0/289.0)) * 289.0; }
    vec3 permute(vec3 x) { return mod289(((x*34.0)+1.0)*x); }
    
    float snoise(vec2 v) {
        const vec4 C = vec4(0.211324865405187, 0.366025403784439, -0.577350269189626, 0.024390243902439);
        vec2 i = floor(v + dot(v, C.yy));
        vec2 x0 = v - i + dot(i, C.xx);
        vec2 i1 = (x0.x > x0.y) ? vec2(1.0,0.0) : vec2(0.0,1.0);
        vec4 x12 = x0.xyxy + C.xxzz; x12.xy -= i1;
        i = mod289(i);
        vec3 p = permute(permute(i.y + vec3(0.0,i1.y,1.0)) + i.x + vec3(0.0,i1.x,1.0));
        vec3 m = max(0.5 - vec3(dot(x0,x0), dot(x12.xy,x12.xy), dot(x12.zw,x12.zw)), 0.0);
        m = m*m; m = m*m;
        vec3 x = 2.0*fract(p*C.www)-1.0;
        vec3 h = abs(x)-0.5; vec3 ox = floor(x+0.5); vec3 a0 = x-ox;
        m *= 1.79284291400159 - 0.85373472095314*(a0*a0 + h*h);
        vec3 g; g.x = a0.x*x0.x + h.x*x0.y;
        g.yz = a0.yz*x12.xz + h.yz*x12.yw;
        return 130.0 * dot(m, g);
    }

    // Reusable aurora palette: deep blue → cyan → green → purple → pink
    vec3 getPaletteColor(float t) {
        vec3 c1 = vec3(0.0, 0.2, 0.5);   // Deep blue
        vec3 c2 = vec3(0.0, 0.8, 0.9);   // Cyan  
        vec3 c3 = vec3(0.3, 1.0, 0.4);   // Green
        vec3 c4 = vec3(0.7, 0.3, 1.0);   // Purple
        vec3 c5 = vec3(1.0, 0.4, 0.7);   // Pink (for style 3)
        t = fract(t);
        if (t < 0.25) return mix(c1, c2, t*4.0);
        if (t < 0.50) return mix(c2, c3, (t-0.25)*4.0);
        if (t < 0.75) return mix(c3, c4, (t-0.50)*4.0);
        return mix(c4, c5, (t-0.75)*4.0);
    }

    // Camera-based UV adjustment (shared)
    vec2 applyCameraMotion(vec2 uv, float yaw, float pitch, float time) {
        float yawNorm = yaw / 3.14159;
        float yawOffset = yawNorm * abs(yawNorm);
        float pitchNorm = (pitch + 1.5708) / 3.14159;
        float pitchOffset = (pitchNorm - 0.5) * 4.0;
        return uv + vec2(yawOffset, pitchOffset + time*0.0005);
    }

    // ========== STYLE 0: ORIGINAL AURORA ==========
    vec3 styleAurora(vec2 uv, float time, float yaw, float pitch) {
        vec2 coord = applyCameraMotion(uv, yaw, pitch, time);
        float n1 = snoise(coord*3.0)*0.5;
        float n2 = snoise(coord*7.0 + vec2(time*0.01,0.0))*0.3;
        float n3 = snoise(coord*15.0 + vec2(time*0.02,0.0))*0.2;
        float intensity = clamp(n1+n2+n3, 0.0, 1.0);
        vec3 c1 = vec3(sin(uv.x+uv.y+time*0.001), 0.2, 0.3);
        vec3 c2 = vec3(0.9, sin(uv.y+time*0.0005), 0.5);
        return mix(c1, c2, intensity);
    }

    // ========== STYLE 1: BALATRO SWIRL (FIXED: NO SEAM/SEAL) ==========
    vec3 styleBalatro(vec2 uv, float time, float yaw, float pitch) {
        vec2 p = uv * 2.0 - 1.0;  // Center to [-1, 1]
        time *= 0.03; 
        // Polar coordinates
        float angle = atan(p.y, p.x);
        float radius = length(p) + 0.001;  // Avoid center singularity
        
        // 🔑 KEY FIX: Never use raw 'angle' directly in patterns.
        // Instead, use sin(angle*N) and cos(angle*N) which are naturally continuous.
        float cosA = cos(angle * 3.0);  // 3-lobed pattern, continuous
        float sinA = sin(angle * 3.0);
        
        // Time-based rotation (continuous)
        float rot = time * 0.25;
        float cosRot = cos(rot), sinRot = sin(rot);
        
        // Rotate the angular pattern smoothly
        float angularPattern = cosA * cosRot - sinA * sinRot;  // cos(angle*3 + time)
        
        // Radial bands (always continuous)
        float radialBands = sin(radius * 18.0 - time * 0.5) * 0.5 + 0.5;
        
        // Domain warping for organic flow (no angle dependency)
        vec2 warp = vec2(
            sin(p.x * 5.0 + time * 0.2 + radialBands * 2.0),
            cos(p.y * 5.0 - time * 0.15 + angularPattern * 1.5)
        );
        float warpNoise = snoise((uv + warp * 0.1) * 25.0 + time * 0.08) * 0.3;
        
        // Combine patterns
        float pattern = (angularPattern * 0.4 + radialBands * 0.4 + warpNoise * 0.2);
        pattern = pattern * 0.5 + 0.5;  // Normalize to [0,1]
        
        // Apply palette with smooth color flow
        vec3 color = vec3(0.0);
        color += getPaletteColor(pattern + time * 0.01) * smoothstep(0.2, 0.8, pattern);
        color += getPaletteColor(pattern * 1.2 + time * 0.015 + 0.3) * 0.3;
        
        // Subtle grain for texture (Balatro aesthetic)
        float grain = fract(sin(dot(uv + time * 0.001, vec2(12.9898, 78.233))) * 43758.5453);
        color += (grain - 0.5) * 0.04;
        
        // Soft radial vignette (smooth fade, no hard edge)
        float vignette = smoothstep(1.15, 0.15, radius);
        color *= vignette;
        
        // Optional: subtle center glow (not a hard seal)
        float centerGlow = exp(-radius * 8.0) * 0.15;
        color += getPaletteColor(time * 0.02) * centerGlow;
        
        return color;
    }

    // ========== STYLE 2: STARFIELD/NEBULA ==========
    vec3 styleStarfield(vec2 uv, float time, float yaw, float pitch) {
        vec2 p = uv * 2.0 - 1.0;
        vec3 bgColor = vec3(0.02, 0.03, 0.08);
        time *= 0.5;
        
        // Twinkling stars
        vec3 stars = vec3(0.0);
        // {
        //     time *= 0.125;
        //     for(float i = 0.0; i < 60.0; i++) {
        //         vec2 seed = vec2(sin(i*0.13), cos(i*0.19));
        //         vec2 starPos = vec2(
        //             sin(seed.x*120.0 + time*0.008)*0.95,
        //             cos(seed.y*120.0 - time*0.012)*0.95
        //         );
        //         float dist = distance(p, starPos);
        //         float twinkle = 0.5 + 0.5*sin(time*4.0 + i*0.7);
        //         float bright = smoothstep(0.015, 0.0, dist) * twinkle;
        //         stars += vec3(0.9, 1.0, 1.0) * bright * (0.4 + fract(i*0.27));
        //     }
        // }
        
        // Nebula clouds
        vec2 nuv = uv + vec2(time*0.0002, time*0.0001);
        float neb = snoise(nuv*2.5)*0.6 + snoise(nuv*5.0 + time*0.006)*0.4;
        neb = clamp(neb, 0.0, 0.5);
        vec3 nebColor = getPaletteColor(neb + time*0.001) * neb;
        
        // Parallax from camera
        p += vec2(yaw*0.08, -pitch*0.04);
        
        return bgColor + stars + nebColor;
    }

    // ========== STYLE 3: AURORA WAVES (NEW: Flowing horizontal bands) ==========
    vec3 styleWaves(vec2 uv, float time, float yaw, float pitch) {

        time *= 0.25;
        vec2 coord = applyCameraMotion(uv, yaw, -pitch, time);
        
        vec3 color = vec3(0.0);
        {
            time *= 0.0125;
            // Horizontal wave layers with varying frequencies
            float wave1 = sin(coord.y*8.0 + time*0.3 + sin(coord.x*3.0 + time*0.1)*0.5) * 0.5 + 0.5;
            float wave2 = sin(coord.y*12.0 - time*0.2 + cos(coord.x*5.0 - time*0.15)*0.3) * 0.5 + 0.5;
            float wave3 = sin((coord.y + coord.x*0.3)*20.0 + time*0.5) * 0.5 + 0.5;
            
            // Combine with palette and soft blending
            color += getPaletteColor(wave1 + time*0.02) * wave1 * 0.5;
            color += getPaletteColor(wave2 + time*0.025 + 0.3) * wave2 * 0.35;
            color += getPaletteColor(wave3 + time*0.015 + 0.6) * wave3 * 0.15;
        }
        
        // Add subtle vertical gradient for depth
        float gradient = smoothstep(0.0, 1.0, uv.y);
        color = mix(color, color*1.2, gradient*0.3);
        
        // Soft noise overlay for organic feel
        float noise = snoise(coord*18.0 + time*0.05) * 0.06;
        color += noise;
        
        return color;
    }

    // ========== STYLE DISPATCHER (Easy to extend) ==========
    vec3 getStyleResult(vec2 uv, float time, float yaw, float pitch, int styleIndex) {
        // Add new styles here and update NUM_ANIMATION_STYLES
        if (styleIndex == 0) return styleAurora(uv, time, yaw, pitch);
        if (styleIndex == 1) return styleBalatro(uv, time, yaw, pitch);
        if (styleIndex == 2) return styleStarfield(uv, time, yaw, pitch);
        if (styleIndex == 3) return styleWaves(uv, time, yaw, pitch);
        // Fallback to style 0 for safety
        return styleAurora(uv, time, yaw, pitch);
    }

    // ========== WRAPPING MIXER (Key feature) ==========
    vec3 mixAnimationStyles(vec2 uv, float time, float yaw, float pitch, float mode, float numStyles) {
        // Wrap mode to [0, numStyles) with smooth transitions across boundary
        float base = floor(mode);
        float frac = fract(mode);  // fractional part for mixing
        
        // Wrap the integer part: e.g., 4.0 → 0, 5.3 → 1.3
        int styleA = int(mod(base, numStyles));
        int styleB = int(mod(base + 1.0, numStyles));  // next style (with wrap)
        
        vec3 resultA = getStyleResult(uv, time, yaw, pitch, styleA);
        vec3 resultB = getStyleResult(uv, time, yaw, pitch, styleB);
        
        // Mix using fractional part (0.0 = pure A, 1.0 = pure B)
        return mix(resultA, resultB, frac);
    }

    void main() {
        vec2 uv = TexCoord;
        vec3 color = mixAnimationStyles(uv, uTime, uYaw, uPitch, uAnimationMode, uNumStyles);
        float pitchNorm = (uPitch + 1.5708) / 3.14159;
        float opacity = clamp(pitchNorm, 0.2, 1.0);
        FragColor = vec4(color, opacity);
    }
)";