#ifndef ASTEROIDS_RENDER_H_
#define ASTEROIDS_RENDER_H_

// Header guard; see screen.h for how it works.

#include "registry.h"

/// @brief Everything that belongs to the game.
namespace asteroids {

/// @brief Draws every visible entity: rocks first, then pickups, circle
/// sprites (bullets, missiles and particles), ships, and shield rings on
/// top.
///
/// It takes a `const Registry&`, so it can only read the entities, never
/// change them, and the compiler enforces that. Call it between raylib's
/// BeginDrawing() and EndDrawing().
///
/// @param registry The entities to draw.
void RenderSystem(const Registry& registry);

}  // namespace asteroids

#endif  // ASTEROIDS_RENDER_H_
