#pragma once
/// @file FixedForwardList.h
/// @brief 固定容量単方向リスト — 内部バッファからノードを確保

#include "pch_common.h"
#include <cassert>
#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

namespace gx::container {

/// @brief 固定容量単方向リスト
/// @tparam T 要素型
/// @tparam N 最大ノード数
template <typename T, size_t N>
class FixedForwardList
{
    struct Node
    {
        Node* next = nullptr;
        alignas(alignof(T)) char storage[sizeof(T)];

        T*       GetValue()       noexcept { return reinterpret_cast<T*>(storage); }
        const T* GetValue() const noexcept { return reinterpret_cast<const T*>(storage); }
    };

public:
    // ========================================================================
    // Iterator
    // ========================================================================
    class Iterator
    {
    public:
        using value_type = T;
        using difference_type = ptrdiff_t;
        using pointer = T*;
        using reference = T&;
        using iterator_category = std::forward_iterator_tag;

        Iterator() noexcept = default;
        explicit Iterator(Node* node) noexcept : m_node(node) {}

        __forceinline reference operator*() const noexcept { return *m_node->GetValue(); }
        __forceinline pointer   operator->() const noexcept { return m_node->GetValue(); }
        __forceinline Iterator& operator++() noexcept { m_node = m_node->next; return *this; }
        __forceinline Iterator  operator++(int) noexcept { auto t = *this; m_node = m_node->next; return t; }
        __forceinline friend bool operator==(Iterator a, Iterator b) noexcept { return a.m_node == b.m_node; }
        __forceinline friend bool operator!=(Iterator a, Iterator b) noexcept { return a.m_node != b.m_node; }

        Node* GetNode() const noexcept { return m_node; }

    private:
        Node* m_node = nullptr;
    };

    class ConstIterator
    {
    public:
        using value_type = T;
        using difference_type = ptrdiff_t;
        using pointer = const T*;
        using reference = const T&;
        using iterator_category = std::forward_iterator_tag;

        ConstIterator() noexcept = default;
        explicit ConstIterator(const Node* node) noexcept : m_node(node) {}
        ConstIterator(Iterator it) noexcept : m_node(it.GetNode()) {}

        __forceinline reference operator*() const noexcept { return *m_node->GetValue(); }
        __forceinline pointer   operator->() const noexcept { return m_node->GetValue(); }
        __forceinline ConstIterator& operator++() noexcept { m_node = m_node->next; return *this; }
        __forceinline ConstIterator  operator++(int) noexcept { auto t = *this; m_node = m_node->next; return t; }
        __forceinline friend bool operator==(ConstIterator a, ConstIterator b) noexcept { return a.m_node == b.m_node; }
        __forceinline friend bool operator!=(ConstIterator a, ConstIterator b) noexcept { return a.m_node != b.m_node; }

    private:
        const Node* m_node = nullptr;
    };

    using iterator       = Iterator;
    using const_iterator = ConstIterator;
    using value_type     = T;
    using size_type      = size_t;

    // ========================================================================
    // Constructors / Destructor
    // ========================================================================

    FixedForwardList() noexcept
    {
        m_head = nullptr;
        InitFreeList();
    }

    ~FixedForwardList() { Clear(); }

    FixedForwardList(const FixedForwardList& other) : FixedForwardList()
    {
        // Rebuild in correct order
        Node* tail = nullptr;
        for (auto it = other.begin(); it != other.end(); ++it)
        {
            Node* n = AllocNode();
            ::new (static_cast<void*>(n->GetValue())) T(*it);
            n->next = nullptr;
            if (!tail)
                m_head = n;
            else
                tail->next = n;
            tail = n;
            ++m_size;
        }
    }

    FixedForwardList& operator=(const FixedForwardList& other)
    {
        if (this != &other)
        {
            Clear();
            Node* tail = nullptr;
            for (auto it = other.begin(); it != other.end(); ++it)
            {
                Node* n = AllocNode();
                ::new (static_cast<void*>(n->GetValue())) T(*it);
                n->next = nullptr;
                if (!tail)
                    m_head = n;
                else
                    tail->next = n;
                tail = n;
                ++m_size;
            }
        }
        return *this;
    }

    // ========================================================================
    // Capacity
    // ========================================================================

    __forceinline size_type Size() const noexcept { return m_size; }
    __forceinline bool      Empty() const noexcept { return m_size == 0; }
    __forceinline size_type MaxSize() const noexcept { return N; }

    // ========================================================================
    // Element access
    // ========================================================================

    __forceinline T&       Front()       noexcept { assert(m_head); return *m_head->GetValue(); }
    __forceinline const T& Front() const noexcept { assert(m_head); return *m_head->GetValue(); }

    // ========================================================================
    // Iterators
    // ========================================================================

    __forceinline iterator       begin()        noexcept { return Iterator(m_head); }
    __forceinline const_iterator begin()  const noexcept { return ConstIterator(m_head); }
    __forceinline iterator       end()          noexcept { return Iterator(nullptr); }
    __forceinline const_iterator end()    const noexcept { return ConstIterator(nullptr); }

    // ========================================================================
    // Modifiers
    // ========================================================================

    void PushFront(const T& value)
    {
        Node* n = AllocNode();
        ::new (static_cast<void*>(n->GetValue())) T(value);
        n->next = m_head;
        m_head = n;
        ++m_size;
    }

    void PopFront() noexcept
    {
        assert(m_head);
        Node* n = m_head;
        m_head = n->next;
        n->GetValue()->~T();
        FreeNode(n);
        --m_size;
    }

    /// @brief pos の直後に挿入
    iterator InsertAfter(const_iterator pos, const T& value)
    {
        Node* after = const_cast<Node*>(pos.GetNode());
        assert(after);
        Node* n = AllocNode();
        ::new (static_cast<void*>(n->GetValue())) T(value);
        n->next = after->next;
        after->next = n;
        ++m_size;
        return Iterator(n);
    }

    /// @brief pos の直後の要素を削除
    iterator EraseAfter(const_iterator pos)
    {
        Node* after = const_cast<Node*>(pos.GetNode());
        assert(after && after->next);
        Node* target = after->next;
        after->next = target->next;
        target->GetValue()->~T();
        FreeNode(target);
        --m_size;
        return Iterator(after->next);
    }

    void Clear() noexcept
    {
        Node* cur = m_head;
        while (cur)
        {
            Node* next = cur->next;
            cur->GetValue()->~T();
            FreeNode(cur);
            cur = next;
        }
        m_head = nullptr;
        m_size = 0;
    }

    bool Validate() const noexcept { return m_size <= N; }

private:
    void InitFreeList()
    {
        m_freeHead = &m_nodes[0];
        for (size_t i = 0; i < N - 1; ++i)
            m_nodes[i].next = &m_nodes[i + 1];
        m_nodes[N - 1].next = nullptr;
    }

    Node* AllocNode()
    {
        assert(m_freeHead && "FixedForwardList overflow");
        Node* n = m_freeHead;
        m_freeHead = m_freeHead->next;
        n->next = nullptr;
        return n;
    }

    void FreeNode(Node* n)
    {
        n->next = m_freeHead;
        m_freeHead = n;
    }

    Node      m_nodes[N];
    Node*     m_head = nullptr;
    Node*     m_freeHead = nullptr;
    size_type m_size = 0;
};

} // namespace gx::container
