#pragma once

#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "framework/boot.h"
#include "framework/gl_util.h"

#include "assets/api/mesh_data.h"
#include "texture.h"

struct InstanceData
{
    glm::quat instRot; // This might be required to be alligned on its own (I am not sure about that)
    glm::vec3 textureScale;
    glm::vec3 positionOffset;
    glm::vec3 scaleOffset;
    glm::vec2 atlasStart;
    float _pad1; // Aliggn the structure, otherwise Emscrpten dies
    float _pad2;
    float _pad3;
    float _pad4;
    float _pad5;
};

struct AssetMesh
{
    MeshData meshData;

    GLuint meshVAO;
    GLuint instanceVBO;
    int indexCount = 0;

    std::vector<InstanceData> instanceData;

    void sendMeshDataToGpu(MeshData *meshData);

    void sendInstanceDataToGpu();
};

void AssetMesh::sendMeshDataToGpu(MeshData *meshData)
{
    // We use instance rendering, by default is one instance so these will be used
    // (unless the instance data overriden later)
    for (int i = 0; i < 1; i++)
    {
        this->instanceData.push_back({
            glm::quat(1.0f, 0.0f, 0.0f, 0.0f), // instRot
            glm::vec3(1.0f),                   // texture scale
            glm::vec3(0.0f),                   // position offest
            glm::vec3(1.0f),                   // scale offset
            glm::vec2(0.0f),                   //  atlas region
        });
    }
    // this->meshData = /*  copy the param values here and make sure the ownership of underlying texture passed*/
    // need to remember this
    this->indexCount = meshData->indexCount;
    // Even this but maybe TODO think of how can I avoid it

    // Ok, asset imported now just create OpenGL buffers
    // Create VAO
    glGenVertexArrays(1, &this->meshVAO);
    glBindVertexArray(this->meshVAO);

    // Create VBO with vertices
    GLuint VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        meshData->vertexCount * sizeof(Vertex), // all vertices in bytes
        meshData->vertices, GL_STATIC_DRAW);

    // Create EBO with indexes
    GLuint EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER, // This is used for EBO
        meshData->indexCount * sizeof(unsigned int), meshData->indices,
        GL_STATIC_DRAW);

    // Links VBO attributes such as coordinates and colors to VAO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // clang-format off
    // These are the basic
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, position));
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, color));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, texCoords));
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, normal));

    // Required for animations
    glVertexAttribIPointer(4, 4, GL_INT,            sizeof(Vertex), (void*) offsetof(Vertex, bones));
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, weights));
    // clang-format on

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);
    glEnableVertexAttribArray(5);

    // Upload instance data:
    glGenBuffers(1, &this->instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, this->instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, 100 * sizeof(InstanceData), instanceData.data(), GL_DYNAMIC_DRAW);

    // Position Offset Attribute (layout = 6, updates per instance)
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void *)offsetof(InstanceData, positionOffset));
    glVertexAttribDivisor(6, 1);

    // Scale Offset Attribute (layout = 7, updates per instance)
    glEnableVertexAttribArray(7);
    glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void *)offsetof(InstanceData, scaleOffset));
    glVertexAttribDivisor(7, 1);

    // Atlas Start Attribute (layout = 8, updates per instance)
    glEnableVertexAttribArray(8);
    glVertexAttribPointer(8, 2, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void *)offsetof(InstanceData, atlasStart));
    glVertexAttribDivisor(8, 1);

    // These are out of order Istorically
    // Atlas Start Attribute (layout = 9, updates per instance)
    glEnableVertexAttribArray(9);
    glVertexAttribPointer(9, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void *)offsetof(InstanceData, instRot));
    glVertexAttribDivisor(9, 1);

    // TODO this is not matching InstanceData order
    glEnableVertexAttribArray(10);
    glVertexAttribPointer(10, 3, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void *)offsetof(InstanceData, textureScale));
    glVertexAttribDivisor(10, 1);

    // Unbind all to prevent accidentally modifying them
    glBindVertexArray(0);                     // VAO
    glBindBuffer(GL_ARRAY_BUFFER, 0);         // VBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // EBO

    // std::string tag = std::string("MeshLoadAsset p=") + path + " m=" + meshName;
    std::string tag = ("Not enough data to set tag");
    // Sorry i had to change this because it used to be in a function that
    // has these params but now i refactored and split the function
    // shit happens
    checkOpenGLError(tag.c_str());
}

void AssetMesh::sendInstanceDataToGpu()
{
    // Re-upload modified instance data
    if (!instanceData.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, this->instanceVBO);
        glBufferSubData(
            GL_ARRAY_BUFFER,
            0,
            instanceData.size() * sizeof(InstanceData),
            instanceData.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

struct ShaderProgram
{
    static const char *DEFAULT_VERTEX_SHADER;
    static const char *DEFAULT_FRAGMENT_SHADER;
    GLuint id;

    void initDefaultShaderProgram();

    void initShaderProgram(
        const char *vertexShaderText,
        const char *fragmentShaderText);

    // use raw data for perf
    void updateBoneTransformData(std::vector<glm::mat4>);

    void updateDiffuseTexture(Texture &texture);

    void updateTextureScaling(glm::vec3 textureScaling);
    void updateTileSize(glm::vec2 tileSize);
    void updateAtlasStartAndScale(glm::vec2 atlasStart, float atlasScale);
    void updateTextureParamsInOneGo(
        glm::vec3 textureScaling, //rename to density TODO
        glm::vec2 tileSize,
        glm::vec2 atlasStart,
        float atlasScale
    );

    void updateLightPos(glm::vec3 lightPos);
    void updateDepthMap(GLuint depthMap, glm::mat4 lightSpaceMatrix); // #4shadows

    void renderRealMesh(
        AssetMesh &realMesh,
        glm::mat4 modelMatrix,
        glm::mat4 cameraMatrix,
        glm::mat4 projectionMatrix);
};

const char *ShaderProgram::DEFAULT_VERTEX_SHADER =
    GLSL_VERSION
    R"(
	precision highp float;

    layout (location = 0) in vec3  a_pos;
    layout (location = 1) in vec4  a_color;
    layout (location = 2) in vec2  a_texCoords;
    layout (location = 3) in vec3  a_normal;
    layout (location = 4) in ivec4 a_joints;
    layout (location = 5) in vec4  a_weights;
    layout (location = 6) in vec3  a_positionOffset;
    layout (location = 7) in vec3  a_scaleOffset;
    layout (location = 8) in vec2  a_atlasStart;
    layout (location = 9) in vec4  a_instRot;
    layout (location =10) in vec3  a_textureScale;

    out vec3 v_crntPos;
    out vec4 v_color;
    out vec2 v_texCoords;
    out vec3 v_normal;
    out vec3 v_lightPos;
    out vec3 v_textureScale;
    out vec2 v_atlasStart;

    const int MAX_BONES = 47;

    // Converts world-space coordinates to view-space coordinates (camera space).
    // Applied to all objects in the scene to align with the camera's position and orientation.
    uniform mat4 u_worldToView;

    // Converts model-space coordinates to world-space coordinates.
    // This matrix transforms an object's local vertices into the global scene.
    // Applied to each model to position, scale, and rotate it within the world.
    uniform mat4 u_modelToWorld;

    // Projection matrix: transforms view-space coordinates to clip-space coordinates.
    // Determines how objects are projected onto the screen (perspective or orthographic).
    uniform mat4 u_projection;

    // Array of bone transformation matrices for skeletal animation.
    // Each matrix in u_bones adjusts the position and rotation 
    // of a specific bone in model space.
    // MAX_BONES sets the maximum number of bones per model.
    uniform mat4 u_bones[MAX_BONES];

    // #4shadows
    uniform mat4 u_lightSpaceMatrix;
    uniform vec3 u_lightPos;
    out vec4 FragPosLightSpace;

    /* Helper function to apply rotation quat */
    vec3 rotateVecByQuat(vec3 v, vec4 q) {
        // Quaternion multiplication: q * v * q^-1
        vec3 u = q.xyz;
        float s = q.w;

        return 2.0 * dot(u, v) * u
            + (s * s - dot(u, u)) * v
            + 2.0 * s * cross(u, v);
    }

    void main() {

        v_crntPos = a_pos;
        v_lightPos = u_lightPos;

        v_texCoords   = a_texCoords;
        v_color = a_color;
        v_textureScale = a_textureScale;
        v_atlasStart = a_atlasStart;

        mat4 boneTransform = mat4(1.0);
        if (a_joints[0] != -1) {
            boneTransform     = u_bones[a_joints[0]] * a_weights[0];
            boneTransform     += u_bones[a_joints[1]] * a_weights[1];
            boneTransform     += u_bones[a_joints[2]] * a_weights[2];
            boneTransform     += u_bones[a_joints[3]] * a_weights[3];
        }
        vec4 localAnimatedPos       = boneTransform * vec4(a_pos, 1.0f);          // Apply bone transform
        vec3 scaledPosition         = localAnimatedPos.xyz * a_scaleOffset;       // Apply instance TRS on it
        vec3 rotatedPosition        = rotateVecByQuat(scaledPosition, a_instRot);
        vec3 localAnimatedScaledPos = rotatedPosition + a_positionOffset;
        vec4 animatedPos = u_modelToWorld * vec4(localAnimatedScaledPos, 1.0f);   // Finaly apply model MTX

        // for #4shadows out
        // vec4 worldPos = model * vec4(aPos, 1.0);
        FragPosLightSpace = u_lightSpaceMatrix * animatedPos;

        v_crntPos = animatedPos.xyz; // Actually this is correct
        gl_Position = u_projection * u_worldToView * animatedPos;

        // Recalculate normals 
        mat4 normalMatrix = mat4(u_modelToWorld);
        normalMatrix = transpose(inverse(normalMatrix));
        // Rotate the normal using the calculated normal matrix
        vec4 animatedNormal = normalMatrix * boneTransform * vec4(a_normal, 0.0f);
        v_normal = normalize(vec3(animatedNormal));
	}
	)";

const char *ShaderProgram::DEFAULT_FRAGMENT_SHADER =
    GLSL_VERSION
    R"(
	precision highp float;

    in vec3 v_crntPos;
    in vec4 v_color;
    in vec2 v_texCoords;
    in vec3 v_normal;
    in vec3 v_lightPos;
    in vec3 v_textureScale;
    in vec2 v_atlasStart;

	uniform sampler2D u_diffuseTexture;

    uniform vec3 u_textureScale;
    uniform vec2 u_tileSize;
    // uniform vec2 u_atlasStart;
    uniform float u_atlasScale;

    out vec4 FragColor;

    // #4shadows
    in vec4 FragPosLightSpace;
    uniform sampler2D shadowMap;

    // This function is also #4shadows
    float ShadowCalculation(vec4 fragPosLightSpace)
    {
        // Perform perspective divide
        vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
        // Transform to [0,1] range
        projCoords = projCoords * 0.5 + 0.5;

        // Get closest depth from shadow map
        float closestDepth = texture(shadowMap, projCoords.xy).r;
        // Current fragment depth in light space
        float currentDepth = projCoords.z;

        // Apply bias to reduce shadow acne
        float bias = 0.005;
        float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;

        return shadow;
    }

    void main() {
        vec2 texCoords = v_texCoords * u_atlasScale;

        vec3 lightPos = v_lightPos;

        // ambient lighting
        float ambient = 0.4f;

        // diffuse lighting based on point light
        vec3 normal = normalize(v_normal);
        vec3 lightDirection = normalize(lightPos - v_crntPos);
        float diffuse = 0.6f * max(dot(normal, lightDirection), 0.0f);

        /* 
        // diffuse lighting based on directional light 
        vec3 normal = normalize(v_normal);
        vec3 lightDirection = normalize(lightPos); // lightDirection should point towards the scene
        float diffuse = 0.6f * max(dot(normal, lightDirection), 0.0f);
        */

        // vec2 u_textureOffset = vec2(0.0, 0.0);

        // Determine the tile start using a formula
        vec2 tileStart = v_atlasStart + floor(texCoords / u_tileSize) * u_tileSize;

        // Determine which components of v_textureScale to use based on the normal
        vec3 absNormal = abs(v_normal); // Absolute value of the normal

        vec2 textureScale;
        if (absNormal.x > absNormal.y && absNormal.x > absNormal.z) {
            // Surface is mostly facing X, ignore the x component of v_textureScale
            textureScale = vec2(v_textureScale.z, v_textureScale.y)
                         * vec2(u_textureScale.z, u_textureScale.y);
        } else if (absNormal.y > absNormal.x && absNormal.y > absNormal.z) {
            // Surface is mostly facing Y, ignore the y component of v_textureScale
            textureScale = vec2(v_textureScale.x, v_textureScale.z)
                         * vec2(u_textureScale.x, u_textureScale.z);

        } else {
            // Surface is mostly facing Z, ignore the z component of v_textureScale
            textureScale = vec2(v_textureScale.x, v_textureScale.y)
                         * vec2(u_textureScale.x, u_textureScale.y);
        }

        // Scale and repeat the texture coordinates
        vec2 scaledUVs = texCoords * textureScale;
        vec2 repeatedUVs = fract(scaledUVs);

        // Map the repeated UVs to the specific tile in the texture atlas
        vec2 tileUVs = tileStart + repeatedUVs * u_tileSize;

        // Sample the texture using the tile UVs
        vec4 surfaceColor = texture(u_diffuseTexture, tileUVs).rgba;

        vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);

        // #4shadows
        float shadow = ShadowCalculation(FragPosLightSpace);
        FragColor = surfaceColor * vec4(vec3(lightColor * (ambient + diffuse)) * (1.0 - shadow * 0.2), 1.0f) ;
    }
    )";

void ShaderProgram::initDefaultShaderProgram()
{
    this->id = vtx::createShaderProgram(
        ShaderProgram::DEFAULT_VERTEX_SHADER,
        ShaderProgram::DEFAULT_FRAGMENT_SHADER);
}

void ShaderProgram::initShaderProgram(
    const char *vertexShaderText,
    const char *fragmentShaderText)
{
    this->id = vtx::createShaderProgram(
        vertexShaderText, fragmentShaderText);
}

void ShaderProgram::updateBoneTransformData(std::vector<glm::mat4> transformMatrices)
{
    int count = transformMatrices.size();
    if (count >= 47)
    {
        std::cerr << "Too many bones in file for mesh" << count <<  std::endl;
        // vtx::exitVortex(1);
        abort();
    }
    // TODO if index exceedds max bones then exit
    // glUseProgram(this->id);
    glUniformMatrix4fv(
        glGetUniformLocation(this->id, "u_bones"),  // Loc
        count,                                      // count
        GL_TRUE,                                    // transpose
        glm::value_ptr(transformMatrices.data()[0]) // put only one value in specific index
    );
}

void ShaderProgram::updateDiffuseTexture(Texture &diffuseTexture)
{
    glUseProgram(this->id);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, diffuseTexture.id);

    glUniform1i(
        glGetUniformLocation(
            this->id, "u_diffuseTexture"),
        0);
    checkOpenGLError();
}

/**
 * These are for textures that contains mutltiple assets like the ones for clats
 * Remember that tileSize still is global ratio of the tile
 */
void ShaderProgram::updateTextureScaling(glm::vec3 textureScaling)
{
    // glUseProgram(this->id);
    glUniform3f(
        glGetUniformLocation(this->id, "u_textureScale"),
        textureScaling.x,
        textureScaling.y,
        textureScaling.z);
    checkOpenGLError();
}

void ShaderProgram::updateTileSize(glm::vec2 tileSize)
{
    // glUseProgram(this->id);
    glUniform2f(
        glGetUniformLocation(this->id, "u_tileSize"),
        tileSize.x,
        tileSize.y);
    checkOpenGLError();
}

void ShaderProgram::updateAtlasStartAndScale(glm::vec2 atlasStart, float atlasScale)
{
    // glUseProgram(this->id);

    glUniform1f(
        glGetUniformLocation(this->id, "u_atlasScale"),
        atlasScale);
    checkOpenGLError();
}
void ShaderProgram::updateTextureParamsInOneGo(
    glm::vec3 textureScaling, //rename to density TODO
    glm::vec2 tileSize,
    glm::vec2 atlasStart,
    float atlasScale
) {
    // glUseProgram(this->id);

    glUniform3f(
        glGetUniformLocation(this->id, "u_textureScale"),
        textureScaling.x,
        textureScaling.y,
        textureScaling.z);
    glUniform2f(
        glGetUniformLocation(this->id, "u_tileSize"),
        tileSize.x,
        tileSize.y);
    glUniform1f(
        glGetUniformLocation(this->id, "u_atlasScale"),
        atlasScale);

    checkOpenGLError();
}

// NOTE: this goes hand-in-hand with the lightSpaceMatrix bellow
void ShaderProgram::updateLightPos(glm::vec3 lightPos)
{
    // Does not seems to be called....
    // glUseProgram(this->id);
    glUniform3f(
        glGetUniformLocation(this->id, "u_lightPos"),
        lightPos.x,
        lightPos.y,
        lightPos.z);
    checkOpenGLError();
}

/**
 * Set the viewport back to the screen dimensions and render the scene as usual,
 * but pass the lightSpaceMatrix and the depth map texture as uniforms to your main shader.
 * #4shadows
 */
void ShaderProgram::updateDepthMap(GLuint depthMap, glm::mat4 lightSpaceMatrix)
{
    // Does not seems to be called....
    // glUseProgram(this->id);
    glUniformMatrix4fv(glGetUniformLocation(this->id, "u_lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glUniform1i(glGetUniformLocation(this->id, "shadowMap"), 1);
    checkOpenGLError();
}

void ShaderProgram::renderRealMesh(
    AssetMesh &assetMesh,
    glm::mat4 modelMatrix,
    glm::mat4 cameraMatrix,
    glm::mat4 projectionMatrix)
{
    // glUseProgram(this->id);

    glUniformMatrix4fv(
        glGetUniformLocation(
            this->id, "u_projection"),   // uniform location
        1,                               // number of matrices
        GL_FALSE,                        // transpose
        glm::value_ptr(projectionMatrix) // value
    );

    glUniformMatrix4fv(
        glGetUniformLocation(this->id, "u_modelToWorld"), // uniform location
        1,                                                // number of matrices
        GL_FALSE,                                         // transpose
        glm::value_ptr(modelMatrix)                       // value
    );

    glUniformMatrix4fv(
        glGetUniformLocation(this->id, "u_worldToView"), // uniform location
        1,                                               // number of matrices
        GL_FALSE,                                        // transpose
        glm::value_ptr(cameraMatrix)                     // value
    );

    glBindVertexArray(assetMesh.meshVAO);


    glDrawElementsInstanced(
        GL_TRIANGLES, // Mode
        assetMesh.indexCount,
        //  .indices.size(),  // Index count
        GL_UNSIGNED_INT,                    // Data type of indices array
        (void *)(0 * sizeof(unsigned int)), // Indices pointer
        assetMesh.instanceData.size()       // Instance count
    );
    glBindVertexArray(0);
}
