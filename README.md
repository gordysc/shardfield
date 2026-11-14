# Shardfield

Shardfield is a small arena shooter in C++20 with
[raylib](https://www.raylib.com/). The player flies a ship on a screen that
wraps at the edges. Bullets break rocks into smaller rocks.

## Build in CLion
1. Open this folder in CLion.
2. Wait for CMake to download raylib. This step needs an internet connection.
3. Select the `asteroids` run configuration.
4. Press Run.

## Build from the command line
```bash
cmake -S . -B build
cmake --build build
./build/asteroids
```
