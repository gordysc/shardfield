# ECS in Shardfield

This guide explains the entity component system (ECS) in this game. It
connects each idea to the code that shows it.

## The stages

The git history builds the game in stages. Each stage has a tag.

| Tag | What changes |
|---|---|
| `step-1-plain-structs` | The game in the Breakout style. The `Game` class owns a `Ship`, a vector of bullets and a vector of rocks. |
| `step-2-entity-manager` | A hand-made `Registry` owns all game objects. Components are in `std::vector<std::optional<T>>` pools. |
| `step-3-sparse-sets` | The same `Registry` API on sparse-set storage. No game code changes. |
| `step-4-entt` (branch) | The same game on [EnTT](https://github.com/skypjack/entt), a production ECS library. |

To see a stage, check out its tag:
```bash
git checkout step-1-plain-structs
```

To see the files that one stage changes, compare two tags:
```bash
git diff --stat step-1-plain-structs step-2-entity-manager
```

To go back to the latest code, check out `main`:
```bash
git checkout main
```

## Why stage 1 needs a change

Stage 1 works, but each new kind of object makes it more difficult to change.
- Each kind of object has its own struct, its own vector and its own update loop.
- Bullets and rocks both move and wrap, but each has a copy of that code.
- New rock pieces go into a temporary vector, because a push into `rocks_`
  during the loop can move the vector and break the loop.
- A new feature, such as homing missiles, needs a new struct, a new vector,
  new update code and new collision code.

The ECS stages solve each of these problems.

## Entities, components and systems

An **entity** is only an ID. It has no data and no behaviour. See
`src/entity.h`.

A **component** is a small struct of plain data, such as `Transform`,
`Velocity` or `Lifetime`. See `src/components.h`.

A **system** is a function. It loops over all entities that have a set of
components, and it updates them. See `src/systems.cpp`.

The `Registry` stores the entities and their components. `Each<A, B>()`
calls a function for each live entity that has both an `A` and a `B`. See
`src/registry.h`.

The components of an entity decide what the entity is. Nothing in the code
says "this is a particle". A particle is only `Transform`, `Velocity`,
`Drag`, `Lifetime`, `CircleSprite` and `FadeOut`. The movement, drag,
lifetime and render systems handle it with no particle code at all.

The spawn functions in `src/spawn.cpp` are the one place that knows which
components make up each kind of entity.

## One frame

`Game::UpdatePlaying()` runs the systems in a fixed order:
1. The input systems: ship control, gun, missile launcher.
2. The effect systems: exhaust, homing, trail, shield.
3. The motion systems: drag, speed limit, movement, wrap, lifetime.
4. The collision and pickup systems.
5. `Registry::Flush()`.

The order is important. For example, the collision system must see the
positions after the movement system changes them.

The collision system does not change the score. It returns a
`CollisionReport`, and the `Game` applies the game rules. This keeps the
systems about entities, and the rules in one place.

## Rules inside an Each() loop

An `Each()` loop holds references into the component storage. Some
operations can move that storage, and the references then point at freed
memory. This is undefined behaviour.

Inside an `Each()` loop:
- You can read and change component values.
- You can call `Destroy()`. It only marks the entity. `Each()` skips a
  marked entity, and `Flush()` removes it.
- You cannot call `Create()`, `Add()` or `Remove()`. Put them in a
  `Defer()` command. `Flush()` runs the command after the loops end.

Debug builds check these rules with `assert()`.

Deferred commands are lambdas. Capture values by copy, because the lambda
runs later. The code captures `Random&` by reference only because the
`Random` lives in the `Game` and outlives each `Flush()`.

## Generational handles

An `Entity` has two parts: an `index` and a `generation`. The index is the
slot of the entity in the registry. The generation counts how many times
the slot was used before.

When the registry destroys an entity, it increases the generation of the
slot. A later entity can use the same slot, but it gets the new generation.
An old handle keeps the old generation, so `IsAlive()` returns false for it.

The homing missile shows why this is important. A missile keeps a handle to
its target rock for many frames. If a bullet destroys the rock, a new
particle can take the same slot. Without the generation, the missile would
follow the particle. With the generation, `IsAlive()` returns false, and the
missile finds a new rock. The test `missile retargets when target dies` in
`tests/systems_test.cpp` checks this case.

Deferred commands need the same care. A command can run after its entity is
destroyed, so commands that capture a handle check `IsAlive()` first. See
`ShieldSystem()` and `PickupSystem()`.

## Component storage

Stage 2 keeps one `std::optional<T>` for each entity slot, in each pool. A
lookup is fast. But `Each()` must check every slot, also the slots of
entities without the component.

Stage 3 uses a sparse set for each pool. See `src/sparse_set.h`.
- A dense array holds the components with no gaps between them.
- A sparse array maps an entity slot to a position in the dense array.
- An owners array maps each dense position back to its entity slot.

`Each()` now walks only the smallest of the pools that it needs. To find 10
rocks among 20000 particles, it visits 10 entities, not 20010.

The benchmark in `bench/registry_bench.cpp` measures the difference. These
numbers come from a Release build on an Apple silicon Mac:

| Operation | Stage 2 | Stage 3 |
|---|---|---|
| `Each<Position, Speed>`, 20000 entities | 16–21 µs | 15–18 µs |
| `Each<Rare, Position>`, 10 matches in 20000 | 13–17 µs | 0.01 µs |
| Create and destroy 1000 entities | 12–14 µs | 12–14 µs |

The game runs queries like the second one each frame. It looks for a few
rocks, one ship and some pickups among hundreds of particles.

To run the benchmark, build it in Release mode:
```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --target registry_bench
./build-release/registry_bench
```

## Exercises

These changes practise the ideas in this guide. Each one needs only new
components, new systems and new spawn functions.
1. Add a "rapid fire" pickup. Give the ship a `RapidFire` component with a
   timer, and make `GunSystem()` use a shorter interval while the ship has it.
2. Add an enemy ship that drifts and fires at the player. Use the
   components that exist, and add one `EnemyAi` component.
3. Make the collision system faster with a grid. Put each rock in a grid
   cell, and test each projectile against the rocks in nearby cells only.
4. Add an `Exclude` option to `Each()`, for queries such as "all entities
   with a `Collider` but no `Ship`".
