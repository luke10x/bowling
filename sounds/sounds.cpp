#include <SDL.h>

#include "./../../eggsfm/xfm_api.h"

#include <cstdio>
#include <cstring>
#include <atomic>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>

// #include <clay.h>
// #include "../clayton/clayton_click.h"
// #include "../clayton/claytheme.h"

#include "./sounds.h"
#define BUILTIN_SFX_RUNTIME_IMPLEMENTATION
#include "./builtin_sfx_runtime.h"
#include "../tracker/tracker_song_io.h"

// Forward declaration to break circular dependency with sounds.h
// struct GameSoundSystem;

static inline int soundCoerceVisibleSongIndex(const GameSoundSystem *self, int songIndex)
{
    const int count = self ? std::max(1, self->visibleSongCount()) : TRACKER_BUILTIN_SONG_COUNT;
    if (songIndex < 1 || songIndex > count)
        return 1;
    return songIndex;
}

struct BallRollingPatchAutomation
{
    static void applyRecipe(
        xfm_patch_opn &rollPatch,
        float rollingSurfaceSpeedMps,
        float ballZ,
        float slippery01,
        bool sliding,
        float angularSpeedSigned,
        float ballMassKg,
        bool isEnemyTurn)
    {
        // ===== ROLLING PATCH AUTOMATION RECIPE =====
        // This is the intended tuning surface for the rolling sound: each call
        // maps one world property to one YM2612 patch parameter. Comment out any
        // apply_xfm_patch_auto call below to disable just that automation.
        apply_xfm_patch_auto(XfmPatchAutoCfg{
            .patch = &rollPatch,
            .input = rollingSurfaceSpeedMps,
            .inputFrom = 0.5f,
            .inputTo = 6.0f,
            .clamp = true,
            .param = XFM_OPN_AUTO_OP4_DR,
            .paramFrom = 5,
            .paramTo = 25,
        });
        apply_xfm_patch_auto(XfmPatchAutoCfg{
            .patch = &rollPatch,
            .input = ballMassKg,
            .inputFrom = 3.0f,
            .inputTo = 7.0f,
            .clamp = true,
            .param = XFM_OPN_AUTO_OP1_TL,
            .paramFrom = 35,
            .paramTo = 15,
        });
        apply_xfm_patch_auto(XfmPatchAutoCfg{
            .patch = &rollPatch,
            .input = std::abs(angularSpeedSigned),
            .inputFrom = 1.0f,
            .inputTo = 6.0f,
            .clamp = true,
            .param = XFM_OPN_AUTO_OP2_TL,
            .paramFrom = 55,
            .paramTo = 25,
        });
        const float combinedFriction01 = slippery01; // 0=dry/friction, 1=oiled/slippery.
        const float frictionNearOurEnd01 = std::clamp((0.0f - ballZ) / (0.0f - -18.3f), 0.0f, 1.0f);
        const int frictionAudibleTl = static_cast<int>(std::lround(85.0f + (10.0f - 85.0f) * frictionNearOurEnd01));
        apply_xfm_patch_auto(XfmPatchAutoCfg{
            .patch = &rollPatch,
            .input = combinedFriction01,
            .inputFrom = 0.0f,
            .inputTo = 1.0f,
            .clamp = true,
            .param = XFM_OPN_AUTO_OP3_TL,
            .paramFrom = frictionAudibleTl,
            .paramTo = 85,
        });

        // Spin slows fade from each throw's own start side; raw ballZ would bias
        // player throws quiet and enemy throws loud because their Z travel is reversed.
        const float zFadeStart = isEnemyTurn ? 0.0f : -18.0f;
        const float zFadeEnd = isEnemyTurn ? -18.0f : 0.0f;
        float zFadeInput = ballZ;
        const float angularSpeedMagnitude = std::abs(angularSpeedSigned);
        if (angularSpeedMagnitude > 1.0f) {
            zFadeInput = zFadeStart + (ballZ - zFadeStart) / angularSpeedMagnitude;
        }
        apply_xfm_patch_auto(XfmPatchAutoCfg{
            .patch = &rollPatch,
            .input = zFadeInput,
            .inputFrom = zFadeStart,
            .inputTo = zFadeEnd,
            .clamp = true,
            .param = XFM_OPN_AUTO_OP4_TL,
            .paramFrom = 4,
            .paramTo = 15,
        });
        apply_xfm_patch_auto(XfmPatchAutoCfg{
            .patch = &rollPatch,
            .input = sliding ? 1.0f : 0.0f,
            .inputFrom = 0.0f,
            .inputTo = 1.0f,
            .clamp = true,
            .param = XFM_OPN_AUTO_OP4_SSG,
            .paramFrom = 3,
            .paramTo = 0,
        });
        // ===== END ROLLING PATCH AUTOMATION RECIPE =====
    }
};

static void BallRolling_ResetAutomationCache(GameSoundSystem *sound)
{
    if (!sound) return;
    sound->lastBallRollingOp1Mul = -1;
    sound->lastBallRollingOp1Tl = -1;
    sound->lastBallRollingOp2Mul = -1;
    sound->lastBallRollingOp2Tl = -1;
    sound->lastBallRollingOp2Dr = -1;
    sound->lastBallRollingOp3Tl = -1;
    sound->lastBallRollingOp4Dr = -1;
    sound->lastBallRollingOp4Tl = -1;
    sound->lastBallRollingOp4Ssg = -1;
    sound->lastBallRollingFb = -1;
}

struct SoundTrackerInstrumentBank
{
    xfm_patch_opn patches[256] = {};
    bool patchValid[256] = {};
    XfmMacro macros[256][XFM_MACRO_TARGET_COUNT] = {};
    bool macroEnabled[256][XFM_MACRO_TARGET_COUNT] = {};
    bool macroValid[256][XFM_MACRO_TARGET_COUNT] = {};
};

static xfm_patch_opn soundDefaultPatch()
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

static void soundDefaultMacro(XfmMacro *macro, int target)
{
    if (!macro) return;
    *macro = {};
    macro->target = (uint8_t)std::max((int)XFM_MACRO_TL1, std::min(XFM_MACRO_TARGET_COUNT - 1, target));
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

static bool soundMacroTargetSupportsRelease(int target)
{
    return !((target >= XFM_MACRO_AR1 && target <= XFM_MACRO_RR4) ||
             (target >= XFM_MACRO_SSG1 && target <= XFM_MACRO_SSG4));
}

static void soundNormalizeMacro(XfmMacro *macro)
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
    if (!soundMacroTargetSupportsRelease(macro->target))
        macro->release_start = 0xFF;
}

static void soundParseInstrumentDsl(SoundTrackerInstrumentBank *bank, const char *text)
{
    if (!bank || !text || !text[0]) return;
    *bank = {};
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
                bank->patches[inst] = soundDefaultPatch();
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
        else if (tag == "FM" && inst >= 0 && inst < 256)
        {
            std::string maybeHeaderOrOp;
            in >> maybeHeaderOrOp;
            if (maybeHeaderOrOp == "OP")
                continue;
            int op = (int)std::strtol(maybeHeaderOrOp.c_str(), nullptr, 10);
            int tl, ar, dr, sl, sr, rr, ssg, mul, dt, rs, am;
            in >> tl >> ar >> dr >> sl >> sr >> rr >> ssg >> mul >> dt >> rs >> am;
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
                soundDefaultMacro(&macro, target);
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
                soundNormalizeMacro(&macro);
                bank->macroEnabled[inst][target] = true;
                bank->macroValid[inst][target] = true;
            }
        }
    }
}

static void soundApplySongInstrumentBankToMusicModule(
    xfm_module *musicModule,
    const char *songPattern,
    const char *instrumentsText)
{
    if (!musicModule || !songPattern || !songPattern[0] || !instrumentsText || !instrumentsText[0])
        return;

    SoundTrackerInstrumentBank bank = {};
    soundParseInstrumentDsl(&bank, instrumentsText);

    bool referenced[256] = {};
    TrackerSongIO_MarkReferencedInstruments(songPattern, referenced);

    int nextMacroId = 0;
    for (int inst = 0; inst < 256; ++inst)
    {
        if (!referenced[inst])
            continue;
        if (!bank.patchValid[inst])
            continue;
        xfm_patch_set(musicModule, inst, &bank.patches[inst], sizeof(xfm_patch_opn), XFM_CHIP_YM3438);
        xfm_patch_macro_clear(musicModule, inst, XFM_MACRO_NONE);
        for (int target = XFM_MACRO_TL1; target < XFM_MACRO_TARGET_COUNT; ++target)
        {
            if (!bank.macroEnabled[inst][target] || !bank.macroValid[inst][target])
                continue;
            if (nextMacroId >= XFM_MAX_MACROS)
                break;
            XfmMacro macro = bank.macros[inst][target];
            macro.target = (uint8_t)target;
            soundNormalizeMacro(&macro);
            if (macro.length == 0)
                continue;
            if (xfm_macro_set(musicModule, nextMacroId, &macro) >= 0)
            {
                xfm_patch_macro_set(musicModule, inst, (uint8_t)target, nextMacroId);
                nextMacroId++;
            }
        }
    }
}

static void soundApplySongInstrumentBankToMusicModule(GameSoundSystem *self, int songIndex)
{
    if (!self || !self->musicModule)
        return;
    const char *songPattern = self->getSongPlaybackPattern(songIndex);
    const char *instrumentsText = self->getSongInstruments(songIndex);
    soundApplySongInstrumentBankToMusicModule(self->musicModule, songPattern, instrumentsText);
}

static void soundDeclareSongOnMusicModule(GameSoundSystem *self, xfm_module *module, int songIndex)
{
    if (!self || !module)
        return;

    songIndex = soundCoerceVisibleSongIndex(self, songIndex);
    const char *songPattern = self->getSongPlaybackPattern(songIndex);
    if (!songPattern)
        return;

    soundApplySongInstrumentBankToMusicModule(
        module,
        self->getSongPattern(songIndex),
        self->getSongInstruments(songIndex)
    );
    xfm_module_set_lfo(module, self->getSongLfoEnabled(songIndex), self->getSongLfoFrequency(songIndex));
    xfm_module_set_tuning(
        module,
        (xfm_tuning_mode)self->getSongTuningMode(songIndex),
        self->getSongScaleRoot(songIndex));
    xfm_song_declare(
        module,
        songIndex,
        songPattern,
        std::max(1, self->getSongTickRate(songIndex)),
        std::max(1, self->getSongSpeed(songIndex)));
}

static void soundSilenceSongModule(xfm_module *module)
{
    if (!module)
        return;
    module->active_song.active = false;
    for (int ch = 0; ch < 6; ch++)
    {
        if (module->chip)
            module->chip->key_off(ch);
        module->channel_active[ch] = false;
    }
}

static inline void soundOscilloscopeChooseOpnFnumBlock(double hz, int *outFnum, int *outBlock)
{
    int bestFnum = 0;
    int bestBlock = 0;
    double bestErr = 1.0e30;
    if (hz > 0.0)
    {
        for (int block = 0; block <= 7; block++)
        {
            double fnumD = hz * std::pow(2.0, 20 - block) / 144.0;
            int fnum = std::max(1, std::min(0x7ff, (int)std::round(fnumD)));
            double mapped = ((double)fnum * 144.0) / std::pow(2.0, 20 - block);
            double err = std::abs(mapped - hz);
            if (err < bestErr)
            {
                bestErr = err;
                bestFnum = fnum;
                bestBlock = block;
            }
        }
    }
    *outFnum = bestFnum;
    *outBlock = bestBlock;
}

static inline void soundCaptureOscilloscope(GameSoundSystem *self, int16_t *channelOut[TRACKER_OSC_CHANNELS], int frames)
{
    if (!self || !channelOut || frames <= 0) return;
    xfm_module *m = self->musicModule;
    uint64_t cursor = self->oscilloscopeSampleCursor.load(std::memory_order_relaxed);
    uint32_t write = self->oscilloscopeWriteIndex.load(std::memory_order_relaxed);
    int peak[TRACKER_OSC_CHANNELS] = {};
    for (int ch = 0; ch < TRACKER_OSC_CHANNELS; ch++)
    {
        if (!channelOut[ch]) continue;
        for (int i = 0; i < frames; i++)
        {
            int left = channelOut[ch][i * 2];
            int right = channelOut[ch][i * 2 + 1];
            peak[ch] = std::max(peak[ch], std::max(std::abs(left), std::abs(right)));
        }
    }

    for (int ch = 0; ch < TRACKER_OSC_CHANNELS; ch++)
    {
        bool hasFrequency = m && m->active_song.active && m->active_song.channels[ch].current_hz > 0.0;
        bool keyOn = hasFrequency || peak[ch] > 8;
        int fnum = self->oscilloscopeFnum[ch].load(std::memory_order_relaxed);
        int block = self->oscilloscopeBlock[ch].load(std::memory_order_relaxed);
        if (hasFrequency)
            soundOscilloscopeChooseOpnFnumBlock(m->active_song.channels[ch].current_hz, &fnum, &block);

        int oldFnum = self->oscilloscopeFnum[ch].load(std::memory_order_relaxed);
        int oldBlock = self->oscilloscopeBlock[ch].load(std::memory_order_relaxed);
        bool oldKeyOn = self->oscilloscopeKeyOn[ch].load(std::memory_order_relaxed);
        if (keyOn && (!oldKeyOn || oldFnum != fnum || oldBlock != block))
            self->oscilloscopeNoteStartSample[ch].store(cursor, std::memory_order_relaxed);

        if (fnum > 0)
        {
            self->oscilloscopeFnum[ch].store(fnum, std::memory_order_relaxed);
            self->oscilloscopeBlock[ch].store(block, std::memory_order_relaxed);
        }
        self->oscilloscopeKeyOn[ch].store(keyOn, std::memory_order_relaxed);
    }

    for (int i = 0; i < frames; i++)
    {
        for (int ch = 0; ch < TRACKER_OSC_CHANNELS; ch++)
        {
            int16_t v = 0;
            if (channelOut[ch])
            {
                int16_t left = channelOut[ch][i * 2];
                int16_t right = channelOut[ch][i * 2 + 1];
                v = std::abs((int)left) >= std::abs((int)right) ? left : right;
                if (m && m->volume < 1.0f)
                    v = (int16_t)((float)v * m->volume);
            }
            self->oscilloscopeRing[ch][write] = v;
        }
        write = (write + 1) & (TRACKER_OSC_RING_SIZE - 1);
        cursor++;
    }

    self->oscilloscopeWriteIndex.store(write, std::memory_order_release);
    self->oscilloscopeSampleCursor.store(cursor, std::memory_order_release);
}

// -----------------------------------------------------------------------------
// Sound Settings Panel - Clay UI for audio configuration
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
// Function declarations (implementations in sounds.h after GameSoundSystem is defined)
// -----------------------------------------------------------------------------



/* clang-format off */
// Patches are now defined in sounds/songs_data.h
/* clang-format on */

bool GameSoundSystem::isRestartAllowed() const {
    if (restartState != RestartState::RESTART_IDLE && 
        restartState != RestartState::RESTART_COMPLETE) {
        return false;  // Restart in progress
    }
    // Check grace period
    if (shutdownCompleteTime > 0) {
        uint32_t elapsed = SDL_GetTicks64() - shutdownCompleteTime;
        if (elapsed < GRACE_PERIOD_MS) {
            return false;  // Still in grace period
        }
    }
    return true;
}

const char* GameSoundSystem::getSongPattern(int songIndex) const
{
    const BuiltinSongDefinition *song = BuiltinSong_BySongId(songIndex);
    if (song)
    {
        const BuiltinSongOverride &override = builtinSongOverrides[songIndex - 1];
        if (override.active && !override.uiPattern.empty())
            return override.uiPattern.c_str();
        return song->pattern;
    }
    if (songIndex == TRACKER_USER_SONG_SLOT)
        return userSongVisible && userSongUiPattern[0] ? userSongUiPattern : BUILTIN_SONG_REGISTRY[0].pattern;
    return BUILTIN_SONG_REGISTRY[0].pattern;
}

const char* GameSoundSystem::getSongPlaybackPattern(int songIndex) const
{
    const BuiltinSongDefinition *song = BuiltinSong_BySongId(songIndex);
    if (song)
    {
        const BuiltinSongOverride &override = builtinSongOverrides[songIndex - 1];
        if (override.active && !override.playbackPattern.empty())
            return override.playbackPattern.c_str();
        return builtinSongPlaybackPatterns[songIndex - 1].empty()
            ? song->pattern
            : builtinSongPlaybackPatterns[songIndex - 1].c_str();
    }
    if (songIndex == TRACKER_USER_SONG_SLOT)
        return userSongVisible && userSongPattern[0] ? userSongPattern : BUILTIN_SONG_REGISTRY[0].pattern;
    return BUILTIN_SONG_REGISTRY[0].pattern;
}

void GameSoundSystem::setBuiltinSongPlaybackPattern(int songIndex, const char *pattern)
{
    const BuiltinSongDefinition *song = BuiltinSong_BySongId(songIndex);
    if (!song)
        return;
    builtinSongPlaybackPatterns[songIndex - 1] = pattern ? pattern : "";
    builtinSongPlaybackPatternsReady = true;
}

const char* GameSoundSystem::getSongName(int songIndex) const
{
    const BuiltinSongDefinition *song = BuiltinSong_BySongId(songIndex);
    if (song)
    {
        const BuiltinSongOverride &override = builtinSongOverrides[songIndex - 1];
        if (override.active && !override.displayName.empty())
            return override.displayName.c_str();
        return song->displayName;
    }
    if (songIndex == TRACKER_USER_SONG_SLOT)
        return userSongVisible ? userSongName : "Song 000000";
    return BUILTIN_SONG_REGISTRY[0].displayName;
}

const char* GameSoundSystem::getSongInstruments(int songIndex) const
{
    const BuiltinSongDefinition *song = BuiltinSong_BySongId(songIndex);
    if (song)
    {
        const BuiltinSongOverride &override = builtinSongOverrides[songIndex - 1];
        if (override.active)
            return override.instrumentsText.c_str();
        return song->instruments;
    }
    if (songIndex == TRACKER_USER_SONG_SLOT)
        return userSongVisible ? userSongInstruments : "";
    return BUILTIN_SONG_REGISTRY[0].instruments;
}

int GameSoundSystem::getSongTickRate(int songIndex) const
{
    const BuiltinSongDefinition *song = BuiltinSong_BySongId(songIndex);
    if (song)
    {
        const BuiltinSongOverride &override = builtinSongOverrides[songIndex - 1];
        if (override.active)
            return override.tickRate;
        return song->tickRate;
    }
    if (songIndex == TRACKER_USER_SONG_SLOT)
        return userSongVisible ? userSongTickRate : BUILTIN_SONG_REGISTRY[0].tickRate;
    return BUILTIN_SONG_REGISTRY[0].tickRate;
}

int GameSoundSystem::getSongSpeed(int songIndex) const
{
    const BuiltinSongDefinition *song = BuiltinSong_BySongId(songIndex);
    if (song)
    {
        const BuiltinSongOverride &override = builtinSongOverrides[songIndex - 1];
        if (override.active)
            return override.speed;
        return song->speed;
    }
    if (songIndex == TRACKER_USER_SONG_SLOT)
        return userSongVisible ? userSongSpeed : BUILTIN_SONG_REGISTRY[0].speed;
    return BUILTIN_SONG_REGISTRY[0].speed;
}

int GameSoundSystem::getSongRowsPerBeat(int songIndex) const
{
    const BuiltinSongDefinition *song = BuiltinSong_BySongId(songIndex);
    if (song)
    {
        const BuiltinSongOverride &override = builtinSongOverrides[songIndex - 1];
        if (override.active)
            return override.rowsPerBeat;
        return song->rowsPerBeat;
    }
    if (songIndex == TRACKER_USER_SONG_SLOT)
        return userSongVisible ? userSongRowsPerBeat : BUILTIN_SONG_REGISTRY[0].rowsPerBeat;
    return BUILTIN_SONG_REGISTRY[0].rowsPerBeat;
}

int GameSoundSystem::getSongScaleRoot(int songIndex) const
{
    const BuiltinSongDefinition *song = BuiltinSong_BySongId(songIndex);
    if (song)
    {
        const BuiltinSongOverride &override = builtinSongOverrides[songIndex - 1];
        if (override.active)
            return override.scaleRoot;
        return song->scaleRoot;
    }
    if (songIndex == TRACKER_USER_SONG_SLOT)
        return userSongVisible ? userSongScaleRoot : BUILTIN_SONG_REGISTRY[0].scaleRoot;
    return BUILTIN_SONG_REGISTRY[0].scaleRoot;
}

int GameSoundSystem::getSongTuningMode(int songIndex) const
{
    const BuiltinSongDefinition *song = BuiltinSong_BySongId(songIndex);
    if (song)
    {
        const BuiltinSongOverride &override = builtinSongOverrides[songIndex - 1];
        if (override.active)
            return override.tuningMode;
        return song->tuningMode;
    }
    if (songIndex == TRACKER_USER_SONG_SLOT)
        return userSongVisible ? userSongTuningMode : BUILTIN_SONG_REGISTRY[0].tuningMode;
    return BUILTIN_SONG_REGISTRY[0].tuningMode;
}

bool GameSoundSystem::getSongLfoEnabled(int songIndex) const
{
    const BuiltinSongDefinition *song = BuiltinSong_BySongId(songIndex);
    if (song)
    {
        const BuiltinSongOverride &override = builtinSongOverrides[songIndex - 1];
        if (override.active)
            return override.lfoEnabled;
        return song->lfoEnabled;
    }
    if (songIndex == TRACKER_USER_SONG_SLOT)
        return userSongVisible ? userSongLfoEnabled : BUILTIN_SONG_REGISTRY[0].lfoEnabled;
    return BUILTIN_SONG_REGISTRY[0].lfoEnabled;
}

int GameSoundSystem::getSongLfoFrequency(int songIndex) const
{
    const BuiltinSongDefinition *song = BuiltinSong_BySongId(songIndex);
    if (song)
    {
        const BuiltinSongOverride &override = builtinSongOverrides[songIndex - 1];
        if (override.active)
            return override.lfoFrequency;
        return song->lfoFrequency;
    }
    if (songIndex == TRACKER_USER_SONG_SLOT)
        return userSongVisible ? userSongLfoFrequency : BUILTIN_SONG_REGISTRY[0].lfoFrequency;
    return BUILTIN_SONG_REGISTRY[0].lfoFrequency;
}

int GameSoundSystem::visibleSongCount() const
{
    return userSongVisible ? TRACKER_MAX_SONG_COUNT : TRACKER_BUILTIN_SONG_COUNT;
}

bool GameSoundSystem::setUserSong(
    const char *displayName,
    const char *uiPattern,
    const char *playbackPattern,
    const char *instrumentsText,
    int tickRate,
    int speed,
    int rowsPerBeat,
    int scaleRoot,
    int scaleMode,
    bool lfoEnabled,
    int lfoFrequency,
    int tuningMode)
{
    if (!displayName || !displayName[0] || !uiPattern || !uiPattern[0]) return false;
    if (!playbackPattern || !playbackPattern[0])
        playbackPattern = uiPattern;
    if (!instrumentsText)
        instrumentsText = "";

    size_t uiPatternLen = std::strlen(uiPattern);
    size_t playbackPatternLen = std::strlen(playbackPattern);
    size_t instrumentsLen = std::strlen(instrumentsText);
    if (uiPatternLen + 1 > sizeof(userSongUiPattern) ||
        playbackPatternLen + 1 > sizeof(userSongPattern) ||
        instrumentsLen + 1 > sizeof(userSongInstruments))
    {
        printf(
            "[Sound] ERROR: user song needs %zu/%zu/%zu bytes but only %zu/%zu/%zu bytes are reserved\n",
            uiPatternLen + 1,
            playbackPatternLen + 1,
            instrumentsLen + 1,
            sizeof(userSongUiPattern),
            sizeof(userSongPattern),
            sizeof(userSongInstruments)
        );
        return false;
    }
    std::snprintf(userSongName, sizeof(userSongName), "%s", displayName);
    std::snprintf(userSongUiPattern, sizeof(userSongUiPattern), "%s", uiPattern);
    std::snprintf(userSongPattern, sizeof(userSongPattern), "%s", playbackPattern);
    std::snprintf(userSongInstruments, sizeof(userSongInstruments), "%s", instrumentsText);
    userSongTickRate = std::max(1, tickRate);
    userSongSpeed = std::max(1, speed);
    userSongRowsPerBeat = std::max(1, rowsPerBeat);
    userSongScaleRoot = scaleRoot;
    userSongScaleMode = scaleMode;
    userSongTuningMode = std::max(0, std::min(1, tuningMode));
    userSongLfoEnabled = lfoEnabled;
    userSongLfoFrequency = lfoFrequency;
    userSongVisible = true;
    std::snprintf(
        settings.songNames[TRACKER_USER_SONG_SLOT],
        sizeof(settings.songNames[TRACKER_USER_SONG_SLOT]),
        "%d. %s",
        TRACKER_USER_SONG_SLOT,
        userSongName);
    return true;
}

bool GameSoundSystem::setBuiltinSongOverride(
    int songIndex,
    const char *displayName,
    const char *uiPattern,
    const char *playbackPattern,
    const char *instrumentsText,
    int tickRate,
    int speed,
    int rowsPerBeat,
    int scaleRoot,
    int scaleMode,
    bool lfoEnabled,
    int lfoFrequency,
    int tuningMode)
{
    const BuiltinSongDefinition *song = BuiltinSong_BySongId(songIndex);
    if (!song || !uiPattern || !uiPattern[0])
        return false;
    if (!playbackPattern || !playbackPattern[0])
        playbackPattern = uiPattern;
    BuiltinSongOverride &override = builtinSongOverrides[songIndex - 1];
    override.active = true;
    override.displayName = (displayName && displayName[0]) ? displayName : song->displayName;
    override.uiPattern = uiPattern;
    override.playbackPattern = playbackPattern;
    override.instrumentsText = instrumentsText ? instrumentsText : "";
    override.tickRate = std::max(1, tickRate);
    override.speed = std::max(1, speed);
    override.rowsPerBeat = std::max(1, rowsPerBeat);
    override.scaleRoot = scaleRoot;
    override.scaleMode = scaleMode;
    override.tuningMode = std::max(0, std::min(1, tuningMode));
    override.lfoEnabled = lfoEnabled;
    override.lfoFrequency = lfoFrequency;
    std::snprintf(
        settings.songNames[songIndex],
        sizeof(settings.songNames[songIndex]),
        "%d. %s",
        songIndex,
        override.displayName.c_str());
    return true;
}

bool GameSoundSystem::clearBuiltinSongOverride(int songIndex)
{
    const BuiltinSongDefinition *song = BuiltinSong_BySongId(songIndex);
    if (!song)
        return false;
    builtinSongOverrides[songIndex - 1] = {};
    std::snprintf(
        settings.songNames[songIndex],
        sizeof(settings.songNames[songIndex]),
        "%d. %s",
        songIndex,
        song->displayName);
    return true;
}

bool GameSoundSystem::hasBuiltinSongOverride(int songIndex) const
{
    return BuiltinSong_BySongId(songIndex) && builtinSongOverrides[songIndex - 1].active;
}

    // Call this every frame from game loop to progress restart state machine
bool GameSoundSystem::updateRestart()
{
    if (restartState == RestartState::RESTART_IDLE ||
        restartState == RestartState::RESTART_COMPLETE) {
        return false;  // Nothing to do
    }

    switch (restartState) {
        case RestartState::RESTART_PAUSE_AUDIO:
            // Step 1: Pause audio device
            printf("[SoundRestart] Step 1/5: Pausing audio device...\n");
            if (audioDev) {
                SDL_PauseAudioDevice(audioDev, 1);
            }
            restartState = RestartState::RESTART_WAIT_CALLBACKS;
            restartWaitFrames = restartTargetFrames;
            restartProgress = 0.2f;
            break;

        case RestartState::RESTART_WAIT_CALLBACKS:
            // Step 2: Wait for pending callbacks to finish
            restartWaitFrames--;
            if (restartWaitFrames <= 0) {
                printf("[SoundRestart] Step 2/5: Callbacks finished, destroying modules...\n");
                restartState = RestartState::RESTART_DESTROY_MODULES;
                restartProgress = 0.4f;
            }
            break;

        case RestartState::RESTART_DESTROY_MODULES:
            // Step 3: Destroy old modules (callback now returns early)
            shutdown();
            shutdownCompleteTime = SDL_GetTicks64();  // Start grace period
            printf("[SoundRestart] Step 3/5: Modules destroyed, grace period started (%dms)\n", GRACE_PERIOD_MS);
            restartState = RestartState::RESTART_WAIT_MORE;
            restartWaitFrames = restartTargetFrames;
            restartProgress = 0.6f;
            break;

        case RestartState::RESTART_WAIT_MORE:
            // Step 4: Wait for grace period to complete before re-init
            restartWaitFrames--;
            {
                uint32_t elapsed = SDL_GetTicks64() - shutdownCompleteTime;
                if (restartWaitFrames <= 0 && elapsed >= GRACE_PERIOD_MS) {
                    printf("[SoundRestart] Step 4/5: Grace period complete (%dms), re-initializing...\n", elapsed);
                    restartState = RestartState::RESTART_INIT_NEW;
                    restartProgress = 0.8f;
                } else if (elapsed < GRACE_PERIOD_MS) {
                    // Still waiting for grace period
                    if (restartWaitFrames <= 0) {
                        restartWaitFrames = 1;  // Keep checking
                    }
                }
            }
            break;

        case RestartState::RESTART_INIT_NEW:
            // Step 5: Initialize new system
            {
                printf("[SoundRestart] Step 5/5: Loading synth %d Hz...\n", sampleRate);
                bool result = initSoundSystem(restartSongPattern.c_str());
                restartState = result ? RestartState::RESTART_COMPLETE : RestartState::RESTART_IDLE;
                restartProgress = result ? 1.0f : 0.0f;
                if (result) {
                    printf("[SoundRestart] ✓ Restart complete - audio ready!\n");
                } else {
                    printf("[SoundRestart] ✗ Restart FAILED!\n");
                }
            }
            break;

        case RestartState::RESTART_COMPLETE:
            // Step 6: Done
            printf("[SoundRestart] Complete - resuming audio\n");
            restartState = RestartState::RESTART_IDLE;
            restartProgress = 0.0f;
            break;

        default:
            restartState = RestartState::RESTART_IDLE;
            break;
    }

    return true;  // Still in progress
}

    // Start async restart - call this from applySoundSettings
void GameSoundSystem::startRestart(const char* songPattern)
{
    if (!isRestartAllowed()) {
        if (restartState != RestartState::RESTART_IDLE && 
            restartState != RestartState::RESTART_COMPLETE) {
            restartSongPattern = songPattern ? songPattern : getSongPlaybackPattern(currentSongIndex);
            printf("[SoundRestart] Restart already in progress (state=%d), updated pending pattern\n", (int)restartState);
        } else {
            uint32_t elapsed = SDL_GetTicks64() - shutdownCompleteTime;
            restartSongPattern = songPattern ? songPattern : getSongPlaybackPattern(currentSongIndex);
            restartState = RestartState::RESTART_WAIT_MORE;
            restartWaitFrames = 1;
            restartProgress = 0.6f;
            printf("[SoundRestart] Grace period not elapsed (%dms < %dms), queued restart\n", 
                    elapsed, GRACE_PERIOD_MS);
        }
        return;
    }
    restartSongPattern = songPattern ? songPattern : getSongPlaybackPattern(currentSongIndex);
    restartState = RestartState::RESTART_PAUSE_AUDIO;
    printf("[SoundRestart] Starting async restart (target frames per wait=%d)\n", restartTargetFrames);
}

    // ------------------------------------------------------------------------
    // Audio callback (mix both modules)
    // ------------------------------------------------------------------------

static void my_audio_callback(void* userdata, Uint8* stream, int len)
{
    if (userdata == nullptr) {
        // To awoid bad memory errors in emscripten
        return;
    }
    GameSoundSystem* self = (GameSoundSystem*)userdata;

    // Emscripten: callback runs async, must check ALL state flags FIRST
    if (self->audioShutdownInProgress.load()) {
        std::memset(stream, 0, len);
        return;
    }

    // If restarting, output silence (no modules should be active)
    if (self->restartState != GameSoundSystem::RestartState::RESTART_IDLE &&
        self->restartState != GameSoundSystem::RestartState::RESTART_COMPLETE) {
        std::memset(stream, 0, len);
        return;
    }

    // Safety check - if NO modules are valid, just output silence
    if (!self->musicModule && !self->sfxModule) {
        std::memset(stream, 0, len);
        return;
    }

    int16_t* out = (int16_t*)stream;

    // len is in bytes. For stereo 16-bit audio:
    // 1 sample = 2 bytes
    // 1 frame (L + R) = 4 bytes
    // So number of frames = total bytes / 4
    int frames = len / 4;

    // Clear output buffer
    std::memset(out, 0, len);

    // Mix music (song only - more efficient!)
    if (self->musicModule)
    {
        static constexpr int OSC_CAPTURE_MAX_FRAMES = 8192;
        static int16_t oscChannelBuffers[TRACKER_OSC_CHANNELS][OSC_CAPTURE_MAX_FRAMES * 2];
        int16_t *oscPtrs[TRACKER_OSC_CHANNELS] = {};
        bool captureOsc =
            self->oscilloscopeCaptureEnabled.load(std::memory_order_relaxed) &&
            frames <= OSC_CAPTURE_MAX_FRAMES;
        if (captureOsc)
        {
            for (int ch = 0; ch < TRACKER_OSC_CHANNELS; ch++)
            {
                oscPtrs[ch] = oscChannelBuffers[ch];
                self->musicModule->oscilloscope_channel_buffers[ch] = oscPtrs[ch];
            }
        }
        xfm_mix_song(self->musicModule, out, frames);
        if (captureOsc)
        {
            for (int ch = 0; ch < TRACKER_OSC_CHANNELS; ch++)
                self->musicModule->oscilloscope_channel_buffers[ch] = nullptr;
            soundCaptureOscilloscope(self, oscPtrs, frames);
        }
    }

    if (self->fadingMusicModule && self->fadingMusicFramesRemaining > 0)
    {
        static int16_t fadeBuf[4096 * 2];
        int framesMixed = 0;
        while (framesMixed < frames && self->fadingMusicFramesRemaining > 0)
        {
            const int chunkFrames = std::min(4096, frames - framesMixed);
            const float fade01 =
                self->fadingMusicFramesTotal > 0
                    ? (float)self->fadingMusicFramesRemaining / (float)self->fadingMusicFramesTotal
                    : 0.0f;
            xfm_module_set_volume(self->fadingMusicModule, self->musicVolume * std::clamp(fade01, 0.0f, 1.0f));
            std::memset(fadeBuf, 0, chunkFrames * 2 * sizeof(int16_t));
            xfm_mix_song(self->fadingMusicModule, fadeBuf, chunkFrames);
            int16_t *dst = out + framesMixed * 2;
            for (int i = 0; i < chunkFrames * 2; i++)
            {
                int32_t mixed = (int32_t)dst[i] + fadeBuf[i];
                if (mixed > 32767) mixed = 32767;
                if (mixed < -32768) mixed = -32768;
                dst[i] = (int16_t)mixed;
            }
            self->fadingMusicFramesRemaining = std::max(0, self->fadingMusicFramesRemaining - chunkFrames);
            framesMixed += chunkFrames;
        }
        if (self->fadingMusicFramesRemaining <= 0)
        {
            xfm_module_set_volume(self->fadingMusicModule, 0.0f);
            soundSilenceSongModule(self->fadingMusicModule);
        }
    }

    // Mix SFX into temp buffer then add (SFX only - more efficient!)
    if (self->sfxModule)
    {
        // CRITICAL: Cap frames to prevent buffer overflow.
        // SDL may request more frames than expected on some platforms.
        int mix_frames = frames;
        if (mix_frames > 4096) mix_frames = 4096;
        
        static int16_t sfxBuf[4096 * 2];
        std::memset(sfxBuf, 0, mix_frames * 2 * sizeof(int16_t));
        xfm_mix_sfx(self->sfxModule, sfxBuf, mix_frames);

        for (int i = 0; i < mix_frames * 2; i++)
        {
            int32_t mixed = (int32_t)out[i] + sfxBuf[i];
            if (mixed > 32767) mixed = 32767;
            if (mixed < -32768) mixed = -32768;
            out[i] = (int16_t)mixed;
        }
    }
}

bool GameSoundSystem::reopenAudioDevice()
{
    if (audioDisabled)
        return true;
    if (audioDev)
    {
        audioShutdownInProgress.store(false);
        SDL_PauseAudioDevice(audioDev, 0);
        return true;
    }

    SDL_AudioSpec desired{};
    desired.freq = Sound_PreferredAudioSampleRate(*this);
    desired.format = AUDIO_S16SYS;
    desired.channels = 2;
    desired.samples = (Uint16)Sound_ClampAudioBufferSize(requestedBufferSize);
    desired.callback = my_audio_callback;
    desired.userdata = this;

    SDL_AudioSpec obtained{};
    audioDev = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);
    if (!audioDev)
    {
        printf("[SoundBrowser] Reopen audio device failed: %s\n", SDL_GetError());
        audioShutdownInProgress.store(true);
        return false;
    }

    obtainedSampleRate = obtained.freq > 0 ? obtained.freq : desired.freq;
    obtainedBufferSize = obtained.samples > 0 ? obtained.samples : desired.samples;
    sampleRate = obtainedSampleRate;
    audioShutdownInProgress.store(false);
    SDL_PauseAudioDevice(audioDev, 0);
    printf("[SoundBrowser] Audio device reopened: %d Hz, %d samples\n", obtained.freq, obtainedBufferSize);
    return true;
}

void GameSoundSystem::suspendForBrowser()
{
    if (audioDisabled)
    {
        browserAudioSuspended = false;
        audioStoppedBecauseWindowLeave = false;
        musicWasActiveBeforeBrowserSuspend = false;
        return;
    }
    if (browserAudioSuspended)
        return;
    // Remember whether music was playing, so a resume doesn't accidentally restart
    // user-stopped playback (common in the tracker UI).
    musicWasActiveBeforeBrowserSuspend = musicModule && musicModule->active_song.active;
    browserAudioSuspended = true;
    audioStoppedBecauseWindowLeave = true;
    audioShutdownInProgress.store(true);
    if (audioDev)
    {
        SDL_CloseAudioDevice(audioDev);
        audioDev = 0;
        printf("[SoundBrowser] Audio device closed for browser suspend\n");
    }
}

void GameSoundSystem::resumeFromBrowser(const char* songPattern)
{
    if (audioDisabled)
    {
        browserAudioSuspended = false;
        audioStoppedBecauseWindowLeave = false;
        return;
    }
    // Ignore spurious "resume" events (we listen to pointerdown/touchstart to satisfy autoplay),
    // but still allow this call to reopen the device if we currently have no audio device.
    if (!browserAudioSuspended && audioDev)
        return;
    if (restartState != RestartState::RESTART_IDLE && restartState != RestartState::RESTART_COMPLETE)
        return;

    audioShutdownInProgress.store(false);
    if (!musicModule && !sfxModule)
    {
        printf("[SoundBrowser] Modules missing on resume; reinitializing sound system\n");
        if (!initSoundSystem(songPattern ? songPattern : getSongPlaybackPattern(currentSongIndex)))
            audioShutdownInProgress.store(true);
        browserAudioSuspended = false;
        audioStoppedBecauseWindowLeave = false;
        return;
    }
    if (reopenAudioDevice())
    {
        browserAudioSuspended = false;
        audioStoppedBecauseWindowLeave = false;
        trackerNeedsFullPatchSync = true;
        if (musicWasActiveBeforeBrowserSuspend)
            playCurrentMusic(false);
        return;
    }

    printf("[SoundBrowser] Reopen failed on resume; rebuilding sound system\n");
    shutdown();
    shutdownCompleteTime = 0;
    if (initSoundSystem(songPattern ? songPattern : getSongPlaybackPattern(currentSongIndex)))
    {
        browserAudioSuspended = false;
        audioStoppedBecauseWindowLeave = false;
        trackerNeedsFullPatchSync = true;
        if (!musicWasActiveBeforeBrowserSuspend)
            stopMusic();
    }
}

void GameSoundSystem::playCurrentMusic(bool restart)
{
    if (audioDisabled)
        return;
    if (!audioDev)
    {
        if (!reopenAudioDevice())
            return;
    }
    if (musicModule)
    {
        if (!audioDev)
            return;
        SDL_LockAudioDevice(audioDev);
        if (restart || musicModule->active_song.song_id != currentSongIndex)
            xfm_song_play(musicModule, currentSongIndex, true);
        else
            musicModule->active_song.active = true;
        if (musicLoopEndRow >= 0)
            xfm_song_set_loop_range(musicModule, musicLoopStartRow, musicLoopEndRow);
        SDL_UnlockAudioDevice(audioDev);
    }
}

void GameSoundSystem::startMusicAtRow(int row)
{
    if (audioDisabled)
        return;
    if (!audioDev && !reopenAudioDevice())
        return;
    if (!musicModule || !audioDev)
        return;
    SDL_LockAudioDevice(audioDev);
    if (musicModule->active_song.song_id != currentSongIndex)
        xfm_song_play(musicModule, currentSongIndex, true);
    if (musicLoopEndRow >= 0)
        xfm_song_set_loop_range(musicModule, musicLoopStartRow, musicLoopEndRow);
    musicModule->active_song.active = true;
    xfm_song_jump_to_row(musicModule, row);
    SDL_UnlockAudioDevice(audioDev);
}

void GameSoundSystem::stopMusic()
{
    if (audioDisabled)
        return;
    if (musicModule)
    {
        if (!audioDev)
            return;
        SDL_LockAudioDevice(audioDev);
        musicModule->active_song.active = false;
        for (int ch = 0; ch < 6; ch++)
        {
            if (musicModule->chip)
                musicModule->chip->key_off(ch);
            musicModule->channel_active[ch] = false;
            musicModule->current_patch[ch] = -1;
            musicModule->live_patch_valid[ch] = false;
            musicModule->live_patch_id[ch] = -1;
            musicModule->live_op_mask[ch] = 0x0F;

            XfmSongChannel &chState = musicModule->active_song.channels[ch];
            chState.pending_has_note = false;
            chState.pending_is_off = false;
            chState.pending_note = -1;
            chState.pending_patch = -1;
            chState.wait_for_next_row = false;
            chState.release_keyoff_pending = false;
            chState.envelope_rekey_pending = false;
            chState.retrigger_next_sample = -1;
            chState.patch_morph_active = false;
            chState.patch_morph_pending_start = false;
            for (int target = 0; target < XFM_MACRO_TARGET_COUNT; target++)
            {
                chState.macro_states[target].active = false;
                chState.macro_states[target].released = false;
            }
        }
        SDL_UnlockAudioDevice(audioDev);
    }
    if (fadingMusicModule && audioDev)
    {
        SDL_LockAudioDevice(audioDev);
        soundSilenceSongModule(fadingMusicModule);
        fadingMusicFramesRemaining = 0;
        fadingMusicFramesTotal = 0;
        SDL_UnlockAudioDevice(audioDev);
    }
}

bool GameSoundSystem::initSoundSystem(const char* songPattern)
{
    if (audioDisabled) {
        printf("[SoundInit] Audio disabled; skipping audio initialization\n");
        return true;
    }

    currentSongIndex = soundCoerceVisibleSongIndex(this, currentSongIndex);

    const bool hasSynthModules = musicModule || fadingMusicModule || sfxModule;

    if (audioDev && hasSynthModules)
    {
        printf("[SoundInit] Audio already initialized; reusing current device\n");
        audioShutdownInProgress.store(false);
        SDL_PauseAudioDevice(audioDev, 0);
        if (musicModule)
        {
            SDL_LockAudioDevice(audioDev);
            xfm_song_play(musicModule, currentSongIndex, true);
            if (musicLoopEndRow >= 0)
                xfm_song_set_loop_range(musicModule, musicLoopStartRow, musicLoopEndRow);
            SDL_UnlockAudioDevice(audioDev);
        }
        return true;
    }

    if (audioDev || hasSynthModules)
    {
        printf("[SoundInit] Existing audio state detected before init; shutting it down first\n");
        shutdown();
    }

    printf("[SoundInit] Initializing synth at %d Hz...\n", sampleRate);
    
    SDL_AudioSpec desired{};
    // Use the sampleRate setting for both modes to ensure consistency
    desired.freq     = sampleRate;
    desired.format   = AUDIO_S16SYS;
    desired.channels = 2;
    desired.samples  = (Uint16)Sound_ClampAudioBufferSize(requestedBufferSize);
    desired.callback = my_audio_callback;
    desired.userdata = this;

    SDL_AudioSpec obtained{};

    audioDev = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);
    if (!audioDev)
    {
        printf("Audio error: %s\n", SDL_GetError());
        return false;
    }

    // Safety check - obtained.freq must be valid
    if (obtained.freq <= 0) {
        printf("Audio error: invalid sample rate %d\n", obtained.freq);
        SDL_CloseAudioDevice(audioDev);
        audioDev = 0;
        return false;
    }

    obtainedBufferSize = obtained.samples > 0 ? obtained.samples : desired.samples;
    printf("Audio: %d Hz, %d samples (%.1f ms latency)\n",
            obtained.freq, obtainedBufferSize,
            obtainedBufferSize * 1000.0 / obtained.freq);
    
    // Store the obtained sample rate for later use
    obtainedSampleRate = obtained.freq;
    sampleRate = obtainedSampleRate;

    // Any time we rebuild modules, the tracker must re-upload custom patches/macros.
    trackerNeedsFullPatchSync = true;
    BallRolling_ResetAutomationCache(this);
    ballRollingBasePatchValid = false;

    printf("[SoundInit] Creating synth modules at %d Hz...\n", obtained.freq);
    musicModule = xfm_module_create(obtained.freq, obtainedBufferSize, XFM_CHIP_YM3438);
    sfxModule   = xfm_module_create(obtained.freq, obtainedBufferSize, XFM_CHIP_YM3438);
    if (!musicModule || !sfxModule)
    {
        printf("xfm_module_create failed\n");
        return false;
    }
    printf("[SoundInit] Synth modules created: music=%p, sfx=%p\n", (void*)musicModule, (void*)sfxModule);

    // --------------------------------------------------------------------
    // Load patches (use XFM_CHIP_YM3438 to match module creation)
    // --------------------------------------------------------------------

    BuiltinSfx_ApplyInstrumentBank(sfxModule);
    const int rollingInstrument = BuiltinSfx_GlobalInstrumentForLocal(SFX_BALL_ROLLING, 0);
    if (rollingInstrument >= 0 && rollingInstrument < 256 && sfxModule->patch_present[rollingInstrument])
    {
        ballRollingBasePatch = sfxModule->patches[rollingInstrument];
        ballRollingBasePatchValid = true;
    }
    const BuiltinSfxDefinition *firstSfx = BuiltinSfx_ByIndex(0);
    xfm_module_set_lfo(
        sfxModule,
        firstSfx ? firstSfx->lfoEnabled : true,
        firstSfx ? firstSfx->lfoFrequency : 5
    );

    // --------------------------------------------------------------------
    // Declare song
    // --------------------------------------------------------------------

        const char *effectiveSongPattern = songPattern ? songPattern : getSongPlaybackPattern(currentSongIndex);
        const int songTickRate = std::max(1, getSongTickRate(currentSongIndex));
        const int songTicksPerStep = std::max(1, getSongSpeed(currentSongIndex));
        const bool songLfoEnabled = getSongLfoEnabled(currentSongIndex);
        const int songLfoFrequency = getSongLfoFrequency(currentSongIndex);

    printf("Declaring song...\n");
    soundApplySongInstrumentBankToMusicModule(this, currentSongIndex);
    xfm_module_set_lfo(musicModule, songLfoEnabled, songLfoFrequency);
    xfm_module_set_tuning(
        musicModule,
        (xfm_tuning_mode)getSongTuningMode(currentSongIndex),
        getSongScaleRoot(currentSongIndex));
    xfm_song_declare(musicModule, currentSongIndex, effectiveSongPattern, songTickRate, songTicksPerStep);
    musicLoopStartRow = 0;
    musicLoopEndRow = xfm_song_get_total_rows(musicModule, currentSongIndex) - 1;

    // --------------------------------------------------------------------
    // Declare SFX (patterns now use instrument 00)
    // --------------------------------------------------------------------

    // SFX files mirror chip-wide timing/LFO metadata for tracker/editing, but at runtime
    // the live SFX chip uses one shared global tempo/LFO configuration instead of per-SFX settings.
    for (int i = 0; i < SFX_COUNT; ++i)
    {
        const BuiltinSfxPrepared *prepared = BuiltinSfx_PreparedByIndex(i);
        if (!prepared || !prepared->def)
            continue;
        xfm_sfx_declare(
            sfxModule,
            prepared->def->sfxId,
            prepared->remappedPattern.c_str(),
            prepared->tickRate,
            prepared->speed
        );
    }
    // --------------------------------------------------------------------
    // Volume - apply stored volume levels (preserved across quality changes)
    // --------------------------------------------------------------------

    xfm_module_set_volume(musicModule, musicVolume);
    xfm_module_set_volume(sfxModule, sfxVolume);
    printf("[SoundInit] Synth volumes set: music=%.2f, sfx=%.2f\n", musicVolume, sfxVolume);

    printf("Playing song...\n");
    xfm_song_play(musicModule, currentSongIndex, true);
    xfm_song_set_loop_range(musicModule, musicLoopStartRow, musicLoopEndRow);
    printf("Music should be playing!\n");

    // Emscripten: Clear shutdown flag BEFORE unpausing device so callback sees ready state
    audioShutdownInProgress.store(false);

    SDL_PauseAudioDevice(audioDev, 0);
    printf("DEBUG: musicModule=%p\n", (void*)musicModule);

    return true;
}

    // ------------------------------------------------------------------------
    // Shutdown
    // ------------------------------------------------------------------------

void GameSoundSystem::shutdown()
{
    printf("[SoundShutdown] Shutting down audio...\n");

    // CRITICAL: Set shutdown flag FIRST - callback checks this before anything else
    audioShutdownInProgress.store(true);

    // CRITICAL: Close audio device COMPLETELY to stop callback on Emscripten
    // SDL_PauseAudioDevice is NOT enough - callback keeps running async
    if (audioDev) {
        SDL_CloseAudioDevice(audioDev);
        audioDev = 0;
        printf("[SoundShutdown] Audio device closed\n");
    }
    obtainedBufferSize = 0;

    // Destroy synth modules
    if (musicModule)
    {
        printf("[SoundShutdown] Destroying musicModule %p\n", (void*)musicModule);
        xfm_module_destroy(musicModule);
        musicModule = nullptr;
    }

    if (fadingMusicModule)
    {
        printf("[SoundShutdown] Destroying fadingMusicModule %p\n", (void*)fadingMusicModule);
        xfm_module_destroy(fadingMusicModule);
        fadingMusicModule = nullptr;
    }
    fadingMusicFramesRemaining = 0;
    fadingMusicFramesTotal = 0;

    if (sfxModule)
    {
        printf("[SoundShutdown] Destroying sfxModule %p\n", (void*)sfxModule);
        xfm_module_destroy(sfxModule);
        sfxModule = nullptr;
    }

    printf("[SoundShutdown] Complete\n");
}


bool GameSoundSystem::restartSoundSystem()
{
    // For async restart, just start the state machine
    const char* songPattern = getSongPlaybackPattern(currentSongIndex);
    startRestart(songPattern);
    return true;  // Restart initiated (will complete asynchronously)
}

void GameSoundSystem::redeclareCurrentMusic()
{
    if (audioDisabled || !musicModule)
        return;
    currentSongIndex = soundCoerceVisibleSongIndex(this, currentSongIndex);
    const int songId = currentSongIndex;
    const char *pattern = getSongPlaybackPattern(songId);
    SDL_LockAudioDevice(audioDev);
    soundApplySongInstrumentBankToMusicModule(this, songId);
    xfm_module_set_lfo(musicModule, getSongLfoEnabled(songId), getSongLfoFrequency(songId));
    xfm_module_set_tuning(musicModule, (xfm_tuning_mode)getSongTuningMode(songId), getSongScaleRoot(songId));
    xfm_song_declare(
        musicModule,
        songId,
        pattern,
        std::max(1, getSongTickRate(songId)),
        std::max(1, getSongSpeed(songId)));
    SDL_UnlockAudioDevice(audioDev);
}

    // ------------------------------------------------------------------------
    // Next song
    // ------------------------------------------------------------------------

void GameSoundSystem::nextSong()
{
    int count = visibleSongCount();
    currentSongIndex = soundCoerceVisibleSongIndex(this, currentSongIndex);
    currentSongIndex = (currentSongIndex % count) + 1;

    const char* songPattern = getSongPlaybackPattern(currentSongIndex);
    const int songTickRate = std::max(1, getSongTickRate(currentSongIndex));
    const int songTicksPerStep = std::max(1, getSongSpeed(currentSongIndex));

    if (musicModule && songPattern) {
        // Declare and play new song (this replaces the current one)
        soundApplySongInstrumentBankToMusicModule(this, currentSongIndex);
        xfm_module_set_lfo(musicModule, getSongLfoEnabled(currentSongIndex), getSongLfoFrequency(currentSongIndex));
        xfm_module_set_tuning(
            musicModule,
            (xfm_tuning_mode)getSongTuningMode(currentSongIndex),
            getSongScaleRoot(currentSongIndex));
        xfm_song_declare(musicModule, currentSongIndex, songPattern, songTickRate, songTicksPerStep);
        xfm_song_play(musicModule, currentSongIndex, true);
        clearMusicLoopRange();
        printf("Playing song %d\n", currentSongIndex);
    }
    // Update UI song name
    std::snprintf(settings.currentSongName, sizeof(settings.currentSongName), "%s", settings.songNames[currentSongIndex]);
}

void GameSoundSystem::nextSongForLevelTransition()
{
    const int count = visibleSongCount();
    if (count <= 0)
        return;
    const int oldSongIndex = soundCoerceVisibleSongIndex(this, currentSongIndex);
    const int nextSongIndex = (oldSongIndex % count) + 1;

    if (audioDisabled || !audioDev || !musicModule || !musicModule->active_song.active)
        return;

    const int moduleSampleRate = obtainedSampleRate > 0 ? obtainedSampleRate : Sound_PreferredAudioSampleRate(*this);
    const int moduleBufferSize = obtainedBufferSize > 0 ? obtainedBufferSize : Sound_ClampAudioBufferSize(requestedBufferSize);
    xfm_module *newMusicModule = xfm_module_create(moduleSampleRate, moduleBufferSize, XFM_CHIP_YM3438);
    if (!newMusicModule)
    {
        nextSong();
        return;
    }

    SDL_LockAudioDevice(audioDev);
    if (!musicModule || !musicModule->active_song.active)
    {
        SDL_UnlockAudioDevice(audioDev);
        xfm_module_destroy(newMusicModule);
        return;
    }

    if (fadingMusicModule && fadingMusicModule != musicModule)
        xfm_module_destroy(fadingMusicModule);
    fadingMusicModule = musicModule;
    fadingMusicFramesTotal = std::max(1, moduleSampleRate);
    fadingMusicFramesRemaining = fadingMusicFramesTotal;
    xfm_module_set_volume(fadingMusicModule, musicVolume);

    musicModule = newMusicModule;
    currentSongIndex = nextSongIndex;
    soundDeclareSongOnMusicModule(this, musicModule, currentSongIndex);
    xfm_module_set_volume(musicModule, musicVolume);
    xfm_song_play(musicModule, currentSongIndex, true);
    musicLoopStartRow = 0;
    musicLoopEndRow = xfm_song_get_total_rows(musicModule, currentSongIndex) - 1;
    if (musicLoopEndRow >= 0)
        xfm_song_set_loop_range(musicModule, musicLoopStartRow, musicLoopEndRow);
    trackerNeedsFullPatchSync = true;
    SDL_UnlockAudioDevice(audioDev);

    std::snprintf(settings.currentSongName, sizeof(settings.currentSongName), "%s", settings.songNames[currentSongIndex]);
    printf("Level transition song %d -> %d (old song fades for 1s)\n", oldSongIndex, currentSongIndex);
}

    // ------------------------------------------------------------------------
    // Previous song
    // ------------------------------------------------------------------------

void GameSoundSystem::previousSong()
{
    int count = visibleSongCount();
    currentSongIndex = soundCoerceVisibleSongIndex(this, currentSongIndex);
    currentSongIndex = ((currentSongIndex - 2 + count) % count) + 1;

    const char* songPattern = getSongPlaybackPattern(currentSongIndex);
    const int songTickRate = std::max(1, getSongTickRate(currentSongIndex));
    const int songTicksPerStep = std::max(1, getSongSpeed(currentSongIndex));

    if (musicModule && songPattern) {
        // Declare and play new song (this replaces the current one)
        soundApplySongInstrumentBankToMusicModule(this, currentSongIndex);
        xfm_module_set_lfo(musicModule, getSongLfoEnabled(currentSongIndex), getSongLfoFrequency(currentSongIndex));
        xfm_module_set_tuning(
            musicModule,
            (xfm_tuning_mode)getSongTuningMode(currentSongIndex),
            getSongScaleRoot(currentSongIndex));
        xfm_song_declare(musicModule, currentSongIndex, songPattern, songTickRate, songTicksPerStep);
        xfm_song_play(musicModule, currentSongIndex, true);
        clearMusicLoopRange();
        printf("Playing song %d\n", currentSongIndex);
    }
    // Update UI song name
    std::snprintf(settings.currentSongName, sizeof(settings.currentSongName), "%s", settings.songNames[currentSongIndex]);
}

void GameSoundSystem::setMusicLoopRange(int startRow, int endRow)
{
    musicLoopStartRow = std::max(0, std::min(startRow, endRow));
    musicLoopEndRow = std::max(startRow, endRow);
    if (audioDisabled || !musicModule) return;

    SDL_LockAudioDevice(audioDev);
    xfm_song_set_loop_range(musicModule, musicLoopStartRow, musicLoopEndRow);
    SDL_UnlockAudioDevice(audioDev);
}

void GameSoundSystem::clearMusicLoopRange()
{
    musicLoopStartRow = 0;
    musicLoopEndRow = -1;
    if (audioDisabled || !musicModule) return;

    int rows = xfm_song_get_total_rows(musicModule, currentSongIndex);
    musicLoopEndRow = rows > 0 ? rows - 1 : -1;
    SDL_LockAudioDevice(audioDev);
    xfm_song_set_loop_range(musicModule, musicLoopStartRow, musicLoopEndRow);
    SDL_UnlockAudioDevice(audioDev);
}

    // ------------------------------------------------------------------------
    // SFX playback
    // ------------------------------------------------------------------------

xfm_voice_id GameSoundSystem::playSfx(int id, int priority)
{
    if (audioDisabled) return FM_VOICE_INVALID;
    if (!audioDev && !reopenAudioDevice()) return FM_VOICE_INVALID;

    if (!sfxModule) {
        printf("[SFX] WARNING: sfxModule is null, cannot play SFX %d\n", id);
        return FM_VOICE_INVALID;
    }
    SDL_LockAudioDevice(audioDev);
    xfm_voice_id voice = xfm_sfx_play(sfxModule, id, priority);
    SDL_UnlockAudioDevice(audioDev);
    return voice;
}

static xfm_voice_id GameSoundSystem_PlayTrackerPreviewSlot(
    GameSoundSystem *self,
    int previewSlot,
    int note,
    int octave,
    int instrument,
    int volume,
    const xfm_patch_opn *patchOverride,
    const XfmMacro *macros,
    const bool *macroEnabled,
    const bool *macroValid,
    bool held,
    xfm_voice_id previousVoice
)
{
    if (!self || self->audioDisabled) return FM_VOICE_INVALID;
    if (!self->sfxModule) return FM_VOICE_INVALID;
    if (!self->audioDev && !self->reopenAudioDevice()) return FM_VOICE_INVALID;

    static const char *names[12] = {"C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-"};
    previewSlot = std::max(0, std::min(GameSoundSystem::TRACKER_PREVIEW_POLY_COUNT - 1, previewSlot));
    int safeNote = std::max(0, std::min(11, note));
    int safeOctave = std::max(1, std::min(7, octave));
    int safeInstrument = std::max(0, std::min(255, instrument));
    int safeVolume = std::max(0, std::min(127, volume));
    const int previewInstrument = 0xEB + previewSlot;
    const int previewSfxId = GameSoundSystem::SFX_TRACKER_PREVIEW + previewSlot;
    const int macroBase = previewSlot * (XFM_MACRO_TARGET_COUNT - 1);

    const xfm_patch_opn *sourcePatch = patchOverride;
    if (!sourcePatch && self->musicModule && self->musicModule->patch_present[safeInstrument])
        sourcePatch = &self->musicModule->patches[safeInstrument];
    if (!sourcePatch && self->sfxModule->patch_present[safeInstrument])
        sourcePatch = &self->sfxModule->patches[safeInstrument];
    if (!sourcePatch) return FM_VOICE_INVALID;

    xfm_patch_opn previewPatch = *sourcePatch;
    int tlAdd = ((0x7F - safeVolume) * 127) / 0x7F;
    for (int op = 0; op < 4; op++)
        previewPatch.op[op].TL = (uint8_t)std::min(127, (int)previewPatch.op[op].TL + tlAdd);

    SDL_LockAudioDevice(self->audioDev);
    xfm_patch_set(self->sfxModule, previewInstrument, &previewPatch, sizeof(previewPatch), XFM_CHIP_YM3438);
    xfm_patch_macro_clear(self->sfxModule, previewInstrument, XFM_MACRO_NONE);
    if (macros && macroEnabled && macroValid)
    {
        int macroId = 0;
        for (int target = XFM_MACRO_TL1; target < XFM_MACRO_TARGET_COUNT; target++)
        {
            if (!macroEnabled[target] || !macroValid[target])
                continue;
            int previewMacroId = macroBase + macroId;
            if (previewMacroId >= XFM_MAX_MACROS)
                break;
            XfmMacro macro = macros[target];
            macro.target = (uint8_t)target;
            if (macro.length == 0)
                macro.length = 1;
            if (macro.length > XFM_MAX_MACRO_VALUES)
                macro.length = XFM_MAX_MACRO_VALUES;
            if (macro.has_loop && macro.loop_start >= macro.length)
                macro.loop_start = macro.length - 1;
            if (macro.release_start != 0xFF && macro.release_start >= macro.length)
                macro.release_start = macro.length - 1;
            if (xfm_macro_set(self->sfxModule, previewMacroId, &macro) >= 0)
            {
                xfm_patch_macro_set(self->sfxModule, previewInstrument, (uint8_t)target, previewMacroId);
                macroId++;
            }
        }
    }

    if (previousVoice != FM_VOICE_INVALID)
    {
        xfm_sfx_stop(self->sfxModule, previousVoice);
    }

    xfm_voice_id voice = FM_VOICE_INVALID;
    if (held)
    {
        // Long-running SFX that sustains until the caller explicitly stops it.
        static constexpr int TRACKER_PREVIEW_PRIORITY = 9;
        std::string pattern = "4096\n";
        char firstRow[16];
        std::snprintf(firstRow, sizeof(firstRow), "%s%d%02X7F\n", names[safeNote], safeOctave, previewInstrument);
        pattern += firstRow;
        for (int row = 1; row < 4096; row++)
            pattern += ".......\n";

        xfm_sfx_declare(self->sfxModule, previewSfxId, pattern.c_str(), 60, 1);
        voice = xfm_sfx_play(self->sfxModule, previewSfxId, TRACKER_PREVIEW_PRIORITY);
    }
    else
    {
        // Short preview: note, then a REL within ~1 row time.
        static constexpr int TRACKER_PREVIEW_PRIORITY = 9;
        char pattern[128];
        std::snprintf(
            pattern,
            sizeof(pattern),
            "4\n%s%d%02X7F\n.......\nREL....\n.......\n",
            names[safeNote],
            safeOctave,
            previewInstrument
        );
        xfm_sfx_declare(self->sfxModule, previewSfxId, pattern, 60, 1);
        voice = xfm_sfx_play(self->sfxModule, previewSfxId, TRACKER_PREVIEW_PRIORITY);
    }
    SDL_UnlockAudioDevice(self->audioDev);
    return voice;
}

static xfm_voice_id GameSoundSystem_PlayTrackerPreviewDirectSlot(
    GameSoundSystem *self,
    int previewSlot,
    int note,
    int octave,
    int instrument,
    int volume,
    const xfm_patch_opn *patchOverride)
{
    if (!self || self->audioDisabled) return FM_VOICE_INVALID;
    if (!self->sfxModule || !self->sfxModule->chip) return FM_VOICE_INVALID;
    if (!self->audioDev && !self->reopenAudioDevice()) return FM_VOICE_INVALID;

    const int voice = std::max(0, std::min(GameSoundSystem::TRACKER_PREVIEW_POLY_COUNT - 1, previewSlot));
    const int safeNote = std::max(0, std::min(11, note));
    const int safeOctave = std::max(1, std::min(7, octave));
    const int safeInstrument = std::max(0, std::min(255, instrument));
    const int safeVolume = std::max(0, std::min(127, volume));

    const xfm_patch_opn *sourcePatch = patchOverride;
    if (!sourcePatch && self->musicModule && self->musicModule->patch_present[safeInstrument])
        sourcePatch = &self->musicModule->patches[safeInstrument];
    if (!sourcePatch && self->sfxModule->patch_present[safeInstrument])
        sourcePatch = &self->sfxModule->patches[safeInstrument];
    if (!sourcePatch) return FM_VOICE_INVALID;

    xfm_patch_opn previewPatch = *sourcePatch;
    const int tlAdd = ((0x7F - safeVolume) * 127) / 0x7F;
    for (int op = 0; op < 4; op++)
        previewPatch.op[op].TL = (uint8_t)std::min(127, (int)previewPatch.op[op].TL + tlAdd);

    // Direct keyboard preview uses fixed YM voices instead of mutable SFX
    // patterns. This keeps simultaneous key presses from rewriting each
    // other's active preview data.
    SDL_LockAudioDevice(self->audioDev);
    xfm_sfx_stop(self->sfxModule, voice);
    self->sfxModule->chip->load_patch(previewPatch, voice);
    self->sfxModule->chip->enable_lfo(
        self->sfxModule->lfo_enable,
        static_cast<uint8_t>(self->sfxModule->lfo_freq)
    );
    const int midiNote = 12 + safeOctave * 12 + safeNote;
    static constexpr int XFM_PLAYBACK_OCTAVE_CORRECTION_SEMITONES = 12;
    const double hz = 440.0 * std::pow(2.0, (midiNote + XFM_PLAYBACK_OCTAVE_CORRECTION_SEMITONES - 69) / 12.0);
    self->sfxModule->chip->set_frequency(voice, hz, 0);
    self->sfxModule->chip->key_on(voice);
    self->sfxModule->voices[voice].midi_note = midiNote;
    self->sfxModule->voices[voice].patch_id = -1;
    self->sfxModule->voices[voice].active = true;
    self->sfxModule->voices[voice].age = ++self->sfxModule->voice_age_counter;
    self->sfxModule->voices[voice].priority = 9;
    self->sfxModule->voices[voice].sfx_id = -1;
    self->sfxModule->channel_active[voice] = true;
    self->sfxModule->current_patch[voice] = -1;
    self->sfxModule->live_patch_valid[voice] = false;
    self->sfxModule->live_patch_id[voice] = -1;
    self->sfxModule->live_op_mask[voice] = 0x0F;
    SDL_UnlockAudioDevice(self->audioDev);
    return voice;
}

xfm_voice_id GameSoundSystem::previewTrackerNote(
    int note,
    int octave,
    int instrument,
    int volume,
    const xfm_patch_opn *patchOverride,
    const XfmMacro *macros,
    const bool *macroEnabled,
    const bool *macroValid,
    bool held
)
{
    trackerPreviewVoice = GameSoundSystem_PlayTrackerPreviewSlot(
        this,
        0,
        note,
        octave,
        instrument,
        volume,
        patchOverride,
        macros,
        macroEnabled,
        macroValid,
        held,
        trackerPreviewVoice
    );
    return trackerPreviewVoice;
}

static int GameSoundSystem_FindTrackerPreviewFingerSlot(const GameSoundSystem *self, SDL_FingerID fingerId)
{
    if (!self) return -1;
    for (int i = 0; i < GameSoundSystem::TRACKER_PREVIEW_POLY_COUNT; i++)
        if (self->trackerPreviewFingerActive[i] && self->trackerPreviewFingerIds[i] == fingerId)
            return i;
    return -1;
}

static int GameSoundSystem_FindFreeTrackerPreviewFingerSlot(const GameSoundSystem *self)
{
    if (!self) return -1;
    for (int i = 0; i < GameSoundSystem::TRACKER_PREVIEW_POLY_COUNT; i++)
        if (!self->trackerPreviewFingerActive[i])
            return i;
    return -1;
}

xfm_voice_id GameSoundSystem::previewTrackerFingerNote(
    SDL_FingerID fingerId,
    int note,
    int octave,
    int instrument,
    int volume,
    const xfm_patch_opn *patchOverride,
    const XfmMacro *macros,
    const bool *macroEnabled,
    const bool *macroValid,
    bool directVoice
)
{
    int slot = GameSoundSystem_FindTrackerPreviewFingerSlot(this, fingerId);
    if (slot < 0)
        slot = GameSoundSystem_FindFreeTrackerPreviewFingerSlot(this);
    if (slot < 0)
        return FM_VOICE_INVALID;

    int safeNote = std::max(0, std::min(11, note));
    int safeOctave = std::max(1, std::min(7, octave));
    int safeInstrument = std::max(0, std::min(255, instrument));
    int safeVolume = std::max(0, std::min(127, volume));
    if (trackerPreviewFingerActive[slot] &&
        trackerPreviewFingerDirect[slot] == directVoice &&
        trackerPreviewFingerNotes[slot] == safeNote &&
        trackerPreviewFingerOctaves[slot] == safeOctave &&
        trackerPreviewFingerInstruments[slot] == safeInstrument &&
        trackerPreviewFingerVolumes[slot] == safeVolume)
    {
        return trackerPreviewFingerVoices[slot];
    }

    if (trackerPreviewFingerActive[slot] && trackerPreviewFingerDirect[slot] != directVoice)
        releaseTrackerPreviewFinger(trackerPreviewFingerIds[slot]);

    xfm_voice_id previousVoice = trackerPreviewFingerActive[slot] ? trackerPreviewFingerVoices[slot] : FM_VOICE_INVALID;
    xfm_voice_id voice = directVoice ?
        GameSoundSystem_PlayTrackerPreviewDirectSlot(
            this,
            slot,
            safeNote,
            safeOctave,
            safeInstrument,
            safeVolume,
            patchOverride
        ) :
        GameSoundSystem_PlayTrackerPreviewSlot(
            this,
            slot,
            safeNote,
            safeOctave,
            safeInstrument,
            safeVolume,
            patchOverride,
            macros,
            macroEnabled,
            macroValid,
            /*held=*/true,
            previousVoice
        );
    if (voice == FM_VOICE_INVALID)
        return voice;

    trackerPreviewFingerActive[slot] = true;
    trackerPreviewFingerIds[slot] = fingerId;
    trackerPreviewFingerDirect[slot] = directVoice;
    trackerPreviewFingerVoices[slot] = voice;
    trackerPreviewFingerNotes[slot] = safeNote;
    trackerPreviewFingerOctaves[slot] = safeOctave;
    trackerPreviewFingerInstruments[slot] = safeInstrument;
    trackerPreviewFingerVolumes[slot] = safeVolume;
    return voice;
}

void GameSoundSystem::releaseTrackerPreviewNote()
{
    if (audioDisabled || !sfxModule || !audioDev)
        return;
    SDL_LockAudioDevice(audioDev);
    if (trackerPreviewVoice != FM_VOICE_INVALID)
    {
        xfm_sfx_release_macros_then_stop(sfxModule, trackerPreviewVoice);
    }
    SDL_UnlockAudioDevice(audioDev);
}

void GameSoundSystem::releaseTrackerPreviewFinger(SDL_FingerID fingerId)
{
    if (audioDisabled || !sfxModule || !audioDev)
        return;
    int slot = GameSoundSystem_FindTrackerPreviewFingerSlot(this, fingerId);
    if (slot < 0)
        return;

    SDL_LockAudioDevice(audioDev);
    if (trackerPreviewFingerVoices[slot] != FM_VOICE_INVALID)
    {
        if (trackerPreviewFingerDirect[slot])
        {
            int voice = trackerPreviewFingerVoices[slot];
            if (voice >= 0 && voice < 6 && sfxModule->chip)
            {
                // Direct keyboard chords release like ===: send YM key_off and
                // let the operator RR finish naturally. Do not call
                // xfm_sfx_stop() here; that is for killing/recycling SFX
                // sequencer state, not for note-release audition.
                sfxModule->chip->key_off(voice);
                sfxModule->voices[voice].active = false;
                sfxModule->voices[voice].midi_note = -1;
                sfxModule->voices[voice].patch_id = -1;
                sfxModule->voices[voice].priority = 0;
                sfxModule->voices[voice].sfx_id = -1;
                sfxModule->channel_active[voice] = false;
            }
        }
        else
        {
            xfm_sfx_release_macros_then_stop(sfxModule, trackerPreviewFingerVoices[slot]);
        }
    }
    SDL_UnlockAudioDevice(audioDev);

    trackerPreviewFingerActive[slot] = false;
    trackerPreviewFingerDirect[slot] = false;
    trackerPreviewFingerVoices[slot] = FM_VOICE_INVALID;
    trackerPreviewFingerNotes[slot] = -1;
    trackerPreviewFingerOctaves[slot] = -1;
    trackerPreviewFingerInstruments[slot] = -1;
    trackerPreviewFingerVolumes[slot] = -1;
}

void GameSoundSystem::releaseAllTrackerPreviewNotes()
{
    releaseTrackerPreviewNote();
    for (int i = 0; i < TRACKER_PREVIEW_POLY_COUNT; i++)
    {
        if (trackerPreviewFingerActive[i])
            releaseTrackerPreviewFinger(trackerPreviewFingerIds[i]);
    }
}

bool GameSoundSystem::isTrackerPreviewFingerActive(SDL_FingerID fingerId) const
{
    return GameSoundSystem_FindTrackerPreviewFingerSlot(this, fingerId) >= 0;
}

void GameSoundSystem::stopSfx(xfm_voice_id voice)
{
    if (voice == FM_VOICE_INVALID) return;

    if (!sfxModule) return;
    SDL_LockAudioDevice(audioDev);
    xfm_sfx_stop(sfxModule, voice);
    SDL_UnlockAudioDevice(audioDev);
}

void GameSoundSystem::stopAllSfx()
{
    if (!sfxModule) return;
    SDL_LockAudioDevice(audioDev);
    xfm_sfx_stop_all(sfxModule);
    SDL_UnlockAudioDevice(audioDev);
}

    // ------------------------------------------------------------------------
    // Game hooks
    // ------------------------------------------------------------------------

void GameSoundSystem::playSfxBallHitLane()        { playSfx(SFX_BALL_HIT_LANE, 3); }
// Keep pin hits above glass tinkles so shard spam cannot steal the impact cue.
void GameSoundSystem::playSfxBallHitPins()        { playSfx(SFX_BALL_HIT_PINS, 6); }
void GameSoundSystem::playSfxPinHitsAnotherPin()  { playSfx(SFX_PIN_HIT_PIN, 3); }
void GameSoundSystem::playSfxFinalScoreDisplayed(){ playSfx(SFX_SCORE_DISPLAY, 6); }
void GameSoundSystem::playSfxBallInGutter()       { playSfx(SFX_GUTTER, 5); }
void GameSoundSystem::playSfxBallTimeout()        { playSfx(SFX_TIMEOUT, 4); }
void GameSoundSystem::playSfxCoinPickup()         { playSfx(SFX_COIN_PICKUP, 4); }
void GameSoundSystem::playSfxStrike()             { playSfx(SFX_STRIKE, 7); }
void GameSoundSystem::playSfxSpare()              { playSfx(SFX_SPARE, 7); }
void GameSoundSystem::playSfxNeutralRoll()        { playSfx(SFX_NEUTRAL_ROLL, 4); }
xfm_voice_id GameSoundSystem::playSfxBallRolling()
{
    BallRolling_ResetAutomationCache(this);
    return playSfx(SFX_BALL_ROLLING, 2);
}
void GameSoundSystem::updateBallRollingPatchForMotion(
    float ballSpeedMps,
    float ballZ,
    float slippery01,
    bool sliding,
    float angularSpeedSigned,
    float ballMassKg,
    bool isEnemyTurn)
{
    if (audioDisabled || !sfxModule || !audioDev)
        return;

    const int rollingInstrument = BuiltinSfx_GlobalInstrumentForLocal(SFX_BALL_ROLLING, 0);
    if (rollingInstrument < 0 || rollingInstrument >= 256 || !sfxModule->patch_present[rollingInstrument])
        return;

    SDL_LockAudioDevice(audioDev);
    if (!ballRollingBasePatchValid)
    {
        ballRollingBasePatch = sfxModule->patches[rollingInstrument];
        ballRollingBasePatchValid = true;
    }
    xfm_patch_opn patch = ballRollingBasePatch;
    BallRollingPatchAutomation::applyRecipe(
        patch,
        ballSpeedMps,
        ballZ,
        slippery01,
        sliding,
        angularSpeedSigned,
        ballMassKg,
        isEnemyTurn);
    const int op1Mul = patch.op[0].MUL;
    const int op1Tl = patch.op[0].TL;
    const int op2Mul = patch.op[1].MUL;
    const int op2Tl = patch.op[1].TL;
    const int op2Dr = patch.op[1].DR;
    const int op3Tl = patch.op[2].TL;
    const int op4Dr = patch.op[3].DR;
    const int op4Tl = patch.op[3].TL;
    const int op4Ssg = patch.op[3].SSG;
    const int fb = patch.FB;
    if (op1Mul == lastBallRollingOp1Mul &&
        op1Tl == lastBallRollingOp1Tl &&
        op2Mul == lastBallRollingOp2Mul &&
        op2Tl == lastBallRollingOp2Tl &&
        op2Dr == lastBallRollingOp2Dr &&
        op3Tl == lastBallRollingOp3Tl &&
        op4Dr == lastBallRollingOp4Dr &&
        op4Tl == lastBallRollingOp4Tl &&
        op4Ssg == lastBallRollingOp4Ssg &&
        fb == lastBallRollingFb)
    {
        SDL_UnlockAudioDevice(audioDev);
        return;
    }

    xfm_patch_set(sfxModule, rollingInstrument, &patch, sizeof(patch), XFM_CHIP_YM3438);
    xfm_patch_refresh_live(sfxModule, rollingInstrument);
    SDL_UnlockAudioDevice(audioDev);

    lastBallRollingOp1Mul = op1Mul;
    lastBallRollingOp1Tl = op1Tl;
    lastBallRollingOp2Mul = op2Mul;
    lastBallRollingOp2Tl = op2Tl;
    lastBallRollingOp2Dr = op2Dr;
    lastBallRollingOp3Tl = op3Tl;
    lastBallRollingOp4Dr = op4Dr;
    lastBallRollingOp4Tl = op4Tl;
    lastBallRollingOp4Ssg = op4Ssg;
    lastBallRollingFb = fb;
}
xfm_voice_id GameSoundSystem::playSfxNosLoop()     { return playSfx(SFX_NOS_LOOP, 2); }
void GameSoundSystem::playSfxWin()                { playSfx(SFX_WIN, 7); }
void GameSoundSystem::playSfxLose()               { playSfx(SFX_LOSE, 7); }
void GameSoundSystem::playSfxBuy()                { playSfx(SFX_BUY, 6); }
void GameSoundSystem::playSfxTypewriter()         { playSfx(SFX_TYPEWRITER, 6); }
void GameSoundSystem::playSfxGlassBreak()
{
    playSfx(SFX_GLASS_CRACK, 8);
    playSfx(SFX_GLASS_SCRAPE, 7);
    playSfx(SFX_GLASS_SHARDS, 7);
}
void GameSoundSystem::playSfxGlassTinkle()
{
    const uint64_t now = SDL_GetTicks64();
    if (lastGlassTinkleScheduleAt == 0 || now - lastGlassTinkleScheduleAt > 500)
        glassTinklePriority = 5;

    playSfx(SFX_GLASS_TINKLE, glassTinklePriority);

    if (glassTinklePriority > 0)
        glassTinklePriority--;
    lastGlassTinkleScheduleAt = now;
}
void GameSoundSystem::playSfxBoomBlast()
{
    playSfx(SFX_BOOM_BLAST, 9);
    playSfx(SFX_RUNE_SHOT, 10);
}
void GameSoundSystem::playSfxBallShardImpact()    { playSfx(SFX_BALL_SHARD_IMPACT, 4); }
void GameSoundSystem::playSfxBoltStrike()
{
    playSfx(SFX_BOLT_STRIKE, 9);
    playSfx(SFX_RUNE_SHOT, 10);
}
void GameSoundSystem::playSfxBoltBurn()           { playSfx(SFX_BOLT_BURN, 6); }
void GameSoundSystem::playSfxBoltAsh()            { playSfx(SFX_BOLT_ASH, 7); }
void GameSoundSystem::playSfxBoltSave()           { playSfx(SFX_BOLT_SAVE, 7); }
void GameSoundSystem::playSfxRuneShot()           { playSfx(SFX_RUNE_SHOT, 10); }
void GameSoundSystem::playSfxChestSpawn()         { playSfx(SFX_CHEST_SPAWN, 6); }
void GameSoundSystem::playSfxChestDespawn()       { playSfx(SFX_CHEST_DESPAWN, 5); }
void GameSoundSystem::playSfxChestPickup()        { playSfx(SFX_CHEST_PICKUP, 7); }
xfm_voice_id GameSoundSystem::playSfxChestReadyLoop()
{
    return playSfx(SFX_CHEST_READY_LOOP, 2);
}
void GameSoundSystem::playSfxChestSpinOut()       { playSfx(SFX_CHEST_SPIN_OUT, 7); }

    // ------------------------------------------------------------------------
    // Volume
    // ------------------------------------------------------------------------

void GameSoundSystem::setMusicVolume(float v)
{
    musicVolume = v;
    if (musicModule) xfm_module_set_volume(musicModule, v);
    if (fadingMusicModule && fadingMusicFramesRemaining > 0)
    {
        const float fade01 =
            fadingMusicFramesTotal > 0
                ? (float)fadingMusicFramesRemaining / (float)fadingMusicFramesTotal
                : 0.0f;
        xfm_module_set_volume(fadingMusicModule, v * std::clamp(fade01, 0.0f, 1.0f));
    }
}

void GameSoundSystem::setSfxVolume(float v)
{
    sfxVolume = v;
    if (sfxModule) xfm_module_set_volume(sfxModule, v);
}
    
    // ------------------------------------------------------------------------
    // Sound Settings UI
    // ------------------------------------------------------------------------
    
void GameSoundSystem::showSoundSettings()
{
    settings.activated = true;
}

void GameSoundSystem::hideSoundSettings()
{
    settings.activated = false;
}
    
// -----------------------------------------------------------------------------
// SoundSettings function implementations (must be after GameSoundSystem is defined)
// -----------------------------------------------------------------------------
