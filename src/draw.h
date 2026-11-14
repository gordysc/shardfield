#ifndef ASTEROIDS_DRAW_H_
#define ASTEROIDS_DRAW_H_

// Header guard; see screen.h for how it works.

#include <raylib.h>

#include <span>
#include <vector>

#include "random.h"

/// @brief Everything that belongs to the game.
namespace asteroids {

/// @brief Colour behind everything: a very dark blue.
inline constexpr Color kBackgroundColor{10, 12, 24, 255};
/// @brief Main colour of the player's ship.
inline constexpr Color kShipColor{80, 220, 200, 255};
/// @brief Colour of the ship's cockpit window.
inline constexpr Color kCockpitColor{220, 255, 250, 255};
/// @brief Colour of the ship's engine flame.
inline constexpr Color kFlameColor{255, 150, 60, 255};

/// @brief One background star.
struct Star {
  /// @brief Where the star is on screen.
  Vector2 position;
  /// @brief Size of the star in pixels.
  float radius;
  /// @brief How bright the star is, from 0 (invisible) to 255 (white).
  unsigned char brightness;
};

/// @brief Scatters background stars across the screen.
/// @param random Source of the random positions and sizes.
/// @return The stars, in no particular order.
std::vector<Star> MakeStars(Random& random);

/// @brief Draws the background stars.
///
/// std::span (C++20) is a non-owning view of a run of elements stored
/// somewhere else, here the caller's vector. It is just a pointer and a
/// length, so passing it by value is cheap, and it accepts a vector, an array
/// or a C array alike. Because it doesn't own the stars, the caller must
/// keep them alive while the function runs (which is automatic for a call).
///
/// @param stars The stars to draw.
void DrawStars(std::span<const Star> stars);

/// @brief Draws a filled triangle, whichever order its corners are in.
///
/// raylib skips triangles whose corners are listed clockwise on screen
/// ("backface culling"), so a triangle can silently vanish. This helper
/// checks the order and swaps two corners if needed.
///
/// @param a First corner.
/// @param b Second corner.
/// @param c Third corner.
/// @param color Fill colour.
void DrawTriangleAnyOrder(Vector2 a, Vector2 b, Vector2 c, Color color);

/// @brief Draws the player's ship.
/// @param position Centre of the ship.
/// @param heading Direction the nose points, in degrees clockwise from up.
/// @param radius Size of the ship; the nose sits a little past this.
/// @param thrusting Whether to draw the engine flame.
/// @param alpha Opacity, from 0 (invisible) to 1 (solid). Used to make the
///        ship blink while it can't be hit.
void DrawShip(Vector2 position, float heading, float radius, bool thrusting,
              float alpha);

/// @brief Draws a line of text centred horizontally on the screen.
///
/// raylib is a C library, so it takes text as `const char*` (a pointer to
/// null-terminated characters), not std::string. The pointer is only
/// borrowed: the function reads the text during the call and never frees it
/// or keeps it.
///
/// @param text The text to draw.
/// @param y Distance in pixels from the top of the window to the text.
/// @param font_size Height of the text in pixels.
/// @param color Colour of the text.
void DrawCenteredText(const char* text, int y, int font_size, Color color);

}  // namespace asteroids

#endif  // ASTEROIDS_DRAW_H_
