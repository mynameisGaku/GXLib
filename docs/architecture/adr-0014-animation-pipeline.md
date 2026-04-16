# ADR-0014: Animation Pipeline (Skeleton + Animator State Machine + Blend Tree + IK Suite + Motion Matching + GPU Skinning)

## Status
Accepted

## Date
2026-04-16

## Engine Compatibility

| Field | Value |
|-------|-------|
| **Engine** | Custom — GXLib (self-hosted) |
| **Domain** | Animation |
| **Knowledge Risk** | LOW — skeletal animation, blend trees, additive layering, two-bone IK / CCD / FABRIK, motion matching, root motion extraction, Verlet spring bones, and compute-shader skinning are well-documented patterns within LLM training data |
| **References Consulted** | `docs/engine-reference/gxlib/VERSION.md`, `GXLib/Graphics/3D/{Skeleton,Animator,AnimationClip,AnimationLayer,AnimationPlayer,AnimatorStateMachine,BlendTree,BlendStack,AnimationEventDispatcher,ComputeSkinning,FootIK,FullBodyIK,LookAtIK,IKSolver,MotionMatching,RootMotionLocomotion,ProceduralAnimation,SpringBone,Humanoid}.{h,cpp}`, `Shaders/ComputeSkinning.hlsl`, CHANGELOG Phases 0/1/3 |
| **Post-Cutoff APIs Used** | None |
| **Verification Required** | Determinism of blend evaluation across CPUs (FP associativity in additive layer accumulation); GPU skinning correctness against CPU reference at 100+ bones; FABRIK convergence on degenerate IK chains; motion matching pose-search cost under 10k-clip database; **motion matching SIMD-path determinism** (AVX2 vs scalar fallback distance computation must produce identical "winners" inside the rollback window per ADR-0013 §13 — pin one path or quantize); ragdoll blend-out smoothness; root motion drift over long sessions; concurrent-with-physics-broadphase wall-clock at 20+ Animators |

## ADR Dependencies

| Field | Value |
|-------|-------|
| **Depends On** | ADR-0001 (doc), ADR-0004 (ECS — animated entities mirror Transform via EntityBridge), ADR-0006 (Job System — blend-tree evaluation + IK solves submit here), ADR-0007 (Asset DB — Skeleton, AnimationClip, AnimatorController are AssetDB-backed with hot reload), ADR-0008 (Rendering — ComputeSkinning runs as a FrameGraph compute pass; feeds GBuffer), ADR-0009 (Physics — ragdoll handoff swaps Animator-driven pose for PhysicsWorld-driven body pose) |
| **Enables** | Future Editor ADR (timeline editor, animator state-graph editor), future Cinematic ADR, future advanced IK (physics-based active ragdoll) |
| **Blocks** | None (code already exists across Phases 0/1/3; retroactive) |
| **Ordering Note** | Animation evaluates BEFORE physics step in the frame schedule; physics consumes the Animator-produced pose as kinematic target. This ordering is binding for ADR-0009 + ADR-0013 (rollback re-simulation must replay animation in the same order). |

## Context

### Problem Statement
A modern game engine library needs a complete animation stack: skeletal hierarchy, clip playback with looping/blending, layered animations (additive + override masks), state machines for gameplay logic ("Idle" → "Run" → "Jump"), blend trees (1D speed, 2D directional), IK for foot planting / look-at / two-handed weapon hold, motion matching for high-fidelity locomotion, root motion extraction for capsule controller drive, secondary motion (spring bones for hair / cloth accessories / antennae), procedural overlays (head turn, lean), and GPU skinning for performance at scale. GXLib built this incrementally across Phases 0/1/3. This ADR codifies the topology, the per-frame schedule (where animation sits relative to physics, ECS, rendering), the determinism contract for rollback compatibility, and the integration boundaries with Asset DB, Job System, ECS, and Rendering — so future ADRs (cinematic, advanced IK, editor timeline) extend a stable spine.

### Constraints
- Windows-only (per ADR-0002)
- Must coexist with ECS (ADR-0004) — animated entities have a Transform component on the ECS side; the animator owns the local skeleton and writes the world Transform once per frame via EntityBridge
- Skeleton hierarchy is NOT an ECS component (ECS components are POD per ADR-0004 forbidden pattern `virtual_or_inherited_ecs_component`); it lives in the Animation module and is referenced by handle from ECS
- Must respect ADR-0009 fixed-timestep determinism rule (no wall-clock dt in animation tick during the rollback window — use the same fixed step + interpolation factor)
- Animation must not allocate on the per-frame hot path (no `new` / `delete` inside `Animator::Update`)
- Hot reload of clips and animator controllers via AssetDatabase (ADR-0007)
- Job System integration (ADR-0006) for parallel blend-tree / IK / motion-matching evaluation; no animation-owned thread pool
- GPU skinning must respect ADR-0008's FrameGraph (compute pass declares Read on bone-matrix buffer + Write on skinned-vertex buffer; barriers automatic)

### Requirements
- **Skeleton**: hierarchical bone tree; bind pose; humanoid preset (15-bone biped per ADR-0009 ragdoll); custom bone chains
- **AnimationClip**: keyframed bone tracks (translation/rotation/scale); supports root motion track separation
- **AnimationPlayer**: low-level clip playback (play/pause/seek/loop/rate); used standalone or by higher layers
- **AnimationLayer**: per-layer mask + blend mode (Override / Additive / BlendIn); arbitrary count
- **BlendTree**: 1D (speed parameter), 2D (direction × speed), Direct (per-clip weights); evaluated per frame
- **BlendStack**: ordered crossfade stack (legacy clip transition support)
- **AnimatorStateMachine**: states + transitions + transition conditions + state-enter/exit callbacks
- **Animator**: top-level facade — owns AnimatorStateMachine + AnimationLayer[] + parameter bag (float/int/bool/trigger)
- **AnimationEventDispatcher**: per-clip event tracks fire callbacks at exact frame; routes through EventBus
- **IK suite**: `FootIK` (per-foot raycast + pole vector), `FullBodyIK` (constraint-based, multi-effector), `LookAtIK` (head/eye chain with joint limits), `IKSolver` (CCD / FABRIK toggleable)
- **MotionMatching**: pose-feature database; per-frame nearest-neighbor search; smooth blend to selected clip
- **RootMotionLocomotion**: extract translation/rotation from clip's root track; drives CharacterController (ADR-0009 §10) movement
- **SpringBone**: secondary motion (hair, cloth, accessories); Verlet integration with bone-attached anchors and configurable stiffness/damping
- **ProceduralAnimation**: runtime overlay system (head turn toward target, body lean by velocity)
- **GPU skinning** (`ComputeSkinning`): bone matrices uploaded per frame; compute shader transforms vertices into skinned buffer consumed by rendering passes
- **Humanoid retargeting**: 15-bone humanoid bind allows clips authored on one humanoid to drive any conforming skeleton

## Decision

**GXLib animation is a layered stack: low-level `AnimationClip` / `AnimationPlayer` for raw playback; mid-level `AnimationLayer` + `BlendTree` + `BlendStack` for composition; top-level `AnimatorStateMachine` + `Animator` for gameplay-driven behaviour. IK (`FootIK`, `FullBodyIK`, `LookAtIK`) runs as a post-pose-evaluation pass. Motion matching is an alternative top-level driver. Root motion extracted from clips drives `CharacterController` (ADR-0009 §10). Spring bones add secondary motion in a final pass. GPU skinning via `ComputeSkinning` runs as a FrameGraph compute pass (ADR-0008) consuming bone matrices and producing skinned vertex buffers. Skeletons live in the Animation module (NOT ECS components, per ADR-0004 POD rule); ECS holds an opaque `AssetHandle&lt;Skeleton&gt;` + Transform mirror via EntityBridge. Animation evaluation is deterministic under fixed timestep — binding for rollback (ADR-0013 §13).**

Concrete rules:

1. **Per-frame schedule (binding for ADR-0009 + ADR-0013).**
   ```
   1. Input + Game Logic (gameplay updates Animator parameters)
   2. Animation tick (fixed dt):
        a. AnimatorStateMachine evaluates transitions
        b. AnimationLayer[] evaluates BlendTrees / BlendStacks → per-bone local pose
        c. AnimationEventDispatcher fires events for crossed timestamps
        d. RootMotionLocomotion extracts root delta → applies to CharacterController
        e. IK pass: FootIK → FullBodyIK → LookAtIK (in this order)
        f. ProceduralAnimation overlays (head turn, lean)
        g. SpringBone Verlet step (secondary motion)
        h. World pose computed; mirrored to ECS Transform via EntityBridge
   3. Physics step (kinematic bodies read Animator-produced poses; ragdoll bodies write back if active)
   4. Rendering (ComputeSkinning compute pass produces skinned vertices; SceneRenderer draws)
   ```
   This order is binding. Rollback re-simulation (ADR-0013 §13) replays animation in this same order.

2. **Skeleton ownership.** `gx::Skeleton` is a Resource (AssetDB-backed per ADR-0007), not an ECS component. It carries the bind-pose bone hierarchy + bone names + humanoid mapping (when applicable). Multiple Animator instances can share a single Skeleton resource; per-instance pose state lives on the Animator.

3. **Animator instance.** `gx::Animator` is a per-character object held by gameplay code (or by an ECS resource component carrying an opaque `AnimatorHandle`). It owns:
   - Pointer to shared `Skeleton` resource
   - Per-instance pose buffer (local + world transforms)
   - `AnimatorStateMachine` (one per Animator)
   - `AnimationLayer[]` (variable count, ordered)
   - Parameter bag (`HashMap<string_view, Variant>`) — float / int / bool / trigger
   - References to active clips via `AssetHandle<AnimationClip>`

4. **AnimationClip** is an asset (per ADR-0007). Stores bone-keyed tracks (translation `Vector3`, rotation `Quaternion`, scale `Vector3`) + root motion track (separate, opt-in). Sampling at time `t` interpolates between keyframes using cubic / linear / step per-track. Hot reload via AssetReloader rebinds at next sample (in-flight playback continues from current time).

5. **AnimationLayer** carries: blend mode (`Override` / `Additive` / `BlendIn`), bone mask (which bones the layer affects), weight `[0, 1]`, source (BlendTree / BlendStack / direct clip). Evaluation is bottom-up: layer 0 establishes the base pose; subsequent layers blend in per their mode + mask.

6. **BlendTree.** 1D (single param drives weights between N clips), 2D (Cartesian — direction × speed), Direct (caller sets per-clip weight). Weights computed per frame from current parameter values; underlying clips sampled and blended per-bone (LERP for translation/scale, NLERP for rotation; avoids SLERP cost in the hot path).

7. **BlendStack.** Ordered crossfade history — push a new clip with fade-in duration; older entries fade out; auto-cleanup when weight reaches zero. Provides legacy "current → next clip" transition semantics for code that doesn't use a state machine.

8. **AnimatorStateMachine.** States hold a source (BlendTree / clip / sub-state-machine) and optional `OnEnter` / `OnExit` callbacks. Transitions hold conditions over the parameter bag (`speed > 5.0f`, `isJumping == true`, trigger `Attack`) and an exit-time / blend duration. One state active per state machine; sub-state-machines are first-class.

9. **AnimationEventDispatcher.** Each AnimationClip carries an event track — `(time, eventName, payload)` tuples. As clip playback time crosses an event time, the dispatcher fires through the EventBus (consumed by Audio for footsteps per ADR-0010, by VFX for hit-flash, by gameplay for damage windows). Events fire at most once per crossing; replay during rollback re-fires deterministically.

10. **IK passes.** Run after layer/blend evaluation, before procedural overlays:
    - **FootIK** — per-foot: raycast down via PhysicsWorld (ADR-0009 `physics_broadphase_query`) for ground contact; two-bone IK (knee pole vector preserved); blends out when both feet are airborne.
    - **FullBodyIK** — multi-effector solver (`IKSolver` strategy: CCD or FABRIK). Used for context-sensitive grabs (climb, ledge), two-handed weapon hold, cinematic poses.
    - **LookAtIK** — chained head/neck/eye lookup toward a target with per-joint cone limits.
    - All IK solves are bounded iterations (default CCD ≤ 10, FABRIK ≤ 5) with deterministic per-frame seed.

11. **MotionMatching.** Optional alternative driver to AnimatorStateMachine. Pose-feature database (foot position + velocity + trajectory) indexed for nearest-neighbour search; per frame, search for the best-matching pose in the database given current trajectory + velocity, then crossfade to that clip's playback head. Search is `O(N)` over feature vectors; a feature-vector database of 10k poses queries in ~0.3 ms with SIMD.

12. **RootMotionLocomotion.** Extracts the root track delta (translation + yaw rotation) from the active clip(s); accumulates and forwards to `CharacterController::MoveAndSlide` (ADR-0009 §10). The Animator never directly moves the entity Transform — `CharacterController` is the authority for position changes (kinematic capsule sweep + slide).

13. **SpringBone.** Per-character chain of Verlet particles attached to skeleton bones. Configurable: stiffness, damping, gravity, collider list. Step runs after all primary animation passes; uses a fixed substep count (default 2) — independent of frame rate to avoid jitter under varying fps.

14. **ProceduralAnimation.** Runtime overlays applied AFTER IK and BEFORE SpringBone. Examples: head turn toward look-target (lerp current to target with max angular velocity), body lean by velocity, breathing offset on torso. Composable; each overlay receives the current pose and returns a modified pose.

15. **GPU skinning (`ComputeSkinning`).** Per Animator, the world bone matrices are uploaded to a GPU buffer once per frame (after step (h) above). A FrameGraph compute pass (ADR-0008) declares `Read` on the bone-matrix buffer + skin-weight buffer + bind-pose vertex buffer, and `Write` on the skinned-vertex buffer. SceneRenderer's GBuffer pass reads the skinned-vertex buffer. Shader: `Shaders/ComputeSkinning.hlsl`. Falls back to CPU skinning when compute shader unavailable (caps-gated per ADR-0002 `unchecked_optional_gpu_feature`).

16. **Humanoid retargeting.** `gx::Humanoid` defines a 15-bone biped skeleton (matches ADR-0009 ragdoll layout). Clips authored against the Humanoid bind can drive any skeleton that conforms — the retargeter maps source bone → destination bone by Humanoid role at clip-load time. Non-humanoid skeletons skip retargeting; clips are 1:1 to bone names.

17. **Determinism (binding for ADR-0013 rollback).**
    - Animation tick uses fixed dt (matches ADR-0009 fixed timestep) — no `Time::DeltaTime()` wall clock in the animation tick scope during rollback window.
    - Blend weight computation uses deterministic operations (no hash-set iteration over active clips; layers iterate in declared order).
    - IK solver iterations are bounded and seeded deterministically (no `std::rand`; uses `gx::Random` if randomness needed).
    - Floating-point fast-math is **disabled** in animation translation units (matches `nondeterministic_reduction_in_rollback_physics_stage` family).
    - Motion matching nearest-neighbour search uses stable tie-breaking (lowest pose-id wins on equal distance).

18. **Job System integration (ADR-0006).**
    - Per-Animator blend evaluation can be parallelised across Animators via `JobSystem::ParallelFor` (each Animator is independent — no shared state mutation during evaluation).
    - IK solves likewise (per-Animator, per-effector independent).
    - Motion matching pose-search is per-Animator and parallelisable.
    - Final ECS Transform mirror runs on the main thread (per ADR-0004 EntityBridge contract).
    - Animation parameter writes from gameplay (`Animator::SetParameter`) happen on the main thread; reads inside Job evaluation see a consistent snapshot taken at frame start.

19. **Hot reload (ADR-0007).**
    - **AnimationClip reload** — `AssetReloader` updates the clip data; in-flight playback resamples at the new keyframes from the current time.
    - **Skeleton reload** — bone count/order changes invalidate dependent Animators; reload handler resets affected Animators to bind pose and re-binds clip tracks by bone name.
    - **AnimatorController reload** — state-machine graph swaps at next state-machine tick; current state preserved if its name still exists, else reset to entry state.

20. **Ragdoll handoff (ADR-0009 ragdoll integration).**
    - `Animator::EnableRagdoll(blendIn)` triggers transition: Animator stops writing pose; PhysicsWorld ragdoll bodies become the source of truth for bone transforms; `RagdollBuilder` (ADR-0009 §9) builds the body chain from the current pose.
    - `Animator::DisableRagdoll(blendOut)` reverses: capture current ragdoll body poses → blend back to Animator-driven pose over `blendOut` seconds.
    - **The handoff is atomic at frame boundary (between animation tick and physics step) of the SAME frame.** Concretely: a gameplay call to `EnableRagdoll()` during game logic of frame N takes effect after frame N's animation tick has produced a final pose; that final pose is the seed pose passed to `RagdollBuilder`, which is then consumed by frame N's physics step (which now drives the ragdoll bodies). There is no one-frame delay. Symmetric for `DisableRagdoll()`: frame N's physics step produces the last ragdoll pose, frame N's blend-back captures it, and frame N+1's animation tick begins blending toward animator-driven motion.

### Architecture Diagram

```
   gameplay code → Animator::SetParameter("Speed", 4.5f), SetTrigger("Jump")
                       │
                       ▼
   gx::Animator  (per character; owns pose + parameter bag)
       │
       ├── Skeleton (shared resource, AssetDB-backed)
       │
       ├── AnimatorStateMachine
       │       └── States{ source = BlendTree | clip | sub-SM }
       │             └── Transitions over parameter bag
       │
       ├── AnimationLayer[]   (each: mask + blend mode + weight)
       │       └── BlendTree | BlendStack | direct clip
       │             └── AnimationClip (AssetDB; keyframed bone tracks)
       │                   └── AnimationEventDispatcher → EventBus
       │
       ├── RootMotionLocomotion  → CharacterController.MoveAndSlide (ADR-0009 §10)
       │
       ├── IK suite (post-pose):
       │       FootIK → FullBodyIK → LookAtIK
       │             └── IKSolver (CCD / FABRIK)
       │             └── PhysicsWorld raycast (ADR-0009 §14 concurrent-read OK)
       │
       ├── ProceduralAnimation overlays (head turn, lean, breathing)
       │
       └── SpringBone Verlet step (secondary motion)
                       │
                       ▼
       World pose → EntityBridge → ECS Transform component (per ADR-0004)
                       │
                       ▼
       Bone matrices → GPU buffer
                       │
                       ▼
       FrameGraph ComputeSkinning pass (ADR-0008)
            Read: bone matrices + skin weights + bind-pose verts
            Write: skinned vertex buffer
                       │
                       ▼
       SceneRenderer GBuffer pass consumes skinned vertices

   Ragdoll handoff:
       Animator.EnableRagdoll() → PhysicsWorld owns bone transforms
       Animator.DisableRagdoll(blendOut) → blend ragdoll pose back to animator-driven

   Alternative driver:
       MotionMatching pose-feature DB → per-frame nearest-neighbour → crossfade

   Per-frame schedule (binding for rollback):
       Input → Animation tick → Physics step → Render
```

### Key Interfaces

- `gx::Skeleton` (asset) — `BoneCount()`, `BoneName(idx)`, `Parent(idx)`, `BindPose(idx)`, `Humanoid* AsHumanoid()`
- `gx::Animator::SetParameter(name, value)`, `SetTrigger(name)`, `Update(dt)`, `GetBoneTransform(idx) → Mat4`
- `gx::Animator::PlayClip(AssetHandle<AnimationClip>, layer, fadeIn)`, `Crossfade(...)`, `Stop(layer)`
- `gx::Animator::EnableRagdoll(float blendIn)`, `DisableRagdoll(float blendOut)`
- `gx::AnimationLayer { mode, mask, weight, source }`
- `gx::BlendTree::Set1DParameter(value)`, `Set2DParameter(x, y)`, `SetDirectWeight(clipIdx, w)`
- `gx::AnimatorStateMachine::AddState(name, source)`, `AddTransition(from, to, condition, blendDuration)`, `GetCurrentState()`
- `gx::AnimationEventDispatcher::OnEvent(string name, function<void(EventPayload)>)`
- `gx::FootIK::SetEnabled(bool)`, `gx::LookAtIK::SetTarget(worldPos)`, `gx::FullBodyIK::AddEffector(boneIdx, target)`
- `gx::IKSolver::SetStrategy(CCD | FABRIK)`, `SetMaxIterations(n)`
- `gx::MotionMatching::LoadDatabase(AssetId)`, `Update(trajectory, velocity)`
- `gx::RootMotionLocomotion::Apply(CharacterController&)`
- `gx::SpringBone::AddChain(rootBoneIdx, params)`, `Step(dt)`
- `gx::ProceduralAnimation::AddOverlay(unique_ptr<IOverlay>)`
- `gx::ComputeSkinning::DispatchSkinning(animator, frameGraph)` — registers the compute pass
- `gx::Humanoid::Map(sourceBone, destBone)` — retargeting binding

## Alternatives Considered

### Alternative 1: Adopt Ozz-animation or ACL as the runtime
- **Description**: Wrap an established animation runtime
- **Pros**: Battle-tested SIMD-optimised sampling/blending; small footprint; ACL has excellent compression
- **Cons**: Adds a 100-200 KB dependency; Ozz's API model conflicts with our Asset DB / EventBus / ECS contract; would require an adapter layer for hot reload; doesn't include state machine, IK suite, motion matching, or spring bones — we'd still build those ourselves
- **Rejection Reason**: In-house stack already exists across Phases 0/1/3 and integrates natively with our subsystems. Ozz's wins (compression, SIMD) are real but don't justify replacing a working stack.

### Alternative 2: Animation as ECS systems (skeleton as components)
- **Description**: Decompose Skeleton into per-bone ECS components; AnimatorStateMachine as an ECS system; blend evaluation iterates archetypes
- **Pros**: Uniform with rest of engine; archetype iteration parallelises naturally
- **Cons**: Skeleton is a hierarchical pointer-rich structure that doesn't fit ADR-0004's POD-component rule (`virtual_or_inherited_ecs_component`); per-bone components explode the ECS query cost (humanoid = 15 entities per character); state machine + blend tree have polymorphic node types that fight ECS's flat-storage model
- **Rejection Reason**: Forces the wrong shape onto data the ECS architecture is explicitly designed against. The handle-from-ECS pattern (per ADR-0009 BodyHandle) is the established escape hatch.

### Alternative 3: Single state-machine driver only (drop motion matching)
- **Description**: Ship without `MotionMatching`; locomotion uses BlendTree + state machine only
- **Pros**: Simpler stack; no pose-feature database to author
- **Cons**: BlendTree locomotion has a quality ceiling (foot sliding, awkward turns); motion matching is the modern standard for character locomotion in AAA games — cutting it limits the genre fit
- **Rejection Reason**: Motion matching is already implemented (Phase 3). Removing it loses production value with no maintenance saving.

### Alternative 4: CPU skinning only (drop ComputeSkinning)
- **Description**: Skin vertices on CPU; skip the compute shader path
- **Pros**: Simpler; no caps-gating
- **Cons**: CPU skinning at 60 fps with 10+ skinned characters at 5000+ verts each saturates a CPU core; GPU compute is the standard solution (Unity / Unreal both default to GPU)
- **Rejection Reason**: Performance-critical for any scene with multiple skinned characters. CPU fallback retained for caps-gated low-end hardware (per ADR-0002 `unchecked_optional_gpu_feature`).

## Consequences

### Positive
- Designer-authored animator graphs + blend trees + state machines map directly to gameplay parameters
- IK suite covers the common cases (foot planting, look-at, two-handed weapon) without per-game implementation
- Motion matching unlocks AAA-quality locomotion when the project invests in a pose database
- Determinism contract makes animation rollback-safe (ADR-0013) — replay produces identical poses
- GPU skinning scales to 50+ skinned characters within frame budget
- Spring bones + procedural overlays solve secondary motion without hand-authored polish work
- Ragdoll handoff is atomic and well-defined — death animations and impact reactions become a one-call API
- Hot reload makes iteration on clip timing immediate

### Negative
- Skeleton-not-being-an-ECS-component is an ergonomic wart for code that wants pure ECS — the handle indirection is a small but real cost
- Motion matching pose database is project-specific authoring work; without investment, MotionMatching is unused dead code
- Determinism rules (no wall-clock dt, no fast-math in animation TUs) constrain animation-adjacent code
- CPU-skinning fallback is a separate code path to maintain
- IK strategy choice (CCD vs FABRIK) is per-IKSolver — inconsistent strategies across a project is possible

### Risks
- **Animation evaluation cost** at 100+ characters can dominate a single core. *Mitigation*: per-Animator parallel evaluation via JobSystem; LOD reduces tick rate for distant characters (future ADR — currently per-game responsibility).
- **GPU skinning compute pass scheduling** must finish before GBuffer pass starts; FrameGraph dependency declaration handles this. *Mitigation*: FrameGraph debug-validation layer (ADR-0008) catches missed declarations.
- **Ragdoll-blend-back jitter** if the captured ragdoll pose is drastically different from the animator pose. *Mitigation*: blend-out duration ≥ 0.3 s recommended; debug visualization shows pose delta during blend.
- **Motion matching latency** when the database is large and SIMD is unavailable. *Mitigation*: caps gate on AVX2; database size warning in the log.
- **Spring bone explosion** under fast motion or low substep count. *Mitigation*: default substep = 2; per-chain override; auto-substep based on velocity is a future improvement.
- **Determinism drift** if a future contributor adds wall-clock-dt code in animation TUs. *Mitigation*: forbidden_pattern entry in registry; review gate on touches under `GXLib/Graphics/3D/Animation*`.
- **Humanoid retarget mismatch** when source/destination skeletons have different bone proportions. *Mitigation*: retargeter normalises by bone length ratio; visual debug overlay shows mismatch warnings.

## GDD Requirements Addressed

| GDD System | Requirement | How This ADR Addresses It |
|------------|-------------|--------------------------|
| (None — ADR-only project per ADR-0001) | N/A | TR-chr-010 ("Animation pipeline: skeleton, IK, blend trees, motion matching, spring bone") — elevated from Gap to Covered |

## Performance Implications

- **CPU**: Per-Animator tick (state machine + 2-layer blend tree + IK + spring bones) ≤ 0.05 ms on mid CPU. **Animation runs concurrent with the physics broadphase + narrow-phase passes on JobSystem workers** — they share no writers (animation writes the per-Animator pose buffer + ECS Transform mirror queue; physics broadphase reads body AABBs, narrow-phase computes contact manifolds; pose mirror to ECS Transform happens on main thread between the parallel section and physics constraint solve). Wall-clock allocation: animation ≤ 1.0 ms wall (≈20 characters at full per-tick cost, more with cheaper LOD'd Animators), running concurrent with the 2.5 ms physics worker-wall window. Frames with > 20 active Animators must use animation LOD (tick-rate scaling for distant characters — per-game responsibility until a future ADR codifies it). Motion matching adds ~0.3 ms per Animator using it; budgeted within the 1.0 ms animation wall.
- **Memory**: ~16 KB per Animator (skeleton-shared); per-instance pose buffers ~4 KB for a humanoid; clip data depends on length and key density (~50-200 KB for typical clips).
- **GPU**: ComputeSkinning ≤ 0.2 ms at 1080p for 50 skinned characters at 5000 verts each (compute dispatch dominated by vertex count, not character count).
- **Load Time**: Clip load + retargeting ~5 ms per clip on first use; cached afterward.
- **Network**: N/A (animation state is derived from gameplay parameters that DO replicate — see ADR-0013).

## Migration Plan

Not applicable — this ADR is retroactive. Animation landed across Phase 0 (Skeleton, AnimationClip, AnimationPlayer, Animator basics), Phase 1 (BlendTree, Humanoid, FootIK, LookAtIK, AnimatorStateMachine), Phase 3 (BlendStack, FullBodyIK, MotionMatching, RootMotionLocomotion, SpringBone, ProceduralAnimation, ComputeSkinning, AnimationEventDispatcher, AnimationLayer). Going forward:

1. New IK strategies extend `IKSolver` via the strategy interface; existing `CCD` / `FABRIK` are stable
2. Animation LOD (tick-rate scaling for distant characters) is a future extension — this ADR makes no commitment
3. Cinematic / timeline editor is a future ADR (ADR-0015 Editor) — will consume this ADR's `AnimationPlayer` and event dispatcher
4. Active ragdoll (physics-driven with target pose tracking) is a future ADR — extends ADR-0009 + this ADR

## Validation Criteria

- **Determinism**: identical inputs (parameter timeline + dt) produce byte-identical bone transforms after 1000 ticks across AMD + Intel CPUs (within 1e-5 tolerance)
- **GPU vs CPU skinning parity**: same character/clip rendered with both paths produces visually identical output (RGB diff ≤ 1/255 per channel)
- **State machine stress**: 100-state graph with 500 transitions evaluates in ≤ 0.02 ms
- **Motion matching latency**: 10k-pose database queries in ≤ 0.5 ms with AVX2 enabled
- **Ragdoll blend smoothness**: enable ragdoll mid-walk, blend out over 0.5 s — no visible pose pop
- **Foot IK contact**: character on uneven terrain — both feet plant on ground within 1 cm tolerance over 60 s of walk cycle
- **Hot reload**: edit a clip's keyframe; in-flight playback transitions to new data within 1 frame; bone-name-stable; no crash
- **Spring bone stability**: long hair chain (10 bones) at 60 fps walk → 10 fps sudden frame drop → no explosion; settles within 1 s

## Related Decisions

- ADR-0001 (Documentation strategy)
- ADR-0004 (ECS — Animator referenced by handle from ECS; Transform mirror via EntityBridge; Skeleton not-an-ECS-component per POD rule)
- ADR-0006 (Job System — per-Animator parallel evaluation; IK solves; motion matching search)
- ADR-0007 (Asset Database — Skeleton, AnimationClip, AnimatorController hot reload)
- ADR-0008 (Rendering — ComputeSkinning is a FrameGraph compute pass; feeds GBuffer)
- ADR-0009 (Physics — ragdoll handoff API; FootIK uses physics_broadphase_query concurrent-read; CharacterController consumes RootMotion)
- ADR-0013 (Networking — animation determinism enables rollback re-simulation; replicated parameters drive Animator state)
- (Future) ADR-0015 Editor — timeline editor + animator state-graph editor consume this ADR
- `GXLib/Graphics/3D/{Skeleton,Animator,AnimationClip,AnimationPlayer,AnimatorStateMachine,AnimationLayer,BlendTree,BlendStack,AnimationEventDispatcher,ComputeSkinning,FootIK,FullBodyIK,LookAtIK,IKSolver,MotionMatching,RootMotionLocomotion,ProceduralAnimation,SpringBone,Humanoid}.{h,cpp}` (source of truth)
- `Shaders/ComputeSkinning.hlsl`
- CHANGELOG.md Phases 0, 1, 3
