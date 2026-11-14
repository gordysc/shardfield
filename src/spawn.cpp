#include "spawn.h"

#include <raylib.h>

#include <cmath>

#include "components.h"
#include "entity.h"
#include "geometry.h"
#include "random.h"
#include "registry.h"
#include "rock.h"
#include "screen.h"
#include "tuning.h"

namespace asteroids {

Entity SpawnShip(Registry& registry, Vector2 position) {
  const Entity ship = registry.Create();
  // Each Add() call attaches one component. The braces build a temporary
  // component, which Add() moves into the registry's storage.
  registry.Add(ship, Transform{.position = position, .rotation = 0.0f});
  registry.Add(ship, Velocity{});
  registry.Add(ship, Drag{.rate = kShipDrag});
  registry.Add(ship, SpeedLimit{.max = kShipMaxSpeed});
  registry.Add(ship, Wraps{.margin = kShipRadius});
  registry.Add(ship, Collider{.radius = kShipRadius * kShipHitScale});
  registry.Add(ship, Ship{.thrusting = false,
                          .invulnerable_for = kSpawnInvulnerability});
  registry.Add(ship, Gun{});
  registry.Add(ship, MissileLauncher{});
  return ship;
}

Entity SpawnBullet(Registry& registry, Vector2 position, Vector2 velocity) {
  const Entity bullet = registry.Create();
  registry.Add(bullet, Transform{.position = position});
  registry.Add(bullet, Velocity{.linear = velocity});
  registry.Add(bullet, Wraps{.margin = kBulletRadius});
  registry.Add(bullet, Collider{.radius = kBulletRadius});
  registry.Add(bullet, Lifetime{.seconds_left = kBulletLifetime});
  registry.Add(bullet, Projectile{});
  registry.Add(bullet,
               CircleSprite{.radius = kBulletRadius, .color = kBulletColor});
  return bullet;
}

Entity SpawnMissile(Registry& registry, Vector2 position, float heading,
                    Entity target) {
  const Vector2 direction = HeadingToDirection(heading);
  const Entity missile = registry.Create();
  registry.Add(missile, Transform{.position = position, .rotation = heading});
  registry.Add(missile, Velocity{.linear = {direction.x * kMissileSpeed,
                                            direction.y * kMissileSpeed}});
  registry.Add(missile, Wraps{.margin = kMissileRadius});
  registry.Add(missile, Collider{.radius = kMissileRadius});
  registry.Add(missile, Lifetime{.seconds_left = kMissileLifetime});
  // Projectile is the same tag bullets have, so CollisionSystem already
  // knows that missiles break rocks. No collision code changed for them.
  registry.Add(missile, Projectile{});
  registry.Add(missile,
               Homing{.target = target, .turn_rate = kMissileTurnRate});
  registry.Add(missile,
               TrailEmitter{.rate = kMissileTrailRate, .color = kSmokeColor});
  registry.Add(missile,
               CircleSprite{.radius = kMissileRadius, .color = kMissileColor});
  return missile;
}

Entity SpawnPickup(Registry& registry, Vector2 position, Vector2 velocity,
                   PickupKind kind) {
  const Entity pickup = registry.Create();
  registry.Add(pickup, Transform{.position = position});
  registry.Add(pickup, Velocity{.linear = velocity});
  registry.Add(pickup, Wraps{.margin = kPickupRadius});
  registry.Add(pickup, Collider{.radius = kPickupRadius});
  registry.Add(pickup, Lifetime{.seconds_left = kPickupLifetime});
  registry.Add(pickup, Pickup{.kind = kind});
  return pickup;
}

Entity SpawnRock(Registry& registry, Random& random, RockSize size,
                 Vector2 position, Vector2 base_velocity) {
  const Vector2 direction = random.Direction();
  const float speed = RandomRockSpeed(size, random);
  const float radius = RockRadius(size);

  const Entity rock = registry.Create();
  registry.Add(
      rock, Transform{.position = position, .rotation = random.Float(0, 360)});
  registry.Add(rock, Velocity{.linear = {base_velocity.x + direction.x * speed,
                                         base_velocity.y + direction.y * speed},
                              .angular = random.Float(-60.0f, 60.0f)});
  registry.Add(rock, Wraps{.margin = radius});
  registry.Add(rock, Collider{.radius = radius});
  registry.Add(rock, Rock{.size = size, .shape = MakeRockShape(random)});
  return rock;
}

Entity SpawnParticle(Registry& registry, Vector2 position, Vector2 velocity,
                     float radius, Color color, float lifetime) {
  const Entity particle = registry.Create();
  registry.Add(particle, Transform{.position = position});
  registry.Add(particle, Velocity{.linear = velocity});
  registry.Add(particle, Drag{.rate = kParticleDrag});
  registry.Add(particle, Lifetime{.seconds_left = lifetime});
  registry.Add(particle, CircleSprite{.radius = radius, .color = color});
  registry.Add(particle, FadeOut{.duration = lifetime});
  return particle;
}

void SpawnBurst(Registry& registry, Random& random, Vector2 position,
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

void SpawnRocksAwayFrom(Registry& registry, Random& random, int count,
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
