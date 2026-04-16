#pragma once

#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "framework/boot.h"
#include "framework/gl_util.h"
#include "assets/api/mesh_data.h"
#include "texture.h"

#pragma once

#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "framework/boot.h"
#include "framework/gl_util.h"
#include "assets/api/mesh_data.h"
#include "texture.h"

// Interface UNCHANGED. Zero extra members. Won't break default constructors.
struct SimpleShaderProgram
{
    static const char *SIMPLE_VERTEX_SHADER;
    static const char *SIMPLE_FRAGMENT_SHADER;
    GLuint id;

    void initSimpleShaderProgram();
    void initSimpleShaderProgram(const char *vertexShaderText, const char *fragmentShaderText);

    void updateDiffuseTexture(Texture &texture);
    void updateLightParams(glm::vec3 lightPos, glm::vec3 lightColor = glm::vec3(1.0f), float ambientStrength = 0.3f);

    void renderSimpleMesh(
        AssetMesh &assetMesh,
        glm::mat4 modelMatrix,
        glm::mat4 viewMatrix,
        glm::mat4 projectionMatrix);
};

// ─────────────────────────────────────────────────────────────
// SHADERS - MATCH YOUR EXACT VAO LAYOUT
// ─────────────────────────────────────────────────────────────
const char *SimpleShaderProgram::SIMPLE_VERTEX_SHADER = 
    GLSL_VERSION
    R"(
    // Match AssetMesh::sendMeshDataToGpu() EXACTLY:
    layout (location = 0) in vec3 a_pos;        // position
    layout (location = 1) in vec4 a_color;      // color (we ignore it, but declare to match type)
    layout (location = 2) in vec2 a_texCoords;  // UVs
    layout (location = 3) in vec3 a_normal;     // normal

    out vec2 v_texCoords;
    out vec3 v_normal;
    out vec3 v_fragPos;

    uniform mat4 u_model;
    uniform mat4 u_view;
    uniform mat4 u_projection;

    void main() {
        vec4 worldPos = u_model * vec4(a_pos, 1.0);
        vec4 viewPos = u_view * worldPos;
        gl_Position = u_projection * viewPos;
        
        v_fragPos = viewPos.xyz;
        v_texCoords = a_texCoords;
        v_normal = normalize(mat3(transpose(inverse(mat3(u_model)))) * a_normal);
    }
    )";

const char *SimpleShaderProgram::SIMPLE_FRAGMENT_SHADER = 
    GLSL_VERSION
    GLSL_FRAGMENT_PRECISION
    R"(
    in vec2 v_texCoords;
    in vec3 v_normal;
    in vec3 v_fragPos;

    uniform sampler2D u_diffuseTexture;
    uniform vec3 u_lightPos;
    uniform vec3 u_lightColor;
    uniform float u_ambientStrength;

    out vec4 FragColor;

    void main() {
        vec4 baseColor = texture(u_diffuseTexture, v_texCoords);
        vec3 normal = normalize(v_normal);
        vec3 lightDir = normalize(u_lightPos - v_fragPos);
        
        vec3 ambient = u_ambientStrength * u_lightColor;
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diff * u_lightColor;
        
        vec3 result = (ambient + diffuse) * baseColor.rgb;
        FragColor = vec4(result, baseColor.a);
    }
    )";

// ─────────────────────────────────────────────────────────────
// IMPLEMENTATION
// ─────────────────────────────────────────────────────────────
void SimpleShaderProgram::initSimpleShaderProgram()
{
    this->id = vtx::createShaderProgram(
        SimpleShaderProgram::SIMPLE_VERTEX_SHADER,
        SimpleShaderProgram::SIMPLE_FRAGMENT_SHADER);
}

void SimpleShaderProgram::initSimpleShaderProgram(
    const char *vertexShaderText,
    const char *fragmentShaderText)
{
    this->id = vtx::createShaderProgram(vertexShaderText, fragmentShaderText);
}

void SimpleShaderProgram::updateDiffuseTexture(Texture &diffuseTexture) {
    glUseProgram(this->id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, diffuseTexture.id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glUniform1i(glGetUniformLocation(this->id, "u_diffuseTexture"), 0);
    checkOpenGLError();
}

void SimpleShaderProgram::updateLightParams(
    glm::vec3 lightPos, 
    glm::vec3 lightColor, 
    float ambientStrength)
{
    GLint loc;
    loc = glGetUniformLocation(this->id, "u_lightPos");
    if (loc >= 0) glUniform3f(loc, lightPos.x, lightPos.y, lightPos.z);
    loc = glGetUniformLocation(this->id, "u_lightColor");
    if (loc >= 0) glUniform3f(loc, lightColor.x, lightColor.y, lightColor.z);
    loc = glGetUniformLocation(this->id, "u_ambientStrength");
    if (loc >= 0) glUniform1f(loc, ambientStrength);
    checkOpenGLError();
}

void SimpleShaderProgram::renderSimpleMesh(
    AssetMesh &assetMesh,
    glm::mat4 modelMatrix,
    glm::mat4 viewMatrix,
    glm::mat4 projectionMatrix)
{
    glUseProgram(this->id);

    GLint loc;
    loc = glGetUniformLocation(this->id, "u_model");
    if (loc >= 0) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
    loc = glGetUniformLocation(this->id, "u_view");
    if (loc >= 0) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
    loc = glGetUniformLocation(this->id, "u_projection");
    if (loc >= 0) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

    glBindVertexArray(assetMesh.meshVAO);

    // 🔥 CRITICAL FOR WEBGL/EMSCRIPTEN:
    // 1. Disable attributes our shader doesn't use (4+)
    // 2. Reset instance divisors to 0 (prevents divisor mismatch errors)
    for (GLuint attr = 4; attr < 16; ++attr) {
        glDisableVertexAttribArray(attr);
        glVertexAttribDivisor(attr, 0);
    }

    // ✅ ALWAYS use glDrawElements for simple single objects.
    // glDrawElementsInstanced REQUIRES at least one attribute with divisor=1.
    // Since we just reset all divisors to 0, Instanced will fail.
    glDrawElements(GL_TRIANGLES, assetMesh.indexCount, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
    checkOpenGLError("SimpleShaderProgram::renderSimpleMesh");
}