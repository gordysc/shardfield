#ifndef ASTEROIDS_SPAWN_H_
#define ASTEROIDS_SPAWN_H_

// Header guard; see screen.h for how it works.

#include <raylib.h>

#include "components.h"
#include "ecs.h"
#include "random.h"
#include "rock.h"

/// @brief Everything that belongs to the game.
namespace asteroids {

// Spawn functions build each kind of entity from its components. They are
// the only place that knows which components make up a ship, a bullet or a
// rock. ECS libraries often call these "prefabs" or "archetypes".
//
// EnTT would allow calling them inside a view loop. Systems still wrap the
// calls in Defer() (see ecs.h), so that new entities appear at Flush(), the
// same frame rule as on `main`. Without that, a rock piece made by one
// bullet could be hit by the next bullet in the same frame.

/// @brief Creates the player's ship, briefly unable to be hurt.
/// @param registry Where to create it.
/// @param position Where the ship appears.
/// @return The new ship.
entt::entity SpawnShip(entt::registry& registry, Vector2 position);

/// @brief Creates a bullet.
/// @param registry Where to create it.
/// @param position Where the bullet starts.
/// @param velocity Its velocity in pixels per second.
/// @return The new bullet.
entt::entity SpawnBullet(entt::registry& registry, Vector2 position,
                         Vector2 velocity);

/// @brief Creates a homing missile.
/// @param registry Where to create it.
/// @param position Where the missile starts.
/// @param heading Which way it starts flying, in degrees clockwise from up.
/// @param target What to chase. Pass entt::null to let it pick the
///        nearest rock itself.
/// @return The new missile.
entt::entity SpawnMissile(entt::registry& registry, Vector2 position,
                          float heading, entt::entity target);

/// @brief Creates a pickup that drifts slowly and vanishes if not collected.
/// @param registry Where to create it.
/// @param position Where the pickup appears.
/// @param velocity Its drift in pixels per second.
/// @param kind What it gives the ship.
/// @return The new pickup.
entt::entity SpawnPickup(entt::registry& registry, Vector2 position,
                         Vector2 velocity, PickupKind kind);

/// @brief Creates a rock with a random shape, spin and drift.
/// @param registry Where to create it.
/// @param random Source of the random shape, spin and drift.
/// @param size Which size of rock.
/// @param position Where the rock starts.
/// @param base_velocity Velocity to add on top of the random drift. A piece
///        of a broken rock inherits some of its parent's.
/// @return The new rock.
entt::entity SpawnRock(entt::registry& registry, Random& random, RockSize size,
                       Vector2 position, Vector2 base_velocity);

/// @brief Creates one particle: a small circle that drifts, slows down and
/// fades away.
/// @param registry Where to create it.
/// @param position Where the particle starts.
/// @param velocity Its velocity in pixels per second.
/// @param radius Its size in pixels.
/// @param color Its colour.
/// @param lifetime Seconds until it has faded out and is destroyed.
/// @return The new particle.
entt::entity SpawnParticle(entt::registry& registry, Vector2 position,
                           Vector2 velocity, float radius, Color color,
                           float lifetime);

/// @brief Creates a burst of particles flying out in every direction, for
/// explosions.
/// @param registry Where to create them.
/// @param random Source of the random directions, speeds and sizes.
/// @param position Centre of the burst.
/// @param base_velocity Velocity every particle starts with on top of its
///        own, usually the velocity of whatever exploded.
/// @param count How many particles to create.
/// @param color Their colour.
void SpawnBurst(entt::registry& registry, Random& random, Vector2 position,
                Vector2 base_velocity, int count, Color color);

/// @brief Creates large rocks at random spots, none of them close to a
/// given point.
/// @param registry Where to create them.
/// @param random Source of the random spots.
/// @param count How many rocks to create.
/// @param avoid The point to keep clear, usually where the ship appears.
void SpawnRocksAwayFrom(entt::registry& registry, Random& random, int count,
                        Vector2 avoid);

}  // namespace asteroids

#endif  // ASTEROIDS_SPAWN_H_
