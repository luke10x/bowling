#pragma once

#include "../rendertexture.h"

// Draw the reward card's animated gold directly into whichever shop preview
// texture currently belongs to it. No ball scene or extra framebuffer is needed.
struct GoldRewardPreviewShader
{
    GLuint program = 0;
    GLuint vao = 0;

    void render(RenderTexture &target, float seconds, float consumeFlash01,
                int screenWidth, int screenHeight, GLuint restoreProgram)
    {
        if (!program)
        {
            static const char *vertex = GLSL_VERSION R"(
                void main() {
                    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
                    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
                }
            )";
            static const char *fragment = GLSL_VERSION R"(
                precision highp float;
                uniform vec2 u_size;
                uniform float u_time;
                uniform float u_consumeFlash;
                out vec4 fragColor;

                void main() {
                    vec2 uv = gl_FragCoord.xy / u_size;
                    // Broad diagonal ribbons echo the supplied brushed-gold reference.
                    float diagonal = uv.x + uv.y * 0.24;
                    float satin = 0.5 + 0.5 * sin(diagonal * 9.0 - u_time * 0.65);
                    float fine = 0.5 + 0.5 * sin(diagonal * 17.0 - u_time * 0.82 + 0.7);
                    float sheen = pow(satin, 2.2) * 0.52 + pow(fine, 5.0) * 0.18;
                    vec3 gold = mix(vec3(0.96, 0.69, 0.19), vec3(0.55, 0.31, 0.07),
                                    smoothstep(0.48, 1.33, diagonal) * 0.64);
                    gold = mix(gold, vec3(1.0, 0.91, 0.43), sheen);
                    gold = mix(gold, vec3(1.0, 0.97, 0.75), u_consumeFlash * 0.72);
                    fragColor = vec4(gold, 1.0);
                }
            )";
            program = vtx::createShaderProgram(vertex, fragment);
            glGenVertexArrays(1, &vao);
        }

        const GLboolean depthEnabled = glIsEnabled(GL_DEPTH_TEST);
        const GLboolean blendEnabled = glIsEnabled(GL_BLEND);
        const GLboolean cullEnabled = glIsEnabled(GL_CULL_FACE);
        GLboolean depthWrite = GL_TRUE;
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthWrite);
        target.bindForWriting();
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);
        glUseProgram(program);
        glUniform2f(glGetUniformLocation(program, "u_size"), (float)target.width, (float)target.height);
        glUniform1f(glGetUniformLocation(program, "u_time"), seconds);
        glUniform1f(glGetUniformLocation(program, "u_consumeFlash"), consumeFlash01);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
        target.unbind(screenWidth, screenHeight);
        glDepthMask(depthWrite);
        if (depthEnabled) glEnable(GL_DEPTH_TEST);
        if (blendEnabled) glEnable(GL_BLEND);
        if (cullEnabled) glEnable(GL_CULL_FACE);
        glUseProgram(restoreProgram);
    }
};
