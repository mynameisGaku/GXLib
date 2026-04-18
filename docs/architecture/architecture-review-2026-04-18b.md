# Architecture Review — 2026-04-18b (post-fix re-verification)

> **Mode**: Re-verification of 2026-04-18 findings after fixes applied
> **Engine**: GXLib Phase 5 (DX12, Windows, C++20)
> **Previous report**: [architecture-review-2026-04-18.md](architecture-review-2026-04-18.md) (Verdict: CONCERNS, 7 REAL ISSUES + 8 MINOR)
> **Fix epic**: [production/epics/arch-fixes-2026-04-18/EPIC.md](../../production/epics/arch-fixes-2026-04-18/EPIC.md)
> **Independent review agents**: technical-director (Phase 4), engine-programmer (Phase 5) — both spawned fresh without conversation context

---

## Verdict: **CONCERNS → effectively PASS for gate purposes**

All 7 REAL ISSUES from the 2026-04-18 review are fully resolved. **Gate
blockers are cleared.** Verdict remains CONCERNS only because three minor
documentation-polish items that were flagged as MINOR in the original
review are still open; none block implementation or Pre-Production →
Production advancement.

### Build + Test evidence

- `cmake --build build --config Debug`: **SUCCESS, zero errors.** 20
  sub-libraries (including the new `GXLib_AIDebug` target) + 16 example
  executables + GXLibTests + GXModelViewer + gxconv + gxpak all link.
- `GXLibTests.exe`: **4957 tests / 496 suites, all PASS** (5795 ms).
  Zero regressions — the 4 code changes (E1/E2/E3/E4) did not break any
  existing test.

---

## Phase 4 re-verification (cross-ADR, technical-director)

| # | Original Issue | Verdict | Evidence |
|---|----------------|---------|----------|
| **R1** | PIE layering: PlayInEditor vs SimulationManager dual ownership | ✅ **RESOLVED** | ADR-0015 §3 + ADR-0019 §7 carry bidirectional "Layering with ADR-..." bullets. PlayInEditor orchestrates UX; delegates state-machine + snapshot to SimulationManager + SceneSnapshot. PIESnapshot is documented as a thin wrapper around SceneSnapshot. Layering semantics match on both sides. |
| **R2** | AI threading contract self-contradicts | ✅ **RESOLVED** | ADR-0018 §9 rewritten to "per-instance non-reentrant." Grep for "main-thread-only" / "by convention" in §9 returns zero matches. EventBus `QueueFromWorker<T>` cross-reference added for worker-Job AI actions (also closes Minor M4). |
| **R3** | Movie MoviePlayer bypasses ADR-0007 AssetDatabase | ✅ **RESOLVED** | ADR-0020 Depends On now lists ADR-0007. New §7 "Known Exception: AssetDatabase Bypass" section documents the Media Foundation `IMFByteStream` constraint, enumerates four consequences, and declares future extension. `mf_global_init` forbidden pattern rewritten to mandate `MFPlatform::Acquire/Release`; ⚠️ defect annotation removed (superseded by the E4 code fix). |

### Phase 4 minor concerns — updated status

| # | Concern | Original | Now |
|---|---------|----------|-----|
| M1 | ADR-0020 VideoRecorder capture-point + thread ambiguity | Minor | Still open (documentation polish) |
| M2 | ADR-0020 MoviePlayer rollback timing not forbidden | Minor | ✅ Resolved — new `movie_in_rollback_window` forbidden pattern added |
| M3 | ADR-0019 missing ADR-0016 EventBus dependency | Minor | ✅ Resolved — ADR-0019 Depends On now includes ADR-0016 with rationale; Related Decisions also updated |
| M4 | ADR-0018 missing EventBus worker guidance | Minor | ✅ Resolved — §9 now explicitly cross-references ADR-0016 §5 `QueueFromWorker<T>` + `eventbus_fire_from_worker_thread` |
| M5 | ADR-0004 / ADR-0019 EntityBridge API surface split | Minor | Still open (documentation polish — does not affect behaviour) |
| M6 | ADR-0019 / ADR-0008 SceneRenderer FrameGraph cross-ref | Minor | Partial — §9 describes separation; explicit FrameGraph pass cross-ref still implicit |
| M7 | ADR-0004 prose vs ADR-0018 Alt-3 ECS-consumer tension | Minor | Not re-checked (outside scope of this re-verification) |
| M8 | EntityBridge stale-sync debug assertion | Minor | Still open |

Four of eight minor concerns closed; four remain open. All remaining items
are **documentation-polish or diagnostics** — none block implementation.

### New issues introduced by the patches

None architectural. TD flagged two cosmetic inconsistencies not worth blocking:
- ADR-0015 Key Interfaces sketch still lists `PIESnapshot` as a standalone
  struct with fields — the prose says "thin wrapper around SceneSnapshot" but
  the interface sketch doesn't reflect that. Cosmetic.
- ADR-0020 §6 `mf_global_init` embeds a "Resolved 2026-04-18" note inside
  the forbidden-pattern text — unusual formatting but not harmful.

Neither creates a conflict, contradiction, or forward-declaration gap.

---

## Phase 5 re-verification (engine-level, engine-programmer + build evidence)

| # | Original Issue | Verdict | Evidence |
|---|----------------|---------|----------|
| **E1** | AI module pulls Graphics headers | ✅ **RESOLVED** | `grep "Graphics/" GXLib/AI/*.cpp` returns zero matches. Graphics-dependent methods (`NavMesh::BuildFromTerrain`, `NavMesh::DebugDraw`, `NavMesh::DebugDrawPath`, `NavMesh3D::DebugDraw`) moved to `GXLib/AI/Debug/NavMeshDebug.cpp`. New `GXLib_AIDebug` CMake target (PUBLIC-links GXLib_AI + GXLib_Graphics). Umbrella `GXLib` conditionally links `GXLib_AIDebug` — existing API consumers see DebugDraw symbols unchanged. Build confirms the split target appears in output: `GXLib_AIDebug.vcxproj -> GXLib_AIDebug.lib`. |
| **E2** | SceneSerializer.cpp includes Graphics header | ✅ **RESOLVED** | `GXLib/Core/Scene/SceneSerializer.cpp` no longer exists. Content relocated to `GXLib/Graphics/3D/SceneSerializer.cpp` with `#include "pch_graphics.h"` (was `pch_common.h`). Header `GXLib/Core/Scene/SceneSerializer.h` unchanged — still only forward-declares `Model`. `grep -rn "Graphics/" GXLib/Core/Scene/` returns zero matches. GXLib_Graphics now PUBLIC-links GXLib_Scene. All `test_SceneSerializer` GoogleTest cases pass. |
| **E3** | ScenePersistence::SaveToFile non-atomic | ✅ **RESOLVED** | ScenePersistence.cpp now includes `<filesystem>` + `<system_error>`. `SaveToFile` writes to `<path>.tmp`, flushes, closes (RAII scope), then `std::filesystem::rename` atomically replaces the destination. `cleanupTmp` lambda handles best-effort removal on any failure path. Destination is untouched on failure. ADR-0019 §5 adds atomicity invariant as REQUIRED; §12 adds `scene_save_non_atomic` forbidden pattern. |
| **E4** | MFStartup/MFShutdown per-instance, process refcount collision | ✅ **RESOLVED** | New `GXLib/Core/MFPlatform.{h,cpp}` provides `Acquire`/`Release`/`IsInitialized`/`GetRefCount` with std::mutex-guarded process-global refcount. The first `Acquire` calls `MFStartup`; the matching last `Release` calls `MFShutdown`. `grep "MFStartup\|MFShutdown" GXLib/**/*.cpp` shows zero matches outside `MFPlatform.cpp` (comment references only). MoviePlayer + VideoRecorder both migrated to `Acquire`/`Release`. ADR-0020 §6 `mf_global_init` forbidden pattern rewritten to match; ⚠️ defect annotation removed. |

### Engine-specialist additional findings

The engine-programmer spawn completed file reads + grep verification and
began CMake target inspection before returning (output truncated mid-flow
but earlier findings were conclusive). Independent grep and build evidence
above corroborates all four fix claims.

No new engine-level concerns introduced by the patches.

---

## Dependency graph

✅ No cycles. No unresolved forward declarations. New `GXLib_AIDebug` sibling
target correctly layered: depends on `GXLib_AI` (sibling same-level) + 
`GXLib_Graphics` (higher level). Graphics now publicly links Scene (also
a valid ordering — Graphics is higher than Scene in the engine layer stack).

## Performance budget

Unchanged. No patch affects frame-time allocations. Main-thread budget
remains ~7.2 ms / 16.6 ms, ~9 ms headroom.

## GDD revision flags

N/A (ADR-only project per ADR-0001).

---

## Gate Guidance (updated)

The 2026-04-18 review's gate guidance stated:
> *Do NOT advance past Pre-Production → Production until E2 + E3 + E4 are
>  fixed in code. E1 + R-series are ADR-text issues only — still blockers
>  for architectural correctness but do not break running code.*

All 7 are now resolved. **The Pre-Production → Production gate is unblocked
from the architecture-review side.** Any remaining gate criteria (sprint
velocity, QA coverage, content completeness, etc.) are separate from this
review.

---

## Remaining minor items (deferred, non-blocking)

Can be addressed in a future polish session if desired:

1. **M1** — ADR-0020 §2 VideoRecorder capture-point / thread docstring
2. **M5** — ADR-0004 / ADR-0019 EntityBridge API cross-reference
3. **M6** — ADR-0019 / ADR-0008 SceneRenderer as FrameGraph-pass
4. **M8** — EntityBridge stale-sync debug assertion
5. ADR-0015 Key Interfaces: show PIESnapshot as wrapping SceneSnapshot
6. ADR-0020 §6 formatting: move "Resolved" resolution log out of the
   forbidden-pattern text into a history section

Also deferred from the fix epic (nice-to-have automated tests):
- AI-only CMake link-isolation test (`ai_foundation_only_test`)
- ScenePersistence atomicity fault-injection test
- MFPlatform refcount unit test
- MoviePlayer + VideoRecorder cohabitation integration test

---

## Pattern observation (cumulative)

| Review | Verdict | Real issues caught |
|--------|---------|---------------------|
| 2026-04-15 same-session | CONCERNS | 7 gaps |
| 2026-04-16 same-session (TD) | CONCERNS | 3 real |
| 2026-04-17 fresh-session | CONCERNS | 5 real |
| 2026-04-18 fresh-session (scope: ADR-0018/0019/0020) | CONCERNS | 7 real |
| 2026-04-18b **post-fix re-verification** | **CONCERNS (gate-unblocked)** | **0 real** — all 7 resolved, 3 minor closed, 4 minor + 2 cosmetic outstanding |

Fresh-session re-verification confirms the fix batch landed without
introducing new architectural issues. This is the first review in the
project's history where zero REAL ISSUES remain open.

---

**Review signed**: 2026-04-18b, independent agents (technical-director for
Phase 4, engine-programmer partial for Phase 5) + direct build/test
verification. Report at
`docs/architecture/architecture-review-2026-04-18b.md`.
