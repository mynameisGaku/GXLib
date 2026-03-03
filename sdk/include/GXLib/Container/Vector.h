#pragma once
/// @file Vector.h
/// @brief 動的配列コンテナ (trivially_relocatable最適化、CompressedPairアロケータ)

#include "pch_common.h"
#include "Container/Allocator.h"
#include "Container/CompressedPair.h"
#include "Container/TypeTraits.h"
#include "Container/Iterator.h"
#include <cassert>
#include <cstddef>
#include <cstring>
#include <initializer_list>
#include <iterator>
#include <new>
#include <type_traits>
#include <utility>
#include <vector>

namespace gx::container {

/// @brief 動的配列コンテナ
/// @tparam T 要素型
/// @tparam AllocatorType アロケータ型 (デフォルト: Allocator)
///
/// 3-pointer layout (m_begin, m_end, m_capacityEnd) でアロケータは
/// CompressedPair によってゼロバイト圧縮される。
/// trivially_relocatable な型に対しては memcpy による高速リロケートを行う。
template <typename T, typename AllocatorType = Allocator>
class Vector
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
    using iterator        = T*;
    using const_iterator  = const T*;
    using allocator_type  = AllocatorType;

    // ========================================================================
    // Constructors / Destructor
    // ========================================================================

    /// @brief デフォルトコンストラクタ
    Vector() noexcept
        : m_begin(nullptr)
        , m_end(nullptr)
        , m_capacityAllocator(nullptr)
    {}

    /// @brief アロケータ指定コンストラクタ
    explicit Vector(const allocator_type& alloc) noexcept
        : m_begin(nullptr)
        , m_end(nullptr)
        , m_capacityAllocator(alloc, nullptr)
    {}

    /// @brief n個デフォルト構築
    explicit Vector(size_type n, const allocator_type& alloc = allocator_type())
        : m_begin(nullptr)
        , m_end(nullptr)
        , m_capacityAllocator(alloc, nullptr)
    {
        DoInit(n);
        uninitialized_default_construct(m_begin, n);
        m_end = m_begin + n;
        SyncDebugVars();
    }

    /// @brief n個の値で初期化
    Vector(size_type n, const T& value, const allocator_type& alloc = allocator_type())
        : m_begin(nullptr)
        , m_end(nullptr)
        , m_capacityAllocator(alloc, nullptr)
    {
        DoInit(n);
        gx::container::uninitialized_fill(m_begin, n, value);
        m_end = m_begin + n;
        SyncDebugVars();
    }

    /// @brief イテレータ範囲コンストラクタ
    template <typename InputIt,
              typename = std::enable_if_t<!std::is_integral_v<InputIt>>>
    Vector(InputIt first, InputIt last, const allocator_type& alloc = allocator_type())
        : m_begin(nullptr)
        , m_end(nullptr)
        , m_capacityAllocator(alloc, nullptr)
    {
        DoAssignFromIterator(first, last, typename std::iterator_traits<InputIt>::iterator_category{});
        SyncDebugVars();
    }

    /// @brief 初期化子リストコンストラクタ
    Vector(std::initializer_list<T> il, const allocator_type& alloc = allocator_type())
        : m_begin(nullptr)
        , m_end(nullptr)
        , m_capacityAllocator(alloc, nullptr)
    {
        size_type n = il.size();
        DoInit(n);
        gx::container::uninitialized_copy(m_begin, il.begin(), n);
        m_end = m_begin + n;
        SyncDebugVars();
    }

    /// @brief コピーコンストラクタ
    Vector(const Vector& other)
        : m_begin(nullptr)
        , m_end(nullptr)
        , m_capacityAllocator(other.GetAllocator(), nullptr)
    {
        size_type n = other.size();
        if (n > 0)
        {
            DoInit(n);
            gx::container::uninitialized_copy(m_begin, other.m_begin, n);
            m_end = m_begin + n;
        }
        SyncDebugVars();
    }

    /// @brief コピーコンストラクタ (アロケータ指定)
    Vector(const Vector& other, const allocator_type& alloc)
        : m_begin(nullptr)
        , m_end(nullptr)
        , m_capacityAllocator(alloc, nullptr)
    {
        size_type n = other.size();
        if (n > 0)
        {
            DoInit(n);
            gx::container::uninitialized_copy(m_begin, other.m_begin, n);
            m_end = m_begin + n;
        }
        SyncDebugVars();
    }

    /// @brief ムーブコンストラクタ
    Vector(Vector&& other) noexcept
        : m_begin(other.m_begin)
        , m_end(other.m_end)
        , m_capacityAllocator(other.GetAllocator(), other.m_capacityAllocator.Second())
    {
        other.m_begin = nullptr;
        other.m_end = nullptr;
        other.m_capacityAllocator.Second() = nullptr;
        SyncDebugVars();
        other.SyncDebugVars();
    }

    /// @brief ムーブコンストラクタ (アロケータ指定)
    Vector(Vector&& other, const allocator_type& alloc) noexcept
        : m_begin(nullptr)
        , m_end(nullptr)
        , m_capacityAllocator(alloc, nullptr)
    {
        // 同一アロケータならポインタを奪う
        m_begin = other.m_begin;
        m_end = other.m_end;
        m_capacityAllocator.Second() = other.m_capacityAllocator.Second();
        other.m_begin = nullptr;
        other.m_end = nullptr;
        other.m_capacityAllocator.Second() = nullptr;
        SyncDebugVars();
        other.SyncDebugVars();
    }

    /// @brief std::vector からの変換コンストラクタ
    template <typename StdAlloc, typename = std::enable_if_t<!std::is_same_v<T, bool>>>
    Vector(const std::vector<T, StdAlloc>& other)
        : m_begin(nullptr)
        , m_end(nullptr)
        , m_capacityAllocator(nullptr)
    {
        size_type n = other.size();
        if (n > 0)
        {
            DoInit(n);
            gx::container::uninitialized_copy(m_begin, other.data(), n);
            m_end = m_begin + n;
        }
        SyncDebugVars();
    }

    /// @brief デストラクタ
    ~Vector()
    {
        DoDestroy();
    }

    // ========================================================================
    // Assignment
    // ========================================================================

    /// @brief std::vector からの代入
    template <typename StdAlloc, typename = std::enable_if_t<!std::is_same_v<T, bool>>>
    Vector& operator=(const std::vector<T, StdAlloc>& other)
    {
        clear();
        size_type n = other.size();
        if (n > capacity())
        {
            DoFree();
            DoInit(n);
        }
        gx::container::uninitialized_copy(m_begin, other.data(), n);
        m_end = m_begin + n;
        SyncDebugVars();
        return *this;
    }

    /// @brief コピー代入
    Vector& operator=(const Vector& other)
    {
        if (this != &other)
        {
            clear();
            if (other.size() > capacity())
            {
                DoFree();
                DoInit(other.size());
            }
            gx::container::uninitialized_copy(m_begin, other.m_begin, other.size());
            m_end = m_begin + other.size();
            SyncDebugVars();
        }
        return *this;
    }

    /// @brief ムーブ代入
    Vector& operator=(Vector&& other) noexcept
    {
        if (this != &other)
        {
            DoDestroy();
            m_begin = other.m_begin;
            m_end = other.m_end;
            m_capacityAllocator.Second() = other.m_capacityAllocator.Second();
            m_capacityAllocator.First() = other.GetAllocator();
            other.m_begin = nullptr;
            other.m_end = nullptr;
            other.m_capacityAllocator.Second() = nullptr;
            SyncDebugVars();
            other.SyncDebugVars();
        }
        return *this;
    }

    /// @brief 初期化子リスト代入
    Vector& operator=(std::initializer_list<T> il)
    {
        assign(il);
        return *this;
    }

    /// @brief n個の値で代入
    void assign(size_type n, const T& value)
    {
        clear();
        if (n > capacity())
        {
            DoFree();
            DoInit(n);
        }
        gx::container::uninitialized_fill(m_begin, n, value);
        m_end = m_begin + n;
        SyncDebugVars();
    }

    /// @brief イテレータ範囲で代入
    template <typename InputIt,
              typename = std::enable_if_t<!std::is_integral_v<InputIt>>>
    void assign(InputIt first, InputIt last)
    {
        clear();
        DoAssignFromIterator(first, last, typename std::iterator_traits<InputIt>::iterator_category{});
        SyncDebugVars();
    }

    /// @brief 初期化子リストで代入
    void assign(std::initializer_list<T> il)
    {
        assign(il.begin(), il.end());
    }

    // ========================================================================
    // Element access
    // ========================================================================

    /// @brief 添字アクセス
    __forceinline reference operator[](size_type i) noexcept
    {
        assert(i < size());
        return m_begin[i];
    }

    /// @brief 添字アクセス (const)
    __forceinline const_reference operator[](size_type i) const noexcept
    {
        assert(i < size());
        return m_begin[i];
    }

    /// @brief 先頭要素
    __forceinline reference Front() noexcept
    {
        assert(!empty());
        return *m_begin;
    }

    /// @brief 先頭要素 (const)
    __forceinline const_reference Front() const noexcept
    {
        assert(!empty());
        return *m_begin;
    }

    /// @brief 末尾要素
    __forceinline reference Back() noexcept
    {
        assert(!empty());
        return *(m_end - 1);
    }

    /// @brief 末尾要素 (const)
    __forceinline const_reference Back() const noexcept
    {
        assert(!empty());
        return *(m_end - 1);
    }

    /// @brief 生ポインタ取得
    __forceinline pointer Data() noexcept { return m_begin; }

    /// @brief 生ポインタ取得 (const)
    __forceinline const_pointer Data() const noexcept { return m_begin; }

    // ========================================================================
    // Iterators
    // ========================================================================

    __forceinline iterator       begin()        noexcept { return m_begin; }
    __forceinline const_iterator begin()  const noexcept { return m_begin; }
    __forceinline const_iterator cbegin() const noexcept { return m_begin; }
    __forceinline iterator       end()          noexcept { return m_end; }
    __forceinline const_iterator end()    const noexcept { return m_end; }
    __forceinline const_iterator cend()   const noexcept { return m_end; }

    // ========================================================================
    // Capacity
    // ========================================================================

    /// @brief 要素数
    __forceinline size_type size() const noexcept
    {
        return static_cast<size_type>(m_end - m_begin);
    }

    /// @brief キャパシティ
    __forceinline size_type capacity() const noexcept
    {
        return static_cast<size_type>(GetCapacityEnd() - m_begin);
    }

    /// @brief 空かどうか
    __forceinline bool empty() const noexcept { return m_begin == m_end; }

    /// @brief キャパシティを最低 n に拡張
    void reserve(size_type n)
    {
        if (n > capacity())
        {
            DoGrow(n);
            SyncDebugVars();
        }
    }

    /// @brief キャパシティを n に設定 (n < size なら切り詰め)
    void set_capacity(size_type n)
    {
        if (n == 0)
        {
            DoDestroy();
            m_begin = nullptr;
            m_end = nullptr;
            m_capacityAllocator.Second() = nullptr;
            SyncDebugVars();
            return;
        }

        size_type currentSize = size();
        if (n < currentSize)
        {
            // 末尾要素を破棄してから縮小
            gx::container::destroy_range(m_begin + n, currentSize - n);
            m_end = m_begin + n;
            currentSize = n;
        }

        // 新しいバッファを確保
        T* newBegin = DoAllocate(n);
        if (currentSize > 0)
            gx::container::uninitialized_relocate(newBegin, m_begin, currentSize);
        DoFree();
        m_begin = newBegin;
        m_end = newBegin + currentSize;
        SetCapacityEnd(newBegin + n);
        SyncDebugVars();
    }

    /// @brief キャパシティをサイズに合わせて縮小
    void shrink_to_fit()
    {
        if (size() < capacity())
            set_capacity(size());
    }

    // ========================================================================
    // Modifiers — push / emplace
    // ========================================================================

    /// @brief 末尾にコピー追加
    void push_back(const T& value)
    {
        if (m_end == GetCapacityEnd())
            DoGrow(GetNewCapacity(size() + 1));
        ::new (static_cast<void*>(m_end)) T(value);
        ++m_end;
        SyncDebugVars();
    }

    /// @brief 末尾にムーブ追加
    void push_back(T&& value)
    {
        if (m_end == GetCapacityEnd())
            DoGrow(GetNewCapacity(size() + 1));
        ::new (static_cast<void*>(m_end)) T(std::move(value));
        ++m_end;
        SyncDebugVars();
    }

    /// @brief 末尾にデフォルト構築で追加 (EASTL extension)
    void push_back()
    {
        if (m_end == GetCapacityEnd())
            DoGrow(GetNewCapacity(size() + 1));
        ::new (static_cast<void*>(m_end)) T();
        ++m_end;
        SyncDebugVars();
    }

    /// @brief 末尾にインプレース構築
    template <typename... Args>
    reference emplace_back(Args&&... args)
    {
        if (m_end == GetCapacityEnd())
            DoGrow(GetNewCapacity(size() + 1));
        T* p = m_end;
        ::new (static_cast<void*>(p)) T(std::forward<Args>(args)...);
        ++m_end;
        SyncDebugVars();
        return *p;
    }

    /// @brief 末尾要素を削除
    void pop_back()
    {
        assert(!empty());
        --m_end;
        m_end->~T();
        SyncDebugVars();
    }

    // ========================================================================
    // Modifiers — insert
    // ========================================================================

    /// @brief 指定位置にコピー挿入
    iterator insert(const_iterator pos, const T& value)
    {
        assert(pos >= m_begin && pos <= m_end);
        size_type offset = static_cast<size_type>(pos - m_begin);

        if (m_end == GetCapacityEnd())
        {
            DoGrow(GetNewCapacity(size() + 1));
            // pos は無効化されたのでオフセットから再計算
        }

        iterator p = m_begin + offset;
        if (p < m_end)
        {
            // 末尾の1要素を新しいスロットにムーブ構築
            ::new (static_cast<void*>(m_end)) T(std::move(*(m_end - 1)));
            // 中間要素を1つ後方にムーブ代入
            DoMoveBackward(p, m_end - 1, m_end);
            *p = value;
        }
        else
        {
            ::new (static_cast<void*>(m_end)) T(value);
        }
        ++m_end;
        SyncDebugVars();
        return p;
    }

    /// @brief 指定位置にムーブ挿入
    iterator insert(const_iterator pos, T&& value)
    {
        assert(pos >= m_begin && pos <= m_end);
        size_type offset = static_cast<size_type>(pos - m_begin);

        if (m_end == GetCapacityEnd())
            DoGrow(GetNewCapacity(size() + 1));

        iterator p = m_begin + offset;
        if (p < m_end)
        {
            ::new (static_cast<void*>(m_end)) T(std::move(*(m_end - 1)));
            DoMoveBackward(p, m_end - 1, m_end);
            *p = std::move(value);
        }
        else
        {
            ::new (static_cast<void*>(m_end)) T(std::move(value));
        }
        ++m_end;
        SyncDebugVars();
        return p;
    }

    /// @brief 指定位置にイテレータ範囲を挿入
    template <typename InputIt,
              typename = std::enable_if_t<!std::is_integral_v<InputIt>>>
    iterator insert(const_iterator pos, InputIt first, InputIt last)
    {
        assert(pos >= m_begin && pos <= m_end);
        size_type offset = static_cast<size_type>(pos - m_begin);

        // 入力範囲のサイズを計算
        size_type count = 0;
        for (auto it = first; it != last; ++it)
            ++count;

        if (count == 0)
            return m_begin + offset;

        if (size() + count > capacity())
            DoGrow(GetNewCapacity(size() + count));

        iterator p = m_begin + offset;
        size_type tailCount = static_cast<size_type>(m_end - p);

        // 後方要素を移動
        if (tailCount > 0)
        {
            // 末尾から count 分シフト
            for (size_type i = tailCount; i > 0; --i)
            {
                size_type srcIdx = static_cast<size_type>(p - m_begin) + i - 1;
                size_type dstIdx = srcIdx + count;
                if (dstIdx >= size())
                    ::new (static_cast<void*>(m_begin + dstIdx)) T(std::move(m_begin[srcIdx]));
                else
                    m_begin[dstIdx] = std::move(m_begin[srcIdx]);
                if (srcIdx >= size())
                    m_begin[srcIdx].~T();
            }
        }

        // 挿入
        auto it = first;
        for (size_type i = 0; i < count; ++i, ++it)
        {
            size_type idx = offset + i;
            if (idx >= size())
                ::new (static_cast<void*>(m_begin + idx)) T(*it);
            else
                m_begin[idx] = *it;
        }

        m_end += count;
        SyncDebugVars();
        return m_begin + offset;
    }

    /// @brief 指定位置に初期化子リストを挿入
    iterator insert(const_iterator pos, std::initializer_list<T> il)
    {
        return insert(pos, il.begin(), il.end());
    }

    // ========================================================================
    // Modifiers — erase
    // ========================================================================

    /// @brief 指定位置の要素を削除
    iterator erase(const_iterator pos)
    {
        assert(pos >= m_begin && pos < m_end);
        iterator p = const_cast<iterator>(pos);
        p->~T();

        // 後続要素をリロケート
        size_type tailCount = static_cast<size_type>(m_end - p - 1);
        if (tailCount > 0)
        {
            if constexpr (is_trivially_relocatable_v<T>)
            {
                std::memmove(p, p + 1, tailCount * sizeof(T));
            }
            else
            {
                for (iterator it = p; it < m_end - 1; ++it)
                {
                    ::new (static_cast<void*>(it)) T(std::move(*(it + 1)));
                    (it + 1)->~T();
                }
            }
        }
        --m_end;
        SyncDebugVars();
        return p;
    }

    /// @brief 範囲 [first, last) の要素を削除
    iterator erase(const_iterator first, const_iterator last)
    {
        assert(first >= m_begin && first <= m_end);
        assert(last >= first && last <= m_end);

        if (first == last)
            return const_cast<iterator>(first);

        iterator f = const_cast<iterator>(first);
        iterator l = const_cast<iterator>(last);
        size_type eraseCount = static_cast<size_type>(l - f);

        // 削除対象をデストラクト
        gx::container::destroy_range(f, eraseCount);

        // 後続要素をリロケート
        size_type tailCount = static_cast<size_type>(m_end - l);
        if (tailCount > 0)
        {
            if constexpr (is_trivially_relocatable_v<T>)
            {
                std::memmove(f, l, tailCount * sizeof(T));
            }
            else
            {
                for (size_type i = 0; i < tailCount; ++i)
                {
                    ::new (static_cast<void*>(f + i)) T(std::move(l[i]));
                    l[i].~T();
                }
            }
        }
        m_end -= eraseCount;
        SyncDebugVars();
        return f;
    }

    // ========================================================================
    // Modifiers — resize / clear
    // ========================================================================

    /// @brief サイズを n に変更 (デフォルト構築で追加)
    void resize(size_type n)
    {
        size_type currentSize = size();
        if (n > currentSize)
        {
            if (n > capacity())
                DoGrow(n);
            uninitialized_default_construct(m_end, n - currentSize);
            m_end = m_begin + n;
        }
        else if (n < currentSize)
        {
            gx::container::destroy_range(m_begin + n, currentSize - n);
            m_end = m_begin + n;
        }
        SyncDebugVars();
    }

    /// @brief サイズを n に変更 (指定値で追加)
    void resize(size_type n, const T& value)
    {
        size_type currentSize = size();
        if (n > currentSize)
        {
            if (n > capacity())
                DoGrow(n);
            gx::container::uninitialized_fill(m_end, n - currentSize, value);
            m_end = m_begin + n;
        }
        else if (n < currentSize)
        {
            gx::container::destroy_range(m_begin + n, currentSize - n);
            m_end = m_begin + n;
        }
        SyncDebugVars();
    }

    /// @brief 全要素を削除 (キャパシティは保持)
    void clear() noexcept
    {
        if (m_begin)
        {
            gx::container::destroy_range(m_begin, size());
            m_end = m_begin;
        }
        SyncDebugVars();
    }

    /// @brief メモリを解放せずにポインタをリセット (アリーナアロケータ向け)
    void reset_lose_memory() noexcept
    {
        m_begin = nullptr;
        m_end = nullptr;
        m_capacityAllocator.Second() = nullptr;
        SyncDebugVars();
    }

    // ========================================================================
    // std compatibility aliases
    // ========================================================================

    __forceinline reference front() noexcept { return Front(); }
    __forceinline const_reference front() const noexcept { return Front(); }
    __forceinline reference back() noexcept { return Back(); }
    __forceinline const_reference back() const noexcept { return Back(); }
    __forceinline pointer data() noexcept { return Data(); }
    __forceinline const_pointer data() const noexcept { return Data(); }
    __forceinline reference at(size_type i) { assert(i < size()); return m_begin[i]; }
    __forceinline const_reference at(size_type i) const { assert(i < size()); return m_begin[i]; }
    __forceinline void swap(Vector& other) noexcept { Vector tmp(std::move(*this)); *this = std::move(other); other = std::move(tmp); }

    // ========================================================================
    // Allocator
    // ========================================================================

    /// @brief アロケータ参照
    __forceinline allocator_type& GetAllocator() noexcept
    {
        return m_capacityAllocator.First();
    }

    /// @brief アロケータ参照 (const)
    __forceinline const allocator_type& GetAllocator() const noexcept
    {
        return m_capacityAllocator.First();
    }

    // ========================================================================
    // Validation (EASTL extension)
    // ========================================================================

    /// @brief コンテナの内部状態を検証
    bool Validate() const noexcept
    {
        if (m_begin == nullptr)
            return (m_end == nullptr) && (GetCapacityEnd() == nullptr);
        if (m_end < m_begin)
            return false;
        if (GetCapacityEnd() < m_end)
            return false;
        return true;
    }

    /// @brief イテレータの所属と有効性を検証
    IteratorValidity ValidateIterator(const_iterator it) const noexcept
    {
        if (it >= m_begin && it <= m_end)
            return IteratorValidity::Valid;
        return IteratorValidity::NotOwned;
    }

    // ========================================================================
    // Comparison
    // ========================================================================

    friend bool operator==(const Vector& a, const Vector& b)
    {
        if (a.size() != b.size())
            return false;
        for (size_type i = 0; i < a.size(); ++i)
        {
            if (!(a[i] == b[i]))
                return false;
        }
        return true;
    }

    friend bool operator!=(const Vector& a, const Vector& b)
    {
        return !(a == b);
    }

    friend bool operator<(const Vector& a, const Vector& b)
    {
        size_type minSize = (a.size() < b.size()) ? a.size() : b.size();
        for (size_type i = 0; i < minSize; ++i)
        {
            if (a[i] < b[i]) return true;
            if (b[i] < a[i]) return false;
        }
        return a.size() < b.size();
    }

    friend bool operator>(const Vector& a, const Vector& b)  { return b < a; }
    friend bool operator<=(const Vector& a, const Vector& b) { return !(b < a); }
    friend bool operator>=(const Vector& a, const Vector& b) { return !(a < b); }

private:
    // ========================================================================
    // Internal helpers
    // ========================================================================

    /// @brief キャパシティ末端ポインタ取得
    __forceinline T* GetCapacityEnd() const noexcept
    {
        return m_capacityAllocator.Second();
    }

    /// @brief キャパシティ末端ポインタ設定
    __forceinline void SetCapacityEnd(T* p) noexcept
    {
        m_capacityAllocator.Second() = p;
    }

    /// @brief 初期バッファ確保 (要素は未初期化)
    void DoInit(size_type n)
    {
        if (n > 0)
        {
            m_begin = DoAllocate(n);
            m_end = m_begin;
            SetCapacityEnd(m_begin + n);
        }
    }

    /// @brief アロケータからメモリ確保
    T* DoAllocate(size_type n)
    {
        return static_cast<T*>(
            GetAllocator().allocate(n * sizeof(T), alignof(T), 0, 0));
    }

    /// @brief アロケータにメモリ返却
    void DoFree()
    {
        if (m_begin)
        {
            GetAllocator().deallocate(m_begin, static_cast<size_type>(GetCapacityEnd() - m_begin) * sizeof(T));
            m_begin = nullptr;
            m_end = nullptr;
            SetCapacityEnd(nullptr);
        }
    }

    /// @brief 全要素破棄 + メモリ解放
    void DoDestroy()
    {
        if (m_begin)
        {
            gx::container::destroy_range(m_begin, size());
            DoFree();
        }
    }

    /// @brief 2x 成長ポリシー
    __forceinline size_type GetNewCapacity(size_type minCapacity) const noexcept
    {
        size_type oldCap = capacity();
        // 2x growth, minimum 8
        size_type newCap = (oldCap > 0) ? (oldCap * 2) : 8;
        if (newCap < minCapacity)
            newCap = minCapacity;
        return newCap;
    }

    /// @brief 新しいバッファに拡張 (既存要素をリロケート)
    void DoGrow(size_type newCap)
    {
        size_type oldSize = size();
        T* newBegin = DoAllocate(newCap);

        if (oldSize > 0)
            gx::container::uninitialized_relocate(newBegin, m_begin, oldSize);

        if (m_begin)
            GetAllocator().deallocate(m_begin, static_cast<size_type>(GetCapacityEnd() - m_begin) * sizeof(T));

        m_begin = newBegin;
        m_end = newBegin + oldSize;
        SetCapacityEnd(newBegin + newCap);
    }

    /// @brief 後方へムーブ代入 (挿入用)
    static void DoMoveBackward(iterator first, iterator last, iterator destLast)
    {
        while (last != first)
        {
            --last;
            --destLast;
            *destLast = std::move(*last);
        }
    }

    /// @brief InputIterator 用 assign
    template <typename InputIt>
    void DoAssignFromIterator(InputIt first, InputIt last, std::input_iterator_tag)
    {
        for (; first != last; ++first)
            push_back(*first);
    }

    /// @brief RandomAccessIterator 用 assign (サイズ事前計算)
    template <typename RandomIt>
    void DoAssignFromIterator(RandomIt first, RandomIt last, std::random_access_iterator_tag)
    {
        size_type n = static_cast<size_type>(last - first);
        if (n > 0)
        {
            if (n > capacity())
            {
                DoFree();
                DoInit(n);
            }
            // ポインタの場合は uninitialized_copy で最適化
            if constexpr (std::is_pointer_v<RandomIt> && std::is_trivially_copyable_v<T>)
            {
                std::memcpy(m_begin, first, n * sizeof(T));
            }
            else
            {
                for (size_type i = 0; i < n; ++i)
                    ::new (static_cast<void*>(m_begin + i)) T(first[i]);
            }
            m_end = m_begin + n;
        }
    }

    /// @brief ForwardIterator 用 assign
    template <typename FwdIt>
    void DoAssignFromIterator(FwdIt first, FwdIt last, std::forward_iterator_tag)
    {
        size_type n = static_cast<size_type>(std::distance(first, last));
        if (n > 0)
        {
            if (n > capacity())
            {
                DoFree();
                DoInit(n);
            }
            size_type i = 0;
            for (auto it = first; it != last; ++it, ++i)
                ::new (static_cast<void*>(m_begin + i)) T(*it);
            m_end = m_begin + n;
        }
    }

    // ========================================================================
    // Data members
    // ========================================================================

    T* m_begin;  ///< バッファ先頭
    T* m_end;    ///< 有効要素末尾 (past-the-end)

    /// @brief (Allocator, T* capacityEnd) の圧縮ペア
    /// Allocator がステートレス (empty class) なら 0 バイトに圧縮される
    CompressedPair<allocator_type, T*> m_capacityAllocator;

    size_type m_dbgSize = 0;       ///< [Debug] size() のキャッシュ（デバッガ表示用）
    size_type m_dbgCapacity = 0;   ///< [Debug] capacity() のキャッシュ（デバッガ表示用）

    /// @brief デバッグ変数をポインタから同期
    __forceinline void SyncDebugVars() noexcept
    {
        m_dbgSize = static_cast<size_type>(m_end - m_begin);
        m_dbgCapacity = static_cast<size_type>(GetCapacityEnd() - m_begin);
    }
};

/// @brief swap (ADL用)
template <typename T, typename A>
void swap(Vector<T, A>& a, Vector<T, A>& b) noexcept
{
    Vector<T, A> tmp(std::move(a));
    a = std::move(b);
    b = std::move(tmp);
}

} // namespace gx::container
