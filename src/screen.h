#ifndef ASTEROIDS_SCREEN_H_
#define ASTEROIDS_SCREEN_H_

// A header guard. The first time a .cpp file includes this header,
// ASTEROIDS_SCREEN_H_ is not defined yet, so the contents are compiled and
// the macro gets defined. If the same .cpp file includes the header again
// (often indirectly, through another header), the #ifndef is false and the
// contents are skipped, which avoids "redefinition" errors.

/// @brief Everything that belongs to the game.
namespace asteroids {

// `inline` (C++17) on a variable in a header means every .cpp file that
// includes the header shares one variable. Without it, each .cpp file would
// get its own private copy.

/// @brief Window width in pixels.
inline constexpr int kScreenWidth = 960;
/// @brief Window height in pixels.
inline constexpr int kScreenHeight = 720;

}  // namespace asteroids

#endif  // ASTEROIDS_SCREEN_H_
