#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <cstdlib>
#include <cstddef>

#include "./framework/gl_header.h"
#include "./framework/gl_util.h"

// Simple particle module (ported from gaslight-runner's confetti.h).
// Designed to be extended with more effects later; currently supports confetti bursts
// and a slow transparent lane snow effect.
struct Particles
{
    struct ParticleVertex
    {
        glm::vec3 position;
        glm::vec3 velocity;
        glm::vec4 color;
        float life;
    };

    struct SnowVertex
    {
        glm::vec3 corner;
        glm::vec3 origin;
        glm::vec4 color;
        float spawnTime;
        float ttl;
        float size;
        float fallSpeed;
        float phase;
    };

    struct Snowflake
    {
        glm::vec3 origin = glm::vec3(0.0f);
        glm::vec4 color = glm::vec4(0.0f);
        float spawnTime = -10000.0f;
        float ttl = 0.0f;
        float size = 0.0f;
        float fallSpeed = 0.0f;
        float phase = 0.0f;
        bool active = false;
    };

    static constexpr int CONFETTI_PARTICLES = 200;
    static constexpr int SNOW_FLAKES = 220;
    static constexpr int SNOW_BATCH_SIZE = 10;
    static constexpr float SNOW_SPAWN_INTERVAL = 1.25f;

    GLuint shader = 0;
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint snowShader = 0;
    GLuint snowVao = 0;
    GLuint snowVbo = 0;

    std::vector<ParticleVertex> verts;
    std::vector<Snowflake> snowflakes;
    std::vector<SnowVertex> snowVerts;
    float time = 1000.0f;
    float snowTime = 0.0f;
    float snowSpinRadians = 0.0f;
    float snowSpinVelocity = 0.0f;
    float snowSpawnTimer = 0.0f;
    unsigned int snowSeed = 4321u;
    int snowCursor = 0;
    glm::mat4 modelToWorld = glm::mat4(1.0f);

    static const char *VS;
    static const char *FS;
    static const char *SNOW_VS;
    static const char *SNOW_FS;

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

        initSnow();
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

    void drawSnow(float deltaTime, float spinDeltaRadians, const glm::mat4 &view, const glm::mat4 &proj)
    {
        if (!snowShader || !snowVao)
            return;

        snowTime += deltaTime;
        const float rawSpinVelocity = deltaTime > 1e-6f && std::isfinite(spinDeltaRadians)
            ? spinDeltaRadians / deltaTime
            : 0.0f;
        const float targetSpinVelocity = std::isfinite(rawSpinVelocity)
            ? glm::clamp(rawSpinVelocity, -1.0f, 1.0f)
            : 0.0f;
        const float smoothing = 1.0f - expf(-deltaTime * 4.0f);
        snowSpinVelocity += (targetSpinVelocity - snowSpinVelocity) * smoothing;
        snowSpinRadians += snowSpinVelocity * deltaTime;
        snowSpinRadians = std::isfinite(snowSpinRadians)
            ? std::fmod(snowSpinRadians, glm::two_pi<float>())
            : 0.0f;
        snowSpawnTimer += deltaTime;
        while (snowSpawnTimer >= SNOW_SPAWN_INTERVAL)
        {
            snowSpawnTimer -= SNOW_SPAWN_INTERVAL;
            spawnSnowBatch(0.0f, true);
        }

        GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
        GLboolean depthMaskWasEnabled = GL_TRUE;
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMaskWasEnabled);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        glUseProgram(snowShader);
        glBindVertexArray(snowVao);

        glUniformMatrix4fv(glGetUniformLocation(snowShader, "u_worldToView"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(snowShader, "u_projection"), 1, GL_FALSE, glm::value_ptr(proj));
        glUniform1f(glGetUniformLocation(snowShader, "u_time"), snowTime);
        glUniform1f(glGetUniformLocation(snowShader, "u_spinRadians"), snowSpinRadians);

        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)snowVerts.size());

        glBindVertexArray(0);
        glDepthMask(depthMaskWasEnabled);
        if (!blendWasEnabled)
            glDisable(GL_BLEND);
    }

  private:
    float random01()
    {
        snowSeed = snowSeed * 1664525u + 1013904223u;
        return ((snowSeed >> 8) & 0x00ffffff) / 16777215.0f;
    }

    float randomRange(float minValue, float maxValue)
    {
        return minValue + (maxValue - minValue) * random01();
    }

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

    void initSnow()
    {
        snowShader = vtx::createShaderProgram(SNOW_VS, SNOW_FS);
        snowflakes.resize(SNOW_FLAKES);
        snowVerts.resize(SNOW_FLAKES * 6);
        snowTime = 0.0f;
        snowSpinRadians = 0.0f;
        snowSpinVelocity = 0.0f;
        snowSpawnTimer = 0.0f;
        snowSeed = 4321u;
        snowCursor = 0;

        glGenVertexArrays(1, &snowVao);
        glBindVertexArray(snowVao);

        glGenBuffers(1, &snowVbo);
        glBindBuffer(GL_ARRAY_BUFFER, snowVbo);
        glBufferData(
            GL_ARRAY_BUFFER,
            sizeof(SnowVertex) * snowVerts.size(),
            nullptr,
            GL_DYNAMIC_DRAW
        );

        glVertexAttribPointer(
            0, (int)(sizeof(SnowVertex::corner) / sizeof(float)), GL_FLOAT, GL_FALSE,
            sizeof(SnowVertex), (void *)offsetof(SnowVertex, corner)
        );
        glVertexAttribPointer(
            1, (int)(sizeof(SnowVertex::origin) / sizeof(float)), GL_FLOAT, GL_FALSE,
            sizeof(SnowVertex), (void *)offsetof(SnowVertex, origin)
        );
        glVertexAttribPointer(
            2, (int)(sizeof(SnowVertex::color) / sizeof(float)), GL_FLOAT, GL_FALSE,
            sizeof(SnowVertex), (void *)offsetof(SnowVertex, color)
        );
        glVertexAttribPointer(
            3, 1, GL_FLOAT, GL_FALSE, sizeof(SnowVertex),
            (void *)offsetof(SnowVertex, spawnTime)
        );
        glVertexAttribPointer(
            4, 1, GL_FLOAT, GL_FALSE, sizeof(SnowVertex),
            (void *)offsetof(SnowVertex, ttl)
        );
        glVertexAttribPointer(
            5, 1, GL_FLOAT, GL_FALSE, sizeof(SnowVertex),
            (void *)offsetof(SnowVertex, size)
        );
        glVertexAttribPointer(
            6, 1, GL_FLOAT, GL_FALSE, sizeof(SnowVertex),
            (void *)offsetof(SnowVertex, fallSpeed)
        );
        glVertexAttribPointer(
            7, 1, GL_FLOAT, GL_FALSE, sizeof(SnowVertex),
            (void *)offsetof(SnowVertex, phase)
        );

        for (int i = 0; i < 8; i++)
            glEnableVertexAttribArray(i);

        for (int i = 0; i < 12; i++)
            spawnSnowBatch(18.0f, false);
        uploadSnowVerts();

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    int reusableSnowSlot()
    {
        for (int i = 0; i < SNOW_FLAKES; i++)
        {
            const Snowflake &snow = snowflakes[i];
            if (!snow.active || snowTime - snow.spawnTime > snow.ttl)
                return i;
        }

        int slot = snowCursor;
        snowCursor = (snowCursor + 1) % SNOW_FLAKES;
        return slot;
    }

    void spawnSnowBatch(float maxInitialAge, bool upload)
    {
        for (int i = 0; i < SNOW_BATCH_SIZE; i++)
        {
            Snowflake &snow = snowflakes[reusableSnowSlot()];
            snow.origin = glm::vec3(
                randomRange(-2.5f, 2.5f),
                randomRange(-2.5f, 2.5f),
                randomRange(-10.0f, 10.0f)
            );
            snow.color = glm::vec4(
                randomRange(0.78f, 1.0f),
                randomRange(0.86f, 1.0f),
                1.0f,
                randomRange(0.24f, 0.48f)
            );
            snow.ttl = randomRange(15.0f, 24.0f);
            snow.size = randomRange(0.01f, 0.0275f);
            snow.fallSpeed = randomRange(0.14f, 0.32f);
            snow.phase = randomRange(0.0f, 6.2831853f);

            float initialAge = maxInitialAge > 0.0f ? randomRange(0.0f, maxInitialAge) : 0.0f;
            if (initialAge > snow.ttl - 0.5f)
                initialAge = snow.ttl - 0.5f;
            snow.spawnTime = snowTime - initialAge;
            snow.active = true;
        }

        if (upload)
            uploadSnowVerts();
    }

    void uploadSnowVerts()
    {
        static const glm::vec3 corners[6] = {
            glm::vec3(-1.0f, -1.0f, 0.0f),
            glm::vec3(1.0f, -1.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 0.0f),
            glm::vec3(-1.0f, -1.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 0.0f),
            glm::vec3(-1.0f, 1.0f, 0.0f),
        };

        for (int i = 0; i < SNOW_FLAKES; i++)
        {
            const Snowflake &snow = snowflakes[i];
            for (int v = 0; v < 6; v++)
            {
                SnowVertex vertex = {};
                vertex.corner = corners[v];
                vertex.origin = snow.origin;
                vertex.color = snow.active ? snow.color : glm::vec4(0.0f);
                vertex.spawnTime = snow.spawnTime;
                vertex.ttl = snow.ttl;
                vertex.size = snow.size;
                vertex.fallSpeed = snow.fallSpeed;
                vertex.phase = snow.phase;
                snowVerts[i * 6 + v] = vertex;
            }
        }

        glBindVertexArray(snowVao);
        glBindBuffer(GL_ARRAY_BUFFER, snowVbo);
        glBufferSubData(
            GL_ARRAY_BUFFER,
            0,
            sizeof(SnowVertex) * snowVerts.size(),
            snowVerts.data()
        );
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
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

const char *Particles::SNOW_VS =
    GLSL_VERSION R"(
precision mediump float;

uniform float u_time;
uniform float u_spinRadians;
uniform mat4 u_worldToView;
uniform mat4 u_projection;

layout(location = 0) in vec3  a_corner;
layout(location = 1) in vec3  a_origin;
layout(location = 2) in vec4  a_color;
layout(location = 3) in float a_spawnTime;
layout(location = 4) in float a_ttl;
layout(location = 5) in float a_size;
layout(location = 6) in float a_fallSpeed;
layout(location = 7) in float a_phase;

out vec4 v_color;

void main() {
    float age = max(u_time - a_spawnTime, 0.0);
    float alive = step(age, a_ttl) * step(0.0, a_ttl);
    float fadeIn = smoothstep(0.0, 1.0, age);
    float fadeOut = 1.0 - smoothstep(max(a_ttl - 2.0, 0.0), a_ttl, age);

    mat2 spinRot = mat2(
        cos(u_spinRadians), -sin(u_spinRadians),
        sin(u_spinRadians),  cos(u_spinRadians)
    );

    vec2 fieldXY = spinRot * a_origin.xy;
    vec2 localXY = spinRot * (
        a_corner.xy * a_size +
        vec2(sin(age * 0.85 + a_phase) * 0.28, 0.0)
    );
    vec3 worldPos = vec3(
        fieldXY.x + localXY.x,
        fieldXY.y + localXY.y - age * a_fallSpeed,
        a_origin.z
    );

    vec3 closestLanePoint = vec3(
        clamp(worldPos.x, -0.531, 0.531),
        0.0,
        clamp(worldPos.z, -18.3, -5.0)
    );
    float laneDistance = length(worldPos - closestLanePoint);
    float laneFade = smoothstep(0.5, 1.0, laneDistance);

    v_color = vec4(a_color.rgb, a_color.a * fadeIn * fadeOut * laneFade * alive);
    gl_Position = u_projection * u_worldToView * vec4(worldPos, 1.0);
}
)";

const char *Particles::SNOW_FS =
    GLSL_VERSION R"(
precision mediump float;
in vec4 v_color;
out vec4 FragColor;
void main() {
    FragColor = v_color;
}
)";
