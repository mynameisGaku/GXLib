# GXLib Examples

Layer 1.5 samples — the bridge between the 35-line `template/main.cpp` and the 86 KLoC `GXModelViewer`. Reference: **ADR-0017 (Two-Layer Accessibility Pillar)**.

## What Each Sample Teaches

| # | Name | Layer | Teaches |
|---|------|-------|---------|
| 01 | hello-sprite | L1 | `LoadGraph`, `DrawRotaGraph`, mouse-follow, graceful asset fallback |
| 02 | hello-sound | L1 | `LoadSoundMem`, `PlaySoundMem`, `PlayMusic`, volume control |
| 03 | hello-input | L1 | Keyboard, mouse, gamepad, `ActionMapping` rebinding |
| 04 | hello-3d | L1 | `LoadModel`, `SetCameraPositionAndTarget`, orbit camera |
| 05 | custom-postfx | L2 | `SetPostFXMask`, `GetPostEffects()` — the Layer 2 entry pattern |
| 06 | custom-shader-model | L2 | `ShaderRegistry::RegisterCustomShaderModel` — custom HLSL shader as new material type |
| 07 | hello-ecs | L1.5 | `gx::ecs::World` — entities, components, `ForEach` query |
| 08 | hello-physics | L1.5 | `PhysicsWorld2D` — rigid bodies, raycast, fixed timestep |
| 09 | hello-animation | L1.5 | `PlayModelAnimation` — skeletal animation + clip switching |
| 10 | custom-asset-type | L2 | `AssetDatabase::RegisterType<T>` — custom asset type with hot reload |
| 11 | custom-audio-dsp | L2 | `IAudioEffect` — custom DSP effects on an AudioBus (threading-safe tremolo demo) |
| 12 | hello-gui | L1.5 | `gx::GetUIContext()` — real Widget tree (Panel/Button/TextWidget, onClick, layoutRect) |
| 13 | custom-widget | L2 | `gx::GUI::Widget` derivation — CircularGauge (RenderSelf + OnEvent override) |
| 14 | hello-network | L1 | `GX_StartServer` / `GX_Connect` / `GX_Broadcast` — simple chat demo |
| 15 | hello-physics3d | L2 | `PhysicsWorld3D` — CreateBoxShape + AddBody + Step + GetPosition |
| 16 | custom-ik | L2 | `LookAtIK` — head bone tracks mouse cursor (IK API pattern) |

**Layer 1** — DXLib-compatible procedural API only (`GXLib.h`).
**Layer 2** — Direct access to engine subsystems via accessor returns.

## Build

Two modes, pick whichever fits your workflow:

### Mode A: In-tree (recommended — builds SDK + all samples together)

From the repo root:

```
cmake -B build -S .
cmake --build build --config Debug
```

`GX_BUILD_EXAMPLES=ON` is the default. The generated Visual Studio solution
(`build/GXLib.sln`) groups all 16 samples under the `Examples/` folder.
Right-click any sample → **"Set as Startup Project"** → F5 to run.

To skip examples: `cmake -B build -S . -DGX_BUILD_EXAMPLES=OFF`

### Mode B: Standalone against an installed SDK

If you've already built and installed the SDK (`cmake --install ... --prefix GXLib-SDK`):

```
cmake -B build -S examples -DGXLib_DIR=path/to/GXLib-SDK/cmake
cmake --build build --config Debug
```

### Output

Each sample produces an executable `gxlib_example_NN`. The build system
automatically copies `Shaders/` next to the executable. VS debugging is
pre-configured to the correct working directory.

## Asset Fallbacks

Every sample handles missing assets gracefully (logs a warning + uses
placeholder). You can run them with no `Assets/` directory and still see
working code.

## Reference

- **ADR-0017** — Layer 1 / Layer 2 separation, samples requirement (L1.5)
- **ADR-0003** — DXLib Compat API layer (the Layer 1 surface used by 01–04)
- **ADR-0008** — Rendering Pipeline (the FrameGraph that sample 05 hints at)
