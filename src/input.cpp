#include "input.h"

#include <raylib.h>

namespace asteroids {

Input ReadInput() {
  // IsKeyDown stays true for as long as the key is held. IsKeyPressed is
  // true only on the frame the key goes down, which suits one-off actions
  // like pausing.
  //
  // SPACE both starts the game and fires the gun. That is fine: on the
  // title screen only `start` is used, and during play only `fire`.
  return Input{
      .turn_left = IsKeyDown(KEY_LEFT),
      .turn_right = IsKeyDown(KEY_RIGHT),
      .thrust = IsKeyDown(KEY_UP),
      .fire = IsKeyDown(KEY_SPACE),
      .fire_missile =
          IsKeyPressed(KEY_LEFT_SHIFT) || IsKeyPressed(KEY_RIGHT_SHIFT),
      .start = IsKeyPressed(KEY_SPACE),
      .pause = IsKeyPressed(KEY_P),
  };
}

}  // namespace asteroids
