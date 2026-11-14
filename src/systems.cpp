#include "systems.h"

#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <optional>

#include "components.h"
#include "draw.h"
#include "ecs.h"
#include "geometry.h"
#include "input.h"
#include "random.h"
#include "rock.h"
#include "spawn.h"
#include "tuning.h"

namespace asteroids {

// Helpers visible only in this file; see draw.cpp.
namespace {

/// @brief Works out how many particles an emitter makes this frame.
///
/// `rate * dt` particles are due, usually a fraction like 1.5. This makes
/// the whole part, then one more with a chance equal to the leftover
/// fraction. On average that gives exactly `rate` per second, at any frame
/// rate.
///
/// @param rate Particles per second.
/// @param dt Seconds since the last frame.
/// @param random Source of the chance for the leftover fraction.
/// @return How many particles to make.
int ParticlesDue(float rate, float dt, Random& random) {
  const float due = rate * dt;
  const int whole = static_cast<int>(due);
  return whole + (random.Chance(due - whole) ? 1 : 0);
}

/// @brief Finds the live rock closest to a point.
///
/// It takes a `const entt::registry&`, so the view inside hands out const
/// components. Calling it from inside another view loop is fine, because it
/// only reads.
///
/// @param registry The registry to search.
/// @param from The point to measure from.
/// @return The nearest rock, or entt::null if there are none.
entt::entity NearestRock(const entt::registry& registry, Vector2 from) {
  entt::entity nearest = entt::null;
  // Comparing squared distances gives the same order as real distances and
  // skips a square root per rock.
  float nearest_distance_sq = 0.0f;
  registry.view<Rock, Transform>(entt::exclude<Dying>)
      .each([&](entt::entity rock, const Rock&, const Transform& transform) {
        const float dx = transform.position.x - from.x;
        const float dy = transform.position.y - from.y;
        const float distance_sq = dx * dx + dy * dy;
        if (nearest == entt::null || distance_sq < nearest_distance_sq) {
          nearest = rock;
          nearest_distance_sq = distance_sq;
        }
      });
  return nearest;
}

/// @brief Destroys a rock and queues its two smaller pieces, if it has any.
/// @param registry The registry the rock lives in.
/// @param random Source of the pieces' random shapes and drift.
/// @param rock The rock to break. Must have Rock, Transform and Velocity.
void BreakRock(entt::registry& registry, Random& random, entt::entity rock) {
  // Copy out what the pieces need now. After Flush() the rock and its
  // components are gone, so the deferred command can't read them then.
  const RockSize size = registry.get<Rock>(rock).size;
  const Vector2 position = registry.get<Transform>(rock).position;
  const Vector2 velocity = registry.get<Velocity>(rock).linear;
  Destroy(registry, rock);

  // Bigger rocks make bigger bursts of dust.
  const int dust = size == RockSize::kLarge    ? 18
                   : size == RockSize::kMedium ? 12
                                               : 8;
  Defer(registry, [position, velocity, dust, &random](entt::registry& r) {
    SpawnBurst(r, random, position, velocity, dust, kRockRimColor);
  });

  const std::optional<RockSize> smaller = SplitSize(size);
  if (!smaller) {
    // The smallest rocks sometimes leave a pickup behind. The random choices
    // are made now and captured by value, so the command needs no Random.
    if (random.Chance(kPickupChance)) {
      const PickupKind kind =
          random.Chance(0.5f) ? PickupKind::kMissiles : PickupKind::kShield;
      const Vector2 drift{velocity.x * 0.3f, velocity.y * 0.3f};
      Defer(registry, [position, drift, kind](entt::registry& r) {
        SpawnPickup(r, position, drift, kind);
      });
    }
    return;
  }
  const Vector2 inherited{velocity.x * kSplitInheritance,
                          velocity.y * kSplitInheritance};
  // This runs inside a view loop. EnTT would allow creating the pieces
  // right here, but then a second bullet could hit a piece in this same
  // frame. Deferring them to the next Flush() keeps the frame rule from
  // `main`: new entities appear after the systems have run.
  //
  // The lambda's capture list decides what it keeps:
  // - `piece_size`, `position` and `inherited` are captured by value
  //   (copied into the lambda), so they are still valid at Flush() time.
  // - `&random` is captured by reference, because the pieces must draw from
  //   the game's one generator, not from a copy. That reference is only safe
  //   because the Random (a Game member) outlives the next Flush(). Capturing
  //   a local variable by reference here would leave the lambda pointing at
  //   a dead variable: undefined behaviour.
  Defer(registry, [piece_size = *smaller, position, inherited,
                   &random](entt::registry& r) {
    SpawnRock(r, random, piece_size, position, inherited);
    SpawnRock(r, random, piece_size, position, inherited);
  });
}

}  // namespace

void ShipControlSystem(entt::registry& registry, const Input& input, float dt) {
  // The lambda is called once per ship. `[&]` captures `input` and `dt` by
  // reference, which is safe because each() calls the lambda right away,
  // while they are still alive. The unnamed `entt::entity` parameter means "I
  // don't need the handle".
  registry.view<Ship, Transform, Velocity>(entt::exclude<Dying>)
      .each([&](entt::entity, Ship& ship, Transform& transform,
                Velocity& velocity) {
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

void GunSystem(entt::registry& registry, const Input& input, float dt) {
  registry.view<Gun, Transform, Velocity>(entt::exclude<Dying>)
      .each([&](entt::entity, Gun& gun, const Transform& transform,
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
        Defer(registry,
              [nose, shot](entt::registry& r) { SpawnBullet(r, nose, shot); });
      });
}

void ExhaustSystem(entt::registry& registry, Random& random, float dt) {
  registry.view<Ship, Transform, Velocity>(entt::exclude<Dying>)
      .each([&](entt::entity, const Ship& ship, const Transform& transform,
                const Velocity& velocity) {
        if (!ship.thrusting) return;
        const int count = ParticlesDue(kExhaustRate, dt, random);
        if (count == 0) return;

        const Vector2 back = HeadingToDirection(transform.rotation + 180.0f);
        const Vector2 nozzle{
            transform.position.x + back.x * kShipRadius * 0.7f,
            transform.position.y + back.y * kShipRadius * 0.7f};
        const Vector2 base = velocity.linear;
        const float heading = transform.rotation;
        Defer(registry,
              [nozzle, base, heading, count, &random](entt::registry& r) {
                for (int i = 0; i < count; ++i) {
                  // Spray backwards in a narrow cone, 20 degrees either side.
                  const Vector2 spray = HeadingToDirection(
                      heading + 180.0f + random.Float(-20.0f, 20.0f));
                  const float speed = random.Float(80.0f, 160.0f);
                  SpawnParticle(r, nozzle,
                                Vector2{base.x + spray.x * speed,
                                        base.y + spray.y * speed},
                                random.Float(1.5f, 3.0f), kFlameColor,
                                random.Float(0.2f, 0.4f));
                }
              });
      });
}

void MissileLauncherSystem(entt::registry& registry, const Input& input,
                           float dt) {
  registry.view<MissileLauncher, Transform>(entt::exclude<Dying>)
      .each([&](entt::entity, MissileLauncher& launcher,
                const Transform& transform) {
        launcher.cooldown = std::max(0.0f, launcher.cooldown - dt);
        if (!input.fire_missile || launcher.missiles == 0 ||
            launcher.cooldown > 0) {
          return;
        }
        --launcher.missiles;
        launcher.cooldown = kMissileInterval;

        const Vector2 direction = HeadingToDirection(transform.rotation);
        const Vector2 nose{transform.position.x + direction.x * kShipRadius,
                           transform.position.y + direction.y * kShipRadius};
        const float heading = transform.rotation;
        // The target is picked now and its handle captured by value. If a
        // bullet breaks that rock later this frame, the missile is born with
        // a stale handle. That is fine: HomingSystem() checks IsAlive()
        // every frame and picks a new target.
        const entt::entity target = NearestRock(registry, transform.position);
        Defer(registry, [nose, heading, target](entt::registry& r) {
          SpawnMissile(r, nose, heading, target);
        });
      });
}

void HomingSystem(entt::registry& registry, float dt) {
  registry.view<Homing, Transform, Velocity>(entt::exclude<Dying>)
      .each([&](entt::entity, Homing& homing, Transform& transform,
                Velocity& velocity) {
        // The key ECS idea in this game: `homing.target` is a handle kept from
        // an earlier frame, and the rock it named may be gone. Its slot may
        // even hold a brand new entity by now. IsAlive() compares versions,
        // so it reports "gone" in both cases, and the missile picks a fresh
        // target.
        if (!IsAlive(registry, homing.target)) {
          homing.target = NearestRock(registry, transform.position);
        }
        // try_get() returns nullptr when there is no target (entt::null) or the
        // target has no Transform. Either way, fly straight on.
        const Transform* target = registry.try_get<Transform>(homing.target);
        if (target == nullptr) return;

        // Turn the velocity towards the target, but by no more than the turn
        // rate allows this frame, and keep the speed the same.
        const float speed = std::hypot(velocity.linear.x, velocity.linear.y);
        const float current = DirectionToHeading(velocity.linear);
        const float wanted = DirectionToHeading(
            Vector2{target->position.x - transform.position.x,
                    target->position.y - transform.position.y});
        const float max_turn = homing.turn_rate * dt;
        const float turn =
            std::clamp(ShortestTurn(wanted - current), -max_turn, max_turn);
        const Vector2 direction = HeadingToDirection(current + turn);
        velocity.linear = Vector2{direction.x * speed, direction.y * speed};
        transform.rotation = current + turn;
      });
}

void TrailSystem(entt::registry& registry, Random& random, float dt) {
  registry.view<TrailEmitter, Transform>(entt::exclude<Dying>)
      .each([&](entt::entity, const TrailEmitter& trail,
                const Transform& transform) {
        const int count = ParticlesDue(trail.rate, dt, random);
        if (count == 0) return;
        const Vector2 position = transform.position;
        const Color color = trail.color;
        Defer(registry, [position, color, count, &random](entt::registry& r) {
          for (int i = 0; i < count; ++i) {
            const Vector2 drift = random.Direction();
            const float speed = random.Float(5.0f, 25.0f);
            SpawnParticle(
                r, position, Vector2{drift.x * speed, drift.y * speed},
                random.Float(1.0f, 2.2f), color, random.Float(0.3f, 0.6f));
          }
        });
      });
}

void ShieldSystem(entt::registry& registry, float dt) {
  registry.view<Shield>(entt::exclude<Dying>)
      .each([&registry, dt](entt::entity entity, Shield& shield) {
        shield.seconds_left -= dt;
        if (shield.seconds_left > 0) return;
        // EnTT allows removing the current entity's components inside each(),
        // but it is deferred anyway, so the shield lasts until the end of the
        // frame like everything else. The ship may
        // be destroyed later this same frame, and then `entity` is no longer
        // alive when the command runs. The check guards against that; the
        // destroy removes the Shield anyway.
        Defer(registry, [entity](entt::registry& r) {
          if (IsAlive(r, entity)) r.remove<Shield>(entity);
        });
      });
}

void DragSystem(entt::registry& registry, float dt) {
  registry.view<Velocity, Drag>(entt::exclude<Dying>)
      .each([dt](entt::entity, Velocity& velocity, const Drag& drag) {
        // Each second, the velocity shrinks to exp(-rate) of what it was.
        // Using exp() instead of "multiply by 0.99 each frame" makes the
        // slowdown the same whatever the frame rate.
        const float keep = std::exp(-drag.rate * dt);
        velocity.linear.x *= keep;
        velocity.linear.y *= keep;
      });
}

void SpeedLimitSystem(entt::registry& registry) {
  registry.view<Velocity, SpeedLimit>(entt::exclude<Dying>)
      .each([](entt::entity, Velocity& velocity, const SpeedLimit& limit) {
        // std::hypot(x, y) is sqrt(x * x + y * y): the speed.
        const float speed = std::hypot(velocity.linear.x, velocity.linear.y);
        if (speed <= limit.max) return;
        const float scale = limit.max / speed;
        velocity.linear.x *= scale;
        velocity.linear.y *= scale;
      });
}

void MovementSystem(entt::registry& registry, float dt) {
  // `[dt]` captures only `dt`, by value: a copy of one float. Naming exactly
  // what a lambda captures documents what it depends on.
  registry.view<Transform, Velocity>(entt::exclude<Dying>)
      .each([dt](entt::entity, Transform& transform, const Velocity& velocity) {
        transform.position.x += velocity.linear.x * dt;
        transform.position.y += velocity.linear.y * dt;
        transform.rotation += velocity.angular * dt;
      });
}

void WrapSystem(entt::registry& registry) {
  registry.view<Transform, Wraps>(entt::exclude<Dying>)
      .each([](entt::entity, Transform& transform, const Wraps& wraps) {
        transform.position = WrapPosition(transform.position, wraps.margin);
      });
}

void LifetimeSystem(entt::registry& registry, float dt) {
  // `[&registry, dt]` captures the registry by reference (it must not be
  // copied, and copying is deleted anyway) and `dt` by value.
  registry.view<Lifetime>(entt::exclude<Dying>)
      .each([&registry, dt](entt::entity entity, Lifetime& lifetime) {
        lifetime.seconds_left -= dt;
        // Destroy() from ecs.h is safe inside each(): it only tags the entity.
        if (lifetime.seconds_left <= 0) Destroy(registry, entity);
      });
}

CollisionReport CollisionSystem(entt::registry& registry, Random& random) {
  CollisionReport report;

  // --- Projectiles against rocks ---
  // Two view loops nest: for every projectile, check every rock. That is
  // fine for a few dozen of each. Thousands would need a spatial grid so
  // that each projectile only checks nearby rocks.
  registry.view<Projectile, Transform, Collider>(entt::exclude<Dying>)
      .each(
          // Projectile is an empty "tag" type, so EnTT leaves it out of the
          // lambda's parameters: it only filters which entities are visited.
          [&](entt::entity shot, const Transform& shot_transform,
              const Collider& shot_collider) {
            registry.view<Rock, Transform, Collider>(entt::exclude<Dying>)
                .each([&](entt::entity rock, const Rock& rock_data,
                          const Transform& rock_transform,
                          const Collider& rock_collider) {
                  // each() has no `break`. Once this shot has hit a rock it is
                  // marked dying, so this check skips the remaining rocks.
                  if (!IsAlive(registry, shot)) return;
                  if (!CheckCollisionCircles(
                          shot_transform.position, shot_collider.radius,
                          rock_transform.position, rock_collider.radius)) {
                    return;
                  }
                  report.points += RockPoints(rock_data.size);
                  // Breaking marks the rock dying, so later shots in this loop
                  // skip it and can't score it twice.
                  BreakRock(registry, random, rock);
                  Destroy(registry, shot);
                });
          });

  // --- Rocks against the ship ---
  registry.view<Ship, Transform, Collider>(entt::exclude<Dying>)
      .each([&](entt::entity ship, const Ship& ship_data,
                const Transform& ship_transform,
                const Collider& ship_collider) {
        if (ship_data.invulnerable_for > 0 || registry.all_of<Shield>(ship)) {
          return;
        }
        registry.view<Rock, Transform, Collider>(entt::exclude<Dying>)
            .each([&](entt::entity rock, const Rock&,
                      const Transform& rock_transform,
                      const Collider& rock_collider) {
              if (!IsAlive(registry, ship)) return;
              if (!CheckCollisionCircles(
                      ship_transform.position, ship_collider.radius,
                      rock_transform.position, rock_collider.radius)) {
                return;
              }
              BreakRock(registry, random, rock);
              Destroy(registry, ship);
              report.ship_destroyed = true;

              // The ship explodes in its own colours. Its velocity is read
              // now, while the component still exists.
              const Vector2 position = ship_transform.position;
              const Vector2 velocity = registry.get<Velocity>(ship).linear;
              Defer(registry, [position, velocity, &random](entt::registry& r) {
                SpawnBurst(r, random, position, velocity, 40, kShipColor);
                SpawnBurst(r, random, position, velocity, 20, kFlameColor);
              });
            });
      });

  return report;
}

void PickupSystem(entt::registry& registry) {
  registry.view<Ship, Transform, Collider>(entt::exclude<Dying>)
      .each([&](entt::entity ship, const Ship&, const Transform& ship_transform,
                const Collider& ship_collider) {
        registry.view<Pickup, Transform, Collider>(entt::exclude<Dying>)
            .each([&](entt::entity pickup, const Pickup& pickup_data,
                      const Transform& pickup_transform,
                      const Collider& pickup_collider) {
              if (!CheckCollisionCircles(
                      ship_transform.position, ship_collider.radius,
                      pickup_transform.position, pickup_collider.radius)) {
                return;
              }
              Destroy(registry, pickup);
              switch (pickup_data.kind) {
                case PickupKind::kMissiles:
                  // Changing a component's value inside each() is fine. The
                  // `if` with an initializer declares `launcher` only for this
                  // `if`.
                  if (MissileLauncher* launcher =
                          registry.try_get<MissileLauncher>(ship);
                      launcher != nullptr) {
                    launcher->missiles += kMissilesPerPickup;
                  }
                  break;
                case PickupKind::kShield:
                  if (Shield* shield = registry.try_get<Shield>(ship);
                      shield != nullptr) {
                    // Already shielded: top it up in place.
                    shield->seconds_left = kShieldDuration;
                  } else {
                    // Adding a component is a structural change, so it waits
                    // for Flush(). The ship might die before then, hence the
                    // IsAlive() check on the captured handle.
                    Defer(registry, [ship](entt::registry& r) {
                      if (IsAlive(r, ship)) {
                        r.emplace<Shield>(
                            ship, Shield{.seconds_left = kShieldDuration});
                      }
                    });
                  }
                  break;
              }
            });
      });
}

}  // namespace asteroids
