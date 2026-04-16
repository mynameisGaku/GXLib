# 05-custom-postfx — Layer 2 Custom PostFX Insertion

**Layer**: 2 (extension point per ADR-0017 T1.5)

## What This Demonstrates

`PostEffectPipeline::InsertCustomEffect` allows user code to inject a custom
render pass into the canonical PostFX chain at any of 7 insertion points.
The pipeline handles ping-pong render target management; the custom effect
just reads `input` and writes `output`.

## API Used

```cpp
// 1. Derive from ICustomEffect
class MyEffect : public PostEffectPipeline::ICustomEffect {
    void Execute(ID3D12GraphicsCommandList* cmd,
                 RenderTarget& input, RenderTarget& output,
                 uint32_t w, uint32_t h) override {
        // Bind your PSO / root sig
        // Set input.GetSRV() as source, output.GetRTV() as destination
        // Dispatch or DrawFullscreen
    }
};

// 2. Register
auto& pipe = GetPostEffects();
pipe.InsertCustomEffect(
    "MyEffect",
    PostEffectPipeline::PostFXInsertPoint::AfterBloom,
    std::make_unique<MyEffect>());

// 3. Unregister when done (optional — destroyed at pipeline shutdown)
pipe.RemoveCustomEffect("MyEffect");
```

## Insertion Points (7 total)

| Point | Space | Chain position |
|-------|-------|----------------|
| `AfterSSAO` | HDR | After SSAO/ContactShadows, before Reflections |
| `AfterBloom` | HDR | After Bloom/LensFlare, before DoF |
| `AfterDoF` | HDR | After DoF, before MotionBlur |
| `AfterMotionBlur` | HDR | After MotionBlur, before Outline |
| `BeforeToneMap` | HDR | After ColorGrading, before tone-mapping (last HDR point) |
| `AfterToneMap` | LDR | After tone-mapping → LDR, before FXAA |
| `BeforeFXAA` | LDR | Same LDR ping-pong, just before FXAA if enabled |

## Contract

- `Execute` is called every frame while the registered point is active.
- `input` is in `PIXEL_SHADER_RESOURCE` state (SRV-ready).
- `output` is in `RENDER_TARGET` state (RTV-ready).
- After return, the pipeline transitions `output` to SRV automatically.
- `width` / `height` reflect the current render resolution (DynamicResolution-adjusted for HDR targets, full res for LDR).

## HDR vs LDR Spaces

- **HDR** (R16G16B16A16_FLOAT): scene color before tone-mapping. Use for physically-based effects.
- **LDR** (R8G8B8A8_UNORM): after tone-mapping. Use for color-space / UI-related effects.

## Reference

- ADR-0017 L2 pillar
- ADR-0008 Rendering Pipeline §5 (canonical PostFX order)
- `docs/implementation-gap-analysis-2026-04-16.md` T1.5
