#ifndef ASTEROIDS_SPAWN_H_
#define ASTEROIDS_SPAWN_H_

// Header guard; see screen.h for how it works.

#include <raylib.h>

#include "entity.h"
#include "random.h"
#include "registry.h"
#include "rock.h"

/// @brief Everything that belongs to the game.
namespace asteroids {

// Spawn functions build each kind of entity from its components. They are
// the only place that knows which components make up a ship, a bullet or a
// rock. ECS libraries often call these "prefabs" or "archetypes".
//
// They call Registry::Create() and Add(), so they must not run inside an
// Each() loop. Systems that spawn things wrap the call in Registry::Defer().

/// @brief Creates the player's ship, briefly unable to be hurt.
/// @param registry Where to create it.
/// @param position Where the ship appears.
/// @return The new ship.
Entity SpawnShip(Registry& registry, Vector2 position);

/// @brief Creates a bullet.
/// @param registry Where to create it.
/// @param position Where the bullet starts.
/// @param velocity Its velocity in pixels per second.
/// @return The new bullet.
Entity SpawnBullet(Registry& registry, Vector2 position, Vector2 velocity);

/// @brief Creates a rock with a random shape, spin and drift.
/// @param registry Where to create it.
/// @param random Source of the random shape, spin and drift.
/// @param size Which size of rock.
/// @param position Where the rock starts.
/// @param base_velocity Velocity to add on top of the random drift. A piece
///        of a broken rock inherits some of its parent's.
/// @return The new rock.
Entity SpawnRock(Registry& registry, Random& random, RockSize size,
                 Vector2 position, Vector2 base_velocity);

/// @brief Creates large rocks at random spots, none of them close to a
/// given point.
/// @param registry Where to create them.
/// @param random Source of the random spots.
/// @param count How many rocks to create.
/// @param avoid The point to keep clear, usually where the ship appears.
void SpawnRocksAwayFrom(Registry& registry, Random& random, int count,
                        Vector2 avoid);

}  // namespace asteroids

#endif  // ASTEROIDS_SPAWN_H_
