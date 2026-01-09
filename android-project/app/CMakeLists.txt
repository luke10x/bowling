cmake_minimum_required(VERSION 3.22)

project(main LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Root of your real game ----
set(GAME_ROOT ${CMAKE_SOURCE_DIR}/../..)

set(GAME_SRC ${GAME_ROOT})
set(THIRDPARTY ${GAME_ROOT}/3rdparty)

# ---- Library ----
add_library(main SHARED
    ${GAME_SRC}/android-project/app/jni/src/android_gaslight.cpp
)

# ---- Includes ----
target_include_directories(main PRIVATE
    ${GAME_SRC}
    ${THIRDPARTY}/glm
    ${THIRDPARTY}/SDL/include
)

# ---- SDL ----
#add_library(SDL2 SHARED IMPORTED)
#set_target_properties(SDL2 PROPERTIES
#    IMPORTED_LOCATION
#      ${THIRDPARTY}/SDL/libs/${ANDROID_ABI}/libSDL2.so
#)

# Build SDL2 from source
add_subdirectory(${THIRDPARTY}/SDL SDL_build)

# ---- Link ----
target_link_libraries(main
    SDL2
    GLESv3
    OpenSLES
    log
    android
)