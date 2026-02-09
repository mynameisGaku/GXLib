# Phase 4l Summary: VolumetricLight (God Rays)

## Overview
GPU Gems 3 のスクリーン空間ラディアルブラー方式で、ディレクショナルライトからのゴッドレイ（光の筋）を合成するポストエフェクト。
太陽のスクリーン位置を基点に、深度バッファで遮蔽判定しながらラディアルブラーを適用する。

## Algorithm
1. CPU側: ライト逆方向の遠方点 (`camPos - lightDir*1000`) をVP行列で射影 → NDC → UV変換
2. CPU側: ビュー空間Zで前方チェック + 画面外フェードアウトで太陽可視性を計算
3. GPU側: ピクセルUVから太陽スクリーン位置へ向かうレイを計算
4. GPU側: ハッシュノイズでピクセル毎にレイ開始位置をジッター（バンディング軽減）
5. GPU側: `numSamples` ステップでレイマーチ、各ステップで深度をリニアサンプリング
6. GPU側: `smoothstep(0.99, 1.0, depth)` で遮蔽判定（スカイ=光、オブジェクト=遮蔽）
7. GPU側: `weight * illuminationDecay` でステップ毎の寄与を蓄積、`decay` で減衰
8. GPU側: 最終色 = `godRay * exposure * intensity * sunVisible * lightColor`
9. シーンに加算合成: `result = sceneColor.rgb + finalGodRay`

## New Files
| File | Description |
|------|-------------|
| `GXLib/Graphics/PostEffect/VolumetricLight.h` | VolumetricLight class, VolumetricLightConstants struct (48B) |
| `GXLib/Graphics/PostEffect/VolumetricLight.cpp` | Initialization, SRV heap, sun position calculation, execution |
| `Shaders/VolumetricLight.hlsl` | Radial blur god ray shader with jitter + linear depth sampling |

## Architecture
- SSR/MotionBlur/OutlineEffectと同じ 2-SRV 専用ヒープパターン (scene + depth)
- SRVヒープ: 2 slots x 2 frames = 4 slots
- RS: b0(CB) + DescTable(t0=scene, t1=depth) + s0(linear) + s1(point)
- HDR空間で処理（SSR後、Bloom前）
- 定数バッファ: 48B (sunScreenPos 8B + params 24B + lightColor 12B + sunVisible 4B) -> 256-align

## Key Design Decisions

### Sun Screen Position (CPU側)
- 太陽ワールド位置 = `camPos - normalize(lightDir) * 1000` (光の逆方向の遠方)
- VP行列で射影 → NDC → UV変換 (`x*0.5+0.5`, `-y*0.5+0.5`)
- PostEffectPipelineはRenderer3Dにアクセスできないため、ライト方向はSetLightDirection()で外部設定

### Sun Visibility
- ビュー空間Zで前方チェック (z <= 0 → visible = 0)
- 画面内(距離~0.7)は100%、画面外は距離に応じてフェードアウト、距離2.0以上で0
- UpdateSunInfo()を毎フレーム呼出し（enabled状態に関係なく）

### Jitter + Linear Sampling (品質改善)
- 初期実装ではバンディング（ジャギー）が発生
- **ハッシュノイズジッター**: ピクセル毎にレイ開始位置をランダムオフセット → 規則的バンディングをノイズに変換
- **リニア深度サンプリング**: PointSamplerからLinearSamplerに変更 → オブジェクトエッジで自然なブレンド
- **smoothstep遷移**: binary判定からsmoothstep(0.99, 1.0)に変更 → 滑らかな遮蔽遷移

### C++ saturate()問題
- `saturate()` はHLSLイントリンシックで、C++では使用不可
- `(std::max)(0.0f, (std::min)(1.0f, value))` で代替

## Parameters
| Parameter | Default | Description |
|-----------|---------|-------------|
| density | 1.0 | 散乱密度（レイが太陽位置まで伸びる） |
| decay | 0.97 | 穏やかな減衰（画面の約60%まで可視） |
| weight | 0.04 | 控えめなサンプル寄与で過度な明るさを防止 |
| exposure | 0.35 | 保守的な値でシーンが洗い流されない |
| numSamples | 96 | 品質/パフォーマンスのバランス |
| intensity | 1.0 | ニュートラル、調整しやすい |
| lightColor | (1.0, 0.98, 0.95) | 暖色系の太陽光 |
| lightDirection | (0.3, -1.0, 0.5) | デフォルトライト方向 |
| enabled | false | パフォーマンス考慮でデフォルトOFF |

## Modified Files
| File | Changes |
|------|---------|
| `GXLib/Graphics/PostEffect/PostEffectPipeline.h` | VolumetricLight member + accessor added |
| `GXLib/Graphics/PostEffect/PostEffectPipeline.cpp` | VolumetricLight integration (after SSR, before Bloom); UpdateSunInfo() called every frame |
| `Sandbox/main.cpp` | Vキーでトグル、SetLightDirection/Color初期設定、HUD表示追加(Y=285 GodRay状態+SunUV+Visible)、ShadowDebug行をY=310に移動、ヘルプに"V: GodRays"追加 |

## Effect Chain Position
```
HDR Scene -> [SSAO(multiply)] -> [SSR] -> [VolumetricLight] -> [Bloom] -> [DoF] -> [MotionBlur] -> [Outline]
          -> [ColorGrading(HDR)] -> [Tonemapping(HDR->LDR)]
          -> [FXAA(LDR)] -> [Vignette+ChromAb(LDR)] -> Backbuffer
```

**配置理由:**
- SSR後: 反射は既に計算済み
- Bloom前: ゴッドレイの明るい光がBloomに自然に寄与する

## Verification
- Build: OK (cmake -B build && cmake --build build --config Debug)
- Vキーでトグル: ON/OFF切り替え
- カメラを太陽方向に向けるとゴッドレイが表示される
- オブジェクトのシルエットによりレイが遮蔽される
- カメラが太陽の反対方向を向くとエフェクトがフェードアウト
- Bloom ON時にゴッドレイがBloomで光る
- ジッター+リニアサンプリングにより、バンディングやジャギーが抑制されている
- HUDにSunUV/Visible情報がリアルタイム表示される
