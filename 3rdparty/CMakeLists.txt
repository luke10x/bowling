cmake_minimum_required(VERSION 3.20)

project(fetch_dependenties NONE)

include(FetchContent)

FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
)
FetchContent_MakeAvailable(stb)

FetchContent_Declare(
    clay
    GIT_REPOSITORY https://github.com/luke10x/clay.git
    GIT_TAG gles3-renderer
)
FetchContent_Populate(clay)

# NOTE: only header only libraries should be instaleld into this _deps,
# Arch specific Makefiles should use different CMakeLists.txt