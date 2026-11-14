#include "rock.h"

#include <optional>

#include "random.h"

namespace asteroids {

float RockRadius(RockSize size) {
  // There is a `case` for every RockSize and no `default`, so if a new size
  // is added later, the compiler warns (-Wswitch) that it is missing here.
  switch (size) {
    case RockSize::kSmall:
      return 14.0f;
    case RockSize::kMedium:
      return 26.0f;
    case RockSize::kLarge:
      return 48.0f;
  }
  // Every case above returns, so this line is never reached. It is here
  // because some compilers can't tell and warn "control reaches end of
  // non-void function" without it.
  return 0.0f;
}

int RockPoints(RockSize size) {
  switch (size) {
    case RockSize::kSmall:
      return 100;
    case RockSize::kMedium:
      return 50;
    case RockSize::kLarge:
      return 20;
  }
  return 0;
}

std::optional<RockSize> SplitSize(RockSize size) {
  switch (size) {
    case RockSize::kSmall:
      // std::nullopt is the "no value" state of any std::optional.
      return std::nullopt;
    case RockSize::kMedium:
      return RockSize::kSmall;
    case RockSize::kLarge:
      return RockSize::kMedium;
  }
  return std::nullopt;
}

float RandomRockSpeed(RockSize size, Random& random) {
  switch (size) {
    case RockSize::kSmall:
      return random.Float(90.0f, 160.0f);
    case RockSize::kMedium:
      return random.Float(60.0f, 110.0f);
    case RockSize::kLarge:
      return random.Float(30.0f, 70.0f);
  }
  return 0.0f;
}

RockShape MakeRockShape(Random& random) {
  RockShape shape{};
  // A range-based `for` with `float&` writes into the array itself.
  for (float& bump : shape) bump = random.Float(0.72f, 1.12f);
  return shape;
}

}  // namespace asteroids
