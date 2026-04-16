# GXLib Implementation Gap Analysis

> **Date**: 2026-04-16
> **Source**: 3 parallel Explore agents — Graphics extensibility (Layer 2), cross-subsystem completeness, Layer 1 entry friction
> **Pillar**: Per ADR-0017 (Two-Layer Accessibility) — every gap is scored against L1 (beginner-usable) or L2 (core-modifiable) impact
> **Status**: Inputs to epic / story planning

## Verdict

GXLib has **broad but uneven** Phase 0–5 coverage. The Layer 1 entry surface is good but has discoverable friction (no mid-complexity samples, silent failures, all-Japanese docs). The Layer 2 extension surface has clean foundations (FrameGraph, ShaderLibrary, Material) but **3 critical extension points are hard-locked** (PostFX chain order, MaterialDomain enum, ShaderRegistry PSO set). Several subsystems are skeletons or simulations rather than production-ready (Movie, NAT, Matchmaking, CloudSave, RTGI).

---

## Tier 1 — Pillar Violations (immediate, unblocks everything else)

These directly violate ADR-0017 forbidden_patterns. Fixing them is non-negotiable for the pillar to mean anything.

| # | Gap | Pillar Violation | Impact |
|---|-----|------------------|--------|
| **T1.1** | Compat layer functions return -1 silently with no `GX_LOG_ERROR` (e.g. `Compat_2D.cpp:61-67`, GraphicsDevice.cpp throughout) | `silent_failure_in_compat_layer` | HIGH — beginners can't diagnose anything |
| **T1.2** | No `samples/` or `examples/` directory with mid-complexity samples (`hello-sprite`, `hello-sound`, `hello-input`, `hello-3d`, `custom-postfx`, `custom-shader`) — only the 35-line template and the 86 KLoC GXModelViewer exist | L1.5 + L2.2 violation across the SDK | HIGH — beginners and Layer 2 users both blocked from discovery |
| **T1.3** | Template `main.cpp` ignores `GX_Init` return value — beginner sees black window on init failure | L1.3 violation in the canonical entry point | MEDIUM-HIGH |
| **T1.4** | `GX/*.h` headers (Draw2D.h, Audio.h, Input.h, Math.h) are facade wrappers with forward-declarations only — beginners trying to discover the API by reading these find nothing useful | L1.4 — discoverability failure | MEDIUM |
| **T1.5** | PostFX chain hard-coded in `PostEffectPipeline::Resolve()` (line 404) with no insertion API | `internal_only_extension_point` (PostFX is exactly the kind of Layer 2 extension users want) | HIGH |
| **T1.6** | `MaterialDomain` is a closed enum (`gxformat/shader_model.h:14-25`) — Custom=255 exists but is second-class, not integrated with batching/LOD | `internal_only_extension_point` | HIGH |
| **T1.7** | `ShaderRegistry` hard-codes 14 PSO combinations; new shader models require modifying engine source | `internal_only_extension_point` | HIGH |

## Tier 2 — Implementation Skeletons / Stubs (functional gaps)

ADRs claim these features; code does not deliver them.

| # | Subsystem | Specific Gap | Source |
|---|-----------|--------------|--------|
| **T2.1** | Movie | `MoviePlayer::DecodeNextFrame()` body missing entirely; Media Foundation decode loop unimplemented; **0 test files** | `GXLib/Movie/MoviePlayer.cpp` |
| **T2.2** | Graphics RT | RTGI/RTReflections/RTSoftShadows initialise pipeline but GPU resource chain incomplete; not integrated into main renderer | `GXLib/Graphics/RayTracing/*` |
| **T2.3** | Graphics | `GPUDrivenRenderer.cpp:181, 536` — explicit "PSO not ready" / "GPU resources incomplete" stubs; falls back to non-GPU-driven path | `GXLib/Graphics/3D/GPUDrivenRenderer.cpp` |
| **T2.4** | Network | `NATTraversal::Discover()` is placeholder state machine; STUN actually-send-packet not implemented; hole punch mocked | `GXLib/IO/Network/NATTraversal.cpp:24` |
| **T2.5** | Network | `MatchmakingLobby` is in-memory only (comment: シミュレーション); no backend RPC | `GXLib/IO/Network/MatchmakingLobby.cpp` |
| **T2.6** | Network | `CloudSave` always returns success ("常に接続成功"); no real cloud calls; no conflict resolution beyond timestamp | `GXLib/IO/Network/CloudSave.cpp` |
| **T2.7** | Network | `RollbackNetcode` lacks desync detection; no input confirmation timeout; no frame prediction bias correction | `GXLib/IO/Network/RollbackNetcode.cpp` |
| **T2.8** | Editor | `PlayInEditor` only saves/restores Transform; no component state save, no prefab variant tracking, no destroyed-entity undo | `GXLib/Editor/PlayInEditor.cpp` |
| **T2.9** | Editor | `ShaderGraph` lacks cycle detection + missing-input validation | `GXLib/Editor/ShaderGraph.cpp` |
| **T2.10** | Animation | `MotionMatching::AddClip()` doesn't validate skeleton compatibility; `Build()` may proceed silently on mismatched clips | `GXLib/Graphics/3D/MotionMatching.cpp` |

## Tier 3 — Layer 2 Extensibility Improvements (post-Tier-1)

After Tier 1's hardest violations are fixed, these polish the Layer 2 promise.

| # | Gap | Action |
|---|-----|--------|
| **T3.1** | Custom shader registered via `Renderer3D::CreateMaterialShader()` does NOT participate in `ShaderHotReload` or variant generation | Extend ShaderLibrary public API to register external shader paths with hot-reload |
| **T3.2** | FrameGraph has no public API for pass selection heuristics (e.g., "skip shadow pass when no shadow casters"); only global `SetPassEnabled()` | Add user-facing pass-condition callback or pass-group toggle |
| **T3.3** | No documented threading-contract examples for FrameGraph custom passes | Add Doxygen with worked example showing main-thread vs worker-thread access |
| **T3.4** | No example custom audio DSP effect (ADR-0010 §8 says `AudioDSP::RegisterEffect<T>` is the extension point but no in-repo example) | Add `examples/custom-audio-dsp/` with worked custom effect |
| **T3.5** | No example custom asset type via `AssetDatabase::RegisterType<T>` | Add `examples/custom-asset-type/` |

## Tier 4 — Quality / Operations

| # | Gap | Action |
|---|-----|--------|
| **T4.1** | ~50 test files for ~450 source files (~11% test:source ratio); Movie has 0 tests; Network has 2 tests for 5 critical components | Targeted test additions per Tier-2 fix; CI green-bar requirement |
| **T4.2** | All Doxygen comments in Japanese — barrier for global audience | Bilingual or English-primary on the Compat / Layer 1 surface; Japanese OK on internals |
| **T4.3** | `gxlib_setup.bat` requires pre-installed VS2022; CMake errors are opaque if missing | Detect VS2022 absence with actionable message + link |
| **T4.4** | No `accessibility-scorecard.md` per ADR-0017 §Migration Plan | Generate per-subsystem L1/L2 scorecard; publish per release |

## Cross-Cutting Wins (existing strengths to preserve)

These are already good — protect them when fixing the gaps above.

- **DXLib Compat coverage**: 109 functions implemented (~83% of typical DXLib usage); migration is mostly copy-paste
- **Template main.cpp**: 35 lines, sensible defaults (1280×720 / 32-bit / windowed / alpha blend), readable
- **FrameGraph public API**: callback-driven, well-documented, actually extensible — `AddPass(name, executeFn)` works as advertised
- **ShaderLibrary**: `RegisterPSORebuilder` callback already exists for Layer 2 hot-reload integration
- **Material data-driven shaderModel field**: enum routing means no hard-coded if/else trees in user code
- **Material `shaderHandle` escape hatch**: bypass default PSO entirely for custom-shader materials
- **AudioBus / DSP insert chain**: `AudioDSP::RegisterEffect<T>` Layer 2 surface already public per ADR-0010
- **Asset Database type registration**: `RegisterType<T>` Layer 2 surface already public per ADR-0007

---

## Recommended Implementation Order

**Sprint 1 (Pillar Violations — 2 weeks)**
- T1.1: silent-failure audit + GX_LOG_ERROR addition (Compat + GraphicsDevice)
- T1.2: bootstrap `examples/` with hello-sprite, hello-sound, hello-input
- T1.3: template main.cpp checks GX_Init return
- T1.4: rewrite GX/*.h headers as educational entry points (curated subset of Compat with proper Doxygen + examples) OR delete them as misleading

**Sprint 2 (Layer 2 unlock — 2 weeks)**
- T1.5: PostFX chain insertion API (`PostEffectPipeline::InsertEffectAfter(StageEnum, unique_ptr<IPostEffect>)`)
- T1.6: MaterialDomain extension API (registration of new domain enum value with routing rule)
- T1.7: ShaderRegistry plugin path (custom shader model registration with PSO callback)
- T3.4 + T3.5: matching `examples/custom-postfx/`, `examples/custom-shader/`, `examples/custom-audio-dsp/`

**Sprint 3 (Skeletons → Production — 3 weeks)**
- T2.1 Movie (Media Foundation actually-decode + tests)
- T2.4–T2.7 Networking production-quality (real STUN, real backend, desync detection)
- T2.8 PlayInEditor full state save/restore
- T2.10 MotionMatching skeleton compatibility validation

**Sprint 4 (Polish — 2 weeks)**
- T2.2/T2.3 Graphics GPU-driven + RT integration
- T2.9 ShaderGraph validation
- T4.1 test coverage push (target 25% test:source minimum)
- T4.2 bilingual Doxygen on Compat surface
- T4.4 accessibility scorecard generation

---

## Per-Pillar Pass/Fail at Current State

| Pillar Criterion | Current State | Pass? |
|---|---|---|
| L1.1 DXLib-shaped procedural call exists | 109 functions, ~83% DXLib coverage | ✅ |
| L1.2 Sensible defaults | Template works at minimal args | ✅ |
| L1.3 Failure paths return -1 + Logger | Returns -1 ✓, Logger ✗ | ❌ (T1.1) |
| L1.4 Doxygen comments with examples | 4.4 comments/fn on Compat — but `GX/*.h` facades have nothing | ⚠️ |
| L1.5 Minimal usage example exists | template/main.cpp ✓, but no `samples/` mid-tier | ⚠️ (T1.2) |
| L2.1 Extension points public, not `detail::` | FrameGraph ✓ ShaderLibrary ✓ AudioDSP ✓ AssetDB ✓ — **but PostFX/MaterialDomain/ShaderRegistry hard-locked** | ⚠️ (T1.5–T1.7) |
| L2.2 In-repo example of extension | None of FrameGraph custom pass / custom shader / custom DSP / custom asset has a documented example | ❌ (T3.4–T3.5) |
| L2.3 Doxygen explains override contract | FrameGraph ✓ ShaderLibrary ✓; ShaderRegistry low; PostFX low | ⚠️ |
| L2.4 Transitive types are public | Mostly ✓ — FrameGraph leaks no detail:: | ✅ |
| L2.5 Modification works as app-side code | FrameGraph ✓ AudioDSP ✓ — but PostFX requires fork | ⚠️ |

**Overall pillar compliance**: ~60%. Tier 1 fixes lift it to ~85%; Tier 2 to ~95%.

---

## Next Step

User picks the sprint cadence. Three concrete options:
- **A**: Do all 4 sprints serially (~9 weeks) — most thorough
- **B**: Sprint 1 + Sprint 2 only (pillar + Layer 2 unlock, ~4 weeks) — get the SDK identity solid, defer skeleton subsystems
- **C**: Cherry-pick top 5 items across all tiers — fastest visible wins
