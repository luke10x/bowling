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

    struct LaneDustParticle
    {
        glm::vec3 origin = glm::vec3(0.0f);
        glm::vec2 velocity = glm::vec2(0.0f);
        glm::vec4 color = glm::vec4(0.0f);
        float spawnTime = -10000.0f;
        float ttl = 0.0f;
        float size = 0.0f;
        float phase = 0.0f;
        bool active = false;
    };

    struct LaneDustVertex
    {
        glm::vec3 corner;
        glm::vec3 origin;
        glm::vec4 color;
        float spawnTime;
        float ttl;
        float size;
        glm::vec2 velocity;
        float phase;
    };

    struct BlockSparkParticle
    {
        glm::vec3 origin = glm::vec3(0.0f);
        glm::vec3 velocity = glm::vec3(0.0f);
        glm::vec4 color = glm::vec4(0.0f);
        float spawnTime = -10000.0f;
        float ttl = 0.0f;
        float size = 0.0f;
        float spin = 0.0f;
        float phase = 0.0f;
        bool active = false;
    };

    struct BlockSparkVertex
    {
        glm::vec3 corner;
        glm::vec3 origin;
        glm::vec4 color;
        float spawnTime;
        float ttl;
        float size;
        glm::vec3 velocity;
        float spin;
        float phase;
    };

    static constexpr int CONFETTI_PARTICLES = 200;
    static constexpr int SNOW_FLAKES = 220;
    static constexpr int BALL_TRACE_PARTICLES = 480;
    static constexpr float SNOW_SPAWN_INTERVAL = 1.25f;
    static constexpr int SNOW_REFRESH_STEPS = 16;
    static constexpr int SNOW_BATCH_SIZE = (SNOW_FLAKES + SNOW_REFRESH_STEPS - 1) / SNOW_REFRESH_STEPS;
    static constexpr float SNOW_MAX_SPIN_SPEED = 0.25f;
    static constexpr float SNOW_SPIN_APPROACH_RATE = 2.0f;
    static constexpr float BALL_TRACE_SPAWN_INTERVAL = 0.022f;
    static constexpr float BALL_TRACE_MAX_INITIAL_AGE = 0.18f;
    static constexpr int LANE_DUST_PARTICLES = 160;
    static constexpr float LANE_DUST_MAX_INITIAL_AGE = 0.05f;
    static constexpr int BLOCK_SPARK_PARTICLES = 140;

    GLuint shader = 0;
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint snowShader = 0;
    GLuint snowVao = 0;
    GLuint snowVbo = 0;
    GLuint ballTraceShader = 0;
    GLuint ballTraceVao = 0;
    GLuint ballTraceVbo = 0;
    GLuint laneDustShader = 0;
    GLuint laneDustVao = 0;
    GLuint laneDustVbo = 0;
    GLuint blockSparkShader = 0;
    GLuint blockSparkVao = 0;
    GLuint blockSparkVbo = 0;

    std::vector<ParticleVertex> verts;
    std::vector<Snowflake> snowflakes;
    std::vector<SnowVertex> snowVerts;
    std::vector<BallTraceParticle> ballTraceParticles;
    std::vector<BallTraceVertex> ballTraceVerts;
    std::vector<LaneDustParticle> laneDustParticles;
    std::vector<LaneDustVertex> laneDustVerts;
    std::vector<BlockSparkParticle> blockSparkParticles;
    std::vector<BlockSparkVertex> blockSparkVerts;
    float time = 1000.0f;
    float snowTime = 0.0f;
    float ballTraceTime = 0.0f;
    float laneDustTime = 0.0f;
    float blockSparkTime = 0.0f;
    float snowSpinRadians = 0.0f;
    float snowSpinVelocity = 0.0f;
    float snowSpawnTimer = 0.0f;
    float ballTraceSpawnTimer = 0.0f;
    unsigned int snowSeed = 4321u;
    unsigned int ballTraceSeed = 9876u;
    unsigned int laneDustSeed = 2468u;
    unsigned int blockSparkSeed = 13579u;
    int snowCursor = 0;
    int ballTraceCursor = 0;
    int laneDustCursor = 0;
    int blockSparkCursor = 0;
    int visibleSnowflakes = SNOW_FLAKES;
    int visibleBallTraceParticles = BALL_TRACE_PARTICLES;
    int visibleLaneDustParticles = LANE_DUST_PARTICLES;
    int visibleBlockSparkParticles = BLOCK_SPARK_PARTICLES;
    glm::mat4 modelToWorld = glm::mat4(1.0f);

    static const char *VS;
    static const char *FS;
    static const char *SNOW_VS;
    static const char *SNOW_FS;
    static const char *BALL_TRACE_VS;
    static const char *BALL_TRACE_FS;
    static const char *LANE_DUST_VS;
    static const char *LANE_DUST_FS;
    static const char *BLOCK_SPARK_VS;
    static const char *BLOCK_SPARK_FS;

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
        initLaneDust();
        initBlockSparks();
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
        GLboolean depthWasEnabled = glIsEnabled(GL_DEPTH_TEST);
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
        if (depthWasEnabled)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);
        if (!blendWasEnabled)
            glDisable(GL_BLEND);
    }

    void drawBallTrace(
        float deltaTime,
        const glm::vec3 &ballCenter,
        float intensity,
        bool allowSpawn,
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

        if (allowSpawn)
        {
            while (ballTraceSpawnTimer >= spawnInterval)
            {
                ballTraceSpawnTimer -= spawnInterval;
                spawnBallTraceBurst(ballCenter, clampedIntensity, burstCount, BALL_TRACE_MAX_INITIAL_AGE * 0.45f, false);
                spawned = true;
            }
        }

        GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
        GLboolean depthWasEnabled = glIsEnabled(GL_DEPTH_TEST);
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

    void drawLaneDust(float deltaTime, const glm::mat4 &view, const glm::mat4 &proj)
    {
        if (!laneDustShader || !laneDustVao || visibleLaneDustParticles <= 0)
            return;

        laneDustTime += deltaTime;

        GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
        GLboolean depthWasEnabled = glIsEnabled(GL_DEPTH_TEST);
        GLboolean depthMaskWasEnabled = GL_TRUE;
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMaskWasEnabled);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        glUseProgram(laneDustShader);
        glBindVertexArray(laneDustVao);

        glUniformMatrix4fv(glGetUniformLocation(laneDustShader, "u_worldToView"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(laneDustShader, "u_projection"), 1, GL_FALSE, glm::value_ptr(proj));
        glUniform1f(glGetUniformLocation(laneDustShader, "u_time"), laneDustTime);

        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(visibleLaneDustParticles * 6));

        glBindVertexArray(0);
        glDepthMask(depthMaskWasEnabled);
        if (depthWasEnabled)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);
        if (!blendWasEnabled)
            glDisable(GL_BLEND);
    }

    void drawBlockSparks(float deltaTime, const glm::mat4 &view, const glm::mat4 &proj)
    {
        if (!blockSparkShader || !blockSparkVao || visibleBlockSparkParticles <= 0)
            return;

        blockSparkTime += deltaTime;

        GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
        GLboolean depthWasEnabled = glIsEnabled(GL_DEPTH_TEST);
        GLboolean depthMaskWasEnabled = GL_TRUE;
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMaskWasEnabled);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        glDisable(GL_DEPTH_TEST);

        glUseProgram(blockSparkShader);
        glBindVertexArray(blockSparkVao);

        glUniformMatrix4fv(glGetUniformLocation(blockSparkShader, "u_worldToView"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(blockSparkShader, "u_projection"), 1, GL_FALSE, glm::value_ptr(proj));
        glUniform1f(glGetUniformLocation(blockSparkShader, "u_time"), blockSparkTime);

        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(visibleBlockSparkParticles * 6));

        glBindVertexArray(0);
        glDepthMask(depthMaskWasEnabled);
        if (depthWasEnabled)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);
        if (!blendWasEnabled)
            glDisable(GL_BLEND);
    }

    void burstLaneDustRipple(const glm::vec3 &center, float intensity)
    {
        const float clampedIntensity = glm::clamp(intensity, 0.0f, 1.0f);
        const int burstCount = glm::clamp(96 + (int)glm::round(clampedIntensity * 96.0f), 96, 192);
        spawnLaneDustBurst(center, clampedIntensity, burstCount, LANE_DUST_MAX_INITIAL_AGE, true);
    }

    void burstBlockSparks(
        const glm::vec3 &center,
        const glm::vec2 &awayDir,
        float intensity,
        const glm::vec4 &tint = glm::vec4(0.98f, 0.84f, 0.40f, 1.0f)
    )
    {
        const float clampedIntensity = glm::clamp(intensity, 0.0f, 1.0f);
        const int burstCount = glm::clamp(36 + (int)glm::round(clampedIntensity * 56.0f), 36, 92);
        spawnBlockSparkBurst(center, awayDir, clampedIntensity, burstCount, 0.08f, true, tint);
    }

    void burstBallTraceNos(
        const glm::vec3 &ballCenter,
        float intensity,
        bool freshOnly = false,
        float brightnessScale = 1.0f,
        float ttlScale = 1.0f,
        float sizeScale = 1.0f
    )
    {
        const float clampedIntensity = glm::clamp(intensity, 0.0f, 1.0f);
        // NOS is continuous, unlike gem pickup. Keep each refresh lighter so the pooled trace can
        // build a longer tail instead of constantly replacing itself with fresh particles.
        const int burstCount = glm::clamp(2 + (int)glm::round(clampedIntensity * 6.0f), 2, 8);
        const float maxInitialAge = freshOnly ? 0.0f : (BALL_TRACE_MAX_INITIAL_AGE * 0.12f);
        spawnBallTraceBurst(
            ballCenter,
            clampedIntensity,
            burstCount,
            maxInitialAge,
            true,
            brightnessScale,
            ttlScale,
            sizeScale
        );
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

    float laneDustRandom01()
    {
        laneDustSeed = laneDustSeed * 1664525u + 1013904223u;
        return ((laneDustSeed >> 8) & 0x00ffffff) / 16777215.0f;
    }

    float blockSparkRandom01()
    {
        blockSparkSeed = blockSparkSeed * 1664525u + 1013904223u;
        return ((blockSparkSeed >> 8) & 0x00ffffff) / 16777215.0f;
    }

    float randomRange(float minValue, float maxValue)
    {
        return minValue + (maxValue - minValue) * random01();
    }

    float ballTraceRandomRange(float minValue, float maxValue)
    {
        return minValue + (maxValue - minValue) * ballTraceRandom01();
    }

    float laneDustRandomRange(float minValue, float maxValue)
    {
        return minValue + (maxValue - minValue) * laneDustRandom01();
    }

    float blockSparkRandomRange(float minValue, float maxValue)
    {
        return minValue + (maxValue - minValue) * blockSparkRandom01();
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

    void initLaneDust()
    {
        laneDustShader = vtx::createShaderProgram(LANE_DUST_VS, LANE_DUST_FS);
        laneDustParticles.resize(LANE_DUST_PARTICLES);
        laneDustVerts.resize(LANE_DUST_PARTICLES * 6);
        laneDustTime = 0.0f;
        laneDustSeed = 2468u;
        laneDustCursor = 0;
        visibleLaneDustParticles = LANE_DUST_PARTICLES;

        glGenVertexArrays(1, &laneDustVao);
        glBindVertexArray(laneDustVao);

        glGenBuffers(1, &laneDustVbo);
        glBindBuffer(GL_ARRAY_BUFFER, laneDustVbo);
        glBufferData(
            GL_ARRAY_BUFFER,
            sizeof(LaneDustVertex) * laneDustVerts.size(),
            nullptr,
            GL_DYNAMIC_DRAW
        );

        glVertexAttribPointer(
            0, (int)(sizeof(LaneDustVertex::corner) / sizeof(float)), GL_FLOAT, GL_FALSE,
            sizeof(LaneDustVertex), (void *)offsetof(LaneDustVertex, corner)
        );
        glVertexAttribPointer(
            1, (int)(sizeof(LaneDustVertex::origin) / sizeof(float)), GL_FLOAT, GL_FALSE,
            sizeof(LaneDustVertex), (void *)offsetof(LaneDustVertex, origin)
        );
        glVertexAttribPointer(
            2, (int)(sizeof(LaneDustVertex::color) / sizeof(float)), GL_FLOAT, GL_FALSE,
            sizeof(LaneDustVertex), (void *)offsetof(LaneDustVertex, color)
        );
        glVertexAttribPointer(
            3, 1, GL_FLOAT, GL_FALSE, sizeof(LaneDustVertex),
            (void *)offsetof(LaneDustVertex, spawnTime)
        );
        glVertexAttribPointer(
            4, 1, GL_FLOAT, GL_FALSE, sizeof(LaneDustVertex),
            (void *)offsetof(LaneDustVertex, ttl)
        );
        glVertexAttribPointer(
            5, 1, GL_FLOAT, GL_FALSE, sizeof(LaneDustVertex),
            (void *)offsetof(LaneDustVertex, size)
        );
        glVertexAttribPointer(
            6, (int)(sizeof(LaneDustVertex::velocity) / sizeof(float)), GL_FLOAT, GL_FALSE,
            sizeof(LaneDustVertex), (void *)offsetof(LaneDustVertex, velocity)
        );
        glVertexAttribPointer(
            7, 1, GL_FLOAT, GL_FALSE, sizeof(LaneDustVertex),
            (void *)offsetof(LaneDustVertex, phase)
        );

        for (int i = 0; i < 8; i++)
            glEnableVertexAttribArray(i);

        uploadLaneDustVerts();

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void initBlockSparks()
    {
        blockSparkShader = vtx::createShaderProgram(BLOCK_SPARK_VS, BLOCK_SPARK_FS);
        blockSparkParticles.resize(BLOCK_SPARK_PARTICLES);
        blockSparkVerts.resize(BLOCK_SPARK_PARTICLES * 6);
        blockSparkTime = 0.0f;
        blockSparkSeed = 13579u;
        blockSparkCursor = 0;
        visibleBlockSparkParticles = BLOCK_SPARK_PARTICLES;

        glGenVertexArrays(1, &blockSparkVao);
        glBindVertexArray(blockSparkVao);

        glGenBuffers(1, &blockSparkVbo);
        glBindBuffer(GL_ARRAY_BUFFER, blockSparkVbo);
        glBufferData(
            GL_ARRAY_BUFFER,
            sizeof(BlockSparkVertex) * blockSparkVerts.size(),
            nullptr,
            GL_DYNAMIC_DRAW
        );

        glVertexAttribPointer(
            0, (int)(sizeof(BlockSparkVertex::corner) / sizeof(float)), GL_FLOAT, GL_FALSE,
            sizeof(BlockSparkVertex), (void *)offsetof(BlockSparkVertex, corner)
        );
        glVertexAttribPointer(
            1, (int)(sizeof(BlockSparkVertex::origin) / sizeof(float)), GL_FLOAT, GL_FALSE,
            sizeof(BlockSparkVertex), (void *)offsetof(BlockSparkVertex, origin)
        );
        glVertexAttribPointer(
            2, (int)(sizeof(BlockSparkVertex::color) / sizeof(float)), GL_FLOAT, GL_FALSE,
            sizeof(BlockSparkVertex), (void *)offsetof(BlockSparkVertex, color)
        );
        glVertexAttribPointer(
            3, 1, GL_FLOAT, GL_FALSE, sizeof(BlockSparkVertex),
            (void *)offsetof(BlockSparkVertex, spawnTime)
        );
        glVertexAttribPointer(
            4, 1, GL_FLOAT, GL_FALSE, sizeof(BlockSparkVertex),
            (void *)offsetof(BlockSparkVertex, ttl)
        );
        glVertexAttribPointer(
            5, 1, GL_FLOAT, GL_FALSE, sizeof(BlockSparkVertex),
            (void *)offsetof(BlockSparkVertex, size)
        );
        glVertexAttribPointer(
            6, (int)(sizeof(BlockSparkVertex::velocity) / sizeof(float)), GL_FLOAT, GL_FALSE,
            sizeof(BlockSparkVertex), (void *)offsetof(BlockSparkVertex, velocity)
        );
        glVertexAttribPointer(
            7, 1, GL_FLOAT, GL_FALSE, sizeof(BlockSparkVertex),
            (void *)offsetof(BlockSparkVertex, spin)
        );
        for (int i = 0; i < 8; i++)
            glEnableVertexAttribArray(i);

        uploadBlockSparkVerts();

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

    int reusableLaneDustSlot()
    {
        if (visibleLaneDustParticles <= 0)
            return 0;

        for (int i = 0; i < visibleLaneDustParticles; i++)
        {
            const LaneDustParticle &dust = laneDustParticles[i];
            if (!dust.active || laneDustTime - dust.spawnTime > dust.ttl)
                return i;
        }

        int slot = laneDustCursor;
        laneDustCursor = (laneDustCursor + 1) % visibleLaneDustParticles;
        return slot;
    }

    int reusableBlockSparkSlot()
    {
        if (visibleBlockSparkParticles <= 0)
            return 0;

        for (int i = 0; i < visibleBlockSparkParticles; i++)
        {
            const BlockSparkParticle &spark = blockSparkParticles[i];
            if (!spark.active || blockSparkTime - spark.spawnTime > spark.ttl)
                return i;
        }

        int slot = blockSparkCursor;
        blockSparkCursor = (blockSparkCursor + 1) % visibleBlockSparkParticles;
        return slot;
    }

    void spawnBallTraceBurst(
        const glm::vec3 &ballCenter,
        float intensity,
        int count,
        float maxInitialAge,
        bool upload,
        float brightnessScale = 1.0f,
        float ttlScale = 1.0f,
        float sizeScale = 1.0f
    )
    {
        if (visibleBallTraceParticles <= 0)
            return;

        count = glm::clamp(count, 0, visibleBallTraceParticles);
        const float brightness = glm::clamp(brightnessScale, 0.15f, 1.0f);
        const float ttlMul = glm::clamp(ttlScale, 0.20f, 1.5f);
        const float sizeMul = glm::clamp(sizeScale, 0.20f, 1.5f);
        for (int i = 0; i < count; i++)
        {
            BallTraceParticle &trail = ballTraceParticles[reusableBallTraceSlot()];
            const float pulse = glm::clamp(intensity, 0.0f, 1.0f);
            const float xJitter = ballTraceRandomRange(-0.036f, 0.036f) * (0.5f + pulse);
            const float yJitter = ballTraceRandomRange(-0.036f, 0.036f) * (0.5f + pulse);
            trail.origin = ballCenter + glm::vec3(xJitter, yJitter, 0.0f);
            trail.color = glm::vec4(
                ballTraceRandomRange(0.80f, 1.0f) * brightness,
                ballTraceRandomRange(0.90f, 1.0f) * brightness,
                ballTraceRandomRange(0.82f, 1.0f) * brightness,
                ballTraceRandomRange(0.26f, 0.62f) * (0.35f + 0.65f * pulse) * glm::mix(0.75f, 1.0f, brightness)
            );
            trail.ttl = ballTraceRandomRange(0.12f, 0.32f) * (0.70f + 0.50f * pulse) * ttlMul;
            trail.size = ballTraceRandomRange(0.006f, 0.016f) * (0.65f + 0.75f * pulse) * sizeMul;
            trail.drift = glm::vec2(
                ballTraceRandomRange(-0.08f, 0.08f),
                ballTraceRandomRange(-0.04f, 0.08f)
            ) * (0.35f + 0.85f * pulse) * glm::mix(0.85f, 1.0f, sizeMul);
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

    void spawnLaneDustBurst(
        const glm::vec3 &center,
        float intensity,
        int count,
        float maxInitialAge,
        bool upload
    )
    {
        if (visibleLaneDustParticles <= 0)
            return;

        count = glm::clamp(count, 0, visibleLaneDustParticles);
        const float pulse = glm::clamp(intensity, 0.0f, 1.0f);
        for (int i = 0; i < count; i++)
        {
            LaneDustParticle &dust = laneDustParticles[reusableLaneDustSlot()];
            const float angle = laneDustRandomRange(0.0f, glm::two_pi<float>());
            const float ringRadius = laneDustRandomRange(0.015f, 0.055f) * (0.70f + 0.55f * pulse);
            const float speed = laneDustRandomRange(0.55f, 1.55f) * (0.70f + 0.90f * pulse);
            const float outward = laneDustRandomRange(0.85f, 1.0f);
            const glm::vec2 dir = glm::vec2(cosf(angle), sinf(angle));
            dust.origin = glm::vec3(
                center.x + dir.x * ringRadius,
                center.y + laneDustRandomRange(0.002f, 0.010f),
                center.z + dir.y * ringRadius
            );
            dust.velocity = dir * speed * outward;
            dust.color = glm::vec4(
                laneDustRandomRange(0.80f, 0.96f),
                laneDustRandomRange(0.72f, 0.88f),
                laneDustRandomRange(0.58f, 0.76f),
                laneDustRandomRange(0.35f, 0.80f) * (0.70f + 0.85f * pulse)
            );
            dust.ttl = laneDustRandomRange(0.44f, 0.92f) * (0.90f + 0.55f * pulse);
            dust.size = laneDustRandomRange(0.012f, 0.028f) * (0.95f + 0.65f * pulse);
            dust.phase = laneDustRandomRange(0.0f, 6.2831853f);

            float initialAge = maxInitialAge > 0.0f ? laneDustRandomRange(0.0f, maxInitialAge) : 0.0f;
            if (initialAge > dust.ttl - 0.04f)
                initialAge = glm::max(0.0f, dust.ttl - 0.04f);
            dust.spawnTime = laneDustTime - initialAge;
            dust.active = true;
        }

        if (upload)
            uploadLaneDustVerts();
    }

    void uploadLaneDustVerts()
    {
        static const glm::vec3 corners[6] = {
            glm::vec3(-1.0f, -1.0f, 0.0f),
            glm::vec3(1.0f, -1.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 0.0f),
            glm::vec3(-1.0f, -1.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 0.0f),
            glm::vec3(-1.0f, 1.0f, 0.0f),
        };

        for (int i = 0; i < visibleLaneDustParticles; i++)
        {
            const LaneDustParticle &dust = laneDustParticles[i];
            for (int v = 0; v < 6; v++)
            {
                LaneDustVertex vertex = {};
                vertex.corner = corners[v];
                vertex.origin = dust.origin;
                vertex.color = dust.active ? dust.color : glm::vec4(0.0f);
                vertex.spawnTime = dust.spawnTime;
                vertex.ttl = dust.ttl;
                vertex.size = dust.size;
                vertex.velocity = dust.velocity;
                vertex.phase = dust.phase;
                laneDustVerts[i * 6 + v] = vertex;
            }
        }

        glBindVertexArray(laneDustVao);
        glBindBuffer(GL_ARRAY_BUFFER, laneDustVbo);
        glBufferSubData(
            GL_ARRAY_BUFFER,
            0,
            sizeof(LaneDustVertex) * visibleLaneDustParticles * 6,
            laneDustVerts.data()
        );
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void spawnBlockSparkBurst(
        const glm::vec3 &center,
        const glm::vec2 &awayDir,
        float intensity,
        int count,
        float maxInitialAge,
        bool upload,
        const glm::vec4 &tint
    )
    {
        if (visibleBlockSparkParticles <= 0)
            return;

        glm::vec2 dir2 = awayDir;
        if (!std::isfinite(dir2.x) || !std::isfinite(dir2.y) || glm::dot(dir2, dir2) < 1e-6f)
            dir2 = glm::vec2(0.0f, 1.0f);
        dir2 = glm::normalize(dir2);

        glm::vec3 axis = glm::normalize(glm::vec3(dir2.x, 0.28f, dir2.y));
        if (!std::isfinite(axis.x) || !std::isfinite(axis.y) || !std::isfinite(axis.z))
            axis = glm::vec3(0.0f, 0.45f, 1.0f);
        glm::vec3 helper = glm::abs(axis.y) < 0.92f ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
        glm::vec3 tangent = glm::normalize(glm::cross(helper, axis));
        glm::vec3 bitangent = glm::cross(axis, tangent);

        count = glm::clamp(count, 0, visibleBlockSparkParticles);
        const float pulse = glm::clamp(intensity, 0.0f, 1.0f);
        for (int i = 0; i < count; i++)
        {
            BlockSparkParticle &spark = blockSparkParticles[reusableBlockSparkSlot()];
            const float u = blockSparkRandom01();
            const float v = blockSparkRandom01();
            const float cosTheta = glm::pow(u, 0.42f);
            const float sinTheta = glm::sqrt(glm::max(0.0f, 1.0f - cosTheta * cosTheta));
            const float phi = glm::two_pi<float>() * v;
            glm::vec3 launchDir = axis * cosTheta +
                                  tangent * (cosf(phi) * sinTheta) +
                                  bitangent * (sinf(phi) * sinTheta);
            if (launchDir.y < 0.0f)
                launchDir.y *= 0.35f;
            launchDir = glm::normalize(launchDir);

            const float startOffset = blockSparkRandomRange(0.004f, 0.028f) * (0.75f + 0.6f * pulse);
            const float lateral = blockSparkRandomRange(-0.018f, 0.018f) * (0.55f + 0.55f * pulse);
            const float vertical = blockSparkRandomRange(-0.012f, 0.034f) * (0.55f + 0.55f * pulse);
            spark.origin = center + launchDir * startOffset + tangent * lateral + glm::vec3(0.0f, vertical, 0.0f);
            const float speed = blockSparkRandomRange(1.55f, 3.30f) * (0.80f + 0.75f * pulse);
            spark.velocity = launchDir * speed;
            const glm::vec3 tintRgb = glm::clamp(glm::vec3(tint), glm::vec3(0.0f), glm::vec3(1.0f));
            const float tintStrength = glm::clamp(tint.a, 0.0f, 1.0f);
            spark.color = glm::vec4(
                glm::mix(blockSparkRandomRange(0.82f, 1.0f), tintRgb.x, tintStrength),
                glm::mix(blockSparkRandomRange(0.68f, 0.96f), tintRgb.y, tintStrength),
                glm::mix(blockSparkRandomRange(0.26f, 0.62f), tintRgb.z, tintStrength),
                blockSparkRandomRange(0.52f, 0.96f) * (0.65f + 0.70f * pulse)
            );
            spark.ttl = blockSparkRandomRange(0.22f, 0.58f) * (0.85f + 0.55f * pulse);
            spark.size = blockSparkRandomRange(0.010f, 0.024f) * (0.85f + 0.55f * pulse);
            spark.spin = blockSparkRandomRange(-8.0f, 8.0f);
            spark.phase = blockSparkRandomRange(0.0f, 6.2831853f);

            float initialAge = maxInitialAge > 0.0f ? blockSparkRandomRange(0.0f, maxInitialAge) : 0.0f;
            if (initialAge > spark.ttl - 0.03f)
                initialAge = glm::max(0.0f, spark.ttl - 0.03f);
            spark.spawnTime = blockSparkTime - initialAge;
            spark.active = true;
        }

        if (upload)
            uploadBlockSparkVerts();
    }

    void uploadBlockSparkVerts()
    {
        static const glm::vec3 corners[6] = {
            glm::vec3(-1.0f, -1.0f, 0.0f),
            glm::vec3(1.0f, -1.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 0.0f),
            glm::vec3(-1.0f, -1.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 0.0f),
            glm::vec3(-1.0f, 1.0f, 0.0f),
        };

        for (int i = 0; i < visibleBlockSparkParticles; i++)
        {
            const BlockSparkParticle &spark = blockSparkParticles[i];
            for (int v = 0; v < 6; v++)
            {
                BlockSparkVertex vertex = {};
                vertex.corner = corners[v];
                vertex.origin = spark.origin;
                vertex.color = spark.active ? spark.color : glm::vec4(0.0f);
                vertex.spawnTime = spark.spawnTime;
                vertex.ttl = spark.ttl;
                vertex.size = spark.size;
                vertex.velocity = spark.velocity;
                vertex.spin = spark.spin;
                vertex.phase = spark.phase;
                blockSparkVerts[i * 6 + v] = vertex;
            }
        }

        glBindVertexArray(blockSparkVao);
        glBindBuffer(GL_ARRAY_BUFFER, blockSparkVbo);
        glBufferSubData(
            GL_ARRAY_BUFFER,
            0,
            sizeof(BlockSparkVertex) * visibleBlockSparkParticles * 6,
            blockSparkVerts.data()
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
    float fadeIn = smoothstep(0.0, 0.02, age);
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

const char *Particles::LANE_DUST_VS =
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
layout(location = 6) in vec2  a_velocity;
layout(location = 7) in float a_phase;

out vec4 v_color;

void main() {
    float age = max(u_time - a_spawnTime, 0.0);
    float alive = step(age, a_ttl) * step(0.0, a_ttl);
    float fadeIn = smoothstep(0.0, 0.03, age);
    float fadeOut = 1.0 - smoothstep(max(a_ttl - 0.18, 0.0), a_ttl, age);
    float travel = age * age * 1.0 + age * 0.28;
    vec2 drift = a_velocity * travel;
    vec2 wobble = vec2(
        sin(age * 18.0 + a_phase) * 0.010,
        cos(age * 16.0 + a_phase) * 0.010
    );
    vec3 worldPos = vec3(
        a_origin.x + drift.x + wobble.x + a_corner.x * a_size * 0.6,
        a_origin.y + sin(age * 6.0 + a_phase) * 0.003,
        a_origin.z + drift.y + wobble.y + a_corner.y * a_size * 0.6
    );

    v_color = vec4(a_color.rgb, a_color.a * fadeIn * fadeOut * alive);
    gl_Position = u_projection * u_worldToView * vec4(worldPos, 1.0);
}
)";

const char *Particles::LANE_DUST_FS =
    GLSL_VERSION R"(
precision mediump float;
in vec4 v_color;
out vec4 FragColor;
void main() {
    FragColor = v_color;
}
)";

const char *Particles::BLOCK_SPARK_VS =
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
layout(location = 6) in vec3  a_velocity;
layout(location = 7) in float a_spin;

out vec4 v_color;

void main() {
    float age = max(u_time - a_spawnTime, 0.0);
    float alive = step(age, a_ttl) * step(0.0, a_ttl);
    float fadeIn = smoothstep(0.0, 0.03, age);
    float fadeOut = 1.0 - smoothstep(max(a_ttl - 0.12, 0.0), a_ttl, age);

    float angle = age * a_spin;
    mat2 rot = mat2(cos(angle), -sin(angle), sin(angle), cos(angle));
    vec2 spunCorner = rot * a_corner.xy;
    vec3 drift = a_velocity * (age * 0.95 + age * age * 0.55);
    drift.y -= age * age * 1.45;
    vec3 worldPos = a_origin + drift;
    worldPos += vec3(spunCorner.x * a_size, sin(age * 14.0) * 0.004 + spunCorner.y * a_size, 0.0);

    v_color = vec4(a_color.rgb, a_color.a * fadeIn * fadeOut * alive);
    gl_Position = u_projection * u_worldToView * vec4(worldPos, 1.0);
}
)";

const char *Particles::BLOCK_SPARK_FS =
    GLSL_VERSION R"(
precision mediump float;
in vec4 v_color;
out vec4 FragColor;
void main() {
    FragColor = v_color;
}
)";
