# Phase 4k Summary: OutlineEffect (Edge Detection Outline)

## Overview
深度バッファから法線を再構成し、Sobelエッジ検出でアウトラインを合成するポストエフェクト。
Forward+レンダリング（GBuffer無し）環境向け。トゥーン/NPR表現に活用可能。

## Algorithm
1. 3x3近傍の深度をビュー空間Zに変換
2. Sobelオペレータで深度エッジを検出（距離非依存）
3. 中心+4近傍で法線を再構成し、`1 - dot(center, neighbor)` の最大値で法線エッジを検出
4. `edge = max(depthFactor, normalFactor) * intensity`
5. `lerp(sceneColor, lineColor, edge)` で合成
6. スカイボックス (depth >= 1.0) はスキップ

## New Files
| File | Description |
|------|-------------|
| `GXLib/Graphics/PostEffect/OutlineEffect.h` | OutlineEffect class, OutlineConstants struct (112B) |
| `GXLib/Graphics/PostEffect/OutlineEffect.cpp` | Initialization, SRV heap, execution (SSR pattern) |
| `Shaders/Outline.hlsl` | Sobel depth edge + normal edge detection shader |

## Architecture
- SSR/MotionBlurと同じ 2-SRV 専用ヒープパターン (scene + depth)
- SRVヒープ: 2 slots x 2 frames = 4 slots
- RS: b0(CB) + DescTable(t0=scene, t1=depth) + s0(linear) + s1(point)
- HDR空間で処理（MotionBlur後、ColorGrading前）
- 定数バッファ: 112B (invProjection 64B + params + lineColor) -> 256-align

## Key Design Decisions

### Sobel Depth Edge (View-space Z)
- 深度バッファ値ではなくビュー空間Zに変換してからSobelを適用
- 距離に依存しないエッジ検出（遠距離でもアウトラインの太さが一定）
- Sobelカーネル: 標準3x3 (Gx/Gy)

### Normal Edge Detection
- 中心ピクセルの法線と4近傍の法線を再構成
- `1 - dot(normalCenter, normalNeighbor)` で法線差を計算
- 4方向の最大値をエッジ値とする
- スカイボックス近傍はスキップ（depth >= 1.0）

### Normal Reconstruction
- SSRと同じ片側差分選択方式（±1px）
- 深度差が小さい側を選択し、エッジ跨ぎを防止
- `cross(dx, dy)` で法線計算、カメラ方向に修正

## Parameters
| Parameter | Default | Description |
|-----------|---------|-------------|
| depthThreshold | 0.5 | ビュー空間Z差のSobelエッジ閾値 |
| normalThreshold | 0.3 | 法線ドット積エッジ閾値 |
| intensity | 1.0 | アウトライン強度 |
| lineColor | (0,0,0,1) | アウトライン色（黒） |
| enabled | false | デフォルトOFF |

## Modified Files
| File | Changes |
|------|---------|
| `GXLib/Graphics/PostEffect/PostEffectPipeline.h` | OutlineEffect member + accessor added |
| `GXLib/Graphics/PostEffect/PostEffectPipeline.cpp` | Outline integration (after MotionBlur, before ColorGrading) |
| `Sandbox/main.cpp` | Oキーでトグル、HUD表示追加(Y=260)、ShadowDebug行をY=285に移動 |

## Effect Chain Position
```
HDR Scene -> [SSAO(multiply)] -> [SSR] -> [Bloom] -> [DoF] -> [MotionBlur] -> [Outline]
          -> [ColorGrading(HDR)] -> [Tonemapping(HDR->LDR)]
          -> [FXAA(LDR)] -> [Vignette+ChromAb(LDR)] -> Backbuffer
```

**配置理由:**
- MotionBlur後: アウトラインがブラーされない（幾何学的エッジは常にシャープ）
- ColorGrading前: アウトライン色もカラーグレーディング+トーンマッピングの影響を受ける

## Verification
- Build: OK (cmake -B build && cmake --build build --config Debug)
- Oキーでトグル: ON/OFF切り替え
- 球体・箱・柱のシルエットにアウトラインが表示される
- 箱の面境界（法線変化）にもエッジが検出される
- スカイボックス: アウトライン対象外 (depth >= 1.0)
- 遠距離でもアウトライン太さが一定（ビュー空間Z使用）
