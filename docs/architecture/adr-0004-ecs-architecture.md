# ADR-0004: Archetype-Based Entity Component System

## Status
Accepted

## Date
2026-04-15

## Engine Compatibility

| Field | Value |
|-------|-------|
| **Engine** | Custom — GXLib (self-hosted) |
| **Domain** | Core / Data-oriented runtime |
| **Knowledge Risk** | LOW — archetype ECS is a well-documented pattern (flecs, EnTT, Unity DOTS); C++20 templates/concepts used are within training data |
| **References Consulted** | `docs/engine-reference/gxlib/VERSION.md`, `GXLib/ECS/` source tree (World, Archetype, ComponentStorage, Query, System, EntityBridge), `CHANGELOG.md` Phase 4 |
| **Post-Cutoff APIs Used** | None |
| **Verification Required** | Iteration benchmark on a 100k-entity scene vs. the prior OOP path to confirm cache-locality gains; verify EntityBridge correctly mirrors Transform/Lifetime between OOP and ECS worlds |

## ADR Dependencies

| Field | Value |
|-------|-------|
| **Depends On** | ADR-0001 (documentation strategy), ADR-0002 (DX12 backend — ECS system outputs feed the render pipeline) |
| **Enables** | Future ADRs on deterministic simulation, save/load of world state, multi-threaded simulation scheduling, networked entity replication |
| **Blocks** | None (code already exists since Phase 4; this ADR is retroactive) |
| **Ordering Note** | Must precede any ADR proposing a new gameplay system that spans thousands of entities (particles, crowd AI, projectile pools) |

## Context

### Problem Statement
Before Phase 4, GXLib gameplay objects were OOP `Node`-style instances managed via `Prefab` + `SceneManager`. This model is ergonomic for hand-placed scene content (cameras, players, UI roots) but degrades when the game spawns tens of thousands of short-lived entities — projectiles, particles, crowd AI, gibs — because each instance incurs virtual dispatch, heap allocation, and cache-unfriendly iteration. DXLib users expect to be able to write `for (auto& enemy : enemies) enemy.Update(dt);` and not worry about performance; GXLib needs an underlying data layout that makes that loop fast regardless of entity count.

Phase 4 introduced an ECS to solve this, with the goal that high-volume systems (particles, AI, projectiles) run in the ECS while hand-placed scene objects keep the OOP/Node API. The question this ADR records: **which ECS flavour**, and **how do the ECS world and OOP scene interoperate**.

### Constraints
- Must coexist with the existing OOP `Node` / `Prefab` / `SceneManager` model — no wholesale rewrite of earlier phases
- C++20, must compile under MSVC and Clang-cl
- Deterministic iteration order is not a hard requirement for general systems, but save/load must be stable across runs (Phase 5 Scene Persistence)
- Hot reload must work: component types added or removed must not corrupt existing worlds
- Lua bindings (Phase 5) must be able to spawn entities and attach components

### Requirements
- Handle ≥ 100k live entities at 60 fps with component access latency ≤ 2 ms per frame on a mid-range CPU
- Support composition: entities gain/lose components at runtime without re-spawning
- Support query iteration filtered by component set (`Query<Transform, Velocity>`)
- Allow systems (`struct MovementSystem`) to declare their component dependencies statically
- Bridge to OOP scene: a Transform change on a Node must be visible to ECS systems, and an ECS-owned entity must be addressable from script

## Decision

**GXLib implements an archetype-based ECS (`GXLib/ECS/`) modelled after flecs / Unity DOTS, with an explicit `EntityBridge` for OOP↔ECS state mirroring. OOP Nodes remain the default for authored scene content; ECS is the path for high-volume gameplay.**

Concrete rules:

1. **Archetype layout.** Entities with identical component sets share an `Archetype`. Each archetype owns contiguous `ComponentStorage` arrays — one per component type — indexed by entity row. Iteration is SoA within an archetype, AoA across archetypes.

2. **Entity identity.** An `Entity` is a 64-bit value: 32-bit index + 32-bit generation (version). Generation invalidates handles on destroy to prevent use-after-free.

3. **Component registration.** Component types are plain C++ structs (POD preferred, move-only allowed). They register via a template-deduced `ComponentId<T>` at first use; no manual registration boilerplate required.

4. **Query API.** `World::Query<Transform, Velocity>()` returns an iterator that walks all archetypes containing the requested component set. Queries are cached by the World for repeated calls per frame.

5. **System API.** A `System` is a struct that declares its component access pattern and a `Run(World&, float dt)` method. Systems are registered on the `World` in a defined order; no automatic dependency resolution — ordering is the author's responsibility (keeps it simple, predictable).

6. **OOP↔ECS bridge.** `EntityBridge` mirrors a minimal shared component set between a `Node` (OOP) and an entity (ECS): Transform (required), Lifetime (optional), Tag (optional). Authored scene objects stay on the Node side; spawned short-lived entities live in ECS. A single Node may have a paired ECS entity for participation in ECS-driven queries (e.g., a hand-placed enemy that receives damage from an ECS projectile system).

7. **No inheritance in components.** Components are flat structs. No virtual methods. Behaviour belongs in systems.

8. **No systems that mutate entity structure mid-iteration.** Add/remove component operations during a query are deferred via a command buffer flushed between systems. This preserves archetype invariants and avoids iterator invalidation.

9. **Lua bindings (Phase 5) go through World.** Scripts call `world:spawn(components...)`, `world:destroy(entity)`, `world:get(entity, ComponentType)`. Raw archetype access is not exposed to scripts.

### Architecture Diagram

```
User code:
   world.Spawn(Transform{}, Velocity{}, Sprite{});
   world.Query<Transform, Velocity>().ForEach([dt](auto& t, auto& v) { ... });
   world.RegisterSystem<MovementSystem>();

GXLib/ECS/
   ├─ World               ─┐
   │   ├─ archetypes[]     │   archetype-keyed entity storage
   │   ├─ entity_index     │   Entity (32idx+32gen) → (archetype, row)
   │   └─ command_buffer   │   deferred structural changes
   │                       │
   ├─ Archetype  ─────────┤    one per unique component-set signature
   │   └─ ComponentStorage│    contiguous SoA per component type
   │                      │
   ├─ Query   ────────────┤    cached archetype-match iterator
   ├─ System  ────────────┤    author-registered update unit
   └─ EntityBridge ───────┘    OOP Node ↔ ECS entity Transform/Lifetime sync
                        │
                        ▼
                  Scene (Node/Prefab tree) — authored content
```

### Key Interfaces
- `gx::Entity` — 64-bit handle (32-bit index + 32-bit generation)
- `gx::World` — top-level container; one per simulation
- `world.Spawn(Components...)` → `Entity`
- `world.Destroy(Entity)` → deferred until command-buffer flush
- `world.Query<Cs...>()` → iterable matching archetypes
- `world.RegisterSystem<Sys>()` → adds system to update order
- `world.Update(float dt)` → runs systems in registered order, flushes command buffers between them
- `gx::EntityBridge::Attach(Node*, Entity)` — pairs a scene Node with an ECS entity

## Alternatives Considered

### Alternative 1: Sparse-set ECS (EnTT-style)
- **Description**: Each component type owns a sparse-set mapping `entity → component`. Entities are IDs; composition is a dynamic bag.
- **Pros**: Adding/removing a component is O(1) with no archetype migration cost; iteration per-component is cache-friendly; simpler data model
- **Cons**: Multi-component iteration (`Query<A,B>`) requires walking the smaller set and probing the others — slower than SoA archetype iteration for 3+ components. Memory overhead per-component for the sparse index.
- **Rejection Reason**: GXLib's target workloads (particles, AI, projectiles) query 3–5 components per system. Archetype iteration is measurably faster in this regime. Sparse-set's structural-change advantage doesn't outweigh the iteration cost here.

### Alternative 2: Stay OOP (no ECS)
- **Description**: Scale the existing Node/Prefab model with pools and cache-friendly batching on a per-system basis
- **Pros**: No dual model to maintain; no bridge complexity
- **Cons**: Each high-volume system reinvents its own SoA layout; virtual dispatch cost accumulates; scales poorly past ~10k entities
- **Rejection Reason**: Forces every gameplay author to re-solve data layout. ECS centralises that work.

### Alternative 3: Pure ECS (replace Nodes entirely)
- **Description**: Remove the Node/Prefab model; everything is an ECS entity
- **Pros**: One coherent model
- **Cons**: Hand-placed scene authoring is awkward without a tree structure; DXLib users expect `Node`-like addressing of named objects; Phase 0–3 code would need a full rewrite
- **Rejection Reason**: Breaks Phase 0–3 compatibility and ergonomics for the 80% case (hand-placed content)

## Consequences

### Positive
- High-volume systems (particles, AI, projectiles) get SoA data layout and cache-friendly iteration for free
- Component composition replaces inheritance trees — easier to reason about, easier to serialize
- Query API gives a uniform way to write "for all entities with X and Y" without per-system bookkeeping
- Deferred structural changes eliminate a whole class of iterator-invalidation bugs

### Negative
- Two object models (Node/OOP for authored content, Entity/ECS for high-volume) — developers must learn when to use which
- EntityBridge must be kept correct; drift between Node Transform and ECS Transform is a real bug class
- Archetype fragmentation: if every entity has a slightly different component mix, archetype count explodes and memory locality suffers — authoring discipline required
- System ordering is manual; no automatic dependency graph (chosen for simplicity, but means authors must think about ordering)

### Risks
- **Archetype migration cost on frequent add/remove** — if a component is toggled every frame, the entity migrates between archetypes each time, costing memory copies. *Mitigation*: document the pattern as a footgun; recommend "disabled" flag components instead of add/remove toggling.
- **EntityBridge drift** between Node Transform and ECS Transform. *Mitigation*: a single canonical owner per paired entity (Node drives if authored, ECS drives if spawned); bridge pulls/pushes once per frame at a defined phase.
- **Lua component access needs type-tag dispatch** that's slower than C++ side. *Mitigation*: batch Lua reads; expose query iteration to scripts rather than per-entity component gets.
- **Save/load stability across refactors** if component types are renamed. *Mitigation*: serialize by stable type-hash (Phase 5 Scene Persistence uses reflection-assigned IDs), not by C++ type name.

## GDD Requirements Addressed

| GDD System | Requirement | How This ADR Addresses It |
|------------|-------------|--------------------------|
| (None — ADR-only project per ADR-0001) | N/A | Requirement sourced from project charter and CHANGELOG Phase 4: "scale to 10k+ entities without hand-rolled SoA per system" — satisfied by archetype ECS + Query API |

## Performance Implications
- **CPU**: Target 100k entities with 4-component query iteration in ≤ 2 ms on mid-range CPU; dominant cost is archetype walk, not component access
- **Memory**: ~32 bytes per entity index + archetype storage proportional to live components; archetype count bounded by unique component-set signatures (watch for fragmentation)
- **Load Time**: Minor — component type registration happens lazily on first use
- **Network**: N/A for this ADR; future replication ADR will build on the Entity+generation handle model

## Migration Plan

Not applicable — this ADR is retroactive. ECS was introduced in Phase 4 and `EntityBridge`/Lua bindings added in Phase 5. Going forward:

1. New high-volume gameplay systems (any system expected to handle > 1k simultaneous entities) default to ECS; authoring tools and docs push users toward ECS for this case
2. New authored-scene features (Prefab, SceneManager additions) stay on the OOP side
3. Any proposal to unify the two models (eliminate one) requires a superseding ADR

## Validation Criteria
- 100k-entity benchmark scene runs at ≥ 60 fps with the Phase 4 query-heavy systems enabled
- Deterministic iteration under fixed seed on consecutive runs (for save/load stability, not for general-purpose determinism)
- EntityBridge round-trip test: mutate Node Transform → ECS-side Transform matches within 1 frame; vice versa
- Lua test: spawn 10k entities from script, query, destroy, no leaks
- No virtual dispatch in the ECS hot path (verify by inspection + profiling)

## Related Decisions
- ADR-0001 (Documentation strategy)
- ADR-0002 (DX12 backend — ECS systems feed GPU particle / instanced draw paths defined here)
- CHANGELOG.md Phase 4 (ECS initial implementation)
- CHANGELOG.md Phase 5 (Lua bindings, Scene Persistence, Hot Reload — all depend on ECS stability)
- `GXLib/ECS/World.h`, `GXLib/ECS/Archetype.h` (source of truth)
