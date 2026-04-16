#pragma once
/// @file ForwardList.h
/// @brief 単方向連結リスト

#include "pch_common.h"
#include "Container/Allocator.h"
#include "Container/CompressedPair.h"
#include <cassert>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace gx::container {

/// @brief 単方向連結リスト
/// @details 先頭挿入/削除 O(1)、insert_after/erase_after O(1)。
///          安定ソート (マージソート)、merge、reverse、unique 等をサポート。
/// @tparam T 要素型
/// @tparam Alloc アロケータ型 (デフォルト: Allocator)
template <typename T, typename Alloc = Allocator>
class ForwardList
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

private:
    // ========================================================================
    // Node
    // ========================================================================

    /// @brief ノード — next を先頭に配置し、NodeBase とレイアウト互換にする
    struct Node
    {
        Node* next;
        T     value;

        template <typename... Args>
        explicit Node(Args&&... args)
            : next(nullptr)
            , value(std::forward<Args>(args)...)
        {}
    };

    /// @brief before_begin 用センチネルベース (value なし)
    /// @note Node と同じオフセットに next を持つ
    struct NodeBase
    {
        Node* next = nullptr;
    };

public:
    // ========================================================================
    // Iterator
    // ========================================================================

    template <typename ValueType, typename NodePtr>
    class IteratorBase
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type        = ValueType;
        using difference_type   = ptrdiff_t;
        using pointer           = ValueType*;
        using reference         = ValueType&;

        IteratorBase() noexcept = default;
        explicit IteratorBase(NodePtr node) noexcept : m_node(node) {}

        __forceinline reference operator*()  const noexcept { return m_node->value; }
        __forceinline pointer   operator->() const noexcept { return &m_node->value; }

        __forceinline IteratorBase& operator++() noexcept { m_node = m_node->next; return *this; }
        __forceinline IteratorBase  operator++(int) noexcept { auto tmp = *this; m_node = m_node->next; return tmp; }

        __forceinline friend bool operator==(const IteratorBase& a, const IteratorBase& b) noexcept { return a.m_node == b.m_node; }
        __forceinline friend bool operator!=(const IteratorBase& a, const IteratorBase& b) noexcept { return a.m_node != b.m_node; }

        NodePtr GetNode() const noexcept { return m_node; }

        /// @brief const_iterator への暗黙変換
        operator IteratorBase<const ValueType, const Node*>() const noexcept
            requires (!std::is_const_v<ValueType>)
        {
            return IteratorBase<const ValueType, const Node*>(m_node);
        }

    private:
        NodePtr m_node = nullptr;
    };

    using iterator       = IteratorBase<T, Node*>;
    using const_iterator = IteratorBase<const T, const Node*>;

    // ========================================================================
    // Constructors / Destructor
    // ========================================================================

    /// @brief デフォルトコンストラクタ
    explicit ForwardList(Alloc alloc = *GetDefaultAllocator())
        : m_allocAndSize(std::move(alloc), 0)
    {}

    /// @brief count 個のデフォルト値で構築
    explicit ForwardList(size_type count, Alloc alloc = *GetDefaultAllocator())
        : m_allocAndSize(std::move(alloc), 0)
    {
        for (size_type i = 0; i < count; ++i)
            PushFront(T{});
    }

    /// @brief count 個の value で構築
    ForwardList(size_type count, const T& value, Alloc alloc = *GetDefaultAllocator())
        : m_allocAndSize(std::move(alloc), 0)
    {
        for (size_type i = 0; i < count; ++i)
            PushFront(value);
    }

    /// @brief イテレータ範囲から構築
    template <typename InputIt, typename = std::enable_if_t<!std::is_integral_v<InputIt>>>
    ForwardList(InputIt first, InputIt last, Alloc alloc = *GetDefaultAllocator())
        : m_allocAndSize(std::move(alloc), 0)
    {
        // 順序を保つため、一旦末尾に繋いでいく
        Node** tail = &m_beforeHead.next;
        for (; first != last; ++first)
        {
            Node* node = AllocNode(*first);
            *tail = node;
            tail = &node->next;
            ++GetSize();
        }
    }

    /// @brief 初期化子リストから構築
    ForwardList(std::initializer_list<T> init, Alloc alloc = *GetDefaultAllocator())
        : m_allocAndSize(std::move(alloc), 0)
    {
        Node** tail = &m_beforeHead.next;
        for (const auto& v : init)
        {
            Node* node = AllocNode(v);
            *tail = node;
            tail = &node->next;
            ++GetSize();
        }
    }

    /// @brief コピーコンストラクタ
    ForwardList(const ForwardList& other)
        : m_allocAndSize(other.GetAllocator(), 0)
    {
        Node** tail = &m_beforeHead.next;
        for (const auto& v : other)
        {
            Node* node = AllocNode(v);
            *tail = node;
            tail = &node->next;
            ++GetSize();
        }
    }

    /// @brief ムーブコンストラクタ
    ForwardList(ForwardList&& other) noexcept
        : m_allocAndSize(std::move(other.GetAllocator()), other.GetSize())
    {
        m_beforeHead.next = other.m_beforeHead.next;
        other.m_beforeHead.next = nullptr;
        other.GetSize() = 0;
    }

    /// @brief デストラクタ
    ~ForwardList()
    {
        Clear();
    }

    /// @brief コピー代入
    ForwardList& operator=(const ForwardList& other)
    {
        if (this != &other)
        {
            Clear();
            Node** tail = &m_beforeHead.next;
            for (const auto& v : other)
            {
                Node* node = AllocNode(v);
                *tail = node;
                tail = &node->next;
                ++GetSize();
            }
        }
        return *this;
    }

    /// @brief ムーブ代入
    ForwardList& operator=(ForwardList&& other) noexcept
    {
        if (this != &other)
        {
            Clear();
            m_beforeHead.next = other.m_beforeHead.next;
            GetSize() = other.GetSize();
            other.m_beforeHead.next = nullptr;
            other.GetSize() = 0;
        }
        return *this;
    }

    /// @brief 初期化子リスト代入
    ForwardList& operator=(std::initializer_list<T> init)
    {
        Clear();
        Node** tail = &m_beforeHead.next;
        for (const auto& v : init)
        {
            Node* node = AllocNode(v);
            *tail = node;
            tail = &node->next;
            ++GetSize();
        }
        return *this;
    }

    // ========================================================================
    // Element access
    // ========================================================================

    /// @brief 先頭要素
    __forceinline reference Front() noexcept
    {
        assert(!Empty());
        return m_beforeHead.next->value;
    }

    /// @brief 先頭要素 (const)
    __forceinline const_reference Front() const noexcept
    {
        assert(!Empty());
        return m_beforeHead.next->value;
    }

    // ========================================================================
    // Iterators
    // ========================================================================

    /// @brief 先頭の前のイテレータ (insert_after/erase_after 用)
    __forceinline iterator BeforeBegin() noexcept
    {
        // NodeBase と Node は next が同じオフセットにある
        return iterator(reinterpret_cast<Node*>(&m_beforeHead));
    }

    __forceinline const_iterator BeforeBegin() const noexcept
    {
        return const_iterator(reinterpret_cast<const Node*>(&m_beforeHead));
    }

    __forceinline iterator       begin()        noexcept { return iterator(m_beforeHead.next); }
    __forceinline const_iterator begin()  const noexcept { return const_iterator(m_beforeHead.next); }
    __forceinline const_iterator cbegin() const noexcept { return const_iterator(m_beforeHead.next); }
    __forceinline iterator       end()          noexcept { return iterator(nullptr); }
    __forceinline const_iterator end()    const noexcept { return const_iterator(nullptr); }
    __forceinline const_iterator cend()   const noexcept { return const_iterator(nullptr); }

    // ========================================================================
    // Capacity
    // ========================================================================

    /// @brief 要素数
    __forceinline size_type Size() const noexcept { return GetSize(); }

    /// @brief 空かどうか
    __forceinline bool Empty() const noexcept { return GetSize() == 0; }

    // ========================================================================
    // Modifiers
    // ========================================================================

    /// @brief 全要素を削除
    void Clear() noexcept
    {
        Node* cur = m_beforeHead.next;
        while (cur)
        {
            Node* next = cur->next;
            cur->value.~T();
            GetAllocator().deallocate(cur, sizeof(Node));
            cur = next;
        }
        m_beforeHead.next = nullptr;
        GetSize() = 0;
    }

    /// @brief 先頭に要素を追加 (コピー)
    void PushFront(const T& value)
    {
        Node* node = AllocNode(value);
        node->next = m_beforeHead.next;
        m_beforeHead.next = node;
        ++GetSize();
    }

    /// @brief 先頭に要素を追加 (ムーブ)
    void PushFront(T&& value)
    {
        Node* node = AllocNode(std::move(value));
        node->next = m_beforeHead.next;
        m_beforeHead.next = node;
        ++GetSize();
    }

    /// @brief 先頭に要素を直接構築
    template <typename... Args>
    reference EmplaceFront(Args&&... args)
    {
        Node* node = AllocNode(std::forward<Args>(args)...);
        node->next = m_beforeHead.next;
        m_beforeHead.next = node;
        ++GetSize();
        return node->value;
    }

    /// @brief 先頭要素を削除
    void PopFront() noexcept
    {
        assert(!Empty());
        Node* head = m_beforeHead.next;
        m_beforeHead.next = head->next;
        head->value.~T();
        GetAllocator().deallocate(head, sizeof(Node));
        --GetSize();
    }

    /// @brief pos の次に要素を挿入 (コピー)
    iterator InsertAfter(const_iterator pos, const T& value)
    {
        Node* posNode = const_cast<Node*>(pos.GetNode());
        Node* node = AllocNode(value);
        node->next = posNode->next;
        posNode->next = node;
        ++GetSize();
        return iterator(node);
    }

    /// @brief pos の次に要素を挿入 (ムーブ)
    iterator InsertAfter(const_iterator pos, T&& value)
    {
        Node* posNode = const_cast<Node*>(pos.GetNode());
        Node* node = AllocNode(std::move(value));
        node->next = posNode->next;
        posNode->next = node;
        ++GetSize();
        return iterator(node);
    }

    /// @brief pos の次に要素を直接構築
    template <typename... Args>
    iterator EmplaceAfter(const_iterator pos, Args&&... args)
    {
        Node* posNode = const_cast<Node*>(pos.GetNode());
        Node* node = AllocNode(std::forward<Args>(args)...);
        node->next = posNode->next;
        posNode->next = node;
        ++GetSize();
        return iterator(node);
    }

    /// @brief pos の次の要素を削除
    iterator EraseAfter(const_iterator pos) noexcept
    {
        Node* posNode = const_cast<Node*>(pos.GetNode());
        assert(posNode->next != nullptr);
        Node* target = posNode->next;
        posNode->next = target->next;
        target->value.~T();
        GetAllocator().deallocate(target, sizeof(Node));
        --GetSize();
        return iterator(posNode->next);
    }

    /// @brief (first, last) 間の要素を削除 (first と last は削除しない)
    iterator EraseAfter(const_iterator first, const_iterator last) noexcept
    {
        Node* firstNode = const_cast<Node*>(first.GetNode());
        Node* lastNode = const_cast<Node*>(last.GetNode());
        Node* cur = firstNode->next;
        while (cur != lastNode)
        {
            Node* next = cur->next;
            cur->value.~T();
            GetAllocator().deallocate(cur, sizeof(Node));
            --GetSize();
            cur = next;
        }
        firstNode->next = lastNode;
        return iterator(lastNode);
    }

    // ========================================================================
    // List operations
    // ========================================================================

    /// @brief リストを反転
    void Reverse() noexcept
    {
        Node* prev = nullptr;
        Node* cur = m_beforeHead.next;
        while (cur)
        {
            Node* next = cur->next;
            cur->next = prev;
            prev = cur;
            cur = next;
        }
        m_beforeHead.next = prev;
    }

    /// @brief 連続する重複要素を削除
    void Unique()
    {
        if (Empty()) return;
        Node* cur = m_beforeHead.next;
        while (cur->next)
        {
            if (cur->value == cur->next->value)
            {
                Node* dup = cur->next;
                cur->next = dup->next;
                dup->value.~T();
                GetAllocator().deallocate(dup, sizeof(Node));
                --GetSize();
            }
            else
            {
                cur = cur->next;
            }
        }
    }

    /// @brief 述語による連続重複削除
    template <typename BinaryPredicate>
    void Unique(BinaryPredicate pred)
    {
        if (Empty()) return;
        Node* cur = m_beforeHead.next;
        while (cur->next)
        {
            if (pred(cur->value, cur->next->value))
            {
                Node* dup = cur->next;
                cur->next = dup->next;
                dup->value.~T();
                GetAllocator().deallocate(dup, sizeof(Node));
                --GetSize();
            }
            else
            {
                cur = cur->next;
            }
        }
    }

    /// @brief 値に一致する全要素を削除
    size_type Remove(const T& value)
    {
        size_type removed = 0;
        Node* prev = reinterpret_cast<Node*>(&m_beforeHead);
        while (prev->next)
        {
            if (prev->next->value == value)
            {
                Node* target = prev->next;
                prev->next = target->next;
                target->value.~T();
                GetAllocator().deallocate(target, sizeof(Node));
                --GetSize();
                ++removed;
            }
            else
            {
                prev = prev->next;
            }
        }
        return removed;
    }

    /// @brief 述語に一致する全要素を削除
    template <typename Predicate>
    size_type RemoveIf(Predicate pred)
    {
        size_type removed = 0;
        Node* prev = reinterpret_cast<Node*>(&m_beforeHead);
        while (prev->next)
        {
            if (pred(prev->next->value))
            {
                Node* target = prev->next;
                prev->next = target->next;
                target->value.~T();
                GetAllocator().deallocate(target, sizeof(Node));
                --GetSize();
                ++removed;
            }
            else
            {
                prev = prev->next;
            }
        }
        return removed;
    }

    /// @brief 2つのソート済みリストをマージ
    void Merge(ForwardList& other)
    {
        Merge(other, [](const T& a, const T& b) { return a < b; });
    }

    /// @brief 比較関数指定マージ
    template <typename Compare>
    void Merge(ForwardList& other, Compare comp)
    {
        if (this == &other || other.Empty()) return;

        Node* prev = reinterpret_cast<Node*>(&m_beforeHead);
        Node* otherCur = other.m_beforeHead.next;

        while (prev->next && otherCur)
        {
            if (comp(otherCur->value, prev->next->value))
            {
                Node* next = otherCur->next;
                otherCur->next = prev->next;
                prev->next = otherCur;
                otherCur = next;
                prev = prev->next;
            }
            else
            {
                prev = prev->next;
            }
        }

        if (otherCur)
            prev->next = otherCur;

        GetSize() += other.GetSize();
        other.m_beforeHead.next = nullptr;
        other.GetSize() = 0;
    }

    /// @brief 安定ソート (マージソート O(n log n))
    void Sort()
    {
        Sort([](const T& a, const T& b) { return a < b; });
    }

    /// @brief 比較関数指定ソート
    template <typename Compare>
    void Sort(Compare comp)
    {
        if (Size() <= 1) return;
        m_beforeHead.next = MergeSortImpl(m_beforeHead.next, GetSize(), comp);
    }

    // ========================================================================
    // Validation
    // ========================================================================

    /// @brief 内部状態の整合性を検証
    bool Validate() const
    {
        size_type count = 0;
        const Node* cur = m_beforeHead.next;
        while (cur)
        {
            ++count;
            cur = cur->next;
        }
        return count == GetSize();
    }

    /// @brief メモリをリークさせてリセット
    void ResetLoseMemory() noexcept
    {
        m_beforeHead.next = nullptr;
        GetSize() = 0;
    }

    // ========================================================================
    // Allocator access
    // ========================================================================

    Alloc& GetAllocator() noexcept { return m_allocAndSize.First(); }
    const Alloc& GetAllocator() const noexcept { return m_allocAndSize.First(); }

    // ========================================================================
    // Comparison
    // ========================================================================

    friend bool operator==(const ForwardList& a, const ForwardList& b)
    {
        if (a.Size() != b.Size()) return false;
        auto itA = a.begin();
        auto itB = b.begin();
        for (; itA != a.end(); ++itA, ++itB)
            if (!(*itA == *itB)) return false;
        return true;
    }

    friend bool operator!=(const ForwardList& a, const ForwardList& b) { return !(a == b); }

private:
    // ========================================================================
    // Internal helpers
    // ========================================================================

    __forceinline size_type& GetSize() noexcept { return m_allocAndSize.Second(); }
    __forceinline size_type GetSize() const noexcept { return m_allocAndSize.Second(); }

    template <typename... Args>
    Node* AllocNode(Args&&... args)
    {
        void* mem = GetAllocator().allocate(sizeof(Node));
        return ::new (mem) Node(std::forward<Args>(args)...);
    }

    /// @brief マージソート実装 (再帰)
    template <typename Compare>
    Node* MergeSortImpl(Node* head, size_type count, Compare& comp)
    {
        if (count <= 1) return head;

        size_type half = count / 2;
        Node* mid = head;
        for (size_type i = 0; i < half - 1; ++i)
            mid = mid->next;
        Node* right = mid->next;
        mid->next = nullptr;

        Node* sortedLeft = MergeSortImpl(head, half, comp);
        Node* sortedRight = MergeSortImpl(right, count - half, comp);

        return MergeTwoLists(sortedLeft, sortedRight, comp);
    }

    /// @brief 2つのソート済みリストをマージ
    template <typename Compare>
    static Node* MergeTwoLists(Node* a, Node* b, Compare& comp)
    {
        Node dummy{};
        Node* tail = &dummy;

        while (a && b)
        {
            if (comp(a->value, b->value) || !comp(b->value, a->value))
            {
                tail->next = a;
                a = a->next;
            }
            else
            {
                tail->next = b;
                b = b->next;
            }
            tail = tail->next;
        }
        tail->next = a ? a : b;
        return dummy.next;
    }

    // ========================================================================
    // Data members
    // ========================================================================

    NodeBase m_beforeHead{};                          ///< before_begin センチネル
    CompressedPair<Alloc, size_type> m_allocAndSize;  ///< アロケータ + 要素数
};

/// @brief swap (ADL用)
template <typename T, typename Alloc>
void swap(ForwardList<T, Alloc>& a, ForwardList<T, Alloc>& b) noexcept
{
    ForwardList<T, Alloc> tmp(std::move(a));
    a = std::move(b);
    b = std::move(tmp);
}

} // namespace gx::container
