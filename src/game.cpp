#include "game.h"

#include <raylib.h>

#include <cmath>

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
      ship_{kScreenCenter, Vector2{0, 0}, 0.0f, false} {}

void Game::Update(float dt) { UpdateShip(dt); }

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

void Game::Draw() const {
  ClearBackground(kBackgroundColor);
  DrawStars(stars_);
  DrawShip(ship_.position, ship_.heading, kShipRadius, ship_.thrusting, 1.0f);
  DrawText("Arrows: turn and thrust", 10, 10, 20, GRAY);
}

}  // namespace asteroids
