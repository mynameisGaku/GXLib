#pragma once
/// @file PriorityQueue.h
/// @brief ヒープベース優先度キュー (EASTL拡張: Change, Remove)

#include "pch_common.h"
#include "Container/Deque.h"
#include "Container/Hash.h"
#include <cassert>
#include <cstddef>
#include <utility>

namespace gx::container {

/// @brief ヒープベース優先度キュー
/// @details 最大ヒープ (Compare = Less<T> の場合)。Push/Pop は O(log n)。
///          EASTL拡張として Change(index) と Remove(index) を提供。
/// @tparam T 要素型
/// @tparam Container 基底コンテナ型 (ランダムアクセス必須)
/// @tparam Compare 比較関数型 (デフォルト: Less<T>、最大ヒープ)
template <typename T, typename Container = Deque<T>, typename Compare = Less<T>>
class PriorityQueue
{
public:
    // ========================================================================
    // Type aliases
    // ========================================================================
    using container_type = Container;
    using value_type     = typename Container::value_type;
    using size_type      = typename Container::size_type;
    using reference      = typename Container::reference;
    using const_reference = typename Container::const_reference;

    // ========================================================================
    // Constructors
    // ========================================================================

    /// @brief デフォルトコンストラクタ
    PriorityQueue() = default;

    /// @brief 比較関数を指定して構築
    explicit PriorityQueue(const Compare& comp)
        : m_compare(comp) {}

    /// @brief 比較関数とコンテナを指定して構築
    PriorityQueue(const Compare& comp, const Container& container)
        : m_container(container), m_compare(comp)
    {
        BuildHeap();
    }

    /// @brief 比較関数とコンテナ (ムーブ) を指定して構築
    PriorityQueue(const Compare& comp, Container&& container)
        : m_container(std::move(container)), m_compare(comp)
    {
        BuildHeap();
    }

    /// @brief イテレータ範囲から構築
    template <typename InputIt>
    PriorityQueue(InputIt first, InputIt last, const Compare& comp = Compare())
        : m_compare(comp)
    {
        for (; first != last; ++first)
            m_container.PushBack(*first);
        BuildHeap();
    }

    // ========================================================================
    // Element access
    // ========================================================================

    /// @brief ヒープの先頭 (最優先要素)
    __forceinline const_reference Top() const
    {
        assert(!Empty());
        return m_container[0];
    }

    // ========================================================================
    // Capacity
    // ========================================================================

    /// @brief 要素数
    __forceinline size_type Size() const { return m_container.Size(); }

    /// @brief 空かどうか
    __forceinline bool Empty() const { return m_container.Empty(); }

    // ========================================================================
    // Modifiers
    // ========================================================================

    /// @brief 要素を追加
    void Push(const T& value)
    {
        m_container.PushBack(value);
        SiftUp(m_container.Size() - 1);
    }

    /// @brief 要素を追加 (ムーブ)
    void Push(T&& value)
    {
        m_container.PushBack(std::move(value));
        SiftUp(m_container.Size() - 1);
    }

    /// @brief 要素を直接構築して追加
    template <typename... Args>
    void Emplace(Args&&... args)
    {
        m_container.EmplaceBack(std::forward<Args>(args)...);
        SiftUp(m_container.Size() - 1);
    }

    /// @brief 最優先要素を削除
    void Pop()
    {
        assert(!Empty());
        if (m_container.Size() > 1)
        {
            using std::swap;
            swap(m_container[0], m_container[m_container.Size() - 1]);
        }
        m_container.PopBack();
        if (!m_container.Empty())
            SiftDown(0);
    }

    /// @brief 全要素を削除
    void Clear() { m_container.Clear(); }

    // ========================================================================
    // std compatibility aliases
    // ========================================================================

    __forceinline size_type size() const { return Size(); }
    __forceinline bool empty() const { return Empty(); }
    __forceinline const_reference top() const { return Top(); }
    void push(const T& value) { Push(value); }
    void push(T&& value) { Push(std::move(value)); }
    template <typename... Args> void emplace(Args&&... args) { Emplace(std::forward<Args>(args)...); }
    void pop() { Pop(); }

    // ========================================================================
    // EASTL extensions
    // ========================================================================

    /// @brief index の要素の値が変更された後、ヒープを修復
    /// @param index 変更された要素のインデックス
    void Change(size_type index)
    {
        assert(index < m_container.Size());
        // まず上方向を試み、移動しなければ下方向
        size_type newPos = SiftUp(index);
        if (newPos == index)
            SiftDown(index);
    }

    /// @brief index の要素を削除してヒープを修復
    /// @param index 削除する要素のインデックス
    void Remove(size_type index)
    {
        assert(index < m_container.Size());
        size_type last = m_container.Size() - 1;
        if (index != last)
        {
            using std::swap;
            swap(m_container[index], m_container[last]);
            m_container.PopBack();
            if (index < m_container.Size())
                Change(index);
        }
        else
        {
            m_container.PopBack();
        }
    }

    // ========================================================================
    // Container access
    // ========================================================================

    /// @brief 基底コンテナへのアクセス (読み取り専用推奨)
    Container& GetContainer() { return m_container; }
    const Container& GetContainer() const { return m_container; }

    // ========================================================================
    // Validation
    // ========================================================================

    /// @brief ヒープ条件の検証
    bool Validate() const
    {
        for (size_type i = 1; i < m_container.Size(); ++i)
        {
            size_type parent = (i - 1) / 2;
            // 親は子以上 (最大ヒープ: comp(parent, child) は false であるべき)
            if (m_compare(m_container[parent], m_container[i]))
                return false;
        }
        return true;
    }

private:
    // ========================================================================
    // Internal heap operations
    // ========================================================================

    /// @brief index の要素を上方向に移動
    /// @return 移動後のインデックス
    size_type SiftUp(size_type index)
    {
        while (index > 0)
        {
            size_type parent = (index - 1) / 2;
            if (m_compare(m_container[parent], m_container[index]))
            {
                using std::swap;
                swap(m_container[parent], m_container[index]);
                index = parent;
            }
            else
            {
                break;
            }
        }
        return index;
    }

    /// @brief index の要素を下方向に移動
    void SiftDown(size_type index)
    {
        size_type size = m_container.Size();
        while (true)
        {
            size_type left  = 2 * index + 1;
            size_type right = 2 * index + 2;
            size_type largest = index;

            if (left < size && m_compare(m_container[largest], m_container[left]))
                largest = left;
            if (right < size && m_compare(m_container[largest], m_container[right]))
                largest = right;

            if (largest != index)
            {
                using std::swap;
                swap(m_container[index], m_container[largest]);
                index = largest;
            }
            else
            {
                break;
            }
        }
    }

    /// @brief ヒープを構築 (O(n))
    void BuildHeap()
    {
        if (m_container.Size() <= 1) return;
        for (size_type i = m_container.Size() / 2; i > 0; --i)
            SiftDown(i - 1);
    }

    // ========================================================================
    // Data members
    // ========================================================================

    Container m_container;  ///< 基底コンテナ
    Compare   m_compare;    ///< 比較関数
};

} // namespace gx::container
