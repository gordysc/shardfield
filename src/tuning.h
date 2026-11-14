#ifndef ASTEROIDS_TUNING_H_
#define ASTEROIDS_TUNING_H_

// Header guard; see screen.h for how it works.

#include <raylib.h>

/// @brief Everything that belongs to the game.
namespace asteroids {

// Gameplay numbers, kept in one place so they are easy to find and tweak.
// Each is `inline constexpr`: a compile-time constant that every .cpp file
// shares (see screen.h).

/// @brief Size of the ship, used for drawing and for collisions.
inline constexpr float kShipRadius = 14.0f;
/// @brief Fraction of the ship's radius used for rock collisions. Slightly
/// smaller than the drawn ship, so near misses feel fair.
inline constexpr float kShipHitScale = 0.8f;
/// @brief How fast the ship turns, in degrees per second.
inline constexpr float kShipTurnSpeed = 270.0f;
/// @brief How fast the engine speeds the ship up, in pixels per second per
/// second.
inline constexpr float kShipThrust = 420.0f;
/// @brief Fastest the ship can fly, in pixels per second.
inline constexpr float kShipMaxSpeed = 420.0f;
/// @brief How quickly the ship drifts to a stop with the engine off. Higher
/// is quicker; see DragSystem().
inline constexpr float kShipDrag = 0.8f;
/// @brief Seconds a new ship can't be hurt.
inline constexpr float kSpawnInvulnerability = 2.5f;

/// @brief Seconds between shots while SPACE is held.
inline constexpr float kFireInterval = 0.18f;
/// @brief Bullet speed in pixels per second, on top of the ship's own speed.
inline constexpr float kBulletSpeed = 620.0f;
/// @brief Seconds a bullet flies before it fizzles out.
inline constexpr float kBulletLifetime = 0.9f;
/// @brief Radius of a bullet in pixels.
inline constexpr float kBulletRadius = 3.0f;
/// @brief Colour of a bullet.
inline constexpr Color kBulletColor{255, 230, 120, 255};

/// @brief Fraction of a broken rock's velocity that each piece keeps.
inline constexpr float kSplitInheritance = 0.5f;
/// @brief Closest a new wave's rock may start to the ship, in pixels.
inline constexpr float kSafeSpawnDistance = 200.0f;

/// @brief Number of lives the player starts each game with.
inline constexpr int kStartingLives = 3;
/// @brief Seconds between losing a life and the next ship appearing.
inline constexpr float kRespawnDelay = 1.5f;
/// @brief Most large rocks a wave can start with.
inline constexpr int kMaxWaveRocks = 11;
/// @brief Seconds between clearing a wave and the next one starting.
inline constexpr float kWaveDelay = 2.0f;
/// @brief Seconds the "WAVE n" banner stays up.
inline constexpr float kBannerTime = 2.0f;

}  // namespace asteroids

#endif  // ASTEROIDS_TUNING_H_
