## Qwen Added Memories
- Emscripten builds fail with C++ redefinition errors when including .cpp files directly (like xfm_impl.cpp). Always use only .h/.api includes and avoid .cpp includes in header files for Emscripten compatibility.
