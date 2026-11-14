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

## Run the tests
Build the project first, then run CTest:
```bash
ctest --test-dir build --output-on-failure
```
The tests do not open a window.

## Controls
- Left and Right arrows: turn the ship.
- Up arrow: fire the engine.
- Space: fire the gun. Hold Space to fire again and again.
- Shift: launch a homing missile, if you have one.
- P: pause the game, or continue a paused game.
- Esc: close the game.

## How to play
Break every rock to start the next wave. Each wave has more rocks. A rock
that touches the ship destroys it, and you lose a life. A new ship blinks
for a short time, and rocks cannot destroy it while it blinks.

Small rocks sometimes leave a pickup when they break. Fly into a pickup to
collect it. A pickup blinks before it disappears.
- **M** gives the ship 3 homing missiles.
- **S** gives the ship a shield for 6 seconds. Rocks cannot destroy a ship
  with a shield.
