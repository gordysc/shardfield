#ifndef ASTEROIDS_ROCK_H_
#define ASTEROIDS_ROCK_H_

// Header guard; see screen.h for how it works.

#include <array>
#include <optional>

#include "random.h"

/// @brief Everything that belongs to the game.
namespace asteroids {

/// @brief How big a rock is. A shot rock breaks into two of the next size
/// down, and the smallest rocks just vanish.
///
/// `enum class` is the modern replacement for a plain `enum`. Its names are
/// scoped (you write `RockSize::kLarge`), and it never silently converts to
/// an int.
enum class RockSize {
  /// @brief The smallest rock. Doesn't split.
  kSmall,
  /// @brief Splits into two small rocks.
  kMedium,
  /// @brief The size every wave starts with. Splits into two medium rocks.
  kLarge,
};

/// @brief How many corners each rock outline has.
inline constexpr int kRockVertices = 11;

/// @brief The bumpy outline of a rock: one radius multiplier per corner.
///
/// Corner `i` sits at angle `i * 360 / kRockVertices` degrees, at
/// `radius * shape[i]` from the centre. A `using` alias gives the array type
/// a readable name; it is the same type, not a new one.
using RockShape = std::array<float, kRockVertices>;

/// @brief Returns the size of a rock, for drawing and collisions.
/// @param size Which size of rock.
/// @return Radius in pixels.
float RockRadius(RockSize size);

/// @brief Returns how many points the player scores for breaking a rock.
/// Smaller rocks are harder to hit, so they are worth more.
/// @param size Which size of rock.
/// @return Points scored.
int RockPoints(RockSize size);

/// @brief Returns the size a rock breaks into.
///
/// std::optional (C++17) holds either a value or nothing. It replaces magic
/// "no value" markers such as -1 or a made-up kNone enumerator, and the
/// caller has to check before using the value.
///
/// @param size The size of the rock that was shot.
/// @return The size of each piece, or std::nullopt for a small rock, which
///         doesn't split.
std::optional<RockSize> SplitSize(RockSize size);

/// @brief Picks a random speed suited to a rock size. Smaller rocks fly
/// faster.
/// @param size Which size of rock.
/// @param random Source of the random speed.
/// @return Speed in pixels per second.
float RandomRockSpeed(RockSize size, Random& random);

/// @brief Makes a random bumpy outline, so no two rocks look the same.
/// @param random Source of the random bumps.
/// @return The outline.
RockShape MakeRockShape(Random& random);

}  // namespace asteroids

#endif  // ASTEROIDS_ROCK_H_
