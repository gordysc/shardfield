#include "game.h"

#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <vector>

#include "draw.h"
#include "geometry.h"
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

}  // namespace

// The part after the colon is the member initializer list. It sets each data
// member as it is created, rather than creating it empty and then assigning
// to it in the body. Members are always initialized in the order they are
// declared in game.h, whatever order this list uses. That matters here:
// `stars_` uses `random_`, so `random_` must be declared first.
Game::Game(unsigned int seed)
    : random_(seed),
      stars_(MakeStars(random_)),
      ship_{kScreenCenter, Vector2{0, 0}, 0.0f, false, 0.0f} {}

void Game::Update(float dt) {
  UpdateShip(dt);
  UpdateGun(dt);
  UpdateBullets(dt);
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

  // Erasing from a vector while a range-based `for` walks over it is
  // undefined behaviour, because erasing shifts later elements and
  // invalidates the loop's iterators. So the loop above only counts down,
  // and std::erase_if (C++20) removes every finished bullet afterwards in
  // one safe pass. The lambda is the test: it gets each bullet and returns
  // true for the ones to remove.
  std::erase_if(bullets_,
                [](const Bullet& bullet) { return bullet.time_left <= 0; });
}

void Game::Draw() const {
  ClearBackground(kBackgroundColor);
  DrawStars(stars_);
  for (const Bullet& bullet : bullets_) {
    DrawCircleV(bullet.position, kBulletRadius, kBulletColor);
  }
  DrawShip(ship_.position, ship_.heading, kShipRadius, ship_.thrusting, 1.0f);
  DrawText("Arrows: turn and thrust, SPACE: fire", 10, 10, 20, GRAY);
}

}  // namespace asteroids
