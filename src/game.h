#ifndef ASTEROIDS_GAME_H_
#define ASTEROIDS_GAME_H_

// Header guard; see screen.h for how it works.

#include <raylib.h>

#include <vector>

#include "draw.h"
#include "random.h"

/// @brief Everything that belongs to the game.
namespace asteroids {

/// @brief The player's ship.
struct Ship {
  /// @brief Centre of the ship on screen.
  Vector2 position;
  /// @brief Velocity in pixels per second.
  Vector2 velocity;
  /// @brief Direction the nose points, in degrees clockwise from up.
  float heading;
  /// @brief Whether the engine fired this frame. Only used for drawing.
  bool thrusting;
};

/// @brief The whole game. Owns the ship and the background, and knows how to
/// update and draw them.
///
/// main() creates one Game, then calls Update() and Draw() once per frame.
class Game {
 public:
  /// @brief Sets up a fresh game.
  /// @param seed Seed for the random number generator.
  explicit Game(unsigned int seed);

  // `= delete` removes the copy constructor and copy assignment, so copying a
  // Game is a compile error. There is only ever one game, so a copy (for
  // example from passing a Game by value where you meant a reference) would
  // be a bug.

  /// @brief Not copyable: there is only ever one game.
  Game(const Game&) = delete;
  /// @brief Not copy-assignable: there is only ever one game.
  Game& operator=(const Game&) = delete;

  /// @brief Advances the game by one frame.
  /// @param dt Seconds since the last frame.
  void Update(float dt);

  /// @brief Draws the current frame. Call it between raylib's BeginDrawing()
  /// and EndDrawing().
  ///
  /// `const` after the parentheses promises that drawing doesn't change the
  /// Game, and the compiler enforces it.
  void Draw() const;

 private:
  /// @brief Turns and thrusts the ship from the arrow keys, then moves it.
  /// @param dt Seconds since the last frame.
  void UpdateShip(float dt);

  // Google style ends data member names with an underscore, so inside the
  // methods you can tell `ship_` (part of the Game) from a local variable.

  /// @brief Source of every random choice in the game.
  Random random_;
  /// @brief Background stars. They never move.
  std::vector<Star> stars_;
  /// @brief The player's ship.
  Ship ship_;
};

}  // namespace asteroids

#endif  // ASTEROIDS_GAME_H_
