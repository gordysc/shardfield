// Tests for the small layer in ecs.h that sits on top of EnTT: deferred
// destruction, deferred commands, and the version check behind IsAlive().
// On `main`, registry_test.cpp tests the hand-made Registry the same way.

#include "ecs.h"

#include <array>
#include <entt/entity/registry.hpp>
#include <vector>

#include "check.h"

namespace asteroids {
namespace {

/// @brief A test component with one value.
struct Position {
  /// @brief Any number; the tests only compare it.
  float x = 0;
};

/// @brief A component with no data: a "tag" that only marks an entity.
struct Tag {};

/// @brief Destroy() takes effect at once for IsAlive() and views that
/// exclude Dying, but the entity and its components stay until Flush().
void TestDestroyIsDeferredUntilFlush() {
  entt::registry registry;
  InitWorld(registry);
  const entt::entity e = registry.create();
  registry.emplace<Position>(e, Position{1});

  Destroy(registry, e);
  CHECK(!IsAlive(registry, e));
  CHECK(registry.valid(e));
  CHECK(registry.all_of<Position>(e));
  CHECK(CountAlive<Position>(registry) == 0);

  Flush(registry);
  CHECK(!registry.valid(e));

  // Destroying twice, or destroying a gone entity, is harmless.
  Destroy(registry, e);
  Flush(registry);
  CHECK(!IsAlive(registry, e));
}

/// @brief EnTT reuses a freed index with a new version, so an old handle
/// stays dead and the new entity starts with no components.
void TestReusedIndexInvalidatesOldHandles() {
  entt::registry registry;
  InitWorld(registry);
  const entt::entity old_entity = registry.create();
  registry.emplace<Position>(old_entity, Position{42});
  Destroy(registry, old_entity);
  Flush(registry);

  const entt::entity new_entity = registry.create();
  CHECK(entt::to_entity(new_entity) == entt::to_entity(old_entity));
  CHECK(entt::to_version(new_entity) != entt::to_version(old_entity));
  CHECK(IsAlive(registry, new_entity));
  CHECK(!IsAlive(registry, old_entity));
  CHECK(!registry.all_of<Position>(new_entity));
  CHECK(!IsAlive(registry, entt::null));
}

/// @brief Destroying other entities inside a view loop is safe through
/// Destroy(), and the view skips them from then on.
void TestDestroyInsideView() {
  entt::registry registry;
  InitWorld(registry);
  std::array<entt::entity, 4> entities{};
  for (entt::entity& e : entities) {
    e = registry.create();
    registry.emplace<Position>(e, Position{1});
  }
  int visits = 0;
  // The view decides the visiting order, so the first visit marks every
  // *other* entity, whichever entity that is.
  registry.view<Position>(entt::exclude<Dying>)
      .each([&](entt::entity e, Position&) {
        ++visits;
        if (visits > 1) return;
        for (const entt::entity other : entities) {
          if (other != e) Destroy(registry, other);
        }
      });
  CHECK(visits == 1);
  Flush(registry);
  CHECK(CountAlive<Position>(registry) == 1);
}

/// @brief Deferred commands run at Flush() and may queue more commands.
void TestDeferRunsAtFlush() {
  entt::registry registry;
  InitWorld(registry);
  const entt::entity spawner = registry.create();
  registry.emplace<Tag>(spawner);

  // Tag is empty, so EnTT passes only the entity to the lambda.
  registry.view<Tag>(entt::exclude<Dying>).each([&registry](entt::entity) {
    Defer(registry, [](entt::registry& r) {
      const entt::entity child = r.create();
      r.emplace<Position>(child, Position{9});
      Defer(r, [child](entt::registry& r2) { r2.get<Position>(child).x = 10; });
    });
  });
  CHECK(CountAlive<Position>(registry) == 0);

  Flush(registry);
  CHECK(CountAlive<Position>(registry) == 1);
  registry.view<Position>().each(
      [](entt::entity, const Position& p) { CHECK(p.x == 10); });
}

/// @brief ClearWorld() destroys everything and drops queued commands.
void TestClearWorld() {
  entt::registry registry;
  InitWorld(registry);
  const entt::entity e = registry.create();
  registry.emplace<Position>(e, Position{1});
  Defer(registry, [](entt::registry& r) { r.emplace<Position>(r.create()); });

  ClearWorld(registry);
  CHECK(!IsAlive(registry, e));
  Flush(registry);
  CHECK(CountAlive<Position>(registry) == 0);
  const entt::entity next = registry.create();
  CHECK(next != e);
}

}  // namespace
}  // namespace asteroids

/// @brief Runs every ecs.h test.
/// @return 0 if all checks pass, otherwise 1.
int main() {
  using namespace asteroids;
  const std::vector<testing::Test> tests = {
      {"destroy is deferred until flush", TestDestroyIsDeferredUntilFlush},
      {"reused index invalidates old handles",
       TestReusedIndexInvalidatesOldHandles},
      {"destroy inside view", TestDestroyInsideView},
      {"defer runs at flush", TestDeferRunsAtFlush},
      {"clear world", TestClearWorld},
  };
  return testing::RunTests(tests);
}
