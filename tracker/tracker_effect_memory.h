#pragma once

// Read-only reconstruction of the channel state as the edited playback row
// begins. The probe uses the same song sequencer as audible playback;
// 11025 Hz keeps seeking through long songs quick without changing row/tick
// counts or the continuous effects' time-normalized rates.
struct TrackerEffectMemoryOrigin
{
    int row = -1;
    int playbackRow = -1;
    uint8_t value = 0;
};

static inline bool Tracker_EffectMemoryIsOpn(uint8_t code)
{
    return (code >= 0x10 && code <= 0x16) ||
           (code >= 0x19 && code <= 0x1D) || code == 0x30 ||
           (code >= 0x50 && code <= 0x63);
}

static inline bool Tracker_EffectMemoryIsPatchOpn(uint8_t code)
{
    return Tracker_EffectMemoryIsOpn(code) && code != 0x10 && code != 0x30;
}

static inline int Tracker_EffectMemoryTarget(uint8_t code, uint8_t value)
{
    if (code == 0x16 || (code >= 0x50 && code <= 0x55) || code == 0x60)
        return (value >> 4) & 0x0F;
    return 0;
}

struct TrackerEffectMemoryRule
{
    const char *continueMode;
    const char *until;
};

static inline TrackerEffectMemoryRule Tracker_EffectMemoryRule(uint8_t code)
{
    switch (code)
    {
    case 0x00: return {"FOREVER", "0000"};
    case 0x01: return {"FOREVER", "01/02 change\n03xx or E1/E2"};
    case 0x02: return {"FOREVER", "01/02 change\n03xx or E1/E2"};
    case 0x03: return {"TARGET", "REACHED / 0300\nor 01/02"};
    case 0x04: return {"FOREVER", "0400"};
    case 0x07: return {"FOREVER", "0700 or 0A"};
    case 0x08: return {"FOREVER", "NEW 08"};
    case 0x0A: return {"TARGET", "0/127, 0A00\nor 07"};
    case 0x0C: case 0xEC: case 0xED: case 0xFC: return {"ROW", "ROW END"};
    case 0xE1: case 0xE2: return {"TARGET", "REACHED"};
    case 0xE5: return {"NOTE", "NOTE\nor E580"};
    case 0xEA: return {"FOREVER", "EA00"};
    case 0xF5: return {"FOREVER", "F6 TARGET"};
    case 0xF6: return {"MACRO", "END\nor F5"};
    case 0x30: return {"FOREVER", "3000"};
    case 0x10: return {"FOREVER", "NEW 10"};
    default: return Tracker_EffectMemoryIsPatchOpn(code) ?
        TrackerEffectMemoryRule{"ALL NOTES", "NEXT WRITE TO PARAM\n(FX OR MACRO)\nOTHER INSTRUMENT\nSTORED PATCH EDIT"} : TrackerEffectMemoryRule{"", ""};
    }
}

static inline int Tracker_EffectMemoryOpnValue(const xfm_patch_opn &patch, uint8_t code, int op)
{
    op = std::max(0, std::min(3, op));
    switch (code)
    {
    case 0x11: return patch.FB;
    case 0x12: case 0x13: case 0x14: case 0x15: return patch.op[code - 0x12].TL;
    case 0x16: return patch.op[op].MUL;
    case 0x19: case 0x1A: case 0x1B: case 0x1C: case 0x1D: return patch.op[code == 0x19 ? op : code - 0x1A].AR;
    case 0x50: return patch.op[op].AM;
    case 0x51: return patch.op[op].SL;
    case 0x52: return patch.op[op].RR;
    case 0x53: return patch.op[op].DT;
    case 0x54: return patch.op[op].RS;
    case 0x55: return patch.op[op].SSG;
    case 0x56: case 0x57: case 0x58: case 0x59: case 0x5A: return patch.op[code == 0x56 ? op : code - 0x57].DR;
    case 0x5B: case 0x5C: case 0x5D: case 0x5E: case 0x5F: return patch.op[code == 0x5B ? op : code - 0x5C].SR;
    case 0x61: return patch.ALG;
    case 0x62: return patch.FMS;
    case 0x63: return patch.AMS;
    default: return 0;
    }
}

static inline void Tracker_EffectMemoryPatchBase(
    const xfm_module *probe, int channel, uint8_t code, int target,
    char *out, size_t size)
{
    if (!probe || !Tracker_EffectMemoryIsPatchOpn(code)) return;
    const int patchId = probe->active_song.channels[channel].current_patch;
    if (patchId < 0 || !probe->patch_present[patchId]) return;
    if (code == 0x60)
    {
        std::snprintf(out, size, "patch 0x0F");
        return;
    }
    const xfm_patch_opn &patch = probe->patches[patchId];
    const bool allOps = code == 0x19 || code == 0x56 || code == 0x5B ||
        ((code >= 0x50 && code <= 0x55) && target == 0);
    if (allOps)
    {
        std::snprintf(out, size, "patch 0x%02X/%02X/%02X/%02X",
            Tracker_EffectMemoryOpnValue(patch, code, 0),
            Tracker_EffectMemoryOpnValue(patch, code, 1),
            Tracker_EffectMemoryOpnValue(patch, code, 2),
            Tracker_EffectMemoryOpnValue(patch, code, 3));
        return;
    }
    std::snprintf(out, size, "patch 0x%02X",
        Tracker_EffectMemoryOpnValue(patch, code, target > 0 ? target - 1 : 0));
}

static inline bool Tracker_EffectMemoryFormatOpn(
    const xfm_module *probe, int channel, uint8_t code, int target,
    bool requireDifference, char *out, size_t size)
{
    if (!probe || !probe->live_patch_valid[channel]) return false;
    const XfmSongChannel &state = probe->active_song.channels[channel];
    if (state.current_patch < 0 || !probe->patch_present[state.current_patch]) return false;
    const xfm_patch_opn &live = probe->live_patches[channel];
    const xfm_patch_opn &base = probe->patches[state.current_patch];
    if (code == 0x60)
    {
        const int mask = probe->live_op_mask[channel] & 0x0F;
        if (requireDifference && mask == 0x0F) return false;
        std::snprintf(out, size, "mask %X", mask);
        return true;
    }
    if (target >= 1 && target <= 4)
    {
        int op = target - 1;
        int value = Tracker_EffectMemoryOpnValue(live, code, op);
        if (requireDifference && value == Tracker_EffectMemoryOpnValue(base, code, op)) return false;
        std::snprintf(out, size, "OP%d = %d", target, value);
        return true;
    }
    const bool allOps = code == 0x19 || code == 0x56 || code == 0x5B ||
        (code >= 0x50 && code <= 0x55);
    if (allOps)
    {
        bool changed = false;
        for (int op = 0; op < 4; ++op)
            changed |= Tracker_EffectMemoryOpnValue(live, code, op) != Tracker_EffectMemoryOpnValue(base, code, op);
        if (requireDifference && !changed) return false;
        std::snprintf(out, size, "OP1-4 %d/%d/%d/%d",
            Tracker_EffectMemoryOpnValue(live, code, 0), Tracker_EffectMemoryOpnValue(live, code, 1),
            Tracker_EffectMemoryOpnValue(live, code, 2), Tracker_EffectMemoryOpnValue(live, code, 3));
        return true;
    }
    int value = Tracker_EffectMemoryOpnValue(live, code, 0);
    if (requireDifference && value == Tracker_EffectMemoryOpnValue(base, code, 0)) return false;
    std::snprintf(out, size, "%d", value);
    return true;
}

static inline bool Tracker_EffectMemoryFormatControl(
    const Tracker *tracker, const xfm_module *probe, uint8_t code, uint8_t value,
    char *out, size_t size)
{
    const int ch = tracker->editChannel;
    const XfmSongChannel &state = probe->active_song.channels[ch];
    switch (code)
    {
    case 0x00:
        if (!state.effect_arpeggio_active) return false;
        std::snprintf(out, size, "+%d / +%d semitones", state.effect_arpeggio_step_a, state.effect_arpeggio_step_b); return true;
    case 0x01: case 0x02:
        if ((code == 0x01 && state.pitch_slide_speed <= 0) ||
            (code == 0x02 && state.pitch_slide_speed >= 0)) return false;
        std::snprintf(out, size, "%+d; %.1f Hz", state.pitch_slide_speed, state.current_hz); return true;
    case 0x03:
        if (state.portamento_speed <= 0) return false;
        if (state.portamento_active)
            std::snprintf(out, size, "speed %d → %.1f Hz", state.portamento_speed, state.target_hz);
        else std::snprintf(out, size, "speed %d armed", state.portamento_speed);
        return true;
    case 0x04:
        if (state.vibrato_depth <= 0) return false;
        std::snprintf(out, size, "speed %d, ±%d cents", state.vibrato_speed, (int)std::lround(state.vibrato_depth * 127.0 * 100.0 / (16.0 * 128.0))); return true;
    case 0x07:
        if (state.tremolo_depth <= 0) return false;
        std::snprintf(out, size, "speed %d, 0-%d volume", state.tremolo_speed, state.tremolo_depth); return true;
    case 0x08:
        if (state.effect_pan < 0) return false;
        std::snprintf(out, size, "%s", state.effect_pan == 3 ? "left + right" :
            state.effect_pan == 2 ? "left" : state.effect_pan == 1 ? "right" : "muted"); return true;
    case 0x0A:
        if (state.volume_slide_speed == 0) return false;
        std::snprintf(out, size, "volume %d/127 (%+d/4 ticks)", state.current_volume, state.volume_slide_speed); return true;
    case 0xE1: case 0xE2:
        if (!state.note_slide_active || (code == 0xE1 && state.note_slide_step_hz < 0) ||
            (code == 0xE2 && state.note_slide_step_hz > 0)) return false;
        std::snprintf(out, size, "%.1f → %.1f Hz", state.current_hz, state.note_slide_target_hz); return true;
    case 0xE5:
        if (state.fine_pitch_cents == 0) return false;
        std::snprintf(out, size, "%+d cents", state.fine_pitch_cents); return true;
    case 0xEA:
        if (!state.legato_enabled) return false;
        std::snprintf(out, size, "on"); return true;
    case 0xF5:
        if (value > 0 && value < XFM_MACRO_TARGET_COUNT)
        {
            if ((state.macro_disabled_mask & (1ull << value)) == 0) return false;
            std::snprintf(out, size, "target %d off", value); return true;
        }
        if (state.macro_disabled_mask == 0) return false;
        std::snprintf(out, size, "mask %llX off", (unsigned long long)state.macro_disabled_mask); return true;
    case 0xF6:
        if (value == 0)
        {
            int activeTargets = 0;
            for (int target = 1; target < XFM_MACRO_TARGET_COUNT; ++target)
                if (state.macro_states[target].active) activeTargets++;
            if (activeTargets == 0) return false;
            std::snprintf(out, size, "%d targets active", activeTargets); return true;
        }
        if (value > 0 && value < XFM_MACRO_TARGET_COUNT && state.macro_states[value].active)
        {
            std::snprintf(out, size, "target %d step %d", value, state.macro_states[value].pos); return true;
        }
        return false;
    case 0x30:
        if (!state.envelope_hard_reset) return false;
        std::snprintf(out, size, "on"); return true;
    case 0x10:
        if (probe->lfo_enable == tracker->songLfoEnabled && probe->lfo_freq == tracker->songLfoFrequency) return false;
        std::snprintf(out, size, "%s, freq %d", probe->lfo_enable ? "on" : "off", probe->lfo_freq); return true;
    default: return false;
    }
}

static inline void Tracker_EffectMemoryClosestNote(double hz, char *out, size_t size)
{
    if (!std::isfinite(hz) || hz <= 0.0)
    {
        std::snprintf(out, size, "—");
        return;
    }
    static constexpr const char *names[12] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
    // Label the nearest sounding pitch in standard concert-pitch octaves.
    const int midi = std::clamp((int)std::lround(69.0 + 12.0 * std::log2(hz / 440.0)), 0, 127);
    const double noteHz = 440.0 * std::pow(2.0, (midi - 69) / 12.0);
    const int cents = (int)std::lround(1200.0 * std::log2(hz / noteHz));
    if (cents == 0)
        std::snprintf(out, size, "%s%d", names[midi % 12], midi / 12 - 1);
    else
        std::snprintf(out, size, "%s%d%c.%02d", names[midi % 12], midi / 12 - 1,
            cents > 0 ? '+' : '-', std::abs(cents));
}

static inline bool Tracker_EffectMemoryHereNumber(
    const Tracker *tracker, const xfm_module *probe, uint8_t code, int target,
    uint8_t commandValue, int *number, char *detail, size_t detailSize)
{
    const int ch = tracker->editChannel;
    const XfmSongChannel &state = probe->active_song.channels[ch];
    if (Tracker_EffectMemoryIsPatchOpn(code))
    {
        if (!probe->live_patch_valid[ch]) return false;
        if (code == 0x60)
        {
            *number = probe->live_op_mask[ch] & 0x0F;
            std::snprintf(detail, detailSize, "operator mask");
            return true;
        }
        if (target > 0)
        {
            *number = Tracker_EffectMemoryOpnValue(probe->live_patches[ch], code, target - 1);
            std::snprintf(detail, detailSize, "OP%d", target);
            return true;
        }
        if (code == 0x19 || code == 0x56 || code == 0x5B || (code >= 0x50 && code <= 0x55))
            return false; // Four operator values have no single effective number.
        *number = Tracker_EffectMemoryOpnValue(probe->live_patches[ch], code, 0);
        return true;
    }
    switch (code)
    {
    case 0x01: case 0x02:
        *number = state.pitch_slide_speed;
        if (!probe->channel_active[ch])
        {
            std::snprintf(detail, detailSize, "key off; speed retained");
            return true;
        }
        {
            char note[12] = {};
            Tracker_EffectMemoryClosestNote(state.current_hz, note, sizeof(note));
            std::snprintf(detail, detailSize, "now %.1f Hz (%s)", state.current_hz, note);
        }
        return true;
    case 0x03:
        *number = state.portamento_speed;
        if (!probe->channel_active[ch])
            std::snprintf(detail, detailSize, "key off; speed retained");
        else if (state.portamento_active)
        {
            const double startHz = state.portamento_start_hz > 0.0 ?
                state.portamento_start_hz : state.current_hz;
            const double semitonesPerTick = state.portamento_speed / 32.0;
            const double totalTicks = std::abs(12.0 * std::log2(state.target_hz / startHz)) /
                semitonesPerTick;
            const double remainingTicks = std::abs(12.0 * std::log2(state.target_hz / state.current_hz)) /
                semitonesPerTick;
            const int tickRate = std::max(1, probe->song_patterns[1].tick_rate);
            const int durationMs = (int)std::lround(1000.0 * totalTicks / tickRate);
            const int remainingMs = (int)std::lround(1000.0 * remainingTicks / tickRate);
            char currentNote[12] = {}, startNote[12] = {}, targetNote[12] = {};
            Tracker_EffectMemoryClosestNote(state.current_hz, currentNote, sizeof(currentNote));
            Tracker_EffectMemoryClosestNote(startHz, startNote, sizeof(startNote));
            Tracker_EffectMemoryClosestNote(state.target_hz, targetNote, sizeof(targetNote));
            std::snprintf(detail, detailSize,
                "now %.1f Hz (%s)\nfrom %.1f Hz (%s)\nto %.1f Hz (%s)\nin %d ms; %d ms left",
                state.current_hz, currentNote, startHz, startNote,
                state.target_hz, targetNote, durationMs, remainingMs);
        }
        else if (state.current_hz > 0.0)
        {
            char note[12] = {};
            Tracker_EffectMemoryClosestNote(state.current_hz, note, sizeof(note));
            std::snprintf(detail, detailSize, "now %.1f Hz (%s)\n%s",
                state.current_hz, note, state.pitch_slide_speed != 0 ?
                "pitch slide overrides glide" : "place 03 on next note");
        }
        else std::snprintf(detail, detailSize, "waiting for first note");
        return true;
    case 0x04:
        *number = state.vibrato_depth;
        std::snprintf(detail, detailSize, "speed %d; ±%d cents", state.vibrato_speed, (int)std::lround(state.vibrato_depth * 127.0 * 100.0 / (16.0 * 128.0)));
        return true;
    case 0x07:
        *number = state.tremolo_depth;
        std::snprintf(detail, detailSize, "speed %d; 0–%d TL", state.tremolo_speed, state.tremolo_depth * 4);
        return true;
    case 0x08:
        *number = std::max(0, state.effect_pan);
        std::snprintf(detail, detailSize, "%s", state.effect_pan == 3 ? "left + right" :
            state.effect_pan == 2 ? "left" : state.effect_pan == 1 ? "right" : "muted");
        return true;
    case 0x0A:
        *number = state.volume_slide_speed == 0 ? 0 : state.current_volume;
        if (state.volume_slide_speed == 0)
            std::snprintf(detail, detailSize, "slide off");
        else std::snprintf(detail, detailSize, "volume of 127; %+d/4 ticks", state.volume_slide_speed);
        return true;
    case 0x0C: case 0xEC: case 0xED: case 0xFC:
        *number = commandValue;
        std::snprintf(detail, detailSize, "%s", code == 0x0C && commandValue == 0 ? "repeat off" :
            code == 0x0C ? "repeat interval in ticks" : "tick in this row");
        return true;
    case 0xE5:
        *number = state.fine_pitch_cents;
        std::snprintf(detail, detailSize, "cents");
        return true;
    case 0xEA:
        *number = state.legato_enabled ? 1 : 0;
        std::snprintf(detail, detailSize, "%s", state.legato_enabled ? "on" : "off");
        return true;
    case 0x10:
        *number = probe->lfo_freq;
        std::snprintf(detail, detailSize, "LFO %s", probe->lfo_enable ? "on" : "off");
        return true;
    case 0x30:
        *number = state.envelope_hard_reset ? 1 : 0;
        std::snprintf(detail, detailSize, "%s", state.envelope_hard_reset ? "on" : "off");
        return true;
    default: return false;
    }
}

static inline void Tracker_AnalyzeEffectMemory(Tracker *tracker, const GameSoundSystem *sound)
{
    if (!tracker) return;
    tracker->effectMemoryCount = 0;
    tracker->effectMemoryUnavailable = false;
    tracker->effectMemoryDirty = false;
    const bool looped = tracker->loopEnabled;
    const int loopStart = std::max(0, std::min(tracker->loopStart, tracker->rowCount - 1));
    const int loopEnd = std::max(loopStart, std::min(tracker->loopEnd, tracker->rowCount - 1));
    const bool selectionOverride = looped && Tracker_ShouldUseSelectionPlaybackOverride(tracker);
    if ((looped && (tracker->editRow < loopStart || tracker->editRow > loopEnd)) ||
        (!selectionOverride && !Tracker_RowEnabledForPlayback(tracker, tracker->editRow)))
    {
        tracker->effectMemoryUnavailable = true;
        return;
    }

    const bool solo = tracker->channelSelectionEnabled;
    const int firstChannel = solo ? tracker->channelStart : 0;
    const int lastChannel = solo ? tracker->channelEnd : TRACKER_CHANNELS - 1;
    const int loopRows = loopEnd - loopStart + 1;
    TrackerEffectMemoryOrigin origins[256][5] = {};
    int lastPatchRow = -1;
    int channelPatch[TRACKER_CHANNELS];
    std::fill(channelPatch, channelPatch + TRACKER_CHANNELS, -1);
    auto recordRow = [&](int row, int playbackRow, bool globalOnly)
    {
        for (int ch = 0; ch < TRACKER_CHANNELS; ++ch)
        {
            if (ch < firstChannel || ch > lastChannel) continue;
            const char *cell = tracker->cells[row][ch].text;
            const int patch = Tracker_ParseCellInstrument(cell);
            if (patch >= 0)
            {
                if (!globalOnly && ch == tracker->editChannel && channelPatch[ch] != patch)
                    lastPatchRow = row;
                channelPatch[ch] = patch;
            }
            for (int slot = 0; slot < TRACKER_MAX_EFFECT_SLOTS; ++slot)
            {
                const int pos = 7 + slot * 4;
                if (!Tracker_IsHex(cell[pos]) || !Tracker_IsHex(cell[pos + 1]) ||
                    !Tracker_IsHex(cell[pos + 2]) || !Tracker_IsHex(cell[pos + 3])) break;
                const int code = Tracker_ParseHexByte(cell + pos);
                const int value = Tracker_ParseHexByte(cell + pos + 2);
                if (code < 0 || value < 0 || code >= 256) continue;
                if ((globalOnly && code != 0x10) ||
                    (ch != tracker->editChannel && code != 0x10)) continue;
                // The sequencer ignores OPN commands on a channel without a patch.
                if (code == 0x10 && channelPatch[ch] < 0) continue;
                if (code == 0x10 && ch != tracker->editChannel)
                {
                    // The LFO is chip-global. Later channels on the same row win.
                    origins[code][0] = {row, playbackRow, (uint8_t)value};
                    continue;
                }
                int target = Tracker_EffectMemoryTarget((uint8_t)code, (uint8_t)value);
                if (target > 4) continue;
                origins[code][target] = {row, playbackRow, (uint8_t)value};
            }
        }
    };

    int targetPlaybackRow = 0;
    if (looped)
    {
        // Channel state resets when the tracker loop wraps. The chip LFO does
        // not, so retain its last command from the preceding iteration.
        for (int row = loopStart; row <= loopEnd; ++row)
            recordRow(row, row - loopStart, true);
        const bool previousLoopLfo = origins[0x10][0].row >= 0;
        std::fill(channelPatch, channelPatch + TRACKER_CHANNELS, -1);
        const int passOffset = previousLoopLfo ? loopRows : 0;
        for (int row = loopStart; row < tracker->editRow; ++row)
            recordRow(row, passOffset + row - loopStart, false);
        targetPlaybackRow = passOffset + tracker->editRow - loopStart;
    }
    else
    {
        int playbackRow = 0;
        for (int row = 0; row < tracker->editRow; ++row)
        {
            if (!Tracker_RowEnabledForPlayback(tracker, row)) continue;
            recordRow(row, playbackRow++, false);
        }
        targetPlaybackRow = playbackRow;
    }
    recordRow(tracker->editRow, targetPlaybackRow, false);

    static constexpr int probeRate = 11025;
    xfm_module *probe = xfm_module_create(probeRate, 512, XFM_CHIP_YM3438);
    if (!probe) { tracker->effectMemoryUnavailable = true; return; }
    // Snapshot fallback patches quickly; all probe construction and synthesis
    // happen after the audio callback is unlocked.
    xfm_patch_opn fallbackPatches[256] = {};
    bool fallbackPresent[256] = {};
    if (sound && sound->audioDev) SDL_LockAudioDevice(sound->audioDev);
    if (sound && sound->musicModule)
        for (int inst = 0; inst < 256; ++inst)
            if (!tracker->editPatchValid[inst] && sound->musicModule->patch_present[inst])
            {
                fallbackPatches[inst] = sound->musicModule->patches[inst];
                fallbackPresent[inst] = true;
            }
    if (sound && sound->audioDev) SDL_UnlockAudioDevice(sound->audioDev);
    for (int inst = 0; inst < 256; ++inst)
    {
        if (tracker->editPatchValid[inst])
            xfm_patch_set(probe, inst, &tracker->editPatches[inst], sizeof(xfm_patch_opn), XFM_CHIP_YM3438);
        else if (fallbackPresent[inst])
            xfm_patch_set(probe, inst, &fallbackPatches[inst], sizeof(xfm_patch_opn), XFM_CHIP_YM3438);
    }
    int macroId = 0;
    for (int inst = 0; inst < 256 && macroId < XFM_MAX_MACROS; ++inst)
    {
        if (!probe->patch_present[inst]) continue;
        for (int target = XFM_MACRO_TL1; target < XFM_MACRO_TARGET_COUNT && macroId < XFM_MAX_MACROS; ++target)
        {
            if (!tracker->editMacroEnabled[inst][target] || !tracker->editMacroValid[inst][target]) continue;
            XfmMacro macro = tracker->editMacros[inst][target];
            macro.target = (uint8_t)target;
            Tracker_NormalizeMacroUiState(&macro);
            if (macro.length == 0) continue;
            if (xfm_macro_set(probe, macroId, &macro) >= 0)
                xfm_patch_macro_set(probe, inst, (uint8_t)target, macroId++);
        }
    }
    xfm_module_set_lfo(probe, tracker->songLfoEnabled, tracker->songLfoFrequency);
    xfm_module_set_tuning(probe, (xfm_tuning_mode)tracker->songTuningMode, tracker->songScaleRoot);
    const std::string pattern = looped ?
        Tracker_BuildSongRangePatternText(tracker, loopStart, loopEnd, solo, firstChannel, lastChannel) :
        Tracker_BuildFlatPatternText(tracker, solo, firstChannel, lastChannel);
    const int tickRate = std::max(1, tracker->songTickRate);
    const int speed = std::max(1, tracker->songSpeed);
    if (xfm_song_declare(probe, 1, pattern.c_str(), tickRate, speed) < 0)
    {
        tracker->effectMemoryUnavailable = true;
        xfm_module_destroy(probe);
        return;
    }
    xfm_song_play(probe, 1, looped);
    if (looped)
    {
        xfm_song_set_loop_range(probe, 0, loopRows - 1);
        xfm_song_set_loop_reset_state(probe, true);
    }
    const int samplesPerRow = probe->song_patterns[1].samples_per_row;
    // Advance into this row so its scheduled commands are reflected in HERE.
    int64_t frames = (int64_t)targetPlaybackRow * samplesPerRow + 1;
    int16_t scratch[2048 * 2] = {};
    while (frames > 0)
    {
        int chunk = (int)std::min<int64_t>(frames, 2048);
        xfm_mix_song(probe, scratch, chunk);
        frames -= chunk;
    }

    for (int defIndex = 1; defIndex < TRACKER_EFFECT_DEF_COUNT; ++defIndex)
    {
        const TrackerEffectDef &def = TRACKER_EFFECT_DEFS[defIndex];
        const uint8_t code = def.code;
        const bool rowOnly = code == 0x0C || code == 0xEC || code == 0xED || code == 0xFC;
        for (int target = 0; target <= 4; ++target)
        {
            const TrackerEffectMemoryOrigin &origin = origins[code][target];
            if (origin.row < 0) continue;
            const bool current = origin.row == tracker->editRow;
            if (rowOnly && !current) continue;
            if (Tracker_EffectMemoryIsPatchOpn(code) && origin.row < lastPatchRow) continue;
            char effective[80] = {};
            bool active = rowOnly ? false : code == 0x10 || code == 0x30 || !Tracker_EffectMemoryIsOpn(code)
                ? Tracker_EffectMemoryFormatControl(tracker, probe, code, origin.value, effective, sizeof(effective))
                : Tracker_EffectMemoryFormatOpn(probe, tracker->editChannel, code, target,
                    !current, effective, sizeof(effective));
            if (current && rowOnly)
            {
                if (code == 0x0C)
                {
                    if (origin.value == 0) std::snprintf(effective, sizeof(effective), "off");
                    else std::snprintf(effective, sizeof(effective), "every %d ticks", origin.value);
                }
                else std::snprintf(effective, sizeof(effective), "tick %d", origin.value);
                active = true;
            }
            if (current && !active)
            {
                if (code == 0x10)
                    std::snprintf(effective, sizeof(effective), "%s, freq %d",
                        probe->lfo_enable ? "on" : "off", probe->lfo_freq);
                else if (Tracker_EffectMemoryIsPatchOpn(code))
                    std::snprintf(effective, sizeof(effective), "no patch");
                else
                    std::snprintf(effective, sizeof(effective), "%s", code == 0xF6 ? "no macro" : "off");
                active = true;
            }
            if (!active || tracker->effectMemoryCount >= TRACKER_EFFECT_MEMORY_MAX_ROWS) continue;
            TrackerEffectMemoryRow &entry = tracker->effectMemoryRows[tracker->effectMemoryCount++];
            entry = TrackerEffectMemoryRow{};
            entry.code = code;
            entry.initialValue = origin.value;
            Tracker_EffectMemoryPatchBase(probe, tracker->editChannel, code, target,
                entry.patchValue, sizeof(entry.patchValue));
            entry.sourceRow = origin.row;
            entry.rowsAgo = targetPlaybackRow - origin.playbackRow;
            if (target > 0)
                std::snprintf(entry.effect, sizeof(entry.effect), "%02X %s OP%d", code, def.name, target);
            else
                std::snprintf(entry.effect, sizeof(entry.effect), "%02X %s", code, def.name);
            for (char *letter = entry.effect; *letter; ++letter)
                if (*letter >= 'a' && *letter <= 'z') *letter = char(*letter - 'a' + 'A');
            std::snprintf(entry.effective, sizeof(entry.effective), "%s", effective);
            entry.hereValueValid = Tracker_EffectMemoryHereNumber(
                tracker, probe, code, target, origin.value, &entry.hereValue,
                entry.hereDetail, sizeof(entry.hereDetail));
            const TrackerEffectMemoryRule rule = Tracker_EffectMemoryRule(code);
            entry.continueMode = rule.continueMode;
            entry.until = rule.until;
        }
    }
    xfm_module_destroy(probe);
}
