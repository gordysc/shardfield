#ifndef ASTEROIDS_SPARSE_SET_H_
#define ASTEROIDS_SPARSE_SET_H_

// Header guard; see screen.h for how it works.

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

#include "entity.h"

/// @brief Everything that belongs to the game.
namespace asteroids {

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
  /// only the base part would be destroyed, the derived pool's vectors would
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

  /// @brief Returns how many components the pool holds.
  /// @return The count.
  virtual std::size_t Size() const = 0;

  /// @brief Returns the slot index of every entity that has a component
  /// here, packed together with no gaps.
  ///
  /// std::span is a view of the pool's own vector: it doesn't copy or own
  /// the indices. It is only valid until the pool changes.
  ///
  /// @return The owners' slot indices, in storage order.
  virtual std::span<const std::uint32_t> Owners() const = 0;
};

/// @brief Stores every component of type T in one packed array.
///
/// A sparse set uses three vectors:
/// - `dense_` holds the components themselves, side by side with no gaps.
///   Looping over it touches only real components, one after another in
///   memory, which is as fast as a loop gets: the CPU loads memory in
///   chunks, so neighbouring components arrive for free ("cache friendly").
/// - `owners_` runs parallel to `dense_`: `owners_[i]` is the slot index of
///   the entity that owns `dense_[i]`.
/// - `sparse_` maps the other way: `sparse_[slot]` is the position in
///   `dense_` of that entity's component, or kNoIndex if it has none. It has
///   gaps (hence "sparse"), but it only holds small integers.
///
/// So "does entity 7 have a T?" is one lookup, `sparse_[7]`, and "every T"
/// is a straight walk over `dense_`.
///
/// `final` means no class may derive from SparseSet. It also lets the
/// compiler skip the virtual-call machinery when it knows the exact type.
///
/// @tparam T The component type.
template <typename T>
class SparseSet final : public PoolBase {
 public:
  /// @brief Checks whether slot `index` has a component.
  /// @param index The entity's slot.
  /// @return True if it has one.
  bool Has(std::uint32_t index) const {
    return index < sparse_.size() && sparse_[index] != kNoIndex;
  }

  /// @brief Returns the component of slot `index`. It must have one.
  /// @param index The entity's slot.
  /// @return A reference into `dense_`. See Registry::Get() for how long it
  ///         stays valid.
  T& Get(std::uint32_t index) { return dense_[sparse_[index]]; }

  /// @brief Read-only version of Get(), used through a `const SparseSet`.
  /// @param index The entity's slot.
  /// @return A const reference into `dense_`.
  const T& Get(std::uint32_t index) const { return dense_[sparse_[index]]; }

  /// @brief Stores a component for slot `index`, replacing any old one.
  /// @param index The entity's slot.
  /// @param component The component. Taken by value and then moved in; see
  ///        Registry::Add().
  /// @return A reference to the stored component.
  T& Set(std::uint32_t index, T component) {
    if (Has(index)) {
      // std::move hands the component's contents over instead of copying
      // them. The parameter is left "valid but unspecified", which is fine
      // because it is never used again.
      T& existing = dense_[sparse_[index]];
      existing = std::move(component);
      return existing;
    }
    // resize() with a second argument fills the new entries with kNoIndex.
    if (index >= sparse_.size()) sparse_.resize(index + 1, kNoIndex);
    sparse_[index] = static_cast<std::uint32_t>(dense_.size());
    // push_back may move `dense_` to a bigger block of memory. Every
    // reference into it then dangles, which is why the Registry forbids
    // adding components inside Each().
    dense_.push_back(std::move(component));
    owners_.push_back(index);
    return dense_.back();
  }

  /// @brief Removes the component of slot `index`, if any.
  ///
  /// "Swap and pop": the last component moves into the hole, then the
  /// vector shrinks by one. That keeps `dense_` packed with no gaps, and it
  /// costs the same however big the pool is. The price is that the order of
  /// components changes, which is why nothing may rely on it.
  ///
  /// `override` asks the compiler to check that this really replaces a
  /// virtual function in PoolBase.
  ///
  /// @param index The entity's slot.
  void Remove(std::uint32_t index) override {
    if (!Has(index)) return;
    const std::uint32_t hole = sparse_[index];
    const auto last = static_cast<std::uint32_t>(dense_.size() - 1);
    if (hole != last) {
      dense_[hole] = std::move(dense_[last]);
      owners_[hole] = owners_[last];
      // The moved component's owner must now find it at its new position.
      sparse_[owners_[hole]] = hole;
    }
    // pop_back() destroys the last element, which is now either the removed
    // component or the moved-from husk of the one that filled the hole.
    dense_.pop_back();
    owners_.pop_back();
    sparse_[index] = kNoIndex;
  }

  /// @brief Returns how many components the pool holds.
  /// @return The count.
  std::size_t Size() const override { return dense_.size(); }

  /// @brief Returns the slot index of every entity with a component here.
  /// @return A view of `owners_`.
  std::span<const std::uint32_t> Owners() const override { return owners_; }

 private:
  /// @brief Slot index to position in `dense_`, or kNoIndex.
  std::vector<std::uint32_t> sparse_;
  /// @brief The components, packed with no gaps.
  std::vector<T> dense_;
  /// @brief Slot index of the owner of each element of `dense_`.
  std::vector<std::uint32_t> owners_;
};

}  // namespace asteroids

#endif  // ASTEROIDS_SPARSE_SET_H_
