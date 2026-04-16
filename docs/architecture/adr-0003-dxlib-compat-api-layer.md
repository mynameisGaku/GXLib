# ADR-0003: DXLib-Compatible Procedural API Layer

## Status
Accepted

## Date
2026-04-15

## Engine Compatibility

| Field | Value |
|-------|-------|
| **Engine** | Custom — GXLib (self-hosted) |
| **Domain** | Core / Public API |
| **Knowledge Risk** | LOW — DXLib's public API is stable and well-documented; C++20 / MSVC features used are within the LLM training data |
| **References Consulted** | `docs/engine-reference/gxlib/VERSION.md`, `GXLib/Compat/` source tree (GXLib.h, CompatContext, Compat_2D/3D/Font/Input/Math/Particle/Sound/System) |
| **Post-Cutoff APIs Used** | None |
| **Verification Required** | Side-by-side diff of a sample DXLib program ported to GXLib; automated ABI test that forbidden types never appear in public headers (grep gate in CI) |

## ADR Dependencies

| Field | Value |
|-------|-------|
| **Depends On** | ADR-0001 (documentation strategy), ADR-0002 (DX12 backend — supplies the engine the Compat layer translates into) |
| **Enables** | Future ADRs on specific subsystem APIs (Input, Sound, Font, Particle) where the Compat signature shapes the underlying engine contract |
| **Blocks** | None (code already exists since Phase 0; this ADR is retroactive) |
| **Ordering Note** | Must be Accepted before any ADR that touches public API surface, since those ADRs must respect the Compat layer's contract |

## Context

### Problem Statement
GXLib's core value proposition is *"adopt DXLib's simple procedural API, but without being stuck in the DX11 era."* That requires a **compatibility layer** — a procedural C-style facade (free functions in `namespace gx`) that maps DXLib-shaped calls (`DrawGraph`, `DrawString`, `LoadSoundMem`, `GetJoypadInputState`, etc.) onto GXLib's modern DX12 engine. The layer has to balance two competing pulls:

1. **Tight DXLib compatibility** — so a developer can port a DXLib program by changing `#include <DxLib.h>` to `#include "GXLib.h"` and minimal rewriting.
2. **Not a straitjacket** — GXLib offers features DXLib doesn't (HDR, VRS, post-effects, PBR), and users must be able to reach them without fighting the compat layer.

The question is how faithful to be, where to deliberately diverge, and how to keep the layer from leaking DX12 internals to callers.

### Constraints
- Must live in `GXLib/Compat/` behind `GXLib/Compat/GXLib.h`
- Public header may not include `<d3d12.h>` or `<dxgi*.h>` (per ADR-0002 `dx12_type_in_public_header` forbidden pattern)
- C++20 toolchain; functions must be callable from plain C++ (no templates, no concepts in signatures) so the API feels DXLib-procedural
- Must coexist with the class-style API under `GXLib/GX/` (e.g., `gx::App`) — both APIs wrap the same underlying engine
- Japanese documentation is the default for DXLib users; all Doxygen on Compat functions is Japanese-first with optional English

### Requirements
- Provide DXLib's most-used functions with matching semantics: 2D drawing, 3D drawing, font/text, input (keyboard / mouse / gamepad), sound, system (init/end/ProcessMessage/ScreenFlip), math helpers
- Return codes follow DXLib convention: 0 = success, -1 = failure
- `int GX_Init()`, `int GX_End()`, `int ProcessMessage()` replace DXLib's `DxLib_Init()` / `DxLib_End()` / `ProcessMessage()` — name change is deliberate to prevent accidental symbol conflict when both libraries are linked during gradual porting
- Internal state held in a single `CompatContext` (singleton-per-module) that owns `GraphicsDevice`, font manager, sound manager, input manager references
- New-to-GXLib features (HDR, VRS, PostFX) are reachable either through added Compat functions (e.g., `SetPostFXMask`) or by dropping to the class API (`GX::App`, `GetPostEffects()`)

## Decision

**GXLib ships a DXLib-shaped procedural API in `gx::` namespace under `GXLib/Compat/`. It mimics DXLib's function signatures and return-code conventions wherever feasible, prefixes GXLib-unique system functions with `GX_`, and NEVER matches DXLib symbols verbatim in the global namespace.**

Concrete rules:

1. **All compat functions live in `namespace gx`.** No global-namespace symbols. A user writes `gx::DrawGraph(...)`, not `DrawGraph(...)`. This prevents ODR clashes if a project links both DxLib and GXLib during a porting transition, and makes call-site intent explicit in reviews.

2. **Function names mirror DXLib's where the semantic is identical.** `DrawGraph`, `DrawString`, `LoadGraph`, `LoadSoundMem`, `PlaySoundMem`, `GetJoypadInputState` — names match. Parameter order and return-code conventions match (0 success, -1 failure). **Exception**: init/shutdown use `GX_Init()` / `GX_End()` instead of `DxLib_Init()` / `DxLib_End()` — see Requirements.

3. **All state is held in a single `gx::CompatContext`** (file-static singleton). Functions forward to it. The context lazily acquires `GraphicsDevice`, `FontManager`, `SoundManager`, `InputManager` on first use and releases them on `GX_End()`.

4. **Return-code semantics match DXLib.** Functions return `int`: 0 on success, -1 on failure. No exceptions across the API boundary. No `std::expected`. Error details available via a separate `gx::GetLastError()` query.

5. **GXLib-exclusive features are either added procedural functions** (e.g., `gx::SetPostFXMask(PostFXMask)`, `gx::EnableHDROutput(bool)`, `gx::SetVRSTier(int)`) **or reached via the class API.** The compat layer does NOT pretend these map to DXLib concepts.

6. **No DX12 types in Compat headers.** Enforced by CI grep gate (see ADR-0002).

7. **Types used at the Compat boundary live in `Compat/CompatTypes.h`:** integer handles (`int graphHandle`, `int soundHandle`), plain structs (`FontInfo`), and enums. No PIMPL leakage, no `std::shared_ptr<IDevice>`.

8. **Documentation convention: Doxygen in Japanese** on Compat functions (matches existing Phase 0–5 style). English translations may be added as `@english` tags but are not required.

### Architecture Diagram

```
User code
   │  #include "GXLib.h"
   │  gx::GX_Init(); gx::DrawGraph(...); gx::GX_End();
   ▼
GXLib/Compat/
   ├─ GXLib.h           (public procedural API, "namespace gx" free functions)
   ├─ CompatTypes.h     (int handles, plain structs, enums — no DX12 types)
   ├─ CompatContext.h   (singleton owning engine-side managers)
   └─ Compat_{2D,3D,Font,Input,Math,Particle,Sound,System}.cpp
         │
         ▼
GXLib/Graphics/, /Audio/, /Input/, /Graphics/Text/  (engine subsystems)
         │
         ▼
GXLib/Graphics/Device/  (DX12 boundary — ADR-0002)
```

### Key Interfaces
- `int gx::GX_Init();` — replaces `DxLib_Init()`
- `int gx::GX_End();` — replaces `DxLib_End()`
- `int gx::ProcessMessage();` — DXLib-identical name and semantics
- `int gx::DrawGraph(int x, int y, int graphHandle, int transFlag);` — DXLib-identical
- `int gx::LoadGraph(const char* path);` — returns opaque int handle; -1 on failure
- `int gx::SetPostFXMask(PostFXMask mask);` — GXLib-exclusive; not in DXLib
- `gx::CompatContext& gx::GetCompatContext();` — not part of public API but used internally
- `int gx::GetLastError();` — DXLib-style error retrieval

## Alternatives Considered

### Alternative 1: Verbatim DXLib compatibility (global-namespace, identical names)
- **Description**: Export `DrawGraph`, `DxLib_Init`, etc. as global-namespace symbols so a DXLib program compiles unchanged after swapping the header include
- **Pros**: Absolute minimum porting effort — change one `#include` and rebuild
- **Cons**: ODR clashes if the user's project transitively links both DxLib and GXLib during a porting transition; no way for call-site reviewers to see "this is GXLib, not DxLib"; forces GXLib-exclusive functions into the global namespace too, polluting the symbol table
- **Rejection Reason**: The porting-ease gain is small (one `using namespace gx;` or namespace qualifier per call site) and the cost — symbol collision, review opacity — is permanent

### Alternative 2: Class API only (no procedural compat layer)
- **Description**: Drop the Compat/ layer entirely; require all users to adopt `gx::App` and the class-style API
- **Pros**: One coherent API; less surface area to maintain
- **Cons**: Abandons DXLib users — the core value proposition. Forces a rewrite, not a port. Adoption dies at the door.
- **Rejection Reason**: Defeats the project's core positioning

### Alternative 3: Thin aliasing (Compat calls are one-liners into the class API)
- **Description**: Make `gx::DrawGraph` literally just `GetCompatContext().app.GetRenderer2D().DrawGraph(...)` with no logic in between
- **Pros**: Zero duplication; everything goes through one code path
- **Cons**: Loses the opportunity to make DXLib semantics (int handle tables, error-code conventions) cleanly translate to the class API's richer types; makes DXLib quirks leak into the class API, or forces the class API to offer DXLib-shaped overloads
- **Rejection Reason**: The Compat layer legitimately needs its own translation logic (handle tables, error-code conversion, PostFX mask passthrough) — making it a pure alias would push that complexity into the class API where it doesn't belong

## Consequences

### Positive
- DXLib users port with minimal friction: `#include "GXLib.h"` + add `gx::` namespace prefix (or `using namespace gx;`)
- GXLib-exclusive features reachable without breaking DXLib semantics
- Namespace-scoped symbols prevent ODR hazards during gradual porting
- Two APIs (procedural Compat + class API) can evolve independently — class API can adopt modern C++ (expected, optional, span) while Compat stays C-shaped

### Negative
- Two API surfaces to maintain; features must be considered for both
- DXLib's return-code + error-query pattern is dated; modern code should prefer the class API
- Compat layer documentation is Japanese-first — English-speaking contributors may need translation help
- Thin risk of Compat and class-API drift (e.g., different defaults for the same setting)

### Risks
- **Users may expect 100% DXLib source compatibility and be surprised by `GX_Init`/namespace differences.** *Mitigation*: `GXLib/Compat/GXLib.h` opens with a Japanese header comment explaining the delta; docs include a DXLib→GXLib porting table.
- **Symbol diff between DXLib versions moves as DXLib itself updates.** *Mitigation*: target a specific DXLib API snapshot version; document it in this ADR's Related Decisions when a version is pinned.
- **Compat functions that require engine state (e.g., graph handles) leak lifetime complexity.** *Mitigation*: `CompatContext` owns all handle tables; `GX_End()` frees them deterministically.

## GDD Requirements Addressed

| GDD System | Requirement | How This ADR Addresses It |
|------------|-------------|--------------------------|
| (None — ADR-only project per ADR-0001) | N/A | Project charter requirement: "DXLib users can migrate to GXLib with minimal effort" — satisfied by the procedural Compat layer |

## Performance Implications
- **CPU**: Compat functions are thin forwarders; expected overhead ≤ 50 ns per call (negligible vs. draw cost)
- **Memory**: `CompatContext` adds one singleton; handle tables grow with live assets (bounded by `GX_End()`)
- **Load Time**: Unchanged — `GX_Init()` delegates to the same engine init as the class API
- **Network**: N/A

## Migration Plan

Not applicable — this ADR is retroactive. The Compat layer has existed since Phase 0 and is used by sample code in `GXModelViewer/` and elsewhere. This ADR records the existing contract.

Going forward:
1. New DXLib-shaped functions added to Compat must be listed in `GXLib/Compat/GXLib.h` with a Doxygen comment cross-referencing the DXLib equivalent
2. GXLib-exclusive functions use a non-DXLib name (prefix with `GX_` for system-level, or use a clearly GXLib-only verb like `SetPostFXMask`)
3. Any change to Compat function semantics is a breaking change and requires a new ADR or an amendment to this one

## Validation Criteria
- CI grep gate: no `<d3d12.h>` or `<dxgi*.h>` in `GXLib/Compat/*.h`
- CI grep gate: no non-`gx::` public function in `GXLib/Compat/GXLib.h`
- Port test: a small DXLib sample (hello-triangle + sound + input) ports to GXLib with only namespace / init-function changes and runs with identical visual output
- Performance test: Compat call overhead ≤ 50 ns measured via microbenchmark

## Related Decisions
- ADR-0001 (Documentation strategy — justifies why this is an ADR, not a GDD)
- ADR-0002 (DX12 backend — supplies the engine the Compat layer wraps; forbids DX12 type leaks that this ADR also enforces)
- `GXLib/Compat/GXLib.h` (source of truth for the Compat API surface)
- CHANGELOG.md Phase 0 (Compat layer initial implementation)
