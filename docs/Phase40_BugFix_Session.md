# Phase 38-40 サンプル修正セッション

Phase 38-40（ドキュメント改善・レンダリング高度化・ゲームプレイ機能）実装後のビルドエラー修正とランタイムバグ修正の記録。

---

## 1. ビルドエラー修正

### Tilemap.cpp — LoadTexture 引数型不一致
- `TextureManager::LoadTexture()` は `const std::wstring&` を要求するが `std::string` を渡していた
- `std::wstring wTexPath(texPath.begin(), texPath.end())` で変換を追加

### ScriptBindings.cpp — 存在しないAPI / 型不一致
- `Color::ToARGB()` は存在しない → `Color::ToRGBA()` に修正
- `LoadTexture` も同様に `std::wstring` 変換を追加

### TilemapShowcase/main.cpp — 多数のAPI名間違い
| 誤 | 正 |
|---|---|
| `ctx.primitiveBatch` | `ctx.primBatch` |
| `ctx.defaultFont` | `ctx.defaultFontHandle` |
| `GX::TString` / `GX::FormatT` | `TString` / `FormatT`（グローバル） |
| `app.Run()` | `GXEasy::Run(app, app.GetConfig())` |
| `TEXT("{}...")` + `std::format` | `std::format(L"...")` or `FormatT(TEXT(...))` |

### LuaShowcase/main.cpp — API名間違い
- `ctx.textureManager` → `ctx.renderer3D.GetTextureManager()`
- `ctx.defaultFont` → `ctx.defaultFontHandle`
- `app.Run()` → `GXEasy::Run(app, app.GetConfig())`

### MultiThreadShowcase/main.cpp — API名間違い + 未定義メソッド
- `DrawModel(GPUMesh, Transform3D, Material)` は存在しない → `SetMaterial()` + `DrawMesh()` に分離
- `Setup3D()` メソッド定義が欠落 → PostEffect/Camera/Light初期化を含む実装を追加
- `app.Run()` → `GXEasy::Run(app, app.GetConfig())`

---

## 2. ランタイムクラッシュ修正

### TilemapShowcase — Access violation in PrimitiveBatch::DrawBox (0xc0000005)
- **原因**: `PrimitiveBatch` が Begin されていない状態で `DrawBox` を呼び出した
- **修正**: `ctx.EnsurePrimitiveBatch()` をタイル描画ループ前に追加、`ctx.EnsureSpriteBatch()` をテキスト描画前に追加

### LuaShowcase — Access violation in SpriteBatch::AddQuad (0xc0000005)
- **原因**: `SpriteBatch` が Begin されていない状態で `TextRenderer::DrawString` を呼び出した
- **修正**: `ctx.EnsureSpriteBatch()` を描画前に追加（後に DxLib 互換関数に全面移行）

### MultiThreadShowcase — Access violation in Renderer3D::SetMaterial (0xc0000005)
- **原因**: 3Dレンダリングパイプラインが未初期化（`renderer3D.Begin()` 未呼出）
- **修正**: `FlushAll` → `BeginScene` → `renderer3D.Begin` → 描画 → `End` → `EndScene` → depth遷移 → `Resolve` の正規パイプラインを実装

### MultiThreadShowcase — D3D12 例外 (0x00087a) in SetMaterial
- **原因**: `k_MaxObjectsPerFrame = 512` に対し 1001 オブジェクト（1000キューブ＋1床）を描画し、マテリアル定数バッファがオーバーフロー
- **修正**: `k_ObjectCount` を 1000 → 500 に削減（501 < 512）

### MultiThreadShowcase — DrawString 型不一致 (C2664)
- **原因**: `std::wstring::c_str()` (wchar_t*) を DxLib 互換 `DrawString` (TCHAR* = char*) に渡した
- **修正**: `FormatT(TEXT(...))` パターンに変更

---

## 3. 入力が効かない問題

### TilemapShowcase — 矢印キー / Z/X キーが反応しない
- **原因**: `Keyboard::IsKeyDown()` は **Win32 VK コード** を受け取るが、**DirectInput スキャンコード** (0xC8=DIK_UP 等) を渡していた。DIK→VK 変換は DxLib 互換 `CheckHitKey()` 内でのみ行われる
- **修正箇所**:

| ファイル | 誤（DIKコード） | 正（VKコード） |
|---|---|---|
| TilemapShowcase | `0xC8, 0xD0, 0xCB, 0xCD` | `VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT` |
| TilemapShowcase | `0x2C, 0x2D` | `'Z', 'X'` |
| MultiThreadShowcase | `0x02` | `'1'` |
| ScriptBindings.cpp | `KEY_UP=0xC8` 等 | `KEY_UP=VK_UP` 等 |

---

## 4. サンプル内容が伝わらない問題（全面書き直し）

### LuaShowcase — 何も描画されない
- **問題**: Lua の `OnDraw` がコメントだけで何も描画せず、プレイヤー座標も取得・可視化されていなかった
- **修正**:
  - `ScriptEngine` に `GetGlobalFloat()` / `GetGlobalInt()` を新規追加（PIMPL越しにLuaグローバル変数を読み取る）
  - Lua側: `OnUpdate` で座標をグローバル変数 `_px`/`_py`/`_ps` にエクスポート
  - C++側: `GetGlobalFloat("_px")` で座標取得 → 青い矩形＋十字マーク＋ラベルを描画
  - 背景グリッド + HUD（Lua状態・座標表示）追加
  - 全描画を DxLib 互換関数（`DrawBox`/`DrawLine`/`DrawString`）で統一

### MultiThreadShowcase — 3D描画パイプライン欠落 + カメラ操作なし
- **問題**: `renderer3D.Begin()` が呼ばれず何も描画されない。ヘッダに書かれた WASD/マウス操作のコードが存在しない
- **修正**:
  - Walkthrough3D と同じ 3D 描画パイプライン（BeginScene→Begin→描画→End→EndScene→Resolve）を実装
  - WASD/QE カメラ移動 + 右クリックマウスルック（Walkthrough3D パターン）を実装
  - 500個の虹色キューブが波打つアニメーション
  - FPS 表示 + 操作ガイド HUD

---

## 5. 変更ファイル一覧

| ファイル | 変更種別 |
|---|---|
| `GXLib/Graphics/Rendering/Tilemap.cpp` | wstring変換追加 |
| `GXLib/Script/ScriptBindings.cpp` | ToARGB→ToRGBA, wstring変換, VKコード修正 |
| `GXLib/Script/ScriptEngine.h` | `GetGlobalFloat`/`GetGlobalInt` 追加 |
| `GXLib/Script/ScriptEngine.cpp` | 上記の実装 + スタブ |
| `Samples/TilemapShowcase/main.cpp` | API名修正, EnsureBatch追加, VKコード修正 |
| `Samples/LuaShowcase/main.cpp` | 全面書き直し（可視化・入力対応） |
| `Samples/MultiThreadShowcase/main.cpp` | 全面書き直し（3Dパイプライン・カメラ操作） |

---

## 6. 学んだ教訓

1. **`Keyboard::IsKeyDown()` は VK コード**、DxLib 互換 `CheckHitKey()` は DIK コード — 直接 `Keyboard` を使う場合は VK を渡すこと
2. **3Dサンプルは必ず** `FlushAll` → `BeginScene` → `renderer3D.Begin` → 描画 → `End` → `EndScene` → depth遷移 → `Resolve` のフルパイプラインが必要
3. **2D描画前に** `EnsureSpriteBatch()` / `EnsurePrimitiveBatch()` が必須（DxLib 互換関数は内部で呼ぶので不要）
4. **`k_MaxObjectsPerFrame = 512`** — DrawMesh/SetMaterial の呼び出し回数は 512 以内に収めること
5. **DxLib 互換 `DrawString`** は `TCHAR*`（_MBCS=char*）→ `std::wstring` は渡せない。`FormatT(TEXT(...))` を使う
6. **サンプルは DxLib 互換関数を使うのが安全** — `DrawBox`/`DrawLine`/`DrawString` は内部で Ensure を呼ぶのでクラッシュしにくい
