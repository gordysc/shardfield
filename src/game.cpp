#include "game.h"

#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <optional>
#include <vector>

#include "draw.h"
#include "geometry.h"
#include "rock.h"
#include "screen.h"

namespace asteroids {

// Helpers visible only in this file; see draw.cpp.
namespace {

/// @brief Size of the ship, used for drawing and for collisions.
constexpr float kShipRadius = 14.0f;
/// @brief How fast the ship turns, in degrees per second.
constexpr float kShipTurnSpeed = 270.0f;
/// @brief How fast the engine speeds the ship up, in pixels per second per
/// second.
constexpr float kShipThrust = 420.0f;
/// @brief Fastest the ship can fly, in pixels per second.
constexpr float kShipMaxSpeed = 420.0f;
/// @brief How quickly the ship drifts to a stop with the engine off. Higher
/// is quicker; see UpdateShip().
constexpr float kShipDrag = 0.8f;
/// @brief Seconds between shots while SPACE is held.
constexpr float kFireInterval = 0.18f;
/// @brief Bullet speed in pixels per second, on top of the ship's own speed.
constexpr float kBulletSpeed = 620.0f;
/// @brief Seconds a bullet flies before it fizzles out.
constexpr float kBulletLifetime = 0.9f;
/// @brief Radius of a bullet in pixels.
constexpr float kBulletRadius = 3.0f;
/// @brief Colour of a bullet.
constexpr Color kBulletColor{255, 230, 120, 255};
/// @brief Where the ship starts: the middle of the screen.
constexpr Vector2 kScreenCenter{kScreenWidth / 2.0f, kScreenHeight / 2.0f};
/// @brief How many large rocks the first wave has.
constexpr int kFirstWaveRocks = 4;
/// @brief Closest a new wave's rock may start to the ship, in pixels.
constexpr float kSafeSpawnDistance = 200.0f;
/// @brief Fraction of a broken rock's velocity that each piece keeps.
constexpr float kSplitInheritance = 0.5f;

}  // namespace

// The part after the colon is the member initializer list. It sets each data
// member as it is created, rather than creating it empty and then assigning
// to it in the body. Members are always initialized in the order they are
// declared in game.h, whatever order this list uses. That matters here:
// `stars_` uses `random_`, so `random_` must be declared first.
Game::Game(unsigned int seed)
    : random_(seed),
      stars_(MakeStars(random_)),
      ship_{kScreenCenter, Vector2{0, 0}, 0.0f, false, 0.0f},
      score_(0) {
  SpawnWave(kFirstWaveRocks);
}

void Game::Update(float dt) {
  UpdateShip(dt);
  UpdateGun(dt);
  UpdateBullets(dt);
  UpdateRocks(dt);
  HandleBulletHits();
  if (rocks_.empty()) SpawnWave(kFirstWaveRocks);
}

void Game::UpdateShip(float dt) {
  // IsKeyDown stays true for as long as the key is held, so turning and
  // thrusting are smooth. Multiplying by `dt` makes the speed the same at
  // any frame rate.
  if (IsKeyDown(KEY_LEFT)) ship_.heading -= kShipTurnSpeed * dt;
  if (IsKeyDown(KEY_RIGHT)) ship_.heading += kShipTurnSpeed * dt;

  ship_.thrusting = IsKeyDown(KEY_UP);
  if (ship_.thrusting) {
    const Vector2 direction = HeadingToDirection(ship_.heading);
    ship_.velocity.x += direction.x * kShipThrust * dt;
    ship_.velocity.y += direction.y * kShipThrust * dt;
  }

  // Drag: each second, the velocity shrinks to exp(-kShipDrag) of what it
  // was. Using exp() instead of "multiply by 0.99 each frame" makes the
  // slowdown the same whatever the frame rate.
  const float keep = std::exp(-kShipDrag * dt);
  ship_.velocity.x *= keep;
  ship_.velocity.y *= keep;

  // Cap the speed. std::hypot(x, y) is sqrt(x * x + y * y): the length of
  // the velocity, which is the ship's speed.
  const float speed = std::hypot(ship_.velocity.x, ship_.velocity.y);
  if (speed > kShipMaxSpeed) {
    const float scale = kShipMaxSpeed / speed;
    ship_.velocity.x *= scale;
    ship_.velocity.y *= scale;
  }

  ship_.position.x += ship_.velocity.x * dt;
  ship_.position.y += ship_.velocity.y * dt;
  ship_.position = WrapPosition(ship_.position, kShipRadius);
}

void Game::UpdateGun(float dt) {
  // std::max (from <algorithm>) stops the cooldown going below zero.
  ship_.fire_cooldown = std::max(0.0f, ship_.fire_cooldown - dt);
  if (!IsKeyDown(KEY_SPACE) || ship_.fire_cooldown > 0) return;

  ship_.fire_cooldown = kFireInterval;
  const Vector2 direction = HeadingToDirection(ship_.heading);
  // The bullet starts at the ship's nose and adds the ship's own velocity,
  // so shots fired while flying forwards don't crawl out of the gun.
  const Vector2 nose{ship_.position.x + direction.x * kShipRadius,
                     ship_.position.y + direction.y * kShipRadius};
  const Vector2 velocity{ship_.velocity.x + direction.x * kBulletSpeed,
                         ship_.velocity.y + direction.y * kBulletSpeed};
  // push_back copies the temporary Bullet into the vector. If the vector is
  // full it first grows: it allocates a bigger block and moves every bullet
  // across. Any pointer or reference to an old element would then dangle,
  // which is why we never hold on to `&bullets_[i]` across frames.
  bullets_.push_back(Bullet{nose, velocity, kBulletLifetime});
}

void Game::UpdateBullets(float dt) {
  // `Bullet&` is a reference to the bullet inside the vector, so changes
  // stick. A plain `Bullet` would be a copy, and the change would be lost.
  for (Bullet& bullet : bullets_) {
    bullet.position.x += bullet.velocity.x * dt;
    bullet.position.y += bullet.velocity.y * dt;
    bullet.position = WrapPosition(bullet.position, kBulletRadius);
    bullet.time_left -= dt;
  }
}

void Game::UpdateRocks(float dt) {
  for (Rock& rock : rocks_) {
    rock.position.x += rock.velocity.x * dt;
    rock.position.y += rock.velocity.y * dt;
    rock.position = WrapPosition(rock.position, RockRadius(rock.size));
    rock.rotation += rock.spin * dt;
  }
}

void Game::HandleBulletHits() {
  // Pieces of broken rocks go into their own vector first. Pushing them
  // straight into `rocks_` inside the loop below would be a bug: if the
  // vector had to grow, it would move every rock to a new block of memory,
  // and the loop's `rock` reference would dangle (undefined behaviour).
  std::vector<Rock> pieces;

  for (Bullet& bullet : bullets_) {
    if (bullet.time_left <= 0) continue;
    for (Rock& rock : rocks_) {
      // Skipping broken rocks stops two bullets that land on the same frame
      // from breaking one rock twice (and scoring it twice).
      if (rock.broken) continue;
      if (!CheckCollisionCircles(bullet.position, kBulletRadius, rock.position,
                                 RockRadius(rock.size))) {
        continue;
      }
      rock.broken = true;
      bullet.time_left = 0;
      score_ += RockPoints(rock.size);

      // `if (init; condition)` (C++17) declares `smaller` just for this
      // `if`. An optional converts to true when it holds a value, and `*`
      // reads that value.
      if (const std::optional<RockSize> smaller = SplitSize(rock.size);
          smaller) {
        const Vector2 inherited{rock.velocity.x * kSplitInheritance,
                                rock.velocity.y * kSplitInheritance};
        pieces.push_back(MakeRock(*smaller, rock.position, inherited));
        pieces.push_back(MakeRock(*smaller, rock.position, inherited));
      }
      // One bullet breaks at most one rock.
      break;
    }
  }

  // std::erase_if (C++20) removes every element the lambda returns true for,
  // in one pass. Erasing inside the loops above would have shifted elements
  // and invalidated the loops' iterators, which is undefined behaviour.
  std::erase_if(bullets_,
                [](const Bullet& bullet) { return bullet.time_left <= 0; });
  std::erase_if(rocks_, [](const Rock& rock) { return rock.broken; });

  // Now that no loop is walking `rocks_`, it is safe to grow it. insert()
  // with a pair of iterators copies the whole range in one go.
  rocks_.insert(rocks_.end(), pieces.begin(), pieces.end());
}

Rock Game::MakeRock(RockSize size, Vector2 position, Vector2 base_velocity) {
  const Vector2 direction = random_.Direction();
  const float speed = RandomRockSpeed(size, random_);
  // Designated initializers (C++20) name each field, so the order is easy
  // to check against the struct. Fields left out (`broken`) get their
  // default member initializer.
  return Rock{
      .position = position,
      .velocity = Vector2{base_velocity.x + direction.x * speed,
                          base_velocity.y + direction.y * speed},
      .rotation = random_.Float(0, 360),
      .spin = random_.Float(-60.0f, 60.0f),
      .size = size,
      .shape = MakeRockShape(random_),
  };
}

void Game::SpawnWave(int count) {
  for (int i = 0; i < count; ++i) {
    // Pick random spots until one is far enough from the ship. A cap on the
    // attempts guarantees the loop ends even in some freak case.
    Vector2 position{};
    for (int attempt = 0; attempt < 32; ++attempt) {
      position = Vector2{random_.Float(0, kScreenWidth),
                         random_.Float(0, kScreenHeight)};
      if (std::hypot(position.x - ship_.position.x,
                     position.y - ship_.position.y) > kSafeSpawnDistance) {
        break;
      }
    }
    rocks_.push_back(MakeRock(RockSize::kLarge, position, Vector2{0, 0}));
  }
}

void Game::Draw() const {
  ClearBackground(kBackgroundColor);
  DrawStars(stars_);
  for (const Rock& rock : rocks_) {
    DrawRock(rock.position, rock.rotation, RockRadius(rock.size), rock.shape);
  }
  for (const Bullet& bullet : bullets_) {
    DrawCircleV(bullet.position, kBulletRadius, kBulletColor);
  }
  DrawShip(ship_.position, ship_.heading, kShipRadius, ship_.thrusting, 1.0f);
  // TextFormat works like printf: `%d` is replaced by an int argument. It
  // returns a pointer into a buffer that raylib owns and reuses, so use the
  // text straight away and never keep the pointer.
  DrawText(TextFormat("Score: %d", score_), 10, 10, 20, RAYWHITE);
}

}  // namespace asteroids
