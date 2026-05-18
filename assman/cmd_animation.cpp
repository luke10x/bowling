#include <cassert>
#include <cctype>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "api/anim_data.h"

static inline void assimpToColMajor16(const aiMatrix4x4& mat, float out16[16])
{
    // Assimp is row-major; our runtime uses column-major floats.
    // out16 is column-major (like GL / glm::value_ptr).
    // Column c, row r => out16[c*4 + r]
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            out16[c * 4 + r] = mat[r][c];
}

struct AssmanAnimConfig
{
    std::string meshName;
    std::vector<std::string> clipNames;
};

static AssmanAnimConfig parse_anim_cfg(const std::string& path)
{
    std::ifstream in(path);
    if (!in)
        throw std::runtime_error("Could not open config: " + path);

    AssmanAnimConfig cfg;
    std::string line;
    while (std::getline(in, line))
    {
        // strip comments
        auto hash = line.find('#');
        if (hash != std::string::npos)
            line = line.substr(0, hash);

        // trim
        auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
        while (!line.empty() && isSpace(line.front())) line.erase(line.begin());
        while (!line.empty() && isSpace(line.back())) line.pop_back();
        if (line.empty())
            continue;

        std::istringstream ss(line);
        std::string cmd;
        ss >> cmd;
        if (cmd == "mesh")
        {
            ss >> cfg.meshName;
        }
        else if (cmd == "clip")
        {
            std::string clip;
            ss >> clip;
            if (!clip.empty())
                cfg.clipNames.push_back(clip);
        }
        else
        {
            throw std::runtime_error("Unknown directive in config: " + cmd);
        }
    }

    if (cfg.meshName.empty())
        throw std::runtime_error("Config missing `mesh <MeshName>`");
    if (cfg.clipNames.empty())
        throw std::runtime_error("Config missing at least one `clip <AnimationName>`");

    return cfg;
}

static aiMesh* find_mesh_by_name(const aiScene* scene, const std::string& meshName)
{
    for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
    {
        aiMesh* m = scene->mMeshes[meshIndex];
        if (m->mName.length > 0 && meshName == m->mName.C_Str())
            return m;
    }
    return nullptr;
}

static const aiAnimation* find_animation_by_name(const aiScene* scene, const std::string& animName)
{
    for (unsigned int i = 0; i < scene->mNumAnimations; ++i)
    {
        const aiAnimation* a = scene->mAnimations[i];
        if (a->mName.length > 0 && animName == a->mName.C_Str())
            return a;
    }
    return nullptr;
}

static void collect_nodes(
    const aiNode* node,
    std::unordered_map<std::string, const aiNode*>& outByName
)
{
    outByName[node->mName.C_Str()] = node;
    for (unsigned int i = 0; i < node->mNumChildren; ++i)
        collect_nodes(node->mChildren[i], outByName);
}

static void write_fixed_name64(char out[64], const std::string& s)
{
    std::memset(out, 0, 64);
    if (!s.empty())
    {
        std::strncpy(out, s.c_str(), 63);
        out[63] = 0;
    }
}

static float safe_ticks_per_second(double tps)
{
    // Assimp uses 0 to indicate "unspecified".
    float v = (tps > 0.0) ? (float)tps : 25.0f;
    if (v < 1.0f)
        v = 25.0f;
    return v;
}

static void write_anim_file(
    const std::string& outPath,
    const std::vector<AssmanAnimBoneInfo>& bones,
    const std::vector<std::string>& clipNames,
    const std::vector<std::vector<uint8_t>>& clipPayloads
)
{
    std::ofstream out(outPath, std::ios::binary);
    if (!out)
        throw std::runtime_error("Could not open output file: " + outPath);

    AssmanAnimHeader hdr = {};
    hdr.magic = ASSMAN_ANIM_MAGIC;
    hdr.version = ASSMAN_ANIM_VERSION;
    hdr.boneCount = (uint32_t)bones.size();
    hdr.clipCount = (uint32_t)clipNames.size();

    out.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
    out.write(reinterpret_cast<const char*>(bones.data()), (std::streamsize)(bones.size() * sizeof(AssmanAnimBoneInfo)));

    for (size_t ci = 0; ci < clipNames.size(); ++ci)
    {
        // clipPayload already contains [clipHeader][tracks][keys...]
        out.write(reinterpret_cast<const char*>(clipPayloads[ci].data()), (std::streamsize)clipPayloads[ci].size());
    }
}

static const aiNodeAnim* find_channel(const aiAnimation* anim, const std::string& nodeName)
{
    if (!anim)
        return nullptr;
    for (unsigned int i = 0; i < anim->mNumChannels; ++i)
    {
        const aiNodeAnim* ch = anim->mChannels[i];
        if (ch->mNodeName.length > 0 && nodeName == ch->mNodeName.C_Str())
            return ch;
    }
    return nullptr;
}

static void append_bytes(std::vector<uint8_t>& out, const void* data, size_t len)
{
    size_t start = out.size();
    out.resize(start + len);
    std::memcpy(out.data() + start, data, len);
}

static void build_clip_payload(
    std::vector<uint8_t>& out,
    const aiScene* scene,
    const aiAnimation* anim,
    const std::string& clipName,
    const std::vector<std::string>& boneNames
)
{
    AssmanAnimClipHeader ch = {};
    write_fixed_name64(ch.name, clipName);
    const float tps = safe_ticks_per_second(anim ? anim->mTicksPerSecond : 0.0);
    ch.ticksPerSecond = tps;
    ch.durationSeconds = anim ? (float)(anim->mDuration / (double)tps) : 0.0f;
    append_bytes(out, &ch, sizeof(ch));

    // Track headers in bone order.
    std::vector<AssmanAnimTrackHeader> tracks;
    tracks.resize(boneNames.size());

    // First pass: compute counts.
    for (size_t bi = 0; bi < boneNames.size(); ++bi)
    {
        const aiNodeAnim* channel = find_channel(anim, boneNames[bi]);
        AssmanAnimTrackHeader th = {};
        if (channel)
        {
            th.posKeyCount = channel->mNumPositionKeys;
            th.rotKeyCount = channel->mNumRotationKeys;
            th.scaleKeyCount = channel->mNumScalingKeys;
        }
        tracks[bi] = th;
    }
    append_bytes(out, tracks.data(), tracks.size() * sizeof(AssmanAnimTrackHeader));

    // Second pass: write keys.
    for (size_t bi = 0; bi < boneNames.size(); ++bi)
    {
        const aiNodeAnim* channel = find_channel(anim, boneNames[bi]);
        const AssmanAnimTrackHeader& th = tracks[bi];
        if (!channel)
        {
            // No keys for this bone.
            continue;
        }

        for (uint32_t i = 0; i < th.posKeyCount; ++i)
        {
            const aiVectorKey& k = channel->mPositionKeys[i];
            AssmanAnimPosKey pk = {};
            pk.t = (float)(k.mTime / (double)tps);
            pk.x = k.mValue.x;
            pk.y = k.mValue.y;
            pk.z = k.mValue.z;
            append_bytes(out, &pk, sizeof(pk));
        }
        for (uint32_t i = 0; i < th.rotKeyCount; ++i)
        {
            const aiQuatKey& k = channel->mRotationKeys[i];
            AssmanAnimRotKey rk = {};
            rk.t = (float)(k.mTime / (double)tps);
            rk.x = k.mValue.x;
            rk.y = k.mValue.y;
            rk.z = k.mValue.z;
            rk.w = k.mValue.w;
            append_bytes(out, &rk, sizeof(rk));
        }
        for (uint32_t i = 0; i < th.scaleKeyCount; ++i)
        {
            const aiVectorKey& k = channel->mScalingKeys[i];
            AssmanAnimScaleKey sk = {};
            sk.t = (float)(k.mTime / (double)tps);
            sk.x = k.mValue.x;
            sk.y = k.mValue.y;
            sk.z = k.mValue.z;
            append_bytes(out, &sk, sizeof(sk));
        }
    }
}

int cmd_animation(std::string inputPath, std::string cfgPath, std::string outPath)
{
    AssmanAnimConfig cfg = parse_anim_cfg(cfgPath);

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        inputPath.c_str(),
        aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace
    );

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cerr << "Error loading model: " << importer.GetErrorString() << std::endl;
        return 1;
    }

    aiMesh* mesh = find_mesh_by_name(scene, cfg.meshName);
    if (!mesh)
    {
        std::cerr << "animation: mesh not found: " << cfg.meshName << std::endl;
        std::cerr << "animation: file contains meshes:" << std::endl;
        for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
        {
            const aiMesh* m = scene->mMeshes[i];
            if (m->mName.length > 0)
                std::cerr << "  - " << m->mName.C_Str() << std::endl;
        }
        return 1;
    }

    // Node lookup for bind pose and hierarchy.
    std::unordered_map<std::string, const aiNode*> nodesByName;
    nodesByName.reserve(1024);
    collect_nodes(scene->mRootNode, nodesByName);

    // Bone list follows mesh->mBones order (MUST match cmd_mesh indexing).
    const uint32_t boneCount = mesh->mNumBones;
    if (boneCount == 0)
    {
        std::cerr << "animation: mesh has 0 bones: " << cfg.meshName << std::endl;
        return 1;
    }

    std::vector<std::string> boneNames;
    boneNames.reserve(boneCount);
    std::unordered_map<std::string, uint32_t> boneNameToIndex;
    boneNameToIndex.reserve(boneCount * 2);

    for (uint32_t bi = 0; bi < boneCount; ++bi)
    {
        aiBone* b = mesh->mBones[bi];
        std::string name = b->mName.C_Str();
        boneNames.push_back(name);
        boneNameToIndex[name] = bi;
    }

    std::vector<AssmanAnimBoneInfo> bones;
    bones.resize(boneCount);
    for (uint32_t bi = 0; bi < boneCount; ++bi)
    {
        aiBone* b = mesh->mBones[bi];
        std::string name = b->mName.C_Str();

        AssmanAnimBoneInfo info = {};
        write_fixed_name64(info.name, name);

        // Parent index: climb node parents until you find an ancestor that is also a bone.
        int32_t parentIndex = ASSMAN_ANIM_NO_PARENT;
        float bindLocal16[16] = {
            1,0,0,0,
            0,1,0,0,
            0,0,1,0,
            0,0,0,1
        };
        auto itNode = nodesByName.find(name);
        if (itNode != nodesByName.end())
        {
            const aiNode* node = itNode->second;
            assimpToColMajor16(node->mTransformation, bindLocal16);

            const aiNode* p = node->mParent;
            while (p)
            {
                auto itBone = boneNameToIndex.find(p->mName.C_Str());
                if (itBone != boneNameToIndex.end())
                {
                    parentIndex = (int32_t)itBone->second;
                    break;
                }
                p = p->mParent;
            }
        }
        info.parentIndex = parentIndex;

        assimpToColMajor16(b->mOffsetMatrix, info.inverseBind);
        std::memcpy(info.bindLocal, bindLocal16, sizeof(float) * 16);

        bones[bi] = info;
    }

    // Clips
    std::vector<std::vector<uint8_t>> clipPayloads;
    clipPayloads.resize(cfg.clipNames.size());

    for (size_t ci = 0; ci < cfg.clipNames.size(); ++ci)
    {
        const std::string& clipName = cfg.clipNames[ci];
        const aiAnimation* anim = find_animation_by_name(scene, clipName);
        if (!anim)
        {
            std::cerr << "animation: clip not found in GLB: " << clipName << std::endl;
            std::cerr << "animation: file contains animations:" << std::endl;
            for (unsigned int i = 0; i < scene->mNumAnimations; ++i)
            {
                const aiAnimation* a = scene->mAnimations[i];
                if (a->mName.length > 0)
                    std::cerr << "  - " << a->mName.C_Str() << std::endl;
            }
            return 1;
        }
        build_clip_payload(clipPayloads[ci], scene, anim, clipName, boneNames);
    }

    write_anim_file(outPath, bones, cfg.clipNames, clipPayloads);
    std::cout << "Wrote animation: bones=" << boneCount << " clips=" << cfg.clipNames.size() << " -> " << outPath << "\n";
    return 0;
}
