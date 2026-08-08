#pragma once

#include "framework/gl_header.h"
#include "framework/gl_util.h"
#include "framework/boot.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>

struct Thunder
{
    static const char *VERTEX_SHADER;
    static const char *FRAGMENT_SHADER;

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint shaderId = 0;
    bool active = false;
    float age = 0.0f;
    float duration = 1.0f;
    float blindDuration = 0.5f;
    float seed = 0.0f;
    glm::vec2 target = glm::vec2(0.0f);

    void initThunder()
    {
        shaderId = vtx::createShaderProgram(VERTEX_SHADER, FRAGMENT_SHADER);
        const GLfloat quad[] = {
            -1.0f, -1.0f,
             1.0f, -1.0f,
            -1.0f,  1.0f,
             1.0f,  1.0f,
        };
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void *)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }

    void strike(glm::vec2 screenTarget)
    {
        target = screenTarget;
        age = 0.0f;
        active = true;
        seed += 17.31f;
    }

    void connect(glm::vec2 screenTarget)
    {
        target = screenTarget;
    }

    void renderThunder(float deltaTime, int screenWidth, int screenHeight)
    {
        if (!active || screenWidth <= 0 || screenHeight <= 0)
            return;

        age += glm::clamp(deltaTime, 0.0f, 0.05f);
        const float t = glm::clamp(age / duration, 0.0f, 1.0f);
        const float blind01 = age > duration
            ? 1.0f - glm::clamp((age - duration) / glm::max(0.001f, blindDuration), 0.0f, 1.0f)
            : 0.0f;
        if (age >= duration + blindDuration)
        {
            active = false;
            return;
        }

        const glm::vec2 clampedTarget(
            glm::clamp(target.x, 0.0f, (float)screenWidth),
            glm::clamp(target.y, 0.0f, (float)screenHeight)
        );

        GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
        GLboolean depthWasEnabled = glIsEnabled(GL_DEPTH_TEST);
        GLint oldSrcRgb = GL_ONE, oldDstRgb = GL_ZERO, oldSrcAlpha = GL_ONE, oldDstAlpha = GL_ZERO;
        glGetIntegerv(GL_BLEND_SRC_RGB, &oldSrcRgb);
        glGetIntegerv(GL_BLEND_DST_RGB, &oldDstRgb);
        glGetIntegerv(GL_BLEND_SRC_ALPHA, &oldSrcAlpha);
        glGetIntegerv(GL_BLEND_DST_ALPHA, &oldDstAlpha);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);

        glUseProgram(shaderId);
        glUniform1f(glGetUniformLocation(shaderId, "u_screenWidth"), (float)screenWidth);
        glUniform1f(glGetUniformLocation(shaderId, "u_screenHeight"), (float)screenHeight);
        glUniform1f(glGetUniformLocation(shaderId, "u_t"), t);
        glUniform1f(glGetUniformLocation(shaderId, "u_blind01"), blind01);
        glUniform1f(glGetUniformLocation(shaderId, "u_seed"), seed);
        glUniform2f(glGetUniformLocation(shaderId, "u_target"), clampedTarget.x, clampedTarget.y);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);

        if (depthWasEnabled)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);
        if (blendWasEnabled)
            glEnable(GL_BLEND);
        else
            glDisable(GL_BLEND);
        glBlendFuncSeparate(oldSrcRgb, oldDstRgb, oldSrcAlpha, oldDstAlpha);
    }
};

const char *Thunder::VERTEX_SHADER = GLSL_VERSION R"(
precision mediump float;
layout (location = 0) in vec2 a_position;
out vec2 v_texCoord;

void main()
{
    v_texCoord = (a_position + 1.0) * 0.5;
    gl_Position = vec4(a_position, -1.0, 1.0);
}
)";

const char *Thunder::FRAGMENT_SHADER = GLSL_VERSION R"(
precision mediump float;

in vec2 v_texCoord;
uniform float u_screenWidth;
uniform float u_screenHeight;
uniform float u_t;
uniform float u_blind01;
uniform float u_seed;
uniform vec2 u_target;
out vec4 fragColor;

float hash(float x)
{
    return fract(sin(x) * 75154.32912);
}

float noise(float x)
{
    float i = floor(x);
    float f = fract(x);
    f = f * f * (3.0 - 2.0 * f);
    return mix(hash(i), hash(i + 1.0), f);
}

float fbm(float x)
{
    float r = 0.0;
    float a = 0.5;
    float s = 1.0;
    for (int i = 0; i < 4; ++i)
    {
        r += a * noise(x * s);
        s *= 2.1;
        a *= 0.52;
    }
    return r;
}

float boltLine(vec2 p, vec2 a, vec2 b, float width, float wobble)
{
    vec2 ab = b - a;
    float len2 = max(dot(ab, ab), 1.0);
    float h = clamp(dot(p - a, ab) / len2, 0.0, 1.0);
    vec2 n = normalize(vec2(-ab.y, ab.x));
    float taper = sin(h * 3.14159265);
    float live = u_t * 9.0;
    float twist = sin(h * 12.0 - live * 4.2 + u_seed) * 0.36;
    twist += sin(h * 27.0 + live * 2.7 + u_seed * 0.31) * 0.16;
    float jag = (fbm(h * 18.0 + u_seed + floor(live * 5.0) * 0.91) - 0.5) * wobble * taper;
    jag += (noise(h * 43.0 + u_seed * 1.7 + live * 3.0) - 0.5) * wobble * 0.42 * taper;
    jag += twist * wobble * taper;
    vec2 q = a + ab * h + n * jag;
    float d = length(p - q);
    return 1.0 - smoothstep(width, width * 2.7, d);
}

void main()
{
    vec2 p = v_texCoord * vec2(u_screenWidth, u_screenHeight);
    vec2 top = vec2(
        clamp(u_target.x + (hash(u_seed) - 0.5) * u_screenWidth * 0.34 + sin(u_t * 23.0 + u_seed) * 38.0, 10.0, u_screenWidth - 10.0),
        u_screenHeight + 24.0
    );
    vec2 target = u_target;

    float hold = 1.0 - smoothstep(0.76, 1.0, u_t);
    float strikeEnvelope = hold * (0.58 + 0.42 * noise(floor(u_t * 26.0) + u_seed));
    float earlySnap = 1.0 - smoothstep(0.12, 0.82, u_t);
    float pulseA = exp(-u_t * 8.0);
    float pulseB = exp(-abs(u_t - 0.18) * 18.0) * 0.95;
    float pulseC = exp(-abs(u_t - 0.43) * 12.0) * 0.72;
    float pulseD = exp(-abs(u_t - 0.68) * 16.0) * 0.42;
    float brightFlash = clamp(pulseA + pulseB + pulseC + pulseD, 0.0, 1.0);
    float darkFlash = hold * (0.42 + 0.26 * sin(u_t * 31.0 + u_seed));

    float mainBolt = boltLine(p, top, target, 2.0 + 2.5 * earlySnap, 42.0);
    float glowBolt = boltLine(p, top, target, 10.0 + 18.0 * earlySnap, 58.0);
    float farGlow = boltLine(p, top, target, 42.0, 72.0);

    vec2 ab = target - top;
    vec2 branchBase = top + ab * (0.35 + 0.22 * hash(u_seed + floor(u_t * 8.0) + 4.0));
    vec2 side = normalize(vec2(-ab.y, ab.x));
    vec2 branchEnd = branchBase + ab * 0.16 + side * (hash(u_seed + floor(u_t * 11.0) + 7.0) - 0.5) * 190.0;
    float branch = boltLine(p, branchBase, branchEnd, 1.2, 26.0);

    vec2 ballVec = p - target;
    float ballHaloDist = length(ballVec);
    float ballAngle = atan(ballVec.y, ballVec.x);
    float ballCore = 1.0 - smoothstep(18.0, 54.0, ballHaloDist);
    float ballAura = 1.0 - smoothstep(44.0, 150.0, ballHaloDist);
    float ballOutside = max(0.0, ballHaloDist - 38.0);
    float ballRoot = 1.0 - smoothstep(0.0, 48.0, ballOutside);
    float ballSpokeCore = max(0.0, cos(ballAngle * 22.0 + sin(ballOutside * 0.075 + u_t * 18.0) * 0.35));
    float ballSpikes = pow(ballSpokeCore, mix(10.0, 3.0, ballRoot));
    ballSpikes *= exp(-ballOutside / 38.0) * (1.0 - smoothstep(86.0, 132.0, ballOutside));
    float ballWhite = clamp((ballCore * 1.8 + ballAura * 0.62 + ballSpikes * 0.18125) * hold * 0.5, 0.0, 1.0);

    vec3 blue = vec3(0.34, 0.78, 1.00);
    vec3 violet = vec3(0.72, 0.50, 1.00);
    vec3 white = vec3(1.0);
    vec3 rgb = white * mainBolt * 1.65;
    rgb += mix(blue, violet, 0.35) * glowBolt * 0.85;
    rgb += blue * farGlow * 0.18;
    rgb += white * branch * 0.60;
    rgb += white * ballWhite * (1.6 + brightFlash * 0.8);

    float boltAlpha = clamp((mainBolt + glowBolt * 0.45 + farGlow * 0.10 + branch * 0.4) * strikeEnvelope + ballWhite * 0.86, 0.0, 0.98);
    float worldAlpha = clamp(max(darkFlash + brightFlash * 0.78, u_blind01 * 0.32), 0.0, 0.92);
    float worldBright01 = clamp(brightFlash * 1.85, 0.0, 1.0);
    vec3 worldFlash = mix(vec3(0.0), vec3(1.0), worldBright01 * (1.0 - u_blind01));

    float alpha = max(worldAlpha, boltAlpha) * 0.75;
    vec3 color = mix(worldFlash, rgb, clamp(boltAlpha * 1.6, 0.0, 1.0));
    fragColor = vec4(color * 0.75, alpha);
}
)";
