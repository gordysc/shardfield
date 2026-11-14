// Unit tests for the entity registry. They only use plain structs as
// components, so they need no window and no raylib.

#include "registry.h"

#include <array>
#include <vector>

#include "check.h"
#include "entity.h"

namespace asteroids {
namespace {

/// @brief A test component with one value.
struct Position {
  /// @brief Any number; the tests only compare it.
  float x = 0;
};

/// @brief A second test component, so tests can ask for "both".
struct Speed {
  /// @brief Any number; the tests only compare it.
  float value = 0;
};

/// @brief A component with no data: a "tag" that only marks an entity.
struct Tag {};

/// @brief New entities are alive and get different handles.
void TestCreateGivesDistinctLiveEntities() {
  Registry registry;
  const Entity a = registry.Create();
  const Entity b = registry.Create();
  CHECK(a != b);
  CHECK(registry.IsAlive(a));
  CHECK(registry.IsAlive(b));
  CHECK(!registry.IsAlive(kNullEntity));
}

/// @brief Added components can be read, changed and removed.
void TestAddGetRemove() {
  Registry registry;
  const Entity e = registry.Create();
  CHECK(!registry.Has<Position>(e));
  CHECK(registry.TryGet<Position>(e) == nullptr);

  registry.Add(e, Position{3});
  CHECK(registry.Has<Position>(e));
  CHECK(registry.Get<Position>(e).x == 3);

  // Get() returns a reference, so writing through it changes the stored
  // component.
  registry.Get<Position>(e).x = 5;
  CHECK(registry.Get<Position>(e).x == 5);

  // Adding the same type again replaces the old component.
  registry.Add(e, Position{7});
  CHECK(registry.Get<Position>(e).x == 7);

  registry.Remove<Position>(e);
  CHECK(!registry.Has<Position>(e));
}

/// @brief Destroy() takes effect at once for IsAlive() and Each(), but the
/// components stay readable until Flush().
void TestDestroyIsDeferredUntilFlush() {
  Registry registry;
  const Entity e = registry.Create();
  registry.Add(e, Position{1});

  registry.Destroy(e);
  CHECK(!registry.IsAlive(e));
  CHECK(registry.Has<Position>(e));
  CHECK(registry.Count<Position>() == 0);

  registry.Flush();
  CHECK(!registry.IsAlive(e));
  CHECK(!registry.Has<Position>(e));

  // Destroying twice, or destroying a gone entity, is harmless.
  registry.Destroy(e);
  registry.Flush();
  CHECK(!registry.IsAlive(e));
}

/// @brief A reused slot gets a new generation, so old handles stay dead and
/// the new entity starts with no components.
void TestReusedSlotsInvalidateOldHandles() {
  Registry registry;
  const Entity old_entity = registry.Create();
  registry.Add(old_entity, Position{42});
  registry.Destroy(old_entity);
  registry.Flush();

  const Entity new_entity = registry.Create();
  CHECK(new_entity.index == old_entity.index);
  CHECK(new_entity.generation != old_entity.generation);
  CHECK(new_entity != old_entity);
  CHECK(registry.IsAlive(new_entity));
  CHECK(!registry.IsAlive(old_entity));
  CHECK(!registry.Has<Position>(new_entity));
  CHECK(!registry.Has<Position>(old_entity));
}

/// @brief Each() visits exactly the entities that have every requested
/// component.
void TestEachMatchesAllComponents() {
  Registry registry;
  const Entity both = registry.Create();
  registry.Add(both, Position{1});
  registry.Add(both, Speed{10});
  const Entity only_position = registry.Create();
  registry.Add(only_position, Position{2});
  const Entity only_speed = registry.Create();
  registry.Add(only_speed, Speed{20});

  std::vector<Entity> visited;
  registry.Each<Position, Speed>(
      [&visited](Entity e, Position&, Speed&) { visited.push_back(e); });
  CHECK(visited.size() == 1);
  CHECK(!visited.empty() && visited[0] == both);

  CHECK(registry.Count<Position>() == 2);
  CHECK(registry.Count<Speed>() == 2);
  CHECK(registry.Count<Tag>() == 0);
}

/// @brief Components passed to Each() are references into the registry.
void TestEachCanChangeComponents() {
  Registry registry;
  for (int i = 0; i < 3; ++i) {
    const Entity e = registry.Create();
    registry.Add(e, Position{0});
    registry.Add(e, Speed{static_cast<float>(i)});
  }
  registry.Each<Position, Speed>(
      [](Entity, Position& p, const Speed& s) { p.x += s.value; });

  float total = 0;
  // The const overload of Each() runs here, because `view` is a const
  // reference. It hands out `const Position&`.
  const Registry& view = registry;
  view.Each<Position>([&total](Entity, const Position& p) { total += p.x; });
  CHECK(total == 0 + 1 + 2);
}

/// @brief Destroying entities inside Each() is safe, and Each() skips them
/// from then on.
void TestDestroyInsideEach() {
  Registry registry;
  std::array<Entity, 4> entities{};
  for (Entity& e : entities) {
    e = registry.Create();
    registry.Add(e, Position{1});
  }
  int visits = 0;
  registry.Each<Position>([&](Entity e, Position&) {
    ++visits;
    // The first visit destroys the last entity, so it is never visited.
    if (e == entities[0]) registry.Destroy(entities[3]);
  });
  CHECK(visits == 3);
  registry.Flush();
  CHECK(registry.Count<Position>() == 3);
}

/// @brief Deferred commands run at Flush() and may create entities.
void TestDeferRunsAtFlush() {
  Registry registry;
  const Entity spawner = registry.Create();
  registry.Add(spawner, Tag{});

  registry.Each<Tag>([&registry](Entity, Tag&) {
    registry.Defer([](Registry& r) {
      const Entity child = r.Create();
      r.Add(child, Position{9});
      // A command may queue another command; Flush() runs it too.
      r.Defer([child](Registry& r2) { r2.Get<Position>(child).x = 10; });
    });
  });
  CHECK(registry.Count<Position>() == 0);

  registry.Flush();
  CHECK(registry.Count<Position>() == 1);
  registry.Each<Position>([](Entity, Position& p) { CHECK(p.x == 10); });
}

/// @brief Clear() destroys everything and old handles stay dead.
void TestClear() {
  Registry registry;
  const Entity e = registry.Create();
  registry.Add(e, Position{1});
  registry.Defer([](Registry& r) { r.Create(); });

  registry.Clear();
  CHECK(!registry.IsAlive(e));
  CHECK(registry.Count<Position>() == 0);

  // The dropped command must not run later.
  registry.Flush();
  const Entity next = registry.Create();
  CHECK(next != e);
  CHECK(!registry.IsAlive(e));
}

}  // namespace
}  // namespace asteroids

/// @brief Runs every registry test.
/// @return 0 if all checks pass, otherwise 1.
int main() {
  // `using namespace` inside a function body only affects that body, which
  // keeps the shortcut from leaking into other code.
  using namespace asteroids;
  // A std::vector rather than a C array (`Test tests[] = {...}`): it knows
  // its own size and converts straight to the std::span RunTests() takes.
  const std::vector<testing::Test> tests = {
      {"create gives distinct live entities",
       TestCreateGivesDistinctLiveEntities},
      {"add, get, remove", TestAddGetRemove},
      {"destroy is deferred until flush", TestDestroyIsDeferredUntilFlush},
      {"reused slots invalidate old handles",
       TestReusedSlotsInvalidateOldHandles},
      {"each matches all components", TestEachMatchesAllComponents},
      {"each can change components", TestEachCanChangeComponents},
      {"destroy inside each", TestDestroyInsideEach},
      {"defer runs at flush", TestDeferRunsAtFlush},
      {"clear", TestClear},
  };
  return testing::RunTests(tests);
}
