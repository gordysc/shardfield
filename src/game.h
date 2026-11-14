#ifndef ASTEROIDS_GAME_H_
#define ASTEROIDS_GAME_H_

// Header guard; see screen.h for how it works.

#include <raylib.h>

#include <vector>

#include "draw.h"
#include "random.h"
#include "rock.h"

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
  /// @brief Seconds until the gun can fire again. 0 means ready.
  float fire_cooldown;
};

/// @brief One bullet fired by the ship.
struct Bullet {
  /// @brief Centre of the bullet on screen.
  Vector2 position;
  /// @brief Velocity in pixels per second.
  Vector2 velocity;
  /// @brief Seconds until the bullet fizzles out.
  float time_left;
};

/// @brief One drifting, spinning rock.
struct Rock {
  /// @brief Centre of the rock on screen.
  Vector2 position;
  /// @brief Velocity in pixels per second.
  Vector2 velocity;
  /// @brief How far the rock has spun, in degrees.
  float rotation;
  /// @brief How fast the rock spins, in degrees per second.
  float spin;
  /// @brief Which size of rock this is.
  RockSize size;
  /// @brief The rock's bumpy outline.
  RockShape shape;
  /// @brief Set when a bullet breaks the rock. Broken rocks are erased at
  /// the end of the frame; see Game::HandleBulletHits().
  bool broken = false;
};

/// @brief The whole game. Owns the ship, bullets, rocks and score, and knows
/// how to update and draw them.
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
  /// @brief Fires a bullet when SPACE is held and the gun is ready.
  /// @param dt Seconds since the last frame.
  void UpdateGun(float dt);
  /// @brief Moves every bullet and counts down its lifetime.
  /// @param dt Seconds since the last frame.
  void UpdateBullets(float dt);
  /// @brief Moves and spins every rock.
  /// @param dt Seconds since the last frame.
  void UpdateRocks(float dt);
  /// @brief Breaks rocks that bullets touch, scores them and adds the
  /// smaller pieces. Also removes spent bullets.
  void HandleBulletHits();
  /// @brief Adds a new rock.
  /// @param size Which size of rock.
  /// @param position Where the rock starts.
  /// @param base_velocity Velocity to add on top of the rock's own random
  ///        drift. A piece of a broken rock inherits some of its parent's.
  /// @return The new rock. The caller decides where to store it.
  Rock MakeRock(RockSize size, Vector2 position, Vector2 base_velocity);
  /// @brief Adds a wave of large rocks, away from the ship.
  /// @param count How many rocks to add.
  void SpawnWave(int count);

  // Google style ends data member names with an underscore, so inside the
  // methods you can tell `ship_` (part of the Game) from a local variable.

  /// @brief Source of every random choice in the game.
  Random random_;
  /// @brief Background stars. They never move.
  std::vector<Star> stars_;
  /// @brief The player's ship.
  Ship ship_;
  /// @brief Bullets in flight. Bullets are added when fired and erased when
  /// they run out, so this vector grows and shrinks all the time.
  std::vector<Bullet> bullets_;
  /// @brief Rocks still in play.
  std::vector<Rock> rocks_;
  /// @brief Points scored so far.
  int score_;
};

}  // namespace asteroids

#endif  // ASTEROIDS_GAME_H_
