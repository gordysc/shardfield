#include "render.h"

#include <raylib.h>

#include <algorithm>
#include <cmath>

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

  registry.Each<Pickup, Transform>([&registry](Entity entity,
                                               const Pickup& pickup,
                                               const Transform& transform) {
    // A pickup about to vanish blinks 4 times a second. GetTime() is seconds
    // since the window opened, and std::fmod(t, 0.25) is the time since the
    // last quarter-second mark. It is below 0.125 for the first half of
    // each quarter second, so the pickup is dim for that half.
    float alpha = 1.0f;
    const Lifetime* lifetime = registry.TryGet<Lifetime>(entity);
    if (lifetime != nullptr && lifetime->seconds_left < kPickupWarningTime &&
        std::fmod(GetTime(), 0.25) < 0.125) {
      alpha = 0.25f;
    }
    // There is a `case` for every PickupKind and no `default`, so a new
    // kind makes the compiler warn here until it gets a look.
    switch (pickup.kind) {
      case PickupKind::kMissiles:
        DrawPickup(transform.position, kPickupRadius, kMissilePickupColor, "M",
                   alpha);
        break;
      case PickupKind::kShield:
        DrawPickup(transform.position, kPickupRadius, kShieldColor, "S", alpha);
        break;
    }
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
        // DrawShip() needs the ship's own data, but the shield ring is drawn
        // in the next loop, which only visits shielded ships.
        DrawShip(transform.position, transform.rotation, kShipRadius,
                 ship.thrusting, dim ? 0.3f : 1.0f);
      });

  // Shield is a component that comes and goes, so "ships with a shield" is
  // just another query. The ring pulses by fading in and out over time.
  registry.Each<Shield, Transform>([](Entity, const Shield& shield,
                                      const Transform& transform) {
    const float pulse =
        0.55f + 0.25f * static_cast<float>(std::sin(GetTime() * 6.0));
    // Fade the ring away over the last second so its end isn't a
    // surprise.
    const float alpha = pulse * std::min(1.0f, shield.seconds_left);
    DrawRing(transform.position, kShipRadius * 1.6f, kShipRadius * 1.6f + 2.5f,
             0.0f, 360.0f, 32, Fade(kShieldColor, alpha));
  });
}

}  // namespace asteroids
