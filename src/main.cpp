#include <raylib.h>

#include <random>

#include "game.h"
#include "input.h"
#include "screen.h"

/// @brief Opens the game window and runs the game loop until it is closed.
/// @return 0 on a normal exit.
int main() {
  InitWindow(asteroids::kScreenWidth, asteroids::kScreenHeight, "Shardfield");
  SetTargetFPS(60);

  // std::random_device asks the operating system for a truly unpredictable
  // number. It is slow, so it only picks the seed; the Game's own generator
  // makes every number after that.
  std::random_device seed_source;

  // `game` is a local variable, so its destructor runs when main() returns,
  // which is after CloseWindow(). That is fine, because Game holds no raylib
  // resources. If it ever loads textures or sounds, wrap the next few lines
  // in their own `{ }` block, so that the Game is destroyed (and can unload
  // them) before the window closes.
  asteroids::Game game(seed_source());
  while (!WindowShouldClose()) {
    game.Update(GetFrameTime(), asteroids::ReadInput());

    BeginDrawing();
    game.Draw();
    DrawFPS(10, asteroids::kScreenHeight - 25);
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
