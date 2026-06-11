#pragma once

#include "framework/gl_header.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "framework/boot.h"
#include "framework/gl_util.h"
#include "mesh.h"

struct ElectroBall
{
    GLuint surfaceShader = 0;
    GLuint shellShader = 0;

    float time = 0.0f;
    float hitPulse = 0.0f;
    float shellIntensity = 0.0f;
    bool active = false;

    static const char *SHELL_VERTEX_SHADER;
    static const char *SHELL_FRAGMENT_SHADER;
    static const char *SURFACE_VERTEX_SHADER;
    static const char *SURFACE_FRAGMENT_SHADER;

    void initElectroBall()
    {
        surfaceShader = vtx::createShaderProgram(SURFACE_VERTEX_SHADER, SURFACE_FRAGMENT_SHADER);
        shellShader = vtx::createShaderProgram(SHELL_VERTEX_SHADER, SHELL_FRAGMENT_SHADER);
    }

    void triggerPinFlash(float strength = 1.0f)
    {
        float s = glm::clamp(strength, 0.0f, 2.0f);
        hitPulse = glm::clamp(glm::max(hitPulse, 0.50f * s), 0.0f, 1.5f);
    }

    void updateElectroBall(float deltaTime, const glm::vec3 &ballPos, bool enabled)
    {
        time += deltaTime;
        hitPulse = glm::max(0.0f, hitPulse - deltaTime * 1.8f);

        active = false;
        shellIntensity = 0.0f;

        if (!enabled)
            return;

        const float zFade = glm::smoothstep(-18.0f, -16.9f, ballPos.z);
        if (zFade <= 0.0f)
            return;

        active = true;
        shellIntensity = zFade * (0.85f + hitPulse);
    }

    void renderElectroBallShell(
        AssetMesh &ballMesh,
        const glm::mat4 &ballModel,
        const glm::mat4 &cameraMatrix,
        const glm::mat4 &projectionMatrix
    )
    {
        if (!shellShader)
            return;
        if (!active || shellIntensity <= 0.001f)
            return;
        renderShell(ballMesh, ballModel, cameraMatrix, projectionMatrix);
    }

    void renderElectroBallSurface(
        AssetMesh &ballMesh,
        const glm::mat4 &ballModel,
        const glm::mat4 &cameraMatrix,
        const glm::mat4 &projectionMatrix
    )
    {
        if (!surfaceShader)
            return;
        if (!active || shellIntensity <= 0.001f)
            return;
        renderSurface(ballMesh, ballModel, cameraMatrix, projectionMatrix);
    }

  private:
    void bindSharedMatrices(
        GLuint program,
        const glm::mat4 &modelMatrix,
        const glm::mat4 &cameraMatrix,
        const glm::mat4 &projectionMatrix
    )
    {
        glUseProgram(program);
        glUniformMatrix4fv(glGetUniformLocation(program, "u_projection"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
        glUniformMatrix4fv(glGetUniformLocation(program, "u_worldToView"), 1, GL_FALSE, glm::value_ptr(cameraMatrix));
        glUniformMatrix4fv(glGetUniformLocation(program, "u_modelToWorld"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
        glm::mat4 invView = glm::affineInverse(cameraMatrix);
        glUniform3f(
            glGetUniformLocation(program, "uCameraWorld"),
            invView[3][0],
            invView[3][1],
            invView[3][2]
        );
        glUniform3f(
            glGetUniformLocation(program, "uBallCenterWorld"),
            modelMatrix[3][0],
            modelMatrix[3][1],
            modelMatrix[3][2]
        );
        glUniform1f(glGetUniformLocation(program, "uTime"), time);
        glUniform1f(glGetUniformLocation(program, "uIntensity"), shellIntensity);
        glUniform1f(glGetUniformLocation(program, "uHitPulse"), hitPulse);
    }

    void renderSurface(
        AssetMesh &ballMesh,
        const glm::mat4 &ballModel,
        const glm::mat4 &cameraMatrix,
        const glm::mat4 &projectionMatrix
    )
    {
        GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
        GLboolean cullWasEnabled = glIsEnabled(GL_CULL_FACE);
        GLboolean depthWasEnabled = glIsEnabled(GL_DEPTH_TEST);
        GLboolean depthMaskWasEnabled = GL_TRUE;
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMaskWasEnabled);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-1.0f, -1.0f);
        glDepthFunc(GL_LEQUAL);

        bindSharedMatrices(surfaceShader, ballModel, cameraMatrix, projectionMatrix);

        glBindVertexArray(ballMesh.meshVAO);
        glDrawElementsInstanced(
            GL_TRIANGLES,
            ballMesh.indexCount,
            GL_UNSIGNED_INT,
            (void *)0,
            ballMesh.instanceData.size()
        );
        glBindVertexArray(0);

        glDisable(GL_POLYGON_OFFSET_FILL);
        glDepthFunc(GL_LESS);
        glDepthMask(depthMaskWasEnabled);
        if (!blendWasEnabled)
            glDisable(GL_BLEND);
        if (!cullWasEnabled)
            glDisable(GL_CULL_FACE);
        if (!depthWasEnabled)
            glDisable(GL_DEPTH_TEST);
    }

    void renderShell(
        AssetMesh &ballMesh,
        const glm::mat4 &ballModel,
        const glm::mat4 &cameraMatrix,
        const glm::mat4 &projectionMatrix
    )
    {
        const float shellScale = 1.0f + (0.01f / 0.11f);
        glm::mat4 shellModel = ballModel * glm::scale(glm::mat4(1.0f), glm::vec3(shellScale));

        GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
        GLboolean cullWasEnabled = glIsEnabled(GL_CULL_FACE);
        GLboolean depthWasEnabled = glIsEnabled(GL_DEPTH_TEST);
        GLboolean depthMaskWasEnabled = GL_TRUE;
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMaskWasEnabled);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        bindSharedMatrices(shellShader, shellModel, cameraMatrix, projectionMatrix);

        glBindVertexArray(ballMesh.meshVAO);
        glDrawElementsInstanced(
            GL_TRIANGLES,
            ballMesh.indexCount,
            GL_UNSIGNED_INT,
            (void *)0,
            ballMesh.instanceData.size()
        );
        glBindVertexArray(0);

        glCullFace(GL_BACK);
        glDepthMask(depthMaskWasEnabled);
        if (!blendWasEnabled)
            glDisable(GL_BLEND);
        if (!cullWasEnabled)
            glDisable(GL_CULL_FACE);
        if (!depthWasEnabled)
            glDisable(GL_DEPTH_TEST);
    }

};

const char *ElectroBall::SURFACE_VERTEX_SHADER = GLSL_VERSION R"(
    precision highp float;

    layout (location = 0) in vec3 a_pos;
    layout (location = 3) in vec3 a_normal;
    layout (location = 6) in vec3 a_positionOffset;
    layout (location = 7) in vec3 a_scaleOffset;
    layout (location = 9) in vec4 a_instRot;

    uniform mat4 u_worldToView;
    uniform mat4 u_modelToWorld;
    uniform mat4 u_projection;
    uniform vec3 uCameraWorld;
    uniform vec3 uBallCenterWorld;

    out vec3 v_worldPos;
    out vec3 v_worldNormal;
    out vec3 v_sphereDir;

    vec3 rotateVecByQuat(vec3 v, vec4 q)
    {
        vec3 u = q.xyz;
        float s = q.w;
        return 2.0 * dot(u, v) * u
            + (s * s - dot(u, u)) * v
            + 2.0 * s * cross(u, v);
    }

    void main()
    {
        vec3 scaledPosition = a_pos * a_scaleOffset;
        vec3 rotatedPosition = rotateVecByQuat(scaledPosition, a_instRot);
        vec3 localPos = rotatedPosition + a_positionOffset;

        vec4 worldPos = u_modelToWorld * vec4(localPos, 1.0);
        mat3 normalMatrix = mat3(transpose(inverse(u_modelToWorld)));
        vec3 worldNormal = normalize(normalMatrix * a_normal);
        vec3 sphereDir = normalize(worldPos.xyz - uBallCenterWorld);

        v_worldPos = worldPos.xyz;
        v_worldNormal = worldNormal;
        v_sphereDir = sphereDir;
        gl_Position = u_projection * u_worldToView * worldPos;
    }
)";

const char *ElectroBall::SURFACE_FRAGMENT_SHADER = GLSL_VERSION R"(
    precision highp float;

    in vec3 v_worldPos;
    in vec3 v_worldNormal;
    in vec3 v_sphereDir;

    uniform vec3 uCameraWorld;
    uniform vec3 uBallCenterWorld;
    uniform float uTime;
    uniform float uIntensity;
    uniform float uHitPulse;

    out vec4 FragColor;

    #define MOD3 vec3(.1031, .11369, .13787)

    vec3 hash33(vec3 p3)
    {
        p3 = fract(p3 * MOD3);
        p3 += dot(p3, p3.yxz + 19.19);
        return -1.0 + 2.0 * fract(vec3(
            (p3.x + p3.y) * p3.z,
            (p3.x + p3.z) * p3.y,
            (p3.y + p3.z) * p3.x
        ));
    }

    float perlinNoise3D(vec3 p)
    {
        vec3 pi = floor(p);
        vec3 pf = p - pi;
        vec3 w = pf * pf * (3.0 - 2.0 * pf);

        return mix(
            mix(
                mix(dot(pf - vec3(0, 0, 0), hash33(pi + vec3(0, 0, 0))),
                    dot(pf - vec3(1, 0, 0), hash33(pi + vec3(1, 0, 0))), w.x),
                mix(dot(pf - vec3(0, 0, 1), hash33(pi + vec3(0, 0, 1))),
                    dot(pf - vec3(1, 0, 1), hash33(pi + vec3(1, 0, 1))), w.x),
                w.z),
            mix(
                mix(dot(pf - vec3(0, 1, 0), hash33(pi + vec3(0, 1, 0))),
                    dot(pf - vec3(1, 1, 0), hash33(pi + vec3(1, 1, 0))), w.x),
                mix(dot(pf - vec3(0, 1, 1), hash33(pi + vec3(0, 1, 1))),
                    dot(pf - vec3(1, 1, 1), hash33(pi + vec3(1, 1, 1))), w.x),
                w.z),
            w.y
        );
    }

    float fbm(vec3 p)
    {
        float total = 0.0;
        float amp = 0.5;
        for (int i = 0; i < 4; ++i)
        {
            total += amp * perlinNoise3D(p);
            p = p * 2.07 + vec3(0.8, -1.2, 1.6);
            amp *= 0.5;
        }
        return total;
    }

    void main()
    {
        vec3 N = normalize(v_worldNormal);
        vec3 V = normalize(uCameraWorld - v_worldPos);
        float facing = clamp(dot(N, V), 0.0, 1.0);

        vec3 sphereDir = normalize(v_worldPos - uBallCenterWorld);
        vec2 uv = vec2(atan(sphereDir.z, sphereDir.x), asin(clamp(sphereDir.y, -1.0, 1.0)));
        vec3 domain = vec3(uv * vec2(7.5, 8.5), uTime * 0.55);

        float w1 = fbm(domain + vec3(0.0, 0.0, 1.0));
        float w2 = fbm(domain.yxz + vec3(2.4, -0.8, 0.0));
        vec2 flow = vec2(
            uv.x * 7.0 + w1 * 2.2 + uTime * 0.55,
            uv.y * 8.5 + w2 * 2.0 - uTime * 0.38
        );

        float lineA = 1.0 - smoothstep(0.085, 0.200, abs(sin(flow.x + 1.1 * sin(flow.y))));
        float lineB = 1.0 - smoothstep(0.080, 0.185, abs(sin(flow.x * 1.45 - flow.y * 1.0 + w1 * 2.0)));
        float lineC = 1.0 - smoothstep(0.070, 0.170, abs(sin(flow.x * 2.0 + flow.y * 1.45 - w2 * 2.1)));
        float veins = max(lineA, max(lineB, lineC));

        float breakup = smoothstep(0.10, 0.62, 0.5 + 0.5 * fbm(sphereDir * 6.0 + vec3(0.0, uTime * 1.1, 0.0)));
        veins *= breakup;
        veins *= smoothstep(0.00, 0.92, facing);

        float coreGlow = smoothstep(0.35, 0.95, veins);
        float branchPulse = 0.88 + 0.16 * sin(uTime * 4.0 + uv.x * 4.2 + uv.y * 5.4);
        veins *= branchPulse;

        float pulse = 0.8 + 0.4 * clamp(uHitPulse, 0.0, 1.0);
        float alpha = uIntensity * (1.15 * veins + 0.45 * coreGlow) * facing * 1.30 * pulse;
        if (alpha < 0.04)
            discard;

        vec3 lineColor = vec3(0.70, 0.96, 1.0);
        vec3 glowColor = vec3(0.18, 0.72, 1.0);
        vec3 color = mix(glowColor, lineColor, clamp(veins * 1.4, 0.0, 1.0));
        color *= (0.80 + 1.05 * veins + 0.45 * coreGlow) * uIntensity * pulse;

        FragColor = vec4(color, clamp(alpha, 0.0, 0.92));
    }
)";

const char *ElectroBall::SHELL_VERTEX_SHADER = GLSL_VERSION R"(
    precision highp float;

    layout (location = 0) in vec3 a_pos;
    layout (location = 3) in vec3 a_normal;
    layout (location = 6) in vec3 a_positionOffset;
    layout (location = 7) in vec3 a_scaleOffset;
    layout (location = 9) in vec4 a_instRot;

    uniform mat4 u_worldToView;
    uniform mat4 u_modelToWorld;
    uniform mat4 u_projection;

    out vec3 v_worldPos;
    out vec3 v_worldNormal;

    vec3 rotateVecByQuat(vec3 v, vec4 q)
    {
        vec3 u = q.xyz;
        float s = q.w;
        return 2.0 * dot(u, v) * u
            + (s * s - dot(u, u)) * v
            + 2.0 * s * cross(u, v);
    }

    void main()
    {
        vec3 scaledPosition = a_pos * a_scaleOffset;
        vec3 rotatedPosition = rotateVecByQuat(scaledPosition, a_instRot);
        vec3 localPos = rotatedPosition + a_positionOffset;

        vec4 worldPos = u_modelToWorld * vec4(localPos, 1.0);
        mat3 normalMatrix = mat3(transpose(inverse(u_modelToWorld)));
        vec3 worldNormal = normalize(normalMatrix * a_normal);

        v_worldPos = worldPos.xyz;
        v_worldNormal = worldNormal;
        gl_Position = u_projection * u_worldToView * worldPos;
    }
)";

const char *ElectroBall::SHELL_FRAGMENT_SHADER = GLSL_VERSION R"(
    precision highp float;

    in vec3 v_worldPos;
    in vec3 v_worldNormal;

    uniform float uTime;
    uniform float uIntensity;
    uniform float uHitPulse;
    uniform vec3 uCameraWorld;

    out vec4 FragColor;

    #define MOD3 vec3(.1031, .11369, .13787)

    vec3 hash33(vec3 p3)
    {
        p3 = fract(p3 * MOD3);
        p3 += dot(p3, p3.yxz + 19.19);
        return -1.0 + 2.0 * fract(vec3(
            (p3.x + p3.y) * p3.z,
            (p3.x + p3.z) * p3.y,
            (p3.y + p3.z) * p3.x
        ));
    }

    float perlinNoise3D(vec3 p)
    {
        vec3 pi = floor(p);
        vec3 pf = p - pi;
        vec3 w = pf * pf * (3.0 - 2.0 * pf);

        return mix(
            mix(
                mix(dot(pf - vec3(0, 0, 0), hash33(pi + vec3(0, 0, 0))),
                    dot(pf - vec3(1, 0, 0), hash33(pi + vec3(1, 0, 0))), w.x),
                mix(dot(pf - vec3(0, 0, 1), hash33(pi + vec3(0, 0, 1))),
                    dot(pf - vec3(1, 0, 1), hash33(pi + vec3(1, 0, 1))), w.x),
                w.z),
            mix(
                mix(dot(pf - vec3(0, 1, 0), hash33(pi + vec3(0, 1, 0))),
                    dot(pf - vec3(1, 1, 0), hash33(pi + vec3(1, 1, 0))), w.x),
                mix(dot(pf - vec3(0, 1, 1), hash33(pi + vec3(0, 1, 1))),
                    dot(pf - vec3(1, 1, 1), hash33(pi + vec3(1, 1, 1))), w.x),
                w.z),
            w.y
        );
    }

    float fbm(vec3 p)
    {
        float total = 0.0;
        float amp = 0.5;
        for (int i = 0; i < 4; ++i)
        {
            total += amp * perlinNoise3D(p);
            p = p * 2.03 + vec3(1.7, -0.9, 0.6);
            amp *= 0.5;
        }
        return total;
    }

    void main()
    {
        vec3 N = normalize(v_worldNormal);
        vec3 V = normalize(uCameraWorld - v_worldPos);
        float fresnel = pow(1.0 - clamp(dot(N, V), 0.0, 1.0), 4.2);

        float field = abs(perlinNoise3D(v_worldPos * 10.0 + vec3(0.0, 0.0, uTime * 3.5)));
        float band = 0.5 + 0.5 * sin(uTime * 18.0 + v_worldPos.y * 35.0 + field * 8.0);
        float arc = smoothstep(0.72, 1.0, field + 0.45 * band);

        vec2 surf;
        surf.x = atan(N.z, N.x);
        surf.y = asin(clamp(N.y, -1.0, 1.0));

        vec3 branchDomain = vec3(
            surf.x * 5.5,
            surf.y * 7.0,
            uTime * 1.8
        );
        float warpA = fbm(branchDomain + vec3(0.0, 0.0, 1.3));
        float warpB = fbm(branchDomain.yxz + vec3(2.1, -0.7, 0.0));
        vec2 flowUv = vec2(
            surf.x * 6.0 + warpA * 1.5 + uTime * 1.1,
            surf.y * 8.5 + warpB * 1.2 - uTime * 0.7
        );

        float trunk = abs(sin(flowUv.x + 1.7 * sin(flowUv.y)));
        float branch1 = abs(sin(flowUv.x * 1.8 - flowUv.y * 1.1 + warpA * 2.0));
        float branch2 = abs(sin(flowUv.x * 2.7 + flowUv.y * 1.6 - warpB * 2.4));

        float trunkLine = 1.0 - smoothstep(0.08, 0.16, trunk);
        float branchLine1 = 1.0 - smoothstep(0.05, 0.12, branch1);
        float branchLine2 = 1.0 - smoothstep(0.05, 0.11, branch2);
        float branchLines = max(trunkLine, max(branchLine1, branchLine2));

        float branchNoise = 0.5 + 0.5 * fbm(v_worldPos * 14.0 + vec3(0.0, uTime * 4.0, 0.0));
        float branchMask = branchLines * smoothstep(0.35, 0.85, branchNoise);
        branchMask *= smoothstep(0.10, 0.55, fresnel + 0.12);

        float alpha = uIntensity * (1.85 * fresnel + 0.30 * arc + 1.10 * branchMask);
        alpha *= 0.85 + 0.45 * clamp(uHitPulse, 0.0, 1.0);
        alpha = clamp(alpha, 0.0, 1.0);
        if (alpha < 0.02)
            discard;

        vec3 color = vec3(0.20, 0.62, 1.0) * (0.30 + 1.95 * fresnel);
        color += vec3(0.90, 0.98, 1.0) * arc * 0.40;
        color += vec3(0.76, 0.93, 1.0) * branchMask * (1.45 + 0.7 * clamp(uHitPulse, 0.0, 1.0));
        color *= uIntensity;

        FragColor = vec4(color, alpha * 0.68);
    }
)";
