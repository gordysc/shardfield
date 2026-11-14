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

  // FadeOut and Lifetime are optional extras here, not requirements: the
  // loop asks for CircleSprite and Transform, then looks the extras up per
  // entity. TryGet() returns nullptr for an entity without one. Bullets have
  // no FadeOut, so they stay solid.
  registry.Each<CircleSprite, Transform>(
      [&registry](Entity entity, const CircleSprite& sprite,
                  const Transform& transform) {
        float alpha = 1.0f;
        const FadeOut* fade = registry.TryGet<FadeOut>(entity);
        const Lifetime* lifetime = registry.TryGet<Lifetime>(entity);
        if (fade != nullptr && lifetime != nullptr && fade->duration > 0) {
          alpha = lifetime->seconds_left / fade->duration;
        }
        DrawCircleV(transform.position, sprite.radius,
                    Fade(sprite.color, alpha));
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
