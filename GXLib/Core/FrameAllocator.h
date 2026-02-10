#pragma once
/// @file FrameAllocator.h
/// @brief フレーム単位リニアアロケータ
///
/// 1フレーム内でのみ有効な一時データ（ソートキー配列、一時バッファ等）を
/// 高速に確保するためのリニア（バンプ）アロケータ。
///
/// フレーム開始時に Reset() を呼ぶと、確保済みメモリが全て解放される。
/// 個別の Free() は不要（フレーム終了で一括解放）。
///
/// 利点:
/// - 確保が O(1)（ポインタを進めるだけ）
/// - キャッシュフレンドリー（連続メモリ配置）
/// - 個別解放不要（ダングリングポインタのリスクなし）

#include "pch.h"

namespace GX
{

/// @brief フレーム単位リニアアロケータ
class FrameAllocator
{
public:
    /// @brief 指定サイズのバッファを持つアロケータを作成する
    /// @param capacityBytes 最大確保可能バイト数
    explicit FrameAllocator(size_t capacityBytes = 1024 * 1024)  // デフォルト1MB
        : m_capacity(capacityBytes), m_offset(0)
    {
        m_buffer = std::make_unique<uint8_t[]>(capacityBytes);
    }

    ~FrameAllocator() = default;

    // コピー禁止
    FrameAllocator(const FrameAllocator&) = delete;
    FrameAllocator& operator=(const FrameAllocator&) = delete;

    // ムーブ許可
    FrameAllocator(FrameAllocator&&) = default;
    FrameAllocator& operator=(FrameAllocator&&) = default;

    /// @brief メモリを確保する（O(1)）
    /// @param bytes 確保するバイト数
    /// @param alignment アラインメント（デフォルト: 16バイト）
    /// @return 確保されたメモリへのポインタ（容量不足時 nullptr）
    void* Allocate(size_t bytes, size_t alignment = 16)
    {
        // 実アドレスベースでアラインメント調整
        uintptr_t base    = reinterpret_cast<uintptr_t>(m_buffer.get());
        uintptr_t current = base + m_offset;
        uintptr_t aligned = (current + alignment - 1) & ~(alignment - 1);
        size_t newOffset  = static_cast<size_t>(aligned - base);
        if (newOffset + bytes > m_capacity)
            return nullptr;

        m_offset = newOffset + bytes;
        return reinterpret_cast<void*>(aligned);
    }

    /// @brief 型指定でメモリを確保する
    /// @tparam T 確保する型
    /// @param count 要素数（デフォルト: 1）
    /// @return 確保されたメモリへのポインタ
    template<typename T>
    T* Allocate(size_t count = 1)
    {
        return static_cast<T*>(Allocate(sizeof(T) * count, alignof(T)));
    }

    /// @brief フレーム開始時にリセット（全メモリを一括解放）
    void Reset() { m_offset = 0; }

    /// @brief 現在の使用量（バイト）
    /// @return 確保済みバイト数
    size_t GetUsedBytes() const { return m_offset; }

    /// @brief 総容量（バイト）
    /// @return バッファの総容量
    size_t GetCapacity() const { return m_capacity; }

    /// @brief 残り容量（バイト）
    /// @return 残りの確保可能バイト数
    size_t GetRemainingBytes() const { return m_capacity - m_offset; }

private:
    std::unique_ptr<uint8_t[]> m_buffer;
    size_t                     m_capacity;
    size_t                     m_offset;
};

} // namespace GX
