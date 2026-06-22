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

    struct BallTraceParticle
    {
        glm::vec3 origin = glm::vec3(0.0f);
        glm::vec4 color = glm::vec4(0.0f);
        float spawnTime = -10000.0f;
        float ttl = 0.0f;
        float size = 0.0f;
        glm::vec2 drift = glm::vec2(0.0f);
        float phase = 0.0f;
        bool active = false;
    };

    struct BallTraceVertex
    {
        glm::vec3 corner;
        glm::vec3 origin;
        glm::vec4 color;
        float spawnTime;
        float ttl;
        float size;
        glm::vec2 drift;
        float phase;
    };

    static constexpr int CONFETTI_PARTICLES = 200;
    static constexpr int SNOW_FLAKES = 220;
    static constexpr int BALL_TRACE_PARTICLES = 180;
    static constexpr float SNOW_SPAWN_INTERVAL = 1.25f;
    static constexpr int SNOW_REFRESH_STEPS = 16;
    static constexpr int SNOW_BATCH_SIZE = (SNOW_FLAKES + SNOW_REFRESH_STEPS - 1) / SNOW_REFRESH_STEPS;
    static constexpr float SNOW_MAX_SPIN_SPEED = 0.25f;
    static constexpr float SNOW_SPIN_APPROACH_RATE = 2.0f;
    static constexpr float BALL_TRACE_SPAWN_INTERVAL = 0.022f;
    static constexpr float BALL_TRACE_MAX_INITIAL_AGE = 0.18f;

    GLuint shader = 0;
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint snowShader = 0;
    GLuint snowVao = 0;
    GLuint snowVbo = 0;
    GLuint ballTraceShader = 0;
    GLuint ballTraceVao = 0;
    GLuint ballTraceVbo = 0;

    std::vector<ParticleVertex> verts;
    std::vector<Snowflake> snowflakes;
    std::vector<SnowVertex> snowVerts;
    std::vector<BallTraceParticle> ballTraceParticles;
    std::vector<BallTraceVertex> ballTraceVerts;
    float time = 1000.0f;
    float snowTime = 0.0f;
    float ballTraceTime = 0.0f;
    float snowSpinRadians = 0.0f;
    float snowSpinVelocity = 0.0f;
    float snowSpawnTimer = 0.0f;
    float ballTraceSpawnTimer = 0.0f;
    unsigned int snowSeed = 4321u;
    unsigned int ballTraceSeed = 9876u;
    int snowCursor = 0;
    int ballTraceCursor = 0;
    int visibleSnowflakes = SNOW_FLAKES;
    int visibleBallTraceParticles = BALL_TRACE_PARTICLES;
    glm::mat4 modelToWorld = glm::mat4(1.0f);

    static const char *VS;
    static const char *FS;
    static const char *SNOW_VS;
    static const char *SNOW_FS;
    static const char *BALL_TRACE_VS;
    static const char *BALL_TRACE_FS;

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
        initBallTrace();
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
        if (!snowShader || !snowVao || visibleSnowflakes <= 0)
            return;

        snowTime += deltaTime;
        const float rawSpinVelocity = deltaTime > 1e-6f && std::isfinite(spinDeltaRadians)
            ? spinDeltaRadians / deltaTime
            : 0.0f;
        const float targetSpinVelocity = std::isfinite(rawSpinVelocity)
            ? glm::clamp(rawSpinVelocity, -SNOW_MAX_SPIN_SPEED, SNOW_MAX_SPIN_SPEED)
            : 0.0f;
        const float smoothing = 1.0f - expf(-deltaTime * SNOW_SPIN_APPROACH_RATE);
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

        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(visibleSnowflakes * 6));

        glBindVertexArray(0);
        glDepthMask(depthMaskWasEnabled);
        if (!blendWasEnabled)
            glDisable(GL_BLEND);
    }

    void drawBallTrace(
        float deltaTime,
        const glm::vec3 &ballCenter,
        float intensity,
        const glm::mat4 &view,
        const glm::mat4 &proj
    )
    {
        if (!ballTraceShader || !ballTraceVao || visibleBallTraceParticles <= 0)
            return;

        ballTraceTime += deltaTime;
        ballTraceSpawnTimer += deltaTime;

        const float clampedIntensity = glm::clamp(intensity, 0.0f, 1.0f);
        const float spawnInterval = glm::mix(0.035f, 0.010f, clampedIntensity);
        const int burstCount = glm::clamp(1 + (int)glm::round(clampedIntensity * 5.0f), 1, 6);
        bool spawned = false;

        while (ballTraceSpawnTimer >= spawnInterval)
        {
            ballTraceSpawnTimer -= spawnInterval;
            spawnBallTraceBurst(ballCenter, clampedIntensity, burstCount, BALL_TRACE_MAX_INITIAL_AGE * 0.45f, false);
            spawned = true;
        }

        GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
        GLboolean depthMaskWasEnabled = GL_TRUE;
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMaskWasEnabled);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        glUseProgram(ballTraceShader);
        glBindVertexArray(ballTraceVao);

        glUniformMatrix4fv(glGetUniformLocation(ballTraceShader, "u_worldToView"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(ballTraceShader, "u_projection"), 1, GL_FALSE, glm::value_ptr(proj));
        glUniform1f(glGetUniformLocation(ballTraceShader, "u_time"), ballTraceTime);

        if (spawned)
            uploadBallTraceVerts();

        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(visibleBallTraceParticles * 6));

        glBindVertexArray(0);
        glDepthMask(depthMaskWasEnabled);
        if (!blendWasEnabled)
            glDisable(GL_BLEND);
    }

    void burstBallTrace(const glm::vec3 &ballCenter, float intensity)
    {
        const float clampedIntensity = glm::clamp(intensity, 0.0f, 1.0f);
        const int burstCount = glm::clamp(4 + (int)glm::round(clampedIntensity * 12.0f), 4, 24);
        spawnBallTraceBurst(ballCenter, clampedIntensity, burstCount, BALL_TRACE_MAX_INITIAL_AGE, true);
    }

    void setSnowflakeCount(int count)
    {
        int clampedCount = glm::clamp(count, 0, SNOW_FLAKES);
        if (clampedCount == visibleSnowflakes)
            return;

        const int previousCount = visibleSnowflakes;
        visibleSnowflakes = clampedCount;
        snowCursor = visibleSnowflakes > 0 ? (snowCursor % visibleSnowflakes) : 0;
        snowSpawnTimer = 0.0f;

        if ((int)snowflakes.size() != SNOW_FLAKES)
            return;

        if (visibleSnowflakes < previousCount)
        {
            for (int i = visibleSnowflakes; i < previousCount; i++)
            {
                snowflakes[i].active = false;
            }
        }
        else
        {
            for (int i = previousCount; i < visibleSnowflakes; i++)
            {
                snowflakes[i] = Snowflake{};
            }
        }

        if (snowVao && snowVbo)
            uploadSnowVerts();
    }

  private:
    float random01()
    {
        snowSeed = snowSeed * 1664525u + 1013904223u;
        return ((snowSeed >> 8) & 0x00ffffff) / 16777215.0f;
    }

    float ballTraceRandom01()
    {
        ballTraceSeed = ballTraceSeed * 1664525u + 1013904223u;
        return ((ballTraceSeed >> 8) & 0x00ffffff) / 16777215.0f;
    }

    float randomRange(float minValue, float maxValue)
    {
        return minValue + (maxValue - minValue) * random01();
    }

    float ballTraceRandomRange(float minValue, float maxValue)
    {
        return minValue + (maxValue - minValue) * ballTraceRandom01();
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
        visibleSnowflakes = SNOW_FLAKES;

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

        for (int spawned = 0; spawned < visibleSnowflakes; spawned += SNOW_BATCH_SIZE)
            spawnSnowBatch(18.0f, false, glm::min(SNOW_BATCH_SIZE, visibleSnowflakes - spawned));
        uploadSnowVerts();

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void initBallTrace()
    {
        ballTraceShader = vtx::createShaderProgram(BALL_TRACE_VS, BALL_TRACE_FS);
        ballTraceParticles.resize(BALL_TRACE_PARTICLES);
        ballTraceVerts.resize(BALL_TRACE_PARTICLES * 6);
        ballTraceTime = 0.0f;
        ballTraceSpawnTimer = 0.0f;
        ballTraceSeed = 9876u;
        ballTraceCursor = 0;
        visibleBallTraceParticles = BALL_TRACE_PARTICLES;

        glGenVertexArrays(1, &ballTraceVao);
        glBindVertexArray(ballTraceVao);

        glGenBuffers(1, &ballTraceVbo);
        glBindBuffer(GL_ARRAY_BUFFER, ballTraceVbo);
        glBufferData(
            GL_ARRAY_BUFFER,
            sizeof(BallTraceVertex) * ballTraceVerts.size(),
            nullptr,
            GL_DYNAMIC_DRAW
        );

        glVertexAttribPointer(
            0, (int)(sizeof(BallTraceVertex::corner) / sizeof(float)), GL_FLOAT, GL_FALSE,
            sizeof(BallTraceVertex), (void *)offsetof(BallTraceVertex, corner)
        );
        glVertexAttribPointer(
            1, (int)(sizeof(BallTraceVertex::origin) / sizeof(float)), GL_FLOAT, GL_FALSE,
            sizeof(BallTraceVertex), (void *)offsetof(BallTraceVertex, origin)
        );
        glVertexAttribPointer(
            2, (int)(sizeof(BallTraceVertex::color) / sizeof(float)), GL_FLOAT, GL_FALSE,
            sizeof(BallTraceVertex), (void *)offsetof(BallTraceVertex, color)
        );
        glVertexAttribPointer(
            3, 1, GL_FLOAT, GL_FALSE, sizeof(BallTraceVertex),
            (void *)offsetof(BallTraceVertex, spawnTime)
        );
        glVertexAttribPointer(
            4, 1, GL_FLOAT, GL_FALSE, sizeof(BallTraceVertex),
            (void *)offsetof(BallTraceVertex, ttl)
        );
        glVertexAttribPointer(
            5, 1, GL_FLOAT, GL_FALSE, sizeof(BallTraceVertex),
            (void *)offsetof(BallTraceVertex, size)
        );
        glVertexAttribPointer(
            6, (int)(sizeof(BallTraceVertex::drift) / sizeof(float)), GL_FLOAT, GL_FALSE,
            sizeof(BallTraceVertex), (void *)offsetof(BallTraceVertex, drift)
        );
        glVertexAttribPointer(
            7, 1, GL_FLOAT, GL_FALSE, sizeof(BallTraceVertex),
            (void *)offsetof(BallTraceVertex, phase)
        );

        for (int i = 0; i < 8; i++)
            glEnableVertexAttribArray(i);

        uploadBallTraceVerts();

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    int reusableSnowSlot()
    {
        if (visibleSnowflakes <= 0)
            return 0;

        for (int i = 0; i < visibleSnowflakes; i++)
        {
            const Snowflake &snow = snowflakes[i];
            if (!snow.active || snowTime - snow.spawnTime > snow.ttl)
                return i;
        }

        int slot = snowCursor;
        snowCursor = (snowCursor + 1) % visibleSnowflakes;
        return slot;
    }

    void spawnSnowBatch(float maxInitialAge, bool upload, int count = SNOW_BATCH_SIZE)
    {
        if (visibleSnowflakes <= 0)
            return;

        count = glm::clamp(count, 0, visibleSnowflakes);
        for (int i = 0; i < count; i++)
        {
            Snowflake &snow = snowflakes[reusableSnowSlot()];
            snow.origin = glm::vec3(
                randomRange(-10.0f, 10.0f),
                randomRange(-10.0f, 10.0f),
                randomRange(-20.0f, 10.0f)
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

        for (int i = 0; i < visibleSnowflakes; i++)
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
            sizeof(SnowVertex) * visibleSnowflakes * 6,
            snowVerts.data()
        );
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    int reusableBallTraceSlot()
    {
        if (visibleBallTraceParticles <= 0)
            return 0;

        for (int i = 0; i < visibleBallTraceParticles; i++)
        {
            const BallTraceParticle &trail = ballTraceParticles[i];
            if (!trail.active || ballTraceTime - trail.spawnTime > trail.ttl)
                return i;
        }

        int slot = ballTraceCursor;
        ballTraceCursor = (ballTraceCursor + 1) % visibleBallTraceParticles;
        return slot;
    }

    void spawnBallTraceBurst(
        const glm::vec3 &ballCenter,
        float intensity,
        int count,
        float maxInitialAge,
        bool upload
    )
    {
        if (visibleBallTraceParticles <= 0)
            return;

        count = glm::clamp(count, 0, visibleBallTraceParticles);
        for (int i = 0; i < count; i++)
        {
            BallTraceParticle &trail = ballTraceParticles[reusableBallTraceSlot()];
            const float pulse = glm::clamp(intensity, 0.0f, 1.0f);
            const float xJitter = ballTraceRandomRange(-0.018f, 0.018f) * (0.5f + pulse);
            const float yJitter = ballTraceRandomRange(-0.018f, 0.018f) * (0.5f + pulse);
            trail.origin = ballCenter + glm::vec3(xJitter, yJitter, 0.0f);
            trail.color = glm::vec4(
                ballTraceRandomRange(0.80f, 1.0f),
                ballTraceRandomRange(0.90f, 1.0f),
                1.0f,
                ballTraceRandomRange(0.26f, 0.62f) * (0.35f + 0.65f * pulse)
            );
            trail.ttl = ballTraceRandomRange(0.12f, 0.32f) * (0.70f + 0.50f * pulse);
            trail.size = ballTraceRandomRange(0.006f, 0.016f) * (0.65f + 0.75f * pulse);
            trail.drift = glm::vec2(
                ballTraceRandomRange(-0.08f, 0.08f),
                ballTraceRandomRange(-0.04f, 0.08f)
            ) * (0.35f + 0.85f * pulse);
            trail.phase = ballTraceRandomRange(0.0f, 6.2831853f);

            float initialAge = maxInitialAge > 0.0f ? ballTraceRandomRange(0.0f, maxInitialAge) : 0.0f;
            if (initialAge > trail.ttl - 0.015f)
                initialAge = glm::max(0.0f, trail.ttl - 0.015f);
            trail.spawnTime = ballTraceTime - initialAge;
            trail.active = true;
        }

        if (upload)
            uploadBallTraceVerts();
    }

    void uploadBallTraceVerts()
    {
        static const glm::vec3 corners[6] = {
            glm::vec3(-1.0f, -1.0f, 0.0f),
            glm::vec3(1.0f, -1.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 0.0f),
            glm::vec3(-1.0f, -1.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 0.0f),
            glm::vec3(-1.0f, 1.0f, 0.0f),
        };

        for (int i = 0; i < visibleBallTraceParticles; i++)
        {
            const BallTraceParticle &trail = ballTraceParticles[i];
            for (int v = 0; v < 6; v++)
            {
                BallTraceVertex vertex = {};
                vertex.corner = corners[v];
                vertex.origin = trail.origin;
                vertex.color = trail.active ? trail.color : glm::vec4(0.0f);
                vertex.spawnTime = trail.spawnTime;
                vertex.ttl = trail.ttl;
                vertex.size = trail.size;
                vertex.drift = trail.drift;
                vertex.phase = trail.phase;
                ballTraceVerts[i * 6 + v] = vertex;
            }
        }

        glBindVertexArray(ballTraceVao);
        glBindBuffer(GL_ARRAY_BUFFER, ballTraceVbo);
        glBufferSubData(
            GL_ARRAY_BUFFER,
            0,
            sizeof(BallTraceVertex) * visibleBallTraceParticles * 6,
            ballTraceVerts.data()
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

const char *Particles::BALL_TRACE_VS =
    GLSL_VERSION R"(
precision mediump float;

uniform float u_time;
uniform mat4 u_worldToView;
uniform mat4 u_projection;

layout(location = 0) in vec3  a_corner;
layout(location = 1) in vec3  a_origin;
layout(location = 2) in vec4  a_color;
layout(location = 3) in float a_spawnTime;
layout(location = 4) in float a_ttl;
layout(location = 5) in float a_size;
layout(location = 6) in vec2  a_drift;
layout(location = 7) in float a_phase;

out vec4 v_color;

void main() {
    float age = max(u_time - a_spawnTime, 0.0);
    float alive = step(age, a_ttl) * step(0.0, a_ttl);
    float fadeIn = smoothstep(0.0, 0.06, age);
    float fadeOut = 1.0 - smoothstep(max(a_ttl - 0.10, 0.0), a_ttl, age);
    float ring = 0.65 + 0.35 * sin(age * 14.0 + a_phase);
    vec2 wobble = vec2(
        sin(age * 11.0 + a_phase) * 0.010,
        cos(age * 13.0 + a_phase) * 0.008
    );
    vec2 drift = a_drift * age + wobble;
    vec3 worldPos = vec3(
        a_origin.x + drift.x + a_corner.x * a_size,
        a_origin.y + drift.y + a_corner.y * a_size * ring,
        a_origin.z
    );

    v_color = vec4(a_color.rgb, a_color.a * fadeIn * fadeOut * alive);
    gl_Position = u_projection * u_worldToView * vec4(worldPos, 1.0);
}
)";

const char *Particles::BALL_TRACE_FS =
    GLSL_VERSION R"(
precision mediump float;
in vec4 v_color;
out vec4 FragColor;
void main() {
    FragColor = v_color;
}
)";
