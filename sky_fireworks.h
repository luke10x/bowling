#pragma once

#include "framework/gl_header.h"
#include "framework/gl_util.h"

// Ten overlapping bursts of 150 sparks, based on the supplied hash-and-polar
// firework. Each spark is one GPU instance instead of a loop in every sky pixel.
struct SkyFireworks
{
    static constexpr int NUM_EXPLOSIONS = 10;
    static constexpr int NUM_PARTICLES = 150;
    GLuint program = 0;
    GLuint vao = 0;

    void render(float seconds, float amount, int width, int height)
    {
        if (amount <= 0.001f || width <= 0 || height <= 0)
            return;
        if (!program)
        {
            static const char *vertex = GLSL_VERSION R"(
                precision highp float;
                uniform float u_time;
                uniform float u_aspect;
                uniform float u_amount;
                out vec2 v_corner;
                out vec3 v_color;
                out float v_alpha;

                float hash11(float t) { return fract(sin(t * 674.3) * 453.2); }
                vec2 hash12(float t) {
                    float x = hash11(t);
                    return vec2(x, fract(sin((t + x) * 714.3) * 263.2));
                }
                vec2 hash12Polar(float t) {
                    float angle = hash11(t) * 6.2831853;
                    float distance01 = fract(sin((t + angle) * 714.3) * 263.2);
                    return vec2(sin(angle), cos(angle)) * distance01;
                }
                void main() {
                    int burst = gl_InstanceID / 150;
                    int particle = gl_InstanceID - burst * 150;
                    // Each burst lasts one third as long; extra staggered bursts
                    // keep the sky populated while individual flares clear sooner.
                    float t = u_time * 2.16 + float(burst) / 10.0;
                    float cycle = floor(t);
                    float age = fract(t);
                    vec2 randomOffset = hash12(float(burst) + 1.0 + cycle);
                    // Keep burst centres and their flares inside the sky view,
                    // including narrow portrait screens.
                    vec2 origin = vec2((randomOffset.x - 0.5) * u_aspect * 0.48,
                                       0.23 + randomOffset.y * 0.19);
                    vec2 direction = hash12Polar(float(particle) + 1.0 +
                                                 float(burst) * 97.0 + cycle * 131.0) *
                                     vec2(0.20 * min(u_aspect, 1.0), 0.20);
                    vec2 centre = origin + direction * age;
                    vec2 corner = vec2((gl_VertexID == 1 || gl_VertexID == 2 || gl_VertexID == 4) ? 1.0 : -1.0,
                                       (gl_VertexID == 2 || gl_VertexID == 4 || gl_VertexID == 5) ? 1.0 : -1.0);
                    float size = mix(0.022, 0.009, age) * (particle == 0 ? 2.5 : 1.0);
                    vec2 p = centre + corner * size;
                    gl_Position = vec4(p.x * 2.0 / u_aspect, p.y * 2.0, 0.0, 1.0);
                    vec3 color = sin(4.0 * vec3(0.34, 0.54, 0.43) * cycle + float(burst)) * 0.25 + 0.75;
                    float twinkle = 0.65 + 0.35 * sin(age * 20.0 + float(particle));
                    float envelope = smoothstep(0.0, 0.09, age) *
                                     (1.0 - smoothstep(0.58, 0.86, age));
                    v_color = color;
                    v_alpha = envelope * twinkle * u_amount;
                    v_corner = corner;
                }
            )";
            static const char *fragment = GLSL_VERSION R"(
                precision mediump float;
                in vec2 v_corner;
                in vec3 v_color;
                in float v_alpha;
                out vec4 FragColor;
                void main() {
                    float radius = length(v_corner);
                    float core = exp(-radius * radius * 9.0);
                    float halo = exp(-radius * radius * 2.4) * 0.28;
                    // The glow must become transparent before its quad edge;
                    // otherwise each spark reveals the square decal boundary.
                    float edgeFade = 1.0 - smoothstep(0.50, 0.98, radius);
                    FragColor = vec4(v_color * (core * 1.7 + halo), v_alpha * edgeFade);
                }
            )";
            program = vtx::createShaderProgram(vertex, fragment);
            glGenVertexArrays(1, &vao);
        }

        const GLboolean blendEnabled = glIsEnabled(GL_BLEND);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glUseProgram(program);
        glUniform1f(glGetUniformLocation(program, "u_time"), seconds);
        glUniform1f(glGetUniformLocation(program, "u_aspect"), float(width) / float(height));
        glUniform1f(glGetUniformLocation(program, "u_amount"), amount);
        glBindVertexArray(vao);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, NUM_EXPLOSIONS * NUM_PARTICLES);
        glBindVertexArray(0);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        if (!blendEnabled) glDisable(GL_BLEND);
    }
};
