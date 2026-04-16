#pragma once
/// @file MemoryProfiler.h
/// @brief メモリプロファイラ — アロケーション追跡・カテゴリ別使用量・リーク検出
///
/// RecordAllocation/RecordFree でメモリ確保・解放を手動記録し、
/// カテゴリ別のメモリ使用量やピーク使用量を追跡する。
/// GetLeakedAllocations() で未解放アロケーションを検出可能。
/// シングルトンではなく通常のクラス。
/// @addtogroup grp_core/// @{

#include "pch_common.h"
#include <unordered_map>

namespace gx
{

/// @brief 個別アロケーション記録
struct AllocationRecord
{
    void*       address     = nullptr; ///< アロケーションアドレス
    size_t      size        = 0;       ///< サイズ（バイト）
    gx::String category;              ///< カテゴリ名
    uint32_t    frameNumber = 0;       ///< 記録時のフレーム番号
};

/// @brief メモリスナップショット（ある時点のメモリ状態）
struct MemorySnapshot
{
    uint64_t totalAllocated   = 0;  ///< 総確保量（累計）
    uint64_t totalFreed       = 0;  ///< 総解放量（累計）
    uint64_t currentUsage     = 0;  ///< 現在の使用量
    uint64_t peakUsage        = 0;  ///< ピーク使用量
    uint32_t allocationCount  = 0;  ///< 総確保回数
    uint32_t freeCount        = 0;  ///< 総解放回数
    uint32_t frameNumber      = 0;  ///< フレーム番号
    gx::HashMap<gx::String, uint64_t> categoryUsage; ///< カテゴリ別使用量
};

/// @brief メモリプロファイラ
class MemoryProfiler
{
public:
    /// @brief 初期化（プロファイリングを有効化する）
    void Initialize();

    /// @brief シャットダウン（プロファイリングを無効化し内部データをクリアする）
    void Shutdown();

    // ----- 記録 -----

    /// @brief アロケーションを記録する
    void RecordAllocation(void* ptr, size_t size, const gx::String& category = "Default");

    /// @brief 解放を記録する
    void RecordFree(void* ptr);

    // ----- フレーム管理 -----

    /// @brief フレーム開始を通知する
    void BeginFrame(uint32_t frameNumber);

    /// @brief フレーム終了を通知する
    void EndFrame();

    // ----- スナップショット取得 -----

    /// @brief 現在のメモリ状態のスナップショットを返す
    /// @return 現在のスナップショット
    MemorySnapshot GetCurrentSnapshot() const;

    /// @brief ピーク時のスナップショットを返す
    /// @return ピーク使用量時のスナップショット
    MemorySnapshot GetPeakSnapshot() const;

    // ----- カテゴリ別情報 -----

    /// @brief 指定カテゴリの現在のメモリ使用量を返す
    /// @param category カテゴリ名
    /// @return 使用量（バイト）
    uint64_t GetCategoryUsage(const gx::String& category) const;

    /// @brief 使用中の全カテゴリ名を返す
    /// @return カテゴリ名の配列
    gx::Vector<gx::String> GetCategories() const;

    // ----- リーク検出 -----

    /// @brief 未解放のアロケーション一覧を返す
    /// @return 未解放のアロケーション記録の配列
    gx::Vector<AllocationRecord> GetLeakedAllocations() const;

    /// @brief メモリリークがあるかどうか
    /// @return リークがあればtrue
    bool HasLeaks() const;

    // ----- 統計 -----

    /// @brief 総確保量（累計バイト数）
    /// @return 総確保バイト数
    uint64_t GetTotalAllocated() const;

    /// @brief 総解放量（累計バイト数）
    /// @return 総解放バイト数
    uint64_t GetTotalFreed() const;

    /// @brief 現在のメモリ使用量
    /// @return 使用量（バイト）
    uint64_t GetCurrentUsage() const;

    /// @brief ピークメモリ使用量
    /// @return ピーク使用量（バイト）
    uint64_t GetPeakUsage() const;

    /// @brief 総アロケーション回数
    /// @return アロケーション回数
    uint32_t GetAllocationCount() const;

    // ----- リセット -----

    /// @brief 全データをリセットする
    void Reset();

    // ----- 有効/無効 -----

    /// @brief プロファイリングの有効/無効を設定する
    void SetEnabled(bool enabled);

    /// @brief プロファイリングが有効かどうか
    /// @return 有効ならtrue
    bool IsEnabled() const;

private:
    gx::HashMap<void*, AllocationRecord> m_activeAllocations; ///< アクティブアロケーション
    MemorySnapshot m_current;  ///< 現在のスナップショット
    MemorySnapshot m_peak;     ///< ピーク時のスナップショット
    bool     m_enabled     = false; ///< 有効フラグ
    uint32_t m_frameNumber = 0;     ///< 現在のフレーム番号
};

} // namespace gx
/// @}
