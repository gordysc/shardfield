#include "systems.h"

#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <optional>

#include "components.h"
#include "entity.h"
#include "geometry.h"
#include "input.h"
#include "random.h"
#include "registry.h"
#include "rock.h"
#include "spawn.h"
#include "tuning.h"

namespace asteroids {

// Helpers visible only in this file; see draw.cpp.
namespace {

/// @brief Destroys a rock and queues its two smaller pieces, if it has any.
/// @param registry The registry the rock lives in.
/// @param random Source of the pieces' random shapes and drift.
/// @param rock The rock to break. Must have Rock, Transform and Velocity.
void BreakRock(Registry& registry, Random& random, Entity rock) {
  // Copy out what the pieces need now. After Flush() the rock and its
  // components are gone, so the deferred command can't read them then.
  const RockSize size = registry.Get<Rock>(rock).size;
  const Vector2 position = registry.Get<Transform>(rock).position;
  const Vector2 velocity = registry.Get<Velocity>(rock).linear;
  registry.Destroy(rock);

  const std::optional<RockSize> smaller = SplitSize(size);
  if (!smaller) return;
  const Vector2 inherited{velocity.x * kSplitInheritance,
                          velocity.y * kSplitInheritance};
  // This runs inside an Each() loop, where creating entities isn't allowed,
  // so the spawn is deferred to the next Flush().
  //
  // The lambda's capture list decides what it keeps:
  // - `piece_size`, `position` and `inherited` are captured by value
  //   (copied into the lambda), so they are still valid at Flush() time.
  // - `&random` is captured by reference, because the pieces must draw from
  //   the game's one generator, not from a copy. That reference is only safe
  //   because the Random (a Game member) outlives the next Flush(). Capturing
  //   a local variable by reference here would leave the lambda pointing at
  //   a dead variable: undefined behaviour.
  registry.Defer(
      [piece_size = *smaller, position, inherited, &random](Registry& r) {
        SpawnRock(r, random, piece_size, position, inherited);
        SpawnRock(r, random, piece_size, position, inherited);
      });
}

}  // namespace

void ShipControlSystem(Registry& registry, const Input& input, float dt) {
  // The lambda is called once per ship. `[&]` captures `input` and `dt` by
  // reference, which is safe because Each() calls the lambda right away,
  // while they are still alive. The unnamed `Entity` parameter means "I
  // don't need the handle".
  registry.Each<Ship, Transform, Velocity>(
      [&](Entity, Ship& ship, Transform& transform, Velocity& velocity) {
        if (input.turn_left) transform.rotation -= kShipTurnSpeed * dt;
        if (input.turn_right) transform.rotation += kShipTurnSpeed * dt;

        ship.thrusting = input.thrust;
        if (ship.thrusting) {
          const Vector2 direction = HeadingToDirection(transform.rotation);
          velocity.linear.x += direction.x * kShipThrust * dt;
          velocity.linear.y += direction.y * kShipThrust * dt;
        }
        ship.invulnerable_for = std::max(0.0f, ship.invulnerable_for - dt);
      });
}

void GunSystem(Registry& registry, const Input& input, float dt) {
  registry.Each<Gun, Transform, Velocity>([&](Entity, Gun& gun,
                                              const Transform& transform,
                                              const Velocity& velocity) {
    gun.cooldown = std::max(0.0f, gun.cooldown - dt);
    if (!input.fire || gun.cooldown > 0) return;
    gun.cooldown = kFireInterval;

    const Vector2 direction = HeadingToDirection(transform.rotation);
    // The bullet starts at the nose and adds the shooter's own velocity, so
    // shots fired while flying forwards don't crawl out of the gun.
    const Vector2 nose{transform.position.x + direction.x * kShipRadius,
                       transform.position.y + direction.y * kShipRadius};
    const Vector2 shot{velocity.linear.x + direction.x * kBulletSpeed,
                       velocity.linear.y + direction.y * kBulletSpeed};
    // Captured by value: `nose` and `shot` are locals that die when this
    // lambda returns, long before Flush() runs the command.
    registry.Defer([nose, shot](Registry& r) { SpawnBullet(r, nose, shot); });
  });
}

void DragSystem(Registry& registry, float dt) {
  registry.Each<Velocity, Drag>(
      [dt](Entity, Velocity& velocity, const Drag& drag) {
        // Each second, the velocity shrinks to exp(-rate) of what it was.
        // Using exp() instead of "multiply by 0.99 each frame" makes the
        // slowdown the same whatever the frame rate.
        const float keep = std::exp(-drag.rate * dt);
        velocity.linear.x *= keep;
        velocity.linear.y *= keep;
      });
}

void SpeedLimitSystem(Registry& registry) {
  registry.Each<Velocity, SpeedLimit>(
      [](Entity, Velocity& velocity, const SpeedLimit& limit) {
        // std::hypot(x, y) is sqrt(x * x + y * y): the speed.
        const float speed = std::hypot(velocity.linear.x, velocity.linear.y);
        if (speed <= limit.max) return;
        const float scale = limit.max / speed;
        velocity.linear.x *= scale;
        velocity.linear.y *= scale;
      });
}

void MovementSystem(Registry& registry, float dt) {
  // `[dt]` captures only `dt`, by value: a copy of one float. Naming exactly
  // what a lambda captures documents what it depends on.
  registry.Each<Transform, Velocity>(
      [dt](Entity, Transform& transform, const Velocity& velocity) {
        transform.position.x += velocity.linear.x * dt;
        transform.position.y += velocity.linear.y * dt;
        transform.rotation += velocity.angular * dt;
      });
}

void WrapSystem(Registry& registry) {
  registry.Each<Transform, Wraps>(
      [](Entity, Transform& transform, const Wraps& wraps) {
        transform.position = WrapPosition(transform.position, wraps.margin);
      });
}

void LifetimeSystem(Registry& registry, float dt) {
  // `[&registry, dt]` captures the registry by reference (it must not be
  // copied, and copying is deleted anyway) and `dt` by value.
  registry.Each<Lifetime>([&registry, dt](Entity entity, Lifetime& lifetime) {
    lifetime.seconds_left -= dt;
    // Destroy() inside Each() is allowed: it only marks the entity.
    if (lifetime.seconds_left <= 0) registry.Destroy(entity);
  });
}

CollisionReport CollisionSystem(Registry& registry, Random& random) {
  CollisionReport report;

  // --- Projectiles against rocks ---
  // Two Each() loops nest: for every projectile, check every rock. That is
  // fine for a few dozen of each. Thousands would need a spatial grid so
  // that each projectile only checks nearby rocks.
  registry.Each<Projectile, Transform, Collider>(
      [&](Entity shot, Projectile&, const Transform& shot_transform,
          const Collider& shot_collider) {
        registry.Each<Rock, Transform, Collider>(
            [&](Entity rock, const Rock& rock_data,
                const Transform& rock_transform,
                const Collider& rock_collider) {
              // Each() has no `break`. Once this shot has hit a rock it is
              // marked dying, so this check skips the remaining rocks.
              if (!registry.IsAlive(shot)) return;
              if (!CheckCollisionCircles(
                      shot_transform.position, shot_collider.radius,
                      rock_transform.position, rock_collider.radius)) {
                return;
              }
              report.points += RockPoints(rock_data.size);
              // Breaking marks the rock dying, so later shots in this loop
              // skip it and can't score it twice.
              BreakRock(registry, random, rock);
              registry.Destroy(shot);
            });
      });

  // --- Rocks against the ship ---
  registry.Each<Ship, Transform, Collider>(
      [&](Entity ship, const Ship& ship_data, const Transform& ship_transform,
          const Collider& ship_collider) {
        if (ship_data.invulnerable_for > 0) return;
        registry.Each<Rock, Transform, Collider>(
            [&](Entity rock, const Rock&, const Transform& rock_transform,
                const Collider& rock_collider) {
              if (!registry.IsAlive(ship)) return;
              if (!CheckCollisionCircles(
                      ship_transform.position, ship_collider.radius,
                      rock_transform.position, rock_collider.radius)) {
                return;
              }
              BreakRock(registry, random, rock);
              registry.Destroy(ship);
              report.ship_destroyed = true;
            });
      });

  return report;
}

}  // namespace asteroids
