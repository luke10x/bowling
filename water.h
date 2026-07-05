#pragma once

#include "framework/gl_header.h"
#include <glm/glm.hpp>
#include <cmath>
#include "framework/gl_util.h"

struct Water
{
    static const char *WATER_VERTEX_SHADER;
    static const char *WATER_FRAGMENT_SHADER;

    GLuint waterVAO = 0;
    GLuint waterShaderId = 0;
    float time = 0.0f;
    float travel = 0.0f;

    static constexpr float kWaterChannelWidthMeters = 20.0f;

    void initWater()
    {
        this->time = 0.0f;
        this->travel = 0.0f;
        this->loadWaterShader();

        const GLfloat fullscreenQuadVertices[] = {
            -1.0f, -1.0f, 1.0f, 0.0f, 0.0f,
             1.0f, -1.0f, 0.998f, 1.0f, 0.0f,
            -1.0f,  1.0f, 0.998f, 0.0f, 1.0f,
             1.0f,  1.0f, 0.998f, 1.0f, 1.0f
        };

        const GLuint fullscreenQuadIndices[] = {0, 1, 2, 1, 3, 2};

        GLuint vbo = 0;
        GLuint ebo = 0;
        glGenVertexArrays(1, &this->waterVAO);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(this->waterVAO);
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

    void loadWaterShader()
    {
        this->waterShaderId = vtx::createShaderProgram(WATER_VERTEX_SHADER, WATER_FRAGMENT_SHADER);
    }

    void renderWater(
        float deltaTime,
        const glm::mat4 cameraMatrix,
        const glm::mat4 projectionMatrix,
        float biomeStyle)
    {
        glUseProgram(this->waterShaderId);

        glm::vec3 forward = glm::normalize(glm::vec3(
            cameraMatrix[0][2], cameraMatrix[1][2], cameraMatrix[2][2]));
        float yaw = atan2(forward.x, forward.z);
        float pitch = asin(glm::clamp(forward.y, -1.0f, 1.0f));
        const glm::mat4 viewMatrix = glm::inverse(cameraMatrix);
        const glm::vec4 originClip = projectionMatrix * viewMatrix * glm::vec4(0.0f, 0.0f, 200.0f, 1.0f);
        float horizonY = 0.50f;
        if (std::abs(originClip.w) > 1.0e-5f)
        {
            const float ndcY = originClip.y / originClip.w;
            horizonY = glm::clamp(ndcY * 0.5f + 0.5f, 0.05f, 0.95f);
        }

        this->time += deltaTime;
        this->travel += deltaTime * 0.08f;

        glUniform1f(glGetUniformLocation(this->waterShaderId, "uYaw"), yaw);
        glUniform1f(glGetUniformLocation(this->waterShaderId, "uPitch"), pitch);
        glUniform1f(glGetUniformLocation(this->waterShaderId, "uHorizonY"), horizonY);
        glUniform1f(glGetUniformLocation(this->waterShaderId, "uTime"), this->time);
        glUniform1f(glGetUniformLocation(this->waterShaderId, "uTravel"), this->travel);
        glUniform1f(glGetUniformLocation(this->waterShaderId, "uBiomeStyle"), biomeStyle);
        glUniform1f(glGetUniformLocation(this->waterShaderId, "uValleyWidth"), kWaterChannelWidthMeters);

        glBindVertexArray(this->waterVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
};

const char *Water::WATER_VERTEX_SHADER = GLSL_VERSION R"(
    precision highp float;
    layout(location = 0) in vec3 aPos;
    layout(location = 1) in vec2 aTexCoord;
    out vec2 TexCoord;
    void main() {
        TexCoord = aTexCoord;
        gl_Position = vec4(aPos, 1.0);
    }
)";

const char *Water::WATER_FRAGMENT_SHADER = GLSL_VERSION R"(
    precision highp float;
    in vec2 TexCoord;
    out vec4 FragColor;

    uniform float uYaw;
    uniform float uPitch;
    uniform float uHorizonY;
    uniform float uTime;
    uniform float uTravel;
    uniform float uBiomeStyle;
    uniform float uValleyWidth;

    float hash12(vec2 p)
    {
        vec3 p3 = fract(vec3(p.xyx) * 0.1031);
        p3 += dot(p3, p3.yzx + 33.33);
        return fract((p3.x + p3.y) * p3.z);
    }

    float valueNoise(vec2 p)
    {
        vec2 i = floor(p);
        vec2 f = fract(p);
        f = f * f * (3.0 - 2.0 * f);
        float a = hash12(i);
        float b = hash12(i + vec2(1.0, 0.0));
        float c = hash12(i + vec2(0.0, 1.0));
        float d = hash12(i + vec2(1.0, 1.0));
        return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
    }

    float fbm(vec2 p)
    {
        float sum = 0.0;
        float amp = 0.5;
        for (int i = 0; i < 4; ++i)
        {
            sum += valueNoise(p) * amp;
            p = p * 2.03 + vec2(17.0, 9.0);
            amp *= 0.5;
        }
        return sum;
    }

    vec3 floorColorForStyle(float style)
    {
        if (style < 0.5) return vec3(0.09, 0.24, 0.36);
        if (style < 1.5) return vec3(0.11, 0.28, 0.43);
        return vec3(0.24, 0.48, 0.66);
    }

    vec3 wallColorForStyle(float style)
    {
        if (style < 0.5) return vec3(0.14, 0.32, 0.44);
        if (style < 1.5) return vec3(0.16, 0.37, 0.50);
        return vec3(0.31, 0.57, 0.73);
    }

    void main()
    {
        vec2 uv = TexCoord;
        float horizon = uHorizonY;
        float belowMask = 1.0 - smoothstep(horizon - 0.01, horizon + 0.03, uv.y);
        if (belowMask <= 0.0001)
        {
            FragColor = vec4(0.0);
            return;
        }

        float depth01 = clamp((horizon - uv.y) / max(horizon, 0.001), 0.0, 1.0);
        float farT = 1.0 - depth01;
        float worldZ = uTravel + farT * 320.0;
        float widthMeters = mix(140.0, 18.0, farT);
        float worldX = (uv.x - 0.5) * widthMeters + uYaw * mix(34.0, 5.0, farT);

        float centerNoise = (fbm(vec2(worldZ * 0.006, 3.0)) - 0.5) * 8.0;
        float valleyHalfWidth = 0.5 * uValleyWidth + (fbm(vec2(worldZ * 0.01, 11.0)) - 0.5) * 3.2;
        float dx = abs(worldX - centerNoise);
        float wallT = smoothstep(valleyHalfWidth - 1.0, valleyHalfWidth + 12.0, dx);

        float floorNoise = fbm(vec2(worldX * 0.035, worldZ * 0.02));
        float wallNoise = fbm(vec2(worldX * 0.05 + 4.0, worldZ * 0.035 + 9.0));
        float valleyLine = smoothstep(0.0, valleyHalfWidth * 0.65, dx);

        vec3 floorColor = floorColorForStyle(uBiomeStyle);
        vec3 wallColor = wallColorForStyle(uBiomeStyle);
        floorColor *= mix(0.75, 1.10, floorNoise);
        wallColor *= mix(0.82, 1.12, wallNoise);

        vec3 terrain = mix(floorColor, wallColor, wallT);
        terrain *= mix(1.08, 0.86, valleyLine * 0.4);

        float ridgeGlow = smoothstep(valleyHalfWidth + 8.0, valleyHalfWidth + 0.5, dx);
        terrain += vec3(0.03, 0.07, 0.10) * ridgeGlow;

        float distanceFog = mix(0.98, 0.62, farT);
        terrain *= distanceFog;

        vec3 deepOcean = vec3(0.03, 0.14, 0.28);
        terrain = mix(terrain, deepOcean, smoothstep(0.58, 0.96, farT) * 0.55);

        float grazingBand = 1.0 - smoothstep(0.03, 0.09, abs(depth01 - 0.45));
        float waterShimmer = 0.92 + 0.08 * fbm(vec2(worldX * 0.045 - uTime * 0.04, worldZ * 0.028));
        terrain *= waterShimmer;
        terrain += vec3(0.02, 0.05, 0.08) * grazingBand;

        float alpha = belowMask * (0.94 - 0.10 * grazingBand);
        FragColor = vec4(terrain, clamp(alpha, 0.0, 1.0));
    }
)";
