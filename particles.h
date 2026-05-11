#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <cstdlib>

#include "./framework/gl_header.h"
#include "./framework/gl_util.h"

// Simple particle module (ported from gaslight-runner's confetti.h).
// Designed to be extended with more effects later; currently supports Confetti bursts.
struct Particles
{
    struct ParticleVertex
    {
        glm::vec3 position;
        glm::vec3 velocity;
        glm::vec4 color;
        float life;
    };

    static constexpr int CONFETTI_PARTICLES = 200;

    GLuint shader = 0;
    GLuint vao = 0;
    GLuint vbo = 0;

    std::vector<ParticleVertex> verts;
    float time = 1000.0f;
    glm::mat4 modelToWorld = glm::mat4(1.0f);

    static const char *VS;
    static const char *FS;

    void init()
    {
        regenerateConfettiVerts();
        time = 1000.0f;

        shader = vtx::createShaderProgram(VS, FS);

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(
            GL_ARRAY_BUFFER,
            sizeof(ParticleVertex) * verts.size(),
            nullptr,
            GL_DYNAMIC_DRAW
        );

        glVertexAttribPointer(
            0, (int)(sizeof(ParticleVertex::position) / sizeof(float)), GL_FLOAT, GL_FALSE,
            sizeof(ParticleVertex), (void *)offsetof(ParticleVertex, position)
        );
        glVertexAttribPointer(
            1, (int)(sizeof(ParticleVertex::velocity) / sizeof(float)), GL_FLOAT, GL_FALSE,
            sizeof(ParticleVertex), (void *)offsetof(ParticleVertex, velocity)
        );
        glVertexAttribPointer(
            2, (int)(sizeof(ParticleVertex::color) / sizeof(float)), GL_FLOAT, GL_FALSE,
            sizeof(ParticleVertex), (void *)offsetof(ParticleVertex, color)
        );
        glVertexAttribPointer(
            3, (int)(sizeof(float) / sizeof(float)), GL_FLOAT, GL_FALSE, sizeof(ParticleVertex),
            (void *)offsetof(ParticleVertex, life)
        );

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(3);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void burstConfetti(const glm::vec3 &worldPos)
    {
        time = 0.0f;
        modelToWorld = glm::translate(glm::mat4(1.0f), worldPos);
        regenerateConfettiVerts();

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(
            GL_ARRAY_BUFFER,
            0,
            sizeof(ParticleVertex) * verts.size(),
            verts.data()
        );
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void draw(float deltaTime, const glm::mat4 &view, const glm::mat4 &proj)
    {
        if (!shader || !vao)
            return;

        time += deltaTime;
        // Confetti is short-lived; don't keep drawing forever.
        if (time > 3.0f)
            return;

        glUseProgram(shader);
        glBindVertexArray(vao);

        glUniformMatrix4fv(glGetUniformLocation(shader, "u_worldToView"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader, "u_projection"), 1, GL_FALSE, glm::value_ptr(proj));
        glUniformMatrix4fv(glGetUniformLocation(shader, "u_modelToWorld"), 1, GL_FALSE, glm::value_ptr(modelToWorld));
        glUniform1f(glGetUniformLocation(shader, "u_time"), time);

        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)verts.size());

        glBindVertexArray(0);
    }

  private:
    void regenerateConfettiVerts()
    {
        verts.clear();
        static int seed = 0;
        srand(seed++);

        const float size = 0.3f;
        for (int i = 0; i < CONFETTI_PARTICLES; i++)
        {
            glm::vec3 vel(
                (rand() % 100 - 50) / 50.0f,
                (rand() % 100) / 40.0f,
                (rand() % 100 - 50) / 50.0f
            );
            glm::vec4 color(
                (rand() / (float)RAND_MAX),
                (rand() / (float)RAND_MAX),
                (rand() / (float)RAND_MAX),
                1.0f
            );

            ParticleVertex p1 = {glm::vec3(-size, -size, 0.0f), vel, color, 1.0f};
            ParticleVertex p2 = {glm::vec3(size, -size, 0.0f), vel, color, 1.0f};
            ParticleVertex p3 = {glm::vec3(0.0f, size, 0.0f), vel, color, 1.0f};
            verts.push_back(p1);
            verts.push_back(p2);
            verts.push_back(p3);
        }
    }
};

const char *Particles::VS =
    GLSL_VERSION R"(
precision mediump float;

uniform float u_time;
uniform mat4 u_modelToWorld;
uniform mat4 u_worldToView;
uniform mat4 u_projection;

layout(location = 0) in vec3  a_position;
layout(location = 1) in vec3  a_velocity;
layout(location = 2) in vec4  a_color;
layout(location = 3) in float a_life;

out vec4 v_color;

void main() {
    float delta = u_time;
    vec3 currentVelocity = a_velocity * 2.0 + vec3(0.0, -9.8 * 0.2, 0.0) * delta;
    vec3 movedPos = a_position + currentVelocity * delta;
    v_color = a_color;
    vec3 worldPos = vec3(u_modelToWorld * vec4(movedPos, 1.0));
    gl_Position = u_projection * u_worldToView * vec4(worldPos, 1.0);
}
)";

const char *Particles::FS =
    GLSL_VERSION R"(
precision mediump float;
in vec4 v_color;
out vec4 FragColor;
void main() {
    FragColor = v_color;
}
)";

