#pragma once
/// @file RingBuffer.h
/// @brief 固定容量リングバッファ (上書きモード)

#include "pch_common.h"
#include "Container/Allocator.h"
#include "Container/CompressedPair.h"
#include "Container/TypeTraits.h"
#include <cassert>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace gx::container {

/// @brief 固定容量リングバッファ (循環バッファ)
/// @details 満杯時に PushBack すると最古の要素を上書きする。
///          PopFront で FIFO 順に取り出す。operator[] で i=0 が最古の要素。
/// @tparam T 要素型
/// @tparam Alloc アロケータ型
template <typename T, typename Alloc = Allocator>
class RingBuffer
{
public:
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
    // Constructors / Destructor
    // ========================================================================

    /// @brief デフォルトコンストラクタ (容量0)
    explicit RingBuffer(Alloc alloc = *GetDefaultAllocator())
        : m_allocPair(std::move(alloc), nullptr)
    {}

    /// @brief 指定容量で構築
    explicit RingBuffer(size_type capacity, Alloc alloc = *GetDefaultAllocator())
        : m_allocPair(std::move(alloc), nullptr)
        , m_capacity(capacity)
    {
        if (m_capacity > 0)
        {
            m_allocPair.Second() = static_cast<T*>(
                GetAllocator().allocate(sizeof(T) * m_capacity));
        }
    }

    /// @brief コピーコンストラクタ
    RingBuffer(const RingBuffer& other)
        : m_allocPair(other.GetAllocator(), nullptr)
        , m_capacity(other.m_capacity)
        , m_head(0)
        , m_size(other.m_size)
    {
        if (m_capacity > 0)
        {
            m_allocPair.Second() = static_cast<T*>(
                GetAllocator().allocate(sizeof(T) * m_capacity));
            for (size_type i = 0; i < m_size; ++i)
                ::new (static_cast<void*>(Buffer() + i)) T(other[i]);
        }
    }

    /// @brief ムーブコンストラクタ
    RingBuffer(RingBuffer&& other) noexcept
        : m_allocPair(std::move(other.GetAllocator()), other.Buffer())
        , m_capacity(other.m_capacity)
        , m_head(other.m_head)
        , m_size(other.m_size)
    {
        other.m_allocPair.Second() = nullptr;
        other.m_capacity = 0;
        other.m_head = 0;
        other.m_size = 0;
    }

    /// @brief デストラクタ
    ~RingBuffer()
    {
        Clear();
        if (Buffer())
        {
            GetAllocator().deallocate(Buffer(), sizeof(T) * m_capacity);
            m_allocPair.Second() = nullptr;
        }
    }

    /// @brief コピー代入
    RingBuffer& operator=(const RingBuffer& other)
    {
        if (this != &other)
        {
            Clear();
            if (m_capacity < other.m_size)
            {
                if (Buffer())
                    GetAllocator().deallocate(Buffer(), sizeof(T) * m_capacity);
                m_capacity = other.m_capacity;
                m_allocPair.Second() = static_cast<T*>(
                    GetAllocator().allocate(sizeof(T) * m_capacity));
            }
            m_head = 0;
            m_size = other.m_size;
            for (size_type i = 0; i < m_size; ++i)
                ::new (static_cast<void*>(Buffer() + i)) T(other[i]);
        }
        return *this;
    }

    /// @brief ムーブ代入
    RingBuffer& operator=(RingBuffer&& other) noexcept
    {
        if (this != &other)
        {
            Clear();
            if (Buffer())
                GetAllocator().deallocate(Buffer(), sizeof(T) * m_capacity);

            m_allocPair.Second() = other.Buffer();
            m_capacity = other.m_capacity;
            m_head = other.m_head;
            m_size = other.m_size;

            other.m_allocPair.Second() = nullptr;
            other.m_capacity = 0;
            other.m_head = 0;
            other.m_size = 0;
        }
        return *this;
    }

    // ========================================================================
    // Element access
    // ========================================================================

    /// @brief i=0 が最古の要素
    __forceinline reference operator[](size_type i) noexcept
    {
        assert(i < m_size);
        return Buffer()[(m_head + i) % m_capacity];
    }

    /// @brief i=0 が最古の要素 (const)
    __forceinline const_reference operator[](size_type i) const noexcept
    {
        assert(i < m_size);
        return Buffer()[(m_head + i) % m_capacity];
    }

    /// @brief 最古の要素
    __forceinline reference Front() noexcept
    {
        assert(!Empty());
        return Buffer()[m_head];
    }

    /// @brief 最古の要素 (const)
    __forceinline const_reference Front() const noexcept
    {
        assert(!Empty());
        return Buffer()[m_head];
    }

    /// @brief 最新の要素
    __forceinline reference Back() noexcept
    {
        assert(!Empty());
        return Buffer()[(m_head + m_size - 1) % m_capacity];
    }

    /// @brief 最新の要素 (const)
    __forceinline const_reference Back() const noexcept
    {
        assert(!Empty());
        return Buffer()[(m_head + m_size - 1) % m_capacity];
    }

    // ========================================================================
    // Capacity
    // ========================================================================

    /// @brief 要素数
    __forceinline size_type Size() const noexcept { return m_size; }

    /// @brief 容量
    __forceinline size_type Capacity() const noexcept { return m_capacity; }

    /// @brief 空かどうか
    __forceinline bool Empty() const noexcept { return m_size == 0; }

    /// @brief 満杯かどうか
    __forceinline bool Full() const noexcept { return m_size == m_capacity; }

    // ========================================================================
    // Modifiers
    // ========================================================================

    /// @brief 末尾に要素を追加 (満杯なら最古を上書き)
    void PushBack(const T& value)
    {
        assert(m_capacity > 0);
        if (Full())
        {
            // 最古の要素を破棄して上書き
            Buffer()[m_head].~T();
            ::new (static_cast<void*>(&Buffer()[m_head])) T(value);
            m_head = (m_head + 1) % m_capacity;
        }
        else
        {
            size_type tail = (m_head + m_size) % m_capacity;
            ::new (static_cast<void*>(&Buffer()[tail])) T(value);
            ++m_size;
        }
    }

    /// @brief 末尾に要素を追加 (ムーブ、満杯なら最古を上書き)
    void PushBack(T&& value)
    {
        assert(m_capacity > 0);
        if (Full())
        {
            Buffer()[m_head].~T();
            ::new (static_cast<void*>(&Buffer()[m_head])) T(std::move(value));
            m_head = (m_head + 1) % m_capacity;
        }
        else
        {
            size_type tail = (m_head + m_size) % m_capacity;
            ::new (static_cast<void*>(&Buffer()[tail])) T(std::move(value));
            ++m_size;
        }
    }

    /// @brief 末尾に要素を直接構築 (満杯なら最古を上書き)
    template <typename... Args>
    reference EmplaceBack(Args&&... args)
    {
        assert(m_capacity > 0);
        if (Full())
        {
            Buffer()[m_head].~T();
            T* ptr = ::new (static_cast<void*>(&Buffer()[m_head])) T(std::forward<Args>(args)...);
            m_head = (m_head + 1) % m_capacity;
            return *ptr;
        }
        else
        {
            size_type tail = (m_head + m_size) % m_capacity;
            T* ptr = ::new (static_cast<void*>(&Buffer()[tail])) T(std::forward<Args>(args)...);
            ++m_size;
            return *ptr;
        }
    }

    /// @brief 最古の要素を取り出して返す
    T PopFront()
    {
        assert(!Empty());
        T value = std::move(Buffer()[m_head]);
        Buffer()[m_head].~T();
        m_head = (m_head + 1) % m_capacity;
        --m_size;
        return value;
    }

    /// @brief 全要素を削除
    void Clear() noexcept
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            for (size_type i = 0; i < m_size; ++i)
                Buffer()[(m_head + i) % m_capacity].~T();
        }
        m_head = 0;
        m_size = 0;
    }

    // ========================================================================
    // Allocator access
    // ========================================================================

    Alloc& GetAllocator() noexcept { return m_allocPair.First(); }
    const Alloc& GetAllocator() const noexcept { return m_allocPair.First(); }

    // ========================================================================
    // Validation
    // ========================================================================

    /// @brief 内部状態の整合性を検証
    bool Validate() const
    {
        if (m_size > m_capacity) return false;
        if (m_capacity > 0 && m_head >= m_capacity) return false;
        if (m_capacity == 0 && Buffer() != nullptr) return false;
        return true;
    }

private:
    // ========================================================================
    // Internal helpers
    // ========================================================================

    __forceinline T* Buffer() noexcept { return m_allocPair.Second(); }
    __forceinline const T* Buffer() const noexcept { return m_allocPair.Second(); }

    // ========================================================================
    // Data members
    // ========================================================================

    CompressedPair<Alloc, T*> m_allocPair;  ///< アロケータ + バッファポインタ
    size_type m_capacity = 0;                ///< バッファ容量
    size_type m_head = 0;                    ///< 読み出し位置
    size_type m_size = 0;                    ///< 要素数
};

} // namespace gx::container
