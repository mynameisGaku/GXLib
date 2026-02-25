# GXLib Phase 36-40 総合実装指令書

## Context

Phase 0-35 完了済み。エンジンは完全な 2D/3D 描画パイプライン、DXR レイトレーシング（反射＋GI）、
15+ ポストエフェクト、GUI、物理(2D + Jolt 3D)、オーディオ(XAudio2 + 3D)、シーングラフ/ECS、
パーティクル(CPU 2D/3D + GPU)、ナビメッシュ、LOD、デカール、IBL、IK、GPU インスタンシング、
アクションマッピング、GXModelViewer（ImGui Docking + 19 パネル）、アセットパイプライン
（gxformat/gxconv/gxloader/gxpak）、22 サンプルプロジェクトを持つ成熟状態。

本指令書は **バグ修正・テスト強化・ドキュメント改善・新機能追加** の 5 フェーズを網羅し、
任意の Claude インスタンスが独立して各 Phase を実装できることを目的とする。

**重要**: GXModelViewer はあくまでも 3D モデルを読み込み・編集・GXLib 独自形式に変換するツールであり、
シーンエディタ化は本指令書のスコープ外。

---

## 全体ロードマップ

| Phase | 名称 | 概要 | 依存 |
|-------|------|------|------|
| **36** | バグ修正 & コード品質 | TODO/FIXME 解消、エッジケース修正、コード衛生 | なし |
| **37** | テスト強化 | 既存テスト拡充 + 未カバー領域の新規テスト追加 | なし |
| **38** | ドキュメント改善 | チュートリアル刷新、API リファレンス、用語集、サンプル解説 | なし |
| **39** | 新エンジン機能（前半） | マルチスレッドレンダリング、Async Compute、間接描画 | Phase 36 |
| **40** | 新エンジン機能（後半） | Lua スクリプティング、2D タイルマップ、ルートモーション | Phase 36 |

Phase 36/37/38 は並列着手可能。Phase 39/40 は Phase 36 完了後に着手。

---

## Phase 36: バグ修正 & コード品質

### 目的
`BugReport.md`（プロジェクトルート）に記載された 76 件のバグ + TODO/FIXME/HACK コメントを
体系的に修正し、エンジンの信頼性を底上げする。

> **入力ドキュメント**: `C:\Users\g0190\Desktop\GXLib\BugReport.md`
> 全体スキャン(53件) + RT詳細調査(18件) + 計算式・手法検証(10件) = 重複除外 **76件**

### 36a: TODO/FIXME 精査手順

1. 以下のコマンドで全 TODO/FIXME/HACK を抽出:
   ```bash
   grep -rn "TODO\|FIXME\|HACK\|XXX\|WORKAROUND" GXLib/ --include="*.cpp" --include="*.h"
   grep -rn "TODO\|FIXME\|HACK" Shaders/ --include="*.hlsl" --include="*.hlsli"
   ```

2. 各項目を以下のカテゴリに分類:
   - **Critical**: クラッシュ、データ破壊、セキュリティ問題
   - **High**: 機能不全、パフォーマンス深刻劣化
   - **Medium**: 機能制限、非最適なコード
   - **Low**: コスメティック、コメント修正
   - **Won't Fix**: 意図的な制限、将来対応

3. Critical → High → Medium の順に修正

### 36b: BugReport.md 全バグ一覧と修正方針

以下は `BugReport.md` から抽出した全バグの修正指示。
**修正順序は Priority Order に従うこと。**

---

#### 36b-1: Critical (6件) — 最優先

##### C-01: DropDown — 配列の範囲外アクセス
- **File:** `GXLib/GUI/Widgets/DropDown.cpp:178`
- **問題:** `RenderSelf()` 内のラムダで `wideItems[i]` にアクセスするが、`m_items` と `m_wideItems` のサイズ不一致時に範囲外アクセス
- **修正方針:** ラムダ内で `i < wideItems.size()` のガードを追加。`SetItems()` でも `m_wideItems` を `m_items` と同期的に更新することを保証
  ```cpp
  if (i < wideItems.size()) { /* 既存の描画処理 */ }
  ```

##### C-02: ListView — 配列の範囲外アクセス
- **File:** `GXLib/GUI/Widgets/ListView.cpp:134`
- **問題:** `m_wideItems[i]` にアクセスする際、`m_wideItems` のサイズが `m_items` と一致することを検証していない
- **修正方針:** C-01 と同様、アクセス前にサイズチェック。`SetItems()` で両配列を同時にリサイズ

##### C-03: TextureManager::CreateRegionHandles — 負インデックスで配列アクセス
- **File:** `GXLib/Graphics/Resource/TextureManager.cpp:184-187`
- **問題:** `AllocateHandle()` 失敗時の返り値を検証せず `m_entries[handle]` にアクセス
- **修正方針:**
  ```cpp
  int handle = AllocateHandle();
  if (handle < 0)
  {
      GX::Logger::Error("TextureManager: Failed to allocate region handle");
      return -1;  // or continue to skip this region
  }
  m_entries[handle] = /* ... */;
  ```

##### RT-C01: Sandbox で CreateGeometrySRVs() が呼ばれていない
- **File:** `Sandbox/main.cpp`
- **問題:** BLAS 構築後に `CreateGeometrySRVs()` が呼ばれていない → ジオメトリ VB/IB SRV とアルベドテクスチャ SRV が未初期化のまま DispatchRays 実行 → GPU ハング
- **修正方針:** BLAS 構築 + GPU フラッシュ後に `g_rtReflections->CreateGeometrySRVs();` を追加
- **参考:** `Samples/DXRShowcase/main.cpp:459` が正しい呼び出し例

##### RT-C02: リソースポインタの寿命管理不備（Use-After-Free リスク）
- **File:** `GXLib/Graphics/RayTracing/RTReflections.h:184-196`
- **問題:** `m_textureResources` と `m_blasGeometry` が `std::vector<ID3D12Resource*>`（生ポインタ）で保持。テクスチャやメッシュが外部で解放されるとダングリングポインタ
- **修正方針:** `std::vector<ComPtr<ID3D12Resource>>` に変更
  ```cpp
  // Before:
  std::vector<ID3D12Resource*> m_textureResources;
  std::vector<BLASGeometryInfo> m_blasGeometry;  // 内部に ID3D12Resource* あり

  // After:
  std::vector<ComPtr<ID3D12Resource>> m_textureResources;
  // BLASGeometryInfo の ID3D12Resource* も ComPtr に変更
  ```

##### RT-C03: ディスクリプタヒープ スロット衝突（frameIndex >= 2 の場合）
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:341`
- **問題:** `heapBase = frameIndex * 4` でフレーム毎のスロット計算。ジオメトリ SRV は固定スロット 8 から開始。frameIndex=2 で衝突。現在 `k_BufferCount=2` なので発動しないが脆弱
- **修正方針:** フレーム毎スロットをジオメトリ/テクスチャスロットの後に配置
  ```cpp
  // ジオメトリ SRV: [0..31], テクスチャ SRV: [32..63], フレーム固有: [64 + frameIndex * 4]
  constexpr uint32_t k_FrameSlotBase = 64;
  uint32_t heapBase = k_FrameSlotBase + frameIndex * 4;
  ```

---

#### 36b-2: High (24件) — 第2優先

##### H-01: TextRenderer — vswprintf_s のバッファサイズ引数欠落
- **File:** `GXLib/Graphics/Rendering/TextRenderer.cpp:157`
- **修正:** `vswprintf_s(buffer, 1024, format, args)` に第2引数を追加

##### H-02: SpriteBatch::Begin — Map の戻り値未チェック
- **File:** `GXLib/Graphics/Rendering/SpriteBatch.cpp:199`
- **修正:**
  ```cpp
  m_mappedVertices = static_cast<SpriteVertex*>(m_vertexBuffer.Map(frameIndex));
  if (!m_mappedVertices) { GX::Logger::Error("SpriteBatch: VB Map failed"); return; }
  ```

##### H-03: PrimitiveBatch::Begin — Map の戻り値未チェック
- **File:** `GXLib/Graphics/Rendering/PrimitiveBatch.cpp:133-134`
- **修正:** `m_mappedTriVertices` と `m_mappedLineVertices` 両方に null チェック追加

##### H-05: AutoExposure — マップドポインタの未検証デリファレンス
- **File:** `GXLib/Graphics/PostEffect/AutoExposure.cpp:231`
- **修正:** `if (!mapped) return;` ガード追加

##### H-06: RTReflections::OnResize — HRESULT 未チェック
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:574-577`
- **修正:** `CreateCommittedResource` の戻り値を `FAILED(hr)` でチェックし、失敗時に LOG_ERROR + return

##### H-07: PostEffectPipeline — null リソースへの UAV バリア
- **File:** `GXLib/Graphics/PostEffect/PostEffectPipeline.cpp:440-446`
- **修正:** `if (m_halfResUAV.Get())` で null チェック後にバリア発行

##### H-08: RTReflections — m_normalRT の null チェック不完全
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:308-360`
- **修正:** 全コードパスで `if (!m_normalRT)` チェックを統一

##### H-09: SSR — normalRT の SRV バインド未検証
- **File:** `GXLib/Graphics/PostEffect/SSR.cpp:138`
- **修正:** `UpdateSRVHeap` で normalRT のリソース有効性を事前チェック

##### H-10: HTTPClient — 非同期操作のリソースリーク
- **File:** `GXLib/IO/Network/HTTPClient.cpp:195-210`
- **修正:** `.detach()` → `std::jthread` に変更するか、デストラクタで `m_running=false` + join。`this` キャプチャの代わりに `shared_from_this()` を使用
  ```cpp
  // デストラクタに追加
  ~HTTPClient()
  {
      m_running = false;
      // 全非同期スレッドの完了を待つ
  }
  ```

##### H-11: WebSocket — ReceiveLoop の Use-After-Free
- **File:** `GXLib/IO/Network/WebSocket.cpp:115-250`
- **修正:** `Close()` で `m_running=false` 設定後、`m_hWebSocket` 操作を mutex で保護
  ```cpp
  std::mutex m_socketMutex;
  // ReceiveLoop 内: std::lock_guard lock(m_socketMutex); でハンドルアクセスを保護
  // Close() 内: lock 取得後にハンドル無効化
  ```

##### H-12: AsyncLoader — 破棄時のレースコンディション
- **File:** `GXLib/IO/AsyncLoader.cpp:12-21`
- **修正:** デストラクタで `m_running.store(false)` 後に `m_cv.notify_all()` + `m_thread.join()`。完了キュー/ステータスマップの操作を mutex で保護

##### H-13: MoviePlayer — null ポインタデリファレンス
- **File:** `GXLib/Movie/MoviePlayer.cpp:104, 288-299`
- **修正:** `if (pOutputType) pOutputType->Release();` で null チェック。`Close()` 後の `m_texManager` 使用箇所に null ガード追加

##### H-14: Compat_2D LoadDivGraph — null ポインタ・バッファオーバーフロー
- **File:** `GXLib/Compat/Compat_2D.cpp:61-81`
- **修正:**
  ```cpp
  if (!handleBuf || allNum <= 0) return -1;
  // allNum の上限チェック（例: 1024）
  if (allNum > 1024) { GX::Logger::Error("LoadDivGraph: allNum too large"); return -1; }
  ```

##### H-15: RTReflections.hlsl — cbuffer コメントとアクセスの不一致
- **File:** `Shaders/RTReflections.hlsl:40-44, 238-239`
- **修正:** コメントを更新して `.z=texIdx, .w=hasTexture` を文書化。C++ 側 (`RTReflections.cpp:201`) で全4成分が設定されていることを確認（既に設定済み → コメント修正のみ）
  ```hlsl
  // g_InstanceRoughnessGeom[i]: .x=roughness, .y=geometryIndex, .z=texIdx, .w=hasTexture
  ```

##### H-16: Texture — 大サイズテクスチャでの整数オーバーフロー
- **File:** `GXLib/Graphics/Resource/Texture.cpp:104-106, 246-248`
- **修正:** `rowPitch * height` 計算を `uint64_t` で行う
  ```cpp
  uint64_t totalSize = static_cast<uint64_t>(rowPitch) * height;
  ```

##### H-17a: BarrierBatch — m_barriers 配列の未初期化
- **File:** `GXLib/Graphics/Device/BarrierBatch.h:22, 62-64`
- **修正:** `m_barriers` をゼロ初期化: `D3D12_RESOURCE_BARRIER m_barriers[16] = {};`

##### H-17b: DropDown::OnEvent — 空アイテム時の範囲外アクセス
- **File:** `GXLib/GUI/Widgets/DropDown.cpp:61`
- **修正:** `if (m_items.empty()) return;` ガードを `onValueChanged` 呼び出し前に追加

##### H-18a: FontManager — pixelData の null チェック欠如
- **File:** `GXLib/Graphics/Rendering/FontManager.cpp:313`
- **修正:** `if (!pixelData) { GX::Logger::Error("FontManager: pixelData is null"); return; }`

##### H-18b: TextInput::DeleteSelection — 選択範囲の境界値不正
- **File:** `GXLib/GUI/Widgets/TextInput.cpp:93`
- **修正:** `s` と `e` をクランプ: `s = (std::max)(0, (std::min)(s, (int)m_text.size())); e = (std::max)(s, (std::min)(e, (int)m_text.size()));`

##### H-19: ScrollView — ゼロ除算リスク
- **File:** `GXLib/GUI/Widgets/ScrollView.cpp:25`
- **修正:** `if (viewH <= 0) return;` ガード追加

##### H-20: 複数ウィジェット — m_renderer の null チェック不統一
- **Files:** `Button.h, TextWidget.h, CheckBox.h, DropDown.h, ListView.h, RadioButton.h, TabView.h, TextInput.h`
- **修正:** 全 `RenderSelf()` メソッド冒頭に `if (!m_renderer) return;` を追加（未対応分のみ）

##### RT-H01: R16_UINT インデックスバッファ非対応（シェーダー側ハードコード）
- **File:** `Shaders/RTReflections.hlsl:193-196`
- **問題:** ClosestHit で `ib.Load(primIdx * 12 + N)` とストライド12バイト固定 → R16_UINT メッシュで壊れる
- **修正方針（選択肢 a — 推奨）:** `BuildBLAS` で R16_UINT を検出し R32_UINT に変換するか、R16_UINT を受け付けない
  ```cpp
  // BuildBLAS 内
  if (indexFormat == DXGI_FORMAT_R16_UINT)
  {
      GX::Logger::Warn("RTReflections: R16_UINT index buffer not supported, skipping");
      return -1;
  }
  ```
- **修正方針（選択肢 b）:** インデックスフォーマットを per-geometry cbuffer/StructuredBuffer でシェーダーに渡す

##### RT-H02: AddInstance() にインスタンス数上限チェックなし
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:165-203`
- **修正:**
  ```cpp
  void RTReflections::AddInstance(/* ... */)
  {
      if (m_instanceData.size() >= k_MaxInstances)
      {
          GX::Logger::Warn("RTReflections: Max instances ({}) reached", k_MaxInstances);
          return;
      }
      // 既存の push_back 処理
  }
  ```

##### RT-H03: BLAS ジオメトリ配列のギャップ（不連続インデックス）
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:138-140`
- **修正:** `BuildBLAS` の戻り値を連番にする。内部で `m_blasGeometry.push_back()` を使い、戻り値は `m_blasGeometry.size() - 1` とする（外部インデックスとの不一致に注意）

---

#### 36b-3: Medium (32件) — 第3優先

##### M-01: TextRenderer — 改行文字の比較誤り
- **File:** `GXLib/Graphics/Rendering/TextRenderer.cpp:101`
- **修正:** `L'\\n'` → `L'\n'` に修正（エスケープの二重化を解消）

##### M-02: FontManager — 未初期化エントリへのアクセス
- **File:** `GXLib/Graphics/Rendering/FontManager.cpp:410`
- **修正:** フリーリストから再利用する際に FontEntry をデフォルト初期化

##### M-03: TextRenderer — テクスチャ座標のオーバーフロー
- **File:** `GXLib/Graphics/Rendering/TextRenderer.cpp:65`
- **修正:** `(std::min)(static_cast<int>(glyph->u0 * k_AtlasSize), k_AtlasSize - 1)` でクランプ

##### M-04: RTReflections — 冗長なリソース状態遷移
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:306-309, 466-469`
- **修正:** `RenderTarget::TransitionTo()` の `m_currentState` 追跡を活用し、同一状態への遷移をスキップ

##### M-05: RTReflections::BuildBLAS — エラーハンドリング不足
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:131-142`
- **修正:** RT-H03 の修正と合わせて、連番インデックスに変更しギャップを排除

##### M-06: RTAccelerationStructure — ストライド検証不足
- **File:** `GXLib/Graphics/RayTracing/RTAccelerationStructure.cpp:42`
- **修正:** `if (stride == 0) { LOG_ERROR(...); return -1; }` ガード追加

##### M-07: RTReflections — テクスチャスロットオーバーフロー
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:180-185`
- **修正:** `if (m_textureResources.size() >= 32) { LOG_WARN(...); return; }` ガード追加

##### M-08: Bloom::OnResize — エラー伝播なし
- **File:** `GXLib/Graphics/PostEffect/Bloom.cpp:296-299`
- **修正:** `CreateMipRenderTargets` を `bool` 返り値に変更し、失敗時に `SetEnabled(false)` 呼び出し

##### M-09: Texture — CreateEvent の戻り値未チェック
- **File:** `GXLib/Graphics/Resource/Texture.cpp:213, 356`
- **修正:** `if (!hEvent) { LOG_ERROR(...); return; }` ガード追加

##### M-10: FileWatcher — イベントハンドルリーク
- **File:** `GXLib/IO/FileWatcher.cpp:42, 57`
- **修正:** `CreateEventA()` 戻り値を検証。デストラクタで `CloseHandle()` を呼ぶ

##### M-11: Crypto — 不統一なエラーハンドリング
- **File:** `GXLib/IO/Crypto.cpp:17-30, 83-93`
- **修正:** `Encrypt/Decrypt` 両方でエラーパスに `hAlg` クリーンアップを追加。エラーログの一貫性を確保

##### M-12: Archive — 整数オーバーフロー
- **File:** `GXLib/IO/Archive.cpp:62`
- **修正:** `if (tocSize > 100 * 1024 * 1024) { LOG_ERROR("Archive: TOC too large"); return false; }` 上限チェック

##### M-13: Sound — ファイル読み込みエラーハンドリング不足
- **File:** `GXLib/Audio/Sound.cpp:44, 60-61, 68-72`
- **修正:** 各 `file.read()` 後に `if (file.fail()) return false;` チェック追加

##### M-14: SoundPlayer — コールバックの寿命管理
- **File:** `GXLib/Audio/SoundPlayer.cpp:38-47`
- **修正:** `VoiceCallback` を `std::shared_ptr` で管理。`m_activeVoices` のエントリ削除時にコールバックが生存していることを保証

##### M-15: PhysicsWorld3D — シェイプ作成の null チェック不足
- **File:** `GXLib/Physics/PhysicsWorld3D.cpp:286, 295, 304`
- **修正:** Jolt Shape 作成結果の有効性チェック追加

##### M-16: TextInput — ループ条件の off-by-one
- **File:** `GXLib/GUI/Widgets/TextInput.cpp:203`
- **修正:** `for (int i = 1; i <= static_cast<int>(display.size()); ++i)` で `display.empty()` 時のガード追加

##### M-17: TabView — activeTab の範囲検証なし
- **File:** `GXLib/GUI/Widgets/TabView.cpp:40`
- **修正:** `m_activeTab = (std::max)(0, (std::min)(m_activeTab, (int)children.size() - 1));` でクランプ

##### M-18: DropDown::SetItems — selectedIndex の検証不正
- **File:** `GXLib/GUI/Widgets/DropDown.cpp:26-27`
- **修正:** `if (items.empty()) { m_selectedIndex = -1; } else if (m_selectedIndex >= (int)items.size()) { m_selectedIndex = 0; }`

##### M-19: PostEffectShowcase — VSync と TargetFps の矛盾
- **File:** `Samples/PostEffectShowcase/main.cpp:44-45`
- **修正:** `config.targetFps = 240` を削除（VSync 有効時は不要）

##### M-20: TextWidget — GetIntrinsicHeight のフォールバック不整合
- **File:** `GXLib/GUI/Widgets/TextWidget.cpp:15`
- **修正:** `m_fontHandle < 0` の場合、`GetIntrinsicWidth()` と `GetIntrinsicHeight()` の両方で `0.0f` を返すように統一

##### M-21: SpriteBatch — AddQuad の境界チェック不正
- **File:** `GXLib/Graphics/Rendering/SpriteBatch.cpp:246`
- **修正:** `m_vertexWriteOffset + (m_spriteCount + 1) * 4 > k_MaxSprites * 4` に修正（+1 で次の追加分を考慮）

##### RT-M01: 深度バッファ状態遷移の非対称性
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:307, 467, 547`
- **修正:** Execute 完了時に深度バッファの状態を呼び出し元が期待する状態に戻す。`TransitionTo()` の `m_currentState` 追跡を活用

##### RT-M02: OnResize() でディスクリプタヒープ未更新
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:550-580`
- **修正:** `OnResize()` 内でフラグを立て、次回 `Execute()` でディスクリプタを再構築

##### RT-M03: 深度 SRV フォーマットのハードコード
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:354`
- **修正:** `depth.GetResource()->GetDesc().Format` からフォーマットを取得し、適切な SRV フォーマットに変換
  ```cpp
  // D24_UNORM_S8_UINT → R24_UNORM_X8_TYPELESS
  // D32_FLOAT → R32_FLOAT
  ```

##### RT-M04: テクスチャスロットの SRV がフレーム跨ぎで残留
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:156-163`
- **修正:** `BeginFrame()` でテクスチャスロット範囲に null SRV を書き込んでクリア

##### RT-M05: m_normalRT の null チェック不完全
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:308-360`
- **修正:** H-08 と統合。全パスで null チェックを一貫させる

##### RT-M06: ClosestHit の法線方向保証なし
- **File:** `Shaders/RTReflections.hlsl:221-228`
- **修正:** シェーディング法線 `N` にも表裏チェックを追加:
  ```hlsl
  if (dot(N, WorldRayDirection()) > 0)
      N = -N;
  ```

##### RT-M07: Composite パスの alpha コメント誤記
- **File:** `Shaders/RTReflectionComposite.hlsl:89`
- **修正:** コメントを正確に: `// alpha = hit type weight (Miss=0.5, ClosestHit=1.0)`

##### MATH-01: Quaternion::ToEuler() の符号誤り
- **File:** `GXLib/Math/Quaternion.h:88-101`
- **修正:** DirectXMath の `XMQuaternionRotationRollPitchYaw(pitch, yaw, roll)` 規約（Z×Y×X 外的 XYZ）に対応する正しい抽出式:
  ```cpp
  float sinP = 2.0f * (w * x + y * z);   // 修正: - → +
  // yaw: atan2(2(wy - zx), 1 - 2(xx + yy))  // 修正: + → -
  // roll: atan2(2(wz - xy), 1 - 2(xx + zz))  // 修正: + → -
  ```
  **注意:** `FromEuler() → ToEuler()` のラウンドトリップテスト（Phase 37 test_Quaternion.cpp）で検証すること

##### MATH-02: PhysicsWorld2D — 角トルクに質量の逆数を使用
- **File:** `GXLib/Physics/PhysicsWorld2D.cpp:65`
- **修正:** 慣性モーメント `I` を RigidBody2D に追加し、角加速度 = torque / I で計算
  ```cpp
  // RigidBody2D に追加:
  float m_inertia = 1.0f;  // 慣性モーメント
  float InverseInertia() const { return (m_inertia > 0) ? 1.0f / m_inertia : 0.0f; }

  // Shape に応じた慣性モーメント計算:
  // 円形: I = 0.5 * m * r^2
  // 矩形: I = (1/12) * m * (w^2 + h^2)

  // PhysicsWorld2D.cpp:65 修正:
  body->angularVelocity += body->m_torqueAccum * (body->InverseInertia() * dt);
  ```

##### MATH-03: PhysicsWorld2D — AABB ブロードフェーズが回転を無視
- **File:** `GXLib/Physics/PhysicsWorld2D.cpp:91-96`
- **修正:** 矩形ボディの AABB 計算で4隅を回転してから min/max を取る:
  ```cpp
  AABB2D ComputeAABB(const RigidBody2D& body)
  {
      if (body.shape.type == ShapeType::Circle)
          return { body.position - Vector2(body.shape.radius), body.position + Vector2(body.shape.radius) };

      // 矩形: 4隅を回転
      float c = std::cos(body.rotation), s = std::sin(body.rotation);
      Vector2 h = body.shape.halfExtents;
      Vector2 corners[4] = { {-h.x, -h.y}, {h.x, -h.y}, {-h.x, h.y}, {h.x, h.y} };
      Vector2 mn = body.position, mx = body.position;
      for (auto& corner : corners)
      {
          Vector2 rotated = { c * corner.x - s * corner.y, s * corner.x + c * corner.y };
          rotated = rotated + body.position;
          mn = { (std::min)(mn.x, rotated.x), (std::min)(mn.y, rotated.y) };
          mx = { (std::max)(mx.x, rotated.x), (std::max)(mx.y, rotated.y) };
      }
      return { mn, mx };
  }
  ```

##### MATH-04: RTReflections ClosestHit — 法線変換に ObjectToWorld3x4 を直接使用
- **File:** `Shaders/RTReflections.hlsl:222`
- **修正:** 非一様スケーリング対応:
  ```hlsl
  // Before:
  float3 N = normalize(mul((float3x3)ObjectToWorld3x4(), normalObj));
  // After:
  float3 N = normalize(mul(normalObj, (float3x3)WorldToObject3x4()));
  ```

---

#### 36b-4: Low (14件) — 余裕があれば修正

##### L-01: SSAO — カーネルサイズ0で除算ゼロ
- **File:** `GXLib/Graphics/PostEffect/SSAO.cpp`
- **修正:** `static_assert(k_KernelSize > 0)` を追加

##### L-02: VolumetricLight — 未初期化 XMFLOAT3
- **File:** `GXLib/Graphics/PostEffect/VolumetricLight.cpp:126-127`
- **修正:** `XMFLOAT3 sunNDC = {0, 0, 0};` で初期化

##### L-03: MeshCollider — 除算ゼロ
- **File:** `GXLib/Physics/MeshCollider.cpp:50, 79`
- **修正:** `weld` の負値チェック追加。`step` が 0 の場合 `step = 1` にフォールバック

##### L-04: PhysicsWorld2D — Raycast 出力ポインタ未検証
- **File:** `GXLib/Physics/PhysicsWorld2D.cpp:260-262`
- **修正:** `if (outBody) *outBody = ...; if (outPoint) *outPoint = ...; if (outNormal) *outNormal = ...;`

##### L-05: Random — 無限ループリスク
- **File:** `GXLib/Math/Random.cpp:54-98`
- **修正:** rejection sampling ループに `constexpr int k_MaxAttempts = 1000;` の上限追加

##### L-06: Collision3D::ClosestPointOnLine — 除算ゼロ
- **File:** `GXLib/Math/Collision/Collision3D.cpp:426`
- **修正:** `float denom = ab.Dot(ab); if (denom < 1e-12f) return start;` ガード追加

##### L-07: Image Widget — UV オフセットの浮動小数点精度
- **File:** `GXLib/GUI/Widgets/Image.cpp:16-19`
- **修正:** 周期的にリセット: `if (offset > 1000.0f) offset -= 1000.0f;` （実質的に不可視なため低優先）

##### L-08: StyleSheet::ParseLength — 例外ハンドリング欠如
- **File:** `GXLib/GUI/StyleSheet.cpp:557`
- **修正:** `try { return std::stof(str); } catch (...) { LOG_WARN(...); return 0.0f; }`

##### MATH-05: DepthOfField — ガウスカーネル重みの正規化不正
- **File:** `Shaders/DepthOfField.hlsl:74-82`
- **修正:** 重み合計を 1.0 に正規化（0.41% の輝度増加 — ほぼ不可視だが正しくするのが好ましい）

##### TECH-01: RTReflections — ポイントライトに太陽シャドウを再利用
- **File:** `Shaders/RTReflections.hlsl:336`
- **修正:** ポイントライト行の `shadow` 変数を `1.0` に置換（追加シャドウレイは性能コストが高いため省略）:
  ```hlsl
  color += brdfP * pointLightColor * pointLightIntensity * NdotLp * atten;  // shadow 削除
  ```

##### TECH-02: PhysicsWorld2D — 衝突解決に角インパルスなし
- **File:** `GXLib/Physics/PhysicsWorld2D.cpp:161-217`
- **修正:** MATH-02 の慣性モーメント追加後に角インパルスを実装:
  ```cpp
  // 接触点と重心の差分ベクトル
  Vector2 rA = contactPoint - bodyA->position;
  Vector2 rB = contactPoint - bodyB->position;
  // 角インパルス: ω += (r × J_normal) / I
  bodyA->angularVelocity -= Cross2D(rA, impulse * normal) * bodyA->InverseInertia();
  bodyB->angularVelocity += Cross2D(rB, impulse * normal) * bodyB->InverseInertia();
  ```

##### TECH-03: PhysicsWorld2D — 摩擦計算にインパルス適用前の相対速度を使用
- **File:** `GXLib/Physics/PhysicsWorld2D.cpp:206`
- **修正:** 法線インパルス適用後に `relVel` を再計算してから摩擦インパルスを算出

##### RT-L02: 半解像度変数名が実態と不一致
- **File:** `GXLib/Graphics/RayTracing/RTReflections.h/cpp`
- **修正:** `m_halfResUAV` → `m_reflectionUAV`、`m_halfWidth` → `m_rtWidth`、`m_halfHeight` → `m_rtHeight` にリネーム

---

#### 36b-5: 品質改善 (Won't Fix / Future — 参考情報)

以下は BugReport.md に記載されているが、Phase 36 では **修正不要**（将来の品質改善として記録）:

- **QUAL-01:** TAA の HDR ブレンド前トーンマップ未実装 → Phase 39 以降で検討
- **QUAL-02:** CSM の対数分割未使用 → 品質改善として Phase 39 以降で検討
- **RT-L01:** cbuffer コメントの不完全さ → H-15 で対応済み
- **RT-L03:** SetCommandList4() の毎フレーム呼び出し → 微小な冗長、修正不要
- **RT-L04:** HDR クランプ値の固定 → トーンマッパー変更時に合わせて調整
- **RT-L05:** ポイントライトのインスタンスマスク未対応 → プロダクション機能として Phase 39+

---

### 36b-6: 追加の既知問題（BugReport 外）

以下は MEMORY.md の Common Issues と開発過程で判明している追加問題：

#### Extra-1: Entity::GetComponent 一時インスタンス問題
- **ファイル**: `GXLib/Core/Scene/Entity.h`
- **問題**: `GetComponent<T>()` が毎回 `T temp;` を生成して `GetType()` を呼ぶ
- **修正方針**: 各コンポーネントに `static constexpr ComponentType k_Type` を追加し、テンプレート内で `T::k_Type` を使用
  ```cpp
  template<typename T>
  T* GetComponent() const
  {
      constexpr ComponentType type = T::k_Type;
      int idx = static_cast<int>(type);
      if (idx >= 0 && idx < static_cast<int>(ComponentType::_Count) && m_componentLookup[idx] >= 0)
          return static_cast<T*>(m_components[m_componentLookup[idx]].get());
      return nullptr;
  }
  ```

#### Extra-2: Scene::DestroyEntity 遅延削除の安全性
- **ファイル**: `GXLib/Core/Scene/Scene.cpp`
- **問題**: `DestroyEntity()` 後、同フレーム内の `Render()` で描画される可能性
- **修正方針**: `DestroyEntity()` 内で即座に `entity->SetActive(false)` を呼ぶ

#### Extra-3: DynamicBuffer フレーム境界
- **ファイル**: `GXLib/Graphics/Resource/DynamicBuffer.h`
- **問題**: `Map()` したまま `Unmap()` を呼ばずにフレームを跨ぐと未定義動作
- **修正方針**: デストラクタで Map 状態をチェックし、未 Unmap なら警告ログ出力

#### Extra-4: PostEffectPipeline Null チェック
- **ファイル**: `GXLib/Graphics/PostEffect/PostEffectPipeline.cpp`
- **問題**: `SetRTReflections(nullptr)` / `SetRTGI(nullptr)` 呼び出し後に各エフェクトの Apply 内で nullptr デリファレンスの可能性
- **修正方針**: Apply 関数の冒頭で null チェック（既に対応済みの箇所もあるが全数確認）

---

### 36c: 修正優先順位サマリー

```
1. Critical (6件)   — C-01, C-02, C-03, RT-C01, RT-C02, RT-C03
2. High (24件)      — H-01〜H-20, RT-H01〜RT-H03
3. Medium (32件)    — M-01〜M-21, RT-M01〜RT-M07, MATH-01〜MATH-04
4. Low (14件)       — L-01〜L-08, MATH-05, TECH-01〜TECH-03, RT-L02
5. Extra (4件)      — Extra-1〜Extra-4
```

**注意:**
- **H-04（Skeleton 行列乗算順序）は誤検出として除外済み** — 計算式検証で正しいことを確認
- 同一ファイルに複数のバグがある場合はまとめて修正すること
- 修正ごとにビルド検証（36e）を実施

### 36d: コード衛生

#### 衛生 1: 未使用 include の削除
- 各 .cpp ファイルで未使用の `#include` を確認し削除
- ただし pch.h 経由のインクルードは触らない

#### 衛生 2: const 正確性
- getter メソッドが `const` 修飾されていない箇所を修正
- 特に `GetXxx() const` パターンの統一

#### 衛生 3: 範囲ベース for の活用
- `for (int i = 0; i < vec.size(); ++i)` パターンを
  `for (const auto& item : vec)` に置換（インデックスが不要な場合）

### 36e: ビルド検証
```bash
cmake -B build -S .
cmake --build build --config Debug
ctest --test-dir build --build-config Debug
```

---

## Phase 37: テスト強化

### 目的
既存テストのカバレッジを拡大し、未テストのシステムに新規テストを追加する。
GPU を必要としない純粋なロジックテストに集中する。

### 37a: 現状分析

#### テストフレームワーク
- **Google Test** (FetchContent gtest)
- CMake: `Tests/CMakeLists.txt`
- 実行: `ctest --test-dir build --build-config Debug`

#### 既存テストファイル（11 ファイル + test_main.cpp）

| ファイル | テスト対象 | テスト数 |
|---------|----------|---------|
| `test_main.cpp` | エントリーポイント（gtest_main 使用） | 0 |
| `test_Vector.cpp` | Vector2, Vector3, Vector4 | ~25 |
| `test_Matrix.cpp` | Matrix4x4 | ~10 |
| `test_Quaternion.cpp` | Quaternion | ~7 |
| `test_Color.cpp` | Color (RGBA, HSV, Lerp, Named) | ~8 |
| `test_MathUtil.cpp` | MathUtil (Lerp, Clamp, SmoothStep, Remap, InverseLerp, Degrees/Radians, NormalizeAngle, Sign, IsPowerOfTwo, NextPowerOfTwo, ApproximatelyEqual) + Random (IntRange, FloatRange, Seed, PointInCircle/Sphere, Direction2D/3D) | ~17 |
| `test_Collision2D.cpp` | AABB2D, Circle, Capsule2D, Polygon2D, Line, Raycast2D | ~15 |
| `test_Collision3D.cpp` | AABB3D, Sphere, OBB SAT, Frustum, Raycast, ClosestPoint | ~15 |
| `test_Spatial.cpp` | Quadtree, Octree, BVH (Build, Query, Raycast) | ~15 |
| `test_Crypto.cpp` | AES-256-CBC (encrypt/decrypt round-trip, wrong key), SHA-256 (known hash, deterministic), GenerateRandomBytes | ~6 |
| `test_Allocator.cpp` | PoolAllocator (alloc/free, new/delete, block growth, reuse), FrameAllocator (basic, typed, reset, alignment, capacity, sequential) | ~10 |

**合計: 約 128 テスト**

> **Google Test v1.15.2** — FetchContent 自動取得、gtest_discover_tests() で CTest 連携

#### 未テスト領域（テスト候補）

**GPU 不要（純粋ロジック）:**
- `Math/Transform2D.h` — 位置・回転・スケール変換
- `Math/Spline.h` — Evaluate, EvaluateByDistance, GetTotalLength, FindClosestParameter
- `Core/Scene/Entity.h` — コンポーネント追加/取得/削除、親子階層
- `Core/Scene/Scene.h` — エンティティ作成/破棄/検索
- `Core/Scene/SceneSerializer.h` — JSON 保存/読み込みラウンドトリップ
- `Input/ActionMapping.h` — アクション定義、バインディング評価
- `IO/FileSystem.h` — VFS マウント、ファイル解決
- `IO/Archive.h` — アーカイブ作成/読み込み
- `GUI/StyleSheet.h` — CSS パース、スタイル適用
- `AI/NavMesh.h` — パス検索（A* アルゴリズム）
- `AI/NavAgent.h` — 経路追従ロジック
- `Physics/PhysicsShape.h` — 形状作成
- `Graphics/3D/LODGroup.h` — LOD 選択ロジック（GPU 不要部分）
- `Graphics/3D/BlendStack.h` — アニメーションブレンド計算
- `Graphics/3D/BlendTree.h` — ブレンドツリー評価
- `Graphics/3D/AnimatorStateMachine.h` — ステートマシン遷移

**GPU 必要（統合テスト、Phase 37 ではスキップ）:**
- Renderer3D, SpriteBatch, PostEffect, RayTracing 等

### 37b: 新規テストファイル一覧

各ファイルの作成手順を以下に記載。全テストは `Tests/` ディレクトリに配置。

> **注意:** `test_MathUtil.cpp`（MathUtil + Random）と `test_Crypto.cpp` と `test_Allocator.cpp` は
> **既に存在する**。以下は未カバー領域の新規テストのみ。

---

#### テスト 1: `test_Spline.cpp`

```cpp
/// @file test_Spline.cpp
/// @brief Spline 単体テスト
#include "pch.h"
#include <gtest/gtest.h>
#include "Math/Spline.h"
#include "Math/Vector3.h"
using namespace GX;
```

**テストケース:**
| テスト名 | 内容 |
|---------|------|
| `Linear_Evaluate_Start` | t=0 で最初の制御点を返す |
| `Linear_Evaluate_End` | t=1 で最後の制御点を返す |
| `Linear_Evaluate_Mid` | t=0.5 で中間点を返す（2点の場合） |
| `CatmullRom_Endpoints` | t=0 と t=1 で端点を通過 |
| `CatmullRom_Smooth` | 中間点が制御点間を滑らかに補間（2次微分連続） |
| `CubicBezier_Endpoints` | t=0 と t=1 で端点を通過 |
| `GetTotalLength_TwoPoints` | 2点間の直線距離 ≈ GetTotalLength() |
| `GetTotalLength_Positive` | 任意の点列で長さ > 0 |
| `EvaluateByDistance_Zero` | distance=0 で始点を返す |
| `EvaluateByDistance_Full` | distance=totalLength で終点を返す |
| `EvaluateByDistance_Monotonic` | distance が増加すると t も増加 |
| `FindClosestParameter` | 制御点上の点で t ≈ 期待値 |
| `Closed_Evaluate` | SetClosed(true) で t=0 ≈ t=1 |
| `Empty_GetTotalLength` | 点なしで 0 を返す |
| `SinglePoint` | 1点のみで Evaluate が同じ点を返す |
| `SetType_Changes` | SetType 後に GetType が正しい値を返す |

---

#### テスト 2: `test_Entity.cpp`

```cpp
/// @file test_Entity.cpp
/// @brief Entity/Scene 単体テスト
#include "pch.h"
#include <gtest/gtest.h>
#include "Core/Scene/Entity.h"
#include "Core/Scene/Scene.h"
#include "Core/Scene/Components.h"
using namespace GX;
```

**テストケース:**
| テスト名 | 内容 |
|---------|------|
| `Scene_CreateEntity` | CreateEntity で Entity* が non-null |
| `Scene_CreateEntity_Name` | 名前が正しく設定される |
| `Scene_FindEntityByName` | FindEntity(name) で正しい Entity を返す |
| `Scene_FindEntityByID` | FindEntityByID(id) で正しい Entity を返す |
| `Scene_DestroyEntity` | DestroyEntity 後に FindEntity が nullptr |
| `Entity_AddComponent` | AddComponent<T> が non-null を返す |
| `Entity_GetComponent` | GetComponent<T> で追加したコンポーネントを取得 |
| `Entity_GetComponent_NotFound` | 未追加のコンポーネントで nullptr |
| `Entity_Transform` | GetTransform() でデフォルト Transform3D |
| `Entity_SetParent` | SetParent 後に GetParent が正しい |
| `Entity_Children` | 子エンティティが GetChildren に含まれる |
| `Entity_RemoveParent` | SetParent(nullptr) で親子関係を解除 |
| `Entity_ActiveState` | SetActive(false) 後に IsActive() == false |
| `Entity_UniqueID` | 2つの Entity の GetID() が異なる |
| `Scene_RootEntities` | 親なしエンティティが GetRootEntities に含まれる |
| `Scene_Update` | Update(dt) がクラッシュしない |
| `Scene_Clear` | Clear() 後に全 Entity が破棄される |

**注意**: Entity/Scene/Components は GPU を使わない純粋なロジックなので単体テスト可能。
ただし `MeshRendererComponent` の `model` フィールドは GPU リソースなので nullptr のままテスト。

---

#### テスト 3: `test_ActionMapping.cpp`

```cpp
/// @file test_ActionMapping.cpp
/// @brief ActionMapping 単体テスト
#include "pch.h"
#include <gtest/gtest.h>
#include "Input/ActionMapping.h"
using namespace GX;
```

**テストケース:**
| テスト名 | 内容 |
|---------|------|
| `DefineAction` | DefineAction 後にアクションが存在 |
| `GetActionValue_Default` | 未定義アクションで 0.0f を返す |
| `ClearAllActions` | ClearAllActions 後に全アクションが消える |
| `KeyBinding_Structure` | InputBinding::Key(VK_SPACE) が正しいバインディング |
| `KeyAxisBinding_Structure` | InputBinding::KeyAxis(W, S) が正しいバインディング |

**注意**: `Update()` は `Keyboard`, `Mouse`, `Gamepad` の実インスタンスが必要。
モック化が困難なため、構造テストに限定する。
将来的には `Keyboard` のモックを追加して Update のテストも行う。

---

#### テスト 4: `test_NavMesh.cpp`

```cpp
/// @file test_NavMesh.cpp
/// @brief NavMesh A* パス検索テスト
#include "pch.h"
#include <gtest/gtest.h>
#include "AI/NavMesh.h"
#include "AI/NavAgent.h"
using namespace GX;
```

**テストケース:**
| テスト名 | 内容 |
|---------|------|
| `Build_EmptyGrid` | 空のグリッドで Build が成功 |
| `Build_WithObstacles` | 障害物ありで Build が成功 |
| `FindPath_StraightLine` | 障害物なしの直線パス |
| `FindPath_AroundObstacle` | 障害物を迂回するパス |
| `FindPath_NoPath` | 到達不可能な場合に空パスを返す |
| `FindPath_SameStartEnd` | 同じ開始/終了点で空パスまたは1点 |
| `SetWalkable_False` | 障害物セル上はパスを通らない |
| `GetCellAt` | 座標からセルインデックスへの変換 |
| `NavAgent_SetDestination` | 目的地設定が正しく保存される |
| `NavAgent_Update` | Update 後に位置が変化する |
| `NavAgent_HasReached` | 目的地到着で HasReachedDestination() == true |

---

#### テスト 5: `test_LODGroup.cpp`

```cpp
/// @file test_LODGroup.cpp
/// @brief LODGroup 選択ロジックテスト
#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/LODGroup.h"
using namespace GX;
```

**テストケース:**
| テスト名 | 内容 |
|---------|------|
| `SelectLOD_Closest` | 近距離で LOD 0 を選択 |
| `SelectLOD_Farthest` | 遠距離で最低 LOD を選択 |
| `SelectLOD_Hysteresis` | ヒステリシスバンド内で LOD が変化しない |
| `Empty_ReturnsNeg1` | LOD なしで -1 を返す |
| `SingleLOD` | LOD 1つのみで常にそれを返す |

**注意**: `SelectLOD` がカメラ距離とスクリーン占有率で判定するため、
Model* は nullptr でもバウンディング球の半径だけ設定してテスト可能かを確認。
不可能な場合はモック構造体を使用。

---

#### テスト 6: `test_SceneSerializer.cpp`

```cpp
/// @file test_SceneSerializer.cpp
/// @brief SceneSerializer JSON ラウンドトリップテスト
#include "pch.h"
#include <gtest/gtest.h>
#include "Core/Scene/Scene.h"
#include "Core/Scene/SceneSerializer.h"
using namespace GX;
```

**テストケース:**
| テスト名 | 内容 |
|---------|------|
| `SaveLoad_EmptyScene` | 空シーンの保存/読み込みラウンドトリップ |
| `SaveLoad_SingleEntity` | 1エンティティの名前と Transform が保持される |
| `SaveLoad_Hierarchy` | 親子関係が保持される |
| `SaveLoad_Components` | コンポーネントデータが保持される |
| `SaveLoad_MultipleEntities` | 複数エンティティの順序と関係が保持される |
| `ToJsonString_Valid` | JSON 文字列が有効な JSON である |
| `FromJsonString_Invalid` | 不正な JSON で false を返す |

**注意**: Model は nullptr のままテスト。ModelLoadCallback には空のラムダを渡す。

---

### 37c: テスト CMakeLists.txt 更新

新規テストファイルを `Tests/CMakeLists.txt` の `TEST_SOURCES` リストに追加する:

```cmake
# 既存リストに追加
set(TEST_SOURCES
    test_main.cpp
    test_Vector.cpp
    test_Matrix.cpp
    test_Quaternion.cpp
    test_Color.cpp
    test_MathUtil.cpp       # 既存 (MathUtil + Random)
    test_Collision2D.cpp
    test_Collision3D.cpp
    test_Spatial.cpp
    test_Crypto.cpp         # 既存
    test_Allocator.cpp      # 既存
    # Phase 37 新規
    test_Spline.cpp
    test_Entity.cpp
    test_ActionMapping.cpp
    test_NavMesh.cpp
    test_LODGroup.cpp
    test_SceneSerializer.cpp
)
```

### 37d: テスト実行と検証

```bash
cmake -B build -S .
cmake --build build --config Debug --target GXLibTests
ctest --test-dir build --build-config Debug --verbose
```

全テストが PASS することを確認。

### 37e: 注意事項

- テストは GPU を使わない純粋なロジックテストに限定する
- `pch.h` は GXLib 本体と同じものを使用（Windows.h, DirectXMath 等が含まれる）
- `using namespace GX;` を各テストファイルの冒頭で宣言
- NavMesh/NavAgent のテストは内部状態に依存するため、API を確認してから書く
- Scene/Entity は GPU リソースを持つコンポーネント（MeshRenderer 等）を使う場合、
  model ポインタは nullptr のままテストする
- `std::vector<bool>` の問題に注意（ImGui テストではないが、Components 内で使う可能性）

---

## Phase 38: ドキュメント改善

### 目的
DocumentationAudit.md で指摘された問題（専門用語未説明、前提知識の暗黙仮定、
「なぜ」の欠如、トラブルシューティング不足）を体系的に改善する。

### 38a: 既存ドキュメント構造

```
docs/
├── tutorials/
│   ├── 01_GettingStarted.md    — 環境構築 + Hello World
│   ├── 02_2DDrawing.md         — スプライト、プリミティブ
│   ├── 03_InputAndSound.md     — キーボード、マウス、サウンド
│   ├── 04_3DRendering.md       — PBR、カメラ、ライト、ポストエフェクト
│   └── 05_GUI.md               — ウィジェット、CSS、XMLレイアウト
├── migration/
│   └── DxLibMigrationGuide.md  — DXLib からの移行ガイド
├── Glossary.md                 — 用語集
├── DocumentationAudit.md       — 監査レポート（本 Phase の入力）
└── Phase31-34_Directive.md     — 過去の指令書
```

### 38b: チュートリアル改善

各チュートリアルに以下のセクションを追加/改善:

#### 全チュートリアル共通の追加セクション

**冒頭に「前提知識」セクションを追加:**
```markdown
## 前提知識
- C++ の基礎（変数、関数、クラス、ポインタ）
- Visual Studio 2022 の基本操作
- コマンドラインの基本操作（cd, mkdir）
```

**末尾に「よくある問題」セクションを追加:**
```markdown
## よくある問題

### Q: ビルドは成功するが、実行時にシェーダーが見つからないエラーが出る
Visual Studio からデバッグ実行する場合、作業ディレクトリが exe の場所と
異なることがあります。VS_DEBUGGER_WORKING_DIRECTORY は CMake で自動設定
されますが、手動で開いた場合はプロジェクトプロパティ → デバッグ →
作業ディレクトリを `$(TargetDir)` に設定してください。

### Q: GX_Init() が -1 を返す
DirectX 12 に対応した GPU とドライバ（Windows 10 1903以降）が必要です。
GPU ドライバを最新に更新してください。

### Q: テクスチャが表示されない（白い四角になる）
テクスチャファイルのパスが正しいか確認してください。
相対パスは exe の場所からの相対です。
```

#### チュートリアル 01: Getting Started 改善

1. **WinMain の説明を追加**:
   ```markdown
   > **WinMain とは？**
   > Windows のデスクトップアプリケーションのエントリーポイント（開始地点）です。
   > コンソールアプリの `main()` に相当します。`HINSTANCE` はアプリケーションの
   > インスタンスハンドルで、Windows がアプリを識別するために使います。
   ```

2. **ダブルバッファリングの概念を説明**:
   ```markdown
   > **なぜ裏画面に描画するのか？**
   > 直接画面に描画すると、描画途中の不完全な画像が一瞬見えてしまいます
   > （ティアリング）。裏画面に描き終えてから一括で表示に切り替える
   > （ScreenFlip）ことで、滑らかな表示になります。この手法を
   > 「ダブルバッファリング」と呼びます。
   ```

3. **GXEasy パターンの推奨**:
   初心者向けに `GXEasy::App` パターンを最初に示し、低レベル API は後の章に回す。

#### チュートリアル 04: 3D Rendering 改善

1. **PBR の説明を追加**:
   ```markdown
   ## PBR（物理ベースレンダリング）とは

   PBR は、光の物理法則に基づいてマテリアルの見た目を計算する手法です。
   従来のレンダリング（Phong シェーディング等）と比べて、以下の利点があります：

   - **一貫性**: どのライティング環境でもマテリアルが自然に見える
   - **直感性**: metallic（金属度）と roughness（粗さ）の2パラメータで
     金属からプラスチックまで表現できる

   | metallic | roughness | 見た目 |
   |----------|-----------|--------|
   | 0.0 | 0.1 | 磨かれたプラスチック |
   | 0.0 | 0.9 | マットな布/木 |
   | 1.0 | 0.1 | 鏡面研磨された金属 |
   | 1.0 | 0.9 | 錆びた/粗い金属 |
   ```

2. **ポストエフェクト略語の展開**:
   ```markdown
   | 略語 | 正式名称 | 効果 |
   |------|---------|------|
   | SSAO | Screen Space Ambient Occlusion | 隅や隙間を暗くして立体感を出す |
   | SSR | Screen Space Reflections | 画面上の情報だけで反射を計算 |
   | DoF | Depth of Field | カメラのピンぼけ効果 |
   | TAA | Temporal Anti-Aliasing | 時間方向にサンプリングしてジャギーを減らす |
   | FXAA | Fast Approximate Anti-Aliasing | 軽量なエッジ滑らか化 |
   | HDR | High Dynamic Range | 明るさの表現範囲を拡大 |
   ```

### 38c: 新規チュートリアル

#### チュートリアル 06: GXEasy で始める 2D ゲーム（新規）

対象読者: C++ 初級者（GXEasy パターンで DXLib 風の簡単な API）

```markdown
# 06. GXEasy で始める 2D ゲーム

## 前提知識
- C++ の基礎（変数、if文、for文、関数）
- Visual Studio のプロジェクト作成

## このチュートリアルで学ぶこと
- GXEasy::App クラスの使い方
- 画面にテキストと図形を表示する方法
- キーボード入力の取得
- 簡単なゲームループの作り方

## Step 1: 空のウィンドウを開く
[GXEasy::App の最小コード]

## Step 2: 背景と文字を表示する
[DrawString, DrawBox]

## Step 3: キー入力で四角を動かす
[CheckHitKey, 位置変数の更新]

## Step 4: 当たり判定を追加する
[矩形 vs 矩形の当たり判定]
```

#### チュートリアル 07: 3D シーンを作る（新規）

対象読者: チュートリアル 04 完了者

```markdown
# 07. 3D シーンを作る

## このチュートリアルで学ぶこと
- Scene と Entity の使い方
- コンポーネントの追加
- カメラ操作
- ライティングの設定

## Step 1: 空のシーンを作成する
## Step 2: エンティティを追加する
## Step 3: カメラとライトを設定する
## Step 4: シーンを保存/読み込みする
```

#### チュートリアル 08: アセットパイプライン（新規）

```markdown
# 08. アセットパイプライン

## このチュートリアルで学ぶこと
- gxconv で 3D モデルを変換する
- gxpak でアセットをバンドルする
- VFS でバンドルからアセットを読み込む
- GXModelViewer でモデルを確認・編集する
```

### 38d: 関数リファレンス

`docs/api/` ディレクトリに主要 API のリファレンスページを作成。

#### 対象（優先度順）:

1. **GXEasy API** (`docs/api/GXEasy.md`)
   - GXEasy::App クラス
   - DXLib 互換関数（DrawGraph, DrawString, CheckHitKey 等）
   - FormatT テンプレート

2. **Math API** (`docs/api/Math.md`)
   - Vector2/3/4
   - Matrix4x4
   - Quaternion
   - Color
   - Spline
   - MathUtil

3. **Input API** (`docs/api/Input.md`)
   - Keyboard, Mouse, Gamepad
   - ActionMapping

4. **Audio API** (`docs/api/Audio.md`)
   - AudioManager
   - SoundPlayer, MusicPlayer
   - 3D Audio (AudioEmitter, AudioListener)

5. **Graphics API** (`docs/api/Graphics.md`)
   - Renderer3D（主要メソッドのみ）
   - Camera3D
   - Material
   - Light
   - PostEffectPipeline

6. **Scene API** (`docs/api/Scene.md`)
   - Scene, Entity
   - Components
   - SceneSerializer

#### リファレンスの形式

各関数について以下を記載:

```markdown
### DrawGraph(x, y, handle, transparent)

画像をそのままのサイズで描画します。

**引数:**
| 名前 | 型 | 説明 |
|------|-----|------|
| x | int | 描画先の左上 X 座標（ピクセル） |
| y | int | 描画先の左上 Y 座標（ピクセル） |
| handle | int | LoadGraph で取得したテクスチャハンドル |
| transparent | int | TRUE: 透過描画, FALSE: 不透過 |

**戻り値:** 0（成功）, -1（失敗）

**使用例:**
```cpp
int tex = LoadGraph("player.png");
DrawGraph(100, 200, tex, TRUE);
```

**注意:**
- handle が無効な場合は何も描画されません
- 座標は画面左上が (0, 0) です
```

### 38e: Glossary.md の拡充

既存の用語集に以下を追加:

| 用語 | 正式名称 | 説明 |
|------|---------|------|
| BLAS | Bottom-Level Acceleration Structure | DXR で個別メッシュのレイトレーシング高速化構造 |
| TLAS | Top-Level Acceleration Structure | DXR でシーン全体のレイトレーシング高速化構造 |
| GI | Global Illumination | 間接光を含むシーン全体の照明計算 |
| BRDF | Bidirectional Reflectance Distribution Function | 表面の光の反射特性を記述する関数 |
| IBL | Image-Based Lighting | 環境マップ画像を使った照明手法 |
| UTS2 | Unity Toon Shader 2.0 | Unity 用のトゥーンシェーダー実装。GXLib の Toon シェーダーの参考元 |
| LOD | Level of Detail | カメラからの距離に応じてモデルの詳細度を切り替える手法 |
| A* | A-star | 最短経路探索アルゴリズム。NavMesh で使用 |
| ECS | Entity Component System | ゲームオブジェクトをエンティティとコンポーネントで構成する設計パターン |
| VFS | Virtual File System | 物理ファイルとアーカイブを統一的に扱うファイルシステム抽象化 |
| PSO | Pipeline State Object | D3D12 のレンダリングパイプラインの状態を一括管理するオブジェクト |

### 38f: README.md 改善

- 特徴一覧の略語に括弧で日本語説明を追加
- Phase 23-35 の新機能を反映
- サンプルプロジェクト一覧を 22 個に更新
- ビルド手順のトラブルシューティングセクションを追加

### 38g: サンプル解説

各サンプルの `main.cpp` 冒頭に以下を記載（既にある程度あるが不足分を補完）:

```cpp
/// @file Samples/XXX/main.cpp
/// @brief [サンプルの1行説明]
///
/// [サンプルの詳細説明（3-5行）]
/// [学べるポイント]
/// [使用している GXLib 機能のリスト]
///
/// Controls:
///   [操作方法のリスト]
```

---

## Phase 39: 新エンジン機能（前半）— レンダリング高度化

### 目的
レンダリングパイプラインを高度化し、大規模シーンへの対応力を向上させる。

### 39a: マルチスレッドコマンド記録

#### 概要
D3D12 の最大の利点であるマルチスレッドコマンドリスト記録を活用し、
CPU バウンドな描画コール発行を並列化する。

#### アーキテクチャ

```
メインスレッド:
  BeginFrame()
  ├─ ワーカースレッド 0: シャドウパス CommandList 記録
  ├─ ワーカースレッド 1: 不透明パス CommandList 記録 (前半)
  ├─ ワーカースレッド 2: 不透明パス CommandList 記録 (後半)
  └─ メインスレッド: ポストエフェクト + UI
  ExecuteCommandLists(shadowCL, opaqueCL0, opaqueCL1, postFxCL)
  Present()
```

#### 新規ファイル
- `GXLib/Graphics/Device/ParallelCommandRecorder.h`
- `GXLib/Graphics/Device/ParallelCommandRecorder.cpp`

#### クラス設計
```cpp
namespace GX
{
    class ParallelCommandRecorder
    {
    public:
        /// @param device D3D12デバイス
        /// @param numWorkers ワーカースレッド数（0=ハードウェア並列数-1）
        void Initialize(ID3D12Device* device, uint32_t numWorkers = 0);
        void Shutdown();

        /// @brief 記録ジョブを追加
        /// @param job 引数は (ID3D12GraphicsCommandList*, uint32_t frameIndex)
        void AddRecordJob(std::function<void(ID3D12GraphicsCommandList*, uint32_t)> job);

        /// @brief 全ジョブを並列実行し、完了を待つ
        /// @return 記録済み CommandList の配列
        std::vector<ID3D12CommandList*> ExecuteAndWait(uint32_t frameIndex);

    private:
        struct WorkerThread
        {
            std::thread thread;
            CommandList cmdList;  // ワーカーごとに独立した CommandList
        };
        std::vector<WorkerThread> m_workers;
        // スレッドプール + ジョブキュー
    };
}
```

#### 実装の注意点
- **CommandAllocator**: フレーム × ワーカー数 の CommandAllocator が必要
  （CommandAllocator はスレッドセーフではないため共有不可）
- **DescriptorHeap**: GPU-visible ヒープはスレッド間で共有可能だが、
  CPU-visible ヒープの割り当てはスレッドローカルに行う
- **RootSignature/PSO**: 読み取り専用なのでスレッド間共有可能
- **リソースバリア**: 各 CommandList のバリアは独立。
  ExecuteCommandLists の順序がバリアの順序を決定する
- **pch.h**: `<thread>`, `<mutex>`, `<condition_variable>`, `<atomic>` は既に含まれている

#### 統合方法
Renderer3D に `SetParallelMode(bool)` を追加:
```cpp
void Renderer3D::Begin(ID3D12GraphicsCommandList* cmd, uint32_t frameIndex,
                       const Camera3D& camera, float time)
{
    if (m_parallelMode && m_parallelRecorder)
    {
        // ワーカースレッドに描画を分配
    }
    else
    {
        // 従来のシングルスレッド描画
    }
}
```

### 39b: Indirect Drawing（間接描画）

#### 概要
GPU 駆動レンダリングの基盤。CPU ではなく GPU 側で描画コマンドを生成する。

#### 新規ファイル
- `GXLib/Graphics/Device/IndirectCommandBuffer.h`
- `GXLib/Graphics/Device/IndirectCommandBuffer.cpp`

#### クラス設計
```cpp
namespace GX
{
    class IndirectCommandBuffer
    {
    public:
        void Initialize(ID3D12Device* device, uint32_t maxCommands);

        /// @brief 間接描画引数を追加（CPU 側で構築）
        void AddDrawIndexedArgs(uint32_t indexCount, uint32_t instanceCount,
                                uint32_t startIndex, int32_t baseVertex,
                                uint32_t startInstance);

        /// @brief GPU で ExecuteIndirect を呼ぶ
        void Execute(ID3D12GraphicsCommandList* cmdList,
                     ID3D12CommandSignature* cmdSig);

        /// @brief コマンド数を取得
        uint32_t GetCommandCount() const;

    private:
        Buffer m_argBuffer;      // D3D12_DRAW_INDEXED_ARGUMENTS の配列
        Buffer m_countBuffer;    // コマンド数（GPU カリング時に使用）
        uint32_t m_maxCommands;
        uint32_t m_currentCount = 0;
    };
}
```

#### 用途
- Scene::Render でフラスタムカリング後の描画リストを IndirectCommandBuffer に書き込み、
  ExecuteIndirect で一括描画
- 将来的に Compute Shader でのGPUカリングと組み合わせ可能

### 39c: Async Compute

#### 概要
グラフィックスキューと並列に Compute キューで計算を行い、GPU 使用率を向上させる。

#### 対象タスク
- SSAO の Compute Shader 版
- Bloom の Compute Shader 版（ダウンサンプル+アップサンプル）
- パーティクル更新（既存の GPUParticleSystem は Graphics キューで Dispatch）
- RTGI のデノイズパス

#### 新規ファイル
- `GXLib/Graphics/Device/AsyncComputeQueue.h`
- `GXLib/Graphics/Device/AsyncComputeQueue.cpp`

#### クラス設計
```cpp
namespace GX
{
    class AsyncComputeQueue
    {
    public:
        void Initialize(ID3D12Device* device);

        /// @brief Compute ジョブをキューに追加
        void Submit(std::function<void(ID3D12GraphicsCommandList*)> computeJob);

        /// @brief GPU フェンスで同期（Graphics キューが結果を使う前に呼ぶ）
        void WaitOnGraphicsQueue(ID3D12CommandQueue* graphicsQueue);

        /// @brief 前フレームの Compute が完了しているか確認
        bool IsComplete() const;

    private:
        CommandQueue m_computeQueue;  // D3D12_COMMAND_LIST_TYPE_COMPUTE
        CommandList m_computeCmdList;
        Fence m_fence;
    };
}
```

#### 実装の注意点
- **リソース状態**: Compute キューでは `D3D12_RESOURCE_STATE_UNORDERED_ACCESS` が必要
- **フェンス同期**: Graphics → Compute → Graphics の依存関係をフェンスで管理
- **タイミング**: 前フレームの Compute 結果を今フレームの Graphics で使用（1フレーム遅延許容）

### 39d: サンプルプロジェクト

- `Samples/MultiThreadShowcase/main.cpp` — マルチスレッド描画のデモ（1000 オブジェクト）

---

## Phase 40: 新エンジン機能（後半）— ゲームプレイ機能

### 目的
ゲーム開発に直結する実用的な機能を追加する。

### 40a: Lua スクリプティング

#### 概要
ゲームロジックを C++ から分離し、ホットリロード可能な Lua スクリプトで記述できるようにする。

#### 依存ライブラリ
- **sol2** (header-only Lua C++ バインディング)
- **Lua 5.4** (FetchContent または ThirdParty に同梱)

#### 新規ファイル
- `GXLib/Script/ScriptEngine.h`
- `GXLib/Script/ScriptEngine.cpp`
- `GXLib/Script/ScriptBindings.h`
- `GXLib/Script/ScriptBindings.cpp`

#### クラス設計
```cpp
namespace GX
{
    class ScriptEngine
    {
    public:
        void Initialize();
        void Shutdown();

        /// @brief Lua ファイルを読み込んで実行
        bool ExecuteFile(const std::string& path);

        /// @brief Lua 文字列を実行
        bool ExecuteString(const std::string& code);

        /// @brief グローバル関数を呼ぶ
        template<typename Ret, typename... Args>
        Ret CallFunction(const std::string& name, Args&&... args);

        /// @brief ホットリロード（FileWatcher と連携）
        void ReloadFile(const std::string& path);

        /// @brief sol::state への直接アクセス（上級者向け）
        sol::state& GetState() { return m_lua; }

    private:
        sol::state m_lua;
    };
}
```

#### バインディング対象（ScriptBindings.cpp）

```cpp
void RegisterBindings(sol::state& lua)
{
    // Math
    lua.new_usertype<Vector2>("Vector2",
        sol::constructors<Vector2(), Vector2(float, float)>(),
        "x", &Vector2::x, "y", &Vector2::y,
        "Length", &Vector2::Length,
        "Normalized", &Vector2::Normalized,
        sol::meta_function::addition, &Vector2::operator+,
        sol::meta_function::subtraction, &Vector2::operator-
    );
    // Vector3, Color, Transform3D 等も同様

    // Input
    lua["IsKeyDown"] = [](int key) { return CheckHitKey(key) != 0; };
    lua["GetMouseX"] = []() { int x, y; GetMousePoint(&x, &y); return x; };
    lua["GetMouseY"] = []() { int x, y; GetMousePoint(&x, &y); return y; };

    // Drawing (GXEasy 互換)
    lua["DrawString"] = &DrawString;
    lua["DrawBox"] = &DrawBox;
    lua["DrawGraph"] = &DrawGraph;
    lua["LoadGraph"] = &LoadGraph;

    // Entity/Scene
    lua.new_usertype<Entity>("Entity",
        "GetName", &Entity::GetName,
        "SetPosition", [](Entity& e, float x, float y, float z) {
            e.GetTransform().SetPosition(x, y, z);
        },
        "GetPosition", [](Entity& e) -> Vector3 {
            auto p = e.GetTransform().GetPosition();
            return Vector3(p.x, p.y, p.z);
        }
    );
}
```

#### 使用例（Lua 側）
```lua
-- game.lua
function OnStart()
    player = scene:CreateEntity("Player")
    player:SetPosition(0, 1, 0)
end

function OnUpdate(dt)
    local speed = 5.0 * dt
    if IsKeyDown(KEY_W) then
        local pos = player:GetPosition()
        player:SetPosition(pos.x, pos.y, pos.z + speed)
    end
end

function OnDraw()
    DrawString(10, 10, "Hello from Lua!", 0xFFFFFFFF)
end
```

#### 実装の注意点
- **sol2**: header-only なので CMake に FetchContent で追加するか ThirdParty/ に配置
- **Lua 5.4**: C ライブラリなので `SKIP_PRECOMPILE_HEADERS ON` + `LANGUAGE C` が必要
  （lz4.c, ufbx.c と同パターン）
- **エラーハンドリング**: Lua 実行エラーは `GX::Logger::Error()` で報告
- **ホットリロード**: `FileWatcher` と連携して .lua ファイル変更時に自動リロード
- **pch.h**: sol2 のヘッダは巨大なので pch.h には入れない。ScriptEngine.cpp 内でのみインクルード

### 40b: 2D タイルマップ

#### 概要
2D ゲーム用のタイルマップシステム。Tiled エディタ (.tmx) のインポートに対応。

#### 新規ファイル
- `GXLib/Graphics/Rendering/Tilemap.h`
- `GXLib/Graphics/Rendering/Tilemap.cpp`

#### クラス設計
```cpp
namespace GX
{
    struct TileLayer
    {
        std::string name;
        int width, height;
        std::vector<int> tileIDs;  // -1 = 空
        float opacity = 1.0f;
        bool visible = true;
    };

    struct Tileset
    {
        int textureHandle;
        int tileWidth, tileHeight;
        int columns;
        int firstGID;
    };

    class Tilemap
    {
    public:
        /// @brief TMX ファイルを読み込む
        bool LoadFromTMX(const std::string& path);

        /// @brief タイルマップを描画
        void Draw(SpriteBatch& batch, const Camera2D& camera);

        /// @brief 指定座標のタイル ID を取得
        int GetTileAt(int layerIndex, int x, int y) const;

        /// @brief 指定タイルが walkable かどうか
        bool IsWalkable(int x, int y) const;

        /// @brief タイル座標 → ワールド座標
        Vector2 TileToWorld(int x, int y) const;

        /// @brief ワールド座標 → タイル座標
        void WorldToTile(float wx, float wy, int& tx, int& ty) const;

        int GetWidth() const;
        int GetHeight() const;
        int GetTileWidth() const;
        int GetTileHeight() const;
        int GetLayerCount() const;
        const TileLayer& GetLayer(int index) const;

    private:
        std::vector<TileLayer> m_layers;
        std::vector<Tileset> m_tilesets;
        int m_width = 0, m_height = 0;
        int m_tileWidth = 0, m_tileHeight = 0;
    };
}
```

#### TMX パース
- TMX は XML 形式。GXLib に既存の `XMLParser` を活用可能
- CSV 形式のタイルデータをパース
- タイルセット画像は `TextureManager::LoadTexture()` で読み込み

#### 描画最適化
- カメラの可視範囲のタイルのみ描画（カリング）
- SpriteBatch のバッチ描画を活用（同一テクスチャのタイルをまとめる）

### 40c: ルートモーション

#### 概要
アニメーションの移動量をキャラクターの Transform に反映する。
歩きアニメーションの足の滑りを防ぐ。

#### 修正ファイル
- `GXLib/Graphics/3D/Animator.h`
- `GXLib/Graphics/3D/Animator.cpp`
- `GXLib/Graphics/3D/Animation.h`

#### 追加 API
```cpp
class Animator
{
public:
    // 既存 API...

    /// @brief ルートモーションを有効化
    void SetRootMotionEnabled(bool enabled);
    bool IsRootMotionEnabled() const;

    /// @brief 今フレームのルートモーション移動量を取得
    /// Transform に加算して使用する
    XMFLOAT3 GetRootMotionDelta() const;

    /// @brief 今フレームのルートモーション回転量を取得
    Quaternion GetRootMotionRotationDelta() const;

private:
    bool m_rootMotion = false;
    XMFLOAT3 m_rootDelta = {};
    Quaternion m_rootRotDelta = Quaternion::Identity();
    XMFLOAT3 m_lastRootPos = {};
};
```

#### 実装方針
1. アニメーション更新時にルートボーン（通常は Hips）の位置変化を計算
2. ルートボーンの移動量を `m_rootDelta` に保存
3. ルートボーンの位置をアニメーション内でゼロに戻す（XZ 平面のみ）
4. ゲーム側で `GetRootMotionDelta()` を Transform に加算

```cpp
// ゲーム側の使用例
void Update(float dt)
{
    animator.Update(dt);
    if (animator.IsRootMotionEnabled())
    {
        auto delta = animator.GetRootMotionDelta();
        auto pos = transform.GetPosition();
        pos.x += delta.x;
        pos.z += delta.z;
        transform.SetPosition(pos.x, pos.y, pos.z);
    }
}
```

### 40d: アニメーションイベント

#### 概要
アニメーションの特定フレームでコールバックを発火する仕組み。
足音、攻撃ヒット判定、エフェクト発生等に使用。

#### 修正ファイル
- `GXLib/Graphics/3D/Animation.h`
- `GXLib/Graphics/3D/Animator.h`
- `GXLib/Graphics/3D/Animator.cpp`

#### 追加 API
```cpp
struct AnimationEvent
{
    float time;           // 発火時刻（秒）
    std::string name;     // イベント名
};

class Animator
{
public:
    /// @brief イベントコールバックを登録
    void SetEventCallback(std::function<void(const std::string&)> callback);

    /// @brief 特定クリップにイベントを追加
    void AddEvent(const std::string& clipName, float time, const std::string& eventName);

private:
    std::function<void(const std::string&)> m_eventCallback;
};
```

#### 使用例
```cpp
animator.SetEventCallback([](const std::string& event) {
    if (event == "footstep")
        PlaySound(footstepSound);
    else if (event == "attack_hit")
        CheckAttackCollision();
});
animator.AddEvent("Walk", 0.3f, "footstep");
animator.AddEvent("Walk", 0.8f, "footstep");
animator.AddEvent("Attack", 0.5f, "attack_hit");
```

### 40e: サンプルプロジェクト

- `Samples/LuaShowcase/main.cpp` — Lua スクリプティングデモ
- `Samples/TilemapShowcase/main.cpp` — 2D タイルマップデモ

---

## 共通ルール（全 Phase 共通）

### コーディング規約
- namespace `GX` 内に配置
- `#pragma once` + Doxygen `/// @file` / `/// @brief`
- `#include "pch.h"` を .cpp の先頭に
- DirectXMath は `using namespace DirectX;` を .cpp 内のみ
- XMFLOAT3/4/4X4 はメンバー格納用、XMVECTOR/XMMATRIX は演算用
- ComPtr で COM オブジェクト管理
- `std::unique_ptr` で所有権管理
- `constexpr` で定数定義
- エラーログは `GX::Logger::Error()`、情報ログは `GX::Logger::Info()`
- No CD3DX12 helpers — raw D3D12 structs を使用
- C++ に saturate() はない — `(std::max)(0.0f, (std::min)(1.0f, x))` を使用
- pch.h に `<sstream>`, `<unordered_set>`, `<deque>`, `<filesystem>` は **ない** — 代替を使用

### ビルド手順（各 Phase 完了時）
```bash
cmake -B build -S .
cmake --build build --config Debug
ctest --test-dir build --build-config Debug
```

### 新規ファイルの CMake 対応
- `GXLib/` 配下は `GLOB_RECURSE` で自動収集（新ディレクトリも含む）
- ただし `GXLib/AI/*.cpp` は Phase 29 で別途追加済み
- 新しい `GXLib/Script/*.cpp` は `file(GLOB_RECURSE GXLIB_SCRIPT_SOURCES GXLib/Script/*.cpp)` で追加
- `ThirdParty/` の C ファイル（Lua 等）は `SKIP_PRECOMPILE_HEADERS ON` + `LANGUAGE C`
- 新しいサンプルは `gxlib_add_sample(SampleName)` マクロ
- `Tests/CMakeLists.txt` に新テストファイルを手動追加

### MEMORY.md 更新
各 Phase 完了後に MEMORY.md の Completed Phases セクションと Common Issues を更新すること。

---

## 既知の技術的制約

| 制約 | 影響 | 回避策 |
|------|------|--------|
| pch.h に `<sstream>` なし | std::stringstream 使用不可 | std::format / std::to_string |
| pch.h に `<unordered_set>` なし | unordered_set 使用不可 | vector + sort + unique |
| pch.h に `<deque>` なし | std::deque 使用不可 | vector + リングバッファ |
| pch.h に `<filesystem>` なし | std::filesystem 使用不可 | Win32 API or `<cstdio>` std::remove |
| Windows.h min/max マクロ | std::min/max と衝突 | `(std::max)(...)` パターン |
| FormatT by-value 引数 | Args&& 使用不可 | P2905R2/MSVC14.44 制約 |
| ImTextureID = ImU64 | ポインタ直接渡し不可 | `static_cast<ImTextureID>(handle.ptr)` |
| std::vector<bool> proxy | ImGui::Checkbox 互換なし | ローカル bool にコピー |
| D3D12 Root SRV | Texture2D.Sample() 不可 | shader-visible ヒープ使用 |
| Color{1,1,1,1} 曖昧性 | int/float オーバーロード競合 | `Color(1.0f, 1.0f, 1.0f, 1.0f)` |

---

## 検証チェックリスト

各 Phase 完了時に以下を確認:

- [ ] `cmake -B build -S . && cmake --build build --config Debug` エラーゼロ
- [ ] `ctest --test-dir build --build-config Debug` 全テスト PASS
- [ ] 新規サンプルが起動してクラッシュしない
- [ ] 既存 22 サンプルが壊れていない
- [ ] GXModelViewer が起動する
- [ ] gxconv / gxpak がビルドできる
- [ ] MEMORY.md が更新されている

---

## 実装優先順序

```
Phase 36 (バグ修正) ──┐
Phase 37 (テスト)   ──┼── 並列着手可能
Phase 38 (ドキュメント)┘
        ↓
Phase 39 (レンダリング高度化) ← Phase 36 完了後
        ↓
Phase 40 (ゲームプレイ機能) ← Phase 36 完了後
```

Phase 39/40 内部の推奨順序:

**Phase 39:**
1. 39b (Indirect Drawing) — 既存パイプラインへの影響が最小
2. 39a (マルチスレッド) — アーキテクチャ変更が大きいため慎重に
3. 39c (Async Compute) — 39a の経験を活かして

**Phase 40:**
1. 40d (アニメーションイベント) — 最も軽量、すぐに効果
2. 40c (ルートモーション) — アニメーション品質向上
3. 40b (タイルマップ) — 2D ゲーム開発の基盤
4. 40a (Lua スクリプティング) — 最も大規模、外部依存あり

---

## ファイル一覧（全 Phase）

### Phase 36: 修正対象（BugReport.md 76件 + Extra 4件）
```
=== Critical ===
GXLib/GUI/Widgets/DropDown.cpp            — C-01: 配列範囲外アクセス
GXLib/GUI/Widgets/ListView.cpp            — C-02: 配列範囲外アクセス
GXLib/Graphics/Resource/TextureManager.cpp — C-03: 負インデックスアクセス
Sandbox/main.cpp                          — RT-C01: CreateGeometrySRVs() 追加
GXLib/Graphics/RayTracing/RTReflections.h  — RT-C02: ComPtr 化
GXLib/Graphics/RayTracing/RTReflections.cpp — RT-C03: ヒープスロット再設計

=== High ===
GXLib/Graphics/Rendering/TextRenderer.cpp  — H-01: vswprintf_s 引数
GXLib/Graphics/Rendering/SpriteBatch.cpp   — H-02: Map null チェック
GXLib/Graphics/Rendering/PrimitiveBatch.cpp — H-03: Map null チェック
GXLib/Graphics/PostEffect/AutoExposure.cpp — H-05: Map null チェック
GXLib/Graphics/RayTracing/RTReflections.cpp — H-06/H-08: HRESULT+null チェック
GXLib/Graphics/PostEffect/PostEffectPipeline.cpp — H-07: null バリア
GXLib/Graphics/PostEffect/SSR.cpp          — H-09: SRV バインド検証
GXLib/IO/Network/HTTPClient.cpp            — H-10: 非同期リソースリーク
GXLib/IO/Network/WebSocket.cpp             — H-11: Use-After-Free
GXLib/IO/AsyncLoader.cpp                   — H-12: レースコンディション
GXLib/Movie/MoviePlayer.cpp                — H-13: null デリファレンス
GXLib/Compat/Compat_2D.cpp                 — H-14: null+オーバーフロー
Shaders/RTReflections.hlsl                 — H-15: コメント修正, RT-H01: R16 対応
GXLib/Graphics/Resource/Texture.cpp        — H-16: 整数オーバーフロー
GXLib/Graphics/Device/BarrierBatch.h       — H-17a: 配列初期化
GXLib/GUI/Widgets/DropDown.cpp             — H-17b: 空アイテムガード
GXLib/Graphics/Rendering/FontManager.cpp   — H-18a: pixelData null
GXLib/GUI/Widgets/TextInput.cpp            — H-18b: 選択範囲境界
GXLib/GUI/Widgets/ScrollView.cpp           — H-19: ゼロ除算
GXLib/GUI/Widgets/Button.h + 7 files       — H-20: m_renderer null 統一
GXLib/Graphics/RayTracing/RTReflections.cpp — RT-H02: インスタンス上限
GXLib/Graphics/RayTracing/RTReflections.cpp — RT-H03: BLAS 連番化

=== Medium ===
GXLib/Graphics/Rendering/TextRenderer.cpp  — M-01: 改行比較
GXLib/Graphics/Rendering/FontManager.cpp   — M-02: 未初期化エントリ
GXLib/Graphics/Rendering/TextRenderer.cpp  — M-03: UV クランプ
GXLib/Graphics/RayTracing/RTReflections.cpp — M-04〜M-07, RT-M01〜RT-M05
GXLib/Graphics/PostEffect/Bloom.cpp        — M-08: エラー伝播
GXLib/Graphics/Resource/Texture.cpp        — M-09: CreateEvent
GXLib/IO/FileWatcher.cpp                   — M-10: ハンドルリーク
GXLib/IO/Crypto.cpp                        — M-11: エラーハンドリング
GXLib/IO/Archive.cpp                       — M-12: 整数オーバーフロー
GXLib/Audio/Sound.cpp                      — M-13: 読み込みエラー
GXLib/Audio/SoundPlayer.cpp                — M-14: コールバック寿命
GXLib/Physics/PhysicsWorld3D.cpp           — M-15: Shape null
GXLib/GUI/Widgets/TextInput.cpp            — M-16: off-by-one
GXLib/GUI/Widgets/TabView.cpp              — M-17: activeTab 範囲
GXLib/GUI/Widgets/DropDown.cpp             — M-18: selectedIndex
Samples/PostEffectShowcase/main.cpp        — M-19: VSync 矛盾
GXLib/GUI/Widgets/TextWidget.cpp           — M-20: 不整合
GXLib/Graphics/Rendering/SpriteBatch.cpp   — M-21: 境界チェック
GXLib/Math/Quaternion.h                    — MATH-01: ToEuler 符号
GXLib/Physics/PhysicsWorld2D.cpp           — MATH-02: 慣性モーメント
GXLib/Physics/PhysicsWorld2D.cpp           — MATH-03: AABB 回転
Shaders/RTReflections.hlsl                 — MATH-04: 法線変換
Shaders/RTReflectionComposite.hlsl         — RT-M07: コメント修正
Shaders/RTReflections.hlsl                 — RT-M06: 法線方向

=== Low ===
GXLib/Graphics/PostEffect/SSAO.cpp         — L-01: static_assert
GXLib/Graphics/PostEffect/VolumetricLight.cpp — L-02: 初期化
GXLib/Physics/MeshCollider.cpp             — L-03: 除算ゼロ
GXLib/Physics/PhysicsWorld2D.cpp           — L-04: Raycast null
GXLib/Math/Random.cpp                      — L-05: 無限ループ
GXLib/Math/Collision/Collision3D.cpp       — L-06: 除算ゼロ
GXLib/GUI/Widgets/Image.cpp                — L-07: UV 精度
GXLib/GUI/StyleSheet.cpp                   — L-08: 例外ハンドリング
Shaders/DepthOfField.hlsl                  — MATH-05: ガウス重み
Shaders/RTReflections.hlsl                 — TECH-01: ポイントライト shadow
GXLib/Physics/PhysicsWorld2D.cpp           — TECH-02: 角インパルス
GXLib/Physics/PhysicsWorld2D.cpp           — TECH-03: 摩擦速度
GXLib/Graphics/RayTracing/RTReflections.h/cpp — RT-L02: 変数名リネーム

=== Extra ===
GXLib/Core/Scene/Entity.h                  — GetComponent constexpr
GXLib/Core/Scene/Scene.cpp                 — DestroyEntity 安全性
GXLib/Graphics/Resource/DynamicBuffer.h    — Map 状態チェック
GXLib/Graphics/PostEffect/*.cpp            — null チェック全数確認
各ファイルの TODO/FIXME 項目                  — 個別対応
```

### Phase 37: 新規テスト（6 ファイル）
```
Tests/test_Spline.cpp          — 新規 (16 テスト)
Tests/test_Entity.cpp          — 新規 (17 テスト)
Tests/test_ActionMapping.cpp   — 新規 (5 テスト)
Tests/test_NavMesh.cpp         — 新規 (11 テスト)
Tests/test_LODGroup.cpp        — 新規 (5 テスト)
Tests/test_SceneSerializer.cpp — 新規 (7 テスト)
Tests/CMakeLists.txt           — 更新 (6 ファイル追加)
```
> **注意:** test_MathUtil.cpp, test_Crypto.cpp, test_Allocator.cpp は既に存在するため新規作成不要

### Phase 38: ドキュメント
```
docs/tutorials/01_GettingStarted.md  — 改善
docs/tutorials/02_2DDrawing.md       — 改善
docs/tutorials/03_InputAndSound.md   — 改善
docs/tutorials/04_3DRendering.md     — 改善
docs/tutorials/05_GUI.md             — 改善
docs/tutorials/06_GXEasy2DGame.md    — 新規
docs/tutorials/07_3DScene.md         — 新規
docs/tutorials/08_AssetPipeline.md   — 新規
docs/api/GXEasy.md                   — 新規
docs/api/Math.md                     — 新規
docs/api/Input.md                    — 新規
docs/api/Audio.md                    — 新規
docs/api/Graphics.md                 — 新規
docs/api/Scene.md                    — 新規
docs/Glossary.md                     — 拡充
README.md                            — 改善
```

### Phase 39: 新規/修正
```
GXLib/Graphics/Device/ParallelCommandRecorder.h   — 新規
GXLib/Graphics/Device/ParallelCommandRecorder.cpp  — 新規
GXLib/Graphics/Device/IndirectCommandBuffer.h      — 新規
GXLib/Graphics/Device/IndirectCommandBuffer.cpp    — 新規
GXLib/Graphics/Device/AsyncComputeQueue.h          — 新規
GXLib/Graphics/Device/AsyncComputeQueue.cpp        — 新規
GXLib/Graphics/3D/Renderer3D.h                     — 並列モード追加
GXLib/Graphics/3D/Renderer3D.cpp                   — 〃
Samples/MultiThreadShowcase/CMakeLists.txt         — 新規
Samples/MultiThreadShowcase/main.cpp               — 新規
```

### Phase 40: 新規/修正
```
GXLib/Script/ScriptEngine.h           — 新規
GXLib/Script/ScriptEngine.cpp         — 新規
GXLib/Script/ScriptBindings.h         — 新規
GXLib/Script/ScriptBindings.cpp       — 新規
GXLib/Graphics/Rendering/Tilemap.h    — 新規
GXLib/Graphics/Rendering/Tilemap.cpp  — 新規
GXLib/Graphics/3D/Animator.h          — ルートモーション+イベント追加
GXLib/Graphics/3D/Animator.cpp        — 〃
GXLib/Graphics/3D/Animation.h         — AnimationEvent 追加
GXLib/CMakeLists.txt                  — Script/ GLOB + Lua 追加
Samples/LuaShowcase/CMakeLists.txt    — 新規
Samples/LuaShowcase/main.cpp          — 新規
Samples/TilemapShowcase/CMakeLists.txt — 新規
Samples/TilemapShowcase/main.cpp      — 新規
```
