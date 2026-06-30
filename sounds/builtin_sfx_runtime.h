#pragma once

#include <array>
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <sstream>
#include <string>

#include "./../../eggsfm/xfm_api.h"
#include "builtin_sfx_registry.h"
#include "../tracker/tracker_song_io.h"

struct BuiltinSfxPrepared
{
    const BuiltinSfxDefinition *def = nullptr;
    std::array<int, 256> localToGlobal {};
    std::string remappedPattern;
};

const BuiltinSfxPrepared *BuiltinSfx_PreparedByIndex(int index);
const BuiltinSfxPrepared *BuiltinSfx_PreparedById(int sfxId);
int BuiltinSfx_GlobalInstrumentCount();
void BuiltinSfx_ApplyInstrumentBank(xfm_module *module);

#ifdef BUILTIN_SFX_RUNTIME_IMPLEMENTATION

static std::array<BuiltinSfxPrepared, BUILTIN_SFX_REGISTRY_COUNT> g_builtinSfxPrepared = {};
static int g_builtinSfxInstrumentCount = 0;
static bool g_builtinSfxReady = false;

struct BuiltinSfxInstrumentBank
{
    xfm_patch_opn patches[256] = {};
    bool patchValid[256] = {};
    XfmMacro macros[256][XFM_MACRO_TARGET_COUNT] = {};
    bool macroEnabled[256][XFM_MACRO_TARGET_COUNT] = {};
    bool macroValid[256][XFM_MACRO_TARGET_COUNT] = {};
};

static BuiltinSfxInstrumentBank g_builtinSfxInstrumentBank = {};
static BuiltinSfxInstrumentBank g_builtinSfxScratchBank = {};

static xfm_patch_opn BuiltinSfx_DefaultPatch()
{
    xfm_patch_opn patch = {};
    patch.ALG = 0;
    patch.FB = 0;
    patch.AMS = 0;
    patch.FMS = 0;
    for (int op = 0; op < 4; op++)
    {
        patch.op[op].DT = 0;
        patch.op[op].MUL = 1;
        patch.op[op].TL = op == 3 ? 0 : 48;
        patch.op[op].RS = 0;
        patch.op[op].AR = 31;
        patch.op[op].AM = 0;
        patch.op[op].DR = 8;
        patch.op[op].SR = 0;
        patch.op[op].SL = 15;
        patch.op[op].RR = 8;
        patch.op[op].SSG = 0;
    }
    return patch;
}

static int BuiltinSfx_MacroMaxTarget()
{
    return XFM_MACRO_TARGET_COUNT - 1;
}

static void BuiltinSfx_DefaultMacro(XfmMacro *macro, int target)
{
    if (!macro) return;
    *macro = {};
    macro->target = (uint8_t)std::max((int)XFM_MACRO_TL1, std::min(BuiltinSfx_MacroMaxTarget(), target));
    macro->length = 0;
    macro->loop_start = 0;
    macro->release_start = 0xFF;
    macro->has_loop = false;
    int16_t value = 0;
    if (macro->target >= XFM_MACRO_MUL1 && macro->target <= XFM_MACRO_MUL4)
        value = 1;
    else if (macro->target == XFM_MACRO_PAN)
        value = 3;
    for (int i = 0; i < XFM_MAX_MACRO_VALUES; i++)
        macro->values[i] = value;
}

static bool BuiltinSfx_MacroTargetSupportsRelease(int target)
{
    return !((target >= XFM_MACRO_AR1 && target <= XFM_MACRO_RR4) ||
             (target >= XFM_MACRO_SSG1 && target <= XFM_MACRO_SSG4));
}

static void BuiltinSfx_NormalizeMacroUiState(XfmMacro *macro)
{
    if (!macro) return;
    macro->length = (uint8_t)std::max(0, std::min(32, (int)macro->length));
    if (macro->length == 0)
    {
        macro->has_loop = false;
        macro->loop_start = 0;
        macro->release_start = 0xFF;
        return;
    }
    if (macro->has_loop && macro->loop_start >= macro->length)
    {
        macro->has_loop = false;
        macro->loop_start = 0;
    }
    if (macro->release_start != 0xFF)
    {
        if (macro->release_start >= macro->length)
            macro->release_start = 0xFF;
        else if (macro->has_loop && macro->release_start <= macro->loop_start)
            macro->release_start = (macro->loop_start + 1 < macro->length) ? (uint8_t)(macro->loop_start + 1) : 0xFF;
    }
    if (!BuiltinSfx_MacroTargetSupportsRelease(macro->target))
        macro->release_start = 0xFF;
}

static void BuiltinSfx_ClearInstrumentBank(BuiltinSfxInstrumentBank *bank)
{
    if (!bank) return;
    *bank = {};
}

static void BuiltinSfx_LoadInstrumentText(BuiltinSfxInstrumentBank *bank, const std::string &text)
{
    if (!bank || text.empty()) return;
    std::istringstream in(text);
    std::string tag;
    int inst = -1;
    while (in >> tag)
    {
        if (tag == "INST")
        {
            std::string hex;
            in >> hex;
            inst = (int)std::strtol(hex.c_str(), nullptr, 16);
            if (inst >= 0 && inst < 256)
            {
                bank->patches[inst] = BuiltinSfx_DefaultPatch();
                bank->patchValid[inst] = true;
            }
        }
        else if (tag == "PATCH" && inst >= 0 && inst < 256)
        {
            int alg, fb, ams, fms;
            in >> alg >> fb >> ams >> fms;
            bank->patches[inst].ALG = (uint8_t)std::max(0, std::min(7, alg));
            bank->patches[inst].FB = (uint8_t)std::max(0, std::min(7, fb));
            bank->patches[inst].AMS = (uint8_t)std::max(0, std::min(3, ams));
            bank->patches[inst].FMS = (uint8_t)std::max(0, std::min(7, fms));
        }
        else if (tag == "OP" && inst >= 0 && inst < 256)
        {
            int op, dt, mul, tl, rs, ar, am, dr, sr, sl, rr, ssg;
            in >> op >> dt >> mul >> tl >> rs >> ar >> am >> dr >> sr >> sl >> rr >> ssg;
            if (op >= 1 && op <= 4)
            {
                xfm_patch_opn_operator &o = bank->patches[inst].op[op - 1];
                o.DT = (int8_t)std::max(-3, std::min(3, dt));
                o.MUL = (uint8_t)std::max(0, std::min(15, mul));
                o.TL = (uint8_t)std::max(0, std::min(127, tl));
                o.RS = (uint8_t)std::max(0, std::min(3, rs));
                o.AR = (uint8_t)std::max(0, std::min(31, ar));
                o.AM = (uint8_t)std::max(0, std::min(1, am));
                o.DR = (uint8_t)std::max(0, std::min(31, dr));
                o.SR = (uint8_t)std::max(0, std::min(31, sr));
                o.SL = (uint8_t)std::max(0, std::min(15, sl));
                o.RR = (uint8_t)std::max(0, std::min(15, rr));
                o.SSG = (uint8_t)std::max(0, std::min(8, ssg));
            }
        }
        else if (tag == "MACRO" && inst >= 0 && inst < 256)
        {
            int target, length, loopStart, releaseStart;
            in >> target >> length >> loopStart >> releaseStart;
            if (target >= XFM_MACRO_TL1 && target < XFM_MACRO_TARGET_COUNT)
            {
                XfmMacro &macro = bank->macros[inst][target];
                BuiltinSfx_DefaultMacro(&macro, target);
                macro.length = (uint8_t)std::max(0, std::min(32, length));
                macro.has_loop = macro.length > 0 && loopStart >= 0 && loopStart < macro.length && loopStart != 255;
                macro.loop_start = macro.has_loop ? (uint8_t)loopStart : 0;
                macro.release_start = (releaseStart == 255 || macro.length == 0) ? 0xFF : (uint8_t)std::max(0, std::min((int)macro.length - 1, releaseStart));
                for (int i = 0; i < macro.length; i++)
                {
                    int v = 0;
                    in >> v;
                    macro.values[i] = (int16_t)v;
                }
                BuiltinSfx_NormalizeMacroUiState(&macro);
                bank->macroEnabled[inst][target] = true;
                bank->macroValid[inst][target] = true;
            }
        }
    }
}

static std::string BuiltinSfx_RemapPatternInstrumentIds(
    const char *pattern,
    const std::array<int, 256> &localToGlobal)
{
    if (!pattern)
        return {};
    std::string out(pattern);
    auto hex = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return 10 + c - 'A';
        if (c >= 'a' && c <= 'f') return 10 + c - 'a';
        return -1;
    };
    auto hexDigit = [](int v) -> char {
        v &= 15;
        return (char)(v < 10 ? ('0' + v) : ('A' + (v - 10)));
    };

    size_t i = 0;
    while (i < out.size() && (out[i] == ' ' || out[i] == '\t' || out[i] == '\n' || out[i] == '\r')) i++;
    while (i < out.size() && out[i] >= '0' && out[i] <= '9') i++;
    while (i < out.size() && out[i] != '\n') i++;
    if (i < out.size() && out[i] == '\n') i++;

    int columnPos = 0;
    for (; i < out.size(); i++)
    {
        char c = out[i];
        if (c == '\n' || c == '\r' || c == '|')
        {
            columnPos = 0;
            continue;
        }
        if (columnPos == 3 && i + 1 < out.size())
        {
            int hi = hex(out[i]);
            int lo = hex(out[i + 1]);
            if (hi >= 0 && lo >= 0)
            {
                int localInst = (hi << 4) | lo;
                int globalInst = localToGlobal[(uint8_t)localInst];
                if (globalInst >= 0 && globalInst <= 255)
                {
                    out[i] = hexDigit(globalInst >> 4);
                    out[i + 1] = hexDigit(globalInst);
                }
            }
        }
        columnPos++;
    }
    return out;
}

static void BuiltinSfx_CopyInstrumentToBank(BuiltinSfxInstrumentBank *dst, const BuiltinSfxInstrumentBank *src, int srcInst, int dstInst)
{
    if (!dst || !src || srcInst < 0 || srcInst > 255 || dstInst < 0 || dstInst > 255)
        return;
    dst->patches[dstInst] = src->patchValid[srcInst] ? src->patches[srcInst] : BuiltinSfx_DefaultPatch();
    dst->patchValid[dstInst] = src->patchValid[srcInst];
    for (int target = XFM_MACRO_TL1; target < XFM_MACRO_TARGET_COUNT; ++target)
    {
        dst->macros[dstInst][target] = src->macros[srcInst][target];
        dst->macroEnabled[dstInst][target] = src->macroEnabled[srcInst][target];
        dst->macroValid[dstInst][target] = src->macroValid[srcInst][target];
    }
}

static void BuiltinSfx_EnsurePrepared()
{
    if (g_builtinSfxReady)
        return;

    BuiltinSfx_ClearInstrumentBank(&g_builtinSfxInstrumentBank);
    g_builtinSfxInstrumentCount = 0;

    for (int i = 0; i < BUILTIN_SFX_REGISTRY_COUNT; ++i)
    {
        const BuiltinSfxDefinition &def = BUILTIN_SFX_REGISTRY[i];
        BuiltinSfxPrepared &prepared = g_builtinSfxPrepared[i];
        prepared.def = &def;
        prepared.localToGlobal.fill(-1);

        BuiltinSfx_ClearInstrumentBank(&g_builtinSfxScratchBank);
        BuiltinSfx_LoadInstrumentText(&g_builtinSfxScratchBank, def.instruments ? def.instruments : "");

        bool referenced[256] = {};
        TrackerSongIO_MarkReferencedInstruments(def.pattern ? def.pattern : "", referenced);
        for (int inst = 0; inst < 256; ++inst)
        {
            if (!referenced[inst])
                continue;
            if (g_builtinSfxInstrumentCount >= 256)
                break;
            const int globalInst = g_builtinSfxInstrumentCount++;
            prepared.localToGlobal[inst] = globalInst;
            BuiltinSfx_CopyInstrumentToBank(&g_builtinSfxInstrumentBank, &g_builtinSfxScratchBank, inst, globalInst);
        }

        prepared.remappedPattern = BuiltinSfx_RemapPatternInstrumentIds(def.pattern, prepared.localToGlobal);
    }

    g_builtinSfxReady = true;
}

const BuiltinSfxPrepared *BuiltinSfx_PreparedByIndex(int index)
{
    BuiltinSfx_EnsurePrepared();
    if (index < 0 || index >= BUILTIN_SFX_REGISTRY_COUNT)
        return nullptr;
    return &g_builtinSfxPrepared[index];
}

const BuiltinSfxPrepared *BuiltinSfx_PreparedById(int sfxId)
{
    BuiltinSfx_EnsurePrepared();
    for (int i = 0; i < BUILTIN_SFX_REGISTRY_COUNT; ++i)
        if (g_builtinSfxPrepared[i].def && g_builtinSfxPrepared[i].def->sfxId == sfxId)
            return &g_builtinSfxPrepared[i];
    return nullptr;
}

int BuiltinSfx_GlobalInstrumentCount()
{
    BuiltinSfx_EnsurePrepared();
    return g_builtinSfxInstrumentCount;
}

void BuiltinSfx_ApplyInstrumentBank(xfm_module *module)
{
    if (!module)
        return;
    BuiltinSfx_EnsurePrepared();
    int nextMacroId = 0;
    for (int inst = 0; inst < g_builtinSfxInstrumentCount; ++inst)
    {
        if (!g_builtinSfxInstrumentBank.patchValid[inst])
            continue;
        xfm_patch_set(module, inst, &g_builtinSfxInstrumentBank.patches[inst], sizeof(xfm_patch_opn), XFM_CHIP_YM3438);
        xfm_patch_macro_clear(module, inst, XFM_MACRO_NONE);
        for (int target = XFM_MACRO_TL1; target < XFM_MACRO_TARGET_COUNT; ++target)
        {
            if (!g_builtinSfxInstrumentBank.macroEnabled[inst][target] || !g_builtinSfxInstrumentBank.macroValid[inst][target])
                continue;
            if (nextMacroId >= XFM_MAX_MACROS)
                break;
            XfmMacro macro = g_builtinSfxInstrumentBank.macros[inst][target];
            macro.target = (uint8_t)target;
            BuiltinSfx_NormalizeMacroUiState(&macro);
            if (macro.length == 0)
                continue;
            if (xfm_macro_set(module, nextMacroId, &macro) >= 0)
            {
                xfm_patch_macro_set(module, inst, (uint8_t)target, nextMacroId);
                nextMacroId++;
            }
        }
    }
}

#endif
