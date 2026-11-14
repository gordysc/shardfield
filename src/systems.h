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

/// @brief Sprays flame particles behind every Ship that is thrusting.
///
/// Needs: Ship, Transform, Velocity. Particles are created at the next
/// Flush().
///
/// @param registry The entities to update.
/// @param random Source of the particles' random spray. It must outlive the
///        next Flush(), because the queued spawns use it.
/// @param dt Seconds since the last frame.
void ExhaustSystem(Registry& registry, Random& random, float dt);

/// @brief Launches a homing missile from every ready MissileLauncher that
/// has missiles left, when the missile key is pressed.
///
/// Needs: MissileLauncher, Transform. Missiles are created at the next
/// Flush().
///
/// @param registry The entities to update.
/// @param input This frame's input.
/// @param dt Seconds since the last frame.
void MissileLauncherSystem(Registry& registry, const Input& input, float dt);

/// @brief Turns every Homing entity towards its target, picking the nearest
/// rock as a new target when the old one is gone.
///
/// Needs: Homing, Transform, Velocity.
///
/// @param registry The entities to update.
/// @param dt Seconds since the last frame.
void HomingSystem(Registry& registry, float dt);

/// @brief Leaves smoke particles behind every TrailEmitter.
///
/// Needs: TrailEmitter, Transform. Particles are created at the next
/// Flush().
///
/// @param registry The entities to update.
/// @param random Source of the particles' random drift. It must outlive the
///        next Flush(), because the queued spawns use it.
/// @param dt Seconds since the last frame.
void TrailSystem(Registry& registry, Random& random, float dt);

/// @brief Counts down every Shield and removes it when it runs out.
///
/// Needs: Shield. The component is removed at the next Flush().
///
/// @param registry The entities to update.
/// @param dt Seconds since the last frame.
void ShieldSystem(Registry& registry, float dt);

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
/// Transform, Collider and Velocity on rocks. A ship with a Shield is safe.
/// Pieces of broken rocks, dust and pickups are created at the next Flush().
///
/// @param registry The entities to check.
/// @param random Source of the pieces' random shapes and drift. It must
///        outlive the next Flush(), because the queued spawns use it.
/// @return What happened, for scoring.
CollisionReport CollisionSystem(Registry& registry, Random& random);

/// @brief Gives the ship whatever pickups it touches.
///
/// Needs: Ship, Transform, Collider; and Pickup, Transform, Collider on
/// pickups. A new Shield is added at the next Flush().
///
/// @param registry The entities to check.
void PickupSystem(Registry& registry);

}  // namespace asteroids

#endif  // ASTEROIDS_SYSTEMS_H_
