#include <raylib.h>

#include "screen.h"

/// @brief Opens the game window and runs the game loop until it is closed.
/// @return 0 on a normal exit.
int main() {
  InitWindow(asteroids::kScreenWidth, asteroids::kScreenHeight, "Shardfield");
  SetTargetFPS(60);

  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(Color{10, 12, 24, 255});
    DrawText("Shardfield", 20, 20, 40, RAYWHITE);
    DrawFPS(10, asteroids::kScreenHeight - 25);
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
