#include "spawn.h"

#include <raylib.h>

#include <cmath>

#include "components.h"
#include "ecs.h"
#include "geometry.h"
#include "random.h"
#include "rock.h"
#include "screen.h"
#include "tuning.h"

namespace asteroids {

entt::entity SpawnShip(entt::registry& registry, Vector2 position) {
  const entt::entity ship = registry.create();
  // Each Add() call attaches one component. The braces build a temporary
  // component, which Add() moves into the registry's storage.
  registry.emplace<Transform>(
      ship, Transform{.position = position, .rotation = 0.0f});
  registry.emplace<Velocity>(ship, Velocity{});
  registry.emplace<Drag>(ship, Drag{.rate = kShipDrag});
  registry.emplace<SpeedLimit>(ship, SpeedLimit{.max = kShipMaxSpeed});
  registry.emplace<Wraps>(ship, Wraps{.margin = kShipRadius});
  registry.emplace<Collider>(ship,
                             Collider{.radius = kShipRadius * kShipHitScale});
  registry.emplace<Ship>(ship, Ship{.thrusting = false,
                                    .invulnerable_for = kSpawnInvulnerability});
  registry.emplace<Gun>(ship, Gun{});
  registry.emplace<MissileLauncher>(ship, MissileLauncher{});
  return ship;
}

entt::entity SpawnBullet(entt::registry& registry, Vector2 position,
                         Vector2 velocity) {
  const entt::entity bullet = registry.create();
  registry.emplace<Transform>(bullet, Transform{.position = position});
  registry.emplace<Velocity>(bullet, Velocity{.linear = velocity});
  registry.emplace<Wraps>(bullet, Wraps{.margin = kBulletRadius});
  registry.emplace<Collider>(bullet, Collider{.radius = kBulletRadius});
  registry.emplace<Lifetime>(bullet, Lifetime{.seconds_left = kBulletLifetime});
  registry.emplace<Projectile>(bullet);
  registry.emplace<CircleSprite>(
      bullet, CircleSprite{.radius = kBulletRadius, .color = kBulletColor});
  return bullet;
}

entt::entity SpawnMissile(entt::registry& registry, Vector2 position,
                          float heading, entt::entity target) {
  const Vector2 direction = HeadingToDirection(heading);
  const entt::entity missile = registry.create();
  registry.emplace<Transform>(
      missile, Transform{.position = position, .rotation = heading});
  registry.emplace<Velocity>(missile,
                             Velocity{.linear = {direction.x * kMissileSpeed,
                                                 direction.y * kMissileSpeed}});
  registry.emplace<Wraps>(missile, Wraps{.margin = kMissileRadius});
  registry.emplace<Collider>(missile, Collider{.radius = kMissileRadius});
  registry.emplace<Lifetime>(missile,
                             Lifetime{.seconds_left = kMissileLifetime});
  // Projectile is the same tag bullets have, so CollisionSystem already
  // knows that missiles break rocks. No collision code changed for them.
  registry.emplace<Projectile>(missile);
  registry.emplace<Homing>(
      missile, Homing{.target = target, .turn_rate = kMissileTurnRate});
  registry.emplace<TrailEmitter>(
      missile, TrailEmitter{.rate = kMissileTrailRate, .color = kSmokeColor});
  registry.emplace<CircleSprite>(
      missile, CircleSprite{.radius = kMissileRadius, .color = kMissileColor});
  return missile;
}

entt::entity SpawnPickup(entt::registry& registry, Vector2 position,
                         Vector2 velocity, PickupKind kind) {
  const entt::entity pickup = registry.create();
  registry.emplace<Transform>(pickup, Transform{.position = position});
  registry.emplace<Velocity>(pickup, Velocity{.linear = velocity});
  registry.emplace<Wraps>(pickup, Wraps{.margin = kPickupRadius});
  registry.emplace<Collider>(pickup, Collider{.radius = kPickupRadius});
  registry.emplace<Lifetime>(pickup, Lifetime{.seconds_left = kPickupLifetime});
  registry.emplace<Pickup>(pickup, Pickup{.kind = kind});
  return pickup;
}

entt::entity SpawnRock(entt::registry& registry, Random& random, RockSize size,
                       Vector2 position, Vector2 base_velocity) {
  const Vector2 direction = random.Direction();
  const float speed = RandomRockSpeed(size, random);
  const float radius = RockRadius(size);

  const entt::entity rock = registry.create();
  registry.emplace<Transform>(
      rock, Transform{.position = position, .rotation = random.Float(0, 360)});
  registry.emplace<Velocity>(
      rock, Velocity{.linear = {base_velocity.x + direction.x * speed,
                                base_velocity.y + direction.y * speed},
                     .angular = random.Float(-60.0f, 60.0f)});
  registry.emplace<Wraps>(rock, Wraps{.margin = radius});
  registry.emplace<Collider>(rock, Collider{.radius = radius});
  registry.emplace<Rock>(rock,
                         Rock{.size = size, .shape = MakeRockShape(random)});
  return rock;
}

entt::entity SpawnParticle(entt::registry& registry, Vector2 position,
                           Vector2 velocity, float radius, Color color,
                           float lifetime) {
  const entt::entity particle = registry.create();
  registry.emplace<Transform>(particle, Transform{.position = position});
  registry.emplace<Velocity>(particle, Velocity{.linear = velocity});
  registry.emplace<Drag>(particle, Drag{.rate = kParticleDrag});
  registry.emplace<Lifetime>(particle, Lifetime{.seconds_left = lifetime});
  registry.emplace<CircleSprite>(
      particle, CircleSprite{.radius = radius, .color = color});
  registry.emplace<FadeOut>(particle, FadeOut{.duration = lifetime});
  return particle;
}

void SpawnBurst(entt::registry& registry, Random& random, Vector2 position,
                Vector2 base_velocity, int count, Color color) {
  for (int i = 0; i < count; ++i) {
    const Vector2 direction = random.Direction();
    const float speed = random.Float(40.0f, 220.0f);
    SpawnParticle(registry, position,
                  Vector2{base_velocity.x + direction.x * speed,
                          base_velocity.y + direction.y * speed},
                  random.Float(1.0f, 3.0f), color, random.Float(0.4f, 1.0f));
  }
}

void SpawnRocksAwayFrom(entt::registry& registry, Random& random, int count,
                        Vector2 avoid) {
  for (int i = 0; i < count; ++i) {
    // Pick random spots until one is far enough away. A cap on the attempts
    // guarantees the loop ends even in some freak case.
    Vector2 position{};
    for (int attempt = 0; attempt < 32; ++attempt) {
      position = Vector2{random.Float(0, kScreenWidth),
                         random.Float(0, kScreenHeight)};
      if (std::hypot(position.x - avoid.x, position.y - avoid.y) >
          kSafeSpawnDistance) {
        break;
      }
    }
    SpawnRock(registry, random, RockSize::kLarge, position, Vector2{0, 0});
  }
}

}  // namespace asteroids
