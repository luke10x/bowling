#pragma once

#include <cstdint>
#include <cstring>
#include <stdexcept>

// Runtime view helpers for the binary format written by `assman animation`.
// This header intentionally mirrors `assman/api/anim_data.h`, but also provides
// parsing helpers that return pointers into the provided blob (no allocations).

static constexpr uint32_t ASSMAN_ANIM_MAGIC = 0x4D494E41u; // 'ANIM'
static constexpr uint32_t ASSMAN_ANIM_VERSION = 1u;
static constexpr int32_t ASSMAN_ANIM_NO_PARENT = -1;

#pragma pack(push, 1)
struct AssmanAnimHeader
{
    uint32_t magic;
    uint32_t version;
    uint32_t boneCount;
    uint32_t clipCount;
};

struct AssmanAnimBoneInfo
{
    char name[64];
    int32_t parentIndex;
    float inverseBind[16];
    float bindLocal[16];
};

struct AssmanAnimClipHeader
{
    char name[64];
    float durationSeconds;
    float ticksPerSecond;
};

struct AssmanAnimTrackHeader
{
    uint32_t posKeyCount;
    uint32_t rotKeyCount;
    uint32_t scaleKeyCount;
};

struct AssmanAnimPosKey
{
    float t;
    float x, y, z;
};

struct AssmanAnimRotKey
{
    float t;
    float x, y, z, w;
};

struct AssmanAnimScaleKey
{
    float t;
    float x, y, z;
};
#pragma pack(pop)

struct AssmanAnimClipView
{
    const AssmanAnimClipHeader* header = nullptr;
    // Track headers are [boneCount] in bone order.
    const AssmanAnimTrackHeader* tracks = nullptr;
    // Key data starts immediately after the tracks array, in track order:
    //  posKeys[], rotKeys[], scaleKeys[] for bone 0, then bone 1, ...
    const uint8_t* keyData = nullptr;
};

struct AssmanAnimView
{
    const AssmanAnimHeader* header = nullptr;
    const AssmanAnimBoneInfo* bones = nullptr; // [boneCount]
    // Clips are laid out sequentially after bones.
    const uint8_t* clipsStart = nullptr;
};

inline AssmanAnimView loadAnimFromBlob(const uint8_t* blob, size_t blobLen)
{
    if (!blob || blobLen < sizeof(AssmanAnimHeader))
        throw std::runtime_error("Anim blob too small");

    const auto* hdr = reinterpret_cast<const AssmanAnimHeader*>(blob);
    if (hdr->magic != ASSMAN_ANIM_MAGIC)
        throw std::runtime_error("Anim blob bad magic");
    if (hdr->version != ASSMAN_ANIM_VERSION)
        throw std::runtime_error("Anim blob unsupported version");

    size_t bonesBytes = sizeof(AssmanAnimBoneInfo) * hdr->boneCount;
    size_t minBytes = sizeof(AssmanAnimHeader) + bonesBytes;
    if (blobLen < minBytes)
        throw std::runtime_error("Anim blob truncated (bones)");

    const uint8_t* cursor = blob + sizeof(AssmanAnimHeader);
    const auto* bones = reinterpret_cast<const AssmanAnimBoneInfo*>(cursor);
    cursor += bonesBytes;

    AssmanAnimView v;
    v.header = hdr;
    v.bones = bones;
    v.clipsStart = cursor;
    return v;
}

inline const char* animNameCStr(const char name64[64])
{
    // Ensure it is terminated for safety; this does not modify input.
    static thread_local char tmp[65];
    std::memcpy(tmp, name64, 64);
    tmp[64] = 0;
    return tmp;
}

