# GXLib Bug Report

**Date:** 2026-02-17
**Scope:** Full project scan + Ray Tracing 詳細調査 + 計算式・手法検証

---

## 全体サマリー

| セクション | Critical | High | Medium | Low/Info | 合計 |
|---|---|---|---|---|---|
| 全体スキャン | 3 | 21 | 21 | 8 | 53 |
| RT 詳細調査 | 3 | 3 | 7 | 5 | 18 |
| 計算式・手法検証 | 0 | 0 | 7 | 3 | 10 |
| **合計（重複除く）** | **6** | **24** | **32** | **14** | **76** |

> **注:** H-04（Skeleton 行列乗算順序）は計算式検証で正しいことが確認されたため除外済み。

---

## Critical

### C-01: DropDown - 配列の範囲外アクセス
- **File:** `GXLib/GUI/Widgets/DropDown.cpp:178`
- **Category:** Buffer Overflow
- **Description:** `RenderSelf()` 内のラムダで `wideItems[i]` にアクセスするが、`m_items` と `m_wideItems` のサイズが異なる場合（`SetItems()` レンダリング中呼び出し等）、範囲外アクセスが発生する。

### C-02: ListView - 配列の範囲外アクセス
- **File:** `GXLib/GUI/Widgets/ListView.cpp:134`
- **Category:** Buffer Overflow
- **Description:** `m_wideItems[i]` にアクセスする際、`i` は `m_items.size()` で制限されるが、`m_wideItems` のサイズが同一であることを検証していない。サイズ不一致時に範囲外アクセスが発生。

### C-03: TextureManager::CreateRegionHandles - 負インデックスで配列アクセス
- **File:** `GXLib/Graphics/Resource/TextureManager.cpp:184-187`
- **Category:** Out-of-Bounds Access
- **Description:** `AllocateHandle()` が失敗時に返す値を検証せずに `m_entries[handle]` にアクセス。負のインデックスで vector にアクセスするため未定義動作。ループ内の `if (firstHandle == -1)` は最初の呼び出しのみチェックしており、AllocateHandle の失敗チェックではない。

---

## High

### H-01: TextRenderer - vswprintf_s のバッファサイズ引数欠落
- **File:** `GXLib/Graphics/Rendering/TextRenderer.cpp:157`
- **Category:** Buffer Overflow
- **Description:** `vswprintf_s(buffer, format, args)` で第2引数のバッファサイズ（1024）が欠落。セキュア版 `vswprintf_s` は4引数必須。バッファオーバーフローのリスク。

### H-02: SpriteBatch::Begin - Map の戻り値未チェック
- **File:** `GXLib/Graphics/Rendering/SpriteBatch.cpp:199`
- **Category:** Null Pointer Dereference
- **Description:** `m_mappedVertices = static_cast<SpriteVertex*>(m_vertexBuffer.Map(frameIndex))` の戻り値が null の場合、後続の `AddQuad()` で null ポインタ参照。

### H-03: PrimitiveBatch::Begin - Map の戻り値未チェック
- **File:** `GXLib/Graphics/Rendering/PrimitiveBatch.cpp:133-134`
- **Category:** Null Pointer Dereference
- **Description:** `m_mappedTriVertices` と `m_mappedLineVertices` の両方が、`Map()` 失敗時に null のまま描画関数で使用される。

### ~~H-04: Skeleton - 行列乗算の順序誤り~~ **【誤検出 — 正しいことを確認済み】**
- **File:** `GXLib/Graphics/3D/Skeleton.cpp:35`
- **Category:** ~~Math Error~~ → **False Positive**
- **Description:** `XMMATRIX bone = invBind * global;` は DirectX の行ベクトル規約（v × M）で正しい順序。`invBindPose × currentGlobalTransform` はスケルタルアニメーションの標準的な骨行列合成公式であり、バグではない。計算式検証レポート（後述）で検証確認済み。

### H-05: AutoExposure - マップドポインタの未検証デリファレンス
- **File:** `GXLib/Graphics/PostEffect/AutoExposure.cpp:231`
- **Category:** Null Pointer Dereference
- **Description:** `Map()` 成功後、`mapped` ポインタを null チェックせずに `*reinterpret_cast<const uint16_t*>(mapped)` でデリファレンス。

### H-06: RTReflections::OnResize - HRESULT 未チェック
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:574-577`
- **Category:** Missing Error Handling
- **Description:** `CreateCommittedResource` の戻り値を確認していない。リソース作成失敗時 `m_halfResUAV` が null のまま後続で使用される。

### H-07: PostEffectPipeline - null リソースへの UAV バリア
- **File:** `GXLib/Graphics/PostEffect/PostEffectPipeline.cpp:440-446`
- **Category:** Null Pointer Dereference
- **Description:** `m_halfResUAV.Get()` が null の場合（H-06 の結果）、null リソースに対して UAV バリアが発行される。

### H-08: RTReflections - m_normalRT の null チェック不完全
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:308-360`
- **Category:** Null Pointer Dereference
- **Description:** `m_normalRT` が null の場合、`CreateShaderResourceView` に nullptr が渡される。一部コードパスでは null チェックがあるが、全パスでは不統一。

### H-09: SSR - normalRT の SRV バインド未検証
- **File:** `GXLib/Graphics/PostEffect/SSR.cpp:138`
- **Category:** Resource Binding Mismatch
- **Description:** `UpdateSRVHeap` で normalRT の SRV を常に作成するが、バインド状態の検証が欠如しており、他システムによる変更でシェーダーリソースの不整合が発生する可能性。

### H-10: HTTPClient - 非同期操作のリソースリーク
- **File:** `GXLib/IO/Network/HTTPClient.cpp:195-210`
- **Category:** Resource Leak / Use-After-Free
- **Description:** `GetAsync()`/`PostAsync()` でスレッドが `.detach()` で起動されるが、HTTPClient 破棄時にスレッドが実行中の場合、キャプチャした `this` ポインタがダングリングになる。

### H-11: WebSocket - ReceiveLoop の Use-After-Free
- **File:** `GXLib/IO/Network/WebSocket.cpp:115-250`
- **Category:** Use-After-Free
- **Description:** `Close()` で `m_running=false` 設定後にスレッドを join するが、`ReceiveLoop()` が `m_hWebSocket` アクセス中に `Close()` でハンドルが無効化される競合。mutex による同期が必要。

### H-12: AsyncLoader - 破棄時のレースコンディション
- **File:** `GXLib/IO/AsyncLoader.cpp:12-21`
- **Category:** Race Condition
- **Description:** デストラクタで `m_running=false` 後に `notify_one()` するが、ワーカースレッドの `m_completedQueue` / `m_statusMap` アクセスと同時実行される競合。

### H-13: MoviePlayer - null ポインタデリファレンス
- **File:** `GXLib/Movie/MoviePlayer.cpp:104, 288-299`
- **Category:** Null Pointer Dereference
- **Description:** `pOutputType->Release()` が null チェックなし。`Close()` 後に `m_texManager` が nullptr のまま使用される可能性。

### H-14: Compat_2D LoadDivGraph - null ポインタ・バッファオーバーフロー
- **File:** `GXLib/Compat/Compat_2D.cpp:61-81`
- **Category:** Null Pointer / Buffer Overflow
- **Description:** `handleBuf` ポインタの null チェックが欠如。また `allNum` の妥当性検証なし。大きな値で配列外書き込みが発生。

### H-15: RTReflections.hlsl - cbuffer コメントとアクセスの不一致
- **File:** `Shaders/RTReflections.hlsl:40-44, 238-239`
- **Category:** Shader Resource Mismatch
- **Description:** `g_InstanceRoughnessGeom` は `.x=roughness, .y=geometryIndex` のみ文書化されているが、シェーダーで `.z`(texIdx) と `.w`(hasTexture) にもアクセス。C++ 側で `.z/.w` が未設定の場合、未初期化データが使用される。

### H-16: Texture - 大サイズテクスチャでの整数オーバーフロー
- **File:** `GXLib/Graphics/Resource/Texture.cpp:104-106, 246-248`
- **Category:** Integer Overflow
- **Description:** `rowPitch * height` の計算で uint32_t オーバーフローが発生する可能性。例: 16384x16384 テクスチャで `65536 * 16384 = 1GB` が uint32_t 上限を超え、バッファサイズが不足してバッファオーバーフロー。

### H-17: BarrierBatch - m_barriers 配列の未初期化
- **File:** `GXLib/Graphics/Device/BarrierBatch.h:22, 62-64`
- **Category:** Uninitialized Data
- **Description:** `m_count` は 0 に初期化されるが、`m_barriers[]` 配列はゼロクリアされない。バリアを Add して Flush する正常フローでは問題ないが、`m_count` が不正な値を持った場合、ガベージデータが GPU に送られる。

### H-18: FontManager - pixelData の null チェック欠如
- **File:** `GXLib/Graphics/Rendering/FontManager.cpp:313`
- **Category:** Null Pointer Dereference
- **Description:** `lock->GetDataPointer(&bufferSize, &pixelData)` 後、`pixelData` が null でないことを確認せずに memcpy ループで使用。

### H-17: DropDown::OnEvent - 空アイテム時の範囲外アクセス
- **File:** `GXLib/GUI/Widgets/DropDown.cpp:61`
- **Category:** Out-of-Bounds
- **Description:** `SetItems()` で空ベクトルを設定した場合、`m_selectedIndex=0` が残り、`onValueChanged(m_items[m_selectedIndex])` で範囲外アクセス。

### H-18: TextInput::DeleteSelection - 選択範囲の境界値不正
- **File:** `GXLib/GUI/Widgets/TextInput.cpp:93`
- **Category:** Logic Error
- **Description:** `m_selStart`/`m_selEnd` が不正な値の場合、`erase(s, e-s)` に負の値が渡される可能性。

### H-19: ScrollView - ゼロ除算リスク
- **File:** `GXLib/GUI/Widgets/ScrollView.cpp:25`
- **Category:** Division by Zero
- **Description:** `viewH` が 0 または負の場合、スクロール計算のクランプロジックが破綻。

### H-20: 複数ウィジェット - m_renderer の null チェック不統一
- **Files:** `Button.h, TextWidget.h, CheckBox.h, DropDown.h, ListView.h, RadioButton.h, TabView.h, TextInput.h`
- **Category:** Null Pointer Dereference
- **Description:** 複数のウィジェットで `m_renderer` を null チェックなしで使用するコードパスが存在。一部のメソッドではチェックしているが不統一。

---

## Medium

### M-01: TextRenderer - 改行文字の比較誤り
- **File:** `GXLib/Graphics/Rendering/TextRenderer.cpp:101`
- **Category:** Logic Error
- **Description:** `DrawStringTransformed()` で `L'\\n'`（エスケープされた文字列）と比較しており、実際の改行文字 `L'\n'` を正しく検出できない。`DrawString()` の line 41 では正しく `L'\n'` と比較。

### M-02: FontManager - 未初期化エントリへのアクセス
- **File:** `GXLib/Graphics/Rendering/FontManager.cpp:410`
- **Category:** Uninitialized Data
- **Description:** `AllocateHandle()` がフリーリストからハンドルを再利用する場合、`m_entries[handle]` が未初期化の `FontEntry` を返す可能性。

### M-03: TextRenderer - テクスチャ座標のオーバーフロー
- **File:** `GXLib/Graphics/Rendering/TextRenderer.cpp:65`
- **Category:** Integer Overflow
- **Description:** `glyph->u0 * FontManager::k_AtlasSize` が float→int キャスト時に k_AtlasSize(2048) と等しい場合、テクスチャ範囲外アクセス。クランプ処理なし。

### M-04: RTReflections - 冗長なリソース状態遷移
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:306-309, 466-469`
- **Category:** Logic Error
- **Description:** `srcHDR`, `depth`, `m_normalRT` が NON_PIXEL_SHADER_RESOURCE に遷移後、PIXEL_SHADER_RESOURCE に再遷移するが、状態チェックなし。冗長な GPU コマンド。

### M-05: RTReflections::BuildBLAS - エラーハンドリング不足
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:131-142`
- **Category:** Missing Error Handling
- **Description:** `BuildBLAS` の戻り値 `idx` が範囲外の場合、`m_blasGeometry.resize(idx + 1)` でギャップのあるデフォルト初期化エントリが生成される。

### M-06: RTAccelerationStructure - ストライド検証不足
- **File:** `GXLib/Graphics/RayTracing/RTAccelerationStructure.cpp:42`
- **Category:** Integer Overflow
- **Description:** `VertexBuffer.StrideInBytes` が 0 の場合や vertexCount との積でオーバーフローする場合、無効な GPU 操作が発生。

### M-07: RTReflections - テクスチャスロットオーバーフロー
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:180-185`
- **Category:** Buffer Overflow
- **Description:** 32 テクスチャを超えて追加された場合、ディスクリプタヒープサイズ（40+32=72）を超える SRV 作成が発生する可能性。

### M-08: Bloom::OnResize - エラー伝播なし
- **File:** `GXLib/Graphics/PostEffect/Bloom.cpp:296-299`
- **Category:** Missing Error Handling
- **Description:** `CreateMipRenderTargets` が void 型のため、`RenderTarget::Create` 失敗時のエラーが伝播されない。無効なレンダーターゲットで処理続行。

### M-09: Texture - CreateEvent の戻り値未チェック
- **File:** `GXLib/Graphics/Resource/Texture.cpp:213, 356`
- **Category:** Missing Error Handling
- **Description:** `CreateEvent()` が NULL を返した場合のエラーハンドリングが欠如。`WaitForSingleObject` に NULL ハンドルが渡される。

### M-10: FileWatcher - イベントハンドルリーク
- **File:** `GXLib/IO/FileWatcher.cpp:42, 57`
- **Category:** Resource Leak
- **Description:** `CreateEventA()` の戻り値を確認せず、NULL でも `ReadDirectoryChangesW()` に渡される。

### M-11: Crypto - 不統一なエラーハンドリング
- **File:** `GXLib/IO/Crypto.cpp:17-30, 83-93`
- **Category:** Error Handling
- **Description:** `Encrypt()` と `Decrypt()` でエラーログの出力が不統一。一部パスで `hAlg` のクリーンアップが欠落。

### M-12: Archive - 整数オーバーフロー
- **File:** `GXLib/IO/Archive.cpp:62`
- **Category:** Integer Overflow
- **Description:** `tocSize` が極端に大きい場合、`resize()` でメモリ不足が発生するが、上限チェックがない。不正なアーカイブファイルで DoS 可能。

### M-13: Sound - ファイル読み込みエラーハンドリング不足
- **File:** `GXLib/Audio/Sound.cpp:44, 60-61, 68-72`
- **Category:** Uninitialized Data
- **Description:** 複数の `file.read()` 呼び出しで読み取り成功を確認していない。トランケートされた WAV ファイルで部分的に読み取られたデータが有効として処理される。

### M-14: SoundPlayer - コールバックの寿命管理
- **File:** `GXLib/Audio/SoundPlayer.cpp:38-47`
- **Category:** Dangling Pointer
- **Description:** `VoiceCallback` の所有権が `m_activeVoices` に移動されるが、外部からボイスが破棄された場合コールバックがダングリングに。

### M-15: PhysicsWorld3D - シェイプ作成の null チェック不足
- **File:** `GXLib/Physics/PhysicsWorld3D.cpp:286, 295, 304`
- **Category:** Null Pointer Dereference
- **Description:** `new JPH::ShapeRefC(...)` の結果を検証せずに割り当て。Jolt 側で例外が発生した場合のハンドリングなし。

### M-16: TextInput - ループ条件の off-by-one
- **File:** `GXLib/GUI/Widgets/TextInput.cpp:203`
- **Category:** Logic Error
- **Description:** `for (int i = 1; i <= static_cast<int>(display.size()); ++i)` で `<=` 使用。空文字列時に不正なアクセス。

### M-17: TabView - activeTab の範囲検証なし
- **File:** `GXLib/GUI/Widgets/TabView.cpp:40`
- **Category:** Logic Error
- **Description:** `m_activeTab` が -1 や children.size() 以上の場合、全子要素が非表示になる。範囲検証なし。

### M-18: DropDown::SetItems - selectedIndex の検証不正
- **File:** `GXLib/GUI/Widgets/DropDown.cpp:26-27`
- **Category:** Off-by-One
- **Description:** `m_selectedIndex >= items.size()` でリセットするが、`items` が空の場合 `m_selectedIndex=0` が設定され不正。

### M-19: PostEffectShowcase - VSync と TargetFps の矛盾
- **File:** `Samples/PostEffectShowcase/main.cpp:44-45`
- **Category:** Logic Error
- **Description:** `config.vsync = true` と `config.targetFps = 240` が同時設定。VSync 有効時は `targetFps` が無視されるため、設定が矛盾。

### M-20: TextWidget - GetIntrinsicHeight のフォールバック不整合
- **File:** `GXLib/GUI/Widgets/TextWidget.cpp:15`
- **Category:** Logic Error
- **Description:** `m_fontHandle < 0` の場合 `computedStyle.fontSize` を返すが、`GetIntrinsicWidth()` は同条件で `0.0f` を返す。不整合な挙動。

### M-21: SpriteBatch - AddQuad の境界チェック不正
- **File:** `GXLib/Graphics/Rendering/SpriteBatch.cpp:246`
- **Category:** Logic Error
- **Description:** `m_vertexWriteOffset + m_spriteCount >= k_MaxSprites` のチェックは、現在追加中の4頂点を考慮していない。境界条件で1スプライト分オーバーする可能性。

---

## Low

### L-01: SSAO - カーネルサイズ0で除算ゼロ
- **File:** `GXLib/Graphics/PostEffect/SSAO.cpp`
- **Category:** Division by Zero
- **Description:** `k_KernelSize == 0` の場合、カーネルスケール計算で除算ゼロ。実質的にはコンパイル時定数のため発生しにくい。

### L-02: VolumetricLight - 未初期化 XMFLOAT3
- **File:** `GXLib/Graphics/PostEffect/VolumetricLight.cpp:126-127`
- **Category:** Uninitialized Variable
- **Description:** `sunNDC` が `XMStoreFloat3` で条件的に割り当て。Transform が無効データを返した場合、未初期化値が使用される。

### L-03: MeshCollider - 除算ゼロ
- **File:** `GXLib/Physics/MeshCollider.cpp:50, 79`
- **Category:** Division by Zero
- **Description:** `1.0f / weld` で `weld=0` のガードはあるが、負の値が通過する。`step = vertices.size() / maxPoints` で `step=0` の可能性。

### L-04: PhysicsWorld2D - Raycast 出力ポインタ未検証
- **File:** `GXLib/Physics/PhysicsWorld2D.cpp:260-262`
- **Category:** Null Pointer Dereference
- **Description:** `outBody`, `outPoint`, `outNormal` を null チェックせずデリファレンス。

### L-05: Random - 無限ループリスク
- **File:** `GXLib/Math/Random.cpp:54-98`
- **Category:** Infinite Loop
- **Description:** Rejection sampling の `for(;;)` ループに最大試行回数の制限なし。乱数生成が壊れた場合に無限ループ。

### L-06: Collision3D::ClosestPointOnLine - 除算ゼロ
- **File:** `GXLib/Math/Collision/Collision3D.cpp:426`
- **Category:** Division by Zero
- **Description:** `ab.Dot(ab)` が除算に使用されるが、線分長ゼロ（同一点）の場合 infinity/NaN が発生。

### L-07: Image Widget - UV オフセットの浮動小数点精度
- **File:** `GXLib/GUI/Widgets/Image.cpp:16-19`
- **Category:** Float Precision
- **Description:** `std::fmod()` で UV オフセットをラップするが、連続更新で浮動小数点誤差が蓄積。

### L-08: StyleSheet::ParseLength - 例外ハンドリング欠如
- **File:** `GXLib/GUI/StyleSheet.cpp:557`
- **Category:** Exception Handling
- **Description:** `std::stof()` が不正フォーマットで例外を投げるが、try-catch がない。

---

## Priority Fix Order

1. **Critical (C-01, C-02):** GUI の配列範囲外アクセス - クラッシュ直結
2. **High (H-01):** vswprintf_s のバッファサイズ - セキュリティ脆弱性
3. **High (H-10, H-11, H-12):** スレッド安全性 - 非同期クラッシュ
4. **High (H-02, H-03, H-05):** null チェック - 初期化失敗時のクラッシュ
5. **High (H-15):** シェーダー cbuffer 不一致 - レンダリング不正
6. **Medium (M-01):** 改行文字の比較誤り - テキスト描画不正

---
---

# Ray Tracing 詳細調査レポート

**Date:** 2026-02-17
**Scope:** GXLib/Graphics/RayTracing/, Shaders/RTReflections*.hlsl, 統合ポイント (PostEffectPipeline, Sandbox, DXRShowcase)

## 概要

レイトレーシングサブシステム全体を C++ コード・HLSL シェーダー・統合ポイントの 3 軸で精査した結果、**Critical 3件、High 3件、Medium 7件、Low/Info 5件** の不整合を検出。

---

## RT-Critical

### RT-C01: Sandbox で CreateGeometrySRVs() が呼ばれていない
- **File:** `Sandbox/main.cpp`
- **Category:** Missing Initialization
- **Description:** BLAS 構築後に `CreateGeometrySRVs()` が呼ばれていない。Dispatch ヒープのスロット [8..39]（ジオメトリ VB/IB SRV）および [40..71]（アルベドテクスチャ SRV）が初期化されないまま DispatchRays が実行される。
- **Impact:** ClosestHit シェーダーで `g_GeometryBuffers[geomIdx*2]` アクセス時にゴミデータまたは無効ディスクリプタが参照される。GPU ハング・クラッシュの原因。
- **Verification:** `Samples/DXRShowcase/main.cpp:459` では正しく `CreateGeometrySRVs()` が呼ばれている。Sandbox には該当呼び出しが存在しない（grep 確認済み）。
- **Fix:** BLAS 構築 + GPU フラッシュ後に `g_rtReflections->CreateGeometrySRVs();` を追加。

### RT-C02: リソースポインタの寿命管理不備（Use-After-Free リスク）
- **File:** `GXLib/Graphics/RayTracing/RTReflections.h:184-196`
- **Category:** Memory Safety
- **Description:** `m_textureResources` と `m_blasGeometry` が `std::vector<ID3D12Resource*>`（生ポインタ）で保持されている。アプリケーション側でテクスチャやメッシュが解放・再作成された場合、ダングリングポインタが残る。
- **Impact:** シェーダーが解放済み GPU メモリを読み取り → GPU ハング、TDR、不正レンダリング。
- **Fix:** `ComPtr<ID3D12Resource>` に変更するか、フレーム開始時に参照を再取得する設計に変更。

### RT-C03: ディスクリプタヒープ スロット衝突（frameIndex >= 2 の場合）
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:341`
- **Category:** Descriptor Heap Collision
- **Description:** `uint32_t heapBase = frameIndex * 4;` でフレーム毎のディスクリプタ位置を計算。ジオメトリ SRV は固定スロット 8 から開始。
  - Frame 0: heapBase=0（スロット 0-3） → OK
  - Frame 1: heapBase=4（スロット 4-7） → OK
  - Frame 2: heapBase=8（スロット 8-11） → **ジオメトリスロットと衝突**
  - Frame 3: heapBase=12（スロット 12-15） → **衝突**
- **Risk:** 現在 `k_BufferCount=2` のため frameIndex は 0 か 1 で実質問題ないが、バッファ数変更時に即座に破綻する脆弱な設計。
- **Fix:** フレーム毎スロットをジオメトリ/テクスチャスロットの後に配置するか、専用ヒープで分離。

---

## RT-High

### RT-H01: R16_UINT インデックスバッファ非対応（シェーダー側ハードコード）
- **File:** `Shaders/RTReflections.hlsl:193-196`
- **Category:** Format Compatibility
- **Description:** ClosestHit シェーダーでインデックスバッファのロードが R32_UINT 前提でハードコード：
  ```hlsl
  uint i0 = ib.Load(primIdx * 12 + 0);   // 4バイト×3 = 12バイト/三角形
  uint i1 = ib.Load(primIdx * 12 + 4);
  uint i2 = ib.Load(primIdx * 12 + 8);
  ```
  しかし C++ 側 `RTReflections::BuildBLAS()` は `DXGI_FORMAT_R16_UINT` も受け付ける。R16_UINT 使用時はストライド 6 バイト/三角形になるため、完全に誤ったインデックスがロードされる。
- **Impact:** R16_UINT メッシュの反射が完全に壊れる（不正な三角形参照 → ゴミジオメトリ）。
- **Fix:** (a) BuildBLAS で R32_UINT のみ許可するか、(b) インデックスフォーマットを cbuffer でシェーダーに渡して分岐。

### RT-H02: AddInstance() にインスタンス数上限チェックなし
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:165-203`
- **Category:** Buffer Overflow
- **Description:** `AddInstance()` は `m_instanceData.push_back()` するが、`k_MaxInstances=512` を超えた場合のガードがない。GPU 側 cbuffer は 512 要素固定（`g_InstanceAlbedoMetallic[512]`）のため、512 を超える `InstanceIndex()` でシェーダーが配列外アクセス。
- **Impact:** シェーダー内での未定義動作（GPU ハング、不正レンダリング）。
- **Fix:** `if (m_instanceData.size() >= k_MaxInstances) { LOG_WARN(...); return; }`

### RT-H03: BLAS ジオメトリ配列のギャップ（不連続インデックス）
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:138-140`
- **Category:** Array Integrity
- **Description:** `BuildBLAS` の戻り値 `idx` が不連続の場合、`m_blasGeometry.resize(idx + 1)` でギャップ要素がデフォルト初期化される。`CreateGeometrySRVs()` はギャップ位置で null リソースの SRV を作成する。
- **Impact:** ClosestHit で `geomIdx` がギャップ位置を指すと、null SRV からの読み取りが発生。

---

## RT-Medium

### RT-M01: 深度バッファ状態遷移の非対称性
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:307, 467, 547`
- **Category:** Resource State
- **Description:** 深度バッファの状態遷移フロー：
  1. L307: `NON_PIXEL_SHADER_RESOURCE` に遷移（Dispatch 用）
  2. L467: `PIXEL_SHADER_RESOURCE` に遷移（Composite 用）
  3. L547: `DEPTH_WRITE` に遷移（復元）
  呼び出し元（PostEffectPipeline）が深度を `PIXEL_SHADER_RESOURCE` として期待している場合、`DEPTH_WRITE` で返却されるため不整合。

### RT-M02: OnResize() でディスクリプタヒープ未更新
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:550-580`
- **Category:** Resize Handling
- **Description:** `OnResize()` で `m_halfResUAV` は再作成されるが、`m_dispatchHeap` と `m_compositeHeap` は更新されない。次の `Execute()` でディスクリプタが再作成されるため通常動作するが、リサイズ直後の描画で旧ディスクリプタが使用されるリスク。

### RT-M03: 深度 SRV フォーマットのハードコード
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:354`
- **Category:** Format Assumption
- **Description:** `srvDesc.Format = DXGI_FORMAT_R32_FLOAT` が固定値。`DepthBuffer` の実際のフォーマット（`D24_UNORM_S8_UINT` 等）と不一致の場合、SRV 作成失敗または不正データ。
- **Fix:** `depth.GetResource()->GetDesc().Format` からフォーマットを取得。

### RT-M04: テクスチャスロットの SRV がフレーム跨ぎで残留
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:156-163`
- **Category:** Stale Descriptor
- **Description:** `BeginFrame()` で `m_textureLookup` と `m_textureResources` はクリアされるが、ディスパッチヒープのスロット [40..71] にある SRV ディスクリプタは上書きされない。同一スロットに異なるテクスチャが割り当てられた場合、旧フレームの SRV が参照される可能性。

### RT-M05: m_normalRT の null チェック不完全
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:308-360`
- **Category:** Null Safety
- **Description:** `m_normalRT` が null の場合、L360 で `nullptr` が `CreateShaderResourceView` に渡される。D3D12 は null SRV を許容するが、シェーダー側（`g_Normal.SampleLevel()`）では null チェックなく使用される。

### RT-M06: ClosestHit の法線方向保証なし
- **File:** `Shaders/RTReflections.hlsl:221-228`
- **Category:** Shader Logic
- **Description:** 補間頂点法線 `N` は表面向きであることが保証されていない。ジオメトリック法線 `Ng` には表裏チェック（L227-228）があるが、シェーディング法線 `N` にはない。裏面ヒット時に BRDF 計算が不正になる。
- **Mitigation:** L310-312 の grazing angle 補正で部分的に対処されているが、完全ではない。

### RT-M07: Composite パスの alpha コメント誤記
- **File:** `Shaders/RTReflectionComposite.hlsl:89`
- **Category:** Documentation
- **Description:** コメント「alpha = フレネル × ヒット種別重み」だが、実際は alpha はヒット種別のみ（Miss=0.5, ClosestHit=1.0）。フレネルは BRDF 計算で RGB に含まれる。

---

## RT-Low/Info

### RT-L01: cbuffer コメントの不完全さ（既報 H-15 の詳細）
- **File:** `Shaders/RTReflections.hlsl:40-44`
- **Description:** `g_InstanceRoughnessGeom` のコメントが `.x=roughness, .y=geometryIndex` のみ。実際は `.z=texIdx, .w=hasTexture` も使用されている。C++ 側（`RTReflections.cpp:201`）では全 4 成分を設定済み。

### RT-L02: 半解像度変数名が実態と不一致
- **File:** `GXLib/Graphics/RayTracing/RTReflections.h/cpp`
- **Description:** `m_halfResUAV`, `m_halfWidth`, `m_halfHeight` が実際にはフル解像度で使用されている。MEMORY.md にも「フル解像度 dispatch」と記載あり。

### RT-L03: SetCommandList4() が毎フレーム呼び出し
- **File:** `Sandbox/main.cpp, PostEffectPipeline.h:129`
- **Description:** `SetCommandList4()` は初期化時に 1 回で十分だが、毎フレーム呼ばれている。パフォーマンス影響は微小だが設計として冗長。

### RT-L04: HDR クランプ値の固定
- **File:** `Shaders/RTReflections.hlsl:340`
- **Description:** `color = min(color, 5.0)` で HDR 値を 5.0 にクランプ。ACES Tonemap で ACES(5.0)≈0.96 のため妥当だが、定数化されておらずトーンマッパー変更時に調整が必要。

### RT-L05: ポイントライトのインスタンスマスク未対応
- **File:** `Shaders/RTReflections.hlsl:325-338`
- **Description:** ClosestHit でポイントライトは全インスタンスに一律適用。TLAS インスタンスマスクによるライティング除外に非対応。デモ用途では問題ないが、プロダクション用途では制限。

---

## RT 修正優先順位

1. **RT-C01 (CRITICAL):** Sandbox の `CreateGeometrySRVs()` 追加 — 即座にクラッシュ回避
2. **RT-C02 (CRITICAL):** リソースポインタを `ComPtr` 化 — Use-After-Free 防止
3. **RT-H01 (HIGH):** R16_UINT インデックス対応 or 制限 — メッシュ互換性確保
4. **RT-H02 (HIGH):** AddInstance の上限チェック追加 — バッファオーバーフロー防止
5. **RT-C03 (CRITICAL):** ディスクリプタヒープスロット設計見直し — 将来の拡張性確保
6. **RT-H03 (HIGH):** BLAS インデックスの連続性保証 — null SRV 回避
7. **RT-M01〜M07:** 中優先度の修正（深度状態遷移、フォーマット検証、法線安全性等）

---
---

# 計算式・手法 検証レポート

**Date:** 2026-02-17
**Scope:** PBR/BRDF 数式、3D 数学（行列・クォータニオン）、物理シミュレーション、DXR レイトレーシング、ポストエフェクト

## 概要

プロジェクト全体の計算式と使用手法を検証した結果、**数式バグ 5件、手法上の問題 3件、品質制限 2件** を検出。PBR BRDF（GGX NDF, Schlick Fresnel, Smith Geometry, Cook-Torrance）およびレンダリングパイプライン（シェーダーテーブル、BLAS/TLAS 構築、リソースバインディング、cbuffer レイアウト）は全て正確であることを確認。

---

## 検証済み（正確）

以下の数式・手法は全て正しいことを検証済み：

| 対象 | ファイル | 結果 |
|---|---|---|
| GGX NDF (Trowbridge-Reitz) | `PBRCommon.hlsli:12-23` | alpha=roughness^2 の Disney 規約、除算ゼロガード付き ✓ |
| Schlick Fresnel | `PBRCommon.hlsli:47-50` | F0 + (1-F0)(1-cosθ)^5、saturate ガード ✓ |
| Smith Geometry (Schlick-GGX) | `PBRCommon.hlsli:28-42` | k=(r+1)^2/8 は直接光用で正しい ✓ |
| Cook-Torrance BRDF | `PBRCommon.hlsli:55-80` | DGF/(4·NdotV·NdotL) + ε ✓ |
| Lambert Diffuse | `PBRCommon.hlsli:76` | albedo/π 正規化 ✓ |
| エネルギー保存 | `PBRCommon.hlsli:73-74` | kD=(1-kS)(1-metallic) ✓ |
| 法線マッピング TBN | `PBR.hlsl:200-205` | float3x3(T,B,N) + mul(normalMap,TBN) ✓ |
| Specular Anti-Aliasing | `PBR.hlsl:220-226` | Tokuyoshi & Kaplanyan 2019 方式 ✓ |
| ワールド座標復元 | `RTReflections.hlsl:83-89` | UV→NDC→clip→invVP→perspDiv ✓ |
| 重心座標補間 | `RTReflections.hlsl:214-219` | DXR BuiltInTriangleIntersectionAttributes 準拠 ✓ |
| シェーダーテーブル配置 | `RTPipeline.cpp:202-237` | AlignUp(32,32)=32, テーブル64B境界 ✓ |
| DispatchRays 記述子 | `RTPipeline.cpp:250-261` | Stride, Size, StartAddress 全正確 ✓ |
| BLAS/TLAS 構築 | `RTAccelerationStructure.cpp` | PrebuildInfo, UAV バリア, Transform 転置 ✓ |
| ルートシグネチャ←→HLSL | `RTPipeline.cpp + RTReflections.hlsl` | 全7パラメータ一致 ✓ |
| cbuffer レイアウト | `RTReflections.h:22-44 + .hlsl:14-37` | 320B C++/HLSL 完全一致 ✓ |
| SSAO サンプリング | `SSAO.hlsl` | 半球サンプル、レンジチェック、bilateral blur ✓ |
| Bloom Karis Average | `Bloom.hlsl:38-56` | w=1/(1+lum) ファイアフライ抑制 ✓ |
| TAA 分散クリッピング | `TAA.hlsl:60-85` | Salvi 2016 / Karis 2014 方式 ✓ |
| Motion Blur リプロジェクション | `MotionBlur.hlsl:33-44` | UV→world→prevClip→prevUV ✓ |
| Transform3D SRT 合成 | `Transform3D.cpp:14` | S*R*T（行ベクトル規約） ✓ |
| Skeleton 骨行列 | `Skeleton.cpp:35` | invBind*global（行ベクトル規約で正しい） ✓ |
| CSM フラスタム計算 | `CascadedShadowMap.cpp:60-82` | tanHalfFov → ビュー空間コーナー ✓ |
| Collision3D SAT (OBB) | `Collision3D.cpp:91-148` | 15軸テスト (Ericson 準拠) ✓ |
| Moller-Trumbore | `Collision3D.cpp:240-269` | レイ-三角形交差 ✓ |
| 自動露出 | `AutoExposure.hlsl + .cpp` | log輝度→exp逆変換→指数平滑 ✓ |

---

## MATH-BUG: 数式バグ

### MATH-01: Quaternion::ToEuler() の符号誤り
- **File:** `GXLib/Math/Quaternion.h:88-101`
- **Severity:** Medium
- **Description:** `XMQuaternionRotationRollPitchYaw(pitch, yaw, roll)` は回転順 Z×Y×X（外的 XYZ）を適用する。この規約に対応するオイラー角抽出公式は：
  ```
  sinP = 2(wx + yz)   // 正しい
  ```
  しかし実装では：
  ```cpp
  float sinP = 2.0f * (w * x - y * z);  // 符号が逆
  ```
  同様に yaw（L95: `w*y + z*x` → 正しくは `w*y - z*x`）と roll（L99: `w*z + x*y` → 正しくは `w*z - x*y`）も符号が反転。`FromEuler() → ToEuler()` のラウンドトリップが不正確になる。
- **Impact:** Quaternion からオイラー角を取得する全てのコードパスに影響。デバッグ表示やエディタ用途で使用されている場合、誤った角度が表示される。

### MATH-02: PhysicsWorld2D — 角トルクに質量の逆数を使用（慣性モーメントの逆数が正しい）
- **File:** `GXLib/Physics/PhysicsWorld2D.cpp:65`
- **Severity:** Medium-High
- **Description:**
  ```cpp
  body->angularVelocity += body->m_torqueAccum * (body->InverseMass() * dt);
  ```
  正しい物理式は `角加速度 = トルク / 慣性モーメント(I)` であり、`トルク / 質量(m)` ではない。
  - 円形: `I = 0.5 × m × r²`
  - 矩形: `I = (1/12) × m × (w² + h²)`

  `1/mass` を使用すると、大きな物体の回転が速すぎ、小さな物体の回転が遅すぎる不正確な挙動となる。

### MATH-03: PhysicsWorld2D — AABB ブロードフェーズが回転を無視
- **File:** `GXLib/Physics/PhysicsWorld2D.cpp:91-96`
- **Severity:** Medium
- **Description:**
  ```cpp
  return {
      body.position - body.shape.halfExtents,
      body.position + body.shape.halfExtents
  };
  ```
  矩形ボディの AABB 計算で `body.rotation` を考慮していない。回転した矩形の AABB は、4隅を回転してから min/max を取る必要がある。回転したボディは正しくない衝突判定範囲を持ち、本来衝突すべきペアが見逃される。

### MATH-04: RTReflections ClosestHit — 法線変換に ObjectToWorld3x4 を直接使用
- **File:** `Shaders/RTReflections.hlsl:222`
- **Severity:** Low-Medium
- **Description:**
  ```hlsl
  float3 N = normalize(mul((float3x3)ObjectToWorld3x4(), normalObj));
  ```
  非一様スケーリング時、法線の正しい変換には逆転置行列 `WorldToObject3x4()` の転置（= `mul(normalObj, (float3x3)WorldToObject3x4())`）を使用する必要がある。現在の実装では、例えば `scale={2,1,1}` のインスタンスで法線が歪む。
  - 一様スケールのみの場合は問題なし。
  - **参考:** 4gamer 記事でも言及されている通り、DXR の反射はラスタライズ結果と視覚的一貫性を保つ必要があり、法線の不正確さは反射品質に直結する。

### MATH-05: DepthOfField — ガウスカーネル重みの正規化不正
- **File:** `Shaders/DepthOfField.hlsl:74-82`
- **Severity:** Low
- **Description:** ガウス重みの合計：
  ```
  0.1963 + 2×(0.1745 + 0.1217 + 0.0667 + 0.0287 + 0.0097 + 0.0026)
  = 0.1963 + 2×0.4039 = 0.1963 + 0.8078 = 1.0041
  ```
  重みが 1.0 より 0.41% 大きく、ぼかし適用時に画像が微妙に明るくなる。

---

## TECH-ISSUE: 手法上の問題

### TECH-01: RTReflections — ポイントライトに太陽シャドウを再利用
- **File:** `Shaders/RTReflections.hlsl:336`
- **Severity:** Medium
- **Description:**
  ```hlsl
  color += brdfP * pointLightColor * pointLightIntensity * NdotLp * atten * shadow;
  ```
  `shadow` 変数は太陽方向のシャドウレイ結果（L282-306）。ポイントライトの遮蔽は太陽の遮蔽とは無関係であり、ポイントライトから見通しが効く表面でも太陽が遮蔽されていればポイントライトも暗くなってしまう。
  - **正しい実装:** ポイントライト用に別途シャドウレイ（ヒットポイント→ポイントライト方向）を発射するか、`shadow = 1.0` でポイントライトに影なしとする。
  - **参考:** 4gamer 記事の Futuremark アプローチ「レイトレを極力行わない」方針では、シャドウレイ本数を最小限にする戦略が推奨されているが、異なる光源のシャドウ結果を共有することは物理的に不正確。

### TECH-02: PhysicsWorld2D — 衝突解決に角インパルスなし
- **File:** `GXLib/Physics/PhysicsWorld2D.cpp:161-217`
- **Severity:** Medium
- **Description:** `ResolveCollision()` は法線・摩擦インパルスを線形速度にのみ適用し、角速度への影響を計算していない。物理的には、接触点が重心から離れている場合、衝撃は `ω += (r × J) / I` の角運動量変化を生じるべき。現在の実装ではオフセンター衝突でもオブジェクトが回転しない。

### TECH-03: PhysicsWorld2D — 摩擦計算にインパルス適用前の相対速度を使用
- **File:** `GXLib/Physics/PhysicsWorld2D.cpp:206`
- **Severity:** Low
- **Description:** `relVel`（L182 で計算）は法線インパルス適用前の値。L197-198 でインパルス適用後の速度を使用すべき。実際には誤差は小さく、多くの簡易物理エンジンで同様の簡略化が行われている。

---

## QUALITY: 品質制限（バグではないが改善余地あり）

### QUAL-01: TAA — HDR ブレンド前のトーンマップ未実装
- **File:** `Shaders/TAA.hlsl`
- **Description:** 現在のブレンドはリニア HDR 空間で行われる。プロダクション TAA（Karis 2014）では `color/(1+luminance)` 変換後にブレンドし逆変換する手法が標準。高輝度ハイライトでゴースティングが発生する可能性。

### QUAL-02: CSM — 対数分割未使用
- **File:** `GXLib/Graphics/3D/CascadedShadowMap.cpp:44-48`
- **Description:** カスケード分割は手動比率 `{0.05, 0.15, 0.4, 1.0}` による線形分割。Practical Split Scheme (PSSM) の対数/線形ブレンド（`C_log^i * (1-λ) + C_lin^i * λ`）を使用すると、近距離の影解像度が向上する。

---

## DXR 手法の検証（参考記事との比較）

4gamer 記事（Futuremark GDC 2018）では「レイトレーシングを極力行わないこと」が最適解として提唱されており、反射取得のみに DXR を活用する戦略が紹介されている。GXLib の DXR 実装はこの方針に沿っており：

| 項目 | GXLib 実装 | 評価 |
|---|---|---|
| レイ使用目的 | 反射 + シャドウレイのみ | ✓ 記事方針に合致 |
| ラスタライズ主体 | PBR パスは従来ラスタライズ | ✓ 正しいアプローチ |
| BLAS ビルドフラグ | PREFER_FAST_TRACE | ✓ 静的ジオメトリに適切 |
| TLAS ビルドフラグ | PREFER_FAST_BUILD | ✓ 毎フレーム再構築に適切 |
| シャドウレイ最適化 | ACCEPT_FIRST_HIT + SKIP_CLOSEST_HIT | ✓ 標準的な高速化手法 |
| 解像度 | フル解像度 dispatch | △ 記事はフル HD で約200万本が限界と指摘。半解像度の検討余地あり |

---

## 修正優先順位（計算式・手法）

1. **MATH-02 (M-H):** PhysicsWorld2D 角トルク — 物理シミュレーション全体に影響
2. **TECH-01 (M):** ポイントライトシャドウ — レンダリング品質に直結
3. **MATH-03 (M):** AABB ブロードフェーズ回転無視 — 衝突検出の信頼性
4. **TECH-02 (M):** 衝突解決の角インパルス欠如 — 物理リアリズム
5. **MATH-01 (M):** Quaternion::ToEuler() 符号 — 使用箇所次第で影響
6. **MATH-04 (L-M):** RT 法線変換 — 非一様スケール時のみ顕在化
7. **MATH-05 (L):** DoF ガウス重み — 0.4% の輝度増加（ほぼ不可視）
8. **TECH-03 (L):** 摩擦の相対速度 — 一般的な簡略化
9. **QUAL-01〜02:** 品質改善（TAA トーンマップ、CSM 対数分割）
