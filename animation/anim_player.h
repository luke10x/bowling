#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include <cstdint>
#include <cstring>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "../assets/api/anim_data.h"

// Minimal runtime animation evaluator for AssmanAnimView.
// This is intentionally small: one clip, no blending, no events.

struct AssmanAnimPlayer
{
    AssmanAnimView anim = {};
    // Cached clip offsets (computed once after load).
    std::vector<const uint8_t*> clipPtrs;

    int activeClip = 0;
    float t = 0.0f; // seconds
    bool loop = true;

    // Outputs (reused buffers to avoid allocations per frame)
    std::vector<glm::mat4> boneMatrices; // final matrices (global * inverseBind)
    std::vector<glm::mat4> globalMatrices;
    std::vector<glm::mat4> localMatrices;

    void loadFromBlob(const uint8_t* blob, size_t blobLen)
    {
        anim = loadAnimFromBlob(blob, blobLen);
        clipPtrs.clear();
        clipPtrs.reserve(anim.header->clipCount);

        const uint8_t* cursor = anim.clipsStart;
        for (uint32_t ci = 0; ci < anim.header->clipCount; ++ci)
        {
            clipPtrs.push_back(cursor);

            // Skip clip header + tracks + key payload
            const auto* ch = reinterpret_cast<const AssmanAnimClipHeader*>(cursor);
            cursor += sizeof(AssmanAnimClipHeader);

            const auto* tracks = reinterpret_cast<const AssmanAnimTrackHeader*>(cursor);
            cursor += sizeof(AssmanAnimTrackHeader) * anim.header->boneCount;

            // Walk key payload.
            for (uint32_t bi = 0; bi < anim.header->boneCount; ++bi)
            {
                cursor += sizeof(AssmanAnimPosKey) * tracks[bi].posKeyCount;
                cursor += sizeof(AssmanAnimRotKey) * tracks[bi].rotKeyCount;
                cursor += sizeof(AssmanAnimScaleKey) * tracks[bi].scaleKeyCount;
            }
        }

        boneMatrices.resize(anim.header->boneCount, glm::mat4(1.0f));
        globalMatrices.resize(anim.header->boneCount, glm::mat4(1.0f));
        localMatrices.resize(anim.header->boneCount, glm::mat4(1.0f));
    }

    int findClipByName(const char* name) const
    {
        for (uint32_t ci = 0; ci < anim.header->clipCount; ++ci)
        {
            const auto* ch = reinterpret_cast<const AssmanAnimClipHeader*>(clipPtrs[ci]);
            if (std::strncmp(animNameCStr(ch->name), name, 64) == 0)
                return (int)ci;
        }
        return -1;
    }

    void setClip(int clipIndex, bool resetTime = true)
    {
        if (clipIndex < 0 || clipIndex >= (int)anim.header->clipCount)
            return;
        activeClip = clipIndex;
        if (resetTime)
            t = 0.0f;
    }

    static inline glm::mat4 mat4FromArray16(const float m[16])
    {
        glm::mat4 out(1.0f);
        // column-major
        std::memcpy(&out[0][0], m, sizeof(float) * 16);
        return out;
    }

    static inline glm::vec3 lerp(const glm::vec3& a, const glm::vec3& b, float f)
    {
        return a + (b - a) * f;
    }

    static inline glm::vec3 sampleVec3(const AssmanAnimPosKey* keys, uint32_t n, float tSec, const glm::vec3& fallback)
    {
        if (n == 0)
            return fallback;
        if (n == 1 || tSec <= keys[0].t)
            return glm::vec3(keys[0].x, keys[0].y, keys[0].z);
        if (tSec >= keys[n - 1].t)
            return glm::vec3(keys[n - 1].x, keys[n - 1].y, keys[n - 1].z);
        uint32_t i = 0;
        while (i + 1 < n && keys[i + 1].t < tSec) ++i;
        uint32_t j = i + 1;
        float dt = keys[j].t - keys[i].t;
        float f = (dt > 0.0f) ? (tSec - keys[i].t) / dt : 0.0f;
        return lerp(
            glm::vec3(keys[i].x, keys[i].y, keys[i].z),
            glm::vec3(keys[j].x, keys[j].y, keys[j].z),
            glm::clamp(f, 0.0f, 1.0f)
        );
    }

    static inline glm::vec3 sampleScale3(const AssmanAnimScaleKey* keys, uint32_t n, float tSec, const glm::vec3& fallback)
    {
        if (n == 0)
            return fallback;
        if (n == 1 || tSec <= keys[0].t)
            return glm::vec3(keys[0].x, keys[0].y, keys[0].z);
        if (tSec >= keys[n - 1].t)
            return glm::vec3(keys[n - 1].x, keys[n - 1].y, keys[n - 1].z);
        uint32_t i = 0;
        while (i + 1 < n && keys[i + 1].t < tSec) ++i;
        uint32_t j = i + 1;
        float dt = keys[j].t - keys[i].t;
        float f = (dt > 0.0f) ? (tSec - keys[i].t) / dt : 0.0f;
        return lerp(
            glm::vec3(keys[i].x, keys[i].y, keys[i].z),
            glm::vec3(keys[j].x, keys[j].y, keys[j].z),
            glm::clamp(f, 0.0f, 1.0f)
        );
    }

    static inline glm::quat sampleQuat(const AssmanAnimRotKey* keys, uint32_t n, float tSec, const glm::quat& fallback)
    {
        if (n == 0)
            return fallback;
        if (n == 1 || tSec <= keys[0].t)
            return glm::quat(keys[0].w, keys[0].x, keys[0].y, keys[0].z);
        if (tSec >= keys[n - 1].t)
            return glm::quat(keys[n - 1].w, keys[n - 1].x, keys[n - 1].y, keys[n - 1].z);
        uint32_t i = 0;
        while (i + 1 < n && keys[i + 1].t < tSec) ++i;
        uint32_t j = i + 1;
        float dt = keys[j].t - keys[i].t;
        float f = (dt > 0.0f) ? (tSec - keys[i].t) / dt : 0.0f;
        f = glm::clamp(f, 0.0f, 1.0f);
        glm::quat a(keys[i].w, keys[i].x, keys[i].y, keys[i].z);
        glm::quat b(keys[j].w, keys[j].x, keys[j].y, keys[j].z);
        return glm::normalize(glm::slerp(a, b, f));
    }

    void tick(float dtSeconds)
    {
        if (!anim.header || anim.header->clipCount == 0)
            return;
        const auto* ch = reinterpret_cast<const AssmanAnimClipHeader*>(clipPtrs[activeClip]);
        float dur = glm::max(0.001f, ch->durationSeconds);
        t += dtSeconds;
        if (loop)
        {
            while (t >= dur) t -= dur;
        }
        else
        {
            if (t > dur) t = dur;
        }
    }

    // Evaluates active clip at current t, producing `boneMatrices` (final matrices).
    // Returns the boneMatrices buffer (for convenience).
    const std::vector<glm::mat4>& evaluate()
    {
        if (!anim.header || anim.header->clipCount == 0)
            return boneMatrices;

        const uint32_t boneCount = anim.header->boneCount;

        const uint8_t* cursor = clipPtrs[activeClip];
        const auto* ch = reinterpret_cast<const AssmanAnimClipHeader*>(cursor);
        cursor += sizeof(AssmanAnimClipHeader);
        const auto* tracks = reinterpret_cast<const AssmanAnimTrackHeader*>(cursor);
        cursor += sizeof(AssmanAnimTrackHeader) * boneCount;

        float tSec = glm::clamp(t, 0.0f, glm::max(0.0f, ch->durationSeconds));

        // Pass 1: compute local matrices for each bone (bindLocal overridden by sampled TRS if keys exist).
        for (uint32_t bi = 0; bi < boneCount; ++bi)
        {
            const AssmanAnimTrackHeader& th = tracks[bi];
            const auto* posKeys = reinterpret_cast<const AssmanAnimPosKey*>(cursor);
            cursor += sizeof(AssmanAnimPosKey) * th.posKeyCount;
            const auto* rotKeys = reinterpret_cast<const AssmanAnimRotKey*>(cursor);
            cursor += sizeof(AssmanAnimRotKey) * th.rotKeyCount;
            const auto* scaleKeys = reinterpret_cast<const AssmanAnimScaleKey*>(cursor);
            cursor += sizeof(AssmanAnimScaleKey) * th.scaleKeyCount;

            glm::mat4 bindLocal = mat4FromArray16(anim.bones[bi].bindLocal);
            glm::vec3 bindScale(1.0f);
            glm::quat bindRot(1, 0, 0, 0);
            glm::vec3 bindT(0.0f);
            glm::vec3 skew;
            glm::vec4 perspective;
            // If decomposition fails, keep identity fallbacks.
            (void)glm::decompose(bindLocal, bindScale, bindRot, bindT, skew, perspective);

            glm::vec3 T = sampleVec3(posKeys, th.posKeyCount, tSec, bindT);
            glm::quat R = sampleQuat(rotKeys, th.rotKeyCount, tSec, bindRot);
            glm::vec3 S = sampleScale3(scaleKeys, th.scaleKeyCount, tSec, bindScale);

            glm::mat4 M(1.0f);
            M = glm::translate(M, T) * glm::toMat4(R) * glm::scale(glm::mat4(1.0f), S);
            localMatrices[bi] = M;
        }

        // Pass 2: global + final.
        for (uint32_t bi = 0; bi < boneCount; ++bi)
        {
            int32_t parent = anim.bones[bi].parentIndex;
            if (parent == ASSMAN_ANIM_NO_PARENT)
                globalMatrices[bi] = localMatrices[bi];
            else
                globalMatrices[bi] = globalMatrices[(uint32_t)parent] * localMatrices[bi];

            glm::mat4 invBind = mat4FromArray16(anim.bones[bi].inverseBind);
            boneMatrices[bi] = globalMatrices[bi] * invBind;
        }

        return boneMatrices;
    }
};
