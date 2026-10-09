cmake_minimum_required(VERSION 3.16)
project(fritz_reborn C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# SDL3: use an existing installation (e.g. -DCMAKE_PREFIX_PATH=build/deps/prefix),
# otherwise download and build it.
find_package(SDL3 CONFIG QUIET)
if(NOT SDL3_FOUND)
    include(FetchContent)
    FetchContent_Declare(SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.2.8
        GIT_SHALLOW TRUE)
    set(SDL_TESTS OFF CACHE BOOL "" FORCE)
    set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(SDL3)
endif()

add_executable(fritz
    src/main.c
    src/gui/screen.c
    src/gui/gfx.c
    src/gui/desktop.c
    src/gui/layout.c
    src/gui/font.c
    src/gui/board.c
    src/gui/sprites.c
    src/game/game_board.c
    src/gui/ui_text.c
    src/game/language.c
    src/assets/exe.c
    src/game/settings.c
    src/gui/palette.c
    src/platform/sdl.c)
target_include_directories(fritz PRIVATE include)
target_link_libraries(fritz PRIVATE SDL3::SDL3)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(fritz PRIVATE -Wall -Wextra -Wpedantic)
endif()
