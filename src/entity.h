#ifndef ASTEROIDS_ENTITY_H_
#define ASTEROIDS_ENTITY_H_

// Header guard; see screen.h for how it works.

#include <cstdint>
#include <limits>

/// @brief Everything that belongs to the game.
namespace asteroids {

/// @brief Index stored in an Entity that refers to no slot at all.
///
/// std::numeric_limits<T>::max() is the largest value type T can hold, here
/// 4,294,967,295. No registry gets that many slots, so it is safe to use as
/// "nothing".
inline constexpr std::uint32_t kNoIndex =
    std::numeric_limits<std::uint32_t>::max();

/// @brief A handle to one thing in the game: a ship, a rock, a bullet...
///
/// In an entity component system (ECS) an entity has no data or behaviour
/// of its own. It is only a name that components are attached to. The
/// Registry stores the components; see registry.h.
///
/// The handle has two parts:
/// - `index` says which slot in the Registry the entity lives in. When an
///   entity is destroyed, its slot is reused for a later entity.
/// - `generation` counts how many times that slot has been reused. The
///   Registry bumps it on every destroy, so an old handle to a destroyed
///   entity no longer matches the slot, even after the slot is reused.
///
/// Without the generation, a homing missile still aiming at destroyed rock
/// #42 would silently start chasing whatever new entity (say, a particle)
/// got slot 42 next. With it, Registry::IsAlive() tells the missile that its
/// target is gone.
///
/// Entity is two 32-bit integers, so copy it freely and pass it by value.
struct Entity {
  /// @brief Which slot in the Registry the entity lives in.
  std::uint32_t index = kNoIndex;
  /// @brief How many times the slot had been reused when this handle was
  /// made.
  std::uint32_t generation = 0;

  /// @brief Two handles are equal when both parts match.
  ///
  /// `= default` (C++20) asks the compiler to write the comparison: it
  /// compares each member in order. `friend` lets it be written inside the
  /// struct while still being a normal two-argument function, so `a == b`
  /// works with either side converted. Defining == also gives != for free.
  friend bool operator==(const Entity&, const Entity&) = default;
};

/// @brief A handle that never refers to a live entity. A default-made
/// Entity is the same as this.
inline constexpr Entity kNullEntity{};

}  // namespace asteroids

#endif  // ASTEROIDS_ENTITY_H_
