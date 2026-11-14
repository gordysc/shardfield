#ifndef ASTEROIDS_REGISTRY_H_
#define ASTEROIDS_REGISTRY_H_

// Header guard; see screen.h for how it works.

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

#include "entity.h"

/// @brief Everything that belongs to the game.
namespace asteroids {

/// @brief Hands out a small, unique number for each component type.
///
/// The Registry keeps one storage pool per component type in a vector, and
/// uses this number as the index into it. That is faster than looking types
/// up in a map.
class ComponentIds {
 public:
  /// @brief Returns the number for component type T.
  ///
  /// A `static` local variable is created the first time the function runs
  /// and then kept for the rest of the program. Each different T makes a
  /// different function (that is what a template is), so each T gets its own
  /// `id`, set once from the shared counter. Function templates in headers
  /// are `inline` automatically, so every .cpp file sees the same `id` for
  /// the same T.
  ///
  /// @tparam T The component type.
  /// @return A number unique to T, counting up from 0.
  template <typename T>
  static std::size_t Of() {
    static const std::size_t id = next_++;
    return id;
  }

 private:
  /// @brief The number the next new component type will get. `inline
  /// static` (C++17) defines the variable right here in the header.
  inline static std::size_t next_ = 0;
};

/// @brief What every component pool can do, whatever type it stores.
///
/// The Registry holds pools for many different component types in one
/// vector. A vector can only hold one type, so it holds pointers to this
/// shared base class. When an entity is destroyed, the Registry calls
/// Remove() on every pool without knowing what each one stores. That trick
/// is called "type erasure".
class PoolBase {
 public:
  /// @brief Destroys the pool.
  ///
  /// A base class that is deleted through a base pointer (here, by
  /// std::unique_ptr<PoolBase>) needs a `virtual` destructor. Without it,
  /// only the base part would be destroyed, the derived pool's vector would
  /// leak, and the behaviour would be undefined.
  virtual ~PoolBase() = default;

  /// @brief Removes the component of the entity in slot `index`, if it has
  /// one.
  ///
  /// `= 0` makes this "pure virtual": PoolBase has no body for it, and every
  /// derived class must provide one.
  ///
  /// @param index The entity's slot.
  virtual void Remove(std::uint32_t index) = 0;
};

/// @brief Stores every component of type T, one optional slot per entity
/// slot.
///
/// This is the simplest possible layout: `slots_[i]` holds entity i's T, or
/// nothing. Lookups are instant, but looping over all T means checking every
/// entity slot, even when only a handful of entities have a T.
///
/// `final` means no class may derive from Pool. It also lets the compiler
/// skip the virtual-call machinery when it knows the exact type.
///
/// @tparam T The component type.
template <typename T>
class Pool final : public PoolBase {
 public:
  /// @brief Checks whether slot `index` has a component.
  /// @param index The entity's slot.
  /// @return True if it has one.
  bool Has(std::uint32_t index) const {
    return index < slots_.size() && slots_[index].has_value();
  }

  /// @brief Returns the component in slot `index`. The slot must have one.
  /// @param index The entity's slot.
  /// @return A reference into the pool. See Registry::Get() for how long it
  ///         stays valid.
  T& Get(std::uint32_t index) { return *slots_[index]; }

  /// @brief Read-only version of Get(), used through a `const Pool`.
  /// @param index The entity's slot.
  /// @return A const reference into the pool.
  const T& Get(std::uint32_t index) const { return *slots_[index]; }

  /// @brief Stores a component in slot `index`, replacing any old one.
  /// @param index The entity's slot.
  /// @param component The component. Taken by value and then moved in; see
  ///        Registry::Add().
  /// @return A reference to the stored component.
  T& Set(std::uint32_t index, T component) {
    // resize() adds empty optionals until the slot exists. It can move the
    // whole vector to a bigger block of memory, which is one reason the
    // Registry forbids adding components inside Each().
    if (index >= slots_.size()) slots_.resize(index + 1);
    // std::move turns `component` into an rvalue, so the optional takes over
    // its contents instead of copying them. `component` is left "valid but
    // unspecified", which is fine because we never use it again.
    slots_[index] = std::move(component);
    return *slots_[index];
  }

  /// @brief Removes the component in slot `index`, if any.
  ///
  /// `override` asks the compiler to check that this really replaces a
  /// virtual function in PoolBase. A typo in the name or parameters then
  /// becomes a compile error instead of a silent new function.
  ///
  /// @param index The entity's slot.
  void Remove(std::uint32_t index) override {
    // reset() destroys the stored T and leaves the optional empty.
    if (index < slots_.size()) slots_[index].reset();
  }

 private:
  /// @brief One optional component per entity slot.
  std::vector<std::optional<T>> slots_;
};

/// @brief Owns every entity and component, and finds entities by the
/// components they have.
///
/// Typical use:
/// @code
///   Entity rock = registry.Create();
///   registry.Add(rock, Transform{...});
///   registry.Add(rock, Velocity{...});
///   registry.Each<Transform, Velocity>(
///       [&](Entity, Transform& t, Velocity& v) { t.position += ... });
/// @endcode
///
/// Rules while an Each() loop is running:
/// - Reading and changing component values is fine.
/// - Destroy() is fine: it only marks the entity, and Each() skips marked
///   entities from then on. The entity really goes away in Flush().
/// - Create(), Add() and Remove() are not allowed, because they can move
///   components in memory and leave the loop's references dangling. Use
///   Defer() to run that code at the next Flush() instead. Debug builds
///   check this rule with assert().
class Registry {
 public:
  /// @brief Creates an empty registry.
  Registry() = default;

  // A Registry owns its pools through std::unique_ptr, which can't be
  // copied, so the compiler wouldn't generate a copy constructor anyway.
  // Deleting it explicitly gives a clearer error message.

  /// @brief Not copyable: entity handles belong to one registry.
  Registry(const Registry&) = delete;
  /// @brief Not copy-assignable: entity handles belong to one registry.
  Registry& operator=(const Registry&) = delete;

  /// @brief Makes a new entity with no components.
  /// @return A handle to it.
  Entity Create();

  /// @brief Marks an entity for destruction at the next Flush().
  ///
  /// From now on IsAlive() returns false and Each() skips the entity, but
  /// its components stay readable until Flush(). Destroying an entity that
  /// is already marked or gone does nothing.
  ///
  /// @param entity The entity to destroy.
  void Destroy(Entity entity);

  /// @brief Queues code to run at the next Flush().
  ///
  /// Use it to create entities from inside an Each() loop. The command is a
  /// std::function: a wrapper that can hold any callable (here, usually a
  /// lambda) that takes a `Registry&`. Lambdas capture variables by value
  /// with `[=]` or by reference with `[&]`. A captured reference must still
  /// be valid when Flush() runs, so prefer capturing by value.
  ///
  /// @param command The code to run.
  void Defer(std::function<void(Registry&)> command);

  /// @brief Runs deferred commands and destroys marked entities.
  ///
  /// Call it once per frame, after all systems have run.
  void Flush();

  /// @brief Destroys every entity at once, and drops queued commands.
  ///
  /// Each slot's generation is still bumped, so old handles stay invalid.
  void Clear();

  /// @brief Checks whether a handle refers to an entity that exists and is
  /// not marked for destruction.
  /// @param entity The handle to check.
  /// @return True if the entity is alive.
  bool IsAlive(Entity entity) const;

  /// @brief Attaches a component to an entity, replacing any old one of the
  /// same type.
  ///
  /// The component type is deduced from the argument, so you write
  /// `registry.Add(e, Velocity{...})` without `<Velocity>`.
  ///
  /// The component is taken by value. A temporary like `Velocity{...}` is
  /// moved into the parameter and then moved into the pool, so nothing is
  /// copied. A named variable is copied once into the parameter, which
  /// leaves the caller's variable untouched.
  ///
  /// @tparam T The component type.
  /// @param entity The entity. Must exist.
  /// @param component The component to attach.
  /// @return A reference to the stored component.
  template <typename T>
  T& Add(Entity entity, T component) {
    assert(iterating_ == 0 && "Add() inside Each(); use Defer()");
    assert(Exists(entity));
    return PoolFor<T>().Set(entity.index, std::move(component));
  }

  /// @brief Detaches a component from an entity. Does nothing if the entity
  /// doesn't have one.
  /// @tparam T The component type.
  /// @param entity The entity. Must exist.
  template <typename T>
  void Remove(Entity entity) {
    assert(iterating_ == 0 && "Remove() inside Each(); use Defer()");
    assert(Exists(entity));
    PoolFor<T>().Remove(entity.index);
  }

  /// @brief Checks whether an entity has a component.
  /// @tparam T The component type.
  /// @param entity The entity.
  /// @return True if the entity exists and has a T.
  template <typename T>
  bool Has(Entity entity) const {
    const Pool<T>* pool = FindPool<T>();
    return Exists(entity) && pool != nullptr && pool->Has(entity.index);
  }

  /// @brief Returns an entity's component. The entity must have one.
  ///
  /// The reference points into the pool's storage. It stays valid until a
  /// component of the same type is added or removed, or the entity is
  /// flushed. Use it straight away and don't store it; store the Entity
  /// handle instead and call Get() again later.
  ///
  /// @tparam T The component type.
  /// @param entity The entity.
  /// @return A reference to the component.
  template <typename T>
  T& Get(Entity entity) {
    assert(Has<T>(entity));
    return PoolFor<T>().Get(entity.index);
  }

  /// @brief Read-only version of Get(), used through a `const Registry&`.
  /// @tparam T The component type.
  /// @param entity The entity.
  /// @return A const reference to the component.
  template <typename T>
  const T& Get(Entity entity) const {
    assert(Has<T>(entity));
    return FindPool<T>()->Get(entity.index);
  }

  /// @brief Returns a pointer to an entity's component, or nullptr if it
  /// has none.
  ///
  /// A raw pointer here means "maybe a component, and you don't own it".
  /// Never delete it. The same lifetime rules as Get() apply.
  ///
  /// @tparam T The component type.
  /// @param entity The entity.
  /// @return The component, or nullptr.
  template <typename T>
  T* TryGet(Entity entity) {
    return Has<T>(entity) ? &PoolFor<T>().Get(entity.index) : nullptr;
  }

  /// @brief Calls `fn` once for every live entity that has all of the
  /// component types `Ts`.
  ///
  /// `fn` is called as `fn(Entity, Ts&...)`. `Ts...` is a "parameter pack":
  /// any number of types. `Fn&&` here is a forwarding reference: it binds to
  /// any callable, temporary or not, without copying it.
  ///
  /// This version visits every entity slot and checks each pool, so it costs
  /// time for every entity, even ones with none of the components.
  ///
  /// @tparam Ts The component types an entity must have.
  /// @tparam Fn The callable's type, deduced from the argument.
  /// @param fn What to call for each matching entity.
  template <typename... Ts, typename Fn>
  void Each(Fn&& fn) {
    static_assert(sizeof...(Ts) > 0, "Each() needs at least one component");
    // The guard object counts this loop as running for as long as it
    // exists. Its destructor runs when Each() returns for any reason,
    // including an exception thrown by `fn`, so the count can't get stuck.
    // This "do it in a destructor" pattern is called RAII.
    const IterationGuard guard(iterating_);
    // Look each pool up once, before the loop. The std::tuple holds one pool
    // pointer per type in `Ts`. The `...` after `&PoolFor<Ts>()` repeats the
    // expression for each type in the pack ("pack expansion"), and
    // std::get<Pool<A>*>(pools) later picks out the pointer for type A.
    const std::tuple<Pool<Ts>*...> pools{&PoolFor<Ts>()...};
    // Index loop, because the slot index is the entity's index. The size is
    // read once: Create() can't run during Each(), so it can't change.
    const auto slot_count = static_cast<std::uint32_t>(slots_.size());
    for (std::uint32_t i = 0; i < slot_count; ++i) {
      if (!slots_[i].alive || slots_[i].dying) continue;
      // A "fold expression" (C++17): `(test && ...)` expands to
      // `test<A> && test<B> && ...` for every type in the pack.
      if (!(std::get<Pool<Ts>*>(pools)->Has(i) && ...)) continue;
      fn(Entity{i, slots_[i].generation},
         std::get<Pool<Ts>*>(pools)->Get(i)...);
    }
  }

  /// @brief Read-only version of Each(). `fn` gets `const Ts&...`.
  ///
  /// It duplicates the loop above rather than sharing it, because the two
  /// versions need different pool pointer types (const and non-const).
  ///
  /// @tparam Ts The component types an entity must have.
  /// @tparam Fn The callable's type, deduced from the argument.
  /// @param fn What to call for each matching entity.
  template <typename... Ts, typename Fn>
  void Each(Fn&& fn) const {
    static_assert(sizeof...(Ts) > 0, "Each() needs at least one component");
    const std::tuple<const Pool<Ts>*...> pools{FindPool<Ts>()...};
    // If any pool doesn't exist yet, no entity can have all the components.
    if (((std::get<const Pool<Ts>*>(pools) == nullptr) || ...)) return;
    const auto slot_count = static_cast<std::uint32_t>(slots_.size());
    for (std::uint32_t i = 0; i < slot_count; ++i) {
      if (!slots_[i].alive || slots_[i].dying) continue;
      if (!(std::get<const Pool<Ts>*>(pools)->Has(i) && ...)) continue;
      fn(Entity{i, slots_[i].generation},
         std::get<const Pool<Ts>*>(pools)->Get(i)...);
    }
  }

  /// @brief Counts the live entities that have component T.
  /// @tparam T The component type.
  /// @return How many there are.
  template <typename T>
  std::size_t Count() const {
    std::size_t count = 0;
    Each<T>([&count](Entity, const T&) { ++count; });
    return count;
  }

 private:
  /// @brief Book-keeping for one entity slot.
  struct Slot {
    /// @brief Bumped every time the slot's entity is destroyed.
    std::uint32_t generation = 0;
    /// @brief Whether an entity currently lives in the slot.
    bool alive = false;
    /// @brief Whether the entity is marked for destruction at Flush().
    bool dying = false;
  };

  /// @brief Counts a running Each() loop for as long as it exists. See
  /// Each().
  class IterationGuard {
   public:
    /// @brief Starts counting.
    ///
    /// Storing a reference member means the guard must not outlive the
    /// counter it points to. That holds here: the guard is a local inside a
    /// Registry method, and the counter is a member of that Registry.
    ///
    /// @param counter The Registry's count of running loops.
    explicit IterationGuard(int& counter) : counter_(counter) { ++counter_; }
    /// @brief Stops counting.
    ~IterationGuard() { --counter_; }
    /// @brief Not copyable: a copy would decrement the counter twice.
    IterationGuard(const IterationGuard&) = delete;
    /// @brief Not copy-assignable, for the same reason.
    IterationGuard& operator=(const IterationGuard&) = delete;

   private:
    /// @brief The counter this guard increments and decrements.
    int& counter_;
  };

  /// @brief Checks whether a handle refers to an existing entity, including
  /// one marked for destruction whose components are still readable.
  /// @param entity The handle to check.
  /// @return True if the slot is in use by this exact generation.
  bool Exists(Entity entity) const;

  /// @brief Removes an entity and its components immediately.
  /// @param entity The entity. Must exist.
  void DestroyNow(Entity entity);

  /// @brief Returns the pool for component T, creating it on first use.
  /// @tparam T The component type.
  /// @return The pool.
  template <typename T>
  Pool<T>& PoolFor() {
    const std::size_t id = ComponentIds::Of<T>();
    if (id >= pools_.size()) pools_.resize(id + 1);
    // std::make_unique allocates the pool on the heap and wraps it in a
    // unique_ptr, which deletes it automatically when the Registry is
    // destroyed. It replaces a bare `new` that someone has to remember to
    // `delete`. The pools live on the heap, so they stay put even when
    // `pools_` itself grows and moves its pointers around.
    if (!pools_[id]) pools_[id] = std::make_unique<Pool<T>>();
    // static_cast from base to derived is safe here because the id is unique
    // to T, so the pool at this index is always a Pool<T>.
    return static_cast<Pool<T>&>(*pools_[id]);
  }

  /// @brief Returns the pool for component T without creating it.
  /// @tparam T The component type.
  /// @return The pool, or nullptr if no T was ever added.
  template <typename T>
  const Pool<T>* FindPool() const {
    const std::size_t id = ComponentIds::Of<T>();
    if (id >= pools_.size() || !pools_[id]) return nullptr;
    return static_cast<const Pool<T>*>(pools_[id].get());
  }

  /// @brief One entry per entity slot ever used.
  std::vector<Slot> slots_;
  /// @brief Slots whose entities were destroyed, ready for reuse.
  std::vector<std::uint32_t> free_slots_;
  /// @brief Entities marked by Destroy(), removed at the next Flush().
  std::vector<Entity> pending_destroy_;
  /// @brief Commands queued by Defer(), run at the next Flush().
  std::vector<std::function<void(Registry&)>> deferred_;
  /// @brief One pool per component type, indexed by ComponentIds::Of<T>().
  /// Empty entries are types this registry has never seen.
  std::vector<std::unique_ptr<PoolBase>> pools_;
  /// @brief How many Each() loops are running right now (they can nest).
  int iterating_ = 0;
};

}  // namespace asteroids

#endif  // ASTEROIDS_REGISTRY_H_
