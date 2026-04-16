#pragma once
/// @file TLSFAllocator.h
/// @brief TLSF (Two-Level Segregated Fit) アロケータ
///
/// O(1) の確保・解放を提供する汎用メモリアロケータ。
/// GPU メモリやオブジェクトプールの内部管理に適し、
/// 隣接ブロックの自動合体 (coalescing) によるフラグメンテーション低減と
/// デフラグ計画の算出にも対応する。
/// @addtogroup grp_core/// @{

#include "pch_common.h"
#include <cstdint>
#include <vector>
#include <unordered_map>

namespace gx
{

/// @brief TLSFブロックヘッダー
struct TLSFBlock
{
    size_t size = 0;              ///< ブロックサイズ（ヘッダー含まない）
    bool   free = true;           ///< 空きブロックかどうか
    size_t offset = 0;            ///< プール内オフセット
    TLSFBlock* prev = nullptr;    ///< 物理的前方ブロック
    TLSFBlock* next = nullptr;    ///< 物理的次方ブロック
    TLSFBlock* prevFree = nullptr; ///< フリーリスト前
    TLSFBlock* nextFree = nullptr; ///< フリーリスト次
};

/// @brief デフラグ移動計画
struct DefragMove
{
    size_t srcOffset = 0;  ///< 移動元オフセット
    size_t dstOffset = 0;  ///< 移動先オフセット
    size_t size = 0;       ///< 移動するバイト数
};

/// @brief TLSFアロケータ
class TLSFAllocator
{
public:
    TLSFAllocator() = default;
    ~TLSFAllocator();

    /// @brief 初期化
    /// @param poolSize メモリプールサイズ
    /// @param minBlockSize 最小ブロックサイズ（2のべき乗）
    /// @return 成功なら true
    bool Initialize(size_t poolSize, size_t minBlockSize = 16);

    /// @brief メモリ確保 O(1)
    /// @param size 確保するバイト数
    /// @return オフセット (失敗時はUINT64_MAX)
    size_t Allocate(size_t size);

    /// @brief メモリ解放 O(1)
    /// @param offset 解放するブロックのオフセット
    void Free(size_t offset);

    /// @brief プールサイズ
    /// @return プールの総バイト数
    size_t GetPoolSize() const { return m_poolSize; }

    /// @brief 使用中サイズ
    /// @return 使用中のバイト数
    size_t GetUsedSize() const { return m_usedSize; }

    /// @brief 空きサイズ
    /// @return 空きバイト数
    size_t GetFreeSize() const { return m_poolSize - m_usedSize; }

    /// @brief アロケーション数
    /// @return 現在のアロケーション数
    uint32_t GetAllocationCount() const { return m_allocationCount; }

    /// @brief 最大ブロックサイズ（フラグメンテーション指標）
    /// @return 最大の空きブロックサイズ
    size_t GetLargestFreeBlock() const;

    /// @brief デフラグ計画を計算
    /// @return 移動計画の配列
    gx::Vector<DefragMove> ComputeDefragPlan() const;

    /// @brief リセット（全解放）
    void Reset();

private:
    void MappingInsert(size_t size, uint32_t& fl, uint32_t& sl) const;
    void MappingSearch(size_t size, uint32_t& fl, uint32_t& sl) const;
    TLSFBlock* FindFreeBlock(uint32_t fl, uint32_t sl);
    void InsertFreeBlock(TLSFBlock* block);
    void RemoveFreeBlock(TLSFBlock* block);
    TLSFBlock* SplitBlock(TLSFBlock* block, size_t size);

    static constexpr uint32_t FL_INDEX_COUNT = 32;
    static constexpr uint32_t SL_INDEX_LOG2  = 4;
    static constexpr uint32_t SL_INDEX_COUNT = 16;  // 2^4 sub-divisions per FL

    size_t m_poolSize = 0;               ///< プールの総サイズ
    size_t m_minBlockSize = 16;          ///< 最小ブロックサイズ
    size_t m_usedSize = 0;               ///< 使用中の合計サイズ
    uint32_t m_allocationCount = 0;      ///< 現在のアロケーション数

    uint32_t m_flBitmap = 0;                                     ///< 第1レベルビットマップ
    uint32_t m_slBitmap[FL_INDEX_COUNT] = {};                    ///< 第2レベルビットマップ
    TLSFBlock* m_freeList[FL_INDEX_COUNT][SL_INDEX_COUNT] = {};  ///< 2次元フリーリスト

    gx::Vector<std::unique_ptr<TLSFBlock>> m_blockStorage;      ///< ブロックの所有権管理
    gx::HashMap<size_t, TLSFBlock*> m_offsetToBlock;      ///< オフセット→ブロックのマップ
};

} // namespace gx
/// @}
