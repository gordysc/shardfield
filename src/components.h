#ifndef ASTEROIDS_COMPONENTS_H_
#define ASTEROIDS_COMPONENTS_H_

// Header guard; see screen.h for how it works.

#include <raylib.h>

#include "rock.h"

/// @brief Everything that belongs to the game.
namespace asteroids {

// Components are plain data: small structs with no methods and no
// behaviour. Behaviour lives in systems (systems.h), which loop over every
// entity that has a certain set of components.
//
// An entity "is" whatever its components make it. Nothing says "this is a
// bullet" except that it has Projectile, Transform, Velocity, Collider,
// Lifetime and CircleSprite. Give a rock a Lifetime and it would fade out of
// existence like a bullet, with no new code.

/// @brief Where an entity is and which way it faces.
struct Transform {
  /// @brief Centre of the entity on screen.
  Vector2 position{};
  /// @brief Facing, in degrees clockwise from straight up.
  float rotation = 0.0f;
};

/// @brief How an entity moves each second. Used by MovementSystem().
struct Velocity {
  /// @brief Movement in pixels per second.
  Vector2 linear{};
  /// @brief Spin in degrees per second.
  float angular = 0.0f;
};

/// @brief Slows an entity down over time. Used by DragSystem().
struct Drag {
  /// @brief How quickly the entity slows. Each second its velocity shrinks
  /// to exp(-rate) of what it was.
  float rate = 0.0f;
};

/// @brief Caps an entity's speed. Used by SpeedLimitSystem().
struct SpeedLimit {
  /// @brief Fastest the entity may move, in pixels per second.
  float max = 0.0f;
};

/// @brief Makes an entity reappear at the opposite edge when it leaves the
/// screen. Used by WrapSystem().
struct Wraps {
  /// @brief How far past an edge, in pixels, the entity may go before it
  /// wraps. Usually its radius, so it slides fully off before reappearing.
  float margin = 0.0f;
};

/// @brief A circle used for collisions. Used by CollisionSystem().
struct Collider {
  /// @brief Radius of the circle in pixels.
  float radius = 0.0f;
};

/// @brief Destroys the entity when the time runs out. Used by
/// LifetimeSystem().
struct Lifetime {
  /// @brief Seconds left before the entity is destroyed.
  float seconds_left = 0.0f;
};

/// @brief Marks the player's ship and holds its own state. Ship entities
/// are steered by ShipControlSystem() and drawn as the ship shape.
struct Ship {
  /// @brief Whether the engine fired this frame. Only used for drawing.
  bool thrusting = false;
  /// @brief Seconds until rocks can hurt the ship again.
  float invulnerable_for = 0.0f;
};

/// @brief A gun that fires bullets from the entity's nose. Used by
/// GunSystem().
struct Gun {
  /// @brief Seconds until the gun can fire again. 0 means ready.
  float cooldown = 0.0f;
};

/// @brief A "tag" component: it has no data and only marks the entity as
/// something that breaks rocks on contact.
///
/// An empty struct still takes 1 byte in C++, because every object needs
/// its own address. That is fine here.
struct Projectile {};

/// @brief Marks a rock and holds what makes each rock different.
struct Rock {
  /// @brief Which size of rock this is.
  RockSize size = RockSize::kLarge;
  /// @brief The rock's bumpy outline.
  RockShape shape{};
};

/// @brief Draws the entity as a filled circle. Used by RenderSystem().
struct CircleSprite {
  /// @brief Radius of the circle in pixels.
  float radius = 0.0f;
  /// @brief Fill colour.
  Color color{};
};

}  // namespace asteroids

#endif  // ASTEROIDS_COMPONENTS_H_
