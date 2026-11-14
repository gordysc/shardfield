// A small benchmark for the entity registry. It measures the three things a
// game does every frame: loop over a common pair of components, loop over a
// rare component, and create and destroy short-lived entities.
//
// Build it in Release mode, or the numbers mean little:
//   cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
//   cmake --build build-release --target registry_bench
//   ./build-release/registry_bench

#include <chrono>
#include <cstdio>
#include <vector>

#include "entity.h"
#include "registry.h"

namespace asteroids {
namespace {

/// @brief Stand-in for Transform: something most entities have.
struct Position {
  /// @brief Horizontal position.
  float x = 0;
  /// @brief Vertical position.
  float y = 0;
};

/// @brief Stand-in for Velocity: something most entities have.
struct Speed {
  /// @brief Horizontal speed.
  float x = 1;
  /// @brief Vertical speed.
  float y = 1;
};

/// @brief Stand-in for Rock: something only a few entities have.
struct Rare {
  /// @brief Any number.
  int value = 1;
};

/// @brief Entities with Position and Speed, like particles.
constexpr int kCommonCount = 20000;
/// @brief Entities that also have Rare, like rocks.
constexpr int kRareCount = 10;
/// @brief Times each measurement repeats, to smooth out noise.
constexpr int kRepeats = 200;
/// @brief Entities created and destroyed per repeat in the churn test.
constexpr int kChurnPerRepeat = 1000;

/// @brief Runs `fn` kRepeats times and returns the average time per run.
///
/// `Fn&&` is a forwarding reference, so any callable works without a copy.
/// steady_clock is the clock to use for measuring durations: unlike
/// system_clock, it never jumps when the computer's time is adjusted.
///
/// @tparam Fn The callable's type, deduced from the argument.
/// @param fn The work to time.
/// @return Microseconds per run.
template <typename Fn>
double MicrosecondsPerRun(Fn&& fn) {
  const auto start = std::chrono::steady_clock::now();
  for (int i = 0; i < kRepeats; ++i) fn();
  const auto elapsed = std::chrono::steady_clock::now() - start;
  // duration<double, std::micro> converts the elapsed time to microseconds
  // as a double, keeping the fraction.
  return std::chrono::duration<double, std::micro>(elapsed).count() / kRepeats;
}

}  // namespace
}  // namespace asteroids

/// @brief Fills a registry, times each pattern and prints the results.
/// @return 0 always.
int main() {
  using namespace asteroids;
  Registry registry;
  for (int i = 0; i < kCommonCount; ++i) {
    const Entity e = registry.Create();
    registry.Add(e, Position{});
    registry.Add(e, Speed{});
    // Spread the rare entities through the registry, the way rocks end up
    // scattered among thousands of particles.
    if (i % (kCommonCount / kRareCount) == 0) registry.Add(e, Rare{});
  }

  // `sum` collects a result from every loop and is printed at the end. That
  // stops the compiler from deleting loops whose results look unused.
  float sum = 0;

  const double common = MicrosecondsPerRun([&] {
    registry.Each<Position, Speed>([](Entity, Position& p, const Speed& s) {
      p.x += s.x;
      p.y += s.y;
    });
  });
  registry.Each<Position>([&sum](Entity, const Position& p) { sum += p.x; });

  const double rare = MicrosecondsPerRun([&] {
    registry.Each<Rare, Position>(
        [&sum](Entity, const Rare& r, const Position& p) {
          sum += static_cast<float>(r.value) + p.y;
        });
  });

  const double churn = MicrosecondsPerRun([&] {
    std::vector<Entity> made;
    made.reserve(kChurnPerRepeat);
    for (int i = 0; i < kChurnPerRepeat; ++i) {
      const Entity e = registry.Create();
      registry.Add(e, Position{});
      registry.Add(e, Speed{});
      made.push_back(e);
    }
    for (const Entity e : made) registry.Destroy(e);
    registry.Flush();
  });

  std::printf("Each<Position, Speed> over %d entities: %8.1f us\n",
              kCommonCount, common);
  std::printf("Each<Rare, Position>, %d of %d match:  %8.1f us\n", kRareCount,
              kCommonCount, rare);
  std::printf("Create + destroy %d entities:         %8.1f us\n",
              kChurnPerRepeat, churn);
  std::printf("(checksum %.0f)\n", sum);
  return 0;
}
