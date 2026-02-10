# Phase 4c-e Summary: FXAA, Vignette + Chromatic Aberration, Color Grading

## Overview
3つのポストエフェクトを追加:
- **FXAA** (Fast Approximate Anti-Aliasing): トーンマッピング後のLDR画像に適用するアンチエイリアシング
- **Vignette + Chromatic Aberration**: 画面端の周辺減光 + R/G/Bチャンネルの色収差
- **Color Grading**: HDR空間でのカラーグレーディング（露出・コントラスト・彩度・色温度）

## New Files

| File | Description |
|------|-------------|
| `Shaders/FXAA.hlsl` | FXAA 3.11 Quality実装（輝度ベースのエッジ検出 + サブピクセルAA） |
| `Shaders/Vignette.hlsl` | ビネット（距離ベース周辺減光） + 色収差（R/G/B異UVサンプリング） |
| `Shaders/ColorGrading.hlsl` | 露出補正、コントラスト、彩度、色温度調整 |

## Architecture

### Effect Chain (Updated)
```
HDR Scene → [Bloom] → [ColorGrading(HDR)] → [Tonemapping(HDR→LDR)]
          → [FXAA(LDR)] → [Vignette+ChromAb(LDR)] → Backbuffer
```

### Ping-Pong RT Strategy
- **HDR**: `m_hdrRT` と `m_hdrPingPongRT` でHDRエフェクトをピンポン
- **LDR**: `m_ldrRT[0]` と `m_ldrRT[1]` でLDRエフェクトをピンポン
- **最終パス最適化**: チェーンの最後のエフェクトは中間RTを介さず直接バックバッファRTVに描画

### DrawFullscreenToRTV
バックバッファへの直接描画用ヘルパー関数を追加。`DrawFullscreen()`は`RenderTarget&`を要求するため、生の`D3D12_CPU_DESCRIPTOR_HANDLE`でバックバッファに直接描画する`DrawFullscreenToRTV()`を追加。

### FXAA Details
- 周囲9点(N,S,W,E,NW,NE,SW,SE,M)の輝度を計算
- 輝度差が閾値以下なら早期リターン（エッジなし）
- エッジ方向（水平/垂直）を判定
- サブピクセルAAとエッジに沿ったブレンドを適用

### Vignette Details
- `smoothstep(radius, radius+0.4, dist*1.414)` で距離ベースの減光
- `lerp(1, vignette, intensity)` でintensity=0でエフェクト無効化
- 色収差: UV中心からの方向に沿って R/B チャンネルをオフセット

### Color Grading Details
- HDR空間で適用（トーンマッピング前）
- 露出: `exp2(exposure)` で乗算
- コントラスト: `(color - 0.5) * contrast + 0.5` で0.5を基準にスケーリング
- 彩度: 輝度とのlerp
- 色温度: R/Bチャンネルのバランス調整（簡易版）

## Modified Files

| File | Changes |
|------|---------|
| `GXLib/Graphics/PostEffect/PostEffectPipeline.h` | FXAA/Vignette/ColorGrading用のPSO, CB, パラメータ追加。LDR RT[2]追加。DrawFullscreenToRTV()追加 |
| `GXLib/Graphics/PostEffect/PostEffectPipeline.cpp` | 全エフェクトチェーン実装。Resolve()で最後のパスを直接バックバッファに描画する最適化 |
| `Sandbox/main.cpp` | キー5:FXAA, 6:Vignette, 7:ColorGrading トグル。HUDに全エフェクト状態表示 |

## Parameters

| Effect | Parameter | Default | Range |
|--------|-----------|---------|-------|
| FXAA | qualitySubpix | 0.75 | 0-1 |
| FXAA | edgeThreshold | 0.166 | 0.063-0.333 |
| Vignette | intensity | 0.5 | 0-1 |
| Vignette | radius | 0.8 | 0-1 |
| ChromAb | chromaticStrength | 0.003 | 0-0.01 |
| ColorGrading | contrast | 1.0 | 0.5-2.0 |
| ColorGrading | saturation | 1.0 | 0-2.0 |
| ColorGrading | temperature | 0.0 | -1 to 1 |

## Verification
- Build: OK (全ファイルコンパイル成功)
- Runtime: 各エフェクトのON/OFF切替が期待通り動作すること（要実行確認）
