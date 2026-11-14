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

/// @brief Which screen the game is on. Decides what each frame updates and
/// draws.
enum class GameState {
  /// @brief Title screen, shown when the game starts. Rocks drift behind it.
  kTitle,
  /// @brief Normal play.
  kPlaying,
  /// @brief Play is frozen until the player unpauses.
  kPaused,
  /// @brief The player has lost their last life.
  kGameOver,
};

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
  /// @brief Seconds until rocks can hurt the ship again. A new ship gets a
  /// few seconds so it isn't destroyed the moment it appears.
  float invulnerable_for;
  /// @brief False between losing a life and the next ship appearing.
  bool alive;
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
  /// @brief Set when the rock breaks. Broken rocks are erased at the end of
  /// the frame; see Game::HandleCollisions().
  bool broken = false;
};

/// @brief The whole game. Owns the ship, bullets, rocks, score, lives and
/// current screen, and knows how to update and draw them.
///
/// main() creates one Game, then calls Update() and Draw() once per frame.
class Game {
 public:
  /// @brief Sets up the title screen.
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
  /// @brief Moves between screens when the player presses SPACE or P.
  void HandleStateKeys();
  /// @brief Resets score, lives and rocks, and starts wave 1.
  void StartGame();
  /// @brief Runs one frame of normal play.
  /// @param dt Seconds since the last frame.
  void UpdatePlaying(float dt);
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
  /// @brief Handles bullets hitting rocks and rocks hitting the ship, then
  /// removes everything that broke or ran out.
  void HandleCollisions();
  /// @brief Marks a rock as broken and adds its pieces to `pieces`.
  /// @param rock The rock to break.
  /// @param pieces Where to put the smaller rocks. They are kept apart from
  ///        `rocks_` until no loop is walking over it.
  void BreakRock(Rock& rock, std::vector<Rock>& pieces);
  /// @brief Takes a life and either starts the respawn countdown or ends
  /// the game.
  void DestroyShip();
  /// @brief Puts a fresh ship in the middle of the screen.
  void SpawnShip();
  /// @brief Moves to the next wave and adds its rocks.
  void StartNextWave();
  /// @brief Creates a rock (but doesn't store it).
  /// @param size Which size of rock.
  /// @param position Where the rock starts.
  /// @param base_velocity Velocity to add on top of the rock's own random
  ///        drift. A piece of a broken rock inherits some of its parent's.
  /// @return The new rock. The caller decides where to store it.
  Rock MakeRock(RockSize size, Vector2 position, Vector2 base_velocity);
  /// @brief Adds large rocks at random spots away from the ship.
  /// @param count How many rocks to add.
  void SpawnRocks(int count);

  /// @brief Draws the rocks, bullets and ship.
  void DrawField() const;
  /// @brief Draws the score, wave and lives along the top of the screen.
  void DrawHud() const;
  /// @brief Draws the centred text for the current screen.
  void DrawMessage() const;

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
  /// @brief Points scored so far this game.
  int score_;
  /// @brief Lives left, including the ship in play.
  int lives_;
  /// @brief The current wave number, starting at 1.
  int wave_;
  /// @brief Seconds until the next ship appears after losing a life.
  float respawn_timer_;
  /// @brief Seconds until the next wave, counted down once every rock is
  /// gone.
  float next_wave_timer_;
  /// @brief Seconds left to show the "WAVE n" banner.
  float banner_timer_;
  /// @brief Which screen the game is on.
  GameState state_;
};

}  // namespace asteroids

#endif  // ASTEROIDS_GAME_H_
