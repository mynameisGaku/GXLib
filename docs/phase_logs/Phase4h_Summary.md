# Phase 4h Summary: Depth of Field (DoF)

## Overview
被写界深度 (Depth of Field) ポストエフェクトを実装。
フォーカス距離から離れた領域をぼかし、カメラのレンズ効果をシミュレートする。

## Algorithm
1. **CoC生成**: 深度バッファ → ビュー空間Z復元 → フォーカス距離からのずれ → Circle of Confusion (R16_FLOAT)
2. **ブラー**: CoC加重ガウシアンブラー (H/V分離, half-res)
3. **合成**: CoC値でシャープHDRとブラーHDRをlerp

## New Files
| File | Description |
|------|-------------|
| `GXLib/Graphics/PostEffect/DepthOfField.h` | DoF class declaration |
| `GXLib/Graphics/PostEffect/DepthOfField.cpp` | DoF initialization, 3-pass execution |
| `Shaders/DepthOfField.hlsl` | CoC generation + blur (H/V) + composite shaders |

## Architecture
- 3-pass pipeline: CoC → BlurH → BlurV → Composite
- CoC map: R16_FLOAT full-resolution
- Blur: half-resolution HDR RTs for performance
- Composite: 3-SRV combined heap (sharp + blurred + CoC)
- CopyDescriptors で shader-visible ヒープにSRVをまとめる
- DoF/MotionBlur共通パターン: 専用 DescriptorHeap で複数テクスチャをバインド

## Parameters
| Parameter | Default | Description |
|-----------|---------|-------------|
| focalDistance | 10.0 | フォーカス距離 (ビュー空間Z) |
| focalRange | 5.0 | フォーカス鮮明範囲 |
| bokehRadius | 8.0 | ボケの最大半径 (ピクセル) |

## Modified Files
| File | Changes |
|------|---------|
| `GXLib/Graphics/PostEffect/DepthOfField.h` | New |
| `GXLib/Graphics/PostEffect/DepthOfField.cpp` | New |
| `Shaders/DepthOfField.hlsl` | New |
| `GXLib/Graphics/PostEffect/PostEffectPipeline.h` | DoF member + accessor added |
| `GXLib/Graphics/PostEffect/PostEffectPipeline.cpp` | DoF integration (after Bloom, before ColorGrading) |
| `Sandbox/main.cpp` | 0キーでDoFトグル、F/Gキーでフォーカス距離調整 |

## Effect Chain Position
```
HDR Scene → [SSAO(multiply)] → [Bloom] → [DoF] → [ColorGrading(HDR)]
          → [Tonemapping(HDR→LDR)] → [FXAA(LDR)]
          → [Vignette+ChromAb(LDR)] → Backbuffer
```

## Verification
- Build: OK
- DoF ON/OFF: 0キーでトグル動作確認
- フォーカス距離調整: F/Gキーで遠近のボケが変化
- Half-res blur: パフォーマンス良好
- D3D12 Debug Layer: エラーなし
