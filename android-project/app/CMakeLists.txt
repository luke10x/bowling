cmake_minimum_required(VERSION 3.22)

project(main LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(GAME_ROOT ${CMAKE_SOURCE_DIR}/../..)

set(GAME_SRC ${GAME_ROOT})
set(THIRDPARTY ${GAME_ROOT}/3rdparty)

include(FetchContent)
FetchContent_Declare(
        stb
        GIT_REPOSITORY https://github.com/nothings/stb.git
        GIT_TAG master
)
FetchContent_MakeAvailable(stb)

FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG 1a31e31ae
)
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(
        jolt
        GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
        GIT_TAG v5.4.0
)
FetchContent_MakeAvailable(jolt)


# Clay requires CMake v3.27, we don't have that in Android yet
# So cannot just manage Clay dependency like this
#FetchContent_Declare(
#        clay
#        GIT_REPOSITORY https://github.com/luke10x/clay.git
#        GIT_TAG bugfix/gles3-renderer-measure-text-to-use-correct-font
#)
#FetchContent_MakeAvailable(clay)

# Add source files
add_library(main SHARED
    ${GAME_SRC}/android-project/android_main.cpp
    
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

# Include paths
target_include_directories(main PRIVATE
    ${GAME_SRC}
    ${GAME_SRC}/../clay # Too bad I cannot use FetchContent for clay
    ${THIRDPARTY}/glm
    ${THIRDPARTY}/SDL/include
    ${stb_SOURCE_DIR}
    ${imgui_SOURCE_DIR}
    ${jolt_SOURCE_DIR}
)

# Build libraries
add_subdirectory(${THIRDPARTY}/SDL SDL_build)
add_subdirectory(${jolt_SOURCE_DIR}/Build jolt_build)

# Link libraries
target_link_libraries(main
    Jolt
    SDL2
    GLESv3
    OpenSLES
    log
    android
)