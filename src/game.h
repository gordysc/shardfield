#ifndef ASTEROIDS_GAME_H_
#define ASTEROIDS_GAME_H_

// Header guard; see screen.h for how it works.

#include <vector>

#include "draw.h"
#include "input.h"
#include "random.h"
#include "registry.h"

/// @brief Everything that belongs to the game.
namespace asteroids {

/// @brief Which screen the game is on. Decides what each frame updates and
/// draws.
///
/// `enum class` is the modern replacement for a plain `enum`. Its names are
/// scoped (you write `GameState::kPlaying`), and it never silently converts
/// to an int.
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

/// @brief The whole game: the entity registry plus the rules that sit above
/// it (score, lives, waves and which screen is showing).
///
/// Everything that moves is an entity in `registry_`, updated by the
/// systems in systems.h. The Game only decides which systems run, in what
/// order, and what the results mean for the score and lives.
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
  /// @param input What the player asked for this frame. Taken by const
  ///        reference: the Game reads it but never changes it.
  void Update(float dt, const Input& input);

  /// @brief Draws the current frame. Call it between raylib's BeginDrawing()
  /// and EndDrawing().
  ///
  /// `const` after the parentheses promises that drawing doesn't change the
  /// Game, and the compiler enforces it.
  void Draw() const;

 private:
  /// @brief Moves between screens on the start and pause keys.
  /// @param input This frame's input.
  void HandleStateKeys(const Input& input);
  /// @brief Resets score, lives and entities, and starts wave 1.
  void StartGame();
  /// @brief Runs one frame of normal play.
  /// @param dt Seconds since the last frame.
  /// @param input This frame's input.
  void UpdatePlaying(float dt, const Input& input);
  /// @brief Runs the systems that only move things: drag, speed limits,
  /// movement, wrapping and lifetimes. Used on every screen except pause.
  /// @param dt Seconds since the last frame.
  void UpdateMotion(float dt);
  /// @brief Moves to the next wave and adds its rocks.
  void StartNextWave();

  /// @brief Draws the score, wave and lives along the top of the screen.
  void DrawHud() const;
  /// @brief Draws the centred text for the current screen.
  void DrawMessage() const;

  // Google style ends data member names with an underscore, so inside the
  // methods you can tell `score_` (part of the Game) from a local variable.

  /// @brief Source of every random choice in the game.
  Random random_;
  /// @brief Background stars. They never move, so they aren't entities.
  std::vector<Star> stars_;
  /// @brief Every ship, bullet and rock, with their components.
  Registry registry_;
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
