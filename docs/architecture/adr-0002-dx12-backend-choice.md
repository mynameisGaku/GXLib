# ADR-0002: DirectX 12 Rendering Backend

## Status
Accepted

## Date
2026-04-15

## Engine Compatibility

| Field | Value |
|-------|-------|
| **Engine** | Custom — GXLib (self-hosted) |
| **Domain** | Rendering |
| **Knowledge Risk** | LOW — DX12 API is within the LLM training data; DXR 1.1, Mesh Shaders, Sampler Feedback, and DirectStorage 1.2+ are widely documented |
| **References Consulted** | `docs/engine-reference/gxlib/VERSION.md`, `GXLib/Graphics/` source tree, `CHANGELOG.md` Phases 0/4 |
| **Post-Cutoff APIs Used** | None — all target features (HDR, VRS Tier 1/2, Mesh Shaders, Sampler Feedback, DirectStorage) predate the LLM cutoff |
| **Verification Required** | Feature-level capability checks at runtime (`CheckFeatureSupport`) before enabling VRS / Mesh Shaders / Sampler Feedback on end-user hardware |

## ADR Dependencies

| Field | Value |
|-------|-------|
| **Depends On** | ADR-0001 (documentation strategy — establishes that this is an ADR, not a GDD) |
| **Enables** | Future ADRs on DXLib compatibility layer, post-processing pipeline, GPU particles, texture streaming, ray tracing, RT shadows, DirectStorage asset loading |
| **Blocks** | None (code already exists since Phase 0; this ADR is retroactive) |
| **Ordering Note** | Foundational. Every rendering/GPU ADR depends on this one. |

## Context

### Problem Statement
DXLib (the API GXLib emulates) is built on DirectX 11, which does not expose modern GPU features that are now expected of a 2020s game engine: HDR output, Variable Rate Shading, Mesh Shaders, Sampler Feedback, DirectStorage, hardware ray tracing, and explicit multi-queue scheduling. Implementing post-effects, screen effects, and GPU-driven pipelines on top of DX11 is painful enough that game developers routinely reinvent them per project. GXLib's core value proposition is **"adopt DXLib's simple procedural API, but without being stuck in the DX11 era."** That requires a rendering backend that unlocks the modern feature set while hiding the ceremony from the game-side caller.

### Constraints
- Windows-only target (no need for cross-platform rendering abstraction)
- Must support a DXLib-compatible procedural API (`DrawGraph`, `DrawString`, etc.) without forcing users to manage command lists, descriptor heaps, or fences
- C++20 toolchain (MSVC and Clang-cl)
- Must run on commodity consumer GPUs (GeForce 10-series and later, Radeon RX 400 and later as floor) — not just high-end hardware
- Already 784 source files of Graphics code built against this choice (Phase 0 shipped DX12 framework; Phases 1–5 added PBR, Deferred, HDR, VRS, Mesh Shaders, DirectStorage, GPU particles, texture streaming)

### Requirements
- 60 fps at 1080p on mid-range consumer hardware with the full Phase 5 feature set enabled
- Support all of: HDR10 / scRGB output, VRS Tier 1 and Tier 2, Mesh Shader pipeline, Sampler Feedback, DirectStorage, DXR 1.1 ray tracing (where hardware allows)
- Deferred + Forward+ hybrid pipeline
- Graceful feature fallback when hardware lacks a capability (VRS off → full-rate; Mesh Shaders off → legacy vertex pipeline; DirectStorage off → standard I/O)
- Keep public GXLib headers free of `<d3d12.h>` — backend details must not leak to game code

## Decision

**GXLib uses DirectX 12 (Feature Level 12_1 minimum, 12_2 where available) as its sole rendering backend on Windows.**

- A thin `GraphicsDevice` abstraction (in `GXLib/Graphics/Device/`) owns the `ID3D12Device`, queues, swap chain, and descriptor heap allocator. The abstraction does NOT attempt cross-API portability — it exists to provide a stable seam for future changes and to keep DX12 types out of consumer headers.
- The public procedural API (`Compat/`) and the class API (`GX/App.h`) use forward-declared handles; game code never includes `<d3d12.h>` transitively.
- Feature gating is runtime: at startup `GraphicsDevice::QueryCapabilities()` fills a `GraphicsCaps` struct (VRS tier, Mesh Shader support, Sampler Feedback support, DirectStorage availability, DXR support). Systems consult caps before enabling optional paths.
- The pipeline architecture is **Deferred rendering by default**, with Forward+ as an opt-in path for transparency-heavy scenes (see Phase 1 CHANGELOG).
- DX12 resource creation funnels through the `Resource/` module; descriptor heap management funnels through the `Device/` module; command recording funnels through `ParallelRenderQueue` and `FrameGraph` (see Phase 4 additions).
- No Vulkan, no DX11 fallback, no OpenGL. Scope is explicitly Windows + DX12.

### Architecture Diagram

```
Game code (user)
   │
   ▼
Compat/ (DXLib-compatible procedural API)          GX/App.h (class API)
   │                                                     │
   └──────────────┬──────────────────────────────────────┘
                  ▼
         GXLib/Graphics/  (public-facing subsystems)
           ├─ 3D/            (Animator, PBR, Deferred)
           ├─ PostEffect/    (HDR, tone-mapping, bloom, SSR, etc.)
           ├─ Pipeline/      (Forward+, Deferred, Shadow)
           ├─ FrameGraph/    (render-pass graph)
           ├─ Rendering/     (draw command construction)
           ├─ RayTracing/    (DXR 1.1)
           └─ Text/, Layer/, Resource/
                  │
                  ▼
         GXLib/Graphics/Device/   ←── DX12 abstraction seam
           ├─ GraphicsDevice     (ID3D12Device, queues, descriptors)
           ├─ GraphicsCaps       (runtime capability struct)
           ├─ CommandContext     (command list recording)
           └─ SwapChain          (DXGI swap chain + HDR)
                  │
                  ▼
         Direct3D 12 / DXGI (Windows SDK)
```

### Key Interfaces
- `gx::GraphicsDevice` — sole owner of `ID3D12Device`; non-copyable singleton per application.
- `gx::GraphicsCaps` — runtime feature struct returned by `GraphicsDevice::GetCaps()`; includes `vrsTier`, `supportsMeshShader`, `supportsSamplerFeedback`, `supportsDirectStorage`, `dxrTier`.
- Forward-declared public handles: `gx::TextureHandle`, `gx::ShaderHandle`, `gx::PipelineHandle` — opaque IDs, no DX12 types exposed.
- Shaders: HLSL in `Shaders/` compiled with DXC (SM 6.0 baseline, SM 6.5+ for Mesh Shaders / Ray Tracing).

## Alternatives Considered

### Alternative 1: Stay on DirectX 11 (mimic DXLib more literally)
- **Description**: Implement GXLib on DX11 for a tighter DXLib semantic match
- **Pros**: Simpler per-call overhead; familiar resource model; existing DXLib developers map 1:1
- **Cons**: No HDR output, no VRS, no Mesh Shaders, no DirectStorage, no Sampler Feedback, no bindless resources, no DXR. **This is the very constraint DXLib imposes that GXLib exists to escape.** Also: DX11 is in maintenance mode; Microsoft has shifted investment to DX12
- **Rejection Reason**: Defeats the project's core value proposition

### Alternative 2: Vulkan
- **Description**: Use Vulkan as the rendering backend
- **Pros**: Cross-platform (Linux, Android); SPIR-V tooling ecosystem
- **Cons**: GXLib has declared Windows-only scope, so portability is unused; Vulkan on Windows has weaker vendor driver support than DX12 for features like DirectStorage (GPU decompression) and Sampler Feedback; HDR scRGB output is more mature on DXGI than on Vulkan-on-Windows
- **Rejection Reason**: No portability benefit given Windows-only scope, while losing Windows-specific feature maturity

### Alternative 3: Dual backend (DX12 + Vulkan)
- **Description**: Ship both backends behind a render abstraction
- **Pros**: Futureproof for non-Windows targets; more rigorous abstraction forces cleaner internal seams
- **Cons**: ~2× the rendering maintenance cost; every new feature has to be implemented twice; testing matrix doubles; the "seam" becomes a leaky abstraction that blocks aggressive DX12-specific optimization (bindless, work graphs, mesh nodes)
- **Rejection Reason**: Scope creep — GXLib's 784 files are aggressive in using DX12 specifics (FrameGraph, GPU-driven pipelines). Maintaining parity on Vulkan is a second project

## Consequences

### Positive
- Full access to the modern DX12 feature set (HDR, VRS, Mesh Shaders, Sampler Feedback, DirectStorage, DXR) — unlocking every CHANGELOG Phase 4/5 feature
- Single-backend focus means 100% of rendering engineering time moves the Windows experience forward
- DXGI integration gives mature HDR output, flip-model swap chains, and tearing control
- DXC + HLSL 6.x is a well-supported toolchain with active Microsoft investment

### Negative
- Windows-only. macOS/Linux/Console are out of scope and require a separate backend (future project, not GXLib)
- DX12 has higher API surface area than DX11; the internal `Graphics/Device/` layer is responsible for absorbing that complexity so users never see it
- Feature fallback paths (VRS off, Mesh Shaders off) are tested less frequently than the happy path; need CI coverage on a low-feature device profile
- DirectX 12 requires Windows 10 1803+ (practically: Windows 10 2004+ for Mesh Shaders, Windows 11 for Agility SDK runtime updates)

### Risks
- **Consumer GPUs without Mesh Shader support exist (older GTX 10-series, RX 400/500).** *Mitigation*: runtime caps check + legacy vertex pipeline fallback in `Pipeline/`.
- **DirectStorage requires NVMe + DirectStorage-compatible driver; not all users have it.** *Mitigation*: `IO/` module already falls back to `ReadFile` when DS is unavailable (Phase 4).
- **HDR output behaves differently per OS version and display stack.** *Mitigation*: verify on at least Windows 10 21H2 and Windows 11 23H2; gate HDR behind user-opt-in setting.
- **Agility SDK version drift.** *Mitigation*: pin Agility SDK version in `ThirdParty/`; bump deliberately per Phase, not implicitly.

## GDD Requirements Addressed

| GDD System | Requirement | How This ADR Addresses It |
|------------|-------------|--------------------------|
| (None — ADR-only project per ADR-0001) | N/A | Requirements sourced from project charter: "modern GPU features without DXLib-era friction" — satisfied by DX12 + caps-gated feature set |

## Performance Implications
- **CPU**: Lower per-draw overhead than DX11 via parallel command-list recording (`ParallelRenderQueue`); frame submission budget target ≤ 2 ms on mid-range CPUs
- **Memory**: Higher baseline (~150 MB) due to descriptor heaps, pipeline state objects, and Agility SDK; acceptable on 8 GB systems
- **Load Time**: Faster with DirectStorage (GPU decompression); PSO caching mandatory to avoid first-frame hitches
- **Network**: N/A (rendering ADR)

## Migration Plan

Not applicable — this ADR is retroactive. DX12 was chosen in Phase 0 (2026-01-15) and has been the backend since. This ADR records the existing state and makes the decision explicit for future ADRs to reference.

Going forward:
1. Any new rendering ADR must state its DX12 feature-level requirement and fallback plan.
2. Any proposal to add a second backend (Vulkan, Metal, etc.) must supersede this ADR explicitly.

## Validation Criteria
- All Phase 0–5 features ship and run on reference hardware (tested per Phase in CI)
- `GraphicsCaps` correctly reports feature availability on ≥3 GPU tiers (high-end, mid-range, minimum-spec)
- No `<d3d12.h>` leaks into any public header (grep gate in CI)
- Frame time ≤ 16.6 ms at 1080p with full pipeline on mid-range GPU

## Related Decisions
- ADR-0001 (Documentation strategy)
- CHANGELOG.md Phase 0 (initial DX12 framework)
- CHANGELOG.md Phase 4 (HDR, VRS, Mesh Shaders, Sampler Feedback, DirectStorage)
- `docs/engine-reference/gxlib/VERSION.md`
