#pragma once

#include <stdint.h>

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

//[header][vertex data][index data]
// auto header = reinterpret_cast<const MeshDataHeader*>(blob);
// auto vertices = reinterpret_cast<const float*>(blob + sizeof(MeshDataHeader));
// auto indices = reinterpret_cast<const uint32_t*>(blob + sizeof(MeshDataHeader)
                                                // + header->vertexCount * 3 * sizeof(float));