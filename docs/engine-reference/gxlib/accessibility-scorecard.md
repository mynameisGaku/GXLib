# GXLib Accessibility Scorecard

> **Generated**: 2026-04-18
> **Pillar**: ADR-0017 Two-Layer Accessibility
> **Scoring method**: per-subsystem evaluation against L1.1–L1.5 (beginner-usable) and L2.1–L2.5 (core-modifiable)

## Legend

| Symbol | Meaning |
|--------|---------|
| ✅ | Criterion met |
| ⚠️  | Partial / inconsistent |
| ❌ | Criterion not met |
| — | Not applicable for this subsystem |

---

## Layer 1 Criteria (Beginner-usable)

| Criterion | Test |
|-----------|------|
| **L1.1** | DXLib-shaped `gx::` procedural call OR `gx::App` method exists |
| **L1.2** | Sensible defaults — minimal-args call produces reasonable result |
| **L1.3** | Failure returns -1 AND logs via `gx::Logger` (no silent fail) |
| **L1.4** | Doxygen `///` with description + return semantics + example |
| **L1.5** | Minimal usage example reachable (template / `examples/` / docstring) |

## Layer 2 Criteria (Core-modifiable)

| Criterion | Test |
|-----------|------|
| **L2.1** | Extension point public header (no `detail::` / friend lock) |
| **L2.2** | In-repo example of extension exists |
| **L2.3** | Doxygen explains override contract + threading + integration |
| **L2.4** | Transitive types on extension path also public |
| **L2.5** | Extension works as application-side code (no engine recompile) |

---

## Subsystem Scorecard

### Graphics / Rendering

| Criterion | Score | Notes |
|-----------|-------|-------|
| L1.1 | ✅ | 2D: LoadGraph/DrawGraph/etc (18+ fns). 3D: LoadModel/DrawModel/SetCameraPositionAndTarget |
| L1.2 | ✅ | 1280×720 / 32-bit / windowed / alpha-blend defaults work |
| L1.3 | ✅ | Compat_2D.cpp + Compat_3D.cpp all -1 paths log via GX_VALIDATE_MODEL_HANDLE + explicit GX_LOG_ERROR (Sprint 1 T1.1) |
| L1.4 | ✅ | GX/Draw2D.h + GX/Text.h rewritten with full Doxygen + bilingual + examples (Sprint 1 T1.4). Compat/GXLib.h averages 4.4 comments/fn |
| L1.5 | ✅ | template/main.cpp + examples/01-hello-sprite + examples/04-hello-3d |
| **L1 subtotal** | **5/5** | |
| L2.1 | ✅ | FrameGraph::AddPass public. PostEffectPipeline::ICustomEffect public. ShaderRegistry::CustomShaderModelDesc public |
| L2.2 | ✅ | examples/05-custom-postfx (ICustomEffect), examples/06-custom-shader-model (ShaderRegistry.RegisterCustomShaderModel + Rainbow.hlsl) |
| L2.3 | ✅ | Doxygen on ICustomEffect / CustomShaderModelDesc explains contract, 3-RT GBuffer format, ping-pong behaviour |
| L2.4 | ✅ | RenderTarget / PipelineStateBuilder / Vertex3D_PBR all public headers |
| L2.5 | ✅ | RegisterCustomShaderModel compiles user HLSL at runtime via Shader compiler. InsertCustomEffect attaches user code. No engine recompile. |
| **L2 subtotal** | **5/5** | |

### Audio

| Criterion | Score | Notes |
|-----------|-------|-------|
| L1.1 | ✅ | LoadSoundMem / PlaySoundMem / PlayMusic + gx::LoadSound alias |
| L1.2 | ✅ | Sensible defaults (volume 255, one-shot) |
| L1.3 | ⚠️ | Compat_Sound.cpp: PlayMusic logs on failure (Sprint 1). Other paths silent but currently don't have -1 returns to log. |
| L1.4 | ✅ | GX/Audio.h full Doxygen + bilingual + example (Sprint 1 T1.4) |
| L1.5 | ✅ | examples/02-hello-sound |
| **L1 subtotal** | **4.5/5** | |
| L2.1 | ✅ | IAudioEffect interface public in Audio/AudioEffect.h (ADR-0017 L2 added). AudioBus::AddEffect / RemoveEffect registration API public. |
| L2.2 | ✅ | examples/11-custom-audio-dsp demonstrates Tremolo derivation + registration. IXAPO bridge (XAPOBridge.h) now dispatches Process() via SetEffectChain. |
| L2.3 | ✅ | Threading contract (audio callback thread, no heap, atomic params) documented in Doxygen + README |
| L2.4 | ✅ | Transitive types (AudioBus, AudioManager) public |
| L2.5 | ✅ | Effect can be authored + registered + dispatched app-side via XAPOBridge (2026-04-17) |
| **L2 subtotal** | **5/5** | IXAPO bridge closes the L2 gap |

### Physics

| Criterion | Score | Notes |
|-----------|-------|-------|
| L1.1 | ⚠️ | CharacterController + RigidBody3D have direct APIs but no DXLib-flat Compat wrappers (DXLib has no physics) |
| L1.2 | ✅ | Reasonable defaults (fixed-timestep 60Hz per ADR-0009) |
| L1.3 | N/A | Not a Compat-layer subsystem |
| L1.4 | ✅ | Good Doxygen on public physics API |
| L1.5 | ✅ | examples/08-hello-physics (PhysicsWorld2D + RigidBody + Raycast) |
| **L1 subtotal** | **4/5** | Sample 08 closes L1.5 gap |
| L2.1 | ✅ | PhysicsShape public, RagdollBuilder public, PhysicsMaterial via AssetDB |
| L2.2 | ✅ | examples/15-hello-physics3d (CreateBoxShape + AddBody + Step + GetPosition) |
| L2.3 | ✅ | PhysicsShape interface documented (sphere/box/capsule/cylinder/convexhull/mesh) |
| L2.4 | ✅ | Vector3 / Matrix4x4 public |
| L2.5 | ✅ | Deriving PhysicsShape works app-side |
| **L2 subtotal** | **4/5** | Sample 15 closes L2.2 gap |

### Asset Database / IO

| Criterion | Score | Notes |
|-----------|-------|-------|
| L1.1 | — | AssetDB is Layer 1.5 (typed loaders exposed via gx:: already — LoadGraph/LoadSoundMem/LoadModel all route through it) |
| L1.2 | ✅ | AssetId from path works |
| L1.3 | ✅ | Logging exists on path resolve failure |
| L1.4 | ✅ | AssetDatabase.h well-documented |
| L1.5 | ✅ | All L1 examples transitively demonstrate assets |
| **L1 subtotal** | **4/5** |  |
| L2.1 | ✅ | IFileProvider public, AssetDatabase::RegisterType<T> public |
| L2.2 | ✅ | examples/10-custom-asset-type (AssetDatabase FindAsset + DetectChanges pattern) |
| L2.3 | ✅ | Provider + type registration contracts documented |
| L2.4 | ✅ | AssetHandle<T>, AssetId public |
| L2.5 | ✅ | Registering new asset types works app-side |
| **L2 subtotal** | **5/5** | Sample 10 closes L2.2 gap |

### Input

| Criterion | Score | Notes |
|-----------|-------|-------|
| L1.1 | ✅ | CheckHitKey / GetMouseInput / GetMousePoint / GetJoypadInputState + ActionMapping |
| L1.2 | ✅ | Sensible deadzones, 4-pad XInput default |
| L1.3 | ✅ | Most paths return 0 on success; failures rare (hot-plug handled) |
| L1.4 | ✅ | GX/Input.h rewritten with full Doxygen + bilingual + examples (Sprint 1 T1.4) |
| L1.5 | ✅ | examples/03-hello-input |
| **L1 subtotal** | **5/5** | |
| L2.1 | ✅ | Keyboard / Mouse / Gamepad / IMEHandler all public per ADR-0011 |
| L2.2 | ⚠️ | ActionMapping usage is in example 03, but no "add custom input device" sample (low priority — XInput is the only device) |
| L2.3 | ✅ | Input-ownership contract documented in ADR-0011 + ADR-0012 |
| L2.4 | ✅ | Underlying types public |
| L2.5 | ✅ | Can write app-side input-remapping UI trivially |
| **L2 subtotal** | **4.5/5** | |

### GUI

| Criterion | Score | Notes |
|-----------|-------|-------|
| L1.1 | ⚠️ | GUI is Layer 1.5 (UIContext + Widget). DrawString/DrawBox are L1 (handled under Graphics above). No gx:: procedural "DrawMenu" helper — by design per ADR-0012. |
| L1.2 | ✅ | Widget defaults reasonable |
| L1.3 | ✅ | Widget creation returns nullptr on failure (checked pattern) |
| L1.4 | ✅ | UIContext.h / Widget.h good Doxygen |
| L1.5 | ✅ | examples/12-hello-gui (code-built Panel/Button/Text tree + onClick) |
| **L1 subtotal** | **4.5/5** | |
| L2.1 | ✅ | Widget base class public, all 17 widgets public |
| L2.2 | ✅ | examples/13-custom-widget (CircularGauge, Widget derivation + RenderSelf + OnEvent) |
| L2.3 | ✅ | Widget override contract documented |
| L2.4 | ✅ | StyleSheet, DataBinding public |
| L2.5 | ✅ | Deriving Widget works app-side |
| **L2 subtotal** | **5/5** | |

### Networking

| Criterion | Score | Notes |
|-----------|-------|-------|
| L1.1 | ✅ | 17 GX_* procedural functions: GX_StartServer / GX_Connect / GX_Broadcast / GX_ClientSend + stats + callbacks (2026-04-17) |
| L1.2 | ✅ | GX_StartServer(port, maxClients=16) — sensible default |
| L1.3 | ✅ | All GX_* functions return 0/-1 with GX_LOG_ERROR on failure |
| L1.4 | ✅ | GXLib.h full Doxygen on all 17 networking functions |
| L1.5 | ✅ | examples/14-hello-network (GX_StartServer / GX_Connect / GX_Broadcast chat demo) |
| **L1 subtotal** | **5/5** | Full L1 compliance (2026-04-18) |
| L2.1 | ✅ | All layers public per ADR-0013 (NetworkReplicator / ReplicatedProperty<T> / RollbackNetcode) |
| L2.2 | ❌ | No in-repo multiplayer sample |
| L2.3 | ✅ | ADR-0013 documents layering + contracts |
| L2.4 | ✅ | EntityHandle / AssetId used on wire |
| L2.5 | ⚠️ | Production-quality work blocked on Tier 2 gaps (STUN stub, CloudSave sim, etc.) |
| **L2 subtotal** | **3/5** | |

### Animation

| Criterion | Score | Notes |
|-----------|-------|-------|
| L1.1 | ✅ | PlayModelAnimation / SetModelAnimationTime / GetModelAnimationTime (Compat) |
| L1.2 | ✅ | Default playback rate = 1.0 |
| L1.3 | ✅ | Compat_3D anim paths log via GX_VALIDATE_MODEL_HANDLE + animIndex range checks (Sprint 1 T1.1) |
| L1.4 | ✅ | Compat/GXLib.h documents all anim functions |
| L1.5 | ✅ | examples/09-hello-animation (LoadModel + PlayModelAnimation + clip switching) |
| **L1 subtotal** | **5/5** | Sample 09 closes L1.5 gap |
| L2.1 | ✅ | Skeleton / Animator / BlendTree / IK suite all public per ADR-0014 |
| L2.2 | ❌ | No examples/custom-ik-solver or custom-animation-layer sample |
| L2.3 | ✅ | IK / state machine / blend tree contracts documented |
| L2.4 | ✅ | Bone / Transform3D public |
| L2.5 | ✅ | Deriving IKSolver works app-side |
| **L2 subtotal** | **4/5** | |

### ECS

| Criterion | Score | Notes |
|-----------|-------|-------|
| L1.1 | — | ECS is Layer 1.5-to-2 by design; not exposed via Compat |
| L1.2 | ✅ | Default archetype behaviour sensible |
| L1.3 | N/A | Not a Compat layer |
| L1.4 | ✅ | ADR-0004 + World.h / Query.h well-documented |
| L1.5 | ✅ | examples/07-hello-ecs (World + AddComponent + ForEach particle bounce) |
| **L1 subtotal** | **4/5** | Sample 07 closes L1.5 gap |
| L2.1 | ✅ | World / Query / System public; POD-component rule enforced |
| L2.2 | ✅ | examples/07-hello-ecs (ECS World usage pattern) |
| L2.3 | ✅ | POD-component + command-buffer contracts documented |
| L2.4 | ✅ | EntityHandle / component types public |
| L2.5 | ✅ | Defining new components + systems works app-side |
| **L2 subtotal** | **5/5** | Sample 07 closes L2.2 gap |

---

## Overall Summary

| Subsystem | L1 | L2 | Status |
|-----------|----|----|--------|
| Graphics | 5.0 | 5.0 | ✅ Full pillar compliance |
| Audio | 4.5 | 5.0 | ✅ IXAPO bridge completes L2 (2026-04-17) |
| Physics | 4.0 | 4.0 | ⚠️ L2.2 closed (sample 15); custom-ragdoll example still missing |
| Asset DB | 4.0 | 5.0 | ✅ Sample 10 closes L2 gap |
| Input | 5.0 | 4.5 | ✅ Near-full compliance |
| GUI | 4.5 | 5.0 | ✅ Samples 12/13 now use real Widget API via GetUIContext() |
| Networking | 5.0 | 3.0 | ⚠️ L1 full; L2 stubs remain (STUN/CloudSave sim) |
| Animation | 5.0 | 4.0 | ⚠️ Missing custom-IK L2 example |
| ECS | 4.0 | 5.0 | ✅ Sample 07 closes both gaps |

**Average across 9 subsystems**: L1 = 4.6/5 (92%), L2 = 4.5/5 (90%)

### Progression

| Date | L1 avg | L2 avg | Notes |
|------|--------|--------|-------|
| 2026-04-16 (initial) | 50% | 36% | Pillar established |
| 2026-04-16 (sprint 1) | 68% | 40% | Silent-failure logging + Doxygen |
| 2026-04-16 (sprint 2) | 76% | 74% | Extension APIs + IAudioEffect + samples 01-06 |
| 2026-04-16 (batch) | 84% | 86% | Samples 07-13 + GUI/Networking wrappers |
| 2026-04-18 (current) | **90%** | **88%** | IXAPO bridge + Networking Compat + GetUIContext + Widget examples + EventBus replay |

**Revised average**: L1 = 4.1/5 (82%), L2 = 4.1/5 (82%)

**Final after GUI samples 12/13**:
- GUI L1: 3.5 → 4.5 (sample 12)
- GUI L2: 4.0 → 5.0 (sample 13)

**Session-end average**: L1 = 4.2/5 (**84%**), L2 = 4.3/5 (**86%**)

## Full compliance (5/5 + 5/5) subsystems
- Graphics ✅
- GUI ✅  
- Asset DB (near: 4/5 L1, 5/5 L2)

## Priority Gap List

### Immediate (pillar compliance, small effort)
1. `examples/07-custom-audio-dsp/` — blocked on IAudioEffect interface design (ADR extension)
2. `examples/08-custom-asset-type/` — AssetDatabase::RegisterType<T> already exists, just needs a sample
3. `examples/09-hello-ecs/` — demonstrate World + Query + System basic loop
4. `examples/10-hello-animation/` — play + blend animation clips

### Medium effort
5. IAudioEffect interface + RegisterEffect<T> (ADR extension for ADR-0010)
6. `examples/11-custom-widget/` — derive gx::GUI::Widget, register with UIContext
7. `examples/12-hello-physics/` — rigid bodies + raycast

### Large effort (scoped separately)
8. `examples/13-custom-ik-solver/` — needs Animation ADR extension
9. `examples/14-multiplayer/` — blocked on networking Tier 2 production work
10. `gx::` procedural Compat wrappers for Networking (new L1 surface)

## Progression Since Sprint 1/2 Start

| Date | L1 average | L2 average | Notes |
|------|-----------|-----------|-------|
| 2026-04-15 | ~2.5 | ~1.8 | Before ADR-0017 + Sprint 1/2 |
| 2026-04-16 (post Sprint 1) | 3.4 | 2.0 | +Logger + examples 01-05 + Doxygen |
| 2026-04-16 (post Sprint 2) | 3.8 | 3.7 | +PostFX/ShaderModel extension APIs + example 06 + Material.shaderModel Compat |

Graphics reached full 5/5 on both layers this cycle.

---

## How to Use This Scorecard

- **Per PR**: any public API change should not lower any subsystem's L1 or L2 score
- **Per release**: regenerate this scorecard; regressions = blocking issues
- **Per ADR**: new ADRs claiming extension points must raise L2 scores for their domain
- **For newcomers**: read the Graphics row to see what "full compliance" looks like

## Reference
- ADR-0017 Two-Layer Accessibility Pillar
- `docs/implementation-gap-analysis-2026-04-16.md` (superseded by this scorecard for Layer tracking)
