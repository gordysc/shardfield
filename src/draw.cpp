#include "draw.h"

#include <raylib.h>

#include <array>
#include <cmath>
#include <span>
#include <utility>
#include <vector>

#include "geometry.h"
#include "random.h"
#include "rock.h"
#include "screen.h"

namespace asteroids {

// An unnamed namespace makes everything inside it visible only in this .cpp
// file (internal linkage), so these names can't clash with names in other
// files. Google style uses it for helpers that don't belong in the header.
namespace {

/// @brief How many background stars to scatter.
constexpr int kStarCount = 140;

/// @brief Turns a point in the ship's local coordinates into a screen point.
/// @param local Point relative to the ship's centre, with the ship facing up.
/// @param position Centre of the ship on screen.
/// @param heading The ship's heading in degrees.
/// @return The point on screen.
Vector2 ToScreen(Vector2 local, Vector2 position, float heading) {
  const Vector2 rotated = RotatePoint(local, heading);
  return Vector2{position.x + rotated.x, position.y + rotated.y};
}

}  // namespace

std::vector<Star> MakeStars(Random& random) {
  // `Random&` is a reference: the function uses the caller's generator
  // itself, not a copy. A copy would hand out the same numbers again the
  // next time the caller used its own generator.
  std::vector<Star> stars;
  // reserve() allocates room for every star up front, so push_back never has
  // to grow the vector (allocate a bigger block and move everything) mid-loop.
  stars.reserve(kStarCount);
  for (int i = 0; i < kStarCount; ++i) {
    // static_cast is the C++ way to convert between number types on purpose.
    // It is easier to search for and stricter than a C-style `(float)x`.
    stars.push_back(Star{
        Vector2{random.Float(0, kScreenWidth), random.Float(0, kScreenHeight)},
        random.Float(0.6f, 1.6f),
        static_cast<unsigned char>(random.Int(60, 200))});
  }
  // Returning a local vector by value is cheap: the compiler either builds it
  // directly in the caller's variable (copy elision) or moves it.
  return stars;
}

void DrawStars(std::span<const Star> stars) {
  // `const Star&` reads each star in place without copying it.
  for (const Star& star : stars) {
    const unsigned char b = star.brightness;
    DrawCircleV(star.position, star.radius, Color{b, b, b, 255});
  }
}

void DrawTriangleAnyOrder(Vector2 a, Vector2 b, Vector2 c, Color color) {
  // The cross product of the edges a->b and a->c tells us the corner order.
  // raylib wants counter-clockwise order as seen on screen, and because
  // screen y grows downwards, that order gives a negative cross product.
  const float cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
  // std::swap exchanges two variables' values.
  if (cross > 0) std::swap(b, c);
  DrawTriangle(a, b, c, color);
}

void DrawShip(Vector2 position, float heading, float radius, bool thrusting,
              float alpha) {
  // The ship is a dart shape: a nose, two swept-back wings and a notch
  // between them. Each point is in local coordinates, scaled by the radius.
  const float r = radius;
  const Vector2 nose = ToScreen({0, -1.3f * r}, position, heading);
  const Vector2 right_wing = ToScreen({0.9f * r, r}, position, heading);
  const Vector2 notch = ToScreen({0, 0.5f * r}, position, heading);
  const Vector2 left_wing = ToScreen({-0.9f * r, r}, position, heading);

  if (thrusting) {
    // GetTime() is seconds since the window opened. Feeding it through sin()
    // makes the flame length flicker many times a second.
    const float flicker = 0.9f + 0.4f * std::sin(GetTime() * 40.0);
    DrawTriangleAnyOrder(
        ToScreen({-0.4f * r, 0.65f * r}, position, heading),
        ToScreen({0.4f * r, 0.65f * r}, position, heading),
        ToScreen({0, (0.65f + flicker) * r}, position, heading),
        Fade(kFlameColor, alpha));
  }

  // The dart is concave (it dents in at the notch), and a triangle can only
  // draw a convex shape, so it is drawn as two triangles that share the
  // nose-to-notch edge.
  const Color body = Fade(kShipColor, alpha);
  DrawTriangleAnyOrder(nose, right_wing, notch, body);
  DrawTriangleAnyOrder(nose, notch, left_wing, body);
  DrawCircleV(ToScreen({0, -0.2f * r}, position, heading), 0.22f * r,
              Fade(kCockpitColor, alpha));
}

void DrawRock(Vector2 position, float rotation, float radius,
              const RockShape& shape) {
  // Work out every corner on screen first. std::array has a fixed size known
  // at compile time, so it lives on the stack: no heap allocation per rock
  // per frame, unlike a std::vector would need.
  std::array<Vector2, kRockVertices> corners{};
  for (int i = 0; i < kRockVertices; ++i) {
    // Index loop rather than range-based `for`, because the index `i` is
    // what decides each corner's angle.
    const float angle = rotation + i * (360.0f / kRockVertices);
    const Vector2 offset = RotatePoint({0, -radius * shape[i]}, angle);
    corners[i] = Vector2{position.x + offset.x, position.y + offset.y};
  }

  // Fill: a fan of triangles from the centre to each pair of neighbouring
  // corners. The bumps can make the outline slightly concave, which a single
  // convex polygon couldn't draw, but a fan from the centre handles it.
  // `(i + 1) % kRockVertices` wraps the last corner back round to the first.
  for (int i = 0; i < kRockVertices; ++i) {
    DrawTriangleAnyOrder(position, corners[i], corners[(i + 1) % kRockVertices],
                         kRockColor);
  }
  for (int i = 0; i < kRockVertices; ++i) {
    DrawLineEx(corners[i], corners[(i + 1) % kRockVertices], 2.0f,
               kRockRimColor);
  }
}

void DrawPickup(Vector2 position, float radius, Color color, const char* letter,
                float alpha) {
  const Color faded = Fade(color, alpha);
  // A ring is the area between two circles, drawn from 0 to 360 degrees in
  // 24 straight segments.
  DrawRing(position, radius - 3.0f, radius, 0.0f, 360.0f, 24, faded);
  constexpr int kLetterSize = 16;
  const int width = MeasureText(letter, kLetterSize);
  // static_cast<int> drops the fraction, because DrawText() takes whole
  // pixel positions.
  DrawText(letter, static_cast<int>(position.x) - width / 2,
           static_cast<int>(position.y) - kLetterSize / 2, kLetterSize, faded);
}

void DrawCenteredText(const char* text, int y, int font_size, Color color) {
  // MeasureText returns how many pixels wide the text will be at this size.
  const int x = (kScreenWidth - MeasureText(text, font_size)) / 2;
  DrawText(text, x, y, font_size, color);
}

}  // namespace asteroids
