## Qwen Added Memories
- Emscripten builds fail with C++ redefinition errors when including .cpp files directly (like xfm_impl.cpp). Always use only .h/.api includes and avoid .cpp includes in header files for Emscripten compatibility.
- The game uses `adaptive_audio.h` for an adaptive audio quality system that monitors FPS and offers WAV caching if performance is low. It's integrated into `game.cpp` via the `AdaptiveAudioSystem` struct in `UserContext`.
- `game.cpp` contains the main game loop in `vtx::loop()`, which renders one frame per call. The game is strictly single-threaded when built for Emscripten (no `USE_PTHREADS` in Makefile.emscripten, and the frame-rate limiting sleep code is disabled via `#ifndef __EMSCRIPTEN__`).
- The adaptive audio system has a yieldable WAV export state machine (`AdaptiveAudio_ExportWAV`) designed to be called every frame until completion, which is necessary for Emscripten's single-threaded environment to avoid blocking the main thread.
