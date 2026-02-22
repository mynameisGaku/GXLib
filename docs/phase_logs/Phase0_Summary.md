# GXLib Phase 0: D3D12基盤構築 — 作業レポート

## 概要

DXライブラリの完全上位互換フレームワーク「GXLib」のPhase 0として、
DirectX 12ベースの基盤を構築し、**色付き三角形（Hello Triangle）の描画**に成功した。

---

## 技術仕様

| 項目 | 内容 |
|------|------|
| 言語 | C++20 |
| ビルドシステム | CMake（VS2022 v145ツールセット） |
| シェーダーコンパイラ | Windows SDK DXC (dxcompiler.dll) |
| 名前空間 | `GX::` |
| 命名規則 | クラス/メソッド: PascalCase, メンバ: m_camelCase |
| PCH | pch.h（Windows + DX12 + STL + DirectXMath） |
| コンパイルフラグ | `/utf-8`（日本語コメント対応） |

---

## 作成ファイル一覧（30ファイル）

### ビルド構成（5ファイル）

| ファイル | 役割 |
|----------|------|
| `CMakeLists.txt` | ルートCMake（C++20、/utf-8、サブプロジェクト統合） |
| `GXLib/CMakeLists.txt` | エンジンライブラリ（STATIC、PCH、DX12リンク） |
| `Sandbox/CMakeLists.txt` | テストアプリ（WIN32 EXE、シェーダーコピー） |
| `GXLib/pch.h` | プリコンパイルドヘッダー |
| `GXLib/pch.cpp` | PCH生成用ソース |

### Core（8ファイル）

| クラス | ファイル | 役割 |
|--------|----------|------|
| `Logger` | Logger.h/cpp | ログレベル別出力（Info/Warn/Error）、OutputDebugString + printf |
| `Timer` | Timer.h/cpp | QueryPerformanceCounterベース高精度タイマー、デルタタイム・FPS計算 |
| `Window` | Window.h/cpp | Win32ウィンドウ作成、WndProc、メッセージループ、リサイズコールバック |
| `Application` | Application.h/cpp | Initialize → Run → Shutdown ライフサイクル、FPSタイトル表示 |

### Graphics/Device（12ファイル）

| クラス | ファイル | 役割 |
|--------|----------|------|
| `GraphicsDevice` | GraphicsDevice.h/cpp | DXGIFactory → Adapter列挙 → D3D12Device作成、デバッグレイヤー |
| `Fence` | Fence.h/cpp | ID3D12Fence + Event、Signal/WaitForValue/WaitForGPU |
| `CommandQueue` | CommandQueue.h/cpp | ID3D12CommandQueue、ExecuteCommandLists、Flush |
| `DescriptorHeap` | DescriptorHeap.h/cpp | RTV/DSV/CBV_SRV_UAVヒープ管理、Allocate/GetHandle |
| `SwapChain` | SwapChain.h/cpp | IDXGISwapChain4、ダブルバッファリング、Present、Resize |
| `CommandList` | CommandList.h/cpp | ID3D12GraphicsCommandList + Allocator×2、Reset/Close |

### Graphics/Pipeline（6ファイル）

| クラス | ファイル | 役割 |
|--------|----------|------|
| `Shader` | Shader.h/cpp | DXC (IDxcCompiler3) によるHLSLコンパイル（VS/PS） |
| `RootSignatureBuilder` | RootSignature.h/cpp | ビルダーパターンでルートシグネチャ構築 |
| `PipelineStateBuilder` | PipelineState.h/cpp | ビルダーパターンでGraphics PSO構築 |

### Graphics/Resource（2ファイル）

| クラス | ファイル | 役割 |
|--------|----------|------|
| `Buffer` | Buffer.h/cpp | 頂点/インデックスバッファ作成（UPLOADヒープ） |

### Shaders（1ファイル）

| ファイル | 役割 |
|----------|------|
| `Shaders/HelloTriangle.hlsl` | 頂点シェーダー（VSMain）+ ピクセルシェーダー（PSMain） |

### Sandbox（1ファイル）

| ファイル | 役割 |
|----------|------|
| `Sandbox/main.cpp` | WinMain、全システム初期化、三角形描画ループ |

---

## ディレクトリ構成

```
GXLib/
├── CMakeLists.txt
├── GXLib/
│   ├── CMakeLists.txt
│   ├── pch.h
│   ├── pch.cpp
│   ├── Core/
│   │   ├── Application.h / .cpp
│   │   ├── Logger.h / .cpp
│   │   ├── Timer.h / .cpp
│   │   └── Window.h / .cpp
│   └── Graphics/
│       ├── Device/
│       │   ├── GraphicsDevice.h / .cpp
│       │   ├── Fence.h / .cpp
│       │   ├── CommandQueue.h / .cpp
│       │   ├── CommandList.h / .cpp
│       │   ├── DescriptorHeap.h / .cpp
│       │   └── SwapChain.h / .cpp
│       ├── Pipeline/
│       │   ├── Shader.h / .cpp
│       │   ├── RootSignature.h / .cpp
│       │   └── PipelineState.h / .cpp
│       └── Resource/
│           └── Buffer.h / .cpp
├── Sandbox/
│   ├── CMakeLists.txt
│   └── main.cpp
└── Shaders/
    └── HelloTriangle.hlsl
```

---

## 描画パイプライン

Sandbox/main.cppでの描画フロー：

```
1. Application初期化 → Window作成（1280x720）
2. GraphicsDevice初期化（デバッグレイヤー有効）
3. CommandQueue + CommandList作成
4. SwapChain作成（ダブルバッファリング、FLIP_DISCARD）
5. DXCでシェーダーコンパイル（HelloTriangle.hlsl）
6. RootSignature作成（パラメータなし、IA入力のみ）
7. PipelineState作成（VS + PS + InputLayout + CullNone + DepthOff）
8. 頂点バッファ作成（3頂点: 赤・青・緑）

メインループ:
  Timer.Tick() → ProcessMessages()
  → CommandList.Reset()
  → ResourceBarrier(PRESENT → RENDER_TARGET)
  → ClearRenderTargetView(ダークブルー)
  → SetViewport/Scissor → SetRootSignature → SetPSO
  → IASetVertexBuffers → DrawInstanced(3, 1)
  → ResourceBarrier(RENDER_TARGET → PRESENT)
  → CommandList.Close() → ExecuteCommandLists()
  → SwapChain.Present() → Fence.Signal()
```

---

## ビルド方法

```bash
# CMake構成
cmake -B build -G "Visual Studio 17 2022" -A x64

# ビルド（Debug）
cmake --build build --config Debug

# 実行
build\Sandbox\Debug\Sandbox.exe
```

---

## 実行結果

- ウィンドウが画面中央に1280x720で表示される
- ダークブルーの背景に赤・青・緑のグラデーション三角形が描画される
- タイトルバーにFPSが1秒ごとに更新表示される
- ESCキーで終了
- ウィンドウリサイズ時にSwapChainが自動リサイズ

---

## コメントスタイル

全クラスに初学者向け日本語コメントを付与：

```cpp
/// @brief GPU同期用フェンスクラス
///
/// 【初学者向け解説】
/// CPUとGPUは非同期で動作します。CPUがGPUに描画コマンドを送っても、
/// GPUがすぐに処理を完了するとは限りません。
///
/// フェンスは「GPUの作業完了を待つ」ための仕組みです。
/// イメージとしては、レストランで注文した料理が「できたよ！」と
/// 知らせてくれるベルのようなものです。
```

---

## Phase 0 完了チェックリスト

- [x] CMakeプロジェクト構成
- [x] Win32ウィンドウ作成・メッセージループ
- [x] D3D12デバイス初期化（Factory, Device, CommandQueue）
- [x] SwapChain作成（ダブルバッファリング）
- [x] DescriptorHeap管理クラス
- [x] FenceによるCPU-GPU同期
- [x] CommandAllocator / CommandList管理
- [x] パイプラインステート基盤（RootSignatureビルダー、PSOビルダー）
- [x] DXCによるシェーダーコンパイル
- [x] 三角形描画（Hello Triangle）
- [x] フレームタイミング・FPS制御
- [x] ログシステム
- [x] 全クラスに初学者向け日本語コメント
- [x] Debugビルド成功（エラー・警告なし）
