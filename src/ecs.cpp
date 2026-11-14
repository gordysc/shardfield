#include "ecs.h"

#include <entt/entity/registry.hpp>
#include <functional>
#include <utility>
#include <vector>

namespace asteroids {

void InitWorld(entt::registry& registry) {
  // ctx().emplace<T>() creates the T if the context doesn't have one yet.
  registry.ctx().emplace<CommandQueue>();
}

bool IsAlive(const entt::registry& registry, entt::entity entity) {
  // valid() is false for entt::null, for destroyed entities and for old
  // handles whose slot has been reused with a newer version.
  return registry.valid(entity) && !registry.all_of<Dying>(entity);
}

void Destroy(entt::registry& registry, entt::entity entity) {
  if (!IsAlive(registry, entity)) return;
  // emplace<Dying>() adds a component to an entity that isn't necessarily
  // the one a loop is visiting. EnTT allows that during iteration: adding
  // never moves components that already exist.
  registry.emplace<Dying>(entity);
}

void Defer(entt::registry& registry,
           std::function<void(entt::registry&)> command) {
  registry.ctx().get<CommandQueue>().commands.push_back(std::move(command));
}

void Flush(entt::registry& registry) {
  // `queue` is a reference to the CommandQueue inside the registry's
  // context, not a copy.
  CommandQueue& queue = registry.ctx().get<CommandQueue>();
  // A command may queue more commands or destroy entities, so loop until
  // nothing is left. Moving the vector out first means a command that calls
  // Defer() pushes into a fresh vector, not the one being walked.
  while (true) {
    std::vector<std::function<void(entt::registry&)>> commands =
        std::move(queue.commands);
    queue.commands.clear();
    for (std::function<void(entt::registry&)>& command : commands) {
      command(registry);
    }

    // view<Dying>() lists every marked entity. registry.destroy() with an
    // iterator range destroys them all, and with them their components.
    // Destroying bumps each slot's version, so old handles go stale.
    const auto dying = registry.view<Dying>();
    const bool nothing_dying = dying.empty();
    registry.destroy(dying.begin(), dying.end());

    if (nothing_dying && queue.commands.empty()) break;
  }
}

void ClearWorld(entt::registry& registry) {
  registry.ctx().get<CommandQueue>().commands.clear();
  // clear() with no template arguments empties every pool and destroys
  // every entity, bumping versions as it goes.
  registry.clear();
}

}  // namespace asteroids
