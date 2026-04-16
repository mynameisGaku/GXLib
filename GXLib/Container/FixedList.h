#pragma once
/// @file FixedList.h
/// @brief 固定容量双方向リスト — 内部バッファからノードを確保

#include "pch_common.h"
#include <cassert>
#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

namespace gx::container {

/// @brief 固定容量双方向リスト
/// @tparam T 要素型
/// @tparam N 最大ノード数
template <typename T, size_t N>
class FixedList
{
    struct Node
    {
        Node* prev = nullptr;
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
        using iterator_category = std::bidirectional_iterator_tag;

        Iterator() noexcept = default;
        explicit Iterator(Node* node) noexcept : m_node(node) {}

        __forceinline reference operator*() const noexcept { return *m_node->GetValue(); }
        __forceinline pointer   operator->() const noexcept { return m_node->GetValue(); }
        __forceinline Iterator& operator++() noexcept { m_node = m_node->next; return *this; }
        __forceinline Iterator  operator++(int) noexcept { auto t = *this; m_node = m_node->next; return t; }
        __forceinline Iterator& operator--() noexcept { m_node = m_node->prev; return *this; }
        __forceinline Iterator  operator--(int) noexcept { auto t = *this; m_node = m_node->prev; return t; }
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
        using iterator_category = std::bidirectional_iterator_tag;

        ConstIterator() noexcept = default;
        explicit ConstIterator(const Node* node) noexcept : m_node(node) {}
        ConstIterator(Iterator it) noexcept : m_node(it.GetNode()) {}

        __forceinline reference operator*() const noexcept { return *m_node->GetValue(); }
        __forceinline pointer   operator->() const noexcept { return m_node->GetValue(); }
        __forceinline ConstIterator& operator++() noexcept { m_node = m_node->next; return *this; }
        __forceinline ConstIterator  operator++(int) noexcept { auto t = *this; m_node = m_node->next; return t; }
        __forceinline ConstIterator& operator--() noexcept { m_node = m_node->prev; return *this; }
        __forceinline ConstIterator  operator--(int) noexcept { auto t = *this; m_node = m_node->prev; return t; }
        __forceinline friend bool operator==(ConstIterator a, ConstIterator b) noexcept { return a.m_node == b.m_node; }
        __forceinline friend bool operator!=(ConstIterator a, ConstIterator b) noexcept { return a.m_node != b.m_node; }

        const Node* GetNode() const noexcept { return m_node; }

    private:
        const Node* m_node = nullptr;
    };

    using iterator       = Iterator;
    using const_iterator = ConstIterator;
    using value_type     = T;
    using size_type      = size_t;
    using reference      = T&;
    using const_reference = const T&;

    // ========================================================================
    // Constructors / Destructor
    // ========================================================================

    FixedList() noexcept
    {
        m_sentinel.next = &m_sentinel;
        m_sentinel.prev = &m_sentinel;
        InitFreeList();
    }

    ~FixedList() { Clear(); }

    FixedList(const FixedList& other) : FixedList()
    {
        for (auto it = other.begin(); it != other.end(); ++it)
            PushBack(*it);
    }

    FixedList& operator=(const FixedList& other)
    {
        if (this != &other)
        {
            Clear();
            for (auto it = other.begin(); it != other.end(); ++it)
                PushBack(*it);
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

    __forceinline reference       Front()       noexcept { assert(m_size > 0); return *m_sentinel.next->GetValue(); }
    __forceinline const_reference Front() const noexcept { assert(m_size > 0); return *m_sentinel.next->GetValue(); }
    __forceinline reference       Back()        noexcept { assert(m_size > 0); return *m_sentinel.prev->GetValue(); }
    __forceinline const_reference Back()  const noexcept { assert(m_size > 0); return *m_sentinel.prev->GetValue(); }

    // ========================================================================
    // Iterators
    // ========================================================================

    __forceinline iterator       begin()        noexcept { return Iterator(m_sentinel.next); }
    __forceinline const_iterator begin()  const noexcept { return ConstIterator(m_sentinel.next); }
    __forceinline iterator       end()          noexcept { return Iterator(&m_sentinel); }
    __forceinline const_iterator end()    const noexcept { return ConstIterator(&m_sentinel); }

    // ========================================================================
    // Modifiers
    // ========================================================================

    void PushBack(const T& value)
    {
        Node* n = AllocNode();
        ::new (static_cast<void*>(n->GetValue())) T(value);
        LinkBefore(&m_sentinel, n);
        ++m_size;
    }

    void PushFront(const T& value)
    {
        Node* n = AllocNode();
        ::new (static_cast<void*>(n->GetValue())) T(value);
        LinkBefore(m_sentinel.next, n);
        ++m_size;
    }

    void PopBack() noexcept
    {
        assert(m_size > 0);
        Node* n = m_sentinel.prev;
        Unlink(n);
        n->GetValue()->~T();
        FreeNode(n);
        --m_size;
    }

    void PopFront() noexcept
    {
        assert(m_size > 0);
        Node* n = m_sentinel.next;
        Unlink(n);
        n->GetValue()->~T();
        FreeNode(n);
        --m_size;
    }

    iterator Insert(const_iterator pos, const T& value)
    {
        Node* n = AllocNode();
        ::new (static_cast<void*>(n->GetValue())) T(value);
        LinkBefore(const_cast<Node*>(pos.GetNode() ? pos.GetNode() : &m_sentinel), n);
        ++m_size;
        return Iterator(n);
    }

    iterator Erase(const_iterator pos)
    {
        Node* n = const_cast<Node*>(pos.GetNode());
        assert(n != &m_sentinel);
        Node* next = n->next;
        Unlink(n);
        n->GetValue()->~T();
        FreeNode(n);
        --m_size;
        return Iterator(next);
    }

    void Clear() noexcept
    {
        Node* cur = m_sentinel.next;
        while (cur != &m_sentinel)
        {
            Node* next = cur->next;
            cur->GetValue()->~T();
            FreeNode(cur);
            cur = next;
        }
        m_sentinel.next = &m_sentinel;
        m_sentinel.prev = &m_sentinel;
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
        assert(m_freeHead && "FixedList overflow");
        Node* n = m_freeHead;
        m_freeHead = m_freeHead->next;
        n->prev = nullptr;
        n->next = nullptr;
        return n;
    }

    void FreeNode(Node* n)
    {
        n->next = m_freeHead;
        m_freeHead = n;
    }

    static void LinkBefore(Node* pos, Node* n)
    {
        n->next = pos;
        n->prev = pos->prev;
        pos->prev->next = n;
        pos->prev = n;
    }

    static void Unlink(Node* n)
    {
        n->prev->next = n->next;
        n->next->prev = n->prev;
    }

    Node      m_sentinel;
    Node      m_nodes[N];
    Node*     m_freeHead = nullptr;
    size_type m_size = 0;
};

} // namespace gx::container
