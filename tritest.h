#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "framework/gl_header.h"
#include "framework/gl_util.h"

struct InstanceData2
{
    glm::vec2 offset; // 8 bytes
    glm::vec2 pad;    // std140 alignment (16 bytes total)
};

struct MiniTriangle
{
    GLuint vao = 0;
    GLuint program = 0;
    GLuint ubo = 0;

    InstanceData2 instances[2];

    void init()
    {
        program = vtx::createShaderProgram(vs, fs);

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glBindVertexArray(0);

        glGenBuffers(1, &ubo);
        glBindBuffer(GL_UNIFORM_BUFFER, ubo);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(instances), nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        GLuint blockIndex = glGetUniformBlockIndex(program, "Instances");
        glUniformBlockBinding(program, blockIndex, 0);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);
    }

    void render(GLuint textureId)
    {
        // Update instance data
        instances[0].offset = glm::vec2(-0.5f, -0.5f);
        instances[1].offset = glm::vec2(0.5f, 0.0f);

        glBindBuffer(GL_UNIFORM_BUFFER, ubo);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(instances), instances);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        glUseProgram(program);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureId);
        glUniform1i(glGetUniformLocation(program, "u_tex"), 0);

        /* Bind UBO */
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);

        glBindVertexArray(vao);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 3, 2);
        glBindVertexArray(0);
    }
    static const char *vs;
    static const char *fs;
};

const char *MiniTriangle::vs = R"(#version 300 es
precision highp float;

out vec2 v_uv;

struct Instance
{
    vec2 offset;
    vec2 pad;
};

layout(std140) uniform Instances
{
    Instance instances[2];
};

void main()
{
    vec2 verts[3] = vec2[](
        vec2(-0.3, -0.3),
        vec2( 0.3, -0.3),
        vec2( 0.0,  0.3)
    );

    vec2 uvs[3] = vec2[](
        vec2(0.0, 0.0),
        vec2(1.0, 0.0),
        vec2(0.5, 1.0)
    );

    vec2 pos = verts[gl_VertexID] + instances[gl_InstanceID].offset;

    gl_Position = vec4(pos, 0.0, 1.0);
    v_uv = uvs[gl_VertexID];
}
)";

const char *MiniTriangle::fs = R"(#version 300 es
precision highp float;

in vec2 v_uv;
out vec4 FragColor;

uniform sampler2D u_tex;

void main()
{
    FragColor = texture(u_tex, v_uv);
}
)";