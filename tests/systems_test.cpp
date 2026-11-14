// Gameplay tests: they build small worlds by hand, run systems on them and
// check the result. Systems take an Input struct instead of reading the
// keyboard, so no window is needed.

#include "systems.h"

#include <raylib.h>

#include <cmath>
#include <vector>

#include "check.h"
#include "components.h"
#include "entity.h"
#include "input.h"
#include "random.h"
#include "registry.h"
#include "rock.h"
#include "screen.h"
#include "spawn.h"
#include "tuning.h"

namespace asteroids {
namespace {

/// @brief One 60 FPS frame, in seconds.
constexpr float kFrame = 1.0f / 60.0f;

/// @brief Checks that two floats are equal to within a small tolerance.
///
/// Floating-point maths rounds at every step, so results that "should" be
/// equal often differ in the last few bits. Comparing with == would make
/// the tests fail for no real reason.
///
/// @param a First value.
/// @param b Second value.
/// @return True if they differ by less than 0.001.
bool Near(float a, float b) { return std::fabs(a - b) < 0.001f; }

/// @brief Collects the sizes of every live rock.
/// @param registry The registry to look in.
/// @return One entry per rock, in registry order.
std::vector<RockSize> RockSizes(const Registry& registry) {
  std::vector<RockSize> sizes;
  registry.Each<Rock>(
      [&sizes](Entity, const Rock& rock) { sizes.push_back(rock.size); });
  return sizes;
}

/// @brief Moves a rock to a spot and stops it, so tests know where it is.
/// @param registry The registry the rock lives in.
/// @param rock The rock.
/// @param position Where to put it.
void PlaceRock(Registry& registry, Entity rock, Vector2 position) {
  registry.Get<Transform>(rock).position = position;
  registry.Get<Velocity>(rock) = Velocity{};
}

/// @brief MovementSystem moves by velocity times time.
void TestMovement() {
  Registry registry;
  const Entity e = registry.Create();
  registry.Add(e, Transform{.position = {100, 100}, .rotation = 0});
  registry.Add(e, Velocity{.linear = {60, -30}, .angular = 90});

  MovementSystem(registry, 0.5f);
  const Transform& t = registry.Get<Transform>(e);
  CHECK(Near(t.position.x, 130));
  CHECK(Near(t.position.y, 85));
  CHECK(Near(t.rotation, 45));
}

/// @brief WrapSystem moves an entity past the right edge to the left edge.
void TestWrap() {
  Registry registry;
  const Entity e = registry.Create();
  registry.Add(e, Transform{.position = {kScreenWidth + 11.0f, 50}});
  registry.Add(e, Wraps{.margin = 10});

  WrapSystem(registry);
  CHECK(registry.Get<Transform>(e).position.x < 0);
}

/// @brief LifetimeSystem destroys an entity once its time is up.
void TestLifetime() {
  Registry registry;
  const Entity e = registry.Create();
  registry.Add(e, Lifetime{.seconds_left = 0.25f});

  LifetimeSystem(registry, 0.2f);
  registry.Flush();
  CHECK(registry.IsAlive(e));

  LifetimeSystem(registry, 0.1f);
  registry.Flush();
  CHECK(!registry.IsAlive(e));
}

/// @brief Holding fire spawns one bullet, then waits for the cooldown.
void TestGunFiresAndCoolsDown() {
  Registry registry;
  SpawnShip(registry, kScreenCenter);
  const Input fire{.fire = true};

  GunSystem(registry, fire, kFrame);
  // The bullet is deferred, so it only exists after Flush().
  CHECK(registry.Count<Projectile>() == 0);
  registry.Flush();
  CHECK(registry.Count<Projectile>() == 1);

  // One frame later the gun is still cooling down.
  GunSystem(registry, fire, kFrame);
  registry.Flush();
  CHECK(registry.Count<Projectile>() == 1);

  // After the cooldown it fires again.
  GunSystem(registry, fire, kFireInterval);
  registry.Flush();
  CHECK(registry.Count<Projectile>() == 2);
}

/// @brief A bullet that hits a large rock breaks it into two medium rocks,
/// scores the large rock's points and is used up.
void TestBulletSplitsRock() {
  Registry registry;
  Random random(1);
  const Entity rock =
      SpawnRock(registry, random, RockSize::kLarge, {300, 300}, {0, 0});
  PlaceRock(registry, rock, {300, 300});
  const Entity bullet = SpawnBullet(registry, {300, 300}, {0, 0});

  const CollisionReport report = CollisionSystem(registry, random);
  registry.Flush();

  CHECK(report.points == RockPoints(RockSize::kLarge));
  CHECK(!report.ship_destroyed);
  CHECK(!registry.IsAlive(rock));
  CHECK(!registry.IsAlive(bullet));
  const std::vector<RockSize> sizes = RockSizes(registry);
  CHECK(sizes.size() == 2);
  for (const RockSize size : sizes) CHECK(size == RockSize::kMedium);
}

/// @brief Two bullets hitting one rock on the same frame break it once.
void TestTwoBulletsBreakRockOnce() {
  Registry registry;
  Random random(2);
  const Entity rock =
      SpawnRock(registry, random, RockSize::kMedium, {300, 300}, {0, 0});
  PlaceRock(registry, rock, {300, 300});
  const Entity first = SpawnBullet(registry, {295, 300}, {0, 0});
  const Entity second = SpawnBullet(registry, {305, 300}, {0, 0});

  const CollisionReport report = CollisionSystem(registry, random);
  registry.Flush();

  CHECK(report.points == RockPoints(RockSize::kMedium));
  CHECK(RockSizes(registry).size() == 2);
  // Exactly one bullet was used up; the other flies on.
  CHECK(registry.IsAlive(first) != registry.IsAlive(second));
}

/// @brief A small rock leaves no pieces.
void TestSmallRockVanishes() {
  Registry registry;
  Random random(3);
  const Entity rock =
      SpawnRock(registry, random, RockSize::kSmall, {300, 300}, {0, 0});
  PlaceRock(registry, rock, {300, 300});
  SpawnBullet(registry, {300, 300}, {0, 0});

  CollisionSystem(registry, random);
  registry.Flush();
  CHECK(registry.Count<Rock>() == 0);
}

/// @brief A rock destroys a ship that can be hurt, and breaks as well.
void TestRockDestroysShip() {
  Registry registry;
  Random random(4);
  const Entity ship = SpawnShip(registry, {300, 300});
  registry.Get<Ship>(ship).invulnerable_for = 0;
  const Entity rock =
      SpawnRock(registry, random, RockSize::kLarge, {300, 300}, {0, 0});
  PlaceRock(registry, rock, {300, 300});

  const CollisionReport report = CollisionSystem(registry, random);
  registry.Flush();

  CHECK(report.ship_destroyed);
  CHECK(report.points == 0);
  CHECK(!registry.IsAlive(ship));
  CHECK(RockSizes(registry).size() == 2);
}

/// @brief A freshly spawned ship can't be hurt.
void TestNewShipIsInvulnerable() {
  Registry registry;
  Random random(5);
  const Entity ship = SpawnShip(registry, {300, 300});
  const Entity rock =
      SpawnRock(registry, random, RockSize::kLarge, {300, 300}, {0, 0});
  PlaceRock(registry, rock, {300, 300});

  const CollisionReport report = CollisionSystem(registry, random);
  registry.Flush();

  CHECK(!report.ship_destroyed);
  CHECK(registry.IsAlive(ship));
  CHECK(registry.IsAlive(rock));
}

/// @brief Breaking a rock throws out dust particles, which are pure
/// composition: nothing but generic components, gone once their Lifetime
/// runs out.
void TestBrokenRockLeavesFadingDust() {
  Registry registry;
  Random random(6);
  const Entity rock =
      SpawnRock(registry, random, RockSize::kSmall, {300, 300}, {0, 0});
  PlaceRock(registry, rock, {300, 300});
  SpawnBullet(registry, {300, 300}, {0, 0});

  CollisionSystem(registry, random);
  registry.Flush();
  CHECK(registry.Count<FadeOut>() > 0);

  // No particle lives longer than a second.
  for (int i = 0; i < 70; ++i) {
    LifetimeSystem(registry, kFrame);
    registry.Flush();
  }
  CHECK(registry.Count<FadeOut>() == 0);
}

/// @brief Thrusting sprays exhaust particles; coasting doesn't.
void TestExhaustOnlyWhileThrusting() {
  Registry registry;
  Random random(7);
  SpawnShip(registry, kScreenCenter);

  ShipControlSystem(registry, Input{}, kFrame);
  ExhaustSystem(registry, random, kFrame);
  registry.Flush();
  CHECK(registry.Count<FadeOut>() == 0);

  ShipControlSystem(registry, Input{.thrust = true}, kFrame);
  ExhaustSystem(registry, random, kFrame);
  registry.Flush();
  CHECK(registry.Count<FadeOut>() > 0);
}

/// @brief The reason Entity has a generation. A missile's target rock is
/// destroyed and its slot is reused by a new entity. The missile must notice
/// that its old handle is dead instead of chasing the newcomer.
void TestMissileRetargetsWhenTargetDies() {
  Registry registry;
  Random random(8);
  const Entity first_target =
      SpawnRock(registry, random, RockSize::kLarge, {500, 300}, {0, 0});
  PlaceRock(registry, first_target, {500, 300});
  const Entity other_rock =
      SpawnRock(registry, random, RockSize::kLarge, {100, 600}, {0, 0});
  PlaceRock(registry, other_rock, {100, 600});
  const Entity missile = SpawnMissile(registry, {100, 300}, 90, first_target);

  registry.Destroy(first_target);
  registry.Flush();
  // The freed slot is reused straight away, by something that has a
  // Transform but isn't a rock.
  const Entity newcomer =
      SpawnParticle(registry, {900, 100}, {0, 0}, 1, WHITE, 5);
  CHECK(newcomer.index == first_target.index);
  CHECK(newcomer != first_target);

  HomingSystem(registry, kFrame);
  const Entity target = registry.Get<Homing>(missile).target;
  CHECK(target != newcomer);
  CHECK(target == other_rock);
}

/// @brief A missile turns towards its target, never faster than its turn
/// rate, and keeps its speed.
void TestMissileTurnsTowardsTarget() {
  Registry registry;
  Random random(9);
  const Entity rock =
      SpawnRock(registry, random, RockSize::kLarge, {400, 300}, {0, 0});
  PlaceRock(registry, rock, {400, 300});
  // Heading 0 is straight up; the rock is to the right.
  const Entity missile = SpawnMissile(registry, {100, 300}, 0, rock);

  HomingSystem(registry, kFrame);
  const Velocity& velocity = registry.Get<Velocity>(missile);
  CHECK(velocity.linear.x > 0);
  CHECK(Near(std::hypot(velocity.linear.x, velocity.linear.y), kMissileSpeed));
  CHECK(registry.Get<Transform>(missile).rotation <=
        kMissileTurnRate * kFrame + 0.001f);
}

/// @brief The launcher only fires when it has missiles, and uses them up.
void TestLauncherNeedsMissiles() {
  Registry registry;
  const Entity ship = SpawnShip(registry, kScreenCenter);
  const Input launch{.fire_missile = true};

  MissileLauncherSystem(registry, launch, kFrame);
  registry.Flush();
  CHECK(registry.Count<Homing>() == 0);

  registry.Get<MissileLauncher>(ship).missiles = 1;
  MissileLauncherSystem(registry, launch, kFrame);
  registry.Flush();
  CHECK(registry.Count<Homing>() == 1);
  CHECK(registry.Get<MissileLauncher>(ship).missiles == 0);
}

/// @brief Touching a missile pickup adds missiles and uses the pickup up.
void TestMissilePickup() {
  Registry registry;
  const Entity ship = SpawnShip(registry, {300, 300});
  const Entity pickup =
      SpawnPickup(registry, {305, 300}, {0, 0}, PickupKind::kMissiles);

  PickupSystem(registry);
  registry.Flush();
  CHECK(!registry.IsAlive(pickup));
  CHECK(registry.Get<MissileLauncher>(ship).missiles == kMissilesPerPickup);
}

/// @brief A shield pickup adds a Shield component, which blocks rocks until
/// ShieldSystem removes it.
void TestShieldComesAndGoes() {
  Registry registry;
  Random random(10);
  const Entity ship = SpawnShip(registry, {300, 300});
  registry.Get<Ship>(ship).invulnerable_for = 0;
  SpawnPickup(registry, {300, 300}, {0, 0}, PickupKind::kShield);

  PickupSystem(registry);
  // Adding a component is deferred, like creating an entity.
  CHECK(!registry.Has<Shield>(ship));
  registry.Flush();
  CHECK(registry.Has<Shield>(ship));

  const Entity rock =
      SpawnRock(registry, random, RockSize::kLarge, {300, 300}, {0, 0});
  PlaceRock(registry, rock, {300, 300});
  CHECK(!CollisionSystem(registry, random).ship_destroyed);
  registry.Flush();

  ShieldSystem(registry, kShieldDuration + 0.1f);
  registry.Flush();
  CHECK(!registry.Has<Shield>(ship));
  CHECK(CollisionSystem(registry, random).ship_destroyed);
  registry.Flush();
}

/// @brief A shield that runs out on the frame its ship is destroyed. The
/// deferred Remove() finds a dead handle and must skip it safely.
void TestShieldExpiryOnDeadShip() {
  Registry registry;
  const Entity ship = SpawnShip(registry, {300, 300});
  registry.Add(ship, Shield{.seconds_left = 0.01f});

  ShieldSystem(registry, kFrame);
  registry.Destroy(ship);
  registry.Flush();
  CHECK(!registry.IsAlive(ship));
  CHECK(registry.Count<Shield>() == 0);
}

/// @brief Thrust speeds the ship up, and SpeedLimitSystem caps it.
void TestThrustAndSpeedLimit() {
  Registry registry;
  const Entity ship = SpawnShip(registry, kScreenCenter);
  const Input thrust{.thrust = true};

  ShipControlSystem(registry, thrust, kFrame);
  // Heading 0 points up, which is negative y on screen.
  CHECK(registry.Get<Velocity>(ship).linear.y < 0);

  registry.Get<Velocity>(ship).linear = {0, -10 * kShipMaxSpeed};
  SpeedLimitSystem(registry);
  CHECK(Near(registry.Get<Velocity>(ship).linear.y, -kShipMaxSpeed));
}

}  // namespace
}  // namespace asteroids

/// @brief Runs every gameplay test.
/// @return 0 if all checks pass, otherwise 1.
int main() {
  using namespace asteroids;
  const std::vector<testing::Test> tests = {
      {"movement", TestMovement},
      {"wrap", TestWrap},
      {"lifetime", TestLifetime},
      {"gun fires and cools down", TestGunFiresAndCoolsDown},
      {"bullet splits rock", TestBulletSplitsRock},
      {"two bullets break a rock once", TestTwoBulletsBreakRockOnce},
      {"small rock vanishes", TestSmallRockVanishes},
      {"rock destroys ship", TestRockDestroysShip},
      {"new ship is invulnerable", TestNewShipIsInvulnerable},
      {"broken rock leaves fading dust", TestBrokenRockLeavesFadingDust},
      {"exhaust only while thrusting", TestExhaustOnlyWhileThrusting},
      {"missile retargets when target dies",
       TestMissileRetargetsWhenTargetDies},
      {"missile turns towards target", TestMissileTurnsTowardsTarget},
      {"launcher needs missiles", TestLauncherNeedsMissiles},
      {"missile pickup", TestMissilePickup},
      {"shield comes and goes", TestShieldComesAndGoes},
      {"shield expiry on dead ship", TestShieldExpiryOnDeadShip},
      {"thrust and speed limit", TestThrustAndSpeedLimit},
  };
  return testing::RunTests(tests);
}
