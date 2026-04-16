#pragma once
/// @file Bitset.h
/// @brief 固定サイズビットセット (constexpr対応)

#include "pch_common.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#ifdef _MSC_VER
#include <intrin.h>
#endif

namespace gx::container {

/// @brief 固定サイズビットセット
/// @details N ビットのビット配列。constexpr 対応の Set/Reset/Test/Flip、
///          ビット演算 (&, |, ^, ~)、popcount、FindFirst/FindLast/FindNext を提供。
/// @tparam N ビット数
template <size_t N>
class Bitset
{
    static_assert(N > 0, "Bitset size must be > 0");

public:
    // ========================================================================
    // Constants
    // ========================================================================

    static constexpr size_t k_BitsPerWord = 64;
    static constexpr size_t k_WordCount = (N + k_BitsPerWord - 1) / k_BitsPerWord;

    /// @brief 無効なビット位置
    static constexpr size_t Npos = static_cast<size_t>(-1);

    // ========================================================================
    // Constructors
    // ========================================================================

    /// @brief デフォルトコンストラクタ (全ビット0)
    constexpr Bitset() noexcept = default;

    /// @brief 整数値から構築
    constexpr explicit Bitset(uint64_t value) noexcept
    {
        m_words[0] = value;
        if constexpr (k_WordCount == 1)
            m_words[0] &= TailMask();
    }

    // ========================================================================
    // Bit manipulation
    // ========================================================================

    /// @brief ビットをセット
    constexpr void Set(size_t pos, bool value = true) noexcept
    {
        assert(pos < N);
        if (value)
            m_words[pos / k_BitsPerWord] |= (uint64_t{1} << (pos % k_BitsPerWord));
        else
            m_words[pos / k_BitsPerWord] &= ~(uint64_t{1} << (pos % k_BitsPerWord));
    }

    /// @brief ビットをリセット (0にする)
    constexpr void Reset(size_t pos) noexcept
    {
        assert(pos < N);
        m_words[pos / k_BitsPerWord] &= ~(uint64_t{1} << (pos % k_BitsPerWord));
    }

    /// @brief ビットを反転
    constexpr void Flip(size_t pos) noexcept
    {
        assert(pos < N);
        m_words[pos / k_BitsPerWord] ^= (uint64_t{1} << (pos % k_BitsPerWord));
    }

    /// @brief ビットのテスト
    __forceinline constexpr bool Test(size_t pos) const noexcept
    {
        assert(pos < N);
        return (m_words[pos / k_BitsPerWord] & (uint64_t{1} << (pos % k_BitsPerWord))) != 0;
    }

    /// @brief 添字アクセス
    __forceinline constexpr bool operator[](size_t pos) const noexcept { return Test(pos); }

    // ========================================================================
    // Bulk operations
    // ========================================================================

    /// @brief 全ビットをセット
    constexpr void SetAll() noexcept
    {
        for (size_t i = 0; i < k_WordCount; ++i)
            m_words[i] = ~uint64_t{0};
        SanitizeTail();
    }

    /// @brief 全ビットをリセット
    constexpr void ResetAll() noexcept
    {
        for (size_t i = 0; i < k_WordCount; ++i)
            m_words[i] = 0;
    }

    /// @brief 全ビットを反転
    constexpr void FlipAll() noexcept
    {
        for (size_t i = 0; i < k_WordCount; ++i)
            m_words[i] = ~m_words[i];
        SanitizeTail();
    }

    // ========================================================================
    // Bitwise operators
    // ========================================================================

    constexpr Bitset operator&(const Bitset& other) const noexcept
    {
        Bitset result;
        for (size_t i = 0; i < k_WordCount; ++i)
            result.m_words[i] = m_words[i] & other.m_words[i];
        return result;
    }

    constexpr Bitset operator|(const Bitset& other) const noexcept
    {
        Bitset result;
        for (size_t i = 0; i < k_WordCount; ++i)
            result.m_words[i] = m_words[i] | other.m_words[i];
        return result;
    }

    constexpr Bitset operator^(const Bitset& other) const noexcept
    {
        Bitset result;
        for (size_t i = 0; i < k_WordCount; ++i)
            result.m_words[i] = m_words[i] ^ other.m_words[i];
        return result;
    }

    constexpr Bitset operator~() const noexcept
    {
        Bitset result;
        for (size_t i = 0; i < k_WordCount; ++i)
            result.m_words[i] = ~m_words[i];
        result.SanitizeTail();
        return result;
    }

    constexpr Bitset& operator&=(const Bitset& other) noexcept
    {
        for (size_t i = 0; i < k_WordCount; ++i)
            m_words[i] &= other.m_words[i];
        return *this;
    }

    constexpr Bitset& operator|=(const Bitset& other) noexcept
    {
        for (size_t i = 0; i < k_WordCount; ++i)
            m_words[i] |= other.m_words[i];
        return *this;
    }

    constexpr Bitset& operator^=(const Bitset& other) noexcept
    {
        for (size_t i = 0; i < k_WordCount; ++i)
            m_words[i] ^= other.m_words[i];
        return *this;
    }

    // ========================================================================
    // Query operations
    // ========================================================================

    /// @brief セットされているビット数 (popcount)
    constexpr size_t Count() const noexcept
    {
        size_t count = 0;
        for (size_t i = 0; i < k_WordCount; ++i)
            count += Popcount64(m_words[i]);
        return count;
    }

    /// @brief ビット数
    constexpr size_t Size() const noexcept { return N; }

    /// @brief 全ビットが 0 か
    constexpr bool None() const noexcept
    {
        for (size_t i = 0; i < k_WordCount; ++i)
            if (m_words[i] != 0) return false;
        return true;
    }

    /// @brief いずれかのビットが 1 か
    constexpr bool Any() const noexcept { return !None(); }

    /// @brief 全ビットが 1 か
    constexpr bool All() const noexcept
    {
        for (size_t i = 0; i + 1 < k_WordCount; ++i)
            if (m_words[i] != ~uint64_t{0}) return false;
        // 最後のワードはテイルマスクで確認
        return m_words[k_WordCount - 1] == TailMask();
    }

    // ========================================================================
    // Find operations
    // ========================================================================

    /// @brief 最初にセットされたビットの位置
    size_t FindFirst() const noexcept
    {
        for (size_t i = 0; i < k_WordCount; ++i)
        {
            if (m_words[i] != 0)
            {
                size_t bit = i * k_BitsPerWord + CountTrailingZeros64(m_words[i]);
                return (bit < N) ? bit : Npos;
            }
        }
        return Npos;
    }

    /// @brief 最後にセットされたビットの位置
    size_t FindLast() const noexcept
    {
        for (size_t i = k_WordCount; i > 0; --i)
        {
            if (m_words[i - 1] != 0)
            {
                size_t bit = (i - 1) * k_BitsPerWord + (k_BitsPerWord - 1 - CountLeadingZeros64(m_words[i - 1]));
                return (bit < N) ? bit : Npos;
            }
        }
        return Npos;
    }

    /// @brief pos の次にセットされたビットの位置 (pos 自身は含まない)
    size_t FindNext(size_t pos) const noexcept
    {
        if (pos + 1 >= N) return Npos;
        pos += 1;

        size_t wordIdx = pos / k_BitsPerWord;
        size_t bitIdx  = pos % k_BitsPerWord;

        // 現在のワードでビットをマスクして検索
        uint64_t word = m_words[wordIdx] >> bitIdx;
        if (word != 0)
        {
            size_t bit = wordIdx * k_BitsPerWord + bitIdx + CountTrailingZeros64(word);
            return (bit < N) ? bit : Npos;
        }

        // 残りのワードを検索
        for (size_t i = wordIdx + 1; i < k_WordCount; ++i)
        {
            if (m_words[i] != 0)
            {
                size_t bit = i * k_BitsPerWord + CountTrailingZeros64(m_words[i]);
                return (bit < N) ? bit : Npos;
            }
        }
        return Npos;
    }

    // ========================================================================
    // Comparison
    // ========================================================================

    constexpr friend bool operator==(const Bitset& a, const Bitset& b) noexcept
    {
        for (size_t i = 0; i < k_WordCount; ++i)
            if (a.m_words[i] != b.m_words[i]) return false;
        return true;
    }

    constexpr friend bool operator!=(const Bitset& a, const Bitset& b) noexcept
    {
        return !(a == b);
    }

private:
    // ========================================================================
    // Internal helpers
    // ========================================================================

    /// @brief 末尾ワードの有効ビットマスク
    static constexpr uint64_t TailMask() noexcept
    {
        constexpr size_t tailBits = N % k_BitsPerWord;
        if constexpr (tailBits == 0)
            return ~uint64_t{0};
        else
            return (uint64_t{1} << tailBits) - 1;
    }

    /// @brief 末尾ワードの未使用ビットをクリア
    constexpr void SanitizeTail() noexcept
    {
        if constexpr (N % k_BitsPerWord != 0)
            m_words[k_WordCount - 1] &= TailMask();
    }

    /// @brief popcount (64-bit)
    static constexpr size_t Popcount64(uint64_t x) noexcept
    {
#if defined(__cpp_lib_bitops) && __cpp_lib_bitops >= 201907L
        return static_cast<size_t>(std::popcount(x));
#else
        // Hamming weight
        x = x - ((x >> 1) & 0x5555555555555555ULL);
        x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
        x = (x + (x >> 4)) & 0x0f0f0f0f0f0f0f0fULL;
        return static_cast<size_t>((x * 0x0101010101010101ULL) >> 56);
#endif
    }

    /// @brief count trailing zeros (64-bit)
    static size_t CountTrailingZeros64(uint64_t x) noexcept
    {
        assert(x != 0);
#ifdef _MSC_VER
        unsigned long index;
        _BitScanForward64(&index, x);
        return static_cast<size_t>(index);
#else
        return static_cast<size_t>(__builtin_ctzll(x));
#endif
    }

    /// @brief count leading zeros (64-bit)
    static size_t CountLeadingZeros64(uint64_t x) noexcept
    {
        assert(x != 0);
#ifdef _MSC_VER
        unsigned long index;
        _BitScanReverse64(&index, x);
        return static_cast<size_t>(63 - index);
#else
        return static_cast<size_t>(__builtin_clzll(x));
#endif
    }

    // ========================================================================
    // Data members
    // ========================================================================

    uint64_t m_words[k_WordCount]{};  ///< ビット格納配列
};

} // namespace gx::container
