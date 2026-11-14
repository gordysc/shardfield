#ifndef ASTEROIDS_ECS_H_
#define ASTEROIDS_ECS_H_

// Header guard; see screen.h for how it works.

#include <cstddef>
#include <entt/entity/registry.hpp>
#include <functional>
#include <vector>

/// @brief Everything that belongs to the game.
namespace asteroids {

// This branch replaces the hand-made Registry (registry.h, sparse_set.h and
// entity.h on `main`) with EnTT, a widely used ECS library. EnTT's
// entt::registry already stores components in sparse sets, and its
// entt::entity already packs an index and a version (EnTT's word for
// generation) into one 32-bit value.
//
// EnTT has no built-in "destroy later" or "run this later". Its documented
// rules allow creating entities and components during a view loop, but not
// destroying *other* entities. This file adds the two missing pieces, so the
// systems keep the same frame rules as on `main`:
// - Destroy() tags an entity with Dying. Views skip Dying entities, and
//   Flush() destroys them.
// - Defer() queues a command, and Flush() runs it.

/// @brief A tag that marks an entity for destruction at the next Flush().
///
/// Every view in the game excludes it, with `entt::exclude<Dying>`.
struct Dying {};

/// @brief Commands queued by Defer(), waiting for Flush().
///
/// One of these lives in the registry's "context": `registry.ctx()` holds
/// one value of any type, a place for global data that systems need but
/// that belongs to no entity. Other ECS libraries call these "resources" or
/// "singletons". Keeping the queue there means systems need only the
/// registry, exactly like on `main`.
struct CommandQueue {
  /// @brief The queued commands, in the order they were queued.
  std::vector<std::function<void(entt::registry&)>> commands;
};

/// @brief Prepares a registry for use with the functions below. Call it
/// once, right after making the registry.
/// @param registry The registry to prepare.
void InitWorld(entt::registry& registry);

/// @brief Checks whether a handle refers to an entity that exists and isn't
/// marked for destruction.
///
/// entt::registry::valid() compares the handle's version with the slot's
/// current version, which is the same generation check as on `main`.
///
/// @param registry The registry to look in.
/// @param entity The handle to check.
/// @return True if the entity is alive.
bool IsAlive(const entt::registry& registry, entt::entity entity);

/// @brief Marks an entity for destruction at the next Flush(). Safe inside
/// a view loop, because it only adds a component.
/// @param registry The registry the entity lives in.
/// @param entity The entity to destroy. Ignored if not alive.
void Destroy(entt::registry& registry, entt::entity entity);

/// @brief Queues code to run at the next Flush().
///
/// See Registry::Defer() on `main` for why capturing by value matters.
///
/// @param registry The registry whose queue to use.
/// @param command The code to run.
void Defer(entt::registry& registry,
           std::function<void(entt::registry&)> command);

/// @brief Runs queued commands, then destroys every Dying entity. Call it
/// once per frame, after all systems have run.
/// @param registry The registry to flush.
void Flush(entt::registry& registry);

/// @brief Destroys every entity and drops queued commands.
/// @param registry The registry to clear.
void ClearWorld(entt::registry& registry);

/// @brief Counts the live entities that have component T.
///
/// A view's size_hint() is only an upper bound, because it can't tell which
/// entities the exclude filter removes without visiting them, so this
/// counts by walking the view.
///
/// @tparam T The component type.
/// @param registry The registry to look in.
/// @return How many there are.
template <typename T>
std::size_t CountAlive(const entt::registry& registry) {
  std::size_t count = 0;
  // A range-based `for` over a view visits each matching entity. For a
  // const registry, the view hands out const components.
  for ([[maybe_unused]] const entt::entity entity :
       registry.view<T>(entt::exclude<Dying>)) {
    ++count;
  }
  return count;
}

}  // namespace asteroids

#endif  // ASTEROIDS_ECS_H_
