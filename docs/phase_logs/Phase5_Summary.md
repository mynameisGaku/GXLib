# Phase 5 Summary: Render Layer System

## Overview
描画レイヤーシステムを実装。レイヤー単位でのRT管理、Z-order合成、ブレンドモード、マスク機能を提供。
Scene(PostFX付き) + UI(2Dオーバーレイ) の2レイヤー構成で動作確認。

## Completion Condition
> 背景→ゲーム→UIが独立レイヤーで管理され合成される → **達成**

---

## New Files

| File | Description |
|------|-------------|
| `GXLib/Graphics/Layer/RenderLayer.h/cpp` | レイヤークラス (RGBA8 LDR RT, Z-order, visibility, opacity, blendMode, mask) |
| `GXLib/Graphics/Layer/LayerStack.h/cpp` | レイヤー管理 (Create/Get/Remove, Z-order sort, OnResize) |
| `GXLib/Graphics/Layer/LayerCompositor.h/cpp` | Z-order順合成 (6 blend PSOs + mask PSOs, 2-SRV dedicated heap) |
| `GXLib/Graphics/Layer/MaskScreen.h/cpp` | DXLib互換マスク (RGBA8 RT, R channel mask, rect/circle draw via VB) |
| `Shaders/LayerComposite.hlsl` | レイヤー合成シェーダー (texture sample + opacity + mask) |
| `Shaders/MaskDraw.hlsl` | マスク描画シェーダー (orthographic projection, fill value) |

## Modified Files

| File | Changes |
|------|---------|
| `GXLib/Graphics/PostEffect/PostEffectPipeline.cpp` | Resolve outputs to any RTV handle (scene layer RT) |
| `Sandbox/main.cpp` | Scene+UI 2-layer構成, L key mask demo toggle |

## Architecture

```
Scene Layer (Z:0, PostFX=true)  ─┐
                                  ├→ LayerCompositor → Backbuffer → Present
UI Layer (Z:1000, Alpha blend)  ─┘
```

- PostEffectPipeline.Resolve() → Scene Layer RT (not backbuffer directly)
- 2D overlay (SpriteBatch/TextRenderer) → UI Layer RT
- LayerCompositor: Z-order ascending, per-layer blend PSO selection

## Key Design Decisions
- **LDR RGBA8 per-layer RT**: PostFX chain handles HDR→LDR, layers are all LDR
- **6 blend modes**: Alpha, Add, Sub, Mul, Screen, None — each has own PSO
- **Mask via separate layer**: MaskScreen wraps a RenderLayer, R channel = mask value
- **2-SRV dedicated heap for masked compositing**: layer texture + mask texture
- **Fullscreen triangle**: SV_VertexID-based, no VB (consistent with PostEffect approach)

## Verification
- Build: OK
- Scene layer with PostFX + UI layer with text/GUI composited correctly
- L key toggles mask demo (rect + circle mask on UI layer)
