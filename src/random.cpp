// A .cpp file includes its own header first. If the header forgot to include
// something it needs, this file then fails to compile, so the mistake is
// caught here rather than in some other file.
#include "random.h"

#include <raylib.h>

#include <cstdint>
#include <random>

#include "geometry.h"

namespace asteroids {

Random::Random(std::uint32_t seed) : engine_(seed) {}

float Random::Float(float min, float max) {
  // A "distribution" turns the engine's raw 32-bit numbers into values with
  // the shape we want. Distributions are cheap to create, so making one per
  // call is fine. uniform_real_distribution gives min <= value < max.
  std::uniform_real_distribution<float> distribution(min, max);
  return distribution(engine_);
}

int Random::Int(int min, int max) {
  // Note the asymmetry with Float(): uniform_int_distribution includes `max`.
  std::uniform_int_distribution<int> distribution(min, max);
  return distribution(engine_);
}

bool Random::Chance(float probability) {
  return Float(0.0f, 1.0f) < probability;
}

Vector2 Random::Direction() { return HeadingToDirection(Float(0, 360)); }

}  // namespace asteroids
