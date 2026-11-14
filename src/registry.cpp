#include "registry.h"

#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include "entity.h"

namespace asteroids {

Entity Registry::Create() {
  assert(iterating_ == 0 && "Create() inside Each(); use Defer()");
  // Reuse a freed slot if there is one. That keeps `slots_` (and every
  // pool) from growing forever in a game that creates and destroys bullets
  // all the time.
  if (!free_slots_.empty()) {
    const std::uint32_t index = free_slots_.back();
    free_slots_.pop_back();
    Slot& slot = slots_[index];
    slot.alive = true;
    // The generation was already bumped when the old entity was destroyed,
    // so this handle differs from every handle the slot gave out before.
    return Entity{index, slot.generation};
  }
  const auto index = static_cast<std::uint32_t>(slots_.size());
  slots_.push_back(Slot{.generation = 0, .alive = true, .dying = false});
  return Entity{index, 0};
}

void Registry::Destroy(Entity entity) {
  if (!IsAlive(entity)) return;
  // Only mark it. Removing components now could pull them out from under a
  // running Each() loop. Pushing to `pending_destroy_` is safe even inside
  // Each(), because no loop walks over that vector until Flush().
  slots_[entity.index].dying = true;
  pending_destroy_.push_back(entity);
}

void Registry::Defer(std::function<void(Registry&)> command) {
  // The std::function is moved into the queue, so whatever the lambda
  // captured by value moves along with it instead of being copied.
  deferred_.push_back(std::move(command));
}

void Registry::Flush() {
  assert(iterating_ == 0 && "Flush() inside Each()");
  // A command may queue more commands or destroy entities, and destroying
  // never queues commands, so loop until both queues are empty.
  while (!deferred_.empty() || !pending_destroy_.empty()) {
    // Move the queue out before running it. A command that calls Defer()
    // pushes to `deferred_`, and pushing to a vector that a loop is walking
    // can move its elements and invalidate the loop's iterators. A
    // moved-from vector is "valid but unspecified", so clear() makes sure
    // it is really empty before new commands land in it.
    std::vector<std::function<void(Registry&)>> commands = std::move(deferred_);
    deferred_.clear();
    for (std::function<void(Registry&)>& command : commands) command(*this);

    std::vector<Entity> doomed = std::move(pending_destroy_);
    pending_destroy_.clear();
    for (const Entity entity : doomed) DestroyNow(entity);
  }
}

void Registry::Clear() {
  assert(iterating_ == 0 && "Clear() inside Each()");
  deferred_.clear();
  pending_destroy_.clear();
  // Destroying slot by slot (rather than just emptying `slots_`) bumps each
  // generation, so a handle kept from before Clear() can't come back to life
  // when its slot is reused.
  for (std::uint32_t i = 0; i < slots_.size(); ++i) {
    if (slots_[i].alive) DestroyNow(Entity{i, slots_[i].generation});
  }
}

bool Registry::IsAlive(Entity entity) const {
  return Exists(entity) && !slots_[entity.index].dying;
}

bool Registry::Exists(Entity entity) const {
  // `&&` stops at the first false part, so slots_[entity.index] is only read
  // when the index is in range. Reading past the end would be undefined
  // behaviour.
  return entity.index < slots_.size() && slots_[entity.index].alive &&
         slots_[entity.index].generation == entity.generation;
}

void Registry::DestroyNow(Entity entity) {
  // `const std::unique_ptr<PoolBase>&` reads each pointer without copying
  // it; a unique_ptr can't be copied anyway. Calling Remove() through the
  // base pointer is virtual dispatch: it runs Pool<T>::Remove for whichever
  // T each pool really stores.
  for (const std::unique_ptr<PoolBase>& pool : pools_) {
    if (pool) pool->Remove(entity.index);
  }
  Slot& slot = slots_[entity.index];
  slot.alive = false;
  slot.dying = false;
  // After about 4 billion reuses of one slot the generation wraps round to
  // 0. Unsigned integers wrap by definition, so that is not undefined
  // behaviour, and no game keeps an old handle around that long.
  ++slot.generation;
  free_slots_.push_back(entity.index);
}

}  // namespace asteroids
