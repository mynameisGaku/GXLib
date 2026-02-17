# Project Analysis: GXLib (Current State - 2026-02-11)

## 1. Project Overview and Philosophy

GXLib is an ambitious C++20 game development framework designed as a complete, superior replacement for the existing DxLib, specifically targeting DirectX 12. Its core philosophy emphasizes functionality, performance, and ease of use, while avoiding excessive abstraction. The project prioritizes creating a correctly functioning system over immediate optimization, and maintains strong compatibility with DxLib's API where appropriate to ease migration. The development process is documented through detailed phase summaries, reflecting a structured and iterative approach.

## 2. Core Technologies and Standards

*   **Language**: C++20
*   **API**: DirectX 12
*   **Build System**: CMake (targeting Visual Studio 2022 v145 toolset)
*   **Shader Compiler**: Windows SDK DXC (dxcompiler.dll)
*   **Platform**: Windows
*   **Coding Standards**:
    *   **Namespace**: `GX::` (often nested, e.g., `GX::GUI::`)
    *   **Naming Conventions**: PascalCase for classes, methods, and types; `m_camelCase` for member variables.
    *   **Character Encoding**: `/utf-8` for source files (supports Japanese comments).
    *   **PCH**: `pch.h` for common headers (Windows, DX12, STL, DirectXMath).

## 3. Architectural Overview

GXLib is structured into several modular subsystems, each responsible for a specific aspect of the engine. Key modules include Core, Graphics (further subdivided into Device, Pipeline, Resource, Rendering, 3D, Layer, PostEffect), GUI, Input, Audio, IO, Math, and Physics. A compatibility layer provides DxLib-like APIs, and a comprehensive ShaderLibrary and ShaderHotReload system support shader management and runtime iteration.

## 4. Key Features and Implementations

### Core
*   **Application Lifecycle**: `Application` class manages `Initialize` → `Run` → `Shutdown`.
*   **Logging**: `Logger` with level-based output (Info/Warn/Error).
*   **Timing**: `Timer` using `QueryPerformanceCounter` for high-precision delta time and FPS calculation.
*   **Window Management**: `Window` class for Win32 window creation, message handling, and resize callbacks.

### Graphics
*   **DirectX 12 Foundation**: `GraphicsDevice`, `CommandQueue`, `SwapChain`, `DescriptorHeap`, `Fence`, `CommandList` for low-level D3D12 operations.
*   **Shader Management**: `Shader` class for HLSL compilation via DXC, `RootSignatureBuilder`, `PipelineStateBuilder`.
*   **Resource Management**:
    *   **Buffers**: `Buffer` (vertex/index) and `DynamicBuffer` (UPLOAD heap for CPU-GPU shared data).
    *   **Textures**: `Texture` (stb_image for loading, GPU transfer), `TextureManager` (handle-based, caching).
    *   **Render Targets**: `RenderTarget` (off-screen rendering), `DepthBuffer`.
    *   **Soft Image**: `SoftImage` for CPU-side pixel manipulation.
*   **2D Rendering**:
    *   `SpriteBatch`: Efficient batched sprite rendering with various blend modes.
    *   `PrimitiveBatch`: Batched drawing of basic shapes (lines, boxes, circles, triangles).
    *   `Camera2D`: 2D camera with position, zoom, and rotation.
    *   `SpriteSheet`, `Animation2D`: For sprite animation.
*   **3D Rendering**:
    *   **PBR**: Physically Based Rendering with Cook-Torrance BRDF.
    *   **Lighting**: Directional, Point, Spot lights.
    *   **Shadows**: Cascaded Shadow Maps (CSM), Spot Shadow Maps, Point Shadow Maps (Texture2DArray).
    *   **Models**: glTF 2.0 loader (`cgltf.h`), skeletal animation (`Skeleton`, `AnimationClip`, `AnimationPlayer`).
    *   `Skybox`: Procedural skybox rendering.
    *   `Fog`: Linear, Exp, Exp2 modes.
    *   `PrimitiveBatch3D`: Debug wireframe primitives.
    *   `Terrain`: Heightmap-based terrain.
*   **Post-Processing Pipeline**: Comprehensive HDR-enabled chain with ping-pong render targets and LDR output.
    *   `HDR Pipeline`: R16G16B16A16_FLOAT render targets.
    *   `Tonemapping`: Reinhard, ACES, Uncharted2 (movable to post-effect pipeline).
    *   `Bloom`, `FXAA`, `Vignette`, `Chromatic Aberration`, `Color Grading`.
    *   `SSAO` (Screen Space Ambient Occlusion) with bilateral blur.
    *   `Depth of Field` (DoF) with CoC generation and Gaussian blur.
    *   `Motion Blur` (camera-based, depth reprojection).
    *   `Screen Space Reflections` (SSR) with ray marching and normal reconstruction from depth.
    *   `OutlineEffect` (Sobel edge detection from depth/normal).
    *   `Volumetric Light` (God Rays) using radial blur and depth occlusion.
    *   `TAA` (Temporal Anti-Aliasing) with Halton sequence jitter and variance clipping.
    *   `AutoExposure` (Eye Adaptation) using log luminance and exponential smoothing.
*   **Rendering Layers**: `RenderLayer` system for managing independent LDR layers (e.g., Scene, UI) with Z-order composition, blend modes, and masking. `LayerCompositor` for final output.

### GUI
*   **Widget System**: Base `Widget` class with tree structure, event handling (Capture → Target → Bubble), layout (`Flexbox`).
*   **UI Elements**: `Panel`, `TextWidget`, `Button`, `Slider`, `CheckBox`, `RadioButton`, `Dialog`, `TextInput`, `Image`, etc.
*   **Styling**: `StyleSheet` for CSS parsing (kebab-case to camelCase), supporting pseudo-classes (`:hover`, `:pressed`).
*   **Rendering**: `UIRenderer` integrates SDF rounded rectangles (`UIRectBatch`), `SpriteBatch`, `TextRenderer`, and scissor stacking.
*   **Declarative UI**: `XMLParser` and `GUILoader` for defining UI layouts from XML files, binding to C++ events.

### Input
*   **Unified Input Management**: `InputManager` integrates `Keyboard`, `Mouse`, and `Gamepad` (XInput).
*   **Polling Model**: Updates input states each frame, supporting `press`, `trigger`, `release`.
*   **Event-driven**: Window message callbacks for input processing.

### Audio
*   **XAudio2 Backend**: `AudioDevice` for XAudio2 engine initialization.
*   **Sound Management**: `Sound` (WAV data), `SoundPlayer` (SE playback with multiple voices), `MusicPlayer` (BGM with looping, fade-in/out).
*   `AudioManager`: Handle-based management for sounds and music.

### IO
*   **Virtual File System (VFS)**: Mount-based `FileSystem` with `IFileProvider` interface, allowing priority-based access to physical files or archives.
*   **Encrypted Archives**: Custom `.gxarc` format with LZ4 compression and AES-256 encryption.
*   **Asynchronous Loading**: `AsyncLoader` for background asset loading.
*   **File Watching**: `FileWatcher` for monitoring file system changes.
*   **Networking**: `TCPSocket`, `UDPSocket`, `HTTPClient` (WinHTTP), `WebSocket` (WinHTTP WebSocket API).

### Math / Physics
*   **Custom Math Library**: `Vector2/3/4`, `Matrix4x4`, `Quaternion`, `Color` types inheriting from DirectXMath's `XMFLOAT` types for zero-overhead interoperability, with operator overloads and utility methods. `MathUtil` for common functions, `Random` (Mersenne Twister).
*   **2D/3D Collision Detection**: `Collision2D`/`Collision3D` with various shapes (AABB, Circle, Sphere, OBB, Triangle), SAT (Separating Axis Theorem), Moller-Trumbore ray-triangle intersection, raycasting.
*   **Spatial Partitioning**: Template-based `Quadtree`, `Octree`, `BVH` (with SAH splits) for efficient spatial queries.
*   **2D Physics**: Custom impulse-based `PhysicsWorld2D` with `RigidBody2D` (gravity, collision response, friction).
*   **3D Physics**: Integration of Jolt Physics v5.3.0 via PIMPL wrapper (`PhysicsWorld3D`) for robust 3D simulation, body/shape management, and raycasting.

### DXLib Compatibility Layer
*   `GXLib.h`: A single include provides a set of global functions mimicking DxLib's API for 2D/3D drawing, input, audio, and font functions.
*   `CompatContext`: Singleton managing the internal GXLib subsystems used by the compatibility layer.
*   `ActiveBatch` auto-switching for `SpriteBatch`/`PrimitiveBatch` to optimize draw calls.

### Developer Tools
*   **Shader Hot Reload**: Monitors shader file changes, debounces, flushes GPU, and triggers PSO rebuilds at runtime (`F9` toggle). Provides error overlay for compilation failures.
*   **PSO Rebuilder**: A mechanism for various renderers to rebuild their Pipeline State Objects dynamically.
*   **JSON Settings**: `nlohmann/json` integration for loading/saving post-effect configurations to `post_effects.json` (`F12` save).

## 5. Strengths of the Project

*   **Comprehensive Features**: Covers a vast range of engine functionalities from low-level graphics to high-level physics, GUI, and networking.
*   **Modern API Usage**: Leverages DirectX 12 and C++20 for performance and modern programming practices.
*   **Modular and Layered Architecture**: Promotes maintainability and extensibility.
*   **Detailed Documentation**: Extensive `PhaseX_Summary.md` files provide clear insights into design decisions, issues, and verification for each stage of development.
*   **Developer Experience**: Features like Shader Hot Reload, DxLib compatibility, and JSON settings significantly enhance iteration speed and ease of use.
*   **Performance Focus**: Batch rendering, double-buffering for dynamic resources, optimized allocators (FrameAllocator, PoolAllocator), and careful post-effect chain management demonstrate a strong emphasis on performance.
*   **Robust Physics**: Integration of Jolt Physics for 3D and a custom impulse-based 2D engine provide solid physics foundations.

## 6. Future Work / Known Limitations

As per `Phase10_RemainingWork.md`:
*   Multi-threaded CommandList recording.
*   Texture streaming (mip splitting + LRU).
*   GPU regression testing (screenshot comparison).
*   Memory leak detection (CRT Debug Heap + Live Objects).
*   Further D3D12 Debug Layer options.

## 7. Coding Conventions and Style

*   **Namespace**: Strictly adheres to `GX::` namespace, often with sub-namespaces for modules (e.g., `GX::GUI`).
*   **Naming**: PascalCase for class, struct, enum names, and public methods. `m_camelCase` for member variables.
*   **Comments**: Extensive use of Japanese comments, especially "beginner-friendly explanations" within classes, indicating a focus on clarity and education. The `/utf-8` compile option supports this.
*   **C++ Modernity**: Embraces C++20 features, smart pointers, and RAII where appropriate.

---
**Analysis Date**: 2026-02-11
