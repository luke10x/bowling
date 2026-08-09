#pragma once

#include "framework/gl_header.h"
#include "framework/gl_util.h"
#include "framework/boot.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

struct ExplosionFx
{
    static const char *VERTEX_SHADER;
    static const char *FRAGMENT_SHADER;
    static constexpr float VISUAL_STRENGTH = 0.34f;

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint shaderId = 0;
    bool active = false;
    float age = 0.0f;
    float duration = 0.85f;
    float seed = 0.0f;
    glm::vec3 target = glm::vec3(0.0f);

    void initExplosion()
    {
        shaderId = vtx::createShaderProgram(VERTEX_SHADER, FRAGMENT_SHADER);
        const GLfloat quad[] = {-1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f};
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void *)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }

    void burst(glm::vec3 worldTarget)
    {
        target = worldTarget;
        target.y = 0.012f;
        age = 0.0f;
        active = true;
        seed += 9.73f;
    }

    void renderExplosion(float deltaTime, const glm::mat4 &view, const glm::mat4 &projection)
    {
        if (!active)
            return;
        age += glm::clamp(deltaTime, 0.0f, 0.05f);
        const float t = glm::clamp(age / duration, 0.0f, 1.0f);
        if (t >= 1.0f)
        {
            active = false;
            return;
        }

        GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
        GLboolean depthWasEnabled = glIsEnabled(GL_DEPTH_TEST);
        GLint oldSrcRgb = GL_ONE, oldDstRgb = GL_ZERO, oldSrcAlpha = GL_ONE, oldDstAlpha = GL_ZERO;
        glGetIntegerv(GL_BLEND_SRC_RGB, &oldSrcRgb);
        glGetIntegerv(GL_BLEND_DST_RGB, &oldDstRgb);
        glGetIntegerv(GL_BLEND_SRC_ALPHA, &oldSrcAlpha);
        glGetIntegerv(GL_BLEND_DST_ALPHA, &oldDstAlpha);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glUseProgram(shaderId);
        const float grow = t * t * (3.0f - 2.0f * t);
        const float scale = glm::mix(0.30f, 1.65f, grow);
        const glm::mat4 model =
            glm::translate(glm::mat4(1.0f), target) *
            glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) *
            glm::scale(glm::mat4(1.0f), glm::vec3(scale));
        glUniformMatrix4fv(glGetUniformLocation(shaderId, "u_model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(shaderId, "u_worldToView"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderId, "u_projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform1f(glGetUniformLocation(shaderId, "u_t"), t);
        glUniform1f(glGetUniformLocation(shaderId, "u_seed"), seed);
        glUniform1f(glGetUniformLocation(shaderId, "u_strength"), VISUAL_STRENGTH);
        glUniform1f(glGetUniformLocation(shaderId, "u_mode"), 0.0f);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        const float plumeH = glm::mix(0.42f, 1.22f, grow);
        const float plumeW = glm::mix(0.32f, 0.72f, grow);
        for (int i = 0; i < 3; ++i)
        {
            const float yaw = glm::radians(60.0f * (float)i);
            const glm::mat4 plumeModel =
                glm::translate(glm::mat4(1.0f), target + glm::vec3(0.0f, plumeH, 0.0f)) *
                glm::rotate(glm::mat4(1.0f), yaw, glm::vec3(0.0f, 1.0f, 0.0f)) *
                glm::scale(glm::mat4(1.0f), glm::vec3(plumeW, plumeH, 1.0f));
            glUniformMatrix4fv(glGetUniformLocation(shaderId, "u_model"), 1, GL_FALSE, glm::value_ptr(plumeModel));
            glUniform1f(glGetUniformLocation(shaderId, "u_mode"), 1.0f + (float)i);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
        glBindVertexArray(0);

        if (depthWasEnabled) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        if (blendWasEnabled) glEnable(GL_BLEND); else glDisable(GL_BLEND);
        glBlendFuncSeparate(oldSrcRgb, oldDstRgb, oldSrcAlpha, oldDstAlpha);
    }
};

const char *ExplosionFx::VERTEX_SHADER = GLSL_VERSION R"(
precision mediump float;
layout (location = 0) in vec2 a_position;
out vec2 v_local;
uniform mat4 u_model;
uniform mat4 u_worldToView;
uniform mat4 u_projection;
void main() {
    v_local = a_position;
    vec4 worldPos = u_model * vec4(a_position, 0.0, 1.0);
    gl_Position = u_projection * u_worldToView * worldPos;
}
)";

const char *ExplosionFx::FRAGMENT_SHADER = GLSL_VERSION R"(
precision mediump float;
in vec2 v_local;
uniform float u_t;
uniform float u_seed;
uniform float u_mode;
uniform float u_strength;
out vec4 fragColor;

float hash(float x) { return fract(sin(x) * 43758.5453); }

float cellNoise(vec2 p)
{
    vec2 c = floor(p);
    return hash(c.x * 37.0 + c.y * 113.0 + u_seed);
}

void main() {
    vec2 d = v_local;
    float grow = u_t * u_t * (3.0 - 2.0 * u_t);
    if (u_mode < 0.5) {
        float r = length(d);
        if (r > 1.04) discard;
        float a = atan(d.y, d.x);
        float radius = mix(0.22, 0.92, grow);
        float fade = 1.0 - smoothstep(0.62, 1.0, u_t);
        float sector = floor((a + 3.14159265) / 6.2831853 * 18.0);
        float jag = 0.74 + 0.34 * hash(sector + floor(u_t * 9.0) * 13.0 + u_seed);
        float edge = radius * jag;
        float shell = step(r, edge) * step(edge - 0.12, r);
        float shards = step(0.58, hash(sector * 3.7 + floor(r * 9.0) + u_seed));
        shell *= mix(0.55, 1.15, shards);
        float core = step(r, 0.23 + 0.08 * (1.0 - grow)) * (1.0 - grow * 0.82);
        float smoke = step(r, radius * 1.06) * step(radius * 0.46, r) * (1.0 - shell * 0.35);
        vec3 hot = mix(vec3(1.0, 0.96, 0.55), vec3(1.0, 0.25, 0.06), grow);
        vec3 smokeCol = vec3(0.12, 0.08, 0.10);
        vec3 col = hot * (core * 1.75 + shell * 1.28) + smokeCol * smoke * 0.22;
        float alpha = clamp((core * 1.15 + shell * 0.92 + smoke * 0.10) * fade, 0.0, 0.92);
        fragColor = vec4(col * u_strength, alpha * u_strength);
        return;
    }

    float y01 = d.y * 0.5 + 0.5;
    if (y01 < 0.0 || y01 > 1.0) discard;
    float x = d.x;
    float passSeed = u_seed + u_mode * 19.37;
    float rise = grow * 0.72;
    float neck = mix(0.72, 0.22, y01);
    float mushroom = 0.42 * exp(-pow((y01 - (0.36 + rise * 0.38)) / 0.28, 2.0));
    float width = neck + mushroom;
    float sideJag = 0.78 + 0.30 * cellNoise(vec2(floor(y01 * 9.0), floor(u_t * 10.0) + u_mode));
    float edgeMask = step(abs(x), width * sideJag);

    vec2 q = vec2(x / max(0.12, width), y01 - 0.12 - rise);
    q.x += (cellNoise(vec2(y01 * 8.0, u_mode)) - 0.5) * 0.32;
    float plumeR = length(q * vec2(1.15, 1.85));
    float cells = cellNoise(vec2(x * 7.0 + passSeed, y01 * 11.0 - u_t * 7.0));
    float cells2 = cellNoise(vec2(x * 13.0 - u_t * 4.0 + passSeed, y01 * 17.0));
    float lump = step(plumeR, 0.82 + cells * 0.30);
    float holes = step(0.18 + grow * 0.35, cells2);
    float fire = lump * holes * edgeMask;
    float core = fire * step(plumeR, 0.38 + 0.18 * (1.0 - grow));
    float smoke = edgeMask * step(plumeR, 1.08 + cells * 0.20) * (1.0 - core);
    float baseFlash = (1.0 - smoothstep(0.0, 0.42, y01)) * (1.0 - grow);
    float fade = 1.0 - smoothstep(0.72, 1.0, u_t);
    vec3 fireCol = mix(vec3(1.0, 0.92, 0.36), vec3(1.0, 0.22, 0.03), y01 + grow * 0.35);
    vec3 smokeCol = mix(vec3(0.19, 0.13, 0.10), vec3(0.05, 0.045, 0.055), y01);
    vec3 col = fireCol * (fire * 1.18 + core * 1.25 + baseFlash * 0.86) + smokeCol * smoke * (0.28 + grow * 0.24);
    float alpha = clamp((fire * 0.74 + core * 0.62 + smoke * 0.20 + baseFlash * 0.52) * fade, 0.0, 0.84);
    fragColor = vec4(col * u_strength, alpha * u_strength);
}
)";
