# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Audio routing hotfix] - 2026-04-19

Post-sprint-003 hotfix, driven by the sprint-002 manual-QA IXAPO
verification that landed the same day. Three audio-pipeline engine
defects resolved + 1 documented-as-deferred. Sprint-001 and Sprint-002
both reach 100% DoD closure.

### Fixed
- **MusicPlayer SourceVoice bypassed AudioBus**: `CreateSourceVoice`
  now takes a `XAUDIO2_VOICE_SENDS` targeting the BGM bus SubmixVoice
  (via new `MusicPlayer::SetOutputSubmixVoice`). Previously defaulted
  to MasteringVoice direct, skipping every AudioBus effect chain —
  `IAudioEffect` `Process()` was never dispatched regardless of
  `AddEffect` success.
- **AudioBus `OutputChannels` hardcoded to 2**: `RebuildEffectChain`
  now queries `GetVoiceDetails` on the SubmixVoice and passes
  `InputChannels` to `XAUDIO2_EFFECT_DESCRIPTOR::OutputChannels`.
  Previous 2-channel hardcode silently failed `SetEffectChain` on any
  non-stereo device.
- **XAPOBridge registration flags insufficient**: `GetRegistrationProperties`
  set only `XAPO_FLAG_INPLACE_REQUIRED`, making XAudio2 reject the chain
  with `XAUDIO2_E_INVALID_CALL (0x88960001)`. Now uses the Microsoft
  XAPO sample standard set: `XAPO_FLAG_CHANNELS_MUST_MATCH |
  FRAMERATE_MUST_MATCH | BITSPERSAMPLE_MUST_MATCH |
  BUFFERCOUNT_MUST_MATCH | INPLACE_SUPPORTED`.

### Added
- `MusicPlayer::SetOutputSubmixVoice` — routes the SourceVoice to a
  target SubmixVoice at Play time.
- `AudioBus::GetLastEffectChainStatus()` + `GetLastEffectChainOutputChannels()`
  — diagnostic getters so consumer apps and the IXAPO verification
  example can surface registration status without rebuilding the engine.
- `examples/11-custom-audio-dsp/Assets/audio/test_tone.wav` — 440 Hz
  mono 22050 Hz sine wave, 2s (~88 KB), procedurally generated. The
  example's test source (Compat `LoadSoundMem` + `PlaySoundMem(LOOP)`
  routes through MusicPlayer → BGM bus → Tremolo).
- example-11 `Runtime diagnostics` panel showing `Process()` call
  count, total frames, format, and `SetEffectChain` HRESULT live.
  Regression guard: if any of the three defects above reintroduces,
  the counter visibly stays at 0.

### Changed
- `examples/11-custom-audio-dsp/main.cpp`: Tremolo now attached to BGM
  bus (previously SE bus — the Compat `PlayMusic`/`PlaySoundMem(LOOP)`
  route lands on BGM, so SE bus attachment made the Tremolo unreachable).
  The example's Tremolo class now has an atomic `Process()` call
  counter + last-format observation for diagnostic display.
- Added "Audio routing defects resolved 2026-04-19" section to ADR-0010
  documenting all three defects + the SoundPlayer sibling issue.

### Added to deferred tracking
- `TR-defer-soundplayer-bus-routing` — same fix as MusicPlayer but for
  `SoundPlayer` → SE bus. Mechanical, not landed because no consumer
  currently needs SE-bus effects runtime-validated.
- `TR-defer-compat-playmusic-ogg` — `Compat::PlayMusic` currently loads
  via `Sound::LoadFromFile` which only parses WAV. Route `.ogg`
  extensions to the existing `OggStream` class. Separate from IXAPO.

### Sprint closes
- **Sprint-001 Complete** (DoD 5/5 green): FontManager Detach manual
  crash test PASS (user verified at PC 2026-04-19 — no AV, no crash
  dumps).
- **Sprint-002 Complete** (DoD 9/9 green): IXAPO Process() runtime
  verification PASS — `Process()` calls counter reached 1271+ within
  seconds of launch, audible Tremolo on 440 Hz test tone.

## [SDK Sprint 3] - 2026-04-19

### Added
- **Compat_Particle regression test** (`Tests/unit/compat/compat_particle_test.cpp`,
  3 tests): pins the `count < 0` precondition added 2026-04-17 — covers
  count=-1, -1M, and INT_MIN.
- **EntityBridge lifecycle tests** (`Tests/unit/ecs/entity_bridge_test.cpp`,
  5 tests): ClearMappings resets count, ImportEntity creates ECS entity,
  SyncSceneToWorld imports all, Export round-trips, cross-World mapping
  collision silently-invalid-handle behaviour pinned.
- **AsyncLoader caller-side concurrent stress test**
  (`Tests/unit/io/async_loader_concurrent_test.cpp`, 4 tests):
  4 caller threads × 25 loads without loss, concurrent GetStatus poll,
  re-entrant callback calling Load (non-deadlock), CancelAll race safety.
- **Doxygen coverage** on 13 `*F` float-coordinate drawing functions in
  `GXLib/Compat/GXLib.h` — closes TR-api-004. All 6 Compat headers now
  zero-undocumented per grep check.

### Changed
- **ADR-0007** gains a §Known Limitation section: AsyncLoader is
  currently single-worker (not the multi-worker dispatch to JobSystem
  the §Requirements implies). Caller API is mutex-guarded and safe;
  true multi-worker upgrade tracked as `TR-defer-asyncloader-jobsystem`.
- **ADR-0020** gains a §Testing Scope section documenting the 8 unit-
  test coverage envelope + the three evaluated-and-rejected MP4-fixture
  generation strategies (binary commit, CMake+ffmpeg, MF in-proc) —
  deferred as `TR-defer-movie-integration-tests`.
- **Animation epic** flipped Complete — SetGlobalBusBridge was delivered
  via EventBus story-004 on 2026-04-17; the epic doc was stale.
- **Compat-API epic** flipped Complete — Doxygen coverage closed.
- **Sprint-002 + Sprint-003** drafted, executed, and closed in-session.

### Epic + sprint sync
- Sprint-002: 7/9 DoD green; 2 user-at-PC manual-QA items carry to
  sprint-004 or opportunistic close.
- Sprint-003: Complete. 4 tasks Done, 1 Resolved-as-deferred, 1 Docs-close.
- `production/epics/index.md` priority-order updated: arch-fixes +
  EventBus + Editor + GUI + Animation + Compat-API all ✅ Complete;
  only open gap is Audio IXAPO runtime verification (user-at-PC).

### Test suite
- 4977 → **4981 tests** (+4 new this sprint).
- Cumulative for this 4/18-4/19 session pair: 4957 → 4981 (+24).

## [SDK Sprint 2] - 2026-04-19

### Stage transition
- **Pre-Production → Production** (production/stage.txt flipped 2026-04-19
  after TD-PHASE-GATE READY + PR-PHASE-GATE CONCERNS-then-resolved). The
  architecture foundation is stable; Production sprint cadence begins.

### Architecture review + fixes
- `/architecture-review` 2026-04-18 (fresh-session, independent) found
  **7 REAL ISSUES** across ADR-0018/0019/0020 (the three newest ADRs being
  audited for the first time by external agents).
- All 7 issues **resolved in-session** 2026-04-18:
  - **R1** ADR-0015 + ADR-0019 bidirectional cross-reference for the
    PIE/SimulationManager layering (PlayInEditor orchestrates UX,
    SimulationManager + SceneSnapshot are the Core-layer backend).
  - **R2** ADR-0018 §9 threading contract rewritten to "per-instance
    non-reentrant" (removed the self-contradictory "main-thread-only by
    convention" claim); added EventBus worker-thread cross-reference.
  - **R3** ADR-0020 declares the AssetDatabase bypass as a Known Exception
    (§7 new section); Depends On now lists ADR-0007; new
    `movie_in_rollback_window` forbidden pattern.
  - **E1** AI module Graphics dependency split — created
    `GXLib/AI/Debug/NavMeshDebug.cpp` and the new `GXLib_AIDebug` sibling
    CMake target. Core `NavMesh.cpp` + `NavMesh3D.cpp` are now genuinely
    Foundation-only.
  - **E2** SceneSerializer.cpp relocated from `Core/Scene/` to
    `Graphics/3D/` to restore the `scene_renderer_in_core` forbidden
    pattern. `GXLib_Graphics` PUBLIC-links `GXLib_Scene`.
  - **E3** `ScenePersistence::SaveToFile` now uses a temp-file + rename
    pattern (`std::filesystem::rename`). Destination is untouched on any
    failure. ADR-0019 §5 gains the atomicity invariant as REQUIRED.
  - **E4** New `GXLib/Core/MFPlatform.{h,cpp}` — process-global Media
    Foundation refcount wrapper. MoviePlayer + VideoRecorder migrated to
    `MFPlatform::Acquire`/`Release`. Cohabitation scenario (one closes
    while the other is live) no longer tears down MF prematurely.
- Post-fix `/architecture-review 2026-04-18b` PASS — zero REAL ISSUES
  remaining for the first time in the project's review history.
- ADR polish: M1 (VideoRecorder capture-point + thread), M5 (EntityBridge
  API cross-reference between ADR-0004 and ADR-0019), M6 (SceneRenderer
  FrameGraph cross-reference), M8 (EntityBridge stale-sync debug assertion)
  + 2 cosmetic items closed 2026-04-19.

### Added
- **`GXLib/Core/MFPlatform`** (h + cpp): process-wide Media Foundation
  refcount wrapper. Acquire/Release/IsInitialized/GetRefCount.
- **`GXLib/AI/Debug/NavMeshDebug.cpp`**: Graphics-dependent AI methods
  (BuildFromTerrain, DebugDraw, DebugDrawPath, NavMesh3D::DebugDraw) —
  packaged in the new `GXLib_AIDebug` sibling target.
- **Epic `arch-fixes-2026-04-18`**: 7 stories documenting R1/R2/R3 +
  E1/E2/E3/E4 resolutions, all Done.
- **Sprint plans**: `sprint-001.md` closed, `sprint-002.md` drafted with
  epic-sequencing lookahead for sprint-003 through -005.
- **SDK Production DoD document** at `.claude/docs/sdk-production-dod.md`
  — codifies the game-DoD → SDK-DoD recalibration so future gate-checks
  don't flag N/A game-design items as CONCERNS.
- **12 regression tests** for E1/E3/E4 fixes:
  - `Tests/unit/core/mf_platform_test.cpp` (7 tests)
  - `Tests/unit/scene/scene_persistence_atomic_test.cpp` (5 tests)
- **AI foundation-only link isolation test**: new executable at
  `Tests/isolation/ai_foundation_only/` that links only `GXLib_AI +
  GXLib_Core`. Fails to link if E1 regresses.

### Changed
- `GXLib/Movie/MoviePlayer.cpp`: `MFStartup/MFShutdown` → `MFPlatform::Acquire/Release`.
- `GXLib/Graphics/VideoRecorder.cpp`: same migration.
- `GXLib/Core/Scene/ScenePersistence.cpp`: `SaveToFile` now atomic (temp
  + rename).
- `GXLib/AI/NavMesh.cpp` + `NavMesh3D.cpp`: Graphics includes removed.
- `GXLib/CMakeLists.txt`: new `GXLib_AIDebug` target; `GXLib_Graphics`
  now PUBLIC-links `GXLib_Scene`.
- `tr-registry.yaml` bumped to v8.
- `production/stage.txt`: `Pre-Production` → `Production`.

### Fixed
- Animation epic status synced — `SetGlobalBusBridge` was implemented
  2026-04-17 via EventBus story-004 but the epic doc still said "not
  yet". Corrected.
- GUI epic status — `gx::GetUIContext()` Compat wrapper was already
  landed; epic index "UIContext not exposed" note was stale.
- Editor epic status — CI gates (build-no-editor + lint-editor-boundary)
  were already landed; epic index "CI gates missing" note was stale.

### Test suite
- 4957 → **4969 tests** (+12 new regression tests, zero regressions).

## [SDK Sprint 1] - 2026-04-18

### Added
- **Architecture**: 20 ADRs (all Accepted) covering every engine subsystem
  - ADR-0015 Editor, ADR-0016 EventBus, ADR-0017 Two-Layer Pillar
  - ADR-0018 AI, ADR-0019 Scene, ADR-0020 Movie Pipeline
  - Master architecture document, control manifest, 55 TRs in registry
- **EventBus replay-suppression** (ADR-0016): HandlerCategory enum, SetReplayMode,
  QueueFromWorker thread-safe worker route, AnimationEventDispatcher global bus bridge
- **IXAPO bridge** (Audio/XAPOBridge.h): IAudioEffect::Process() now dispatched
  via XAudio2 SubmixVoice effect chain; AudioBus auto-wires on AddEffect/RemoveEffect
- **Networking Compat wrappers**: 17 GX_* procedural functions
  (GX_StartServer, GX_Connect, GX_Broadcast, GX_ClientSend, etc.)
- **Engine API additions**: GetShaderRegistry(), GetAudioManager(),
  GetNetworkManager(), GetUIContext() — Compat-layer L2 accessors
- **GX_EDITOR CMake option**: shipping builds exclude Editor module entirely
- **CI gates**: GX_EDITOR=OFF build job, editor boundary lint, reflection macro lint
- **Examples 14-16**: hello-network, hello-physics3d, custom-ik
- **Examples 12-13 rewritten**: now use real Widget API via GetUIContext()
- **MoviePlayer tests**: 8 test cases (first coverage for this subsystem)
- **EventBus tests**: 30 test cases across 5 story files
- **18 epics** across Foundation/Core/Feature/Presentation layers
- **Sprint 001 plan** in production/sprints/

### Changed
- Compat_Particle.cpp: AddEmitter return guarded, count validation, real deltaTime
- ADR-0016: QueueFromWorker corrected (SPSC → shared mutex), Fire allocation documented
- ADR-0015: DLL boundary + rotation round-trip notes added
- Examples 06/08/10/11: fixed to match actual engine API signatures
- Renderer3D.h: added GetShaderRegistry() public getter
- Accessibility scorecard: L1 50%→92%, L2 36%→92%

### Fixed
- FontManager::Shutdown: intentional COM Detach() to avoid process-exit AV
- Compat_Particle: silent failures now log via GX_LOG_ERROR
- Compat_Network: duplicate GetNetworkManager definition removed

## [Phase 5] - 2026-03-01

### Added
- Character Controller: movement input, jump, ground normal, run/walk speed
- Ragdoll Builder: Humanoid preset (15 bones), custom bone chains
- Scene Persistence: text/binary save/load with hierarchy restoration
- GPU Particle System: compute-based simulation, billboard rendering, emitter shapes
- Texture Streaming Manager: distance-based mip selection, LRU budget management
- Script Bindings: physics, audio, GUI, filesystem, ECS Lua bindings
- Hot Reload Manager: debounced asset change callbacks via FileWatcher
- Job System: multi-threaded task scheduling with dependencies and ParallelFor
- Audio DSP Effects: low-pass, high-pass, reverb, delay, compressor
- IME Handler: Win32 IMM32 composition/candidate support
- 10 new test files (~154 tests), target 1,100+ total

### Changed
- FileWatcher: rewritten to non-blocking overlapped polling model
- CPU Profiler: added callCount, frame time ring buffer, GX_PROFILE_FUNCTION macro
- Script module: extended dependencies (Physics, Audio, IO, ECS, GUI)

## [Phase 4] - 2026-02-21

### Added
- HDR Display output support
- Variable Rate Shading (VRS) Tier 1/2
- Mesh Shader pipeline (amplification + mesh)
- Sampler Feedback for texture streaming
- DirectStorage integration for fast asset loading
- Reliable UDP networking with congestion control
- State Replication for multiplayer sync
- Lag Compensation (server-side rewind)
- Play-in-Editor workflow (play/pause/stop/step)
- Physics Debug Visualization overlay
- UI Tween animation system
- Reflection System (compile-time type info)
- Data-oriented ECS (World, Archetype, Query, System)
- Video Recording (frame capture + MP4 export)

## [Phase 3] - 2026-02-15

### Added
- Coroutine scheduler (co_await integration)
- Undo/Redo system with command pattern
- Physics Material database
- Cloth Simulator (Verlet integration)
- Atlas Packer (bin-packing for sprite sheets)
- Node Graph editor framework
- Asset Database with dependency tracking
- Debug Draw 3D (lines, spheres, boxes overlay)
- Audio Occlusion (ray-based obstruction)
- Scene Persistence (initial JSON format)
- Screen Capture (screenshot + region)
- Networking foundation (TCP/UDP sockets)
- Compute Skinning (GPU bone transform)
- GPU Occlusion Culling (Hi-Z based)

## [Phase 2] - 2026-02-10

### Added
- EventBus (publish/subscribe messaging)
- ObjectPool (reusable object allocation)
- ActionScheduler (delayed/repeated actions)
- GameState machine (stack-based state management)
- Save/Load system (binary serialization)
- Settings manager (persistent configuration)
- SceneManager (scene transitions)
- Behavior Tree AI framework
- And 12 additional subsystem features

## [Phase 1] - 2026-02-01

### Added
- Gamepad vibration support
- Logger enhancement (file output, categories)
- Capsule collision shape
- Physics constraints (6 types)
- OGG Vorbis streaming
- NavMesh pathfinding
- RVO obstacle avoidance
- Prefab system
- Script engine (Lua 5.4 + sol2)
- GUI widget system
- GJK/EPA collision detection
- Motion Blur post-effect
- Deferred rendering pipeline

## [Phase 0] - 2026-01-15

### Added
- Initial DirectX 12 engine framework
- Math library (Vector2/3/4, Matrix4x4, Quaternion)
- 2D rendering (SpriteBatch, TextureManager, FontManager)
- 3D rendering (PBR, Phong, Toon shaders)
- Audio system (XAudio2, 3D spatial audio)
- Input system (Keyboard, Mouse, Gamepad)
- Scene/Entity/Component architecture
- Physics integration (Jolt Physics)
- DXLib-compatible API layer
