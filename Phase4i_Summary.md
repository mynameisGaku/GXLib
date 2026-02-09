# Phase 4i Summary: Motion Blur

## Overview
カメラベースのモーションブラーポストエフェクトを実装。
深度再投影方式により、カメラ移動に応じたブラー効果を生成する。

## Algorithm
1. 深度バッファからワールド座標を再構成 (invViewProjection)
2. 前フレームのVP行列でスクリーン座標に再投影
3. 現在位置と前フレーム位置の差分から速度ベクトルを計算
4. 速度方向にHDRシーンをブラー (N samples)
5. スカイボックス (depth >= 1.0) はブラーをスキップ

## New Files
| File | Description |
|------|-------------|
| `GXLib/Graphics/PostEffect/MotionBlur.h` | MotionBlur class declaration |
| `GXLib/Graphics/PostEffect/MotionBlur.cpp` | MotionBlur initialization, execution |
| `Shaders/MotionBlur.hlsl` | Depth reprojection + velocity blur shader |

## Architecture
- DoFと同じ 2-SRV 専用ヒープパターン (scene + depth)
- SRVヒープ: 2 slots × 2 frames = 4 slots
- RS: b0(CB) + DescTable(t0=scene, t1=depth) + s0(linear) + s1(point)
- HDR空間で処理 (Bloom/DoFの後、ColorGradingの前)

## Key Design Decision: UpdatePreviousVP のタイミング
- `UpdatePreviousVP()` は `Execute()` の**後**に呼ぶ必要がある
- 先に呼ぶと今フレームのVPで上書きされ、速度=0になりブラーが効かない
- PostEffectPipeline::Resolve() 内で Execute() 後に呼び出し

## Parameters
| Parameter | Default | Description |
|-----------|---------|-------------|
| intensity | 1.0 | ブラー強度 |
| sampleCount | 16 | ブラーサンプル数 |
| enabled | false | デフォルトOFF (パフォーマンス考慮) |

## Modified Files
| File | Changes |
|------|---------|
| `GXLib/Graphics/PostEffect/MotionBlur.h` | New |
| `GXLib/Graphics/PostEffect/MotionBlur.cpp` | New |
| `Shaders/MotionBlur.hlsl` | New |
| `GXLib/Graphics/PostEffect/PostEffectPipeline.h` | MotionBlur member + accessor added |
| `GXLib/Graphics/PostEffect/PostEffectPipeline.cpp` | MotionBlur integration (after DoF) |
| `Sandbox/main.cpp` | Bキーでトグル、HUD表示追加 |

## Effect Chain Position
```
HDR Scene → [SSAO(multiply)] → [Bloom] → [DoF] → [MotionBlur]
          → [ColorGrading(HDR)] → [Tonemapping(HDR→LDR)]
          → [FXAA(LDR)] → [Vignette+ChromAb(LDR)] → Backbuffer
```

## Lessons Learned
- **UpdatePreviousVP timing**: Execute後に呼ぶ。先に呼ぶと速度=0になる
- **スカイボックス除外**: depth >= 1.0 をチェックしてブラーをスキップ
- **速度クランプ**: 最大速度を画面の10%に制限し、極端なブラーを防止
- **2-SRV専用ヒープ**: D3D12ではCBV_SRV_UAVヒープは1つしかバインドできないため、複数テクスチャ使用時は専用ヒープにまとめる

## Verification
- Build: OK
- Bキーでトグル: ON/OFF切り替え動作確認
- カメラ移動時: 速度に応じたブラー効果
- 静止時: ブラーなし (速度ベクトル ≈ 0)
- スカイボックス: ブラーされない
- D3D12 Debug Layer: エラーなし
