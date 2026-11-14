#ifndef ASTEROIDS_RANDOM_H_
#define ASTEROIDS_RANDOM_H_

// Header guard; see screen.h for how it works.

#include <raylib.h>

#include <cstdint>
#include <random>

/// @brief Everything that belongs to the game.
namespace asteroids {

/// @brief A source of random numbers for gameplay.
///
/// The game passes one Random around by reference instead of calling a
/// global function like rand(). That keeps every random choice traceable to
/// one object, and a test can build a Random with a fixed seed to get the
/// same "random" numbers on every run.
class Random {
 public:
  /// @brief Creates a generator.
  ///
  /// `explicit` stops the compiler from silently turning a plain number into
  /// a Random, for example when a function that takes a `Random&` is called
  /// with an int by mistake.
  ///
  /// @param seed Starting value. The same seed always gives the same
  ///        sequence of numbers.
  explicit Random(std::uint32_t seed);

  /// @brief Returns a random float.
  /// @param min Smallest possible value.
  /// @param max Largest possible value.
  /// @return A value from `min` up to (but not including) `max`.
  float Float(float min, float max);

  /// @brief Returns a random int.
  /// @param min Smallest possible value.
  /// @param max Largest possible value. Unlike Float(), it can be returned.
  /// @return A value from `min` to `max`, both included.
  int Int(int min, int max);

  /// @brief Returns true with the given probability.
  /// @param probability How likely `true` is, from 0 (never) to 1 (always).
  /// @return Whether the event happened.
  bool Chance(float probability);

  /// @brief Returns a unit vector pointing in a random direction.
  /// @return A direction of length 1.
  Vector2 Direction();

 private:
  /// @brief The underlying generator. std::mt19937 (the "Mersenne Twister")
  /// is the standard library's general-purpose generator: fast, and good
  /// enough for games, though not for cryptography.
  std::mt19937 engine_;
};

}  // namespace asteroids

#endif  // ASTEROIDS_RANDOM_H_
