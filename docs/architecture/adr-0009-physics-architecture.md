# ADR-0009: Physics Architecture (Custom In-House, 2D + 3D)

## Status
Accepted

## Date
2026-04-15

## Engine Compatibility

| Field | Value |
|-------|-------|
| **Engine** | Custom — GXLib (self-hosted) |
| **Domain** | Physics |
| **Knowledge Risk** | LOW — GJK/EPA, impulse-based solvers, Verlet cloth, sequential-impulse constraint solvers, and character-controller capsule-sweep patterns are thoroughly documented; no post-cutoff APIs involved |
| **References Consulted** | `docs/engine-reference/gxlib/VERSION.md`, `GXLib/Physics/*` source tree, CHANGELOG Phases 1/3/4/5 |
| **Post-Cutoff APIs Used** | None |
| **Verification Required** | Deterministic integration step under fixed timestep; GJK convergence on degenerate shapes (thin boxes, coplanar polys); constraint stability at high mass ratios (e.g. 100:1); cloth behaviour under tearing/teleport events; character controller stair/slope handling matches standard test suites |

## ADR Dependencies

| Field | Value |
|-------|-------|
| **Depends On** | ADR-0001 (doc strategy), ADR-0004 (ECS — physics components live in the ECS; PhysicsWorld queries may be driven by ECS systems), ADR-0006 (Job System — broadphase and constraint islands parallelise on workers) |
| **Enables** | Future ADRs on networked physics (lag compensation), deterministic replay, GPU-accelerated cloth, ray-query integration with rendering |
| **Blocks** | None (code already exists across Phases 1/3/4/5; retroactive) |
| **Ordering Note** | Must precede any gameplay ADR relying on physics queries (shooting, pickup, character movement) |

## Context

### Problem Statement
GXLib must provide a complete physics stack: 2D rigid bodies (for 2D gameplay — top-down, platformer), 3D rigid bodies with friction/restitution materials, 6 constraint types (hinge, slider, fixed, ball-and-socket, distance, 6DOF), narrow-phase GJK/EPA collision for convex shapes, a broadphase, cloth (Verlet), ragdoll, character controller, buoyancy, and vehicle physics. The question is whether to integrate an established external engine (Jolt, Bullet, PhysX) or build and maintain an in-house stack. GXLib already has an in-house implementation across Phases 1/3/5; this ADR records the commitment to that path, the architectural shape, and the integration contract with the ECS and Job System.

### Constraints
- Must coexist with ECS (ADR-0004) — physics components are ECS components; `PhysicsWorld` queries are called from ECS systems
- Must parallelise through the Job System (ADR-0006) — broadphase and constraint-island solving run on workers; no physics-owned thread pool
- Deterministic under fixed timestep (required for future save/load + replay; not for networked lockstep — that's a future ADR's decision)
- Windows-only, DX12 era — may leverage SSE/AVX but not GPU compute for the core solver (GPU cloth is a separate future ADR)
- Hot reload of physics materials (ADR-0007) — asset changes rebind live bodies

### Requirements
- 2D and 3D rigid-body simulation (separate worlds: `PhysicsWorld2D`, `PhysicsWorld3D`)
- Shapes: sphere, box, capsule, cylinder, convex hull, triangle mesh (static only), heightfield (future)
- Narrow-phase: GJK + EPA for convex pairs; specialised primitives (sphere-sphere, box-box) fast-path where worthwhile
- Broadphase: dynamic AABB tree (bounding volume hierarchy) — insert/update/query O(log n)
- Constraints: hinge, slider, fixed, ball-and-socket, distance, 6DOF-generic
- Cloth: Verlet integrator with distance + bending constraints; tearing; pin-points
- Ragdoll: humanoid preset (15 bones) + custom bone chains via `RagdollBuilder`
- Character controller: capsule-based; slope/step handling; slide-on-wall; is-grounded query
- Buoyancy: for water-volume interaction
- Vehicle: raycast-wheel model (not full wheel-as-constraint)
- Materials: friction, restitution, density — hot-reloadable
- Perf target: 1,000 active dynamic rigid bodies in 3D at 60 fps on a mid-range CPU within a 3 ms budget

## Decision

**GXLib ships a custom in-house physics stack. Two separate worlds (2D + 3D) each own a dynamic AABB-tree broadphase, GJK/EPA narrow-phase, and a sequential-impulse constraint solver with position-based correction (Baumgarte + split-impulse). Cloth uses Verlet. Character controller uses capsule sweep + slide resolution. Physics runs on a fixed timestep (60 Hz default) with interpolation for rendering; broadphase and island solving submit to the Job System. Physics bodies are referenced from ECS components via opaque handles (no raw pointers in components). Materials flow through AssetDatabase.**

Concrete rules:

1. **No external physics engine.** No Jolt, Bullet, PhysX. In-house stack is the decision. Rationale in Alternatives.

2. **Separate 2D and 3D worlds.** `PhysicsWorld2D` (2-vector math) and `PhysicsWorld3D` (3-vector math + quaternions) are distinct classes with separate broadphases. No 2D-in-3D-world hack. A project may use one or both.

3. **Fixed-timestep simulation.** Default `dt = 1/60` s. The engine accumulates wall-clock delta and runs whole steps; leftover time produces an interpolation factor used by rendering for smooth visuals. Variable timestep is forbidden in the core solver loop. Users tune step rate at `PhysicsWorld::SetTimestep`.

4. **Broadphase: Dynamic AABB tree.** Reinsertion on body move when the bounding AABB exits its fattened envelope. Ray queries, overlap queries, and shape casts all route through the tree. Tree update is parallelisable — insertions from the current frame batch and rebuild the touched subtrees on Job System workers.

5. **Narrow-phase: GJK/EPA with fast paths.** GJK for distance + closest-point; EPA for penetration depth. Specialised pairs (sphere-sphere, sphere-box, sphere-capsule, box-box via SAT) fast-path where measurably faster. Contact manifold reduction to 4 points before handing to the solver.

6. **Constraint solver: Sequential-impulse + warm starting.** Islands are built per frame (connected components of bodies-in-contact and joint-linked bodies). Each island solves independently — islands run in parallel on Job System workers. Within an island: N velocity iterations (default 8) + M position iterations (default 3). Joint limits, contacts, and friction all go through the same impulse pipeline. Baumgarte + split-impulse for position error.

7. **Constraint types (6).** `Hinge`, `Slider`, `Fixed`, `BallAndSocket`, `Distance`, `6DOFGeneric`. Damping on joints is a supported parameter (implemented as a weak velocity constraint). Joint breakage (force threshold) supported.

8. **Cloth: Verlet + distance constraints.** `ClothSimulator` integrates per-particle positions via Verlet, then relaxes distance + bending constraints over N iterations. Pins anchor selected particles to world or body transforms. Tearing removes constraints when stretch exceeds a threshold. Collision against capsule and sphere colliders (mesh collision is too expensive for the in-house solver).

9. **Ragdoll: `RagdollBuilder` constructs a chain of rigid bodies + constraints from a skeleton. Humanoid preset creates 15-body layout (head, torso×2, upper/lower arms, hands, upper/lower legs, feet).

10. **Character controller: capsule-based.** `CharacterController` is NOT a rigid body — it's a kinematic capsule that sweep-tests against the world. Responds to slopes (grounded up to max-slope-angle), stairs (step-up via vertical sweep), and slides on walls when blocked. Is-grounded query, move-and-slide, move-and-collide primitives.

11. **ECS integration.** Physics components in ECS (`RigidBody3D`, `Collider`, `CharacterController`) hold an opaque `BodyHandle` that refers to storage owned by `PhysicsWorld3D`. The ECS component holds metadata (mass, material AssetId); the physics world holds per-frame state (position, velocity, accumulated forces). `EntityBridge` (ADR-0004) mirrors Transform between the two. Never store a raw pointer to a physics body in a component.

12. **Job System integration.** `PhysicsWorld::Step(dt)` submits: broadphase refit (single or parallelised), narrow-phase per potential pair (ParallelFor over pair buckets), island build (serial, short), and island solve (one Job per island, depends on build). Post-step ECS mirror pass runs on the main thread.

13. **Materials via AssetDatabase.** `PhysicsMaterial` is an asset — friction, restitution, density. `AssetHandle<PhysicsMaterial>` lives on colliders; hot reload via ADR-0007's AssetReloader rebinds live bodies' effective material.

14. **Thread safety.** Read-only queries (`PhysicsWorld::Raycast`, `OverlapSphere`, `ShapeCast`) are safe to call **from any thread at any time, including concurrent with `PhysicsWorld::Step`**. The broadphase AABB-tree exposes a versioned, copy-on-write read view: writers (Step's broadphase refit + body-move reinsertion) publish a new tree-root pointer atomically; readers acquire the current root once at query entry and traverse a stable snapshot. Concurrent queries observe a consistent prior-frame or current-frame snapshot (never torn). Mutations (`AddBody`, `RemoveBody`, `AddConstraint`, `SetLinearVelocity`, `ApplyForce`) are main-thread-only or enqueued via the command buffer, which flushes at the start of the next step. **Consumers that rely on this concurrent-read guarantee**: AudioOcclusion (ADR-0010 §5, raycast Jobs at Normal priority), Networking InterestManagement (ADR-0013 §6, broadphase reuse for spatial visibility filtering).

15. **Deterministic island solve (binding for rollback per ADR-0013 §13).** Island solve is parallelised across Job System workers, but the final accumulation respects deterministic island-ID order to keep floating-point reduction order identical across runs and CPUs. Concretely:
    - Islands are numbered deterministically by lowest-body-id-in-island; this numbering is stable across runs given identical inputs.
    - Each island-solve Job writes its result into a pre-allocated slot indexed by island-ID (no shared accumulator, no atomic add).
    - After a barrier on all island Jobs, a single main-thread pass merges results in ascending island-ID order. Reduction order is therefore independent of Job completion order or worker thread count.
    - Fast-math (`/fp:fast`) is **disabled** in physics solver translation units — see Forbidden Patterns.

### Architecture Diagram

```
ECS side:                                      Physics side:
  RigidBody3D component  ───► BodyHandle ────► PhysicsWorld3D
    { mass, material_id,                         ├── BroadphaseAABBTree
      body_handle }                              ├── NarrowPhase (GJK/EPA + fast paths)
                                                 ├── ConstraintSolver (sequential impulse)
  Collider component       ───► ShapeRef  ────►  ├── Islands[]      (parallel solve)
    { shape (sphere/box/...),                    ├── Cloth[]         (Verlet simulator)
      material_handle,                           ├── CharacterControllers[] (capsule sweep)
      local_transform }                          ├── Ragdolls[]      (ragdoll body chains)
                                                 └── Materials       (AssetDB-backed)

  EntityBridge (ADR-0004)  ◄────Transform────►  PhysicsWorld3D (per-body transform)

Step(dt):
  main thread ─► enqueue body mutations
              ─► PhysicsWorld::Step(dt)
                   │
                   ├── Broadphase refit       ─► JobSystem
                   ├── Narrow-phase           ─► ParallelFor (pair buckets)
                   ├── Build islands          (serial)
                   ├── Solve islands          ─► Job per island (parallel)
                   ├── Integrate positions    ─► ParallelFor
                   └── Cloth + ragdoll step   ─► Jobs (depends on rigid-body step)
              ─► mirror back to ECS Transforms (main thread)
```

### Key Interfaces

- `gx::PhysicsWorld2D`, `gx::PhysicsWorld3D` — singletons per scene
- `gx::BodyHandle` — 64-bit opaque (index + generation)
- `BodyHandle AddBody(const BodyDesc&)`, `void RemoveBody(BodyHandle)`
- `void Step(float dt)` — fixed-timestep; runs accumulator internally if called per render frame
- `RaycastHit Raycast(ray, filter)`, `OverlapResults OverlapSphere(center, radius, filter)`, `ShapeCastResult Cast(shape, from, to, filter)`
- `void AddConstraint(ConstraintType, BodyHandle a, BodyHandle b, params)` / `RemoveConstraint`
- `gx::CharacterController` — `MoveAndSlide(velocity, dt)`, `IsGrounded()`, `GetGroundNormal()`
- `gx::ClothSimulator::Create(desc) → ClothHandle`, `Step(dt)`, `Pin(particleIdx, worldPos | bodyAttach)`
- `gx::RagdollBuilder::Humanoid(skeleton) → Ragdoll`, `Custom(boneChain) → Ragdoll`

## Alternatives Considered

### Alternative 1: Integrate Jolt Physics
- **Description**: Use Jolt (open-source, MIT, Horizon Zero Dawn lineage) as the 3D physics backend
- **Pros**: Battle-tested in a shipped AAA game; excellent multi-threaded scaling; active development; rich feature set including character controller, vehicles, and soft body
- **Cons**: Adding a 100kLoC dependency that GXLib does not control; Jolt's API model diverges from our ECS/Job-System contract (Jolt owns its own thread pool and job system); retrofitting it to use GXLib's JobSystem is non-trivial; 2D still needs a separate solution (Jolt is 3D-only); reloadable materials and our AssetDatabase integration require adapter code
- **Rejection Reason**: The in-house stack already exists (Phases 1/3/5) and satisfies the feature set. Migrating to Jolt discards that investment, introduces a second job/thread model (violating ADR-0006), and leaves 2D uncovered. Jolt is a strong option for a greenfield engine; not compelling for retrofit.

### Alternative 2: Integrate Bullet Physics
- **Description**: Use Bullet (open-source, zlib) for 3D
- **Pros**: Long history, widely known, works
- **Cons**: Bullet is older; performance trails Jolt and PhysX on modern CPUs; multi-threading support is retrofitted, not native; same ECS / job-system integration concerns as Jolt; no 2D
- **Rejection Reason**: If choosing external, Jolt is the better 3D option — Bullet is not competitive. And same arguments against external apply here.

### Alternative 3: Integrate PhysX 5
- **Description**: Use NVIDIA PhysX 5 (BSD-3)
- **Pros**: Industry-leading features (GPU acceleration, cloth, destruction); AAA pedigree
- **Cons**: Heavy dependency; GPU-accelerated paths need CUDA on NVIDIA for best results — breaks the "any consumer GPU" promise of ADR-0002; complex build integration; historically tricky on Windows-only indie scale; same ECS/job concerns
- **Rejection Reason**: Overkill for GXLib's scope; GPU-path assumptions conflict with our hardware floor; complexity doesn't pay off for an SDK that already has a working in-house stack

### Alternative 4: Single unified 2D/3D world (treat 2D as degenerate 3D)
- **Description**: Use only `PhysicsWorld3D`; simulate 2D games by constraining bodies to Z=0 and rotation to the Z axis
- **Pros**: One codebase; less duplication
- **Cons**: 2D workloads pay 3D cost (SIMD 3-vector math, quaternions, 3×3 inertia tensors) when 2×2 matrices would suffice; broadphase in 3D on a plane wastes a dimension; constraint solver must repeatedly re-enforce the plane constraint; 2D convenience APIs (angle as scalar, 2D raycast) don't map cleanly
- **Rejection Reason**: 2D is a first-class case for GXLib (DXLib heritage). Separate world gives cleaner API and ~2× faster 2D stepping.

## Consequences

### Positive
- Full control: tweak solver iterations, constraint formulations, integration order to match GXLib's goals
- No external dependency lifecycle to manage (version bumps, security patches, build matrix)
- Clean integration with ECS + Job System + AssetDatabase — native to our contracts, no adapter layers
- Windows-only + C++20 lets us use SIMD / `std::atomic` freely without cross-platform compromise
- 2D and 3D have separate tight APIs that suit their workloads

### Negative
- Maintenance burden is ours — every solver bug is a GXLib bug, every new feature is a GXLib task
- Not as battle-tested as Jolt / PhysX — edge cases (degenerate shapes, high mass ratios, stacking tall blocks) may surface over time
- No GPU-accelerated fallback for massive-scale cloth / soft-body; those remain a future ADR if ever needed
- Documentation / community knowledge is smaller (we write our own, users learn from our headers)

### Risks
- **Solver stability at high mass ratios or tall stacks.** *Mitigation*: warm starting, sufficient velocity iterations (default 8), clamp allowed mass ratio in docs (~100:1 recommended).
- **GJK/EPA degenerate cases** (coplanar polys, zero-thickness boxes) cause infinite loops or wrong contacts. *Mitigation*: iteration cap in both algorithms; SAT fast-path for box-box skips GJK; unit tests cover known degenerate inputs.
- **Determinism across CPUs** is fragile with parallel islands — floating-point associativity varies with reduction order. *Mitigation*: deterministic island ordering by body-ID (not scheduling order); reductions use a fixed traversal order; fast-math compiler flags disabled for the solver translation units.
- **Character controller staircase traversal** is a perennial bug magnet. *Mitigation*: established algorithm (vertical step-up + horizontal slide), regression tests on a standard stairway fixture.
- **Cloth explodes at low constraint iteration counts** under fast motion. *Mitigation*: recommend ≥ 10 iterations for fast cloth; allow per-cloth override; substep if strain > threshold.
- **ECS ↔ PhysicsWorld sync lag** if mirror pass skips a frame. *Mitigation*: mirror is part of `Step()` — always runs before `Step` returns.

## GDD Requirements Addressed

| GDD System | Requirement | How This ADR Addresses It |
|------------|-------------|--------------------------|
| (None — ADR-only project per ADR-0001) | N/A | TR-chr-003 ("Physics: rigid body, cloth, ragdoll, GJK/EPA") — elevated from Gap to Covered by this ADR |

## Performance Implications
- **CPU**: 1,000 active rigid bodies in 3D at 60 Hz within 3 ms/frame on mid-range CPU (8 velocity + 3 position iterations, 8 worker threads). Broadphase refit ≤ 0.3 ms. Narrow-phase ≤ 0.8 ms. Constraint solve ≤ 1.5 ms. Mirror + misc ≤ 0.4 ms.
- **Memory**: ~1 KB per dynamic rigid body (body state + AABB-tree node). Cloth ~120 bytes per particle. Ragdoll humanoid ~15 KB.
- **Load Time**: Material asset loading goes through AssetDatabase (ADR-0007) — no physics-specific cost.
- **Network**: N/A (future networked physics is a separate ADR)

## Migration Plan

Not applicable — this ADR is retroactive. Physics landed incrementally: Phase 1 (GJK/EPA, capsule, constraints, NavMesh/RVO), Phase 3 (cloth, material DB), Phase 4 (broadphase + solver maturation), Phase 5 (character controller, ragdoll). Going forward:

1. Any proposal to swap the in-house stack for an external engine (Jolt, etc.) must supersede this ADR explicitly
2. New physics features (soft body, GPU cloth, destruction) extend this ADR
3. Networked physics (lag compensation, rollback) is a separate future ADR that builds on determinism guarantees recorded here

## Validation Criteria
- 1,000-body stress test: 1,000 dynamic boxes in a bounded volume, stable at 60 fps within 3 ms budget on 8-core mid CPU
- Stack stability: 20-box tower under gravity remains stable for 10 seconds without drift
- GJK degenerate test: coplanar triangle vs thin box returns consistent penetration depth over 100 trials
- Constraint stability at 100:1 mass ratio: hinge joint between 1 kg and 100 kg bodies does not explode under gravity
- Character controller: standard stairway fixture (5 steps, 0.3 m each) — controller climbs without stuttering; slides correctly against a 60° wall
- Cloth integration: 1,024-particle cloth over a capsule collider runs at 60 Hz within 0.5 ms
- Determinism: two runs with identical seeds + inputs produce identical body positions after 1,000 steps (tolerance: 1e-5)
- Hot reload: modify a PhysicsMaterial asset; live body friction/restitution changes on next step

## Related Decisions
- ADR-0001 (Documentation strategy)
- ADR-0004 (ECS — physics components live in ECS; opaque BodyHandle prevents raw pointers in components)
- ADR-0006 (Job System — broadphase, narrow-phase, island solving submit here; no physics-owned thread pool)
- ADR-0007 (Asset Database — PhysicsMaterial is asset-backed; hot reload via AssetReloader)
- `GXLib/Physics/{PhysicsWorld2D,PhysicsWorld3D,RigidBody3D,PhysicsConstraint,PhysicsShape,PhysicsMaterial,ClothSimulator,RagdollBuilder,CharacterController,MeshCollider,BuoyancySystem,VehiclePhysics}.{h,cpp}` (source of truth)
- CHANGELOG.md Phases 1, 3, 4, 5
