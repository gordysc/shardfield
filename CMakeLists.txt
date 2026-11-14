cmake_minimum_required(VERSION 3.24)
project(asteroids LANGUAGES C CXX)

# Compile as C++20, and fail rather than quietly fall back to an older
# standard if the compiler doesn't support it.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Write compile_commands.json into the build folder. Tools like clangd and
# clang-tidy read it to learn how each file is compiled.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Fetch raylib at configure time (no system install needed).
include(FetchContent)
# Skip raylib's example programs and games; we only need the library.
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
    GIT_SHALLOW TRUE
)
# raylib's CMakeLists.txt asks for a minimum CMake version below 3.10, which
# makes newer CMake print a deprecation warning meant for raylib's own
# developers. CMAKE_POLICY_VERSION_MINIMUM (CMake 4.0+) tells CMake to treat
# that request as 3.10 instead, so the warning goes away. It is set only
# while raylib is set up, then removed so it doesn't affect anything else.
set(CMAKE_POLICY_VERSION_MINIMUM 3.10)
FetchContent_MakeAvailable(raylib)
unset(CMAKE_POLICY_VERSION_MINIMUM)

# Fetch EnTT, the ECS library this branch uses instead of the hand-made
# Registry on `main`. EnTT is header-only: there is nothing to compile, and
# linking its target only adds its include folder.
FetchContent_Declare(
    EnTT
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG v3.16.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(EnTT)

# Build raylib's own source files with compiler warnings off (-w). They are
# third-party code we don't change, so the build log only shows warnings
# from this project's code.
target_compile_options(raylib PRIVATE -w)

# Everything except main() goes into a static library, `asteroids_core`. The
# game and the tests both link it, so each source file is compiled once and
# the tests check exactly the code the game runs. Only .cpp files are listed:
# each one is compiled on its own, and headers are compiled as part of every
# .cpp file that includes them.
add_library(asteroids_core STATIC
    src/draw.cpp
    src/ecs.cpp
    src/game.cpp
    src/input.cpp
    src/random.cpp
    src/render.cpp
    src/rock.cpp
    src/spawn.cpp
    src/systems.cpp
)

# PUBLIC on an include folder means "use it to build this target, and also
# pass it on to every target that links this one". The tests need it, to
# find headers such as registry.h.
target_include_directories(asteroids_core PUBLIC src)

# The core's headers include <raylib.h> and EnTT's headers, so anything
# that links the core needs both too. PUBLIC passes raylib on to those targets. PRIVATE would
# hide it from them, and INTERFACE would pass it on without the core using
# it itself.
target_link_libraries(asteroids_core PUBLIC raylib EnTT::EnTT)

# macOS system frameworks raylib needs: windows (Cocoa), keyboard and
# controller input (IOKit), graphics (OpenGL) and frame timing (CoreVideo).
# PUBLIC again, because the final programs are where linking happens.
if (APPLE)
    target_link_libraries(asteroids_core PUBLIC
        "-framework Cocoa" "-framework IOKit" "-framework OpenGL" "-framework CoreVideo")
endif()

# The game program: just main(), plus everything in the core.
add_executable(asteroids src/main.cpp)
# PRIVATE: nothing links against a program, so there is nothing to pass on.
target_link_libraries(asteroids PRIVATE asteroids_core)

# -Wall and -Wextra turn on most useful compiler warnings. -Wpedantic warns
# about compiler-specific extensions that aren't standard C++. PRIVATE keeps
# these flags to our own targets, so they don't leak into code that links
# the core.
foreach(target asteroids_core asteroids)
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
endforeach()

# --- Tests ---
# enable_testing() turns on CTest, CMake's test runner. Each add_test() line
# registers one program; `ctest` runs them all and reports which failed (a
# program fails by returning a non-zero exit code).
enable_testing()

add_executable(ecs_test tests/ecs_test.cpp)
target_link_libraries(ecs_test PRIVATE asteroids_core)
target_compile_options(ecs_test PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME ecs_test COMMAND ecs_test)

add_executable(systems_test tests/systems_test.cpp)
target_link_libraries(systems_test PRIVATE asteroids_core)
target_compile_options(systems_test PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME systems_test COMMAND systems_test)

# --- Benchmark ---
# Not a test: it prints timings instead of passing or failing, so it isn't
# registered with add_test(). Build it in Release mode to get real numbers;
# see the comment at the top of bench/registry_bench.cpp.
add_executable(registry_bench bench/registry_bench.cpp)
target_link_libraries(registry_bench PRIVATE asteroids_core)
target_compile_options(registry_bench PRIVATE -Wall -Wextra -Wpedantic)
