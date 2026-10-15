# Audio Performance Notes

This document records performance findings for the Emscripten build and the
real-time `eggsfm` / YMFM synth path. It intentionally does not specify the
event-driven sequencing refactor; that design is still open for discussion.

## Current Baseline

The main bottleneck found so far was the wasm build compiling game code, YMFM,
`eggsfm`, ImGui, WAV data objects, and Jolt with `-O0`. Moving the release build
to `-O3 -flto` produced the largest observed improvement.

Microbenchmark context:

- Benchmark target: `eggsfm` music module plus SFX module, compiled to wasm and
  run under Node with the local `../emsdk`.
- Buffer size: 2048 stereo frames.
- Busy SFX case: six SFX voices active/retriggering.

Observed wasm benchmark results:

| Case | `-O0` wasm | `-O3 -flto` wasm |
| --- | ---: | ---: |
| Music only | ~2.888 ms/callback | ~0.385 ms/callback |
| SFX idle | ~1.136 ms/callback | ~0.169 ms/callback |
| SFX retriggered | ~1.373 ms/callback | ~0.190 ms/callback |
| SFX six voices | ~2.509 ms/callback | ~0.300 ms/callback |
| Music + SFX | ~4.678 ms/callback | ~0.598 ms/callback |

## Build Optimizations Already Applied

`Makefile.emscripten` now has `BUILD=release` and `BUILD=debug` modes.

Release mode:

- Uses `-O3 -flto`.
- Disables Emscripten assertions.
- Builds Jolt with matching release defines.
- Disables Jolt's Release-default debug renderer and profiler.
- Builds Jolt, ImGui, and WAV data objects with matching optimization flags.

Debug mode:

- Uses `-O0 -g3`.
- Keeps Jolt asserts, profiler, and debug renderer defines.
- Enables Emscripten assertions.

Important Jolt note: the app and `libJolt.a` must be compiled with matching Jolt
defines. Mismatches such as `JPH_DEBUG_RENDERER` or `JPH_PROFILE_ENABLED` on only
one side cause Jolt to abort at startup.

## Potential Future Optimizations

### Precompute Note Frequency / FNUM Tables

Current note triggering computes pitch from MIDI notes using floating-point math
and `pow()`. A small table can map MIDI note numbers directly to the YM register
values needed by the OPN chip.

Expected effect:

- Small average CPU win.
- Better worst-case behavior when many SFX notes trigger in the same callback.
- Low risk if verified against the current tuning.

Suggested shape:

- Precompute for the supported MIDI note range at module creation.
- Store block/FNUM or the exact A4/A0 register payload.
- Replace note-trigger `pow()` calls with table lookup.

### Precompute Patch Register Payloads

`load_patch()` converts `xfm_patch_opn` fields into a series of YM register
writes every time a patch is loaded. Most of that conversion is static for a
given patch.

Expected effect:

- Small to moderate improvement during dense note/SFX triggering.
- Mostly reduces event spikes rather than continuous chip generation cost.

Suggested shape:

- Convert each patch to a compact register-write list when `xfm_patch_set()` is
  called.
- At note start, replay the prepacked register writes.
- Keep a small adjustment path for volume changes that affect carrier TL values.

Open question:

- Volume is currently applied by copying the patch and adjusting carrier TL.
  Prepacking should either create volume variants or patch only the affected TL
  registers at note time.

### Try Wasm SIMD

Add an optional Makefile flag for `-msimd128` and measure on real browser
targets.

Expected effect:

- May help Jolt, GLM math, and some compiler-generated loops.
- YMFM benefit is uncertain unless the compiler can vectorize useful pieces.

Suggested shape:

- Add a switch such as `SIMD=1`.
- When enabled, append `-msimd128` to compile and link flags.
- Test on target browsers/devices before making it default.

Risk:

- Older browsers or embedded webviews may not support wasm SIMD.

### WebGL Release Flags

If the Emscripten build is intended to require WebGL2, the build can declare
that explicitly:

- `-s MIN_WEBGL_VERSION=2`
- `-s MAX_WEBGL_VERSION=2`

Potentially test:

- `-s GL_UNSAFE_OPTS=1`

Expected effect:

- Could reduce GL compatibility-layer overhead.
- Mostly rendering-side, not audio-side.

Risk:

- Can change compatibility or expose assumptions in the renderer. This should be
  tested separately from audio changes.

### Avoid Generating SFX Chip When Truly Idle

The gameplay path often uses SFX heavily, so this is not the main gameplay win.
Still, when no SFX voices are active, the callback can skip generating the SFX
chip and skip the SFX mix/add pass.

Expected effect:

- Saves CPU in menus, quiet gameplay moments, and between SFX bursts.
- Little effect during active gameplay if SFX are almost always playing.

Suggested shape:

- Add an `xfm_sfx_has_active()` query or expose an active voice count.
- In the audio callback, skip `xfm_mix_sfx()` and clear/avoid the temp SFX
  buffer when no SFX voices are active.

### Binary Song / SFX Data

The current Furnace-style text is parsed during declaration, not continuously
while the song plays. Therefore binary pattern data is not expected to be a major
real-time CPU win by itself.

Expected effect:

- Faster initialization and song switching.
- Less allocation and parsing work when redeclaring songs.
- Cleaner asset pipeline if generated alongside Furnace exports.

Risk:

- Adds tooling/pipeline complexity.
- Does not address the continuous YMFM chip generation cost.

### Disable C++ Exceptions In Wasm Release

Emscripten currently uses disabled exception handling by default unless exception
support is explicitly enabled. The game code has some asset-loading throws and a
few catches, so this should be verified carefully before making it explicit.

Potential flag:

- `-fno-exceptions`

Expected effect:

- Smaller code and less exception metadata if anything currently pulls it in.

Risk:

- Any actually-thrown exception in included wasm code will terminate rather than
  being catchable. Use only after checking the relevant asset-loading paths.

### Avoid Building Unused Jolt Sample Targets

The Emscripten Jolt build currently links only `libJolt.a` into the game, but
the Jolt CMake project may still build sample or benchmark executables depending
on its target options.

Potential CMake options:

- `-DTARGET_HELLO_WORLD=OFF`
- `-DTARGET_PERFORMANCE_TEST=OFF`

Expected effect:

- Build-time only.
- No runtime CPU improvement.
- Less wasted work when rebuilding Jolt after changing flags.

## Things That Are Probably Not Worth Doing First

### Terser

Terser can reduce JavaScript glue size, but the hot work is in `index.wasm`.
It is not expected to materially improve YMFM/Jolt/game CPU usage.

### Lowering Output Sample Rate Alone

`eggsfm` advances YMFM at the chip's native rate and resamples to the output
rate. Lower output sample rates can reduce some buffer/mix/sequencer work, but
do not proportionally reduce the core YMFM generation cost.

## Web audio safety

`wasm/audio_safety.js` is linked with `--pre-js`. Each SDL output is routed
through a gain node before reaching the speakers. The game loop renews a
1.5-second deadline (at most four times per second); without another frame,
the audio engine fades the gain to zero over 50 ms. An audio callback or JS
interval must not renew this deadline: those could keep running after gameplay
has stopped. Gain automation can run while the main JS thread is blocked.

PANIC disconnects this output before rebuilding SDL and synth state. Page exit
and freeze disconnect it too; hiding the page mutes it immediately. Wasm aborts,
audio callback exceptions, uncaught JS errors, and unhandled promise rejections
also disconnect and close the context without calling back into Wasm. Fatal
errors require a reload. Callback output is cleared before synthesis and again
on an exception so a partial/previous buffer is not left behind.

Run `node --test tests/test_web_audio_safety.cjs` for the deadline, callback
failure, cleanup/restart, and lifecycle regression tests. On a real device,
check foreground playback, background/relaunch, OFF/SYNTH/PANIC, and a main
thread stall longer than two seconds. The guard limits stuck audio; it does
not diagnose or fix the underlying gameplay hang, nor can it control audio
owned by another page or a broken browser/OS audio session. The service worker
only caches assets and has no playback code.

## Runtime logging

Keep normal gameplay quiet on stdout/stderr. FPS still updates the on-screen
counter, but does not print periodically. Do not add per-frame, per-second,
input, throw, score, or song traces. Log actionable events such as automatic
buffer increases, screen size changes, recovery completion/failure, invalid state, and resource
errors. Audio initialization/restarts should not dump each internal step.
