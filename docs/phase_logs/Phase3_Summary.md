# Phase 3: 3D Rendering Engine — Summary

## Overview

Phase 3 implemented a complete 3D rendering engine on top of the Phase 2 infrastructure (2D drawing, text, input, audio). The engine supports Physically Based Rendering (PBR), Cascaded Shadow Maps (CSM), procedural skybox, fog, glTF 2.0 model loading, and skeletal animation.

---

## Sub-Phases

### Phase 3a: Basic 3D Pipeline
- **Vertex3D.h** — 3D PBR vertex format (position, normal, texcoord, tangent)
- **Transform3D.h/cpp** — Position/Rotation/Scale with world matrix computation
- **Camera3D.h/cpp** — Perspective camera with FPS-style movement
- **MeshData.h/cpp** — CPU-side mesh data + MeshGenerator (Box, Sphere, Plane)
- **Renderer3D.h/cpp** — Core 3D renderer with per-object/per-frame constant buffers
- **PBR.hlsl** — Cook-Torrance BRDF with directional/point/spot lights
- **PBRCommon.hlsli** — Shared PBR math (GGX NDF, Smith geometry, Fresnel)
- **LightingUtils.hlsli** — Light evaluation functions

### Phase 3b: Shadow Mapping
- **ShadowMap.h/cpp** — Basic shadow map (depth-only render target)
- **CascadedShadowMap.h/cpp** — 4-cascade shadow maps for large scenes
- **ShadowUtils.hlsli** — PCF soft shadows, cascade selection
- **ShadowDepth.hlsl** — Shadow depth pass vertex shader
- Shadow pipeline: separate root signature, front-face culling, depth bias

### Phase 3c: Material System & Texture Maps
- **Material.h/cpp** — PBR material (albedo, normal, metallic-roughness, AO, emissive)
- **MaterialManager** — Handle-based material management with freelist
- Texture map binding via descriptor tables (t0-t4)
- Material flags for optional texture maps (HAS_ALBEDO_MAP etc.)

### Phase 3d: glTF Model Loading & Skeletal Animation
- **Model.h** — Model container (GPUMesh + sub-meshes + skeleton + animations)
- **Mesh.h/cpp** — GPU mesh with sub-mesh support
- **ModelLoader.h/cpp** — glTF 2.0 loader using cgltf.h
- **Skeleton.h/cpp** — Joint hierarchy with inverse bind matrices
- **AnimationClip.h/cpp** — Keyframe animation (translation, rotation, scale)
- **AnimationPlayer.h/cpp** — Animation playback with bone matrix computation
- **cgltf.h** — Third-party header-only glTF parser

### Phase 3e: Environment & Effects
- **Skybox.h/cpp** — Procedural skybox (sky gradient + sun disk + corona + haze)
- **Skybox.hlsl** — Sky rendering shader
- **Fog.h** — Fog constants (Linear / Exp / Exp2 modes)
- **PrimitiveBatch3D.h/cpp** — Debug wireframe primitives (lines, boxes, spheres, grid)
- **Primitive3D.hlsl** — 3D line rendering shader
- **Terrain.h/cpp** — Heightmap-based terrain generation and rendering

---

## Architecture

### Rendering Pipeline (per frame)
```
1. Shadow Pass (per cascade 0-3)
   - Render scene depth from light's perspective
   - Depth-only output, front-face culling + depth bias
2. Main Pass
   - Clear backbuffer + depth
   - Skybox (depth write OFF, z=w trick)
   - PBR scene rendering (with CSM shadows + fog)
   - Debug primitives (wireframe, alpha blend)
   - 2D text overlay
3. Present
```

### Constant Buffer Layout
| Register | Name | Contents |
|----------|------|----------|
| b0 | ObjectConstants | World matrix, WorldInverseTranspose |
| b1 | FrameConstants | View/Proj/VP, Camera pos, Time, LightVP[4], CascadeSplits, Shadow params, Fog params |
| b2 | LightConstants | LightData[16], AmbientColor, NumLights |
| b3 | MaterialConstants | Albedo, Metallic, Roughness, AO, Emissive factors + flags |
| b4 | BoneConstants | BoneMatrices[128] (skinned models only) |

### Texture Slots
| Register | Usage |
|----------|-------|
| t0-t4 | Material textures (albedo, normal, metrough, AO, emissive) |
| t8-t11 | Shadow maps (4 cascades) |
| s0 | Linear wrap sampler |
| s2 | Shadow comparison sampler |

---

## File List

### C++ Files (GXLib/Graphics/3D/)
| File | Lines | Description |
|------|-------|-------------|
| Vertex3D.h | ~40 | PBR vertex format + input layout |
| Transform3D.h/cpp | ~120 | 3D transform with world matrix |
| Camera3D.h/cpp | ~180 | FPS camera |
| MeshData.h/cpp | ~350 | Mesh generation (Box, Sphere, Plane) |
| Mesh.h/cpp | ~100 | GPU mesh with sub-meshes |
| Model.h | ~60 | Model container |
| ModelLoader.h/cpp | ~500 | glTF 2.0 loader |
| Skeleton.h/cpp | ~80 | Joint hierarchy |
| AnimationClip.h/cpp | ~100 | Keyframe animation data |
| AnimationPlayer.h/cpp | ~150 | Animation playback |
| Light.h/cpp | ~80 | Light types (Directional, Point, Spot) |
| Material.h/cpp | ~120 | PBR material + manager |
| Fog.h | ~30 | Fog constants |
| ShadowMap.h/cpp | ~100 | Depth-only render target |
| CascadedShadowMap.h/cpp | ~250 | 4-cascade shadow system |
| Skybox.h/cpp | ~140 | Procedural skybox |
| PrimitiveBatch3D.h/cpp | ~190 | Debug wireframes |
| Terrain.h/cpp | ~150 | Heightmap terrain |
| Renderer3D.h/cpp | ~550 | Main 3D renderer |

### Shader Files (Shaders/)
| File | Description |
|------|-------------|
| PBR.hlsl | Cook-Torrance BRDF + lighting + CSM + fog |
| PBRCommon.hlsli | NDF, Geometry, Fresnel functions |
| LightingUtils.hlsli | EvaluateLight for all light types |
| ShadowUtils.hlsli | ComputeCascadedShadow with PCF |
| ShadowDepth.hlsl | Shadow pass VS (depth only) |
| Skybox.hlsl | Procedural sky + sun |
| Primitive3D.hlsl | 3D line rendering |

### Third-Party
| File | Description |
|------|-------------|
| cgltf.h | glTF 2.0 parser (header-only) |

---

## Key Design Decisions

1. **PBR Cook-Torrance BRDF** — Industry-standard physically based shading model
2. **Cascaded Shadow Maps (4 cascades)** — Good shadow quality at all distances
3. **Ring buffer for object constants** — Efficient per-object CB updates without per-draw allocation
4. **Shared SRV heap** — TextureManager's SRV heap also hosts shadow map SRVs
5. **Procedural skybox** — No cubemap textures needed, gradient + sun computed in shader
6. **Inline tonemapping** — PBR.hlsl and Skybox.hlsl currently contain Reinhard tonemapping + gamma correction (to be moved to post-effect pipeline in Phase 4)

## Known Limitations (addressed in Phase 4)

- Tonemapping is inline in PBR.hlsl/Skybox.hlsl (no HDR pipeline)
- No post-processing effects (bloom, FXAA, SSAO, etc.)
- Backbuffer is R8G8B8A8_UNORM (LDR), bright specular highlights are clamped after tonemapping
