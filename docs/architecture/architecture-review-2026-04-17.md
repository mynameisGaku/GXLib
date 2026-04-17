# Architecture Review — 2026-04-17 (fresh-session, post-ADR-0015/0016)

> **Independence note**: This is the first review in a fresh session since ADRs 0001-0014 were promoted to Accepted. ADRs 0015 and 0016 are the two new Proposed ADRs reviewed here. Engine specialist consultation was performed (engine-programmer agent).

**Engine**: GXLib (self-hosted, Phase 5, pinned 2026-03-01)
**ADRs reviewed**: 17 (15 Accepted + 2 Proposed)
**GDDs reviewed**: 0 (ADR-only per ADR-0001)
**Verdict**: **CONCERNS** — coverage 100% on charter TRs; 2 real issues in ADR-0016 must be resolved before Accepted promotion; 3 subsystem gaps remain (AI, Scene, Movie).

---

## Traceability Summary

- Total registered TRs: 38 (in tr-registry.yaml v4)
- ✅ Covered: 38 (100% of registered requirements)
- ⚠️ Partial: 0
- ❌ Gap: 0

### Charter-Level Coverage (all 10 closed)

| TR | Domain | ADR | Status |
|---|---|---|---|
| TR-chr-001 PostFX out-of-the-box | Rendering | ADR-0008 | ✅ |
| TR-chr-002 Audio / Music / DSP | Audio | ADR-0010 | ✅ |
| TR-chr-003 Physics + cloth + ragdoll | Physics | ADR-0009 | ✅ |
| TR-chr-004 Asset pipeline / Hot reload / DirectStorage | Core/IO | ADR-0007 | ✅ |
| TR-chr-005 Networking (Reliable UDP, replication, rollback) | Networking | ADR-0013 | ✅ |
| TR-chr-006 Job System | Core | ADR-0006 | ✅ |
| TR-chr-007 Input (K/M + Gamepad + IME) | Input | ADR-0011 | ✅ |
| TR-chr-008 GUI + ImGui integration | UI | ADR-0012 | ✅ |
| TR-chr-009 Editor (PIE / Undo / NodeGraph / Reflection) | Editor | ADR-0015 | ✅ (new — Proposed) |
| TR-chr-010 Animation (skeleton / IK / blend / motion matching) | Animation | ADR-0014 | ✅ |

All 38 registered ADR-derived TRs (`doc-001..003`, `rnd-001..005`, `api-001..004`, `ecs-001..005`, `scr-001..005`, `edit-001..007`, `bus-001..006`) are ✅.

### ADR-0013 §13 Forward-Declaration: RESOLVED

ADR-0016 codifies `HandlerCategory { Idempotent, SideEffect }`, `SetReplayMode(bool)`, and the replay-suppression dispatch contract. The interface matches ADR-0013 §13's specification exactly:
- Handler categorisation enum: ✅ match
- Replay mode flag: ✅ match
- Suppression semantics (SideEffect skipped, Idempotent runs): ✅ match
- Producer-side categorisation as the bus contract: ✅ match

The forward-declaration is fully lifted.

### Subsystem Gap Analysis (not charter-level, but notable)

These engine subsystems have shipped code but no dedicated ADR:

| Subsystem | Source | Phase | Priority | Suggested ADR |
|---|---|---|---|---|
| AI (BehaviorTree, NavMesh, RVO) | `GXLib/AI/` | Phases 1-2 | Medium | ADR-0018 AI Architecture |
| Scene (SceneManager, Prefabs, Persistence) | `GXLib/Core/Scene/` | Phases 2, 5 | Medium | ADR-0019 Scene Architecture |
| Movie (Video playback/recording) | `GXLib/Movie/` | Phase 4 | Low | ADR-0020 Movie Pipeline |

These are retroactive gaps — the code works, but there is no architectural contract to review against.

---

## Cross-ADR Conflict Detection

### No blocking conflicts detected.

Specific checks for the new ADRs:

1. **ADR-0016 EventBus × ADR-0013 Networking**: Replay-suppression contract matches § for §. No conflict.
2. **ADR-0016 EventBus × ADR-0014 Animation**: AnimationEventDispatcher bridge in ADR-0016 §7 is consistent with ADR-0014's naming of AnimationEventDispatcher as a per-clip event producer.
3. **ADR-0015 Editor × ADR-0012 GUI**: Focus-steal guard reaffirmed. PIE `captureInput=true` disables panel focus — consistent with ADR-0012 §9.
4. **ADR-0015 Editor × ADR-0016 EventBus**: UndoSystem forbidden during `IsReplayMode()` — both ADRs agree. No conflict.
5. **ADR-0015 Editor × ADR-0005 Lua**: NodeGraph as peer to Lua with same authority rules — no overlap or conflict with the Lua scripting boundary.

### Frame Budget (unchanged from 2026-04-16b review)

Main-thread serial ≤ ~7.0 ms. Worker-parallel ≤ ~3.0 ms wall. Audio thread ≤ ~1.0 ms (separate).
Critical path ≈ 7-9 ms. **Within 16.6 ms target with ~7 ms headroom.**

ADR-0015 adds 0.5-1.5 ms when editor is active, but `GX_EDITOR=OFF` eliminates this for shipping.
ADR-0016 EventBus adds ~0.01 ms per Fire call. Negligible.

No change to the frame budget analysis.

### ADR Dependency Order (topologically sorted, 17 ADRs)

**Foundation**
1. ADR-0001 — Documentation strategy
2. ADR-0002 — DX12 backend (deps: 0001)

**Core**
3. ADR-0006 — Job System (deps: 0001)
4. ADR-0004 — ECS (deps: 0001, 0002)
5. ADR-0007 — Asset Database + Hot Reload (deps: 0001, 0002, 0006)

**Systems**
6. ADR-0008 — Rendering Pipeline (deps: 0001, 0002, 0006, 0007)
7. ADR-0009 — Physics (deps: 0001, 0004, 0006)
8. ADR-0010 — Audio (deps: 0001, 0006, 0007, 0009)
9. ADR-0011 — Input (deps: 0001, 0002, 0003)
10. ADR-0014 — Animation (deps: 0001, 0004, 0006, 0007, 0008, 0009)

**Surfaces / Bindings**
11. ADR-0003 — DXLib Compat (deps: 0001, 0002)
12. ADR-0005 — Lua Scripting (deps: 0001, 0004)
13. ADR-0012 — GUI (deps: 0001, 0002, 0007, 0008, 0011)
14. ADR-0013 — Networking (deps: 0001, 0004, 0006, 0007, 0009)

**Cross-System / Meta**
15. ADR-0016 — EventBus (deps: 0001, 0004, 0009, 0013, 0014) ← **Proposed**
16. ADR-0015 — Editor (deps: 0001, 0004, 0005, 0007, 0008, 0012, 0014) ← **Proposed**
17. ADR-0017 — Two-Layer Accessibility Pillar (deps: 0001, 0003)

All `Depends On` references resolve to existing ADRs. **No dependency cycles.**

⚠️ ADR-0015 and ADR-0016 depend on Accepted ADRs only — no circular Proposed-on-Proposed dependency. They can be promoted independently.

---

## Engine Compatibility

- 17/17 ADRs include the Engine Compatibility section ✅
- Engine version (GXLib Phase 5, 2026-03-01) referenced consistently ✅
- All Knowledge Risk = LOW across all 17 ADRs ✅
- No deprecated API references ✅
- No post-cutoff API conflicts ✅

### Engine Specialist Findings (fresh-session consultation)

The engine-programmer specialist reviewed ADR-0015 and ADR-0016 against the actual source code and found:

#### 🔴 REAL ISSUE 1: ADR-0016 describes nonexistent `QueueFromWorker` infrastructure

ADR-0016 §5 specifies `QueueFromWorker<T>` backed by "per-worker SPSC ring buffers, lock-free." The actual `JobSystem.h` uses a **single shared mutex-protected queue** with no per-worker lanes and no work-stealing. The SPSC ring buffer does not exist in the codebase.

**Impact**: Ratifying the ADR as written would create a false architectural record. Future implementers may assume the SPSC infrastructure exists.

**Required action**: ADR-0016 §5 must be corrected to either:
- (A) Describe the actual threading model (shared mutex queue + `SubmitMainThread` for cross-thread EventBus dispatch), or
- (B) Mark `QueueFromWorker<T>` as a **proposed new API** that must be implemented before ADR-0016 can be Accepted, with a simpler interim approach documented.

#### 🔴 REAL ISSUE 2: Fire<T> handler vector copy allocates on every call

`EventBus::Fire<T>` (EventBus.h lines 80-84) copies the entire handler vector before iterating — a heap allocation per `Fire` call per event type. ADR-0016 mentions the copy for re-entrancy safety but omits the allocation cost.

**Impact**: For event types fired 100+ times per frame, this is measurable. The ADR's performance section claims "≤ 0.01 ms per Fire" but does not account for allocator pressure.

**Required action**: ADR-0016 Performance section must acknowledge the cost and either:
- (A) Accept it as a trade-off (with a note on when to switch to a generation-counter pattern), or
- (B) Specify a migration path (snapshot pointers instead of full copy, or deferred-erase pattern).

#### ⚠️ MINOR CONCERN 1: `std::type_index` not stable across DLL boundaries

`EventBus` and `TypeRegistry` both use `std::type_index` / `typeid(T)`. On MSVC, `type_info` identity is per-module. If GXLib is ever packaged as a DLL, cross-module EventBus dispatch and Reflection lookup will silently fail.

**Action**: Both ADRs should document: "type_index-based dispatch requires all participants to link against the same module. Migration to string-keyed or hash-keyed dispatch would be required before DLL packaging."

#### ⚠️ MINOR CONCERN 2: Reflection registrar DLL boundary risk

`GX_REFLECT_END` uses an anonymous-namespace static registrar. Safe for static libs; would cause multiple registrations if the same type macro expansion appears in multiple DLLs.

**Action**: ADR-0015 should note: "Anonymous-namespace registrar pattern is static-lib-only. DLL packaging would require a central registration init function."

#### ⚠️ MINOR CONCERN 3: PIE rotation Euler round-trip

`PIEEntityState` stores rotation as three Euler floats. If `Transform` internally uses a Quaternion and `GetRotation()`/`SetRotation()` converts via Euler intermediates, round-trip precision loss is possible for complex rotations (gimbal lock region).

**Action**: ADR-0015 should clarify the Transform rotation storage contract or note the known limitation.

---

## GDD Revision Flags

N/A — ADR-only project per ADR-0001.

## Architecture Document Coverage

`docs/architecture/architecture.md` not present (intentional per ADR-0001). No orphans.

---

## Verdict: **CONCERNS**

### Blocking Issues (must resolve before promoting ADR-0015/0016 to Accepted)

1. **ADR-0016 §5 QueueFromWorker**: The described SPSC per-worker infrastructure does not exist. The section must be corrected to match reality or marked as a proposed-new-API with an interim approach.
2. **ADR-0016 Performance**: Fire<T> allocation cost must be acknowledged and either accepted as trade-off or mitigated.

### Non-Blocking Issues (should be addressed but do not block promotion)

3. **ADR-0015 + ADR-0016**: Add DLL-boundary limitation notes for type_index and reflection registrar.
4. **ADR-0015**: Clarify PIE rotation storage contract (Euler vs Quaternion round-trip).
5. **Subsystem gaps**: ADR-0018 AI, ADR-0019 Scene, ADR-0020 Movie — retroactive, non-urgent.

### Recommended ADR Promotion Path

1. Patch ADR-0016 §5 (fix QueueFromWorker description) and Performance section (acknowledge Fire<T> allocation)
2. Optionally patch ADR-0015 minor concerns
3. Re-run `/architecture-review` to confirm PASS
4. Promote ADR-0015 + ADR-0016 from Proposed → Accepted

### Recommended Next ADRs (priority order)

1. **ADR-0018 AI Architecture** (BehaviorTree + NavMesh + RVO) — medium priority, code exists since Phase 1
2. **ADR-0019 Scene Architecture** (SceneManager + Prefabs + Persistence) — medium priority, code exists since Phase 2
3. **ADR-0020 Movie Pipeline** (Video playback + recording) — low priority
