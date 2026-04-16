# GXLib — Version Reference

| Field | Value |
|-------|-------|
| **Engine** | GXLib (self-hosted) |
| **Current Phase** | Phase 5 (2026-03-01) |
| **Target API Compatibility** | DXLib (DirectX 11 era) |
| **Backend** | DirectX 12 |
| **Language** | C++20 |
| **Platform** | Windows |
| **Project Pinned** | 2026-04-15 |
| **LLM Knowledge Cutoff** | May 2025 |
| **Risk Level** | LOW — DX12 / C++20 are within LLM training data; GXLib-specific APIs must be verified from source |

## About

GXLib is an internal, self-hosted game engine library. It provides a DXLib-compatible
procedural API on top of a modern DirectX 12 backend, so developers coming from DXLib
can adopt modern rendering features (HDR, VRS, Mesh Shaders, DirectStorage, etc.)
without reimplementing post-effects or screen effects from scratch.

Unlike the Godot/Unity/Unreal setup, there is no external engine documentation to
track. The authoritative reference is the source code under `GXLib/` and the
changelog under `CHANGELOG.md`.

## Phase Timeline

| Phase | Date | Highlights |
|-------|------|------------|
| Phase 0 | 2026-01-15 | DX12 framework, Math, 2D/3D rendering, XAudio2 |
| Phase 1 | 2026-02-01 | Gamepad vibration, NavMesh/RVO, Prefabs, Lua (sol2), GUI, GJK/EPA, Deferred rendering |
| Phase 2 | 2026-02-10 | EventBus, ObjectPool, SceneManager, Behavior Tree, Save/Load |
| Phase 3 | 2026-02-15 | Coroutines, Undo/Redo, Cloth, Node Graph editor, Networking foundation, GPU skinning |
| Phase 4 | 2026-02-21 | HDR, VRS, Mesh Shaders, Sampler Feedback, DirectStorage, Reliable UDP, ECS, Video recording |
| Phase 5 | 2026-03-01 | Character Controller, Ragdoll, Scene Persistence, GPU Particles, Texture Streaming, Lua bindings, Hot Reload, Job System, Audio DSP, IME |

See `CHANGELOG.md` at repo root for the full list.

## Subsystem Map

```
GXLib/
├── AI/          Behavior Tree, NavMesh, RVO
├── Audio/       XAudio2, OGG, spatial, DSP effects
├── Compat/      DXLib-compatible procedural API
├── Container/   Custom containers (PCH-free)
├── Core/        Application, Timer, EventBus, Scheduler, Save/Load, AssetDB, JobSystem
├── ECS/         Archetype-based World, Query, System
├── Editor/      Play-in-Editor, Undo/Redo, Node Graph, Reflection
├── GUI/         Widget system, ImGui integration, IME
├── GX/          Class-type API facade (gx::App)
├── Graphics/    DX12, PBR, Deferred, Post-FX, HDR, VRS, Mesh Shaders, GPU particles
├── IO/          FileWatcher, DirectStorage, Hot Reload
├── Input/       Keyboard/Mouse/Gamepad
├── Math/        Vector2/3/4, Quaternion, Matrix4x4, Color, Tween, Random
├── Movie/       Video playback and recording
├── Physics/     GJK/EPA, constraints, Verlet cloth, Ragdoll, Character Controller
├── Script/      Lua 5.4 + sol2 bindings
└── ThirdParty/  Vendored dependencies
```

## Verified Sources

- Source of truth: this repository's `GXLib/` source tree
- Release history: `CHANGELOG.md` (repo root)
- Sample usage: `GXModelViewer/` (reference application)

## Update Policy

Update this file when:
- A new Phase is released (add row to Phase Timeline)
- A major subsystem is added (update Subsystem Map)
- DX12 feature support changes (update Highlights)
