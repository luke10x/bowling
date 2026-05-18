#pragma once

#include <cstdint>

// Binary format written by `assman animation`.
// All integers are little-endian. All floats are IEEE754.

static constexpr uint32_t ASSMAN_ANIM_MAGIC = 0x4D494E41u; // 'ANIM'
static constexpr uint32_t ASSMAN_ANIM_VERSION = 1u;
static constexpr int32_t ASSMAN_ANIM_NO_PARENT = -1;

#pragma pack(push, 1)
struct AssmanAnimHeader
{
    uint32_t magic;   // ASSMAN_ANIM_MAGIC
    uint32_t version; // ASSMAN_ANIM_VERSION
    uint32_t boneCount;
    uint32_t clipCount;
};

struct AssmanAnimBoneInfo
{
    char name[64];         // zero-terminated if shorter
    int32_t parentIndex;   // -1 if none
    float inverseBind[16]; // column-major mat4
    float bindLocal[16];   // column-major mat4 (rest pose local)
};

struct AssmanAnimClipHeader
{
    char name[64];        // zero-terminated if shorter
    float durationSeconds;
    float ticksPerSecond; // the tps used during export (never 0)
};

struct AssmanAnimTrackHeader
{
    uint32_t posKeyCount;
    uint32_t rotKeyCount;
    uint32_t scaleKeyCount;
};

struct AssmanAnimPosKey
{
    float t; // seconds
    float x, y, z;
};

struct AssmanAnimRotKey
{
    float t; // seconds
    float x, y, z, w; // quaternion (x,y,z,w)
};

struct AssmanAnimScaleKey
{
    float t; // seconds
    float x, y, z;
};
#pragma pack(pop)

