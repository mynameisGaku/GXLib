# Architecture Review — 2026-04-16 (run 2, post-ADR-0011/12/13)

> **Independence caveat**: third run in the same session as ADR authoring. Treat as self-consistency check, not independent review. Run again in a fresh session before promoting any ADR `Proposed → Accepted`.

**Engine**: GXLib (self-hosted, Phase 5, pinned 2026-03-01)
**ADRs reviewed**: 13 (all `Proposed`)
**GDDs reviewed**: 0 (ADR-only per ADR-0001)
**Verdict**: **CONCERNS** — coverage 94%; 2 retroactive charter gaps remain (Editor, Animation). No blocking conflicts.

---

## Traceability Summary

- Total requirements: 35 (22 ADR-derived registered TRs + 10 charter-level + 3 newly-elevated charter)
- ✅ Covered: 33 (94%)
- ⚠️ Partial: 0
- ❌ Gaps: 2 (Editor, Animation — both retroactive, code already shipped)

### Charter-Level Coverage

| TR | Domain | ADR | Status |
|---|---|---|---|
| TR-chr-001 PostFX out-of-the-box | Rendering | ADR-0008 | ✅ |
| TR-chr-002 Audio / Music / DSP | Audio | ADR-0010 | ✅ |
| TR-chr-003 Physics + cloth + ragdoll | Physics | ADR-0009 | ✅ |
| TR-chr-004 Asset pipeline / Hot reload / DirectStorage | Core/IO | ADR-0007 | ✅ |
| TR-chr-005 Networking (Reliable UDP, replication, lag-comp, rollback) | Networking | ADR-0013 | ✅ (new this run) |
| TR-chr-006 Job System | Core | ADR-0006 | ✅ |
| TR-chr-007 Input (K/M + Gamepad + IME) | Input | ADR-0011 | ✅ (new this run) |
| TR-chr-008 GUI + ImGui integration | UI | ADR-0012 | ✅ (new this run) |
| TR-chr-009 Editor (PIE / Undo / NodeGraph / Reflection) | Editor | — | ❌ GAP |
| TR-chr-010 Animation (skeleton / IK / blend trees / motion matching) | Animation | — | ❌ GAP |

All 22 registered ADR-derived TRs (`doc-001..003`, `rnd-001..005`, `api-001..004`, `ecs-001..005`, `scr-001..005`) remain ✅.

## Cross-ADR Conflict Detection

**No conflicts detected.** Specific cross-checks against the new ADRs (0011/0012/0013):

- **Input ownership symmetry**: ADR-0011 commits to "InputManager surfaces state, doesn't consume" and ADR-0012 commits to a binding priority order (ImGui → UIContext → game) — these are complementary, not contradictory.
- **Networking determinism vs Physics fixed timestep**: ADR-0013 RollbackNetcode explicitly relies on ADR-0009 `variable_timestep_in_physics_solver` forbidden pattern + deterministic-island-ordering. Aligned by design.
- **Replication payload contracts**: ADR-0013 ReplicatedProperty uses ADR-0004 `EntityHandle` (idx32+gen32) and ADR-0007 `AssetId` (FNV-1a 64) — matches existing forbidden patterns (`raw_physics_body_pointer_in_ecs`) and adds the new `raw_pointer_in_replication_payload` pattern.
- **Hot reload across all asset-backed surfaces**: AssetReloader (ADR-0007) drives texture/shader/material/sound/Lua/font/CSS/XML reloads — single pipeline, no parallel reload paths added by 0011/0012/0013.
- **Threading model**: all new ADRs commit to main-thread-only public API (Input, GUI mutation, NetworkManager calls) with worker-thread serialization/dispatch via JobSystem. Consistent with ADR-0006 main-thread-submit rule.

### Frame Budget (revised — per technical-director independent review)

The earlier flat sum incorrectly added main-thread, worker-parallel, and audio-thread costs as if they were all serial on the main thread. Corrected breakdown:

**Main-thread serial work (the actual 16.6 ms gate):**

| Subsystem | Budget (ms) | Notes |
|---|---|---|
| Input Update | ≤ 0.05 | InputManager::Update on main |
| Audio main-thread API | ≤ 0.3 | AudioManager calls (no mix) |
| Physics: command-buffer flush + island merge + ECS mirror | ≤ 0.7 | post-Step main-thread serial portion |
| Rendering: FrameGraph compile + Present submit | ≤ 0.5 | command-list submit is main-only |
| UI HUD update + dispatch + layout | ≤ 0.5 | UIContext::Update |
| ImGui pass (when editor active) | ≤ 1.0 | Begin/End + draw data |
| Game logic + Lua tick | ≤ 4.0 | nominal allocation |
| **Main-thread subtotal** | **≤ ~7.0** | |

**Worker-thread parallel work (wall-clock, not added to main):**
- Physics broadphase + narrow-phase + parallel island solve: ≤ 2.5 ms wall
- Rendering parallel command-list recording (4 workers): ≤ 1.5 ms wall
- Networking replication tick + serialization: ≤ 0.5 ms wall
- AudioOcclusion raycast batches: ≤ 0.2 ms wall (concurrent with Step under physics_broadphase_query contract)
- AssetDatabase async loads / shader recompiles: variable, off the critical path

**XAudio2-owned thread (separate OS thread, not on main, contends for memory bandwidth only):**
- Audio mix + DSP: ≤ 1.0 ms

**Frame critical path** = max(main-thread serial, max worker wall) + sync barriers ≈ 7-9 ms. Comfortably within 16.6 ms target. The ~7 ms main-thread headroom is the load-bearing number, not the prior subtotal.

## ADR Dependency Order (topologically sorted, 13 ADRs)

**Foundation**
1. ADR-0001 — Documentation strategy
2. ADR-0002 — DX12 backend choice (deps: 0001)

**Core**
3. ADR-0006 — Job System (deps: 0001, 0002)
4. ADR-0007 — Asset Database + Hot Reload (deps: 0001, 0002, 0006)
5. ADR-0004 — Archetype-based ECS (deps: 0001)

**Systems**
6. ADR-0008 — Rendering Pipeline (deps: 0001, 0002, 0006, 0007)
7. ADR-0009 — Physics Architecture (deps: 0001, 0004, 0006)
8. ADR-0010 — Audio Architecture (deps: 0001, 0006, 0007, 0009)
9. ADR-0011 — Input Architecture (deps: 0001, 0002, 0003)

**Surfaces / Bindings**
10. ADR-0012 — GUI Architecture (deps: 0001, 0002, 0007, 0008, 0011)
11. ADR-0003 — DXLib Compat API layer (deps: 0001, 0002)
12. ADR-0005 — Lua Scripting boundary (deps: 0001, 0004, 0006, 0007)

**Networking**
13. ADR-0013 — Networking Architecture (deps: 0001, 0004, 0006, 0007, 0009)

All `Depends On` references resolve. **No dependency cycles.**

⚠️ **All 13 ADRs are still `Proposed`** — per `docs/CLAUDE.md`, stories cannot reference them until `Accepted`. Promotion is the next gate after independent re-review.

## Engine Compatibility

- 13/13 ADRs include the Engine Compatibility section ✅
- Engine version (GXLib Phase 5, 2026-03-01) referenced consistently ✅
- No deprecated API references
- No post-cutoff API conflicts — all decisions sit on stable training-data surfaces (DX12, XAudio2, C++20, Lua 5.4 + sol2, Winsock2, Schannel, IMM32, XInput, OGG/Vorbis, GJK/EPA, Verlet, Chase-Lev, FNV-1a, Freeverb)

### Engine Specialist Consultation
Skipped — same-session run; specialist re-validation should run in a fresh session prior to ADR Accept promotion.

## GDD Revision Flags
N/A — ADR-only project per ADR-0001.

## Architecture Document Coverage
`docs/architecture/architecture.md` not present (intentional per ADR-0001 ADR-only strategy). No orphans.

---

## Verdict: CONCERNS

13 ADRs cover Foundation → Core → Systems → Surfaces → Networking with no detected internal conflicts. Cumulative frame budget within 60 fps target with ~7 ms headroom. Two charter gaps remain (Editor low priority, Animation medium priority); both are retroactive — code already shipped Phases 0–5.

### Blocking Issues
None. Verdict is CONCERNS only because (a) two retroactive charter ADRs remain unwritten and (b) all ADRs are still `Proposed`.

### Required ADRs (priority order)
1. **ADR-0014 Animation Pipeline** — Skeleton, AnimationLayer, BlendTree, IK suite (FullBody/Foot/LookAt), MotionMatching, SpringBone, RootMotionLocomotion, ProceduralAnimation, BlendStack. Closes TR-chr-010.
2. **ADR-0015 Editor / Play-in-Editor** — PIE simulation freeze/resume, Undo/Redo, NodeGraph editor, Reflection-driven PropertyInspector, GXModelViewer panel set. Closes TR-chr-009.

After both are written, re-run `/architecture-review` in a fresh session and promote ADRs `Proposed → Accepted` (this is an SDK-without-stories project, so `/gate-check pre-production` is informational rather than blocking).

---

## Related
- `docs/architecture/adr-0001-documentation-strategy.md` … `adr-0013-networking-architecture.md`
- `docs/architecture/tr-registry.yaml` (22 registered TRs)
- `docs/registry/architecture.yaml` (4 state_ownership, 8+ interfaces, 18 api_decisions, 9 perf_budgets, 24 forbidden_patterns)
- `docs/architecture/architecture-traceability.md`
- Prior reviews: `architecture-review-2026-04-15.md` (5 ADRs, 69%), `architecture-review-2026-04-16.md` (10 ADRs, 85%)
