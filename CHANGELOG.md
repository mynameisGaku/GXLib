# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

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
