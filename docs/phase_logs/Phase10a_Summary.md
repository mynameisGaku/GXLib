# Phase 10a Summary: GPU Timestamp Profiler + Optimization Utilities

## Overview
D3D12 Query Heap (TIMESTAMP) を使用したGPUタイムスタンプ プロファイラ、フレーム単位リニアアロケータ、
固定サイズプールアロケータ、バッチバリア発行ユーティリティを実装。
ダブルバッファリングリードバックによりGPUストールを回避しつつ、フレーム毎のGPU負荷を
スコープ単位で計測可能。P キーでHUDオーバーレイをトグル表示。

## Completion Condition
> GPUプロファイラでボトルネック計測が可能 → **達成**

---

## Sub-phases

| Sub-phase | Content |
|-----------|---------|
| 10a-1 | GPUProfiler (D3D12 Timestamp Query + HUD overlay) |
| 10a-2 | FrameAllocator + PoolAllocator (メモリ最適化ユーティリティ) |
| 10a-3 | BarrierBatch (バッチバリア発行ユーティリティ) |

## New Files

| File | Description |
|------|-------------|
| `GXLib/Graphics/Device/GPUProfiler.h` | GPUプロファイラ ヘッダー (シングルトン、RAII GPUProfileScope、マクロ) |
| `GXLib/Graphics/Device/GPUProfiler.cpp` | GPUプロファイラ実装 (Query Heap作成、リードバック、スコープ計測) |
| `GXLib/Core/FrameAllocator.h` | フレーム単位リニアアロケータ (ヘッダーオンリー) |
| `GXLib/Core/PoolAllocator.h` | 固定サイズプールアロケータ (テンプレート、ヘッダーオンリー) |
| `GXLib/Graphics/Device/BarrierBatch.h` | バッチバリア発行ユーティリティ (テンプレート、ヘッダーオンリー) |

## Modified Files

| File | Changes |
|------|---------|
| `Sandbox/main.cpp` | P キーで GPUProfiler トグル、HUDオーバーレイ描画、BeginFrame/EndFrame 統合 |

## Architecture

### GPUProfiler
```
D3D12 Query Heap (TIMESTAMP, k_MaxTimestamps=256)
  ├── BeginFrame()  → 前フレーム結果リードバック + フレーム開始タイムスタンプ
  ├── BeginScope()  → スコープ開始タイムスタンプ (EndQuery)
  ├── EndScope()    → スコープ終了タイムスタンプ (EndQuery)
  └── EndFrame()    → フレーム終了タイムスタンプ + ResolveQueryData → Readback Buffer

Readback Buffer[0] ←→ Readback Buffer[1]  (ダブルバッファリング)
  Frame N: 書き込み → Frame N+2: CPU読み取り (GPUストール回避)
```

- **シングルトンパターン**: `GPUProfiler::Instance()` で唯一のインスタンスにアクセス
- **ダブルバッファリング**: `k_BufferCount=2` のリードバックバッファで、GPU が書き込み中のバッファとは別のバッファから CPU が読み取る
- **タイムスタンプ周波数**: `ID3D12CommandQueue::GetTimestampFrequency()` で取得、ticks → ms 変換
- **スコープ計測**: `BeginScope`/`EndScope` のペアで任意区間を計測。後方検索で最後の未終了スコープを閉じる（ネスト対応）
- **RAII ヘルパー**: `GPUProfileScope` がコンストラクタで `BeginScope`、デストラクタで `EndScope` を呼ぶ
- **マクロ**: `GX_GPU_PROFILE_SCOPE(cmdList, name)` で簡潔にスコープ計測
- **HUD オーバーレイ**: P キーでトグル（デフォルト OFF）、フレームGPU時間 + 各スコープの ms を表示
- **オーバーフロー保護**: `k_MaxTimestamps=256`（128スコープ分）超過時は最後のスロットを再利用

### FrameAllocator
```
[                    1MB Buffer (default)                    ]
 ↑ m_offset=0 (Reset)
 ├── Allocate(64B) → [####]  offset=64
 ├── Allocate(128B, align=16) → [pad][########]  offset=208
 └── ... O(1) bump allocation ...
 Frame End → Reset() → offset=0 (一括解放)
```

- **リニア（バンプ）アロケータ**: ポインタを進めるだけの O(1) 確保
- **実アドレスベースアラインメント**: `(base + offset + alignment - 1) & ~(alignment - 1)` で正確なアラインメント
- **フレーム開始時 Reset()**: 個別 Free() 不要、ダングリングポインタのリスクなし
- **テンプレート Allocate\<T\>()**: `alignof(T)` を自動適用する型安全バージョン
- **キャッシュフレンドリー**: 連続メモリ配置により局所性を確保
- **デフォルト 1MB**: ソートキー配列、一時バッファ等のフレーム内一時データに適用

### PoolAllocator
```
Block 0: [Node0|Node1|Node2|...|Node63]  (BlockSize=64)
Block 1: [Node0|Node1|...|Node63]         (オンデマンド拡張)
           ↓ FreeList: Node→Node→Node→nullptr
```

- **固定サイズオブジェクトプール**: Widget, Sound, Particle 等の大量生成・破棄向け
- **フリーリスト方式**: 未使用スロットをリンクドリストで管理、Allocate/Free ともに O(1)
- **Union ノード**: `Node` は `next` ポインタと `storage[sizeof(T)]` の共用体（メモリ節約）
- **オンデマンドブロック拡張**: フリーリスト枯渇時に `BlockSize` 個分の新ブロックを確保
- **New/Delete ヘルパー**: placement new + デストラクタ呼び出し付きの型安全インターフェース
- **ヒープ断片化防止**: 同サイズオブジェクトの確保・解放を局所化

### BarrierBatch
```
BarrierBatch<16> batch(cmdList);
  batch.Transition(rt, RENDER_TARGET, PIXEL_SHADER_RESOURCE);
  batch.Transition(ds, DEPTH_WRITE, DEPTH_READ);
  batch.UAV(uavBuffer);
  // デストラクタで自動 Flush → 1回の ResourceBarrier(3, barriers)
```

- **テンプレートバッチ**: `BarrierBatch<N=16>` でスタック配列サイズを指定
- **自動フラッシュ**: 容量超過時およびデストラクタで自動 `ResourceBarrier()` 呼び出し
- **Transition + UAV**: 2種類のバリアをサポート、`before == after` はスキップ
- **GPU パイプライン効率化**: 複数バリアを1回の API 呼び出しにまとめることで同期コスト削減

## Key Design Decisions
- **ダブルバッファリングリードバック**: GPU が ResolveQueryData で書き込み中のバッファを CPU が読まないよう、2フレーム遅延で結果を取得。WaitForValue 完了後なのでデータの整合性は保証される
- **AllocTimestamp() のオーバーフロー対策**: 256 クエリを超えた場合は最後のスロットを再利用し、クラッシュを防止
- **EndScope の後方検索**: スコープのネスト（LIFO順序）に対応するため、最後の未終了エントリを逆順で検索
- **FrameAllocator をヘッダーオンリーに**: インライン化による最大のパフォーマンスを確保（Allocate は数命令のバンプ操作のみ）
- **PoolAllocator の Union ノード**: 未使用スロットでは next ポインタ、使用中はオブジェクトデータとして同じメモリ領域を共用
- **BarrierBatch のデストラクタ自動フラッシュ**: スコープ終了時に確実にバリアが発行され、書き忘れを防止
- **P キーでトグル（デフォルト OFF）**: リリースビルドでもオーバーヘッドなし（m_enabled チェックで即 return）

## Issues Encountered
- **FrameAllocator アラインメント不具合**: 初期実装ではオフセットベースのアラインメント `(m_offset + alignment - 1) & ~(alignment - 1)` を使用していたが、バッファの先頭アドレスがアラインメント境界にない場合に正しくアラインされない問題が発生。実アドレスベース `(base + m_offset + alignment - 1) & ~(alignment - 1)` に修正して解決

## Verification
- Build: OK
- GPUProfiler: P キーで HUD オーバーレイ表示、スコープ毎の GPU 時間（ms）を確認
- FrameAllocator: アラインメント + 容量超過テスト通過
- PoolAllocator: Allocate/Free/New/Delete サイクルテスト通過
- BarrierBatch: バッチ発行 + 自動フラッシュ動作確認
- Tests: 151/151 pass
