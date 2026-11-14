#ifndef ASTEROIDS_INPUT_H_
#define ASTEROIDS_INPUT_H_

// Header guard; see screen.h for how it works.

/// @brief Everything that belongs to the game.
namespace asteroids {

/// @brief What the player asked for this frame.
///
/// The game reads the keyboard in one place, ReadInput(), and passes this
/// struct to the systems. The systems never call raylib's IsKeyDown()
/// themselves, so a test can drive them with a hand-made Input and no
/// window at all.
struct Input {
  /// @brief Turn the ship anticlockwise. Held.
  bool turn_left = false;
  /// @brief Turn the ship clockwise. Held.
  bool turn_right = false;
  /// @brief Fire the engine. Held.
  bool thrust = false;
  /// @brief Fire the gun. Held.
  bool fire = false;
  /// @brief Launch a homing missile. True only on the frame the key goes
  /// down, so each press launches one missile.
  bool fire_missile = false;
  /// @brief Start a game from the title or game over screen. True only on
  /// the frame the key goes down.
  bool start = false;
  /// @brief Pause or unpause. True only on the frame the key goes down.
  bool pause = false;
};

/// @brief Reads the keyboard through raylib.
/// @return This frame's input.
Input ReadInput();

}  // namespace asteroids

#endif  // ASTEROIDS_INPUT_H_
