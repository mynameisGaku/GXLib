# Phase 4b Summary: Bloom

## Overview
HDRシーンの明るい部分から光の滲み(Bloom)エフェクトを生成するポストエフェクトを実装。

## New Files

| File | Description |
|------|-------------|
| `GXLib/Graphics/PostEffect/Bloom.h` | Bloomエフェクトクラス定義 |
| `GXLib/Graphics/PostEffect/Bloom.cpp` | Bloom実装（Threshold→Downsample→Blur→Upsample→Composite） |
| `Shaders/Bloom.hlsl` | Bloomシェーダー（PSThreshold, PSDownsample, PSGaussianBlurH/V, PSAdditive） |

## Architecture

### Processing Pipeline
1. **Threshold**: HDRシーンから閾値以上の明るい部分を抽出 → mipRT[0]
2. **Downsample**: 5段階のダウンサンプル (1/2 → 1/4 → 1/8 → 1/16 → 1/32)
3. **Gaussian Blur**: 各レベルで水平/垂直の9タップGaussianブラー
4. **Upsample**: 小→大の順にアディティブ合成（mipRT[4]→mipRT[3]→...→mipRT[0]）
5. **Composite**: HDRシーンをdestにコピーし、mipRT[0]をアディティブブレンドで合成

### Key Design Decisions
- **Additive Blend PSO**: D3D12では1つのCBV_SRV_UAVヒープしかバインドできないため、2テクスチャ合成を避け、Additive Blend PSOで描画する方式を採用
- **DynamicBuffer frame index 0 固定**: Bloom全パスは1フレーム内で連続実行されるため、frame index 0のみ使用
- **Gaussian weights**: `{0.227027, 0.194596, 0.121622, 0.054054, 0.016216}` (9タップ)

### Parameters
- `threshold` (default: 1.0): 輝度閾値。これ以上の明るさのピクセルのみBloom対象
- `intensity` (default: 0.5): Bloom合成時の強度

## Modified Files

| File | Changes |
|------|---------|
| `GXLib/Graphics/PostEffect/PostEffectPipeline.h` | Bloom メンバ追加、GetBloom() アクセサ |
| `GXLib/Graphics/PostEffect/PostEffectPipeline.cpp` | Bloom.Execute() をResolve()内で呼び出し、hdrPingPongRTをdestとして使用 |
| `GXLib/Graphics/Pipeline/PipelineState.h/cpp` | SetAdditiveBlend() メソッド追加 |
| `Sandbox/main.cpp` | キー4でBloomトグル、HUDにBloom状態表示 |

## Verification
- Build: OK
- Runtime: Bloom ON/OFFで明るい部分にグロー効果が確認できた
