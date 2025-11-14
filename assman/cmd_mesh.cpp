#include <cassert>
#include <fstream>
#include <iostream>
#include <vector>
#include <iomanip>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "api/mesh_data.h"

void loadMeshDataFromAsset(
    std::vector<Vertex> &vertices,
    std::vector<uint32_t> &indices,
    const char *path,
    const char *meshName,
    const int numInstances)
{
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(
        path, // path of the file
        aiProcess_Triangulate | aiProcess_FlipUVs |
            aiProcess_CalcTangentSpace);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
        !scene->mRootNode)
    {
        std::cerr << "Error loading model: "
                  << importer.GetErrorString() << std::endl;
        exit(1);
        return;
    }

    aiMesh *mesh = nullptr;
    unsigned int meshIndex = 0;
    // Iterate through all the meshes in the scene
    for (meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++)
    {

        aiMesh *currentMesh = scene->mMeshes[meshIndex];
        // Print the name of the mesh
        if (currentMesh->mName.length > 0)
        {
            const char *currentMeshName = currentMesh->mName.C_Str();
            if (strcmp(currentMeshName, meshName) == 0)
            {
                mesh = currentMesh;
                break; // Exit the loop
            }
        }
    }
    if (mesh == nullptr)
    {
        std::cerr << "Error loading mesh: " << meshName
                    << " File " << path << " contains the following meshes:"
                    << std::endl; // Fallback name

        for (meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++)
        {
            aiMesh *currentMesh = scene->mMeshes[meshIndex];
            // Print the name of the mesh
            if (currentMesh->mName.length > 0)
            {
                std::cerr << "  - " << currentMesh->mName.C_Str() << std::endl;
            }
        }
                exit(1);
        exit(1);
    }

    // Print the name of the mesh
    if (mesh->mName.length > 0)
    {
    }
    else
    {
        // Fallback name
    }

    // Initialize a vertex bone weight map to hold weights for each
    // vertex
    std::vector<std::vector<std::pair<int, float>>> vertexWeights(
        mesh->mNumVertices);

    // Process bones
    for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones;
         ++boneIndex)
    {
        aiBone *bone = mesh->mBones[boneIndex];

        // Iterate through all the vertices affected by this bone
        for (unsigned int weightIndex = 0;
             weightIndex < bone->mNumWeights; ++weightIndex)
        {
            aiVertexWeight weight = bone->mWeights[weightIndex];
            size_t vertexId = weight.mVertexId;
            float boneWeight = weight.mWeight;

            // Store the bone index and weight in the vertex weight
            // map
            vertexWeights[vertexId].emplace_back(boneIndex, boneWeight);
        }
    }

    aiMaterial *material =
        scene->mMaterials[mesh->mMaterialIndex]; // Get the
                                                 // material
                                                 // assigned to
                                                 // the mesh
    
    // Extract vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;

        // Position
        vertex.position.x = mesh->mVertices[i].x;
        vertex.position.y = mesh->mVertices[i].y;
        vertex.position.z = mesh->mVertices[i].z;

        // Try to get the base color from the material
        aiColor4D baseColor(
            1.0f, 1.0f, 1.0f, 1.0f); // Default color is white

        if (AI_SUCCESS !=
            aiGetMaterialColor(
                material, AI_MATKEY_COLOR_DIFFUSE, &baseColor))
        {
            std::cerr << "Cannot get material color" << material->GetName().C_Str() << std::endl;
            exit(1);
        }

        // Colour (if available, otherwise default white)
        if (mesh->HasVertexColors(0))
        {
            vertex.color.r = mesh->mColors[0][i].r;
            vertex.color.g = mesh->mColors[0][i].g;
            vertex.color.b = mesh->mColors[0][i].b;
            vertex.color.a = 1.0f;
        }
        else
        {
            vertex.color.r = baseColor.r;
            vertex.color.g = baseColor.g;
            vertex.color.b = baseColor.b;
            vertex.color.a = 1.0;
            ;
        }

        vertex.texCoords.u = mesh->mTextureCoords[0][i].x;
        vertex.texCoords.v = mesh->mTextureCoords[0][i].y;

        // Normal (if available)
        if (mesh->HasNormals())
        {
            vertex.normal.x = mesh->mNormals[i].x;
            vertex.normal.y = mesh->mNormals[i].y;
            vertex.normal.z = mesh->mNormals[i].z;
        }

        // Process bone weights and indices for this vertex
        auto &weights = vertexWeights[i];

        // Sort by weight and keep the top 4 weights
        std::sort(
            weights.begin(), weights.end(),
            [](const auto &a, const auto &b)
            {
                return a.second >
                       b.second; // Sort by weight descending
            });

        // Store up to 4 bones and their weights
        for (size_t j = 0; j < weights.size() && j < 4; ++j)
        {
            vertex.bones[j] = weights[j].first;    // Bone index
            vertex.weights[j] = weights[j].second; // Bone weight
        }

        // Normalize weights to sum up to 1
        float weightSum = vertex.weights[0] + vertex.weights[1] +
                          vertex.weights[2] + vertex.weights[3];
        if (weightSum > 0.0f)
        {
            for (int j = 0; j < 4; ++j)
            {
                vertex.weights[j] /= weightSum;
            }
        }

        vertices.push_back(vertex);
    }

    // Extract indices
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
        {
            indices.push_back(face.mIndices[j]);
        }
    }
}
#include <iostream>
#include <vector>
#include <cassert>
#include <typeinfo>
#include "api/mesh_data.h"
#include <iomanip>

void writeMeshDataToFile(const MeshData& md, const std::string& outPath)
{
    std::ofstream out(outPath, std::ios::binary);
    if (!out)
        throw std::runtime_error("Could not open output file: " + outPath);

    // Write header (counts)
    out.write(reinterpret_cast<const char*>(&md.vertexCount), sizeof(md.vertexCount));
    out.write(reinterpret_cast<const char*>(&md.indexCount),  sizeof(md.indexCount));

    // Write vertex buffer
    out.write(reinterpret_cast<const char*>(md.vertices),
              md.vertexCount * sizeof(Vertex));

    // Write index buffer
    out.write(reinterpret_cast<const char*>(md.indices),
              md.indexCount * sizeof(uint32_t));
}

int cmd_mesh(std::string inputPath, std::string meshName, std::string outPath)
{
    std::vector<Vertex> vertexAcc;
    std::vector<uint32_t> indexAcc;

    loadMeshDataFromAsset(vertexAcc, indexAcc, inputPath.c_str(), meshName.c_str(), 1);

    MeshData md;
    md.vertexCount = static_cast<uint32_t>(vertexAcc.size());
    md.vertices = vertexAcc.data();
    md.indexCount  = static_cast<uint32_t>(indexAcc.size());
    md.indices = indexAcc.data();
    writeMeshDataToFile(md, outPath); 

    return 0;
}
