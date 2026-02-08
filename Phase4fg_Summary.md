# Phase 4f-g Summary: SSAO + All-Light Shadows + Cylinder Fix

## Overview
2つのサブフェーズを完了:
- **Phase 4f**: SSAO (Screen Space Ambient Occlusion) - Hemisphere SSAO with bilateral blur
- **Phase 4g**: All-light shadows (Spot + Point shadow maps) + Cylinder winding order fix

## Phase 4f: SSAO

### Implementation
- **Algorithm**: Hemisphere SSAO (64 samples per pixel)
- **Noise**: Hash-based rotation (no noise texture needed)
- **Blur**: Bilateral blur (depth-aware, preserves edges)
- **Composite**: Multiply blend onto HDR scene
- **Input**: Depth buffer SRV + Camera projection/inverse-projection matrices

### New Files
| File | Description |
|------|-------------|
| `Shaders/SSAO.hlsl` | SSAO generation (hemisphere sampling) + bilateral blur + composite passes |
| `GXLib/Graphics/PostEffect/SSAO.h` | SSAO class declaration |
| `GXLib/Graphics/PostEffect/SSAO.cpp` | SSAO initialization, PSO creation, render passes |

### Architecture
- DepthBuffer has `CreateWithOwnSRV()` for shader-visible SRV heap
- DepthBuffer has `TransitionTo()` for explicit state tracking
- PostEffectPipeline::Resolve() takes DepthBuffer & Camera3D args for SSAO
- SSAO uses own half-res RenderTarget for AO map
- 3-pass: Generate AO → Bilateral Blur → Multiply Composite onto scene

### Parameters
| Parameter | Default | Range |
|-----------|---------|-------|
| radius | 0.5 | 0.1-2.0 |
| bias | 0.025 | 0-0.1 |
| intensity | 1.0 | 0-3.0 |
| sampleCount | 64 | 16-128 |

## Phase 4g: All-Light Shadows

### Spot Shadow
- Resolution: 2048x2048
- Single depth-only pass with spot light view/projection matrix
- PCF filtering in PBR shader
- SRV slot: t12

### Point Shadow
- Resolution: 1024x1024 per face
- **Texture2DArray** with 6 slices (one per cube face)
- Own DSV heap with 6 DSV descriptors
- 6 render passes (one per face), face selected by dominant axis
- View matrices: +X, -X, +Y, -Y, +Z, -Z with 90-degree FOV
- PCF filtering with face selection by dominant axis in PBR shader
- SRV slot: t13 (Texture2DArray)

### Shadow CB Layout
- FrameConstants: 1008 bytes total
  - CSM: 528 bytes (4 cascades × 132 bytes)
  - Spot: 80 bytes (ViewProj + params)
  - Point: 400 bytes (6 face ViewProj matrices + params)
- Shadow pass CB: 11 slots × 256 bytes each
  - Slots 0-3: CSM cascades
  - Slot 4: Spot shadow
  - Slots 5-10: Point shadow faces

### SRV Layout in PBR Shader
| Slot | Content |
|------|---------|
| t8-t11 | CSM shadow maps (4 cascades) |
| t12 | Spot shadow map |
| t13 | Point shadow map (Texture2DArray, 6 slices) |

## Cylinder Winding Order Fix

### Problem
`CreateCylinder()` had reversed triangle winding for side faces and bottom cap. With `D3D12_CULL_MODE_BACK`, exterior faces were culled, showing interior faces with inverted lighting.

### Verification Method
Computed screen-space signed areas comparing with working CreateBox:
- Box right face: signed area < 0 = front-facing from outside (correct)
- Cylinder side face: signed area > 0 = back-facing from outside (WRONG)
- Cylinder bottom cap: back-facing from below (WRONG)
- Cylinder top cap: front-facing from above (correct)

### Fix (MeshData.cpp)
**Side faces** - swapped 2nd and 3rd vertex of each triangle:
```cpp
// Before: base+j, base+j+ringCount, base+j+1
// After:  base+j, base+j+1, base+j+ringCount

// Before: base+j+1, base+j+ringCount, base+j+ringCount+1
// After:  base+j+1, base+j+ringCount+1, base+j+ringCount
```

**Bottom cap** - swapped 2nd and 3rd vertex:
```cpp
// Before: center, j+1, j+2
// After:  center, j+2, j+1
```

**Top cap** - no change needed (already correct)

## Modified Files

| File | Changes |
|------|---------|
| `GXLib/Graphics/PostEffect/SSAO.h` | New: SSAO class |
| `GXLib/Graphics/PostEffect/SSAO.cpp` | New: SSAO implementation |
| `Shaders/SSAO.hlsl` | New: SSAO shader (generate + blur + composite) |
| `GXLib/Graphics/Resource/DepthBuffer.h` | Added CreateWithOwnSRV(), TransitionTo(), GetSRVGPUHandle() |
| `GXLib/Graphics/Resource/DepthBuffer.cpp` | Own SRV heap, state tracking |
| `GXLib/Graphics/PostEffect/PostEffectPipeline.h` | Added SSAO member, Resolve() signature change |
| `GXLib/Graphics/PostEffect/PostEffectPipeline.cpp` | SSAO integration in effect chain |
| `GXLib/Graphics/3D/Renderer3D.h/cpp` | Spot + Point shadow map rendering passes |
| `GXLib/Graphics/3D/PointShadowMap.h/cpp` | New: Point shadow map (6-face Texture2DArray) |
| `GXLib/Graphics/3D/Light.h` | SpotLight/PointLight shadow params |
| `Shaders/PBR.hlsl` | Spot + Point shadow sampling, face selection |
| `Shaders/Shadow.hlsl` | Shadow pass vertex shader |
| `GXLib/Graphics/3D/MeshData.cpp` | Cylinder winding order fix (sides + bottom cap) |
| `Sandbox/main.cpp` | SSAO toggle (key 8), shadow demo scene |

## Effect Chain (Updated)
```
HDR Scene → [SSAO(multiply)] → [Bloom] → [ColorGrading(HDR)]
          → [Tonemapping(HDR→LDR)] → [FXAA(LDR)]
          → [Vignette+ChromAb(LDR)] → Backbuffer
```

## Verification
- Build: OK
- SSAO: Hemisphere sampling produces visible darkening in corners/crevices
- Spot Shadow: 2048x2048, PCF filtered
- Point Shadow: 6-face 1024x1024 Texture2DArray, dominant-axis face selection
- Cylinder: Correct winding - sides and bottom visible with proper lighting
