#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sstream>
#include <vector>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

#include "api/anim_data.h"

struct AssmanBakeAnimView
{
    const AssmanAnimHeader *header = nullptr;
    const AssmanAnimBoneInfo *bones = nullptr;
    const uint8_t *clipsStart = nullptr;
};

static const char *assman_anim_name_c_str(const char name64[64])
{
    static thread_local char tmp[65];
    std::memcpy(tmp, name64, 64);
    tmp[64] = 0;
    return tmp;
}

static AssmanBakeAnimView assman_load_anim_from_blob(const uint8_t *blob, size_t blobLen)
{
    if (!blob || blobLen < sizeof(AssmanAnimHeader))
        throw std::runtime_error("Anim blob too small");

    const auto *hdr = reinterpret_cast<const AssmanAnimHeader *>(blob);
    if (hdr->magic != ASSMAN_ANIM_MAGIC)
        throw std::runtime_error("Anim blob bad magic");
    if (hdr->version != ASSMAN_ANIM_VERSION)
        throw std::runtime_error("Anim blob unsupported version");

    const size_t bonesBytes = sizeof(AssmanAnimBoneInfo) * hdr->boneCount;
    const size_t minBytes = sizeof(AssmanAnimHeader) + bonesBytes;
    if (blobLen < minBytes)
        throw std::runtime_error("Anim blob truncated (bones)");

    const uint8_t *cursor = blob + sizeof(AssmanAnimHeader);
    AssmanBakeAnimView view;
    view.header = hdr;
    view.bones = reinterpret_cast<const AssmanAnimBoneInfo *>(cursor);
    cursor += bonesBytes;
    view.clipsStart = cursor;
    return view;
}

struct AssmanBakeAnimPlayer
{
    AssmanBakeAnimView anim = {};
    std::vector<const uint8_t *> clipPtrs;
    int activeClip = 0;
    float t = 0.0f;
    std::vector<glm::mat4> boneMatrices;
    std::vector<glm::mat4> globalMatrices;
    std::vector<glm::mat4> localMatrices;

    static inline glm::mat4 mat4FromArray16(const float m[16])
    {
        glm::mat4 out(1.0f);
        std::memcpy(&out[0][0], m, sizeof(float) * 16);
        return out;
    }

    static inline glm::vec3 lerp(const glm::vec3 &a, const glm::vec3 &b, float f)
    {
        return a + (b - a) * f;
    }

    static inline glm::vec3 sampleVec3(const AssmanAnimPosKey *keys, uint32_t n, float tSec, const glm::vec3 &fallback)
    {
        if (n == 0)
            return fallback;
        if (n == 1 || tSec <= keys[0].t)
            return glm::vec3(keys[0].x, keys[0].y, keys[0].z);
        if (tSec >= keys[n - 1].t)
            return glm::vec3(keys[n - 1].x, keys[n - 1].y, keys[n - 1].z);
        uint32_t i = 0;
        while (i + 1 < n && keys[i + 1].t < tSec)
            ++i;
        const uint32_t j = i + 1;
        const float dt = keys[j].t - keys[i].t;
        const float f = (dt > 0.0f) ? (tSec - keys[i].t) / dt : 0.0f;
        return lerp(
            glm::vec3(keys[i].x, keys[i].y, keys[i].z),
            glm::vec3(keys[j].x, keys[j].y, keys[j].z),
            glm::clamp(f, 0.0f, 1.0f)
        );
    }

    static inline glm::vec3 sampleScale3(const AssmanAnimScaleKey *keys, uint32_t n, float tSec, const glm::vec3 &fallback)
    {
        if (n == 0)
            return fallback;
        if (n == 1 || tSec <= keys[0].t)
            return glm::vec3(keys[0].x, keys[0].y, keys[0].z);
        if (tSec >= keys[n - 1].t)
            return glm::vec3(keys[n - 1].x, keys[n - 1].y, keys[n - 1].z);
        uint32_t i = 0;
        while (i + 1 < n && keys[i + 1].t < tSec)
            ++i;
        const uint32_t j = i + 1;
        const float dt = keys[j].t - keys[i].t;
        const float f = (dt > 0.0f) ? (tSec - keys[i].t) / dt : 0.0f;
        return lerp(
            glm::vec3(keys[i].x, keys[i].y, keys[i].z),
            glm::vec3(keys[j].x, keys[j].y, keys[j].z),
            glm::clamp(f, 0.0f, 1.0f)
        );
    }

    static inline glm::quat sampleQuat(const AssmanAnimRotKey *keys, uint32_t n, float tSec, const glm::quat &fallback)
    {
        if (n == 0)
            return fallback;
        if (n == 1 || tSec <= keys[0].t)
            return glm::quat(keys[0].w, keys[0].x, keys[0].y, keys[0].z);
        if (tSec >= keys[n - 1].t)
            return glm::quat(keys[n - 1].w, keys[n - 1].x, keys[n - 1].y, keys[n - 1].z);
        uint32_t i = 0;
        while (i + 1 < n && keys[i + 1].t < tSec)
            ++i;
        const uint32_t j = i + 1;
        const float dt = keys[j].t - keys[i].t;
        float f = (dt > 0.0f) ? (tSec - keys[i].t) / dt : 0.0f;
        f = glm::clamp(f, 0.0f, 1.0f);
        const glm::quat a(keys[i].w, keys[i].x, keys[i].y, keys[i].z);
        const glm::quat b(keys[j].w, keys[j].x, keys[j].y, keys[j].z);
        return glm::normalize(glm::slerp(a, b, f));
    }

    void loadFromBlob(const uint8_t *blob, size_t blobLen)
    {
        anim = assman_load_anim_from_blob(blob, blobLen);
        clipPtrs.clear();
        clipPtrs.reserve(anim.header->clipCount);

        const uint8_t *cursor = anim.clipsStart;
        for (uint32_t ci = 0; ci < anim.header->clipCount; ++ci)
        {
            clipPtrs.push_back(cursor);

            const auto *ch = reinterpret_cast<const AssmanAnimClipHeader *>(cursor);
            cursor += sizeof(AssmanAnimClipHeader);

            const auto *tracks = reinterpret_cast<const AssmanAnimTrackHeader *>(cursor);
            cursor += sizeof(AssmanAnimTrackHeader) * anim.header->boneCount;

            for (uint32_t bi = 0; bi < anim.header->boneCount; ++bi)
            {
                cursor += sizeof(AssmanAnimPosKey) * tracks[bi].posKeyCount;
                cursor += sizeof(AssmanAnimRotKey) * tracks[bi].rotKeyCount;
                cursor += sizeof(AssmanAnimScaleKey) * tracks[bi].scaleKeyCount;
            }
            (void)ch;
        }

        boneMatrices.resize(anim.header->boneCount, glm::mat4(1.0f));
        globalMatrices.resize(anim.header->boneCount, glm::mat4(1.0f));
        localMatrices.resize(anim.header->boneCount, glm::mat4(1.0f));
    }

    const std::vector<glm::mat4> &evaluate()
    {
        if (!anim.header || anim.header->clipCount == 0)
            return boneMatrices;

        const uint32_t boneCount = anim.header->boneCount;
        const uint8_t *cursor = clipPtrs[activeClip];
        const auto *ch = reinterpret_cast<const AssmanAnimClipHeader *>(cursor);
        cursor += sizeof(AssmanAnimClipHeader);
        const auto *tracks = reinterpret_cast<const AssmanAnimTrackHeader *>(cursor);
        cursor += sizeof(AssmanAnimTrackHeader) * boneCount;

        const float tSec = glm::clamp(t, 0.0f, glm::max(0.0f, ch->durationSeconds));

        for (uint32_t bi = 0; bi < boneCount; ++bi)
        {
            const AssmanAnimTrackHeader &th = tracks[bi];
            const auto *posKeys = reinterpret_cast<const AssmanAnimPosKey *>(cursor);
            cursor += sizeof(AssmanAnimPosKey) * th.posKeyCount;
            const auto *rotKeys = reinterpret_cast<const AssmanAnimRotKey *>(cursor);
            cursor += sizeof(AssmanAnimRotKey) * th.rotKeyCount;
            const auto *scaleKeys = reinterpret_cast<const AssmanAnimScaleKey *>(cursor);
            cursor += sizeof(AssmanAnimScaleKey) * th.scaleKeyCount;

            const glm::mat4 bindLocal = mat4FromArray16(anim.bones[bi].bindLocal);
            glm::vec3 bindScale(1.0f);
            glm::quat bindRot(1, 0, 0, 0);
            glm::vec3 bindT(0.0f);
            glm::vec3 skew;
            glm::vec4 perspective;
            (void)glm::decompose(bindLocal, bindScale, bindRot, bindT, skew, perspective);

            const glm::vec3 T = sampleVec3(posKeys, th.posKeyCount, tSec, bindT);
            const glm::quat R = sampleQuat(rotKeys, th.rotKeyCount, tSec, bindRot);
            const glm::vec3 S = sampleScale3(scaleKeys, th.scaleKeyCount, tSec, bindScale);

            localMatrices[bi] = glm::translate(glm::mat4(1.0f), T) * glm::toMat4(R) * glm::scale(glm::mat4(1.0f), S);
        }

        for (uint32_t bi = 0; bi < boneCount; ++bi)
        {
            const int32_t parent = anim.bones[bi].parentIndex;
            if (parent == ASSMAN_ANIM_NO_PARENT)
                globalMatrices[bi] = localMatrices[bi];
            else
                globalMatrices[bi] = globalMatrices[(uint32_t)parent] * localMatrices[bi];

            const glm::mat4 invBind = mat4FromArray16(anim.bones[bi].inverseBind);
            boneMatrices[bi] = globalMatrices[bi] * invBind;
        }

        return boneMatrices;
    }
};

struct AssmanBakedAnimSource
{
    std::string path;
    std::string name;
    std::string var;
};

struct AssmanBakedAnimConfig
{
    float sampleInterval = 0.10f;
    std::vector<AssmanBakedAnimSource> sources;
};

static std::string assman_trim_copy(std::string line)
{
    auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
    while (!line.empty() && isSpace((unsigned char)line.front()))
        line.erase(line.begin());
    while (!line.empty() && isSpace((unsigned char)line.back()))
        line.pop_back();
    return line;
}

static AssmanBakedAnimConfig assman_parse_minigame_anim_cache_cfg(const std::string &path)
{
    std::ifstream in(path);
    if (!in)
        throw std::runtime_error("Could not open config: " + path);

    AssmanBakedAnimConfig cfg;
    std::string line;
    int lineNo = 0;
    while (std::getline(in, line))
    {
        ++lineNo;
        auto hash = line.find('#');
        if (hash != std::string::npos)
            line = line.substr(0, hash);
        line = assman_trim_copy(line);
        if (line.empty())
            continue;

        std::istringstream ss(line);
        std::string cmd;
        ss >> cmd;
        if (cmd == "sample")
        {
            ss >> cfg.sampleInterval;
        }
        else if (cmd == "anim")
        {
            AssmanBakedAnimSource src;
            ss >> src.path >> src.name >> src.var;
            if (src.path.empty() || src.name.empty() || src.var.empty())
                throw std::runtime_error("Invalid `anim <blob> <SetName> <varName>` at " + path + ":" + std::to_string(lineNo));
            cfg.sources.push_back(src);
        }
        else
        {
            throw std::runtime_error("Unknown directive `" + cmd + "` at " + path + ":" + std::to_string(lineNo));
        }
    }

    if (cfg.sources.empty())
        throw std::runtime_error("Config missing at least one `anim <blob> <SetName> <varName>`");
    if (cfg.sampleInterval <= 0.0f)
        throw std::runtime_error("Config sample interval must be positive");
    return cfg;
}

static std::vector<uint8_t> assman_read_binary_file(const std::string &path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
        throw std::runtime_error("failed to open " + path);
    in.seekg(0, std::ios::end);
    const std::streamoff len = in.tellg();
    in.seekg(0, std::ios::beg);
    std::vector<uint8_t> data((size_t)len);
    if (!data.empty())
        in.read(reinterpret_cast<char *>(data.data()), len);
    return data;
}

static std::string assman_anim_clip_name(const AssmanAnimClipHeader *clip)
{
    return clip ? std::string(assman_anim_name_c_str(clip->name)) : std::string();
}

static std::string assman_enum_token(std::string s)
{
    std::string out;
    out.reserve(s.size());
    for (char c : s)
    {
        if (std::isalnum((unsigned char)c))
            out.push_back((char)std::toupper((unsigned char)c));
        else
            out.push_back('_');
    }
    return out.empty() ? "CLIP" : out;
}

static void assman_write_mat4(std::ostream &out, const glm::mat4 &m)
{
    out << "        {";
    for (int c = 0; c < 4; ++c)
    {
        out << " ";
        for (int r = 0; r < 4; ++r)
        {
            out << std::setprecision(9) << m[c][r];
            if (r < 3)
                out << ", ";
        }
        if (c < 3)
            out << ", ";
    }
    out << " },\n";
}

static void assman_write_baked_anim_set(std::ostream &out, const AssmanBakedAnimSource &src, float sampleInterval)
{
    std::vector<uint8_t> data = assman_read_binary_file(src.path);
    AssmanBakeAnimPlayer anim;
    anim.loadFromBlob(data.data(), data.size());

    const uint32_t clipCount = (uint32_t)anim.clipPtrs.size();
    const uint32_t boneCount = anim.anim.header ? anim.anim.header->boneCount : 0;
    std::vector<uint32_t> offsets;
    std::vector<uint32_t> lengths;
    std::vector<float> durations;
    std::vector<std::vector<glm::mat4>> bakedFrames;

    offsets.reserve(clipCount);
    lengths.reserve(clipCount);
    durations.reserve(clipCount);

    uint32_t totalFrames = 0;
    for (uint32_t clip = 0; clip < clipCount; ++clip)
    {
        const auto *ch = reinterpret_cast<const AssmanAnimClipHeader *>(anim.clipPtrs[clip]);
        const float duration = ch ? std::max(0.001f, ch->durationSeconds) : 0.001f;
        const uint32_t frameCount = std::max(1u, (uint32_t)std::ceil(duration / sampleInterval));

        offsets.push_back(totalFrames);
        lengths.push_back(frameCount);
        durations.push_back(duration);
        totalFrames += frameCount;

        for (uint32_t frame = 0; frame < frameCount; ++frame)
        {
            const float t = std::min(duration, frame * sampleInterval);
            anim.activeClip = (int)clip;
            anim.t = t;
            bakedFrames.push_back(anim.evaluate());
        }
    }

    out << "enum MiniGame" << src.name << "BakedClip {\n";
    for (uint32_t clip = 0; clip < clipCount; ++clip)
    {
        const auto *ch = reinterpret_cast<const AssmanAnimClipHeader *>(anim.clipPtrs[clip]);
        out << "    MINIGAME_" << assman_enum_token(src.name) << "_" << assman_enum_token(assman_anim_clip_name(ch))
            << " = " << clip << ",\n";
    }
    out << "};\n\n";

    out << "static const uint32_t " << src.var << "FrameOffset[] = {\n";
    for (uint32_t clip = 0; clip < clipCount; ++clip)
    {
        const auto *ch = reinterpret_cast<const AssmanAnimClipHeader *>(anim.clipPtrs[clip]);
        out << "    " << offsets[clip] << ", // " << assman_anim_clip_name(ch) << "\n";
    }
    out << "};\n\n";

    out << "static const uint32_t " << src.var << "LengthInFrames[] = {\n";
    for (uint32_t clip = 0; clip < clipCount; ++clip)
    {
        const auto *ch = reinterpret_cast<const AssmanAnimClipHeader *>(anim.clipPtrs[clip]);
        out << "    " << lengths[clip] << ", // " << assman_anim_clip_name(ch) << "\n";
    }
    out << "};\n\n";

    out << "static const float " << src.var << "DurationSeconds[] = {\n";
    for (uint32_t clip = 0; clip < clipCount; ++clip)
    {
        const auto *ch = reinterpret_cast<const AssmanAnimClipHeader *>(anim.clipPtrs[clip]);
        out << "    " << std::setprecision(9) << durations[clip] << "f, // " << assman_anim_clip_name(ch) << "\n";
    }
    out << "};\n\n";

    out << "static constexpr uint32_t " << src.var << "ClipCount = " << clipCount << ";\n";
    out << "static constexpr uint32_t " << src.var << "TotalFrameCount = " << totalFrames << ";\n";
    out << "static constexpr uint32_t " << src.var << "BoneCount = " << boneCount << ";\n";
    out << "static const glm::mat4 " << src.var << "Frames[" << src.var << "TotalFrameCount][" << src.var << "BoneCount] = {\n";
    for (uint32_t frame = 0; frame < totalFrames; ++frame)
    {
        out << "    {\n";
        for (uint32_t bone = 0; bone < boneCount; ++bone)
            assman_write_mat4(out, bakedFrames[frame][bone]);
        out << "    }, // " << src.name << " frame " << frame << "\n";
    }
    out << "};\n\n";

    out << "static const MiniGameBakedAnimSet " << src.var << "Set = {\n";
    out << "    " << src.var << "ClipCount,\n";
    out << "    " << src.var << "BoneCount,\n";
    out << "    " << src.var << "TotalFrameCount,\n";
    out << "    " << src.var << "FrameOffset,\n";
    out << "    " << src.var << "LengthInFrames,\n";
    out << "    " << src.var << "DurationSeconds,\n";
    out << "    &" << src.var << "Frames[0][0]\n";
    out << "};\n\n";
}

int cmd_minigame_anim_cache(
    const std::string &cfgPath,
    const std::string &outPath,
    float sampleOverride
)
{
    AssmanBakedAnimConfig cfg = assman_parse_minigame_anim_cache_cfg(cfgPath);
    if (sampleOverride > 0.0f)
        cfg.sampleInterval = sampleOverride;
    const std::vector<AssmanBakedAnimSource> &sources = cfg.sources;
    const float sampleInterval = cfg.sampleInterval;

    if (sources.empty())
        throw std::runtime_error("minigame-anim-cache requires at least one <anim> <Name> <var> triple");
    if (outPath.empty())
        throw std::runtime_error("minigame-anim-cache missing -o <output>");
    if (sampleInterval <= 0.0f)
        throw std::runtime_error("minigame-anim-cache sample interval must be positive");

    std::ofstream out(outPath);
    if (!out)
        throw std::runtime_error("failed to open output " + outPath);

    out << "#pragma once\n";
    out << "// Generated by assman minigame-anim-cache. Do not edit by hand.\n";
    out << "#include <cstdint>\n";
    out << "#include <glm/glm.hpp>\n\n";
    out << "struct MiniGameBakedAnimSet\n";
    out << "{\n";
    out << "    uint32_t clipCount;\n";
    out << "    uint32_t boneCount;\n";
    out << "    uint32_t totalFrameCount;\n";
    out << "    const uint32_t *frameOffset;\n";
    out << "    const uint32_t *lengthInFrames;\n";
    out << "    const float *durationSeconds;\n";
    out << "    const glm::mat4 *frames;\n";
    out << "};\n\n";
    out << "static constexpr float kMiniGameBakedAnimSampleIntervalSeconds = " << std::setprecision(9)
        << sampleInterval << "f;\n\n";

    for (const AssmanBakedAnimSource &src : sources)
        assman_write_baked_anim_set(out, src, sampleInterval);

    return 0;
}
