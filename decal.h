#pragma once

#include "framework/gl_header.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>

#include "framework/boot.h"
#include "framework/gl_util.h"

#define MAX_DECALS 100

// Matches std140 layout explicitly
struct Decal
{
    glm::mat4 transform; // 64 bytes
    glm::vec2 uvStart;   // 8 bytes
    glm::vec2 uvEnd;     // 8 bytes
    int enabled;          // 4 bytes (explicit, std140-safe)
    int pad[3];          // padding to 16-byte alignment
};

struct DecalBatch
{
    GLuint vao = 0;
    GLuint decalUBO = 0;
    GLuint decalShaderId = 0;
    Decal decals[MAX_DECALS];

    static const char *DECAL_VERTEX_SHADER;
    static const char *DECAL_FRAGMENT_SHADER;

    void loadDecalBatchShader()
    {
        this->decalShaderId = vtx::createShaderProgram(DECAL_VERTEX_SHADER, DECAL_FRAGMENT_SHADER);
    }

    void initDecalBatch()
    {
        /* Shader */
        this->loadDecalBatchShader();

        /* VAO (no vertex attributes, quad generated in shader) */
        {
            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);

            GLuint vbo;
            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            // --
            // No attributes required
            // --
            glBindVertexArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }

        /* Shader */
        loadDecalBatchShader();

        /* UBO */
        {
            glGenBuffers(1, &decalUBO);
            glBindBuffer(GL_UNIFORM_BUFFER, decalUBO);
            glBufferData(GL_UNIFORM_BUFFER, sizeof(Decal) * MAX_DECALS, nullptr, GL_STATIC_DRAW);
            glBindBufferBase(GL_UNIFORM_BUFFER, 0, decalUBO);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
        }
    }

    void
    renderDecals(GLuint diffuseTextureId, const glm::mat4 &worldToView, const glm::mat4 &projection)
    {
        /* Upload decal data */
        glBindBuffer(GL_UNIFORM_BUFFER, decalUBO);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Decal) * MAX_DECALS, decals);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        glUseProgram(decalShaderId);

        glUniformMatrix4fv(
            glGetUniformLocation(decalShaderId, "u_projection"), 1, GL_FALSE,
            glm::value_ptr(projection)
        );

        glUniformMatrix4fv(
            glGetUniformLocation(decalShaderId, "u_worldToView"), 1, GL_FALSE,
            glm::value_ptr(worldToView)
        );

        /* Bind UBO */
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, decalUBO);

        /* Texture */
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseTextureId);
        glUniform1i(glGetUniformLocation(decalShaderId, "u_diffuseTexture"), 0);

        /* Draw */
        glBindVertexArray(vao);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, MAX_DECALS);
        glBindVertexArray(0);

        checkOpenGLError();
    }
};

const char *DecalBatch::DECAL_VERTEX_SHADER =
    GLSL_VERSION
    R"(

    precision highp int;
    precision highp float;

    out vec2 v_uv;

    struct Decal
    {
        mat4 transform;
        vec2 uvStart;
        vec2 uvEnd;
        int enabled;
    };

    layout(std140) uniform Decals
    {
        Decal decals[100];
    };

    uniform mat4 u_worldToView;
    uniform mat4 u_projection;

    void main()
    {
        Decal d = decals[gl_InstanceID];

        if (d.enabled == 0)
        {
            gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
            v_uv = vec2(0.0);
            return;
        }

        vec2 quad[6] = vec2[](
            vec2(-0.5, -0.5),
            vec2( 0.5, -0.5),
            vec2( 0.5,  0.5),
            vec2(-0.5, -0.5),
            vec2( 0.5,  0.5),
            vec2(-0.5,  0.5)
        );

        vec2 localPos = quad[gl_VertexID];

        v_uv = mix(d.uvStart, d.uvEnd, localPos + vec2(0.5));

        vec4 worldPos = d.transform * vec4(localPos, 0.0, 1.0);
        gl_Position = u_projection * u_worldToView * worldPos;
    }
    )";

const char *DecalBatch::DECAL_FRAGMENT_SHADER = GLSL_VERSION
    R"(

    precision highp int;
    precision highp float;

    in vec2 v_uv;
    out vec4 fragColor;

    uniform sampler2D u_diffuseTexture;

    void main()
    {
        vec4 tex = texture(u_diffuseTexture, v_uv);
        if (tex.a < 0.2) {
            // Even with depth test it can render crossed decals in a wrong order.
            // I guess I need to just accept that semi transparent
            // may not always show objects behind if the depth test fails
            // It should be fine for most of the stuff
            // And if there are crossed decals they shoudl be fully transparent to
            // be displayed properly, maybe dithered if necessary
            discard;
        }

        fragColor = tex;
    }
    )";