# Technical Preferences

<!-- Populated by /setup-engine. Updated as the user makes decisions throughout development. -->
<!-- All agents reference this file for project-specific standards and conventions. -->

## Engine & Language

- **Engine**: Custom — GXLib (self-hosted DX12 engine, DXLib-compatible API)
- **Language**: C++20
- **Rendering**: DirectX 12 (HDR output, VRS, Mesh Shaders, Sampler Feedback, DirectStorage)
- **Physics**: GXLib internal (GJK/EPA collision, 6-type constraints, Verlet cloth, ragdoll)

## Input & Platform

<!-- Written by /setup-engine. Read by /ux-design, /ux-review, /test-setup, /team-ui, and /dev-story -->
<!-- to scope interaction specs, test helpers, and implementation to the correct input methods. -->

- **Target Platforms**: PC (Windows)
- **Input Methods**: Keyboard/Mouse, Gamepad
- **Primary Input**: Mixed (library supports all — game projects choose per title)
- **Gamepad Support**: Full (including vibration)
- **Touch Support**: None
- **Platform Notes**: Windows 専用、DirectX 12 必須、IME (IMM32) 対応あり。コンソール/モバイル/Web は非対応。

## Naming Conventions

- **Namespace**: `gx`
- **Classes**: PascalCase (e.g., `Application`, `PlayerController`)
- **Methods / Free Functions**: PascalCase (e.g., `Initialize()`, `TakeDamage()`)
- **Member Variables**: `m_camelCase` (e.g., `m_window`, `m_running`)
- **Local Variables / Struct Fields**: camelCase (e.g., `title`, `width`, `updateCallback`)
- **Signals/Events**: Callback-style `std::function<...>`, names in PascalCase past tense (e.g., `OnHealthChanged`)
- **Files**: PascalCase matching class (e.g., `Application.h` / `Application.cpp`)
- **Scenes/Prefabs**: N/A — GXLib uses Asset Database resources (JSON or binary)
- **Constants**: UPPER_SNAKE_CASE (e.g., `MAX_HEALTH`)
- **Doc Comments**: Doxygen `///` on public APIs (日本語可)

## Performance Budgets

- **Target Framerate**: 60 fps
- **Frame Budget**: 16.6 ms
- **Draw Calls**: [TO BE CONFIGURED — DX12 indirect draws preferred]
- **Memory Ceiling**: [TO BE CONFIGURED]

## Testing

- **Framework**: GXLib custom test runner (`tests/`)
- **Current Size**: ~197 test files, ~1,100+ tests (as of Phase 5, 2026-03-01)
- **Minimum Coverage**: [TO BE CONFIGURED]
- **Required Tests**: Core subsystems (Graphics, Physics, ECS, Audio, IO, Math) — all public APIs must be unit-testable

## Forbidden Patterns

<!-- Add patterns that should never appear in this project's codebase -->
- [None configured yet — add as architectural decisions are made]

## Allowed Libraries / Addons

<!-- Only add libraries that are actively integrated. -->
- DirectX 12 (Windows SDK)
- XAudio2 (audio backend)
- Lua 5.4 + sol2 (scripting)
- ImGui (editor / debug UI — GXModelViewer)
- FBX / glTF loaders (model import)

## Architecture Decisions Log

<!-- Quick reference linking to full ADRs in docs/architecture/ -->
- [No ADRs yet — use /architecture-decision to create one. Recommended first ADRs: DX12 backend choice, DXLib API compatibility layer, ECS architecture (archetype-based), Lua scripting boundary, Job System design.]

## Engine Specialists

<!-- GXLib is itself the engine. The Godot/Unity/Unreal engine specialists DO NOT apply. -->
<!-- Route work to the core programming agents instead. -->

- **Primary**: engine-programmer (GXLib core subsystems)
- **Language/Code Specialist**: engine-programmer (C++20 review — primary covers it)
- **Shader Specialist**: technical-artist (HLSL shaders in `Shaders/`)
- **UI Specialist**: ui-programmer (GXLib `GUI/` module + ImGui integration)
- **Additional Specialists**: performance-analyst (profiling, frame budget), gameplay-programmer (consumer-facing API, DXLib compat layer), technical-director (cross-subsystem architecture)
- **Routing Notes**: This project IS the engine. Engine-internal work → engine-programmer. Public-facing API (`Compat/`, `GX/App.h`) → gameplay-programmer. Rendering internals and HLSL → technical-artist. Cross-cutting architecture decisions → technical-director. Godot/Unity/Unreal specialists are unused.

### File Extension Routing

<!-- Skills use this table to select the right specialist per file type. -->

| File Extension / Type | Specialist to Spawn |
|-----------------------|---------------------|
| Engine core (`GXLib/**/*.h`, `GXLib/**/*.cpp`) | engine-programmer |
| HLSL shaders (`Shaders/*.hlsl`, `*.hlsli`) | technical-artist |
| GUI module (`GXLib/GUI/**`) | ui-programmer |
| DXLib compat layer (`GXLib/Compat/**`) | gameplay-programmer |
| Sample applications (`GXModelViewer/**`) | gameplay-programmer |
| Tests (`Tests/**`, `tests/**`) | engine-programmer |
| Build / CMake | devops-engineer |
| General architecture review | technical-director |
