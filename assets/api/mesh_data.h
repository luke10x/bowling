#pragma once

#include <cstdint>
#include <cstring>
#include <iostream>

struct Vertex
{
    struct
    {
        float x, y, z;
    } position;
    struct
    {
        float r, g, b, a;
    } color;
    struct
    {
        float u, v;
    } texCoords;
    struct
    {
        float x, y, z;
    } normal;
    // Up to 4 bone indices (use -1 for no bone)
    int bones[4] = {-1, -1, -1, -1};
    // Up to 4 bone weights
    float weights[4] = {0.0f};
};


struct MeshData {
    uint32_t vertexCount;
    uint32_t indexCount;
    Vertex*   vertices;
    uint32_t* indices;
};


#pragma pack(push, 1)
struct MeshDataHeader {
    uint32_t vertexCount;
    uint32_t indexCount;
};
#pragma pack(pop)


inline MeshData loadMeshFromBlob(const uint8_t* blob, size_t blobLen)
{
    if (!blob || blobLen < sizeof(MeshDataHeader)) {
        std::cerr << "Blob too small " << std::endl;
        exit(1);
    }

    const MeshDataHeader* header = reinterpret_cast<const MeshDataHeader*>(blob);

    size_t vertexBytes = sizeof(Vertex) * header->vertexCount;
    size_t indexBytes  = sizeof(uint32_t) * header->indexCount;

    size_t requiredSize =
        sizeof(MeshDataHeader) +
        vertexBytes +
        indexBytes;

    if (blobLen < requiredSize)
        throw std::runtime_error("Blob does not contain enough data.");

    // Compute pointers inside the blob
    const uint8_t* cursor = blob + sizeof(MeshDataHeader);

    Vertex* vertices = reinterpret_cast<Vertex*>(const_cast<uint8_t*>(cursor));
    cursor += vertexBytes;

    uint32_t* indices = reinterpret_cast<uint32_t*>(const_cast<uint8_t*>(cursor));

    MeshData md;
    md.vertexCount = header->vertexCount;
    md.indexCount  = header->indexCount;
    md.vertices    = vertices;
    md.indices     = indices;

    return md;
}
