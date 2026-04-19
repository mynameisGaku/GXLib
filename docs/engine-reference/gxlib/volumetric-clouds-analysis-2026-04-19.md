# Volumetric Clouds — 現状分析 & AC7/AC8 到達ロードマップ

> **Date**: 2026-04-19
> **Author**: GXLib engineering (research session with Explore + general-purpose agents)
> **Purpose**: Permanent reference for the "upgrade VolumetricClouds to AC7/AC8 quality" initiative. Feeds ADR-0021 and the volumetric-clouds-ac-quality epic.
> **Scope**: technical inventory of current implementation, target technical definition, published-fact + industry-standard synthesis, gap analysis, phased delivery plan with realistic effort + risk.

---

## 1. 現状実装インベントリ

### 1.1 ファイル配置

| ファイル | 行数 | 役割 |
|---------|------|------|
| `GXLib/Graphics/3D/VolumetricClouds.h` | 284 | Public API, CloudConstants struct |
| `GXLib/Graphics/3D/VolumetricClouds.cpp` | 1034 | 3D noise 生成、temporal pipeline、Execute |
| `Shaders/VolumetricClouds.hlsl` | 570 | Main ray march、density sampling、lighting |
| `Shaders/VolumetricCloudsTemporal.hlsl` | 140 | Bilateral upsample + history blending |
| `GXLib/Graphics/PostEffect/PostEffectPipeline.{h,cpp}` | — | Pipeline 統合点 |
| `GXLib/Compat/DebugOverlay.cpp` (L199-273) | — | ImGui runtime tuning UI |

総 ~2100 行。単位テスト・視覚回帰テスト共に無し。

### 1.2 レンダリング技法

**Ray march**: 32 step 固定 (`marchSteps`) + LOD 加速 (`sampleCloudDensityLOD` 空域で `stepSize *= 2.0`、最大 4x)。Early-out at `transmittance < 0.01`。per-pixel IGN jitter + temporal offset。**Fully adaptive stepping ではない**。

**Density function** (Nubis/Schneider 系 6 段階):
1. Macro 配置: 3 octave FBM (2D) → procedural weather 代替
2. Height profile: altitude-gradient × 3 cloud type (stratus/stratocumulus/cumulus)
3. Shape noise: 128³ base texture or on-the-fly FBM4 fallback
4. Domain warping: 15% 相当、GBA channel から
5. Threshold remap: Nubis `remap(shape, 1.0 - macroMask, ...)`
6. Detail erosion + cauliflower upper sculpt

**3D noise textures**:
- **128³ base (RGBA8, ~2 MB)**: R=Perlin-Worley、G/B/A=Worley at 7/13/23 非整合周期
- **32³ detail (RGBA8, ~128 KB)**: R/G/B=Worley at 3/7/11
- Initialize() で CPU 側に約 2 秒かけて生成 → GPU upload

**Cloud type / coverage**:
- **単一 volumetric layer**、多層構造なし
- `coverage` (0-1) で閾値、`coverageVariation` で spread (stratus=0.25 ↔ cumulus=0.05)
- `cloudType` (0/0.5/1) で height gradient 補間

### 1.3 Lighting

**Single scattering**:
- Light march: 6 step cone (`stepSize *= 1.5` 指数)
- Beer-Lambert: `exp(-opticalDepth * attenuation)`
- Rayleigh 補正 sun color (atmosphere simplified coupling)

**Multi-scattering (Wrenninge 2015 式)**:
- 最大 8 octave cascade
- Per-octave: attenuation (0.3), contribution (0.7), eccentricity (0.5)
- **これは良くできている、AC7 相当レベル** ✅

**Beer-Powder + Silver lining**:
- `powder = 1 - exp(-od * 2)`
- `beerPowder = beer * lerp(1, powder, powderAmount)` (default 0.8)
- Silver lining = HG(cosθ, 0.99) × silverLining × 0.005

**Dual-lobe HG**:
- Forward: HG(cosθ, 0.76 × oEccen)
- Backward: HG(cosθ, -0.4 × oEccen) weight 0.2

**Ambient**:
- Height-lerp (ambientBottom=0.2, ambientTop=0.55)
- Shadow darkness 二次減衰 at bottom
- Dynamic ambient color from sun elevation (twilight/sunset/day)

**Shadowing**:
- Self-shadow via light march のみ
- **地形・オブジェクトへの cloud shadow なし** ❌
- Shadow cascade integration なし

### 1.4 Performance

- **Half-res render** (512×288 for 1024×576) → bilateral upsample
- Temporal reprojection + 3×3 min/max clamp (15% relaxation)
- Motion-adaptive blending (α 0.1 static → 0.5 fast)
- **No 1/16 checkerboard** (Nubis 1 流)
- 推定 1-3 ms at half-res、実測未記録
- GPU particle 不使用、pure ray march

### 1.5 Animation

- Wind: `windDirection × windSpeed × time × 0.002` (macro) + `× 0.01` (shape)
- Altitude shear: height-dependent wind offset
- Detail: 時間的 y-drift のみ
- **Curl noise advection なし** ❌
- **Morphing / breathing なし** ❌

### 1.6 Integration

- PostFX pipeline: SSR → **Clouds** → GodRays → Bloom → DoF → MB → TAA → Tonemap
- Sun transmittance readback (1-frame delayed) for god ray
- Atmosphere: Rayleigh 部分結合、Hillaire 2020 LUT 未結合
- Fog: 別系統 (VolumetricFog) で非結合
- **Game API**: coverage / windSpeed 等の setter のみ、`IsInCloud(pos)` 等の query 不在 ❌

### 1.7 欠落文書コメント

- TODO/FIXME/HACK marker なし
- ADR 未記載 (ADR-0008 Rendering は clouds を扱わない)
- No sprint issues filed for cloud quality
- **documented-as-shipped ではあるが、architecture 的未確定** ⚠️

### 1.8 現状サマリー

**Nubis 1 (SIGGRAPH 2015) 準拠の堅い基盤**。Multi-scattering + Beer-Powder + dual-lobe HG + temporal reprojection までは揃っている。
**ビジュアル単品で静止画品質 AC7 screenshot の 60-70%**、動的・gameplay 連動はほぼ未整備。

---

## 2. AC7 (2019) 技術事実

### 2.1 確定事項

| 項目 | 実装 |
|------|------|
| Host engine | Unreal Engine 4 (Project ACES 初の UE 使用) |
| Cloud middleware | **Simul trueSKY** (UE4 plugin 統合) |
| Target fps | PS4 campaign 60fps 目標 → 実測 30fps 近傍 (Kanno 発言) |
| Cloud 可視範囲 | trueSKY 4.2 以降 = 600 km 無限層対応 |
| 手法 | 低解像度 3D 体積への ray march + scene depth 合成 |
| データ構造 | Cloudspace 3D volume、precomputed inscatter table、separate godray volume |
| Authoring | **Sky sequencer (keyframe)** — artist が coverage/altitude keyframe author、runtime blend |
| 気象推移 | Keyframe 補間 (数分単位で fair → storm) |
| 性能 | Simul vendor claim 2-4 ms/frame (AC7 実測は未公開) |

### 2.2 確定: Gameplay 連動

これが AC7 を AC7 たらしめている部分:

- 雲中突入で視覚 hide、radar/HUD は継続 (stealth とは別機能)
- **Aircraft icing** — 長時間雲中で制御面凍結、ストール強制
- **Missile lock** — 時間延長 + range 減 + homing 劣化
- **DEW attenuation** — レーザー系兵器が著しく弱まる
- **Lightning** — 30 秒 avionics failure + 強制降下
- **Wind** — 機体と missile trajectory に影響

これら全てが実装されていたのが AC7。

### 2.3 未公開事項

- 正確な 3D texture resolution
- Ray-march step count
- Noise stack 詳細
- PS4 実測 ms budget

### 2.4 出典

- Kanno CEDEC 2019「エースコンバット7 空のイノベーション」(TaskForce 23 訳): skywardfm.com/cedec-2019-1
- 80.lv TrueSKY 解説: 80.lv/articles/truesky-making-skies-alive
- UE developer interview: unrealengine.com/en-US/developer-interviews/ace-combat-7-soars-high...

---

## 3. AC8 (2026 予定) 公開情報

### 3.1 確定事項

| 項目 | 内容 |
|------|------|
| Platform | PS5 / Xbox Series X\|S / PC |
| Host engine | **Unreal Engine 5** + **Lumen** (航空機 + 環境照明) |
| Cloud engine | **"Cloudly"** — 完全新規内製、trueSKY 置き換え |
| 開発チーム | Project ACES (Kono 監督 + Shimomoto プロデューサ) |
| 設計思想 | Kono 発言: 「improved visuals without improved functionality には興味ない」 |
| Asset scale | Aircraft ~4K textures (AC7 から 4x)、poly ~6x、map 10,000 km² 1:1 |

### 3.2 Cloudly 確定機能

- **Altitude 別 multi-layer** (雲層 = 自然な altimeter として機能)
- **Contrails / 煙 trail** が tactical 情報
- **Canopy 光反射** で相対位置示唆
- プレイヤーは計器見なくても雲層で高度判断可能 (Kono 設計意図)
- 気象は「signals、飾りではない」

### 3.3 Cloudly 非公開事項

- Underlying data structure (voxel / SDF / 3D noise)
- Lighting model 詳細
- UE5 の built-in `VolumetricCloud` 使うか全置き換えか
- GPU budget

### 3.4 業界推測 (確定ではない)

Kono の「functional 設計」発言 + altitude-layered altimeter 機能から、**voxel or SDF accelerated system** (Nubis Cubed 系譜) の可能性高い。pure 3D noise raymarcher では明示的 "this specific cumulus at altitude N" の shape authoring は難しい。ただし確証なし。

### 3.5 出典

- Automaton West: automaton-media.com/en/news/going-fast-will-feel-realer...
- PC Gamer: pcgamer.com/games/action/ace-combat-8-will-feature-custom-cloud-tech...
- Bandai Namco Europe dev diary: en.bandainamcoent.eu/ace-combat/news/ace-combat-8-wings-of-theve-developer-diary

---

## 4. 参照産業 State-of-the-Art

| 系統 | 代表作 | 年 | 貢献 |
|------|--------|-----|------|
| **Nubis 1** | Horizon Zero Dawn | 2015 | 基礎: 2.5D raymarch + Perlin-Worley。全modern cloudsの祖 |
| **Nubis authoring** | Decima engine | 2017 | **Modeler** 導入 = SDF primitive + regional control (artist 必須) |
| **Nubis Evolved** | Horizon Forbidden West | 2022 | Flyable + superstorm + 雷 + temporal artifact 対策 |
| **Nubis³ / Cubed** | (talks 2023) | 2023 | **Voxel base** に転換。SDF 圧縮 raymarch、fluid sim 駆動 modeling、dark-edge/inner-glow lighting |
| **Frostbite** | Battlefield 1 等 | 2016 | Physical atmosphere + clouds coupling (Hillaire) |
| **Frostbite scalable** | Battlefield V etc | 2020 | **Hillaire 2020 paper** = UE5 SkyAtmosphere の基盤 |
| **RDR2** | Red Dead Redemption 2 | 2019 | **2-layer weather map** + 世界 irradiance probe grid への feed |
| **MSFS 2020** | Microsoft Flight Sim | 2020 | **32 層** sea-level → stratosphere、METAR real-weather 駆動 |
| **UE5 built-in** | VolumetricCloud component | 2022+ | Baseline AAA、gameplay 連動不可 |

### 4.1 Nubis 標準ノイズ構成 (pixelsnafu + Meteoros 実装)

- **Base shape 128³ RGBA**: R=Perlin-Worley、G/B/A=Worley increasing frequency
- **Detail 32³ RGB**: higher-freq Worley for erosion
- **Weather map 512² or 1024² RGB**: R=coverage、G=precipitation、B=cloud type
- **Curl noise 128² 2D**: sample position displacement for wispiness
- **Height-density curve**: analytic, cumulus / stratus / cumulonimbus 別、重要

### 4.2 Light march + scattering

- Cone sampling: 6 samples in cone toward sun, last sample far
- Beer + Beer-Powder blend
- Dual-lobe HG (g=0.8 forward + g=-0.5 backward weighted 0.7/0.3)
- Multi-scattering octaves (Wrenninge): N=2-4 typically

### 4.3 Performance reality

| 実装 | 手法概要 | 実測 |
|------|---------|------|
| Nubis 1 (HZD PS4) | 3D noise + 1/16 temporal | **< 2 ms** |
| Nubis Evolved (HFW PS5) | 拡張 Nubis、1080p native | ~1 ms 通常 / 2-3 ms storm |
| AC7 (trueSKY PS4) | Low-res raymarch | vendor claim 2-4 ms |
| RDR2 (PS4 Pro) | Voxelized + 2-layer | ~2-3 ms 推定 |
| MSFS 2020 (PC high) | 32-layer METAR | 3-6 ms weather-dependent |
| UE5 VolumetricCloud | 半解像度 3D noise | 1-3 ms |

**60 fps (16.6 ms) 目標時の cloud pass: ≤3 ms が実用ライン**

---

## 5. AC クオリティの視覚的 4 条件 (プレイヤーが気付く差)

### 条件 1: Cloud identity が読める

Cumulus / stratus / cumulonimbus を一目で判別可能。噪音パラメタ調整では到達不能。**explicit cloud-type channel in weather map** + **per-type height-density curve** + **modeler (artist tool)** が必要。雲が全部同じ cotton field に見えるのが素人実装の最大の failure。

### 条件 2: Lighting が multi-scattered

Pure Beer-Lambert + HG では雲の内部が炭色。実雲は逆光時に明るい内部 (silver lining) + 横光時に暗い上部。2-4 octave Wrenninge multi-scattering が必須。**GXLib は既にこれは実装済み** ✅

### 条件 3: 世界と coupled

- 地形に cloud shadow (AC7: 海面が暗くなる)
- Atmosphere LUT 共有 (distant clouds aerial perspective)
- Irradiance probe grid へ cloud shadow transmittance を feed (世界全体が曇天で tinted)
- 飛行機 cockpit + 機体にも cloud shadow

GXLib は Rayleigh 部分結合のみ、他は未実装。

### 条件 4: Gameplay として機能

- `IsInCloud(point)` query (icing、radar、missile lock、laser attenuation)
- `DensityAlongRay(start, dir, length)` (laser / radar / radio range)
- `GetCloudTypeAt(point)` (tactical readout)
- Weather front transition API (storm start/stop)

Kono の "functional clouds" 発言 = この条件が AC シリーズの本質。

---

## 6. ギャップ分析 (優先度 × 効果)

| # | ギャップ | 優先度 | 視覚 | Gameplay | 実装コスト |
|---|---------|--------|------|----------|-----------|
| G1 | Multi-layer clouds (altitude 別) | 🔴 critical | ★★★★ | ★★★★ | 中 |
| G2 | Weather map 2D driven | 🔴 critical | ★★★★ | ★★★ | 中 |
| G3 | Gameplay query API | 🔴 critical | — | ★★★★★ | 低 |
| G4 | Terrain shadow (cloud → world) | 🟠 high | ★★★★ | ★★ | 中 |
| G5 | Atmosphere LUT 完全結合 (Hillaire 2020) | 🟠 high | ★★★ | ★ | 中 |
| G6 | Curl noise advection | 🟠 high | ★★★ | — | 低 |
| G7 | 1/16 checkerboard temporal | 🟡 medium | ★★ (stability) | — | 中 |
| G8 | Lightning / 内部発光 | 🟡 medium | ★★★★ (dramatic) | ★★ | 中 |
| G9 | Keyframe sequencer | 🟡 medium | ★★ | ★★★ | 中 |
| G10 | Precipitation particles | 🟢 low | ★★ | ★★★ | 中 |
| G11 | Artist tooling (weather editor) | 🟢 low | 間接 | 間接 | 大 |
| G12 | Voxel accelerator (Nubis Cubed) | 🟢 low | ★★ (vs Phase D) | — | 特大 |

---

## 7. 実装方針 — 段階的、throwaway 最小化

### 設計原則

1. **各 Phase が次 Phase への足場**、途中で捨てる patch は作らない
2. **Data model を先に固める** → rendering は後から差し替え可能
3. **Gameplay API を早期提供** (Phase 3) — 実ゲーム側が cloud に依存する設計を始められる
4. **Artist tools は core の一部** (bolt-on にしない)
5. **Voxel rewrite (Phase F) は optional** — Phase D 時点で AC7 同等、Phase F で AC8 class

### Phase A — Foundation docs (今セッション、完了予定)
- **A1** 本書 = `docs/engine-reference/gxlib/volumetric-clouds-analysis-2026-04-19.md`
- **A2** ADR-0021 Volumetric Cloud Architecture (multi-layer + weather-map + gameplay-API commitment)
- **A3** TR-cloud-* entries in tr-registry.yaml
- **A4** architecture-traceability 更新

Epic / stories は ADR が Accepted になってから `/create-epics` で起票。

### Phase B — Data model foundation (1-2 週間)
- **B1** `WeatherMap` class = 2D texture (512² RGB、runtime editable)
  - R = coverage
  - G = precipitation  
  - B = cloud type (low stratus → cumulus → cumulonimbus)
- **B2** `CloudLayer` struct: altitude band (low/mid/high)、per-type height-density curve
- **B3** `CloudField` = N `CloudLayer` + WeatherMap + Atmosphere reference
- **B4** 現 shader を WeatherMap sample に refactor (procedural FBM を差し替え)
- **B5** Unit test: WeatherMap 設定 → density 変化を確認

成果物: **既存視覚を保ちつつ data model が first-class になる**。

### Phase C — Gameplay query API (1 週間)
- **C1** `gx::CloudQuery::IsInCloud(Vec3 worldPos) -> bool`
- **C2** `gx::CloudQuery::DensityAlongRay(Vec3 start, Vec3 dir, float length) -> float`
- **C3** `gx::CloudQuery::GetCloudTypeAt(Vec3 worldPos) -> CloudType`
- **C4** Async GPU readback pipeline (1 frame delayed、cache friendly)
- **C5** Sample example: example 17? icing stub demo

成果物: **game code が cloud を query できる状態**、以後の gameplay 連動は game side で実装可能。

### Phase D — Render pipeline AC7 parity (3-4 週間)
- **D1** Multi-layer ray march (3 layer、depth-sorted、back-to-front)
- **D2** Atmosphere LUT 完全結合 (Hillaire 2020 準拠 transmittance + multi-scattering)
- **D3** Curl noise advection (morphing)
- **D4** 1/16 checkerboard temporal reconstruction
- **D5** Cloud-to-terrain shadow pass (low-res transmittance map, 2km²/4km² cascaded)
- **D6** ADR-0008 (Rendering) への addendum = FrameGraph 新 pass 登録

成果物: **AC7 trueSKY 同等視覚品質**。プレイヤーが AC7 screenshot と混同するレベル。

### Phase E — Storm + dynamics (2-3 週間)
- **E1** Lightning 実装 (local bright emitter、3D attenuation、flash timing、audio hook)
- **E2** Precipitation particles (cloud density → GPU particle emit、rain/snow)
- **E3** Keyframe sequencer (clear → storm を数秒〜数分で補間)
- **E4** Wind field 拡張 (storm で wind shift)

成果物: AC7 mission 4/5 級 storm sequence 再現可能。

### Phase F — AC8 class (optional、3-6 ヶ月)
- **F1** Voxel accelerator (Nubis Cubed-inspired、regional focus)
- **F2** SDF cloud primitives (artist-placable cumulus cluster)
- **F3** Fluid-sim-driven animation (局地的)
- **F4** Artist authoring tool (weather paint + keyframe editor、ImGui in-editor panel)
- **F5** Contrail + canopy droplet + cockpit rain effect

成果物: **AC8 Cloudly 級** (ただし Cloudly 内部未公開のため完全同等は claim 不可能)。

### Phase G — Production polish (ongoing)
- Performance profiling (sub-3ms budget validation across cloud types + storms)
- Regression test suite (golden image + frame budget monitoring)
- Documentation + reference-app showcase

---

## 8. 累積クオリティ到達見込み

| Phase 完 | 視覚 AC7 到達率 | Gameplay AC7 到達率 | AC8 Cloudly 到達率 |
|----------|-----------------|----------------------|---------------------|
| 現状 | 60-70% | 5% | 3% |
| B 完 | 65% | 15% | 5% |
| C 完 | 65% | 50% | 10% |
| D 完 | **90%** | 70% | 25% |
| E 完 | **95%** | **85%** | 40% |
| F 完 | 95% | 95% | **70-80%** |

**AC7 実用同等 (95% 以上): Phase E 完で到達可能 = engine 時間 3-4 ヶ月集中**
**AC8 Cloudly 級 = Phase F 完で 70-80%、100% は Cloudly 内部が公開されるまで不可能**

---

## 9. リスク + 前提

### リスク

- **R1 scope 膨張**: AC 系は addictive に polish 要求が無限湧きする。Phase ごとに ship 判定 + retrospective を必須化
- **R2 performance 乖離**: Multi-layer + shadow pass で 3ms 超過可能性。early profiling + fallback tier (LOW/MED/HIGH quality preset) を Phase D から導入
- **R3 artist tool 不在で author できず**: Phase F まで tool 無しだと Phase B-E の capability を活用できる data が存在しない。Phase B でせめて **JSON weather map format + default presets** を提供
- **R4 AC8 Cloudly 情報待ち**: 2026 release に向けて Project ACES が GDC/CEDEC で talk 出す可能性。出たら Phase F 設計を更新
- **R5 UE5 port 誘惑**: UE5 VolumetricCloud を GXLib に port したくなるが、GXLib の DX12 pipeline と UE5 RHI 差異 + ライセンス問題で非現実的。自前実装堅持

### 前提

- **P1** Target fps = 60、budget 16.6 ms、cloud pass ≤ 3 ms (Phase D 以降)
- **P2** Resolution 1080p (Phase D 実測)、4K は Phase G optimization 次第
- **P3** Single cloud field instance at a time (multi-world 対応は後回し)
- **P4** GXLib は DX12 Windows 専用、他 RHI は対応しない
- **P5** Phase B-F の総工数見積: **3-4 ヶ月 full-time engineer 1 人相当** (user 1 人 + Claude 支援前提で実時間は 6-9 ヶ月に伸びる)

---

## 10. 次アクション (本ドキュメント確定後)

1. **ADR-0021 Volumetric Cloud Architecture** 起票 (本書と同時 commit)
2. **TR-cloud-001 〜 TR-cloud-020** を tr-registry.yaml に追加
3. **architecture-traceability.md** に cloud TR を登録
4. ADR-0021 `/architecture-review` fresh session で Accepted 昇格確認
5. 昇格後、`/create-epics volumetric-clouds` で epic 起票
6. Phase B story 分解 → sprint-005 候補に

本書 + ADR-0021 が blueprint。実装は別セッションで。

---

## Appendix A — 参照資料

### Project ACES

- [CEDEC 2019 AC7 Sky talk (Kanno, TaskForce 23 訳)](https://www.skywardfm.com/cedec-2019-1)
- [AC7 Clouds + Weather gameplay](https://www.skywardfm.com/post/the-form-and-function-of-clouds-and-weather-in-ace-combat-7)
- [UE developer interview on AC7](https://www.unrealengine.com/en-US/developer-interviews/ace-combat-7-soars-high-with-ue4-to-become-franchise-s-best-installment)
- [AC8 Cloudly details (Automaton)](https://automaton-media.com/en/news/going-fast-will-feel-realer-than-ever-in-ace-combat-8-thanks-to-new-proprietary-cloud-engine-devs-detail-new-features-and-technology/)
- [AC8 Kono on functional clouds (PC Gamer)](https://www.pcgamer.com/games/action/ace-combat-8-will-feature-custom-cloud-tech-but-not-just-for-prettier-skies-i-personally-am-not-particularly-interested-in-improved-visuals-without-improved-functionality/)
- [AC8 UE5 + Cloudly confirmation](https://automaton-media.com/en/news/ace-combat-8-wings-of-theve-brings-new-in-house-cloud-engine-a-new-take-on-cinematic-scenes-and-10000-square-kilometer-map/)

### Nubis 系譜

- [HZD Volumetric Cloudscapes SIGGRAPH 2015 PDF](https://advances.realtimerendering.com/s2015/The%20Real-time%20Volumetric%20Cloudscapes%20of%20Horizon%20-%20Zero%20Dawn%20-%20ARTR.pdf)
- [Nubis Decima Authoring SIGGRAPH 2017 PDF](https://advances.realtimerendering.com/s2017/Nubis%20-%20Authoring%20Realtime%20Volumetric%20Cloudscapes%20with%20the%20Decima%20Engine%20-%20Final%20.pdf)
- [Nubis Cubed overview](https://www.guerrilla-games.com/read/nubis-cubed)
- [Nubis Evolved (HFW)](https://www.guerrilla-games.com/read/nubis-evolved)
- [Schneider publications](https://www.schneidervfx.com/)

### Frostbite / Atmosphere

- [Frostbite Sky/Atmosphere/Clouds SIGGRAPH 2016](https://media.contentapi.ea.com/content/dam/eacom/frostbite/files/s2016_pbs_frostbite_sky_clouds.pdf)
- Hillaire 2020 paper (Scalable Sky/Atmosphere) — search "A Scalable and Production Ready Sky and Atmosphere"

### RDR2 / MSFS

- [SIGGRAPH 2019 Advances RDR2 slides index](https://advances.realtimerendering.com/s2019/index.htm)
- [imgeself RDR2 graphics study](https://imgeself.github.io/posts/2020-06-19-graphics-study-rdr2/)
- [MSFS live weather pipeline](https://flightsimulator.blog/microsoft-flight-simulator-live-weather/)

### Implementation references

- [pixelsnafu Volumetric Clouds resources](https://gist.github.com/pixelsnafu/e3904c49cbd8ff52cb53d95ceda3980e)
- [AmanSachan1/Meteoros (Vulkan Decima re-impl)](https://github.com/AmanSachan1/Meteoros)
- [UE5 VolumetricCloud component](https://dev.epicgames.com/documentation/en-us/unreal-engine/volumetric-cloud-component-in-unreal-engine)
- [trueSKY rendering docs](https://docs.simul.co/programming/rendering.html)

---

*本書はリビングドキュメント。Phase 進行ごとに「現状実装インベントリ」セクションを更新する。新 Phase の retrospective 結果も Section 8 の表に反映する。*
