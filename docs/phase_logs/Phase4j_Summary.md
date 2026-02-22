# Phase 4j Summary: Screen Space Reflections (SSR)

## Overview
スクリーン空間レイマーチングによる反射ポストエフェクトを実装。
Forward+レンダリング（GBuffer無し）環境向けに、深度勾配から法線を再構成する方式。

## Algorithm
1. 深度バッファからビュー空間位置を再構成
2. ±8pxカーネルで深度勾配からビュー空間法線を再構成（中央/片側差分のスムーズブレンド）
3. 反射方向を計算し、レイをスクリーン空間に射影
4. スクリーン空間DDAレイマーチング（1ピクセル/ステップ）
5. ビュー空間Z比較で深度バッファとの交差判定
6. バイナリ精緻化（8反復）でヒット位置を精密化
7. フレネル効果 + エッジフェード + 距離フェード + エッジ信頼度で反射強度を制御

## New Files
| File | Description |
|------|-------------|
| `GXLib/Graphics/PostEffect/SSR.h` | SSR class declaration, SSRConstants struct |
| `GXLib/Graphics/PostEffect/SSR.cpp` | SSR initialization, SRV heap, execution |
| `Shaders/SSR.hlsl` | Screen-space DDA ray marching shader |

## Architecture
- DoF/MotionBlurと同じ 2-SRV 専用ヒープパターン (scene + depth)
- SRVヒープ: 2 slots × 2 frames = 4 slots
- RS: b0(CB) + DescTable(t0=scene, t1=depth) + s0(linear) + s1(point)
- HDR空間で処理（SSAO後、Bloom前）
- 定数バッファ: 224B (3行列 + 8パラメータ) → 256-align

## Key Design Decisions

### Screen-space DDA Ray Marching
- ビュー空間ステッピングでは画面上の不均一なカバレッジでゴースティングが発生
- スクリーン空間DDA（1ピクセル/ステップ）で均一なサンプリングを実現
- 射影深度の線形補間で透視投影正確な深度比較

### 法線再構成（±8px カーネル）
- GBufferが無いため深度勾配から法線を近似
- ±1px: ポリゴンファセットが見える（階段状アーティファクト）
- ±16px 2レベル: カーネル切替境界にアウトラインが出る
- ±8px スムーズブレンド: 中央差分と片側差分をビュー空間Z重みで滑らかに混合

### ビュー空間Zエッジ検出
- 深度バッファ値での固定閾値は遠距離で破綻（非線形圧縮のため）
- ビュー空間Z座標で比較（距離非依存、閾値0.3ユニット）
- エッジ信頼度を法線再構成と統一（±8pxサンプルから算出）

### フレネル反射
- F0 = 0.3（可視性のため高めに設定）
- Schlick近似: `F = F0 + (1-F0) * (1-NdotV)^5`
- グレージング角ほど強い反射

## Parameters
| Parameter | Default | Description |
|-----------|---------|-------------|
| maxDistance | 30.0 | レイの最大距離 (view-space) |
| stepSize | 0.15 | 未使用（DDA方式では自動計算） |
| maxSteps | 200 | 最大ステップ数 |
| thickness | 0.15 | ヒット判定の厚み (view-space units) |
| intensity | 1.0 | 反射強度 |
| enabled | false | デフォルトOFF (パフォーマンス考慮) |

## Modified Files
| File | Changes |
|------|---------|
| `GXLib/Graphics/PostEffect/PostEffectPipeline.h` | SSR member + accessor added |
| `GXLib/Graphics/PostEffect/PostEffectPipeline.cpp` | SSR integration (after SSAO, before Bloom) |
| `Sandbox/main.cpp` | Rキーでトグル、HUD表示追加、SSRデモオブジェクト(ミラーウォール+カラフル球体) |

## Effect Chain Position
```
HDR Scene → [SSAO(multiply)] → [SSR] → [Bloom] → [DoF] → [MotionBlur]
          → [ColorGrading(HDR)] → [Tonemapping(HDR→LDR)]
          → [FXAA(LDR)] → [Vignette+ChromAb(LDR)] → Backbuffer
```

## Lessons Learned
- **Screen-space DDA > View-space stepping**: ビュー空間ステッピングは画面カバレッジが不均一でゴースト/トレイルが発生する。スクリーン空間DDA（1px/step）が正解
- **法線カーネルサイズ**: 小さすぎ(±1px)→ポリゴン見える、大きすぎ(±16px)→アウトライン。±8pxが妥協点
- **2レベルカーネルのハード切替は禁物**: 16px→2pxのハード切替で枠線が出る。lerp()でスムーズに混合すべき
- **深度バッファの非線形性**: 深度バッファ値での固定閾値は遠距離で破綻する。ビュー空間Zで比較すれば距離非依存
- **ViewZFromDepth()**: invProjection の (2,2)/(3,2) 成分だけで安価にビュー空間Zを取得可能
- **バイナリ精緻化**: 8反復でサブピクセル精度のヒット位置を取得。視覚的品質が大幅向上
- **エッジ信頼度の統一**: 法線再構成とエッジ検出を同じ±8pxサンプルから計算することで、範囲の不一致によるアーティファクトを防止

## Verification
- Build: OK
- Rキーでトグル: ON/OFF切り替え動作確認
- ミラーウォール: カラフル球体の反射が見える
- 床面: グレージング角で反射が強くなる（フレネル効果）
- 画面端: フェードアウトする
- スカイボックス: SSR対象外 (depth >= 1.0)
- D3D12 Debug Layer: エラーなし
