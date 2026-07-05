#pragma once

#include "framework/gl_header.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

#include "framework/gl_util.h"

struct WaterVertex
{
    glm::vec3 position;
    glm::vec2 uv;
};

struct Water
{
    static const char *WATER_VERTEX_SHADER;
    static const char *WATER_FRAGMENT_SHADER;

    GLuint waterVAO = 0;
    GLuint waterVBO = 0;
    GLuint waterEBO = 0;
    GLuint waterShaderId = 0;
    GLsizei indexCount = 0;
    float time = 0.0f;

    std::vector<WaterVertex> vertices;
    std::vector<uint32_t> indices;

    static constexpr int kGridX = 48;
    static constexpr int kGridZ = 128;
    static constexpr float kHalfWidthMeters = 120.0f;
    static constexpr float kNearZ = -50.0f;
    static constexpr float kFarZ = 440.0f;
    static constexpr float kHorizonAnchorZ = 200.0f;

    void initWater()
    {
        this->time = 0.0f;
        this->loadWaterShader();
        this->buildPlaneMesh();
    }

    void loadWaterShader()
    {
        this->waterShaderId = vtx::createShaderProgram(WATER_VERTEX_SHADER, WATER_FRAGMENT_SHADER);
    }

    void buildPlaneMesh()
    {
        this->vertices.clear();
        this->indices.clear();
        this->vertices.reserve((kGridX + 1) * (kGridZ + 1));
        this->indices.reserve(kGridX * kGridZ * 6);

        for (int z = 0; z <= kGridZ; ++z)
        {
            const float zT = float(z) / float(kGridZ);
            const float worldZ = glm::mix(kNearZ, kFarZ, zT);
            for (int x = 0; x <= kGridX; ++x)
            {
                const float xT = float(x) / float(kGridX);
                const float worldX = glm::mix(-kHalfWidthMeters, kHalfWidthMeters, xT);
                this->vertices.push_back({
                    glm::vec3(worldX, 0.0f, worldZ),
                    glm::vec2(xT, zT)
                });
            }
        }

        const int stride = kGridX + 1;
        for (int z = 0; z < kGridZ; ++z)
        {
            for (int x = 0; x < kGridX; ++x)
            {
                const uint32_t i0 = uint32_t(z * stride + x);
                const uint32_t i1 = i0 + 1;
                const uint32_t i2 = uint32_t((z + 1) * stride + x);
                const uint32_t i3 = i2 + 1;
                this->indices.push_back(i0);
                this->indices.push_back(i2);
                this->indices.push_back(i1);
                this->indices.push_back(i1);
                this->indices.push_back(i2);
                this->indices.push_back(i3);
            }
        }

        this->indexCount = GLsizei(this->indices.size());

        if (this->waterEBO != 0)
        {
            glDeleteBuffers(1, &this->waterEBO);
            this->waterEBO = 0;
        }
        if (this->waterVBO != 0)
        {
            glDeleteBuffers(1, &this->waterVBO);
            this->waterVBO = 0;
        }
        if (this->waterVAO != 0)
        {
            glDeleteVertexArrays(1, &this->waterVAO);
            this->waterVAO = 0;
        }

        glGenVertexArrays(1, &this->waterVAO);
        glGenBuffers(1, &this->waterVBO);
        glGenBuffers(1, &this->waterEBO);

        glBindVertexArray(this->waterVAO);
        glBindBuffer(GL_ARRAY_BUFFER, this->waterVBO);
        glBufferData(
            GL_ARRAY_BUFFER,
            GLsizeiptr(this->vertices.size() * sizeof(WaterVertex)),
            this->vertices.data(),
            GL_STATIC_DRAW
        );

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->waterEBO);
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            GLsizeiptr(this->indices.size() * sizeof(uint32_t)),
            this->indices.data(),
            GL_STATIC_DRAW
        );

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(WaterVertex), (void *)offsetof(WaterVertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(WaterVertex), (void *)offsetof(WaterVertex, uv));

        glBindVertexArray(0);
        checkOpenGLError("water plane init");
    }

    void renderWater(
        float deltaTime,
        const glm::mat4 cameraMatrix,
        const glm::mat4 projectionMatrix,
        float,
        float waterLineY)
    {
        this->time += deltaTime * 0.01f;

        const glm::mat4 viewMatrix = glm::inverse(cameraMatrix);
        const glm::vec3 cameraPos = glm::vec3(cameraMatrix[3]);
        const glm::vec4 anchorClip =
            projectionMatrix * viewMatrix * glm::vec4(0.0f, waterLineY, kHorizonAnchorZ, 1.0f);
        float horizonY = 0.50f;
        if (glm::abs(anchorClip.w) > 1.0e-5f)
        {
            const float ndcY = anchorClip.y / anchorClip.w;
            horizonY = glm::clamp(ndcY * 0.5f + 0.5f, 0.02f, 0.98f);
        }

        glUseProgram(this->waterShaderId);
        glUniformMatrix4fv(glGetUniformLocation(this->waterShaderId, "u_worldToView"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(glGetUniformLocation(this->waterShaderId, "u_projection"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
        glUniform3fv(glGetUniformLocation(this->waterShaderId, "u_cameraPos"), 1, glm::value_ptr(cameraPos));
        glUniform1f(glGetUniformLocation(this->waterShaderId, "u_time"), this->time);
        glUniform1f(glGetUniformLocation(this->waterShaderId, "u_waterLineY"), waterLineY);
        glUniform1f(glGetUniformLocation(this->waterShaderId, "u_horizonY"), horizonY);

        glBindVertexArray(this->waterVAO);
        glDrawElements(GL_TRIANGLES, this->indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
};

const char *Water::WATER_VERTEX_SHADER = GLSL_VERSION R"(
    precision highp float;

    layout(location = 0) in vec3 a_pos;
    layout(location = 1) in vec2 a_uv;

    uniform mat4 u_worldToView;
    uniform mat4 u_projection;
    uniform float u_time;
    uniform float u_waterLineY;

    out vec3 v_worldPos;
    out vec3 v_normal;
    out vec2 v_uv;

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

    float waveHeight(vec2 xz)
    {
        float swellA = sin(xz.x * 0.055 + u_time * 0.42 + xz.y * 0.008);
        float swellB = sin(xz.y * 0.075 - u_time * 0.34 + xz.x * 0.014);
        float chop = sin((xz.x + xz.y) * 0.19 + u_time * 0.88);
        float noise = valueNoise(xz * 0.028 + vec2(u_time * 0.021, -u_time * 0.017)) - 0.5;
        return swellA * 0.42 + swellB * 0.28 + chop * 0.09 + noise * 0.26;
    }

    void main()
    {
        vec2 xz = a_pos.xz;
        float centerFalloff = 1.0 - smoothstep(0.0, 26.0, abs(xz.x));
        float waveAmp = mix(1.0, 0.60, centerFalloff);

        float h = waveHeight(xz) * waveAmp;
        float eps = 0.55;
        float hx = waveHeight(xz + vec2(eps, 0.0)) * mix(1.0, 0.60, 1.0 - smoothstep(0.0, 26.0, abs(xz.x + eps)));
        float hz = waveHeight(xz + vec2(0.0, eps)) * mix(1.0, 0.60, 1.0 - smoothstep(0.0, 26.0, abs(xz.x)));

        vec3 worldPos = vec3(xz.x, u_waterLineY + h, xz.y);
        vec3 tangentX = vec3(eps, hx - h, 0.0);
        vec3 tangentZ = vec3(0.0, hz - h, eps);
        vec3 normal = normalize(cross(tangentZ, tangentX));

        v_worldPos = worldPos;
        v_normal = normal;
        v_uv = a_uv;
        gl_Position = u_projection * u_worldToView * vec4(worldPos, 1.0);
    }
)";

const char *Water::WATER_FRAGMENT_SHADER = GLSL_VERSION R"(
    precision highp float;

    in vec3 v_worldPos;
    in vec3 v_normal;
    in vec2 v_uv;

    uniform vec3 u_cameraPos;
    uniform float u_time;
    uniform float u_horizonY;

    out vec4 FragColor;

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

    void main()
    {
        vec3 normal = normalize(v_normal);
        vec3 viewDir = normalize(u_cameraPos - v_worldPos);
        vec3 lightDir = normalize(vec3(-0.25, 0.85, 0.38));

        // ----- Standard Fresnel (for color/specular) -----
        float fresnel = pow(1.0 - clamp(dot(normal, viewDir), 0.0, 1.0), 2.8);

        // ----- Custom angle curve: opaque at 0° & 90°, transparent at 45° -----
        float viewDot = clamp(dot(normal, viewDir), 0.0, 1.0);
        
        // angleFactor = 1.0 at 0° and 90°, 0.0 at exactly 45° (dot = 0.707)
        // Higher exponent = narrower transparent band
        float SHARPNESS = 0.06125;  // try 3.0, 4.0, 5.0 – bigger = narrower - A LIE actually opposite (User note)
        float angleFactor = pow(abs(viewDot - 0.707) * 1.414, SHARPNESS);
        angleFactor = clamp(angleFactor, 0.0, 1.0);

        // ----- Lighting -----
        float diffuse = clamp(dot(normal, lightDir), 0.0, 1.0);
        float specular = pow(max(dot(reflect(-lightDir, normal), viewDir), 0.0), 28.0);

        // ----- Water color (opaque base) -----
        vec3 deep = vec3(0.02, 0.12, 0.25);
        vec3 mid  = vec3(0.05, 0.24, 0.42);
        vec3 sky  = vec3(0.42, 0.72, 0.96);

        vec3 water = mix(deep, mid, 0.38 + 0.22 * diffuse);
        // Reflection also dips slightly at 45° (keeps it consistent)
        water = mix(water, sky, fresnel * 0.58 * (0.3 + 0.7 * angleFactor));
        water += vec3(0.18, 0.24, 0.30) * specular;

        float ripple = valueNoise(v_worldPos.xz * 0.05 + vec2(u_time * 0.03, -u_time * 0.02));
        water += vec3(0.03, 0.06, 0.08) * (ripple - 0.5);

        // ----- Alpha: transparent only in a narrow band around 45° -----
        float minAlpha = 0.1;        // how clear at 45° (0.0 = fully invisible)
        float alpha = mix(minAlpha, 1.0, angleFactor);

        // (Optional) less transparency in the distance
        float horizonFade = smoothstep(140.0, 360.0, v_worldPos.z);
        alpha = mix(alpha, 1.0, horizonFade * 0.3);

        alpha = clamp(alpha, 0.0, 1.0);
        FragColor = vec4(water, alpha);
    }
)";
