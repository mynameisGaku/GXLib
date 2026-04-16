# 06-custom-shader-model — Layer 2 Custom Shader Model

**Layer**: 2 (extension point per ADR-0017 T1.7)

## What This Demonstrates

The `ShaderRegistry` now supports user-registered shader models via
`RegisterCustomShaderModel(customId, desc)`. This sample:

1. Registers `Rainbow.hlsl` (a minimal custom VS/PS) as shader model ID 100
2. Loads a cube model
3. Renders it with the custom shader — world-position-based rainbow gradient

## API Used

```cpp
gx::ShaderRegistry::CustomShaderModelDesc desc;
desc.vsPath = L"Shaders/custom/Rainbow.hlsl";
desc.psPath = L"Shaders/custom/Rainbow.hlsl";
desc.vsEntry = "VSMain";
desc.psEntry = "PSMain";
desc.supportsSkinning = true;  // optional: generate SKINNED variant

uint32_t MY_ID = 100;  // any value in [6, 254]
registry.RegisterCustomShaderModel(MY_ID, desc);

// Assign to a material:
// material.shaderModel = static_cast<ShaderModel>(MY_ID);
```

## ID Ranges

| Range | Use |
|-------|-----|
| 0-5   | Built-in (Standard/Unlit/Toon/Phong/Subsurface/ClearCoat) — reserved |
| 6-254 | User custom models (this API) |
| 255   | `Custom` — per-material ad-hoc shader via `Material.shaderHandle` |

## HLSL Contract

Your shader MUST write to 3 render targets:
- `SV_Target0` — HDR color (R16G16B16A16_FLOAT)
- `SV_Target1` — world-space normal encoded as `normal * 0.5 + 0.5` (R16G16B16A16_FLOAT)
- `SV_Target2` — albedo for GI (R8G8B8A8_UNORM)

Vertex input is `Vertex3D_PBR` (static) or `Vertex3D_Skinned` (if `supportsSkinning`).
When `SKINNED` is defined, apply bone weights in the VS.

See `PBRCommon.hlsli` for shared cbuffers (`WorldMatrix`, `ViewProjMatrix`, etc.).

## Compat API (added this sprint)

```cpp
// Register via Layer 2 API
registry.RegisterCustomShaderModel(100, desc);

// Apply via Compat (Layer 1 — one-liner)
int m = LoadModel("player.fbx");
SetModelShaderModel(m, 100);   // all materials → shader model 100

// Per-material granularity
SetMaterialShaderModel(matH, 100);
```

Both `SetMaterialShaderModel` and `SetModelShaderModel` follow DXLib return-code
convention (0 / count / -1).

## Reference

- ADR-0017 L2 pillar — this extension point was committed here
- ADR-0008 Rendering Pipeline — the FrameGraph + ShaderRegistry architecture
- `docs/implementation-gap-analysis-2026-04-16.md` T1.7
