#ifndef ASTEROIDS_SYSTEMS_H_
#define ASTEROIDS_SYSTEMS_H_

// Header guard; see screen.h for how it works.

#include "input.h"
#include "random.h"
#include "registry.h"

/// @brief Everything that belongs to the game.
namespace asteroids {

// A system is a plain function that loops over every entity with a certain
// set of components and updates them. Each system does one job, knows
// nothing about the others, and only sees the components it asks for.
//
// The game calls them in a fixed order every frame (see Game::UpdatePlaying)
// and then calls Registry::Flush(), which applies the spawns and destroys
// that the systems queued.

/// @brief What happened in one frame of collisions, for the game rules
/// (score and lives) to act on.
///
/// The collision system reports these instead of changing the score itself.
/// That keeps the system about entities, and leaves scoring to the Game.
struct CollisionReport {
  /// @brief Points earned from rocks broken this frame.
  int points = 0;
  /// @brief Whether a rock destroyed the ship this frame.
  bool ship_destroyed = false;
};

/// @brief Turns and thrusts every Ship from the player's input, and counts
/// down its invulnerability.
///
/// Needs: Ship, Transform, Velocity.
///
/// @param registry The entities to update.
/// @param input This frame's input.
/// @param dt Seconds since the last frame.
void ShipControlSystem(Registry& registry, const Input& input, float dt);

/// @brief Fires a bullet from every ready Gun while the fire key is held.
///
/// Needs: Gun, Transform, Velocity. Bullets are created at the next Flush().
///
/// @param registry The entities to update.
/// @param input This frame's input.
/// @param dt Seconds since the last frame.
void GunSystem(Registry& registry, const Input& input, float dt);

/// @brief Slows down every entity with Drag.
///
/// Needs: Velocity, Drag.
///
/// @param registry The entities to update.
/// @param dt Seconds since the last frame.
void DragSystem(Registry& registry, float dt);

/// @brief Caps the speed of every entity with a SpeedLimit.
///
/// Needs: Velocity, SpeedLimit.
///
/// @param registry The entities to update.
void SpeedLimitSystem(Registry& registry);

/// @brief Moves and spins every entity by its velocity.
///
/// Needs: Transform, Velocity.
///
/// @param registry The entities to update.
/// @param dt Seconds since the last frame.
void MovementSystem(Registry& registry, float dt);

/// @brief Moves entities that left the screen to the opposite edge.
///
/// Needs: Transform, Wraps.
///
/// @param registry The entities to update.
void WrapSystem(Registry& registry);

/// @brief Counts down every Lifetime and destroys entities whose time is up.
///
/// Needs: Lifetime.
///
/// @param registry The entities to update.
/// @param dt Seconds since the last frame.
void LifetimeSystem(Registry& registry, float dt);

/// @brief Breaks rocks hit by projectiles or by the ship, and destroys
/// whatever hit them.
///
/// Needs: Projectile or Ship, plus Transform and Collider; and Rock,
/// Transform, Collider and Velocity on rocks. Pieces of broken rocks are
/// created at the next Flush().
///
/// @param registry The entities to check.
/// @param random Source of the pieces' random shapes and drift. It must
///        outlive the next Flush(), because the queued spawns use it.
/// @return What happened, for scoring.
CollisionReport CollisionSystem(Registry& registry, Random& random);

}  // namespace asteroids

#endif  // ASTEROIDS_SYSTEMS_H_
