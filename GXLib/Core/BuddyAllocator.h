#pragma once
/// @file BuddyAllocator.h
/// @brief バディアロケータ（2のべき乗アロケーション）
///
/// GPU ヒープやテクスチャアトラスなど、2のべき乗サイズのブロックを
/// 効率的に管理するアロケータ。再帰的な分割と合体 (buddy merging) により、
/// 外部フラグメンテーションを最小化する。
/// @addtogroup grp_core/// @{

#include "pch_common.h"
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace gx
{

/// @brief バディアロケータ
class BuddyAllocator
{
public:
    BuddyAllocator() = default;
    ~BuddyAllocator() = default;

    /// @brief 初期化
    /// @param totalSize 合計サイズ（2のべき乗）
    /// @param minBlockSize 最小ブロックサイズ（2のべき乗）
    /// @return 成功なら true
    bool Initialize(size_t totalSize, size_t minBlockSize = 16);

    /// @brief メモリ確保
    /// @param size 確保するバイト数
    /// @return オフセット（失敗時はUINT64_MAX）
    size_t Allocate(size_t size);

    /// @brief メモリ解放
    /// @param offset 解放するブロックのオフセット
    void Free(size_t offset);

    /// @brief 合計サイズ
    /// @return プールの総バイト数
    size_t GetTotalSize() const { return m_totalSize; }

    /// @brief 使用中サイズ
    /// @return 使用中のバイト数
    size_t GetUsedSize() const { return m_usedSize; }

    /// @brief 空きサイズ
    /// @return 空きバイト数
    size_t GetFreeSize() const { return m_totalSize - m_usedSize; }

    /// @brief アロケーション数
    /// @return 現在のアロケーション数
    uint32_t GetAllocationCount() const { return m_allocationCount; }

    /// @brief 最大オーダー
    /// @return 最大のバディオーダー
    uint32_t GetMaxOrder() const { return m_maxOrder; }

    /// @brief 最小ブロックサイズ
    /// @return 最小ブロックのバイト数
    size_t GetMinBlockSize() const { return m_minBlockSize; }

    /// @brief リセット
    void Reset();

private:
    size_t RoundUpToPow2(size_t size) const;
    uint32_t SizeToOrder(size_t size) const;
    size_t OrderToSize(uint32_t order) const;
    size_t GetBuddyOffset(size_t offset, uint32_t order) const;

    size_t m_totalSize = 0;            ///< プールの総サイズ
    size_t m_minBlockSize = 16;        ///< 最小ブロックサイズ
    uint32_t m_maxOrder = 0;           ///< 最大バディオーダー
    size_t m_usedSize = 0;             ///< 使用中の合計サイズ
    uint32_t m_allocationCount = 0;    ///< 現在のアロケーション数

    std::vector<std::unordered_set<size_t>> m_freeLists;   ///< オーダー別フリーリスト
    std::unordered_map<size_t, uint32_t> m_allocated;      ///< 確保済みブロック（オフセット→オーダー）
};

} // namespace gx
/// @}
