# GXLib 作業状況分析レポート

**日付:** 2026-02-21
**前回分析:** 2026-02-11 (ProjectAnalyze_2026-02-11.md)
**前回バグレポート:** 2026-02-17 (BugReport.md)

---

## 1. 未コミット変更の全体像

最終コミット `fa53e0e` (Fix FBX法線反転) 以降、**59ファイル変更 (+11,249/-861行)** + **新規11ファイル** が未コミット。

| 変更範囲 | ファイル数 | 追加行数(概算) | 内容 |
|----------|----------|------------|------|
| GXLib本体 (Graphics/3D, Resource, IO) | 17 modified + 2 new | ~1,200 | Toon/Outline/Viewer API/FBX修正/PakFileProvider |
| Shaders | 4 modified + 1 new | ~350 | UTS2 Toon, ToonOutline, ShadowUtils, ShaderModelCommon, InfiniteGrid |
| gxformat/gxconv/gxloader | 5 modified | ~250 | ShaderModelParams UTS2化, FBX IBM修正, ワイド文字パス |
| GXModelViewer | 22 modified + 8 new | ~1,600 (独自コード) | Docking/Viewport RT/ギズモ/シャドウ/ピッキング/パネル群 |
| ImGui ThirdParty (docking branch) | 12 modified | ~8,240 | master → docking branch 移行 |

---

## 2. 未コミット変更の機能別分類

### A. GXLib本体への変更（エンジンライブラリ）

#### A-1. UTS2ベースToonシェーダー再構築（Phase 20-21）
- **ShaderModelParams** (`gxformat/shader_model.h`): 旧ramp/band方式を削除、UTS2方式に置換。3ゾーンシェード (shadeColor/shade2ndColor, baseColorStep/baseShadeFeather, shadeColorStep/shade1st2ndFeather)。Phong/SS/CC領域を7エイリアスアクセサで再利用 (rimLightDirMask等)。256B維持。
- **ShaderModelGPUParams** (`ShaderModelConstants.h`): GPU cbuffer配置をUTS2対応。ConvertToGPUParamsにToon分岐追加。
- **ShaderModelCommon.hlsli**: cbuffer変数名をUTS2対応に変更。7つの`#define`マクロでToonエイリアス定義。GetLightShadow()修正。
- **Toon.hlsl**: 完全リライト。halfLambert+CSM統合、片側ランプ3ゾーン遷移、UTS2スペキュラ(power→threshold変換)、UTS2リム(insideMaskリマッピング+方向マスク)。
- **ShadowUtils.hlsli**: k_ShadowSoftness追加、ポイントシャドウ3x3→5x5 PCFアップグレード、キューブフェイス境界マージンクランプ。

#### A-2. スムース法線転写Toonアウトライン（Phase 19）
- **Mesh.h/cpp**: `CreateSmoothNormalBuffer()` (slot 1 VB, stride 12)、`ComputeSmoothNormals()` (位置量子化0.0001→unordered_mapグループ化→法線平均化)。
- **Vertex3D.h**: `k_Vertex3DPBROutlineLayout` / `k_Vertex3DSkinnedOutlineLayout` (slot 0=既存 + slot 1=SMOOTHNORMAL)。
- **ShaderRegistry.cpp/h**: アウトラインPSOに新レイアウト適用、DepthBias(500,0,2)追加。
- **ToonOutline.hlsl**: 完全リライト。スムース法線ベースの押し出し（ハイブリッド：凸領域はscaleDir、凹領域はスムース法線）。ワールド空間でposH.wスケール。UTS2距離減衰、シャドウ統合、baseColor^2暗化。
- **ModelLoader.cpp**: glTF/FBXローダーにスムース法線計算を追加。
- **GxmdModelLoader.cpp**: GXMDローダーにもスムース法線計算を追加。

#### A-3. Viewer/Editor向けAPI拡張（Phase 17）
- **Animator.h/cpp**: `GetCurrentClip/GetCurrentTime/SetCurrentTime/GetSpeed/SetSpeed/IsPaused`、ルートモーションロック (`SetLockRootPosition/Rotation`)。
- **AnimatorStateMachine.h**: `GetTransitions()` 公開。
- **Camera3D.h/cpp**: `SetPitch(float)` / `SetYaw(float)` 追加（オービットカメラ用）。
- **Renderer3D.h/cpp**: `SetWireframeMode(bool)`（ワイヤフレームPSO 2種）、`SetMaterialOverride/ClearMaterialOverride`、`DrawModel/DrawSkinnedModel(submeshVisibility)`。Toonアウトライン2スロットVBバインド。シャドウDepthBias強化。
- **TextureManager.h/cpp**: `GetFilePath(int handle)` 追加。

#### A-4. FBXインポート改善
- **ModelLoader.cpp** (+408行): `FbxAxisSystem::ConvertScene()` 廃止→手動Z反転RH-to-LH変換。ルートボーンに`EvaluateGlobalTransform()`使用。UV事前キャッシュ（FBX SDKバグ回避）。5段階テクスチャパス検索。全プロパティフォールバック。非スキンメッシュのノード変換適用。
- **gxconv/fbx_importer.cpp** (+113行): `space_conversion=ADJUST_TRANSFORMS`。ルートボーン `node_to_world` 使用。IBM転置修正。ルートボーンアニメに `parentWorld*localMat` 合成。

#### A-5. GXPAK VFS統合（Phase 18）
- **PakFileProvider.h/cpp** (新規): `IFileProvider` 実装、`gxloader::PakLoader` 使用、Priority=100。

#### A-6. ワイド文字パスローダー
- **gxloader model_loader.h/cpp**: `LoadGxmdW(wstring)` 追加。
- **gxloader anim_loader.h/cpp**: `LoadGxanW(wstring)` 追加。

### B. GXModelViewer変更

#### B-1. ImGui Docking移行 + DockSpaceレイアウト（Phase 22）
- ImGui master → docking branch 1.92.6（+8,240行）
- `DockSpaceOverViewport()` によるドッキング有効化
- 3Dシーン → m_viewportRT(LDR) → `ImGui::Image()` 表示
- ビューポートウィンドウサイズ駆動のリサイズ（SwapChainはOSウィンドウに依存）
- タブ化: Inspector(Properties/ModelInfo/Skeleton)、Rendering(Lighting/PostEffect/Skybox)

#### B-2. 大規模機能追加（Phase 17拡張）
- **ImGuizmoギズモ統合**: Translate/Rotate/Scale、ワールド/ローカル切替、スナップ、ビューポートツールバー
- **CSMシャドウパス**: 4カスケード+スポット+ポイントシャドウ完全統合
- **InfiniteGrid** (新規3ファイル): シェーダーベース無限グリッド、Y=0面、軸カラー、距離フェード
- **ビューポートピッキング**: レイキャスト→AABB→最近接ヒット、CPUスキニングAABB
- **ボーン可視化**: 関節スフィア+ボーンライン+選択ボーン軸表示
- **AABB可視化**: OBBワイヤフレーム描画
- **D&Dインポート**: WM_DROPFILES + ImGuiドラッグドロップ
- **アニメーションインポート**: .gxan/.fbx/.gltf/.glb対応、ボーン名リマッピング
- **キーボードショートカット**: Space/F/W/T/E/R/L/B
- **オービットカメラ改善**: ビューポート限定入力、ギズモ排他、選択ボーン追従

#### B-3. 新規パネル（Phase 17）
- **AssetBrowserPanel**: ファイル/フォルダブラウザ、D&D対応
- **ModelInfoPanel**: 頂点数/三角形数/VB,IBサイズ/AABB/ボーン数/アニメ一覧
- **SkeletonPanel**: ボーン階層ツリー、選択ボーンTRS表示、ワールド座標、IBM表示

#### B-4. パネル改善
- **PropertyPanel**: ギズモセクション、レンダリングセクション、サブメッシュ可視性、UTS2全パラメータエディタ
- **SceneHierarchyPanel**: スキンドエンティティのボーンツリー展開
- **TimelinePanel**: 完全リライト（トランスポート制御、スクラブ、速度プリセット、ルートロック）
- **AnimatorPanel**: 遷移リンク描画
- **各パネル**: DrawContent()メソッド追加（タブ埋め込み対応）
- **ModelExporter**: TextureManager引数追加（テクスチャパス保存）

---

## 3. 完成条件 (G1-G5) 達成状況

| # | 完成条件 | 状態 | 備考 |
|---|---------|------|------|
| G1 | DXライブラリの全APIカテゴリを網羅 | **達成** | Phase 1-9で全カテゴリカバー |
| G2 | ポストエフェクトパイプライン標準搭載 | **達成** | 13エフェクト + JSON設定 |
| G3 | 描画レイヤーシステム動作 | **達成** | RenderLayer + LayerCompositor + MaskScreen |
| G4 | XMLベースGUIシステム | **達成** | 16種Widget + XML/CSS + C++バインド |
| G5 | サンプルプロジェクト群動作 | **達成** | 5サンプル |

---

## 4. Directive計画 vs 現状のギャップ分析

### 4.1 Directiveに記載あり・未実装の機能

| カテゴリ | 機能 | Directive記載箇所 | 現状 |
|---------|------|-----------------|------|
| 3D | **インスタンシング描画** | 3.3 モデル描画 | **未実装** — DrawModel は1体ずつ |
| 3D | **LODシステム** | 3.3 地形 / Phase 3 | **未実装** — 固定LODのみ |
| 3D | **IBL (Image Based Lighting)** | 3.3 環境マップ | **未実装** — Skyboxはあるがイラディアンスマップ/BRDF LUT なし |
| 3D | **モーフターゲット** | Phase 3 glTFローダー | **未実装** |
| 3D | **Area Light** | 3.3 ライティング | **未実装** |
| Shader | **コンピュートシェーダーパイプライン** | 3.5 | **部分的** — PostEffect内で使用されるがユーザーAPI未公開 |
| Sound | **3Dサウンド (X3DAudio/HRTF)** | 3.6 | **未実装** |
| Sound | **オーディオミキサー (バス/エフェクトチェーン)** | 3.6 | **未実装** |
| Sound | **リアルタイムエフェクト (リバーブ/EQ)** | 3.6 | **未実装** |
| Input | **タッチ入力** | 3.7 | **未実装** |
| Input | **アクションマッピング** | 3.7 | **未実装** |
| Input | **入力バッファリング (格ゲー向け)** | 3.7 | **未実装** |
| Text | **SDFフォントレンダリング** | 3.8 | **DirectWrite方式** — SDF未実装 |
| Text | **リッチテキスト (色・サイズ混在)** | 3.8 | **未実装** |
| Text | **BMFont対応** | 3.8 | **未実装** |
| Core | **DPI対応 (Per-Monitor V2)** | 3.1 | **未実装** |
| Core | **マルチウィンドウ** | 3.1 | **未実装** |
| Core | **EventBus / Delegate** | 5章 ディレクトリ構成 | **未実装** |
| Core | **ConfigManager (JSON/INI)** | 5章 ディレクトリ構成 | **PostEffectSettings のみ** |
| Compat | **DXLib互換API完全網羅** | 3.1-3.12 全項目 | **部分的** — 主要API対応済み、全関数網羅は未検証 |

### 4.2 Directiveに記載なし・追加実装済みの機能

| 機能 | Phase | 備考 |
|------|-------|------|
| DXR レイトレーシング反射 | 11 | RTAccelerationStructure/RTPipeline/RTReflections |
| DXR RTGI グローバルイルミネーション | 12 | 半解像度GI+テンポラル蓄積+A-Trous空間フィルタ |
| アセットパイプライン (gxformat/gxconv/gxloader/gxpak) | 13 | OBJ/FBX/glTF→GXMD/GXAN, LZ4圧縮PAK |
| ShaderRegistry + シェーダーモデルPSO | 14 | 12 PSO (6モデル×2バリアント) |
| アニメーションブレンドシステム | 15 | BlendStack/BlendTree/AnimatorStateMachine |
| GXModelViewer (ImGui 3Dモデルビューア) | 16-22 | 12+パネル, docking, ギズモ, シャドウ |
| UTS2 Toonシェーダー | 18-21 | 3ゾーンシェード, スムース法線アウトライン |
| GXPAK VFS統合 | 18 | PakFileProvider |

---

## 5. 既知バグ状況 (BugReport.md, 2026-02-17)

| 重要度 | 件数 | 修正済み | 未修正 | 主要な未修正項目 |
|--------|------|---------|--------|----------------|
| Critical | 6 | 0 | 6 | DropDown/ListView配列外, TextureManager負インデックス, RT-C01〜C03 |
| High | 24 | 0 | 24 | vswprintf_sバッファサイズ, Map null未チェック, スレッド安全性 |
| Medium | 32 | 0 | 32 | TextRenderer改行, 各種エラーハンドリング |
| Low | 14 | 0 | 14 | 除算ゼロガード, float精度 |
| **合計** | **76** | **0** | **76** | |

### 計算式バグ (MATH-01〜05)
- MATH-01: Quaternion::ToEuler() 符号誤り — **未修正**
- MATH-02: PhysicsWorld2D 角トルクに質量逆数を使用 — **未修正**
- MATH-03: PhysicsWorld2D AABB回転無視 — **未修正**
- MATH-04: RT ClosestHit 法線変換で非一様スケール未対応 — **未修正**
- MATH-05: DoF ガウス重み正規化0.4%超過 — **未修正**

---

## 6. DxLib置き換えの視点で「次にやるべきこと」

**Directiveの核心:** 「DXライブラリの完全上位互換フレームワークをDirectX 12ベースでゼロから構築する」

### 6.1 DxLib互換で不足している機能 (優先度: 高→低)

| 優先度 | 機能 | DxLib該当API | 理由 |
|--------|------|-------------|------|
| **高** | インスタンシング描画 | `MV1DrawModel`(大量描画時) | Directiveに明記。同一メッシュ大量描画で必須 |
| **高** | 3Dサウンド + ミキサー | `Set3DPositionSoundMem`, AudioMixer | Directiveに明記。3Dゲームで必須 |
| **高** | アクションマッピング | (DxLibになし/新規) | Directiveに明記。入力設定ファイルで再マップ |
| **中** | IBL (Image Based Lighting) | (DxLibになし/新規) | Directive 3.3 環境マップ。PBR品質向上の鍵 |
| **中** | SDFフォントレンダリング | `DrawString`の上位互換 | Directive 3.8。現在DirectWrite方式→SDF化で品質・スケーラビリティ向上 |
| **中** | タッチ入力 | `GetTouchInputNum` | Directive 3.7。Windowsタブレット/Surface対応 |
| **中** | リッチテキスト / BMFont | (DxLibになし/新規) | Directive 3.8。ゲーム内テキスト表現力 |
| **低** | DPI対応 | (DxLibになし/新規) | Directive 3.1。マルチモニタ対応 |
| **低** | LODシステム | 地形・モデル | Directive Phase 3。大規模シーンで必須 |

### 6.2 Directiveに明記されていないが価値のある追加機能

| 機能 | DxLib互換性の観点 | 理由 |
|------|-----------------|------|
| **パーティクルシステム** | DxLibにはない→新規差別化 | 2D/3Dエフェクト表現。GPUパーティクル(Compute)で大量描画 |
| **IK (逆運動学)** | DxLibにはない→新規差別化 | 足接地・視線追従。アニメーション品質向上 |

---

## 7. ビルド状態

```
cmake --build build --config Debug → 成功 (2026-02-21 確認)
```

- GXLib.lib: OK
- Sandbox.exe: OK
- GXModelViewer.exe: OK
- 5 Samples: 未確認（CMake再生成が必要な可能性）

---

## 8. 推奨アクション

### 即座に実施すべき
1. **未コミット変更のコミット** — 59ファイル+11新規ファイルが未コミット。Phase 17-22 相当の大量変更がリスクにさらされている

### DxLib置き換えとして次に取り組むべき
2. **インスタンシング描画** — Directiveに明記、3Dゲーム必須
3. **3Dサウンド + オーディオミキサー** — Directiveに明記、3Dゲーム必須
4. **アクションマッピング** — Directiveに明記、入力管理の標準化
5. **パーティクルシステム** — DxLibにない新規差別化機能、表現力大幅向上

### バグ修正（上位のみ）
6. **Critical 6件** — クラッシュ直結のため早期修正推奨
7. **MATH-02** (PhysicsWorld2D角トルク) — 物理シミュレーション全体に影響
