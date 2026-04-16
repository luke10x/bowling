#pragma once

#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "framework/boot.h"
#include "framework/gl_util.h"
#include "assets/api/mesh_data.h"
#include "texture.h"

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
// VERTEX SHADER (precision optional in vertex shaders for GLES)
// ─────────────────────────────────────────────────────────────
const char *SimpleShaderProgram::SIMPLE_VERTEX_SHADER = 
    GLSL_VERSION
    R"(
    layout (location = 0) in vec3 a_pos;
    layout (location = 1) in vec2 a_texCoords;
    layout (location = 2) in vec3 a_normal;

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
        
        mat3 normalMatrix = mat3(transpose(inverse(mat3(u_view * u_model))));
        v_normal = normalize(normalMatrix * a_normal);
    }
    )";

const char *SimpleShaderProgram::SIMPLE_FRAGMENT_SHADER = 
    GLSL_VERSION
    GLSL_FRAGMENT_PRECISION  // ← This adds "precision highp float;\n" for GLES
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
        baseColor.a = 1.0;
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
    
    // ✅ CRITICAL: Ensure texture is complete for GLES
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

    // Use regular draw for simple coins (instancing only if you have instance data)
    if (assetMesh.instanceData.size() <= 1) {
        glDrawElements(GL_TRIANGLES, assetMesh.indexCount, GL_UNSIGNED_INT, 0);
    } else {
        glDrawElementsInstanced(
            GL_TRIANGLES,
            assetMesh.indexCount,
            GL_UNSIGNED_INT,
            0,
            static_cast<GLsizei>(assetMesh.instanceData.size()));
    }

    glBindVertexArray(0);
    checkOpenGLError("SimpleShaderProgram::renderSimpleMesh");
}
// === DEBUG: Before rendering coins ===
void debugCoinRenderState(const char* label) {
    GLint prog; glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
    GLint vao; glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
    GLint tex; glGetIntegerv(GL_TEXTURE_BINDING_2D, &tex);
    GLboolean depthTest; glGetBooleanv(GL_DEPTH_TEST, &depthTest);
    GLboolean blend; glGetBooleanv(GL_BLEND, &blend);
    
    GLint viewport[4]; glGetIntegerv(GL_VIEWPORT, viewport);
    
    std::cerr << "[" << label << "] "
              << "prog=" << prog 
              << " vao=" << vao
              << " tex=" << tex
              << " depthTest=" << depthTest
              << " blend=" << blend
              << " viewport=[" << viewport[0] << "," << viewport[1] 
              << "," << viewport[2] << "," << viewport[3] << "]\n";
}
