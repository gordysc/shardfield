#ifndef ASTEROIDS_GEOMETRY_H_
#define ASTEROIDS_GEOMETRY_H_

// Header guard; see screen.h for how it works.

#include <raylib.h>

#include <cmath>

#include "screen.h"

/// @brief Everything that belongs to the game.
namespace asteroids {

// These helpers are short, so they live in the header as `inline` functions.
// `inline` lets every .cpp file that includes this header compile its own
// copy without the linker complaining about duplicate definitions, and it
// lets the compiler paste the body straight into the caller.

/// @brief Turns a heading into a unit vector (a direction of length 1).
///
/// A heading of 0 points straight up the screen, and the heading grows
/// clockwise, so 90 points right. Screen y grows downwards, which is why the
/// y part is negated.
///
/// @param degrees The heading in degrees.
/// @return The direction the heading points in.
inline Vector2 HeadingToDirection(float degrees) {
  // std::sin and std::cos work in radians. DEG2RAD is a raylib macro equal
  // to PI / 180, which converts degrees to radians.
  const float radians = degrees * DEG2RAD;
  return Vector2{std::sin(radians), -std::cos(radians)};
}

/// @brief Rotates a point around the origin.
///
/// Shapes are described in "local" coordinates, where the shape's centre is
/// (0, 0) and it faces up. Rotating each local point by the shape's heading
/// and adding its position gives the point on screen.
///
/// @param point The point to rotate.
/// @param degrees How far to rotate, clockwise on screen.
/// @return The rotated point.
inline Vector2 RotatePoint(Vector2 point, float degrees) {
  const float radians = degrees * DEG2RAD;
  const float s = std::sin(radians);
  const float c = std::cos(radians);
  return Vector2{point.x * c - point.y * s, point.x * s + point.y * c};
}

/// @brief Moves a position that has left the screen to the opposite edge.
///
/// `margin` lets an object slide fully off one edge before it appears at the
/// other, so it doesn't visibly pop from one side to the other.
///
/// @param position The position to wrap.
/// @param margin How far past an edge, in pixels, the position may go before
///        it wraps.
/// @return The wrapped position.
inline Vector2 WrapPosition(Vector2 position, float margin) {
  // Vector2 is just two floats, so taking it by value (a copy) is as cheap as
  // taking a reference, and it lets us change `position` without touching
  // the caller's variable.
  const float width = kScreenWidth + 2 * margin;
  const float height = kScreenHeight + 2 * margin;
  if (position.x < -margin) position.x += width;
  if (position.x > kScreenWidth + margin) position.x -= width;
  if (position.y < -margin) position.y += height;
  if (position.y > kScreenHeight + margin) position.y -= height;
  return position;
}

}  // namespace asteroids

#endif  // ASTEROIDS_GEOMETRY_H_
