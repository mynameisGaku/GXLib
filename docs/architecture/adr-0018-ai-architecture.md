# ADR-0018: AI Architecture (Behavior Tree, GOAP, NavMesh, RVO)

## Status
Accepted

## Date
2026-04-17

## Engine Compatibility

| Field | Value |
|-------|-------|
| **Engine** | Custom — GXLib (self-hosted) |
| **Domain** | AI / Decision-Making / Pathfinding |
| **Knowledge Risk** | LOW — behavior trees, GOAP backward search, grid/voxel/polygon-mesh A*, funnel algorithm, and RVO half-responsibility cone model are well-documented patterns within LLM training data |
| **References Consulted** | `GXLib/AI/{BehaviorTree,GOAPPlanner,NavMesh,NavMesh3D,PolyNavMesh,NavAgent,RVOSolver}.{h,cpp}` (source of truth), `Tests/test_AIBehaviorTree.cpp`, `Tests/test_AIExtended.cpp`, `Tests/test_NavMesh.cpp`, `Tests/test_NavMesh3D.cpp`, `Tests/test_PolyNavMesh.cpp`, `Tests/test_GOAPPlanner.cpp` (6 test files), CHANGELOG Phases 1-2 |
| **Post-Cutoff APIs Used** | None |
| **Verification Required** | A* convergence under degenerate graphs (zero-cost cells, all-blocked); NavMesh dynamic-obstacle removal restores original walkability (base-grid pattern); GOAPPlanner iteration cap prevents runaway search on unsolvable goals; RVO stability under dense-crowd (100+ agents) without oscillation; PolyNavMesh funnel correctness on concave polygon chains; NavMesh3D off-mesh link pathfinding integrates correctly with voxel A* |

## ADR Dependencies

| Field | Value |
|-------|-------|
| **Depends On** | ADR-0001 (documentation strategy) |
| **Enables** | Future ADRs on ECS-integrated crowd AI (batch NavAgent tick via ECS System + JobSystem), AI perception (sight/hearing/memory), utility-based AI (as an alternative to BT/GOAP), hierarchical task networks |
| **Blocks** | None (code already exists since Phases 1-2; retroactive) |
| **Ordering Note** | The AI module has zero compile-time dependencies on ECS (ADR-0004), Physics (ADR-0009), or JobSystem (ADR-0006). Integration with those systems is an application-level concern — callers extract geometry from physics for NavMesh::BuildFromGeometry, and callers submit AI ticks as Jobs if desired. This intentional decoupling means ADR-0018 can be implemented and tested independently. |

## Context

### Problem Statement

GXLib ships a complete AI toolbox across Phases 1-2: behavior trees for decision-making, GOAP for goal-oriented planning, three NavMesh variants (2D grid, 3D voxel, polygon mesh) for pathfinding, RVO for local avoidance, and NavAgent for path-following. All of this code is tested (6 test files, 100+ test cases) and production-ready, but no ADR has codified the architecture, the contracts between subsystems, or the threading model. This ADR closes that gap retroactively.

### Constraints

- AI module links only `GXLib_Foundation` (Math + Container + Core utilities). No ECS, Physics, or JobSystem dependency at compile time — callers bridge those if needed.
- All AI calls are synchronous. Thread-safety is the caller's responsibility.
- NavMesh builds can be expensive (rasterization) — callers should do this at load time or on a background thread, not per-frame.
- BehaviorTree Blackboard uses `gx::HashMap<String, Value>` — iteration order is nondeterministic. Per ADR-0013 §13, gameplay code in rollback windows must not iterate hash containers. AI ticking in rollback must use deterministic Blackboard access patterns (key-by-key Get, not iteration).
- RVO operates in XZ plane only; Y is pass-through.

### Requirements

- **Behavior Tree**: BTNode hierarchy (Selector/Sequence/Parallel + Inverter/Repeater/Succeeder + Action/Condition), Blackboard shared state, per-frame Tick with Running resumption.
- **GOAP**: WorldState (bool KV), GOAPAction (preconditions + effects + cost + procedural precondition), backward A* planner, best-goal selection by priority, iteration cap, plan validation.
- **NavMesh (2D grid)**: Build from bounds/terrain/geometry, A* pathfinding with optional smoothing, dynamic obstacles (AABB/Cylinder) with handle-based lifecycle, per-cell cost multiplier.
- **NavMesh3D (voxel)**: Build from bounds/geometry, 3D A* with state-bitmask filter, off-mesh links (jump/ladder/teleport), raycast through voxel grid.
- **PolyNavMesh (triangle mesh)**: Build from vertex/index arrays, shared-edge adjacency, A* on polygon centroids + funnel algorithm for smooth paths, slope filtering.
- **RVO**: Stateless free function `ComputeAvoidanceVelocity`, half-responsibility velocity-obstacle model, XZ-plane projection.
- **NavAgent**: Path follower on NavMesh, optional RVO integration via `UpdateWithNeighbors`, configurable speed/angular speed/stopping distance/radius.

## Decision

**GXLib AI is a synchronous, dependency-free toolbox organised into four layers: Decision (BehaviorTree + GOAP), Navigation (NavMesh + NavMesh3D + PolyNavMesh), Avoidance (RVO), and Agent (NavAgent). The module links only GXLib_Foundation and has zero compile-time coupling to ECS, Physics, or JobSystem. All calls are single-threaded by contract — callers bridge to the Job System or ECS as their application demands. Three NavMesh variants serve different spatial needs (2D uniform grid, 3D voxel, polygon mesh); all use A* with domain-appropriate heuristics.**

Concrete rules:

1. **Four-layer architecture.**
   ```
   Decision     BehaviorTree (BTNode hierarchy + Blackboard)
                GOAPPlanner (WorldState + Action + backward A*)
   Navigation   NavMesh (2D grid A* + dynamic obstacles)
                NavMesh3D (3D voxel A* + off-mesh links)
                PolyNavMesh (triangle mesh A* + funnel smooth)
   Avoidance    RVO::ComputeAvoidanceVelocity (stateless)
   Agent        NavAgent (path follower + RVO integration)
   ```
   Each layer is independently usable. NavAgent composes Navigation + Avoidance but Decision is decoupled — the caller connects BT/GOAP output to NavAgent::SetDestination.

2. **Behavior Tree.**
   - Node taxonomy: composites (Selector, Sequence, Parallel), decorators (Inverter, Repeater, Succeeder), leaves (Action, Condition).
   - `BTStatus { Success, Failure, Running }`. Composites resume at `m_currentChild` for Running nodes.
   - `BTParallel` ticks all children every frame; policy `RequireAll` or `RequireOne` determines when it returns Success.
   - `Blackboard` stores `std::variant<bool, int, float, String, Vector3>` keyed by `gx::String`. HashMap-backed — callers must not iterate in rollback windows (per ADR-0013 §13).
   - `BehaviorTree::Tick(dt)` drives the root with the tree-owned Blackboard. One tick per frame.

3. **GOAP Planner.**
   - `WorldState` is a bool-valued key-value map. `Satisfies(goal)` checks all goal entries match.
   - `GOAPAction` has preconditions, effects, cost (float), and optional procedural precondition (runtime lambda check).
   - `MakePlan` runs backward A* from goal state: starts at goal, applies action effects in reverse, heuristic = number of unsatisfied conditions. `MaxIterations` (default 1000) caps search.
   - `MakePlanForBestGoal` selects the highest-priority goal and plans for it.
   - `ValidatePlan` forward-simulates the plan from start state to confirm it reaches goal.
   - Planner is stateless between calls (except registered actions and last stats).

4. **NavMesh (2D grid).**
   - Uniform-cell grid on XZ plane. `Build(bounds, cellSize, maxClimb, maxSlope)` or `BuildFromTerrain`/`BuildFromGeometry`.
   - Each cell has `height`, `walkable`, `costMultiplier`.
   - A* with diagonal-distance heuristic. Optional post-path smoothing.
   - Dynamic obstacles (AABB, Cylinder) via handle-based API. Obstacle application bakes onto `m_grid`; removal restores from `m_baseGrid` — no incremental unbake, full restore from baseline.
   - `FindNearestWalkable` for position snapping. `IsWalkable` for point queries.

5. **NavMesh3D (voxel).**
   - 3D uniform voxel grid. `VoxelState { Blocked, Open, Water, Air }`.
   - A* with 26-neighbor connectivity. State bitmask filter allows path through specific voxel types (e.g., Water+Open but not Air).
   - Off-mesh links (jump, ladder, teleport) with cost and bidirectional flag. A* considers them as virtual edges.
   - `Raycast` through voxel grid for line-of-sight queries.

6. **PolyNavMesh (triangle mesh).**
   - Triangle-mesh graph with shared-edge adjacency. `Build` rasterizes from vertex/index arrays, computes adjacency, filters steep triangles by `maxSlope`.
   - A* on polygon centroids; `FunnelSmooth` (Simple Stupid Funnel Algorithm) produces smooth waypoints.
   - `FindNearestPolygon` for point-to-mesh snapping. `IsPointInMesh` via barycentric test.

7. **RVO (Reciprocal Velocity Obstacles).**
   - Stateless free function: `gx::RVO::ComputeAvoidanceVelocity(self, desiredVelocity, others, timeHorizon)`.
   - XZ-plane projection. Velocity obstacle cone per neighbour within `maxSpeed * timeHorizon + combinedRadius`.
   - Half-responsibility model: each agent assumes the other adjusts by half. Adjusts desired velocity by pushing out of VO cones; clamps to `maxSpeed`.
   - Simplified ORCA-style linear constraint, not full LP solver. Sufficient for crowds up to ~100 agents without oscillation.

8. **NavAgent (path follower).**
   - Binds to a `NavMesh*` via `Initialize`. `SetDestination` calls `FindPath` immediately.
   - `Update(dt)` advances position along path, rotates toward next waypoint.
   - `UpdateWithNeighbors(dt, neighbors)` additionally calls `RVO::ComputeAvoidanceVelocity` with neighbour agents' states.
   - Configurable: `speed`, `angularSpeed`, `stoppingDistance`, `radius`, `enableAvoidance`.
   - `HasReachedDestination()` returns true when within `stoppingDistance` of final waypoint.

9. **Threading contract.**
   - AI objects (BehaviorTree, NavAgent, NavMesh, NavMesh3D, PolyNavMesh, GOAPPlanner) are **per-instance non-reentrant**: each instance must be owned by a single thread at any given time. The AI module provides no internal synchronisation — the caller guarantees exclusivity of each instance.
   - This permits parallel AI ticking via ADR-0006 JobSystem: 100 BehaviorTree instances across 100 Jobs is safe iff each Job owns its own Tree. Sharing a single BehaviorTree, NavAgent, or NavMesh across two concurrent Jobs is undefined behaviour.
   - Single-threaded usage (all AI on the main thread) is the simplest safe mode and is the default for non-batched gameplay. Parallel usage is opt-in by the caller via JobSystem submission.
   - NavMesh builds (`Build`, `BuildFromGeometry`, `BuildFromTerrain`) are heavy and should run off the main thread (background Job) with a completion callback. The NavMesh must not be queried during build — queriers must synchronise against build completion (see `navmesh_query_during_build` forbidden pattern in §10).
   - GOAPPlanner::MakePlan is pure (reads action list, writes a new Plan) — safe to call concurrently from multiple threads if the action list is not mutated concurrently.
   - **EventBus interaction (ADR-0016 §5)**: an AI Action running on a worker Job MUST use `EventBus::QueueFromWorker<T>` to produce events — calling `Fire<T>` from a worker thread is the `eventbus_fire_from_worker_thread` forbidden pattern (ADR-0016). This applies equally whether the worker tick is ticking AI or other gameplay.

10. **Forbidden patterns.**
    - `blackboard_iteration_in_rollback` — iterating `Blackboard::GetAll()` (HashMap) inside a rollback re-simulation window. Use key-by-key `Get()` instead.
    - `navmesh_query_during_build` — calling `FindPath`/`IsWalkable` while `Build` is running on another thread. Caller must synchronise.
    - `rvo_with_y_dependent_avoidance` — RVO operates in XZ only; using Y-separated agents (e.g., multi-floor) requires per-floor agent filtering before calling `ComputeAvoidanceVelocity`.

### Architecture Diagram

```
   Game code / ECS System
       │
       │ BehaviorTree::Tick(dt)    GOAPPlanner::MakePlan(state, goal)
       │        ↓                           ↓
       │ BTStatus (decision)       Plan { actions[], cost }
       │        ↓                           ↓
       │        └──── destination ──────────┘
       │                    ↓
       │ NavAgent::SetDestination(target)
       │        ↓
       │ NavMesh::FindPath(start, end, path)      [2D grid A*]
       │ NavMesh3D::FindPath(start, end, path)    [3D voxel A*]
       │ PolyNavMesh::FindPath(start, end)         [triangle A* + funnel]
       │        ↓
       │ NavAgent::Update(dt)  or  UpdateWithNeighbors(dt, neighbors)
       │        ↓                           ↓
       │   position advance         RVO::ComputeAvoidanceVelocity
       │                                    ↓
       │                            adjusted velocity → movement
       ▼
   Entity position update (ECS Transform or direct)
```

### Key Interfaces

```cpp
namespace gx {

// --- Decision Layer ---

enum class BTStatus { Success, Failure, Running };

class Blackboard {
    void Set(const String& key, const Value& value);
    template<typename T> T Get(const String& key, const T& def = T{}) const;
    bool Has(const String& key) const;
};

class BTNode { virtual BTStatus Tick(float dt, Blackboard& bb) = 0; };
class BTSelector  : public BTNode { void AddChild(shared_ptr<BTNode>); };
class BTSequence  : public BTNode { void AddChild(shared_ptr<BTNode>); };
class BTParallel  : public BTNode { /* RequireAll / RequireOne */ };
class BTAction    : public BTNode { /* std::function<BTStatus(float, Blackboard&)> */ };
class BTCondition : public BTNode { /* std::function<bool(const Blackboard&)> */ };
class BehaviorTree { BTStatus Tick(float dt); Blackboard& GetBlackboard(); };

struct GOAPAction { String name; float cost; WorldState preconditions, effects; };
struct GOAPGoal   { String name; WorldState targetState; float priority; };
struct Plan       { Vector<const GOAPAction*> actions; float totalCost; bool valid; };
class GOAPPlanner {
    void AddAction(const GOAPAction&);
    Plan MakePlan(const WorldState& current, const GOAPGoal& goal) const;
    Plan MakePlanForBestGoal(const WorldState&, const Vector<GOAPGoal>&) const;
};

// --- Navigation Layer ---

class NavMesh {
    bool Build(float minX, float minZ, float maxX, float maxZ, float cellSize, ...);
    bool FindPath(const Vector3& start, const Vector3& end, Vector<Vector3>& path, bool smooth) const;
    uint32_t AddObstacleAABB(...); void RemoveObstacle(uint32_t handle);
};

class NavMesh3D {
    bool Build(const Vector3& min, const Vector3& max, float voxelSize);
    bool FindPath(const Vector3& start, const Vector3& end, Vector<Vector3>& path, uint8_t allowedStates) const;
    uint32_t AddOffMeshLink(const OffMeshLink&);
};

class PolyNavMesh {
    void Build(const float* verts, uint32_t vertCount, const uint32_t* indices, uint32_t idxCount);
    Vector<NavPoint> FindPath(const NavPoint& start, const NavPoint& end) const;
};

// --- Avoidance Layer ---

namespace RVO {
    Vector3 ComputeAvoidanceVelocity(const AgentState& self, Vector3 desired,
                                      const Vector<AgentState>& others, float timeHorizon);
}

// --- Agent Layer ---

class NavAgent {
    void Initialize(NavMesh* mesh);
    void SetDestination(const Vector3& target);
    void Update(float dt);
    void UpdateWithNeighbors(float dt, const Vector<NavAgent*>& neighbors);
    bool HasReachedDestination() const;
};

} // namespace gx
```

## Alternatives Considered

### Alternative 1: Use a third-party AI library (recastnavigation / MicroPather / BehaviorTree.CPP)

- **Pros**: Battle-tested, community maintained, feature-rich (Recast/Detour has navmesh generation + crowd simulation).
- **Cons**: Recast is C-style with its own allocator model (conflicts with gx:: containers); BehaviorTree.CPP brings Boost and XML parsing dependencies; none integrate with GXLib's Math/Container/Foundation layer without an adapter.
- **Rejection Reason**: In-house implementation already exists, is tested, and fits the Foundation-only dependency model. The maintenance cost is bounded by the stable nature of these algorithms.

### Alternative 2: Single NavMesh implementation (polygon mesh only, skip grid/voxel)

- **Pros**: One API to learn; PolyNavMesh is the most general.
- **Cons**: 2D grid NavMesh is simpler for tile-based games and 2D top-down games; 3D voxel NavMesh handles multi-floor/cave geometry that triangle mesh struggles with. Different game genres need different spatial representations.
- **Rejection Reason**: All three variants already exist and serve distinct use cases. Removing any would reduce the library's genre fitness.

### Alternative 3: Integrate AI into ECS at the library level

- **Pros**: AI components on entities; batch-tick via ECS Systems; data-oriented.
- **Cons**: Forces an ECS dependency on all AI users; simple non-ECS games (DXLib-style Compat users) would need to adopt ECS just to use a behavior tree. Violates the ADR-0017 L1 principle (beginner-usable without advanced systems).
- **Rejection Reason**: Application-level integration is the correct approach. Game code writes an ECS System that ticks BehaviorTrees and calls NavAgent::Update — the library provides the building blocks, not the opinionated glue. A future ADR may provide optional ECS-AI bridge components, but the core AI module stays independent.

## Consequences

### Positive

- Zero coupling to ECS/Physics/JobSystem — AI code is testable in isolation and usable by the simplest Compat-layer games.
- Three NavMesh variants cover tile-based, voxel-based, and mesh-based game worlds without forcing one representation.
- BehaviorTree + GOAP gives both reactive (BT) and deliberative (GOAP) decision-making models.
- Stateless RVO function is trivially parallelisable — caller batches agents and calls from multiple Jobs.
- Comprehensive test coverage (6 test files, 100+ cases) provides confidence in the algorithms.

### Negative

- Synchronous-only means large-crowd AI (1000+ agents) can exceed per-frame budget without explicit JobSystem integration by the caller.
- NavMesh dynamic obstacles use a full-restore-from-baseline pattern on removal — O(grid cells) cost, not O(obstacle area). Acceptable for small obstacle counts but visible with hundreds of dynamic obstacles.
- GOAP's bool-only WorldState limits expressiveness (no integer/float world-state variables). Callers work around this with multiple bool keys ("hasAmmo_low", "hasAmmo_high") which is verbose.
- RVO's XZ-only projection doesn't handle multi-floor buildings natively — caller must filter agents by floor.
- PolyNavMesh has no built-in navmesh generation from 3D geometry (unlike Recast) — caller provides pre-authored triangle data.

### Risks

- **BT stack overflow on deep trees**: `BTNode::Tick` recurses. Very deep trees (depth > 100) could blow the stack. *Mitigation*: typical game BTs are 5-15 deep; documented as a known limit.
- **GOAP search explosion**: `MakePlan` is A* with branching factor = action count. With 50+ actions and complex precondition chains, search can hit `MaxIterations`. *Mitigation*: default cap at 1000; `PlannerStats` exposes `nodesExplored` for debugging.
- **NavMesh build cost on main thread**: `BuildFromGeometry` rasterises triangles — O(triangles × cells) for grid, O(triangles × voxels) for 3D. *Mitigation*: documented as "run at load time or on a background Job"; debug log warns if build exceeds 100 ms.
- **RVO oscillation under dense crowds**: simplified half-responsibility model can oscillate when agents are packed tighter than their radius sum. *Mitigation*: acceptable for ≤100 agents; full ORCA LP solver is a future upgrade path for crowd-sim-heavy games.
- **Blackboard HashMap nondeterminism**: `Blackboard::GetAll()` iterates a HashMap — nondeterministic order. *Mitigation*: `blackboard_iteration_in_rollback` forbidden pattern; key-by-key access is deterministic.

## GDD Requirements Addressed

| GDD System | Requirement | How This ADR Addresses It |
|------------|-------------|--------------------------|
| (None — ADR-only project per ADR-0001) | N/A | Retroactive codification of Phase 1-2 AI subsystem. Registers TR-ai-001 through TR-ai-007 for the individual AI capabilities. |

## Performance Implications

- **CPU**: `BehaviorTree::Tick` ≤ 0.01 ms per tree for typical 10-node depth. `NavMesh::FindPath` ≤ 0.1 ms for 200×200 grid with smooth. `NavMesh3D::FindPath` ≤ 0.5 ms for 100³ voxel grid (worst case). `PolyNavMesh::FindPath` ≤ 0.05 ms for 1000-triangle mesh. `GOAPPlanner::MakePlan` ≤ 0.1 ms for 20 actions, ≤ 1 ms near iteration cap. `RVO::ComputeAvoidanceVelocity` ≤ 0.002 ms per agent with 50 neighbours.
- **Memory**: `NavMesh` grid ~8 bytes per cell (200×200 = 320 KB). `NavMesh3D` ~2 bytes per voxel + costs (100³ = 2 MB). `PolyNavMesh` ~80 bytes per triangle (1000 tris = 80 KB). `BehaviorTree` ~200 bytes per node. `Blackboard` grows with entries (~64 bytes each). `GOAPPlanner` ~48 bytes per action.
- **Load Time**: NavMesh builds dominate — `BuildFromGeometry` is O(triangles × cells). For a 200×200 grid and 10k triangles, expect ~50-200 ms. Should run at level load, not per-frame.
- **Network**: N/A for this ADR. AI state replication for multiplayer is a future concern (replicate BT Blackboard keys or GOAP WorldState as part of entity state).

## Migration Plan

Not applicable — retroactive ADR. Going forward:

1. **ECS-AI bridge components** (future ADR): optional `BTComponent` / `NavAgentComponent` that wrap the standalone classes and register as ECS Systems ticked by the JobSystem.
2. **Full ORCA LP solver** (future): upgrade RVO from simplified half-responsibility to full linear-programming ORCA for 1000+ agent crowds.
3. **Recast-style navmesh generation** (future): automated triangle-mesh NavMesh generation from 3D scene geometry, replacing the current manual `Build` from pre-authored data.
4. **Integer/float WorldState for GOAP** (future): extend WorldState from bool-only to `variant<bool, int, float>` for richer planning domains.

## Validation Criteria

- **Existing tests pass**: All 6 test files (test_AIBehaviorTree, test_AIExtended, test_NavMesh, test_NavMesh3D, test_PolyNavMesh, test_GOAPPlanner) — 100+ test cases.
- **BT resumption**: BTSequence with a Running child resumes at that child on next Tick, not from the beginning.
- **GOAP optimality**: planner finds lowest-cost plan when multiple valid plans exist (test_GOAPPlanner covers this).
- **NavMesh obstacle lifecycle**: Add obstacle → path avoids it → Remove obstacle → path goes through original area.
- **NavMesh3D state filter**: path through Water+Open voxels succeeds; path through Air-only fails when Air is not in allowed mask.
- **PolyNavMesh funnel**: smoothed path length ≤ A*-centroid path length (funnel never makes paths longer).
- **RVO no-oscillation**: 50 agents moving toward each other's positions converge without velocity sign-flipping over 100 frames.
- **NavAgent reach**: agent with `speed=3.5` reaches a destination 35 units away within 10±1 seconds.
- **Thread-safety**: two NavAgents on two separate Jobs with separate NavMesh instances — no data race (TSan-clean).

## Related Decisions

- ADR-0001 (Documentation strategy)
- ADR-0004 (ECS — AI-ECS integration is application-level; BT/NavAgent are standalone)
- ADR-0006 (Job System — AI ticks can be submitted as Jobs by the caller; AI module itself is synchronous)
- ADR-0009 (Physics — NavMesh::BuildFromGeometry accepts raw vertex data that the caller extracts from physics meshes)
- ADR-0013 (Networking — Blackboard/WorldState replication for multiplayer AI is a future concern; `blackboard_iteration_in_rollback` forbidden pattern)
- ADR-0016 (EventBus — AI actions may fire events on the global bus; categorisation rules apply)
- ADR-0017 (Two-Layer Accessibility Pillar — AI is L1.5: not in the DXLib-compatible Compat layer, but usable without ECS)
- `GXLib/AI/{BehaviorTree,GOAPPlanner,NavMesh,NavMesh3D,PolyNavMesh,NavAgent,RVOSolver}.{h,cpp}`
- `Tests/test_AI*.cpp`, `Tests/test_NavMesh*.cpp`, `Tests/test_PolyNavMesh.cpp`, `Tests/test_GOAPPlanner.cpp`
- CHANGELOG.md Phases 1, 2
