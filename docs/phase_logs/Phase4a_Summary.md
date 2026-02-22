# Phase 4a: HDR Pipeline + Tonemapping — Summary

## Overview

Phase 4aでは、シーン描画をHDR浮動小数点レンダーターゲット (R16G16B16A16_FLOAT) に切り替え、ポストエフェクトパイプラインの基盤を構築した。トーンマッピング（Reinhard/ACES/Uncharted2）をフルスクリーン三角形パスとして実装し、LDRバックバッファへ出力する。

## 新規ファイル

| File | Description |
|------|-------------|
| `GXLib/Graphics/PostEffect/PostEffectPipeline.h` | ポストエフェクトパイプライン管理クラス |
| `GXLib/Graphics/PostEffect/PostEffectPipeline.cpp` | HDR RT作成、BeginScene/EndScene/Resolve、トーンマッピングPSO |
| `Shaders/Fullscreen.hlsli` | SV_VertexIDベースのフルスクリーン三角形VS（VB不要） |
| `Shaders/Tonemapping.hlsl` | 3種トーンマッピング + ガンマ補正PS |

## 変更ファイル

| File | Change |
|------|--------|
| `GXLib/Graphics/Resource/RenderTarget.h/cpp` | リソースステート管理 (m_currentState, TransitionTo()) 追加 |
| `Shaders/PBR.hlsl` (L241-245) | インラインReinhard+ガンマ削除、HDRリニア値出力 |
| `Shaders/Skybox.hlsl` (L64-68) | 同上 |
| `GXLib/Graphics/3D/Renderer3D.cpp` | PBR PSO → R16G16B16A16_FLOAT |
| `GXLib/Graphics/3D/Skybox.cpp` | Skybox PSO → R16G16B16A16_FLOAT |
| `GXLib/Graphics/3D/PrimitiveBatch3D.cpp` | PrimitiveBatch3D PSO → R16G16B16A16_FLOAT |
| `Sandbox/main.cpp` | HDR RT描画→PostEffect→Tonemapping→LDRフロー、1/2/3キーでトーンマップ切替、+/-で露出調整 |

## 描画フロー

```
Shadow Pass (x4 cascades)
  → HDR RT Clear + DSV Clear
  → Skybox (HDR)
  → PBR 3D Scene (HDR)
  → Debug Primitives (HDR)
  → EndScene (HDR RT → SRV)
  → Backbuffer → RENDER_TARGET
  → Tonemapping (HDR SRV → LDR Backbuffer)
  → 2D Text Overlay (LDR)
  → Backbuffer → PRESENT
```

## 操作

| Key | Action |
|-----|--------|
| 1 | Reinhard tonemapping |
| 2 | ACES Filmic tonemapping |
| 3 | Uncharted2 tonemapping |
| +/- | Exposure調整 |
