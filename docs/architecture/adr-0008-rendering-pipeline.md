# ADR-0008: Rendering Pipeline (Deferred + Forward+ Hybrid, FrameGraph, PostFX Chain, HDR)

## Status
Accepted

## Date
2026-04-15

## Engine Compatibility

| Field | Value |
|-------|-------|
| **Engine** | Custom — GXLib (self-hosted) |
| **Domain** | Rendering |
| **Knowledge Risk** | LOW — clustered forward+ / tiled deferred / FrameGraph patterns are well-documented (Frostbite FrameGraph talk, Doom 2016 clustered lighting); HDR10/scRGB swap-chain workflows are within LLM training data |
| **References Consulted** | `docs/engine-reference/gxlib/VERSION.md`, `GXLib/Graphics/{FrameGraph,Pipeline,PostEffect,3D,Rendering}/*`, CHANGELOG Phases 0/1/3/4/5 |
| **Post-Cutoff APIs Used** | None |
| **Verification Required** | Deferred vs Forward+ selection heuristic (transparency count / material variety); HDR10 output verified on real display on Windows 10 21H2 + Windows 11 23H2; PostFX chain stable under DynamicResolution scaling; FrameGraph automatic transitions correct vs manual barriers |

## ADR Dependencies

| Field | Value |
|-------|-------|
| **Depends On** | ADR-0001 (doc strategy), ADR-0002 (DX12 backend), ADR-0006 (Job System — command-list recording on workers), ADR-0007 (Asset DB — shaders/materials/textures flow through AssetDatabase + ShaderHotReload) |
| **Enables** | Future ADRs on GPU particles, Ray Tracing integration (DXR 1.1), VRS coverage, Mesh Shader pipeline details, Lighting system (direct + GI), Shadow system (Cascaded + Contact) |
| **Blocks** | None (code already exists; retroactive) |
| **Ordering Note** | Upgrades TR-rnd-003 and TR-chr-001 from Partial to Covered in the traceability matrix |

## Context

### Problem Statement
GXLib must render 2D+3D scenes at 60 fps with a modern feature set — HDR output, many dynamic lights, soft shadows, post-effects, and support for both opaque PBR and transparent/stylised materials. A single pipeline choice cannot optimally cover this range: Pure Deferred is efficient for many opaque lights but struggles with transparency and material diversity; Pure Forward+ handles materials flexibly but costs more per-light on dense geometry. This ADR records the **hybrid pipeline strategy**, the **FrameGraph abstraction** that schedules render passes and manages transient resources, the **PostFX chain ordering**, and the **HDR output workflow**.

Phase 1 introduced Deferred; Phase 3 added cloth + compute skinning + GPU occlusion; Phase 4 added HDR / VRS / Mesh Shaders / Sampler Feedback; Phase 5 added GPU particles + texture streaming. This ADR captures the emergent structure as a single coherent decision so future rendering ADRs (shadows, GI, ray tracing) have a stable framing to extend.

### Constraints
- DX12 backend (ADR-0002); all rendering work must respect its forbidden patterns (no DX12 types in public headers, caps-gated optional features, main-thread queue submit per ADR-0006)
- 60 fps / 16.6 ms frame budget (ADR-0002)
- Must support: opaque PBR, transparent, cutout, particles, decals, skinned meshes, 2D sprites, text, GUI overlays
- HDR output on capable displays; fall back gracefully to SDR
- Shader hot reload (already in `Graphics/Pipeline/ShaderHotReload`) must not break in-flight frames
- GPU resources flow through AssetDatabase (ADR-0007) — deferred-release quarantine is mandatory

### Requirements
- Deferred path for dense opaque geometry with many lights
- Forward+ path for transparent / stylised / special materials
- Clustered lighting applied to both paths (shared light list)
- FrameGraph describes all passes + transient resources; automatic resource barrier generation; pass culling when output unused
- PostFX chain with stable ordering, configurable per-camera via `PostFXMask` (already introduced)
- HDR workflow: linear scene colour in float16 all the way through PostFX; tone-map + output transform at the end based on display capability
- DynamicResolution integration (Phase 5) scales main render targets but not UI / text
- Shader variants handled by `ShaderLibrary` + `ShaderVariant`; no per-material combinatorial explosion

## Decision

**GXLib uses a hybrid Deferred + Forward+ pipeline driven by a FrameGraph. Opaque PBR geometry goes through Deferred (GBuffer → clustered-light shade → resolve); transparent, stylised, and special-case materials go through Forward+ (same clustered light list, shaded per-pixel in the material shader). Post-processing runs on a linear-float16 HDR chain with a fixed canonical order, ending in a tone-map + output-transform pass selected at startup based on display capability. PostFX opt-in per camera via `PostFXMask`. DynamicResolution scales only the main-scene render targets.**

Concrete rules:

1. **Hybrid pipeline routing.** Each renderable registers with a `MaterialDomain` enum: `Opaque` (→ Deferred), `Transparent` / `Cutout-stylized` / `Decal-screenspace` / `Particle` (→ Forward+), `UI2D` / `Text` (→ after PostFX, SDR path). Routing is a data property of the material, not a runtime branch.

2. **FrameGraph is the pass scheduler.** `GXLib/Graphics/FrameGraph/FrameGraph` manages:
   - Passes declared per frame (read/write resource handles declared upfront)
   - Transient resources (GBuffer slices, shadow atlas, scratch targets) pooled and aliased
   - Automatic `ResourceBarrier` emission — manual barriers are a forbidden pattern
   - Pass culling: if a pass's output is never read, it's skipped
   - Command-list recording parallelised per independent pass via JobSystem (per ADR-0006)

3. **GBuffer layout (Deferred).** 4 render targets:
   - **RT0**: Albedo (RGB8) + MaterialID (A8)
   - **RT1**: Normal (RGB10A2, octahedral-encoded) + Roughness
   - **RT2**: Metallic + AO + Emissive mask + MotionVector (packed)
   - **RT3**: Depth (D32_FLOAT, separate Z-buffer)
   Fixed layout — extending requires a superseding ADR.

4. **Clustered lighting.** `Graphics/3D/ClusteredLighting` builds a 3D cluster grid (16×9×24 typical) with per-cluster light lists. Both Deferred shade pass and Forward+ material shaders sample the same cluster list. Lights: punctual (point/spot), directional (cascaded shadow), area (approximated). Light count soft cap: 512 per frame; hard cap: 4096.

5. **Canonical PostFX order.** Post-processing runs on linear-float16 scene colour in this fixed order — skipping disabled effects but never reordering:
   ```
   Scene (fp16) →
     AutoExposure (compute — luminance histogram)
     → DepthOfField (circle-of-confusion)
     → MotionBlur (object + camera)
     → Bloom (dual-filter downsample + upsample)
     → LensFlare
     → ChromaticAberration
     → FilmGrain
     → ColorGrading (LUT + HDR-aware)
     → OutlineEffect (post-geometry-edge pass)
     → ContactShadows (screen-space)
     → ToneMap + Output Transform (→ display colour space)
     → SDR UI composite → Present
   ```
   `PostFXMask` is a bitmask per camera enabling/disabling any subset of the above.

6. **HDR workflow.** Main scene targets are `R16G16B16A16_FLOAT` throughout Deferred resolve + Forward+ + PostFX. Tone-map + output transform converts to the swap-chain format at the end:
   - **HDR10** (BT.2020 + ST.2084 PQ) — preferred when display supports it (runtime detection via DXGI)
   - **scRGB** (linear, 16-bit) — alternative HDR output
   - **SDR sRGB** — fallback; uses ACES-filmic or Reinhard tone-mapping
   - User opt-in toggle; default SDR to avoid mis-detection issues

7. **DynamicResolution** (Phase 5) scales the main 3D render targets (GBuffer, transparent buffer, scene-colour HDR). UI, text, and full-screen PostFX that need exact pixels (FilmGrain, ChromaticAberration) operate at native resolution. Tone-map upsamples the scaled scene to native before compositing UI.

8. **Shader management.** `ShaderLibrary` owns compiled PSOs indexed by shader + variant bitmask. `ShaderVariant` enumerates the allowed variant axes (skinning, GPU particles, MSAA, VRS, Mesh Shader path). `ShaderHotReload` watches HLSL sources via AssetReloader (ADR-0007) and rebuilds PSOs in the background; swap happens at frame boundary, old PSO goes to deferred-release quarantine.

9. **Parallel command-list recording.** Independent render passes (shadow cascade N vs N+1, transparent vs particle) record on separate JobSystem workers via ADR-0006's per-worker command list rule. Submission to `ID3D12CommandQueue` is main-thread-only (registered forbidden pattern).

10. **2D path.** 2D sprites / text / GUI bypass the 3D pipeline and run as a separate FrameGraph pass after SDR composition — they are not HDR-aware and should not be tone-mapped.

### Architecture Diagram

```
Frame setup:
   Camera + visible renderables + light list + PostFX mask → FrameGraph builder

FrameGraph (declares passes, transient resources, pass ordering):

   ShadowPass(Cascaded, Contact)     ─┐
   ClusteredLightBuildPass           ─┤  (compute)
                                      ▼
   ComputeSkinningPass (per ADR-0014 §15 — runs before GBufferPass; declares Read on bone matrices + Write on skinned vertex buffer; FrameGraph's Read/Write declarations enforce ordering vs GBufferPass automatically)
                                      │
                                      ▼
   GBufferPass (Deferred, opaque)  ──►  RT0..3 + Depth
                                      │
   SSAOPass                        ───┤  (samples GBuffer)
                                      ▼
   DeferredShadePass            ──► Scene fp16  (clustered lights)
                                      │
   ForwardPlusPass (transparent)  ──► Scene fp16 (same clusters)
                                      │
   DecalPass (screen-space)          ┘
                                      ▼
   PostFX chain (fixed order):
     AutoExposure → DoF → MotionBlur → Bloom → LensFlare → ChromaticAberration
     → FilmGrain → ColorGrading → Outline → ContactShadows
                                      ▼
   ToneMap + OutputTransform  ──►  swap-chain target (HDR10 / scRGB / SDR sRGB)
                                      ▼
   UI2D + Text + DebugDraw        ──►  swap-chain target (SDR composite)
                                      ▼
                                   Present  (main thread only — per ADR-0006)
```

### Key Interfaces

- `gx::FrameGraph::Pass(name)` / `.Read(handle)` / `.Write(handle)` / `.Execute(fn)` — pass declaration
- `gx::MaterialDomain` enum: `Opaque / Transparent / Cutout / Decal / Particle / UI2D / Text`
- `gx::PostFXMask` — 32-bit bitmask (`EnableBloom`, `EnableDoF`, …)
- `gx::ClusteredLighting::BuildLightList(lights, camera)` → cluster grid for the frame
- `gx::ShaderLibrary::GetPSO(shaderId, variantMask)` → PSO handle
- `gx::OutputTransform` enum: `HDR10 / scRGB / SDR_ACES / SDR_Reinhard` (runtime-selected)
- `gx::DynamicResolution::SetScale(0.5..1.0)` — scales GBuffer/Scene-fp16 only

## Alternatives Considered

### Alternative 1: Pure Deferred (no Forward+ path)
- **Description**: All materials go through Deferred; transparency handled via order-independent transparency (OIT) or a separate translucency pass with forward shading built into the same GBuffer consumer
- **Pros**: Simpler routing; one shade path; GBuffer amortises light cost over many dense opaques
- **Cons**: Material diversity (anisotropic hair, subsurface skin, stylised rim-lit toon) forces either fatter GBuffer (more RTs → more bandwidth) or special-case paths. Transparency with >1 layer is fundamentally awkward in Deferred. OIT costs more than Forward+ for the common few-layer case.
- **Rejection Reason**: Material diversity is part of the "modern SDK" value proposition; forcing all materials through Deferred limits expressiveness or inflates the GBuffer to unaffordable sizes

### Alternative 2: Pure Forward+ (no Deferred)
- **Description**: All materials forward-shaded against the clustered light list
- **Pros**: Clean material authoring; MSAA works directly; transparency is free
- **Cons**: Scales poorly when scenes have hundreds of opaque meshes × many lights — overdraw and redundant light evaluation dominate. Modern dense-geometry urban / dungeon scenes are Deferred's sweet spot. Doom 2016 and most AAA engines use Deferred or hybrid for this reason.
- **Rejection Reason**: GXLib must handle dense opaque scenes at 60 fps; Forward+ alone is bandwidth-limited in that regime

### Alternative 3: Visibility Buffer / GPU-driven (single pass)
- **Description**: Ship only a GPU-driven visibility-buffer pipeline (UE5 Nanite-style)
- **Pros**: Extremely geometry-scalable; minimal CPU cost per draw
- **Cons**: Requires mesh shader support universally (current floor hardware lacks it); complicates transparency, decals, and hand-authored effects; overkill for GXLib's target audience of DXLib-graduating developers; huge implementation cost
- **Rejection Reason**: Not realistic for GXLib's scope and hardware floor; adopting later on top of the hybrid base is viable

### Alternative 4: No FrameGraph (hand-coded pass order)
- **Description**: Each pass manually ordered, resource transitions hand-written
- **Pros**: Slightly faster to write the first pipeline
- **Cons**: Manual barriers are a leading source of DX12 bugs (forgotten transitions, over-transitions); adding a new pass means touching every neighbouring pass; pass culling and transient resource aliasing impossible without a graph
- **Rejection Reason**: FrameGraph pays for itself by the 5th pass; GXLib has 20+

## Consequences

### Positive
- Material authors choose domain; routing is data-driven, not code-branched
- FrameGraph eliminates manual barrier bugs and enables automatic transient-resource aliasing (lower GPU memory)
- Shared clustered light list means Deferred and Forward+ agree on lighting — no double-shaded/unlit mismatches at material boundaries
- Fixed PostFX order makes per-effect behaviour predictable and test-stable
- HDR workflow is linear-throughout, so effects that read scene colour (Bloom, DoF) see physically-meaningful values
- DynamicResolution gives graceful degradation without touching UI readability

### Negative
- Two shading paths to maintain (Deferred GBuffer layout + Forward+ light loop); changes to one may need parity in the other
- FrameGraph abstraction is itself complex — compile-time pass declarations vs runtime flexibility tension
- Fixed GBuffer layout (4 RTs) forecloses material features that need more channels
- Clustered light list build runs every frame even when lights are static — not free on mid-range CPUs
- Fixed PostFX order means e.g. "Bloom after ColorGrading" requires a superseding ADR; not a per-camera setting

### Risks
- **HDR detection mis-reports on some drivers**, causing wrong output transform. *Mitigation*: default SDR; opt-in HDR with a simple per-game toggle; log detected DXGI output format.
- **GBuffer bandwidth dominates on low-end GPUs at 1080p+.** *Mitigation*: octahedral normal encoding (RGB10A2 vs RGBA16F), packed motion vectors, DynamicResolution safety net.
- **Shader variant explosion** (skinning × GPU particles × VRS × MSAA × Mesh Shader) = 32 permutations per shader. *Mitigation*: `ShaderVariant` enumerates allowed axes; unused combinations are not compiled; variants resolved by bitmask at PSO lookup.
- **ShaderHotReload swap mid-frame** could cause visual pop. *Mitigation*: swap at frame boundary only; old PSO deferred-released per ADR-0007 quarantine.
- **Forward+ transparent overdraw at high particle count** is expensive. *Mitigation*: GPU particles (Phase 5) billboard at lower resolution bucket; depth-sorted batching; early-out via clustered light culling.
- **Missing resource transitions from FrameGraph** when user writes a custom pass. *Mitigation*: debug-build validation layer checks every `Read`/`Write` declaration against actual command-list usage.

## GDD Requirements Addressed

| GDD System | Requirement | How This ADR Addresses It |
|------------|-------------|--------------------------|
| (None — ADR-only project per ADR-0001) | N/A | TR-rnd-003 (Deferred + Forward+ hybrid pipeline) — elevated from Partial to Covered by this ADR. TR-chr-001 (Post-effects out-of-the-box) — covered by the canonical PostFX chain section. |

## Performance Implications
- **CPU**: Light-cluster build ≈ 0.3 ms/frame for 256 lights on mid CPU; FrameGraph compile ≈ 0.1 ms; parallel command-list recording across 4 workers ≈ 1.5 ms for a typical pass set
- **Memory**: GBuffer at 1080p ≈ 40 MB (4 RTs); transient aliasing reclaims ~30% of scratch; PostFX bounce buffers ≈ 20 MB at 1080p float16
- **Load Time**: PSO compile dominates first-load; `ShaderLibrary` caches PSOs to disk (Phase 4 behaviour); warm-start reaches first frame in <1 s for a typical scene
- **Network**: N/A

## Migration Plan

Not applicable — this ADR is retroactive across Phases 0–5. Going forward:

1. New material domains require a registry extension + forward-compat path
2. New PostFX effects must declare where in the canonical order they fit; insertion is an ADR-level decision
3. Changes to GBuffer layout require a superseding ADR
4. Ray tracing / GI additions extend this ADR, do not replace it

## Validation Criteria
- Scene render test: 10,000 opaque meshes + 500 transparent + 256 lights at 1080p renders in ≤ 13 ms on mid-range GPU (leaves headroom for PostFX)
- Barrier validation: enable the FrameGraph debug-validation layer; run the sample scene; zero manual-barrier violations
- HDR output: on capable display, `HDR10` output verified visually; color-grading LUT behaviour identical in `SDR_ACES` and `HDR10` up to tone-map
- DynamicResolution stability: scale 1.0 → 0.5 → 1.0 over 10 seconds; no flickering or UI degradation
- PostFX mask test: every combination of enabled effects produces visually-plausible output (no NaN, no undefined areas)
- Shader hot reload: modify a material HLSL; new PSO ready within 200 ms; frame not dropped; old PSO released after quarantine

## Related Decisions
- ADR-0001 (Documentation strategy)
- ADR-0002 (DX12 backend — commits to DX12-only rendering, HDR10/scRGB DXGI workflow, caps-gated feature fallback)
- ADR-0006 (Job System — parallel command-list recording, main-thread submit)
- ADR-0007 (Asset DB — shaders / materials / textures flow through AssetDatabase; ShaderHotReload rides the AssetReloader)
- `GXLib/Graphics/FrameGraph/`, `GXLib/Graphics/Pipeline/`, `GXLib/Graphics/PostEffect/`, `GXLib/Graphics/3D/ClusteredLighting.*`, `GXLib/Graphics/3D/CascadedShadowMap.*` (source of truth)
- CHANGELOG.md Phases 0, 1, 3, 4, 5
