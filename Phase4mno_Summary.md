# Phase 4m/n/o Summary: TAA + AutoExposure + JSON Settings

## Overview
Phase 4 の最終3タスクを実装。Temporal Anti-Aliasing (TAA)、自動露出 (Auto-Exposure)、JSON設定ファイルの読み書き。
これにより Phase 4 の完了条件「全エフェクトがON/OFFでき、ビジュアルが劇的に向上する。JSON設定ファイルで制御可能」を達成。

---

## Phase 4m: TAA (Temporal Anti-Aliasing)

### Algorithm
1. Halton(2,3) 数列で8サンプルサイクルのサブピクセルジッターを生成
2. ジッターをプロジェクション行列の _31, _32 に加算 (`Camera3D::GetJitteredProjectionMatrix`)
3. Renderer3D が jittered VP でシーンをレンダリング（シャドウパスは非ジッター）
4. 深度バッファからリプロジェクション: 現フレーム逆VP → ワールド → 前フレームVP → historyUV
5. 3x3近傍の variance clipping (mu +/- gamma*sigma, gamma=1.0) で履歴をクランプ
6. lerp(current, clampedHistory, blendFactor=0.9) でブレンド
7. 画面外の historyUV は current をそのまま使用
8. TAA出力を historyRT に CopyResource で保存

### New Files
| File | Description |
|------|-------------|
| `GXLib/Graphics/PostEffect/TAA.h` | TAA class, TAAConstants struct (160B), Halton generator |
| `GXLib/Graphics/PostEffect/TAA.cpp` | 3-SRV dedicated heap (scene+history+depth), Execute with history copy |
| `Shaders/TAA.hlsl` | Variance clipping + depth reprojection + unjittered sampling |

### Modified Files
| File | Changes |
|------|---------|
| `GXLib/Graphics/3D/Camera3D.h` | SetJitter/ClearJitter/GetJitter/GetJitteredProjectionMatrix + m_jitterOffset member |
| `GXLib/Graphics/3D/Camera3D.cpp` | GetJitteredProjectionMatrix implementation (proj._31/_32 offset) |
| `GXLib/Graphics/3D/Renderer3D.cpp` | FrameConstants uses jittered VP (line 521-522) |
| `GXLib/Graphics/PostEffect/PostEffectPipeline.h` | TAA member + accessor, BeginScene signature: Camera3D& (non-const) |
| `GXLib/Graphics/PostEffect/PostEffectPipeline.cpp` | TAA init/execute/OnResize, BeginScene applies jitter, Resolve integrates TAA after Outline |
| `Sandbox/main.cpp` | T key toggle, BeginScene(camera) call, HUD Y=310, help updated |

### Key Design Decisions
- **BeginScene signature change**: `const Camera3D&` → `Camera3D&` (non-const) for jitter application
- **Non-jittered VP for TAA CB**: TAA uses `camera.GetViewProjectionMatrix()` (non-jittered) for reprojection
- **3-SRV heap**: scene + history + depth, 3 slots x 2 frames = 6 slots
- **First frame handling**: CopyResource src→dest→history without shader execution
- **Effect chain position**: After Outline, before ColorGrading (all HDR spatial effects complete)

---

## Phase 4n: AutoExposure (Eye Adaptation)

### Algorithm
1. HDR scene → 256x256 log luminance RT (R16_FLOAT): `log(max(luminance, 0.0001))`
2. Downsample chain: 256 → 64 → 16 → 4 → 1 (bilinear sampling, 4 passes)
3. 1x1 RT → readback buffer copy (2-frame ring buffer, no GPU stall)
4. CPU reads previous frame's readback: half-float → float conversion
5. targetExposure = keyValue (0.18) / exp(avgLogLuminance)
6. Exponential smoothing: current += (target - current) * (1 - exp(-speed * dt))
7. Clamp to [minExposure, maxExposure]

### New Files
| File | Description |
|------|-------------|
| `GXLib/Graphics/PostEffect/AutoExposure.h` | AutoExposure class with downsample chain + readback ring buffer |
| `GXLib/Graphics/PostEffect/AutoExposure.cpp` | PS-based log-luminance, 4-pass downsample, readback, adaptation |
| `Shaders/AutoExposure.hlsl` | PSLogLuminance (HDR→log(lum)), PSDownsample (bilinear average) |

### Modified Files
| File | Changes |
|------|---------|
| `GXLib/Graphics/PostEffect/PostEffectPipeline.h` | AutoExposure member + accessor, Resolve deltaTime param |
| `GXLib/Graphics/PostEffect/PostEffectPipeline.cpp` | AutoExposure init/OnResize, ComputeExposure before tonemapping |
| `Sandbox/main.cpp` | X key toggle, Resolve(deltaTime) call, HUD Y=335, help updated |

### Key Design Decisions
- **PS-based (no Compute Shader)**: CSインフラ不要、既存パイプラインに統合しやすい
- **2-frame readback ring buffer**: GPU stall 回避、2フレーム遅延は視覚的に問題なし
- **Half-float CPU conversion**: R16_FLOAT readback → manual IEEE 754 half→float conversion
- **Fixed-size downsample chain**: 256→1 は固定サイズなので OnResize 不要
- **Integration point**: トーンマッピング直前で `exposure = ComputeExposure(...)` を呼び出し

---

## Phase 4o: JSON Settings

### New Files
| File | Description |
|------|-------------|
| `GXLib/ThirdParty/json.hpp` | nlohmann/json v3.11.3 single-header (MIT license) |
| `GXLib/Graphics/PostEffect/PostEffectSettings.h` | PostEffectSettings::Load/Save static methods |
| `GXLib/Graphics/PostEffect/PostEffectSettings.cpp` | Full JSON serialization for all 13 effects |

### Modified Files
| File | Changes |
|------|---------|
| `GXLib/Graphics/PostEffect/PostEffectPipeline.h` | LoadSettings/SaveSettings methods, const accessors for all effects |
| `GXLib/Graphics/PostEffect/PostEffectPipeline.cpp` | LoadSettings/SaveSettings delegation to PostEffectSettings |
| `Sandbox/main.cpp` | F12 key save, startup load from post_effects.json |

### JSON Format
```json
{
  "postEffects": {
    "tonemapping": { "mode": "ACES", "exposure": 1.0 },
    "bloom": { "enabled": true, "threshold": 1.0, "intensity": 0.5 },
    "fxaa": { "enabled": true },
    "ssao": { "enabled": false, "radius": 0.5, "bias": 0.025, "power": 2.0 },
    "dof": { "enabled": false, "focalDistance": 10.0, "focalRange": 5.0, "bokehRadius": 4.0 },
    "motionBlur": { "enabled": false, "intensity": 1.0, "sampleCount": 16 },
    "ssr": { "enabled": false, "maxSteps": 200, "intensity": 1.0 },
    "outline": { "enabled": false, "depthThreshold": 0.5, "normalThreshold": 0.3, "intensity": 1.0 },
    "taa": { "enabled": false, "blendFactor": 0.9 },
    "autoExposure": { "enabled": false, "speed": 1.5, "min": 0.1, "max": 10.0, "keyValue": 0.18 },
    "volumetricLight": { "enabled": false, "intensity": 1.0, "density": 1.0, "decay": 0.97 },
    "vignette": { "enabled": false, "intensity": 0.5, "chromaticAberration": 0.003 },
    "colorGrading": { "enabled": false, "contrast": 1.0, "saturation": 1.0, "temperature": 0.0 }
  }
}
```

---

## Final Effect Chain (Phase 4 Complete)
```
HDR Scene -> [SSAO(multiply)] -> [SSR] -> [VolumetricLight] -> [Bloom] -> [DoF]
          -> [MotionBlur] -> [Outline] -> [TAA] -> [ColorGrading(HDR)]
          -> [AutoExposure] -> [Tonemapping(HDR->LDR)]
          -> [FXAA(LDR)] -> [Vignette+ChromAb(LDR)] -> Backbuffer
```

## Phase 4 Completion Status
All 13 post-effects from Framework Plan §3.4 implemented:
- Bloom, Tonemapping (3 modes), HDR, SSAO, DoF, MotionBlur, ColorGrading
- FXAA, TAA, Vignette, ChromaticAberration, SSR, VolumetricLight, OutlineEffect
- AutoExposure (Eye Adaptation)
- JSON settings file (post_effects.json) with F12 save / startup load

**Phase 4 complete. Next: Phase 5 (描画レイヤーシステム).**

## Verification
- Build: OK (cmake -B build -S . && cmake --build build --config Debug)
- All effects toggleable via keyboard (1-9, 0, B, R, O, V, T, X)
- F12 saves post_effects.json, startup loads it
- No regressions in existing effects
