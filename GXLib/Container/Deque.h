#pragma once
/// @file Deque.h
/// @brief チャンクベース双方向キュー (O(1) ランダムアクセス)

#include "pch_common.h"
#include "Container/Allocator.h"
#include "Container/CompressedPair.h"
#include "Container/TypeTraits.h"
#include <cassert>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <type_traits>
#include <utility>
#include <algorithm>

namespace gx::container {

/// @brief チャンクベース双方向キュー
/// @details O(1) の push_front/push_back と O(1) ランダムアクセスを提供。
///          内部はチャンク配列 (T** m_chunks) で管理し、各チャンクは固定サイズの
///          要素配列を持つ。先頭/末尾のチャンクは部分的に使用される。
/// @tparam T 要素型
/// @tparam Alloc アロケータ型
/// @tparam ChunkSize チャンクのバイトサイズ (デフォルト 512)
template <typename T, typename Alloc = Allocator, size_t ChunkSize = 512>
class Deque
{
public:
    // ========================================================================
    // Constants
    // ========================================================================

    /// @brief 1チャンク内の要素数
    static constexpr size_t k_ElementsPerChunk = (ChunkSize / sizeof(T)) > 0
                                                 ? (ChunkSize / sizeof(T))
                                                 : 1;

    // ========================================================================
    // Type aliases
    // ========================================================================
    using value_type      = T;
    using size_type       = size_t;
    using difference_type = ptrdiff_t;
    using reference       = T&;
    using const_reference = const T&;
    using pointer         = T*;
    using const_pointer   = const T*;

    // ========================================================================
    // Iterator
    // ========================================================================

    template <typename ValueType, typename DequePtr>
    class IteratorBase
    {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type        = ValueType;
        using difference_type   = ptrdiff_t;
        using pointer           = ValueType*;
        using reference         = ValueType&;

        IteratorBase() noexcept = default;
        IteratorBase(DequePtr deque, size_type index) noexcept
            : m_deque(deque), m_index(index) {}

        __forceinline reference operator*()  const noexcept { return (*m_deque)[m_index]; }
        __forceinline pointer   operator->() const noexcept { return &(*m_deque)[m_index]; }
        __forceinline reference operator[](difference_type n) const noexcept { return (*m_deque)[m_index + n]; }

        __forceinline IteratorBase& operator++() noexcept { ++m_index; return *this; }
        __forceinline IteratorBase  operator++(int) noexcept { auto tmp = *this; ++m_index; return tmp; }
        __forceinline IteratorBase& operator--() noexcept { --m_index; return *this; }
        __forceinline IteratorBase  operator--(int) noexcept { auto tmp = *this; --m_index; return tmp; }

        __forceinline IteratorBase& operator+=(difference_type n) noexcept { m_index += n; return *this; }
        __forceinline IteratorBase& operator-=(difference_type n) noexcept { m_index -= n; return *this; }

        __forceinline friend IteratorBase operator+(IteratorBase it, difference_type n) noexcept { return IteratorBase(it.m_deque, it.m_index + n); }
        __forceinline friend IteratorBase operator+(difference_type n, IteratorBase it) noexcept { return IteratorBase(it.m_deque, it.m_index + n); }
        __forceinline friend IteratorBase operator-(IteratorBase it, difference_type n) noexcept { return IteratorBase(it.m_deque, it.m_index - n); }
        __forceinline friend difference_type operator-(IteratorBase a, IteratorBase b) noexcept { return static_cast<difference_type>(a.m_index) - static_cast<difference_type>(b.m_index); }

        __forceinline friend bool operator==(const IteratorBase& a, const IteratorBase& b) noexcept { return a.m_index == b.m_index; }
        __forceinline friend bool operator!=(const IteratorBase& a, const IteratorBase& b) noexcept { return a.m_index != b.m_index; }
        __forceinline friend bool operator<(const IteratorBase& a, const IteratorBase& b) noexcept { return a.m_index < b.m_index; }
        __forceinline friend bool operator>(const IteratorBase& a, const IteratorBase& b) noexcept { return a.m_index > b.m_index; }
        __forceinline friend bool operator<=(const IteratorBase& a, const IteratorBase& b) noexcept { return a.m_index <= b.m_index; }
        __forceinline friend bool operator>=(const IteratorBase& a, const IteratorBase& b) noexcept { return a.m_index >= b.m_index; }

        size_type GetIndex() const noexcept { return m_index; }

        /// @brief const_iterator への暗黙変換
        operator IteratorBase<const T, const Deque*>() const noexcept
            requires (!std::is_const_v<ValueType>)
        {
            return IteratorBase<const T, const Deque*>(m_deque, m_index);
        }

    private:
        DequePtr  m_deque = nullptr;
        size_type m_index = 0;
    };

    using iterator       = IteratorBase<T, Deque*>;
    using const_iterator = IteratorBase<const T, const Deque*>;

    // ========================================================================
    // Constructors / Destructor
    // ========================================================================

    /// @brief デフォルトコンストラクタ
    explicit Deque(Alloc alloc = *GetDefaultAllocator())
        : m_allocAndSize(std::move(alloc), 0)
    {}

    /// @brief count 個のデフォルト値で構築
    explicit Deque(size_type count, Alloc alloc = *GetDefaultAllocator())
        : m_allocAndSize(std::move(alloc), 0)
    {
        Resize(count);
    }

    /// @brief count 個の value で構築
    Deque(size_type count, const T& value, Alloc alloc = *GetDefaultAllocator())
        : m_allocAndSize(std::move(alloc), 0)
    {
        Resize(count, value);
    }

    /// @brief イテレータ範囲から構築
    template <typename InputIt, typename = std::enable_if_t<!std::is_integral_v<InputIt>>>
    Deque(InputIt first, InputIt last, Alloc alloc = *GetDefaultAllocator())
        : m_allocAndSize(std::move(alloc), 0)
    {
        for (; first != last; ++first)
            PushBack(*first);
    }

    /// @brief 初期化子リストから構築
    Deque(std::initializer_list<T> init, Alloc alloc = *GetDefaultAllocator())
        : m_allocAndSize(std::move(alloc), 0)
    {
        for (const auto& v : init)
            PushBack(v);
    }

    /// @brief コピーコンストラクタ
    Deque(const Deque& other)
        : m_allocAndSize(other.GetAllocator(), 0)
    {
        for (size_type i = 0; i < other.Size(); ++i)
            PushBack(other[i]);
    }

    /// @brief ムーブコンストラクタ
    Deque(Deque&& other) noexcept
        : m_chunks(other.m_chunks)
        , m_chunkCount(other.m_chunkCount)
        , m_beginOffset(other.m_beginOffset)
        , m_allocAndSize(std::move(other.GetAllocator()), other.GetSize())
    {
        other.m_chunks = nullptr;
        other.m_chunkCount = 0;
        other.m_beginOffset = 0;
        other.GetSize() = 0;
        SyncDebugVars();
        other.SyncDebugVars();
    }

    /// @brief デストラクタ
    ~Deque()
    {
        Clear();
        FreeChunks();
    }

    /// @brief コピー代入
    Deque& operator=(const Deque& other)
    {
        if (this != &other)
        {
            Clear();
            for (size_type i = 0; i < other.Size(); ++i)
                PushBack(other[i]);
        }
        return *this;
    }

    /// @brief ムーブ代入
    Deque& operator=(Deque&& other) noexcept
    {
        if (this != &other)
        {
            Clear();
            FreeChunks();
            m_chunks = other.m_chunks;
            m_chunkCount = other.m_chunkCount;
            m_beginOffset = other.m_beginOffset;
            GetSize() = other.GetSize();
            other.m_chunks = nullptr;
            other.m_chunkCount = 0;
            other.m_beginOffset = 0;
            other.GetSize() = 0;
            SyncDebugVars();
            other.SyncDebugVars();
        }
        return *this;
    }

    /// @brief 初期化子リスト代入
    Deque& operator=(std::initializer_list<T> init)
    {
        Clear();
        for (const auto& v : init)
            PushBack(v);
        return *this;
    }

    // ========================================================================
    // Element access
    // ========================================================================

    /// @brief O(1) ランダムアクセス
    __forceinline reference operator[](size_type i) noexcept
    {
        assert(i < Size());
        size_type absolute = m_beginOffset + i;
        size_type chunkIdx = absolute / k_ElementsPerChunk;
        size_type elemIdx  = absolute % k_ElementsPerChunk;
        return GetChunkData(chunkIdx)[elemIdx];
    }

    /// @brief O(1) ランダムアクセス (const)
    __forceinline const_reference operator[](size_type i) const noexcept
    {
        assert(i < Size());
        size_type absolute = m_beginOffset + i;
        size_type chunkIdx = absolute / k_ElementsPerChunk;
        size_type elemIdx  = absolute % k_ElementsPerChunk;
        return GetChunkData(chunkIdx)[elemIdx];
    }

    /// @brief 境界チェック付きアクセス (assert)
    __forceinline reference At(size_type i) noexcept
    {
        assert(i < Size());
        return (*this)[i];
    }

    __forceinline const_reference At(size_type i) const noexcept
    {
        assert(i < Size());
        return (*this)[i];
    }

    /// @brief 先頭要素
    __forceinline reference Front() noexcept { assert(!Empty()); return (*this)[0]; }
    __forceinline const_reference Front() const noexcept { assert(!Empty()); return (*this)[0]; }

    /// @brief 末尾要素
    __forceinline reference Back() noexcept { assert(!Empty()); return (*this)[Size() - 1]; }
    __forceinline const_reference Back() const noexcept { assert(!Empty()); return (*this)[Size() - 1]; }

    // ========================================================================
    // Iterators
    // ========================================================================

    __forceinline iterator       begin()        noexcept { return iterator(this, 0); }
    __forceinline const_iterator begin()  const noexcept { return const_iterator(this, 0); }
    __forceinline const_iterator cbegin() const noexcept { return const_iterator(this, 0); }
    __forceinline iterator       end()          noexcept { return iterator(this, Size()); }
    __forceinline const_iterator end()    const noexcept { return const_iterator(this, Size()); }
    __forceinline const_iterator cend()   const noexcept { return const_iterator(this, Size()); }

    // ========================================================================
    // Capacity
    // ========================================================================

    /// @brief 要素数
    __forceinline size_type Size() const noexcept { return GetSize(); }

    /// @brief 空かどうか
    __forceinline bool Empty() const noexcept { return GetSize() == 0; }

    /// @brief 現在のチャンクで保持可能な総要素数
    size_type Capacity() const noexcept
    {
        return m_chunkCount * k_ElementsPerChunk;
    }

    // ========================================================================
    // Modifiers
    // ========================================================================

    /// @brief 全要素を削除
    void Clear() noexcept
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            for (size_type i = 0; i < Size(); ++i)
                (*this)[i].~T();
        }
        GetSize() = 0;
        m_beginOffset = 0;
        SyncDebugVars();
    }

    /// @brief 末尾に要素を追加 (コピー)
    void PushBack(const T& value)
    {
        EnsureBackCapacity();
        size_type absolute = m_beginOffset + GetSize();
        size_type chunkIdx = absolute / k_ElementsPerChunk;
        size_type elemIdx  = absolute % k_ElementsPerChunk;
        ::new (static_cast<void*>(&GetChunkData(chunkIdx)[elemIdx])) T(value);
        ++GetSize();
        SyncDebugVars();
    }

    /// @brief 末尾に要素を追加 (ムーブ)
    void PushBack(T&& value)
    {
        EnsureBackCapacity();
        size_type absolute = m_beginOffset + GetSize();
        size_type chunkIdx = absolute / k_ElementsPerChunk;
        size_type elemIdx  = absolute % k_ElementsPerChunk;
        ::new (static_cast<void*>(&GetChunkData(chunkIdx)[elemIdx])) T(std::move(value));
        ++GetSize();
        SyncDebugVars();
    }

    /// @brief 末尾に要素を直接構築
    template <typename... Args>
    reference EmplaceBack(Args&&... args)
    {
        EnsureBackCapacity();
        size_type absolute = m_beginOffset + GetSize();
        size_type chunkIdx = absolute / k_ElementsPerChunk;
        size_type elemIdx  = absolute % k_ElementsPerChunk;
        T* ptr = ::new (static_cast<void*>(&GetChunkData(chunkIdx)[elemIdx])) T(std::forward<Args>(args)...);
        ++GetSize();
        SyncDebugVars();
        return *ptr;
    }

    /// @brief 先頭に要素を追加 (コピー)
    void PushFront(const T& value)
    {
        EnsureFrontCapacity();
        --m_beginOffset;
        size_type chunkIdx = m_beginOffset / k_ElementsPerChunk;
        size_type elemIdx  = m_beginOffset % k_ElementsPerChunk;
        ::new (static_cast<void*>(&GetChunkData(chunkIdx)[elemIdx])) T(value);
        ++GetSize();
        SyncDebugVars();
    }

    /// @brief 先頭に要素を追加 (ムーブ)
    void PushFront(T&& value)
    {
        EnsureFrontCapacity();
        --m_beginOffset;
        size_type chunkIdx = m_beginOffset / k_ElementsPerChunk;
        size_type elemIdx  = m_beginOffset % k_ElementsPerChunk;
        ::new (static_cast<void*>(&GetChunkData(chunkIdx)[elemIdx])) T(std::move(value));
        ++GetSize();
        SyncDebugVars();
    }

    /// @brief 先頭に要素を直接構築
    template <typename... Args>
    reference EmplaceFront(Args&&... args)
    {
        EnsureFrontCapacity();
        --m_beginOffset;
        size_type chunkIdx = m_beginOffset / k_ElementsPerChunk;
        size_type elemIdx  = m_beginOffset % k_ElementsPerChunk;
        T* ptr = ::new (static_cast<void*>(&GetChunkData(chunkIdx)[elemIdx])) T(std::forward<Args>(args)...);
        ++GetSize();
        SyncDebugVars();
        return *ptr;
    }

    /// @brief 末尾要素を削除
    void PopBack() noexcept
    {
        assert(!Empty());
        size_type absolute = m_beginOffset + GetSize() - 1;
        size_type chunkIdx = absolute / k_ElementsPerChunk;
        size_type elemIdx  = absolute % k_ElementsPerChunk;
        GetChunkData(chunkIdx)[elemIdx].~T();
        --GetSize();
        SyncDebugVars();
    }

    /// @brief 先頭要素を削除
    void PopFront() noexcept
    {
        assert(!Empty());
        size_type chunkIdx = m_beginOffset / k_ElementsPerChunk;
        size_type elemIdx  = m_beginOffset % k_ElementsPerChunk;
        GetChunkData(chunkIdx)[elemIdx].~T();
        ++m_beginOffset;
        --GetSize();
        SyncDebugVars();
    }

    /// @brief pos の前に要素を挿入
    iterator Insert(const_iterator pos, const T& value)
    {
        size_type idx = pos.GetIndex();
        assert(idx <= Size());

        if (idx == 0)
        {
            PushFront(value);
            return begin();
        }
        if (idx == Size())
        {
            PushBack(value);
            return iterator(this, Size() - 1);
        }

        // 移動量が少ない側にシフト
        if (idx < Size() / 2)
        {
            PushFront(std::move((*this)[0]));
            for (size_type i = 1; i < idx; ++i)
                (*this)[i] = std::move((*this)[i + 1]);
            (*this)[idx] = value;
        }
        else
        {
            PushBack(std::move((*this)[Size() - 1]));
            for (size_type i = Size() - 2; i > idx; --i)
                (*this)[i] = std::move((*this)[i - 1]);
            (*this)[idx] = value;
        }
        return iterator(this, idx);
    }

    /// @brief pos の前に要素を挿入 (ムーブ)
    iterator Insert(const_iterator pos, T&& value)
    {
        size_type idx = pos.GetIndex();
        assert(idx <= Size());

        if (idx == 0)
        {
            PushFront(std::move(value));
            return begin();
        }
        if (idx == Size())
        {
            PushBack(std::move(value));
            return iterator(this, Size() - 1);
        }

        if (idx < Size() / 2)
        {
            PushFront(std::move((*this)[0]));
            for (size_type i = 1; i < idx; ++i)
                (*this)[i] = std::move((*this)[i + 1]);
            (*this)[idx] = std::move(value);
        }
        else
        {
            PushBack(std::move((*this)[Size() - 1]));
            for (size_type i = Size() - 2; i > idx; --i)
                (*this)[i] = std::move((*this)[i - 1]);
            (*this)[idx] = std::move(value);
        }
        return iterator(this, idx);
    }

    /// @brief 要素を削除
    iterator Erase(const_iterator pos) noexcept
    {
        size_type idx = pos.GetIndex();
        assert(idx < Size());

        if (idx == 0)
        {
            PopFront();
            return begin();
        }
        if (idx == Size() - 1)
        {
            PopBack();
            return end();
        }

        // 移動量が少ない側にシフト
        if (idx < Size() / 2)
        {
            for (size_type i = idx; i > 0; --i)
                (*this)[i] = std::move((*this)[i - 1]);
            PopFront();
        }
        else
        {
            for (size_type i = idx; i < Size() - 1; ++i)
                (*this)[i] = std::move((*this)[i + 1]);
            PopBack();
        }
        return iterator(this, idx);
    }

    /// @brief 範囲 [first, last) を削除
    iterator Erase(const_iterator first, const_iterator last) noexcept
    {
        size_type firstIdx = first.GetIndex();
        size_type lastIdx = last.GetIndex();
        if (firstIdx == lastIdx) return iterator(this, firstIdx);

        size_type count = lastIdx - firstIdx;

        if (firstIdx < Size() - lastIdx)
        {
            // 先頭側が近い：先頭側を後ろにシフト
            for (size_type i = firstIdx; i > 0; --i)
                (*this)[i + count - 1] = std::move((*this)[i - 1]);
            for (size_type i = 0; i < count; ++i)
                PopFront();
        }
        else
        {
            // 末尾側が近い：末尾側を前にシフト
            for (size_type i = lastIdx; i < Size(); ++i)
                (*this)[i - count] = std::move((*this)[i]);
            for (size_type i = 0; i < count; ++i)
                PopBack();
        }
        return iterator(this, firstIdx);
    }

    /// @brief サイズ変更
    void Resize(size_type count)
    {
        if (count > Size())
        {
            while (Size() < count)
                EmplaceBack();
        }
        else
        {
            while (Size() > count)
                PopBack();
        }
    }

    /// @brief サイズ変更 (値指定)
    void Resize(size_type count, const T& value)
    {
        if (count > Size())
        {
            while (Size() < count)
                PushBack(value);
        }
        else
        {
            while (Size() > count)
                PopBack();
        }
    }

    // ========================================================================
    // std compatibility aliases
    // ========================================================================

    __forceinline size_type size() const noexcept { return Size(); }
    __forceinline bool empty() const noexcept { return Empty(); }
    __forceinline reference front() noexcept { return Front(); }
    __forceinline const_reference front() const noexcept { return Front(); }
    __forceinline reference back() noexcept { return Back(); }
    __forceinline const_reference back() const noexcept { return Back(); }
    void push_back(const T& value) { PushBack(value); }
    void push_back(T&& value) { PushBack(std::move(value)); }
    template <typename... Args> reference emplace_back(Args&&... args) { return EmplaceBack(std::forward<Args>(args)...); }
    void push_front(const T& value) { PushFront(value); }
    void push_front(T&& value) { PushFront(std::move(value)); }
    template <typename... Args> reference emplace_front(Args&&... args) { return EmplaceFront(std::forward<Args>(args)...); }
    void pop_back() noexcept { PopBack(); }
    void pop_front() noexcept { PopFront(); }
    void resize(size_type count) { Resize(count); }
    void resize(size_type count, const T& value) { Resize(count, value); }
    void clear() noexcept { Clear(); }
    iterator insert(const_iterator pos, const T& value) { return Insert(pos, value); }
    iterator insert(const_iterator pos, T&& value) { return Insert(pos, std::move(value)); }
    iterator erase(const_iterator pos) { return Erase(pos); }
    iterator erase(const_iterator first, const_iterator last) { return Erase(first, last); }
    __forceinline reference at(size_type i) { assert(i < Size()); return (*this)[i]; }
    __forceinline const_reference at(size_type i) const { assert(i < Size()); return (*this)[i]; }

    // ========================================================================
    // Validation
    // ========================================================================

    /// @brief 内部状態の整合性を検証
    bool Validate() const
    {
        if (Size() == 0)
            return true;
        size_type endAbsolute = m_beginOffset + Size();
        size_type neededChunks = (endAbsolute + k_ElementsPerChunk - 1) / k_ElementsPerChunk;
        return neededChunks <= m_chunkCount;
    }

    // ========================================================================
    // Allocator access
    // ========================================================================

    Alloc& GetAllocator() noexcept { return m_allocAndSize.First(); }
    const Alloc& GetAllocator() const noexcept { return m_allocAndSize.First(); }

    // ========================================================================
    // Comparison
    // ========================================================================

    friend bool operator==(const Deque& a, const Deque& b)
    {
        if (a.Size() != b.Size()) return false;
        for (size_type i = 0; i < a.Size(); ++i)
            if (!(a[i] == b[i])) return false;
        return true;
    }

    friend bool operator!=(const Deque& a, const Deque& b) { return !(a == b); }

private:
    // ========================================================================
    // Internal helpers
    // ========================================================================

    __forceinline size_type& GetSize() noexcept { return m_allocAndSize.Second(); }
    __forceinline size_type GetSize() const noexcept { return m_allocAndSize.Second(); }

    /// @brief チャンクのデータ領域を取得
    __forceinline T* GetChunkData(size_type chunkIdx) noexcept
    {
        assert(chunkIdx < m_chunkCount);
        return m_chunks[chunkIdx];
    }

    __forceinline const T* GetChunkData(size_type chunkIdx) const noexcept
    {
        assert(chunkIdx < m_chunkCount);
        return m_chunks[chunkIdx];
    }

    /// @brief 新しいチャンクを確保
    T* AllocChunk()
    {
        void* mem = GetAllocator().allocate(sizeof(T) * k_ElementsPerChunk);
        return static_cast<T*>(mem);
    }

    /// @brief チャンクを解放
    void FreeChunk(T* chunk)
    {
        GetAllocator().deallocate(chunk, sizeof(T) * k_ElementsPerChunk);
    }

    /// @brief 末尾追加のためのチャンク容量を確保
    void EnsureBackCapacity()
    {
        size_type endAbsolute = m_beginOffset + GetSize();
        size_type neededChunks = endAbsolute / k_ElementsPerChunk + 1;
        while (neededChunks > m_chunkCount)
            GrowBack();
    }

    /// @brief 先頭追加のためのチャンク容量を確保
    void EnsureFrontCapacity()
    {
        if (m_beginOffset == 0)
            GrowFront();
    }

    /// @brief 末尾にチャンクを追加
    void GrowBack()
    {
        ReallocChunkArray(m_chunkCount + 1, 0);
        m_chunks[m_chunkCount - 1] = AllocChunk();
    }

    /// @brief 先頭にチャンクを追加
    void GrowFront()
    {
        ReallocChunkArray(m_chunkCount + 1, 1);
        m_chunks[0] = AllocChunk();
        m_beginOffset += k_ElementsPerChunk;
    }

    /// @brief チャンク配列を再確保
    /// @param newCount 新しいチャンク数
    /// @param frontPad 先頭に空けるスロット数
    void ReallocChunkArray(size_type newCount, size_type frontPad)
    {
        T** newChunks = static_cast<T**>(
            GetAllocator().allocate(sizeof(T*) * newCount));
        std::memset(newChunks, 0, sizeof(T*) * newCount);

        if (m_chunks)
        {
            std::memcpy(newChunks + frontPad, m_chunks, sizeof(T*) * m_chunkCount);
            GetAllocator().deallocate(m_chunks, sizeof(T*) * m_chunkCount);
        }

        m_chunks = newChunks;
        m_chunkCount = newCount;
    }

    /// @brief 全チャンクを解放
    void FreeChunks()
    {
        if (m_chunks)
        {
            for (size_type i = 0; i < m_chunkCount; ++i)
            {
                if (m_chunks[i])
                    FreeChunk(m_chunks[i]);
            }
            GetAllocator().deallocate(m_chunks, sizeof(T*) * m_chunkCount);
            m_chunks = nullptr;
            m_chunkCount = 0;
            m_beginOffset = 0;
        }
    }

    // ========================================================================
    // Data members
    // ========================================================================

    T**       m_chunks = nullptr;       ///< チャンクポインタ配列
    size_type m_chunkCount = 0;         ///< チャンク数
    size_type m_beginOffset = 0;        ///< 先頭要素の絶対オフセット
    CompressedPair<Alloc, size_type> m_allocAndSize;  ///< アロケータ + 要素数

    size_type m_dbgSize = 0;       ///< [Debug] Size() のキャッシュ（デバッガ表示用）
    size_type m_dbgCapacity = 0;   ///< [Debug] Capacity() のキャッシュ（デバッガ表示用）

    /// @brief デバッグ変数を同期
    __forceinline void SyncDebugVars() noexcept
    {
        m_dbgSize = GetSize();
        m_dbgCapacity = m_chunkCount * k_ElementsPerChunk;
    }
};

/// @brief swap (ADL用)
template <typename T, typename Alloc, size_t ChunkSize>
void swap(Deque<T, Alloc, ChunkSize>& a, Deque<T, Alloc, ChunkSize>& b) noexcept
{
    Deque<T, Alloc, ChunkSize> tmp(std::move(a));
    a = std::move(b);
    b = std::move(tmp);
}

} // namespace gx::container
