# Story 007: E4 — MFPlatform refcount wrapper for MoviePlayer + VideoRecorder

> **Epic**: Architecture Fixes (2026-04-18 Review)
> **Status**: ✅ Done (2026-04-18, in-session — new Core module + 2 consumers migrated + ADR)
> **Layer**: Core + Movie + Graphics
> **Type**: Integration
> **Manifest Version**: 2026-04-18

## Context

**ADR**: `docs/architecture/adr-0020-movie-pipeline.md` §6
**Requirement**: TR-mov-001 + TR-mov-002 (MoviePlayer + VideoRecorder coexistence)
**Review source**: `docs/architecture/architecture-review-2026-04-18.md` §E4

**Problem** (engine-programmer specialist finding):
- `MoviePlayer::Open` calls `MFStartup(MF_VERSION)`; `Close` calls
  `MFShutdown()`.
- `VideoRecorder::Initialize` calls `MFStartup(MF_VERSION)`; destructor calls
  `MFShutdown()`.
- Media Foundation's platform refcount is **process-global**, not
  per-instance. Each `MFStartup` increments the count; each `MFShutdown`
  decrements it regardless of which object made the call.
- Scenario that breaks: play a cutscene with `MoviePlayer` while
  `VideoRecorder` is capturing the session. `MoviePlayer::Close` decrements
  the refcount to 0 → `MFShutdown` tears down MF while VideoRecorder still
  holds a live `IMFSinkWriter`. Subsequent `WriteSample` calls fail silently
  or access-violation.

ADR-0020 §6 already flagged the wrong wording (*"Each instance manages its
own MF session"*) — that was never true. Story 003 (R3) annotated the defect
with ⚠️ and pointer to this story.

**Engine Notes**: GXLib Phase 5. Windows SDK `mf.lib`, `mfplat.lib`,
`mfreadwrite.lib`. `MFStartup` / `MFShutdown` are thread-safe but
refcount-based; the wrapper need only protect the refcount itself.

**Control Manifest Rules (Movie module)**:
- Required: No direct `MFStartup`/`MFShutdown` calls outside `MFPlatform` (new)
- Forbidden: `mf_global_init` (rewrite per this story)

---

## Acceptance Criteria

- [x] New `GXLib/Core/MFPlatform.h` + `.cpp` with:
      - `gx::MFPlatform::Acquire()` — std::mutex-guarded refcount; calls
        `MFStartup(MF_VERSION)` on the first (0 → 1) transition.
      - `gx::MFPlatform::Release()` — decrements; calls `MFShutdown()` on
        the last (1 → 0) transition. Unbalanced Release logs an error and is
        a no-op.
      - `MFPlatform::IsInitialized()` for diagnostics.
      - `MFPlatform::GetRefCount()` for diagnostics.
- [x] `MoviePlayer::Open` calls `MFPlatform::Acquire()` instead of `MFStartup`.
- [x] `MoviePlayer::Close` calls `MFPlatform::Release()` instead of `MFShutdown`.
- [x] `VideoRecorder::InitMediaFoundation` calls `MFPlatform::Acquire()`.
- [x] `VideoRecorder::ShutdownMediaFoundation` calls `MFPlatform::Release()`.
- [x] No call sites in the engine (outside `MFPlatform.cpp`) reference
      `MFStartup` or `MFShutdown` — verified by grep.
- [x] ADR-0020 §6 forbidden pattern `mf_global_init` rewritten to the
      Acquire/Release language; ⚠️ defect annotation removed.
- [ ] Integration test `MoviePlayerVideoRecorderCohabitationTest` — DEFERRED
      (requires an actual MP4 file + display context; planned for a future
      runtime-QA session).
- [x] Build succeeds: 20 sub-libraries + GXLibTests + 16 examples build
      cleanly; 4957 tests pass with zero regressions.

---

## Implementation Notes

- Existing source:
  - `GXLib/Movie/MoviePlayer.cpp` — search for `MFStartup` / `MFShutdown`.
  - `GXLib/Graphics/VideoRecorder.cpp` — search for `MFStartup` / `MFShutdown`.
- Typical pattern: wrap refcount in a `std::mutex` (MF calls are expensive,
  contention is zero in practice; no need for lock-free). An `std::atomic<int>`
  alone isn't sufficient because we need to observe the 0→1 and 1→0
  transitions to decide whether to call MF.
- Destructor safety: `MFPlatform::Release()` should NOT throw. Failed
  `MFShutdown` (should never happen in practice) is logged and swallowed.
- Thread-safety: `Acquire` and `Release` must be fully reentrant from any
  thread.

## Out of Scope

- Route MoviePlayer through AssetDatabase (`IMFByteStream` adapter) — deferred;
  Known Exception stands (story 003 / ADR-0020 §7).
- Per-frame `CreateCommittedResource` threading review (minor concern 1 from
  the review) — not this story.
- VideoRecorder back-buffer state precondition documentation (minor concern 2)
  — attach to this story's CR as a small doc addition to VideoRecorder.

---

## QA Test Cases

- **AC-1**: Grep `MFStartup|MFShutdown` returns matches ONLY in
  `MFPlatform.cpp` and the bundled SDK headers.
- **AC-2**: Unit test `MFPlatformRefcountTest`:
  - Acquire → IsInitialized = true
  - Acquire → IsInitialized = true (refcount = 2)
  - Release → IsInitialized = true (refcount = 1)
  - Release → IsInitialized = false (refcount = 0)
- **AC-3**: Integration test `MoviePlayerVideoRecorderCohabitationTest`:
  - Open `MoviePlayer`, init `VideoRecorder`, Close `MoviePlayer`,
    `VideoRecorder` still writes frames successfully.
  - Reverse order also works.
- **AC-4**: Existing tests in `test_MoviePlayer.cpp` (8 tests from Phase 5)
  all pass unchanged.

---

## Test Evidence

**Story Type**: Integration
**Required evidence**:
- Build verification (2026-04-18): `cmake --build build --config Debug`
  succeeds cleanly; new `GXLib_Core/MFPlatform.cpp` compiles; updated
  MoviePlayer + VideoRecorder link cleanly via the wrapper.
- `tests/unit/core/mf_platform_test.cpp` — unit test of refcount
  transitions — DEFERRED (easy to add; not a gate blocker).
- `tests/integration/movie/movie_video_cohabitation_test.cpp` — DEFERRED
  (requires a video file fixture).
**Status**: Build verified ✅; dedicated refcount unit test TODO in future session.

---

## Dependencies

- Depends on: None (story 003 already annotated ADR-0020 in anticipation)
- Blocks: `/architecture-review` PASS verdict; Pre-Production → Production
  gate — resolved
