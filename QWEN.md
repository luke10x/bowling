## Qwen Added Memories
- Emscripten builds fail with C++ redefinition errors when including .cpp files directly (like xfm_impl.cpp). Always use only .h/.api includes and avoid .cpp includes in header files for Emscripten compatibility.
- The game uses `adaptive_audio.h` for an adaptive audio quality system that monitors FPS and offers WAV caching if performance is low. It's integrated into `game.cpp` via the `AdaptiveAudioSystem` struct in `UserContext`.
- `game.cpp` contains the main game loop in `vtx::loop()`, which renders one frame per call. The game is strictly single-threaded when built for Emscripten (no `USE_PTHREADS` in Makefile.emscripten, and the frame-rate limiting sleep code is disabled via `#ifndef __EMSCRIPTEN__`).
- The adaptive audio system has a yieldable WAV export state machine (`AdaptiveAudio_ExportWAV`) designed to be called every frame until completion, which is necessary for Emscripten's single-threaded environment to avoid blocking the main thread.
- **Audio system has two modes** (in `sounds.h`, `GameSoundSystem`):
  - **Synth mode** (default): Real-time YM3438 OPN chip synthesis using `xfm_module` (musicModule + sfxModule). Uses `SYNTH_BUFFER_SIZE = 256` for low-latency playback. Patches are loaded and songs/SFX are synthesized on-the-fly in the SDL audio callback. Preserves original OPN chip behavior with natural LFO phase and patch changes. No preload needed, but requires more CPU power.
  - **Cached mode** (fallback for low-end devices): Pre-generated WAV blobs played back from memory using `xfm_wav_module` (wavMusicModule + wavSfxModule). Uses configurable `WAV_PLAYBACK_BUFFER_SIZE` (default 1024). WAVs are generated to memory at startup via `AdaptiveAudio_ExportWAV` (yieldable state machine), then loaded into WAV modules via `xfm_wav_load_memory`. Takes a couple minutes to cache all audio upfront, but playback is much lighter on CPU and runs on low-end devices. Switching modes requires async audio restart (pause → destroy modules → grace period → re-init).
  - **Terminology**: "Synth" = real-time chip synthesis. "Cached" = pre-generated in-memory WAV blobs. The multi-minute generation process is called "Caching Audio" (not "export" or "render" — those imply files leaving the game or graphics). Code still uses `useWavPlayback` flag and `xfm_export_*` function names for historical reasons, but user-facing text uses Synth/Cached.
- **UI rendering in Emscripten**: Since Emscripten is strictly single-threaded, long blocking actions prevent UI from being drawn until the action completes. Logs (printf) print immediately, but Clay UI rendering is deferred. Solution: break long operations into yieldable state machines (like `AdaptiveAudio_ExportWAV`) so UI can render each frame during the operation. **Pattern: move state updates to the beginning of the loop** so UI is drawn with updated state each frame, rather than being blocked by work at the end.
- **Yieldable WAV export**: The original `xfm_export_song_to_memory` and `xfm_export_sfx_to_memory` blocked for the entire song/SFX render. Added yieldable versions (`xfm_export_song_begin/step/finalize/cleanup`, `xfm_export_sfx_begin/step/finalize/cleanup`) in `xfm_export.cpp` that render in chunks of `XFM_EXPORT_YIELD_SAMPLES` (default 4410 samples = 100ms at 44100Hz). The `AdaptiveAudio_ExportWAV` state machine now has BEGIN→STEP→FINALIZE phases per song/SFX, yielding each frame during STEP phase so UI stays responsive. Configurable via `ADAPTIVE_AUDIO_EXPORT_YIELD_SAMPLES` constant.
- **CRITICAL: YM3438 chip export must use buffer_frames (256) chunks internally**. The original `render_song_to_buffer` called `xfm_mix_song(m, buf, frames_per_chunk)` where `frames_per_chunk = m->buffer_frames` (256) in a tight loop. When making yieldable export, calling `xfm_mix_song` with large chunks (e.g., 4410) in a single call causes missing notes — the YM3438 chip's internal state (phase accumulators, envelope generators, LFO, row boundary detection in `update_song`) expects to be advanced in 256-sample increments. **Fix**: yieldable `xfm_export_song_step` and `xfm_export_sfx_step` internally loop with `buffer_frames` (256) chunks per `xfm_mix_song`/`xfm_mix_sfx` call, but still return control to caller after `samples_per_chunk` (4410) total samples rendered. This preserves chip state progression while still yielding to the game loop.
- **Clay UI does NOT render emoji**. All emoji characters (🎹, 🎵, 🔇, ⚠️) in `CLAY_TEXT()` calls are silently ignored. Use plain text only for Clay UI strings.
- **Clay stores references to strings, not copies**. When passing a `Clay_String` to `CLAY_TEXT`, the `.chars` pointer must remain valid until the next frame's render. Never use local stack variables (e.g., `char msg[128]`) — store strings in the struct (e.g., `self->fpsMessage[128]`) so they persist across frames.
- **Adaptive audio startup flow**: Game starts in Synth mode but muted (volume=0) during FPS monitoring period. After monitoring completes: if FPS is good → unmute; if FPS is low → show modal with measured FPS value, keep muted until user decides. This prevents audible audio during the measurement phase.
- **Adaptive audio relies on fpsCounter for FPS data**. It does NOT do its own accumulation/averaging. `FpsCounter` already accumulates over 5 seconds and stores result in `fps`. Adaptive audio just reads `fpsCounter.fps` after the monitoring duration elapses and `fps > 0`.
- **Songs get fresh YM3438 modules per export** to avoid state leakage (phase, envelopes, LFO) between songs — this was fixed in commit 845ff55. SFX uses a persistent module with `xfm_module_reset_state()` before each export.
- **Sound settings panel stays open during quality changes**. The `wavExportInProgress` flag should NOT hide the panel — the caching progress overlay renders as a full-screen modal on top, so the panel underneath is visually obscured anyway.
# 🔥 Hot-Reload Safety Guidelines (macOS)

> ⚠️ **Add this to your QWEN.md to prevent future hot-reload breakage**

---

## 🚫 Avoid These Patterns in Header-Only Code

| Pattern | Why It Breaks Hot-Reload | Safe Alternative |
|---------|---------------------------|-----------------|
| `#define GLM_ENABLE_EXPERIMENTAL` | Can pull in unstable GLM headers that change layout/ABI | Use only core GLM + `gtc/` headers; avoid `gtx/` unless absolutely required |
| `inline void func() { ... }` in headers | Causes ODR violations / multiple definitions across reload boundaries | Remove `inline`; move definitions to `.cpp` long-term, or leave declaration-only in header |
| `static constexpr float X = 1.0f;` in structs | Values are baked into calling code at compile time → won't update on reload | Use `static inline const float X = 1.0f;` (C++17) for single-definition, runtime-patchable linkage |
| `thread_local std::mt19937 rng{...};` | Thread-local state persists unpredictably across reloads → desynced RNG/state | Use deterministic sequence counters (`static unsigned idx++`) or pass RNG explicitly |
| Functions defined *inside* class bodies | Implicitly `inline` → same ODR issues as explicit `inline` | Declare in class, define in `.cpp`; or if header-only, ensure no stateful/patch-sensitive logic |
| Heap allocation in hot-path update logic | Can cause memory leaks or double-free if allocator state desyncs | Prefer fixed-size `std::array`/pool allocators; avoid `new`/`std::vector` in per-frame update |

---

## ✅ Hot-Reload Friendly Checklist (macOS/dyld)

Before suggesting code changes, verify:

- [ ] **No `constexpr` for tunable gameplay values** → use `static inline const`
- [ ] **No `inline` on function definitions** in headers (declarations only)
- [ ] **No `thread_local`** for game state, RNG, or caches
- [ ] **No GLM experimental headers** unless explicitly approved
- [ ] **Struct layout unchanged** if hot-reloading without restart (no reordering fields)
- [ ] **No static counters with side effects** that assume single initialization (use `static bool initialized` guard)
- [ ] **All new state is self-contained** in the reloaded module (no hidden cross-module dependencies)

---

## 🧩 Example: Hot-Reload Safe Config Struct

```cpp
// ✅ GOOD
struct GameConfig {
    static inline const float PLAYER_SPEED = 5.0f;      // patchable
    static inline const int MAX_ENTITIES = 100;         // patchable
    
    void update(float deltaTime);  // declaration only
};

// ❌ BAD
struct GameConfig {
    static constexpr float PLAYER_SPEED = 5.0f;  // baked, won't reload
    static inline int counter = 0;               // ODR risk if defined in header
    inline void update(float deltaTime) { ... }  // implicit inline → ODR risk
};
```

---

## 🔄 When in Doubt

1. **Prefer declarations in headers, definitions in `.cpp`** — even if it feels verbose.
2. **If header-only is required**, document which functions are "hot-reload sensitive" and avoid stateful logic there.
3. **Test reloads after every structural change** — macOS `dlopen`/`dyld` is strict about symbol resolution.
4. **When adding new features**, ask: *"If this value changed, should it take effect immediately on reload?"* If yes → not `constexpr`.

---

## 🪙 CoinLane-Specific Notes (for this project)

- `CoinFlyConfig` values must use `static inline const` (not `constexpr`)
- `CoinLane::flyAnimations` is a fixed `std::array` — do not replace with `std::vector` without pool allocator
- `Coin::flyTriggered` flag is essential for 2D/3D render sync — do not remove
- `getRandomCoinPattern()` replaced with deterministic `getNextCoinPattern()` — do not reintroduce `thread_local` RNG

---

> 💡 **Pro Tip**: When reviewing generated code, search for `constexpr`, `inline {`, `thread_local`, and `#define GLM` — these are the most common hot-reload footguns on macOS.

---

*Last updated: Based on fixes from CoinLane multi-animation + hot-reload thread, April 2026*
