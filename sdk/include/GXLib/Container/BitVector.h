#pragma once
/// @file BitVector.h
/// @brief 動的サイズビットベクター

#include "pch_common.h"
#include "Container/Allocator.h"
#include "Container/CompressedPair.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <utility>

#ifdef _MSC_VER
#include <intrin.h>
#endif

namespace gx::container {

/// @brief 動的サイズビットベクター
/// @details Resize/Reserve/PushBack で動的にビット数を変更可能。
///          内部は uint64_t ワード配列で管理。popcount、FindFirst/FindNext を提供。
/// @tparam Alloc アロケータ型
template <typename Alloc = Allocator>
class BitVector
{
public:
    // ========================================================================
    // Constants
    // ========================================================================

    static constexpr size_t k_BitsPerWord = 64;

    /// @brief 無効なビット位置
    static constexpr size_t Npos = static_cast<size_t>(-1);

    // ========================================================================
    // Type aliases
    // ========================================================================
    using size_type = size_t;

    // ========================================================================
    // Constructors / Destructor
    // ========================================================================

    /// @brief デフォルトコンストラクタ
    explicit BitVector(Alloc alloc = *GetDefaultAllocator())
        : m_allocPair(std::move(alloc), nullptr)
    {}

    /// @brief 指定ビット数で構築
    explicit BitVector(size_type numBits, bool value = false, Alloc alloc = *GetDefaultAllocator())
        : m_allocPair(std::move(alloc), nullptr)
    {
        if (numBits > 0)
        {
            size_type wordCount = WordsNeeded(numBits);
            m_allocPair.Second() = AllocWords(wordCount);
            m_size = numBits;
            m_wordCapacity = wordCount;
            std::memset(Words(), value ? 0xFF : 0, wordCount * sizeof(uint64_t));
            if (value)
                SanitizeTail();
        }
    }

    /// @brief コピーコンストラクタ
    BitVector(const BitVector& other)
        : m_allocPair(other.GetAllocator(), nullptr)
        , m_size(other.m_size)
    {
        if (m_size > 0)
        {
            size_type wordCount = WordsNeeded(m_size);
            m_allocPair.Second() = AllocWords(wordCount);
            m_wordCapacity = wordCount;
            std::memcpy(Words(), other.Words(), wordCount * sizeof(uint64_t));
        }
    }

    /// @brief ムーブコンストラクタ
    BitVector(BitVector&& other) noexcept
        : m_allocPair(std::move(other.GetAllocator()), other.Words())
        , m_size(other.m_size)
        , m_wordCapacity(other.m_wordCapacity)
    {
        other.m_allocPair.Second() = nullptr;
        other.m_size = 0;
        other.m_wordCapacity = 0;
    }

    /// @brief デストラクタ
    ~BitVector()
    {
        if (Words())
        {
            GetAllocator().deallocate(Words(), m_wordCapacity * sizeof(uint64_t));
            m_allocPair.Second() = nullptr;
        }
    }

    /// @brief コピー代入
    BitVector& operator=(const BitVector& other)
    {
        if (this != &other)
        {
            size_type wordCount = WordsNeeded(other.m_size);
            if (m_wordCapacity < wordCount)
            {
                if (Words())
                    GetAllocator().deallocate(Words(), m_wordCapacity * sizeof(uint64_t));
                m_allocPair.Second() = AllocWords(wordCount);
                m_wordCapacity = wordCount;
            }
            m_size = other.m_size;
            if (m_size > 0)
                std::memcpy(Words(), other.Words(), wordCount * sizeof(uint64_t));
        }
        return *this;
    }

    /// @brief ムーブ代入
    BitVector& operator=(BitVector&& other) noexcept
    {
        if (this != &other)
        {
            if (Words())
                GetAllocator().deallocate(Words(), m_wordCapacity * sizeof(uint64_t));
            m_allocPair.Second() = other.Words();
            m_size = other.m_size;
            m_wordCapacity = other.m_wordCapacity;
            other.m_allocPair.Second() = nullptr;
            other.m_size = 0;
            other.m_wordCapacity = 0;
        }
        return *this;
    }

    // ========================================================================
    // Bit manipulation
    // ========================================================================

    /// @brief ビットをセット
    __forceinline void Set(size_type pos, bool value = true) noexcept
    {
        assert(pos < m_size);
        if (value)
            Words()[pos / k_BitsPerWord] |= (uint64_t{1} << (pos % k_BitsPerWord));
        else
            Words()[pos / k_BitsPerWord] &= ~(uint64_t{1} << (pos % k_BitsPerWord));
    }

    /// @brief ビットをリセット
    __forceinline void Reset(size_type pos) noexcept
    {
        assert(pos < m_size);
        Words()[pos / k_BitsPerWord] &= ~(uint64_t{1} << (pos % k_BitsPerWord));
    }

    /// @brief ビットのテスト
    __forceinline bool Test(size_type pos) const noexcept
    {
        assert(pos < m_size);
        return (Words()[pos / k_BitsPerWord] & (uint64_t{1} << (pos % k_BitsPerWord))) != 0;
    }

    /// @brief 添字アクセス
    __forceinline bool operator[](size_type pos) const noexcept { return Test(pos); }

    // ========================================================================
    // Capacity
    // ========================================================================

    /// @brief ビット数
    __forceinline size_type Size() const noexcept { return m_size; }

    /// @brief 容量 (ビット数)
    __forceinline size_type Capacity() const noexcept { return m_wordCapacity * k_BitsPerWord; }

    /// @brief 空かどうか
    __forceinline bool Empty() const noexcept { return m_size == 0; }

    // ========================================================================
    // Size manipulation
    // ========================================================================

    /// @brief ビット数を変更
    /// @param numBits 新しいビット数
    /// @param value 新しいビットの初期値
    void Resize(size_type numBits, bool value = false)
    {
        size_type oldSize = m_size;
        size_type newWordCount = WordsNeeded(numBits);

        if (newWordCount > m_wordCapacity)
            GrowTo(newWordCount);

        m_size = numBits;

        // 新しいビットを初期化
        if (numBits > oldSize)
        {
            // 旧末尾ワードの残りビットを設定
            if (oldSize > 0)
            {
                size_type oldWordIdx = (oldSize - 1) / k_BitsPerWord;
                size_type bitInWord = oldSize % k_BitsPerWord;
                if (bitInWord != 0)
                {
                    uint64_t mask = ~((uint64_t{1} << bitInWord) - 1);
                    if (value)
                        Words()[oldWordIdx] |= mask;
                    else
                        Words()[oldWordIdx] &= ~mask;
                }
                // 新しいワードを初期化
                size_type startWord = oldWordIdx + 1;
                for (size_type i = startWord; i < newWordCount; ++i)
                    Words()[i] = value ? ~uint64_t{0} : uint64_t{0};
            }
            else
            {
                // 全部新規
                for (size_type i = 0; i < newWordCount; ++i)
                    Words()[i] = value ? ~uint64_t{0} : uint64_t{0};
            }
            SanitizeTail();
        }
    }

    /// @brief 容量を確保
    void Reserve(size_type numBits)
    {
        size_type wordCount = WordsNeeded(numBits);
        if (wordCount > m_wordCapacity)
            GrowTo(wordCount);
    }

    /// @brief 末尾にビットを追加
    void PushBack(bool value)
    {
        size_type wordCount = WordsNeeded(m_size + 1);
        if (wordCount > m_wordCapacity)
            GrowTo(wordCount > m_wordCapacity * 2 ? wordCount : m_wordCapacity * 2);

        // 新しいワードの場合はゼロクリア
        if (m_size % k_BitsPerWord == 0 && m_size / k_BitsPerWord < m_wordCapacity)
            Words()[m_size / k_BitsPerWord] = 0;

        ++m_size;
        Set(m_size - 1, value);
    }

    /// @brief 末尾のビットを削除
    void PopBack() noexcept
    {
        assert(!Empty());
        --m_size;
    }

    /// @brief 全ビットをクリア
    void Clear() noexcept
    {
        m_size = 0;
    }

    // ========================================================================
    // Query operations
    // ========================================================================

    /// @brief セットされているビット数 (popcount)
    size_type Count() const noexcept
    {
        if (m_size == 0) return 0;
        size_type count = 0;
        size_type wordCount = WordsNeeded(m_size);
        for (size_type i = 0; i < wordCount; ++i)
            count += Popcount64(Words()[i]);
        return count;
    }

    /// @brief 最初にセットされたビットの位置
    size_type FindFirst() const noexcept
    {
        if (m_size == 0) return Npos;
        size_type wordCount = WordsNeeded(m_size);
        for (size_type i = 0; i < wordCount; ++i)
        {
            if (Words()[i] != 0)
            {
                size_type bit = i * k_BitsPerWord + CountTrailingZeros64(Words()[i]);
                return (bit < m_size) ? bit : Npos;
            }
        }
        return Npos;
    }

    /// @brief pos の次にセットされたビットの位置
    size_type FindNext(size_type pos) const noexcept
    {
        if (pos + 1 >= m_size) return Npos;
        pos += 1;

        size_type wordIdx = pos / k_BitsPerWord;
        size_type bitIdx  = pos % k_BitsPerWord;
        size_type wordCount = WordsNeeded(m_size);

        uint64_t word = Words()[wordIdx] >> bitIdx;
        if (word != 0)
        {
            size_type bit = wordIdx * k_BitsPerWord + bitIdx + CountTrailingZeros64(word);
            return (bit < m_size) ? bit : Npos;
        }

        for (size_type i = wordIdx + 1; i < wordCount; ++i)
        {
            if (Words()[i] != 0)
            {
                size_type bit = i * k_BitsPerWord + CountTrailingZeros64(Words()[i]);
                return (bit < m_size) ? bit : Npos;
            }
        }
        return Npos;
    }

    // ========================================================================
    // Allocator access
    // ========================================================================

    Alloc& GetAllocator() noexcept { return m_allocPair.First(); }
    const Alloc& GetAllocator() const noexcept { return m_allocPair.First(); }

    // ========================================================================
    // Comparison
    // ========================================================================

    friend bool operator==(const BitVector& a, const BitVector& b) noexcept
    {
        if (a.m_size != b.m_size) return false;
        if (a.m_size == 0) return true;
        size_type wordCount = WordsNeeded(a.m_size);
        return std::memcmp(a.Words(), b.Words(), wordCount * sizeof(uint64_t)) == 0;
    }

    friend bool operator!=(const BitVector& a, const BitVector& b) noexcept
    {
        return !(a == b);
    }

private:
    // ========================================================================
    // Internal helpers
    // ========================================================================

    __forceinline uint64_t* Words() noexcept { return m_allocPair.Second(); }
    __forceinline const uint64_t* Words() const noexcept { return m_allocPair.Second(); }

    /// @brief 必要なワード数を計算
    static constexpr size_type WordsNeeded(size_type numBits) noexcept
    {
        return (numBits + k_BitsPerWord - 1) / k_BitsPerWord;
    }

    /// @brief 末尾ワードの未使用ビットをクリア
    void SanitizeTail() noexcept
    {
        if (m_size == 0) return;
        size_type tailBits = m_size % k_BitsPerWord;
        if (tailBits != 0)
        {
            size_type lastWord = (m_size - 1) / k_BitsPerWord;
            Words()[lastWord] &= (uint64_t{1} << tailBits) - 1;
        }
    }

    /// @brief ワード配列を拡張
    void GrowTo(size_type newWordCapacity)
    {
        if (newWordCapacity == 0) newWordCapacity = 1;
        uint64_t* newWords = AllocWords(newWordCapacity);
        if (Words() && m_size > 0)
        {
            size_type copyWords = WordsNeeded(m_size);
            std::memcpy(newWords, Words(), copyWords * sizeof(uint64_t));
        }
        if (Words())
            GetAllocator().deallocate(Words(), m_wordCapacity * sizeof(uint64_t));
        m_allocPair.Second() = newWords;
        m_wordCapacity = newWordCapacity;
    }

    /// @brief ワード配列を確保
    uint64_t* AllocWords(size_type wordCount)
    {
        return static_cast<uint64_t*>(
            GetAllocator().allocate(wordCount * sizeof(uint64_t)));
    }

    /// @brief popcount (64-bit)
    static size_type Popcount64(uint64_t x) noexcept
    {
#if defined(__cpp_lib_bitops) && __cpp_lib_bitops >= 201907L
        return static_cast<size_type>(std::popcount(x));
#else
        x = x - ((x >> 1) & 0x5555555555555555ULL);
        x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
        x = (x + (x >> 4)) & 0x0f0f0f0f0f0f0f0fULL;
        return static_cast<size_type>((x * 0x0101010101010101ULL) >> 56);
#endif
    }

    /// @brief count trailing zeros (64-bit)
    static size_type CountTrailingZeros64(uint64_t x) noexcept
    {
        assert(x != 0);
#ifdef _MSC_VER
        unsigned long index;
        _BitScanForward64(&index, x);
        return static_cast<size_type>(index);
#else
        return static_cast<size_type>(__builtin_ctzll(x));
#endif
    }

    // ========================================================================
    // Data members
    // ========================================================================

    CompressedPair<Alloc, uint64_t*> m_allocPair;  ///< アロケータ + ワード配列ポインタ
    size_type m_size = 0;                           ///< ビット数
    size_type m_wordCapacity = 0;                   ///< ワード配列容量
};

} // namespace gx::container
