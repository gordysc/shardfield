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
/// @brief Fraction of the ship's radius used for rock collisions. Slightly
/// smaller than the drawn ship, so near misses feel fair.
constexpr float kShipHitScale = 0.8f;
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
/// @brief Seconds a new ship can't be hurt.
constexpr float kSpawnInvulnerability = 2.5f;
/// @brief Seconds between losing a life and the next ship appearing.
constexpr float kRespawnDelay = 1.5f;
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
/// @brief Number of lives the player starts each game with.
constexpr int kStartingLives = 3;
/// @brief Most large rocks a wave can start with.
constexpr int kMaxWaveRocks = 11;
/// @brief Seconds between clearing a wave and the next one starting.
constexpr float kWaveDelay = 2.0f;
/// @brief Seconds the "WAVE n" banner stays up.
constexpr float kBannerTime = 2.0f;
/// @brief Closest a new wave's rock may start to the ship, in pixels.
constexpr float kSafeSpawnDistance = 200.0f;
/// @brief Fraction of a broken rock's velocity that each piece keeps.
constexpr float kSplitInheritance = 0.5f;
/// @brief Font size in pixels for the score and smaller messages.
constexpr int kHudFontSize = 20;

}  // namespace

// The part after the colon is the member initializer list. It sets each data
// member as it is created, rather than creating it empty and then assigning
// to it in the body. Members are always initialized in the order they are
// declared in game.h, whatever order this list uses. That matters here:
// `stars_` uses `random_`, so `random_` must be declared first.
Game::Game(unsigned int seed)
    : random_(seed),
      stars_(MakeStars(random_)),
      ship_{},
      score_(0),
      lives_(kStartingLives),
      wave_(0),
      respawn_timer_(0),
      next_wave_timer_(0),
      banner_timer_(0),
      state_(GameState::kTitle) {
  // A few rocks drift behind the title screen. The ship isn't alive yet, so
  // it isn't drawn.
  SpawnRocks(kMaxWaveRocks / 2);
}

void Game::Update(float dt) {
  HandleStateKeys();
  // There is a `case` for every GameState and no `default`, so if a new state
  // is added later, the compiler warns (-Wswitch) that it is missing.
  switch (state_) {
    case GameState::kTitle:
    case GameState::kGameOver:
      // Two `case` labels in a row share one body. On these screens the
      // rocks keep drifting in the background, but nothing can collide.
      UpdateRocks(dt);
      break;
    case GameState::kPlaying:
      UpdatePlaying(dt);
      break;
    case GameState::kPaused:
      break;
  }
}

void Game::HandleStateKeys() {
  // IsKeyPressed is true only on the frame the key goes down, unlike
  // IsKeyDown, which stays true for as long as the key is held.
  switch (state_) {
    case GameState::kTitle:
    case GameState::kGameOver:
      if (IsKeyPressed(KEY_SPACE)) StartGame();
      break;
    case GameState::kPlaying:
      if (IsKeyPressed(KEY_P)) state_ = GameState::kPaused;
      break;
    case GameState::kPaused:
      if (IsKeyPressed(KEY_P)) state_ = GameState::kPlaying;
      break;
  }
}

void Game::StartGame() {
  score_ = 0;
  lives_ = kStartingLives;
  wave_ = 0;
  // clear() destroys every element but keeps the vector's memory, so the
  // next game's rocks and bullets don't need fresh allocations.
  bullets_.clear();
  rocks_.clear();
  SpawnShip();
  StartNextWave();
  state_ = GameState::kPlaying;
}

void Game::UpdatePlaying(float dt) {
  if (ship_.alive) {
    UpdateShip(dt);
    UpdateGun(dt);
  } else {
    respawn_timer_ -= dt;
    if (respawn_timer_ <= 0) SpawnShip();
  }
  UpdateBullets(dt);
  UpdateRocks(dt);
  HandleCollisions();

  banner_timer_ = std::max(0.0f, banner_timer_ - dt);
  if (rocks_.empty()) {
    next_wave_timer_ -= dt;
    if (next_wave_timer_ <= 0) StartNextWave();
  }
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
  ship_.invulnerable_for = std::max(0.0f, ship_.invulnerable_for - dt);
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

void Game::HandleCollisions() {
  // Pieces of broken rocks go into their own vector first. Pushing them
  // straight into `rocks_` inside the loops below would be a bug: if the
  // vector had to grow, it would move every rock to a new block of memory,
  // and the loop's `rock` reference would dangle (undefined behaviour).
  std::vector<Rock> pieces;

  // --- Bullets against rocks ---
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
      score_ += RockPoints(rock.size);
      BreakRock(rock, pieces);
      bullet.time_left = 0;
      // One bullet breaks at most one rock.
      break;
    }
  }

  // --- Rocks against the ship ---
  if (ship_.alive && ship_.invulnerable_for <= 0) {
    for (Rock& rock : rocks_) {
      if (rock.broken) continue;
      if (CheckCollisionCircles(ship_.position, kShipRadius * kShipHitScale,
                                rock.position, RockRadius(rock.size))) {
        BreakRock(rock, pieces);
        DestroyShip();
        break;
      }
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

void Game::BreakRock(Rock& rock, std::vector<Rock>& pieces) {
  // Both parameters are non-const references: the function changes the
  // caller's rock (marks it broken) and the caller's vector (adds pieces).
  rock.broken = true;
  // `if (init; condition)` (C++17) declares `smaller` just for this `if`. An
  // optional converts to true when it holds a value, and `*` reads it.
  if (const std::optional<RockSize> smaller = SplitSize(rock.size); smaller) {
    const Vector2 inherited{rock.velocity.x * kSplitInheritance,
                            rock.velocity.y * kSplitInheritance};
    pieces.push_back(MakeRock(*smaller, rock.position, inherited));
    pieces.push_back(MakeRock(*smaller, rock.position, inherited));
  }
}

void Game::DestroyShip() {
  ship_.alive = false;
  --lives_;
  if (lives_ == 0) {
    state_ = GameState::kGameOver;
  } else {
    respawn_timer_ = kRespawnDelay;
  }
}

void Game::SpawnShip() {
  // Designated initializers (C++20) name each field, so the order is easy
  // to check against the struct.
  ship_ = Ship{
      .position = kScreenCenter,
      .velocity = Vector2{0, 0},
      .heading = 0.0f,
      .thrusting = false,
      .fire_cooldown = 0.0f,
      .invulnerable_for = kSpawnInvulnerability,
      .alive = true,
  };
}

void Game::StartNextWave() {
  ++wave_;
  // std::min caps the count, so late waves don't fill the whole screen.
  SpawnRocks(std::min(3 + wave_, kMaxWaveRocks));
  next_wave_timer_ = kWaveDelay;
  banner_timer_ = kBannerTime;
}

Rock Game::MakeRock(RockSize size, Vector2 position, Vector2 base_velocity) {
  const Vector2 direction = random_.Direction();
  const float speed = RandomRockSpeed(size, random_);
  // Fields left out of the designated initializer (`broken`) get their
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

void Game::SpawnRocks(int count) {
  for (int i = 0; i < count; ++i) {
    // Pick random spots until one is far enough from the ship. A cap on the
    // attempts guarantees the loop ends even in some freak case.
    Vector2 position{};
    for (int attempt = 0; attempt < 32; ++attempt) {
      position = Vector2{random_.Float(0, kScreenWidth),
                         random_.Float(0, kScreenHeight)};
      if (std::hypot(position.x - kScreenCenter.x,
                     position.y - kScreenCenter.y) > kSafeSpawnDistance) {
        break;
      }
    }
    rocks_.push_back(MakeRock(RockSize::kLarge, position, Vector2{0, 0}));
  }
}

void Game::Draw() const {
  ClearBackground(kBackgroundColor);
  DrawStars(stars_);
  DrawField();
  if (state_ != GameState::kTitle) DrawHud();
  DrawMessage();
}

void Game::DrawField() const {
  // `const Rock&` reads each rock without copying it. It has to be const
  // here anyway: inside a const method, every data member is treated as
  // const.
  for (const Rock& rock : rocks_) {
    DrawRock(rock.position, rock.rotation, RockRadius(rock.size), rock.shape);
  }
  for (const Bullet& bullet : bullets_) {
    DrawCircleV(bullet.position, kBulletRadius, kBulletColor);
  }
  if (ship_.alive) {
    // While the ship can't be hurt it blinks: the timer, multiplied up and
    // cut to a whole number, flips between even and odd 8 times a second.
    const bool dim = static_cast<int>(ship_.invulnerable_for * 8) % 2 == 1;
    DrawShip(ship_.position, ship_.heading, kShipRadius, ship_.thrusting,
             dim ? 0.3f : 1.0f);
  }
}

void Game::DrawHud() const {
  // TextFormat works like printf: `%d` is replaced by an int argument. It
  // returns a pointer into a buffer that raylib owns and reuses, so use the
  // text straight away and never keep the pointer.
  DrawText(TextFormat("Score: %d", score_), 10, 10, kHudFontSize, RAYWHITE);
  DrawCenteredText(TextFormat("Wave %d", wave_), 10, kHudFontSize, GRAY);
  // One small ship per life left, along the top right.
  for (int i = 0; i < lives_; ++i) {
    DrawShip(Vector2{kScreenWidth - 20.0f - i * 24.0f, 22.0f}, 0.0f, 8.0f,
             false, 1.0f);
  }
}

void Game::DrawMessage() const {
  switch (state_) {
    case GameState::kTitle:
      DrawCenteredText("SHARDFIELD", 220, 64, RAYWHITE);
      DrawCenteredText("Press SPACE to start", 330, kHudFontSize, GRAY);
      DrawCenteredText("Arrows turn and thrust, SPACE fires, P pauses", 360,
                       kHudFontSize, GRAY);
      break;
    case GameState::kPlaying:
      if (banner_timer_ > 0) {
        DrawCenteredText(TextFormat("WAVE %d", wave_), 260, 40, RAYWHITE);
      }
      break;
    case GameState::kPaused:
      DrawCenteredText("PAUSED", 260, 40, RAYWHITE);
      DrawCenteredText("Press P to resume", 320, kHudFontSize, GRAY);
      break;
    case GameState::kGameOver:
      DrawCenteredText("GAME OVER", 240, 48, RED);
      DrawCenteredText(TextFormat("Final score: %d", score_), 310, kHudFontSize,
                       RAYWHITE);
      DrawCenteredText("Press SPACE to play again", 350, kHudFontSize, GRAY);
      break;
  }
}

}  // namespace asteroids
