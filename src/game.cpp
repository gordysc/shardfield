#include "game.h"

#include <raylib.h>

#include <algorithm>

#include "components.h"
#include "draw.h"
#include "input.h"
#include "render.h"
#include "screen.h"
#include "spawn.h"
#include "systems.h"
#include "tuning.h"

namespace asteroids {

// Helpers visible only in this file; see draw.cpp.
namespace {

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
      score_(0),
      lives_(kStartingLives),
      wave_(0),
      respawn_timer_(0),
      next_wave_timer_(0),
      banner_timer_(0),
      state_(GameState::kTitle) {
  // `registry_` isn't in the list, so its default constructor runs: it
  // starts empty. InitWorld() adds the command queue from ecs.h. Then a
  // few rocks drift behind the title screen.
  InitWorld(registry_);
  SpawnRocksAwayFrom(registry_, random_, kMaxWaveRocks / 2, kScreenCenter);
}

void Game::Update(float dt, const Input& input) {
  HandleStateKeys(input);
  // There is a `case` for every GameState and no `default`, so if a new state
  // is added later, the compiler warns (-Wswitch) that it is missing.
  switch (state_) {
    case GameState::kTitle:
    case GameState::kGameOver:
      // Two `case` labels in a row share one body. On these screens things
      // keep drifting in the background, but nothing collides.
      UpdateMotion(dt);
      Flush(registry_);
      break;
    case GameState::kPlaying:
      UpdatePlaying(dt, input);
      break;
    case GameState::kPaused:
      break;
  }
}

void Game::HandleStateKeys(const Input& input) {
  switch (state_) {
    case GameState::kTitle:
    case GameState::kGameOver:
      if (input.start) StartGame();
      break;
    case GameState::kPlaying:
      if (input.pause) state_ = GameState::kPaused;
      break;
    case GameState::kPaused:
      if (input.pause) state_ = GameState::kPlaying;
      break;
  }
}

void Game::StartGame() {
  score_ = 0;
  lives_ = kStartingLives;
  wave_ = 0;
  ClearWorld(registry_);
  SpawnShip(registry_, kScreenCenter);
  StartNextWave();
  state_ = GameState::kPlaying;
}

void Game::UpdatePlaying(float dt, const Input& input) {
  // The order matters. Controls change velocities, motion turns velocities
  // into new positions, and collisions then see where everything ended up.
  ShipControlSystem(registry_, input, dt);
  GunSystem(registry_, input, dt);
  MissileLauncherSystem(registry_, input, dt);
  ExhaustSystem(registry_, random_, dt);
  HomingSystem(registry_, dt);
  TrailSystem(registry_, random_, dt);
  ShieldSystem(registry_, dt);
  UpdateMotion(dt);
  const CollisionReport report = CollisionSystem(registry_, random_);
  PickupSystem(registry_);
  // Apply everything the systems queued: new bullets, rock pieces and
  // particles, and the destruction of whatever was hit or ran out of time.
  Flush(registry_);

  score_ += report.points;
  if (report.ship_destroyed) {
    --lives_;
    if (lives_ == 0) {
      state_ = GameState::kGameOver;
      return;
    }
    respawn_timer_ = kRespawnDelay;
  }

  // The Game doesn't keep a handle to the ship. It asks the registry
  // whether one exists, which also covers the frames between losing a life
  // and the next ship appearing.
  if (CountAlive<Ship>(registry_) == 0) {
    respawn_timer_ -= dt;
    if (respawn_timer_ <= 0) SpawnShip(registry_, kScreenCenter);
  }

  banner_timer_ = std::max(0.0f, banner_timer_ - dt);
  if (CountAlive<Rock>(registry_) == 0) {
    next_wave_timer_ -= dt;
    if (next_wave_timer_ <= 0) StartNextWave();
  }
}

void Game::UpdateMotion(float dt) {
  DragSystem(registry_, dt);
  SpeedLimitSystem(registry_);
  MovementSystem(registry_, dt);
  WrapSystem(registry_);
  LifetimeSystem(registry_, dt);
}

void Game::StartNextWave() {
  ++wave_;
  // std::min caps the count, so late waves don't fill the whole screen.
  SpawnRocksAwayFrom(registry_, random_, std::min(3 + wave_, kMaxWaveRocks),
                     kScreenCenter);
  next_wave_timer_ = kWaveDelay;
  banner_timer_ = kBannerTime;
}

void Game::Draw() const {
  ClearBackground(kBackgroundColor);
  DrawStars(stars_);
  // `registry_` is const inside this const method, so this calls the
  // RenderSystem that takes a `const entt::registry&`.
  RenderSystem(registry_);
  if (state_ != GameState::kTitle) DrawHud();
  DrawMessage();
}

void Game::DrawHud() const {
  // TextFormat works like printf: `%d` is replaced by an int argument. It
  // returns a pointer into a buffer that raylib owns and reuses, so use the
  // text straight away and never keep the pointer.
  DrawText(TextFormat("Score: %d", score_), 10, 10, kHudFontSize, RAYWHITE);
  DrawCenteredText(TextFormat("Wave %d", wave_), 10, kHudFontSize, GRAY);
  // The HUD asks the registry for the launcher instead of keeping its own
  // copy of the count, so it can never be out of date.
  int missiles = 0;
  registry_.view<MissileLauncher>(entt::exclude<Dying>)
      .each([&missiles](entt::entity, const MissileLauncher& launcher) {
        missiles += launcher.missiles;
      });
  DrawText(TextFormat("Missiles: %d", missiles), 10, 36, kHudFontSize,
           kMissilePickupColor);
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
      DrawCenteredText("SHIFT launches a homing missile, if you have one", 390,
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
