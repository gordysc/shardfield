#include "render.h"

#include <raylib.h>

#include "components.h"
#include "draw.h"
#include "entity.h"
#include "registry.h"
#include "rock.h"
#include "tuning.h"

namespace asteroids {

void RenderSystem(const Registry& registry) {
  // Every Each() here is the const overload, because `registry` is a const
  // reference. The lambdas get const references to the components.
  registry.Each<Rock, Transform>(
      [](Entity, const Rock& rock, const Transform& transform) {
        DrawRock(transform.position, transform.rotation, RockRadius(rock.size),
                 rock.shape);
      });

  registry.Each<CircleSprite, Transform>(
      [](Entity, const CircleSprite& sprite, const Transform& transform) {
        DrawCircleV(transform.position, sprite.radius, sprite.color);
      });

  registry.Each<Ship, Transform>(
      [](Entity, const Ship& ship, const Transform& transform) {
        // While the ship can't be hurt it blinks: the timer, multiplied up
        // and cut to a whole number, flips between even and odd 8 times a
        // second.
        const bool dim = static_cast<int>(ship.invulnerable_for * 8) % 2 == 1;
        DrawShip(transform.position, transform.rotation, kShipRadius,
                 ship.thrusting, dim ? 0.3f : 1.0f);
      });
}

}  // namespace asteroids
