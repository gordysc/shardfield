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

/// @brief Shortest time between two missiles, in seconds.
inline constexpr float kMissileInterval = 0.3f;
/// @brief Missile speed in pixels per second.
inline constexpr float kMissileSpeed = 380.0f;
/// @brief Fastest a missile turns, in degrees per second.
inline constexpr float kMissileTurnRate = 240.0f;
/// @brief Seconds a missile flies before it burns out.
inline constexpr float kMissileLifetime = 3.0f;
/// @brief Radius of a missile in pixels.
inline constexpr float kMissileRadius = 4.0f;
/// @brief Colour of a missile.
inline constexpr Color kMissileColor{255, 96, 96, 255};
/// @brief Smoke particles a missile leaves per second.
inline constexpr float kMissileTrailRate = 60.0f;
/// @brief Colour of a missile's smoke trail.
inline constexpr Color kSmokeColor{200, 200, 215, 255};

/// @brief Chance that a broken small rock drops a pickup, from 0 to 1.
inline constexpr float kPickupChance = 0.15f;
/// @brief Seconds a pickup waits to be collected before it vanishes.
inline constexpr float kPickupLifetime = 10.0f;
/// @brief Seconds before vanishing that a pickup starts to blink.
inline constexpr float kPickupWarningTime = 3.0f;
/// @brief Radius of a pickup in pixels.
inline constexpr float kPickupRadius = 12.0f;
/// @brief Missiles a missile pickup gives.
inline constexpr int kMissilesPerPickup = 3;
/// @brief Seconds a shield pickup protects the ship.
inline constexpr float kShieldDuration = 6.0f;
/// @brief Colour of a missile pickup.
inline constexpr Color kMissilePickupColor{255, 120, 120, 255};
/// @brief Colour of a shield pickup and of the shield ring.
inline constexpr Color kShieldColor{120, 180, 255, 255};

/// @brief How quickly particles slow down; see DragSystem().
inline constexpr float kParticleDrag = 2.5f;
/// @brief Exhaust particles the engine makes per second while thrusting.
inline constexpr float kExhaustRate = 90.0f;

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
