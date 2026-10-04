#pragma once

#include "framework/gl_header.h"
#include "framework/gl_util.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

// A real coplanar lane overlay, kept separate from the lane material so a dry
// condition reads like a decal rather than a change to the wood texture.
struct LaneDryDecal
{
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint shaderId = 0;

    void init()
    {
        if (shaderId != 0)
            return;

        static const char *vertexShader = GLSL_VERSION R"(
            precision highp float;
            layout(location = 0) in vec3 a_pos;
            layout(location = 1) in vec2 a_uv;
            uniform mat4 u_worldToView;
            uniform mat4 u_projection;
            out vec2 v_uv;
            void main() {
                v_uv = a_uv;
                gl_Position = u_projection * u_worldToView * vec4(a_pos, 1.0);
            }
        )";
        static const char *fragmentShader = GLSL_VERSION R"(
            precision highp float;
            in vec2 v_uv;
            out vec4 FragColor;
            uniform float u_leftDryStrength;
            uniform float u_rightDryStrength;
            uniform float u_leftDryReach01;
            uniform float u_rightDryReach01;
            float hash21(vec2 p) {
                p = fract(p * vec2(123.34, 456.21));
                p += dot(p, p + 45.32);
                return fract(p.x * p.y);
            }
            void main() {
                // v=0 is the player/foul-line end.  Dryness falls away over
                // the whole lane rather than ending in a hard transverse band.
                float sideT = clamp(v_uv.x, 0.0, 1.0);
                float dryStrength = mix(u_leftDryStrength, u_rightDryStrength, sideT);
                float dryReach01 = mix(u_leftDryReach01, u_rightDryReach01, sideT);
                // Each side has its own worn reach, so a ball repeatedly run
                // down one side leaves a longer dry trail there.
                float playerFade = 1.0 - smoothstep(dryReach01 * 0.22, max(0.001, dryReach01), v_uv.y);
                float edgeFade = smoothstep(0.0, 0.09, v_uv.x) *
                                 (1.0 - smoothstep(0.91, 1.0, v_uv.x));
                vec2 cells = floor(vec2(v_uv.x * 62.0, v_uv.y * 155.0));
                float dither = step(0.46, hash21(cells));
                float grain = hash21(cells + vec2(19.7, 7.3));
                float alpha = playerFade * edgeFade * mix(0.035, 0.34, dither) * dryStrength;
                alpha *= mix(0.72, 1.0, grain);
                FragColor = vec4(vec3(0.075, 0.052, 0.042), alpha);
            }
        )";

        shaderId = vtx::createShaderProgram(vertexShader, fragmentShader);
        // Lane: x = 41.857 in wide, z from player (-18.3) to the pin end (0).
        const float vertices[] = {
            -0.531f, 0.004f, -18.3f, 0.0f, 0.0f,
             0.531f, 0.004f, -18.3f, 1.0f, 0.0f,
             0.531f, 0.004f,   0.0f, 1.0f, 1.0f,
            -0.531f, 0.004f, -18.3f, 0.0f, 0.0f,
             0.531f, 0.004f,   0.0f, 1.0f, 1.0f,
            -0.531f, 0.004f,   0.0f, 0.0f, 1.0f,
        };
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
        glBindVertexArray(0);
    }

    void render(
        const glm::mat4 &worldToView,
        const glm::mat4 &projection,
        float leftDryStrength,
        float rightDryStrength,
        float leftDryReach01,
        float rightDryReach01)
    {
        init();
        const GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
        GLboolean depthMask = GL_TRUE;
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        glUseProgram(shaderId);
        glUniformMatrix4fv(glGetUniformLocation(shaderId, "u_worldToView"), 1, GL_FALSE, glm::value_ptr(worldToView));
        glUniformMatrix4fv(glGetUniformLocation(shaderId, "u_projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform1f(glGetUniformLocation(shaderId, "u_leftDryStrength"), glm::clamp(leftDryStrength, 0.0f, 1.0f));
        glUniform1f(glGetUniformLocation(shaderId, "u_rightDryStrength"), glm::clamp(rightDryStrength, 0.0f, 1.0f));
        glUniform1f(glGetUniformLocation(shaderId, "u_leftDryReach01"), glm::clamp(leftDryReach01, 0.0f, 1.0f));
        glUniform1f(glGetUniformLocation(shaderId, "u_rightDryReach01"), glm::clamp(rightDryReach01, 0.0f, 1.0f));
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        glDepthMask(depthMask);
        if (!blendWasEnabled)
            glDisable(GL_BLEND);
    }
};
