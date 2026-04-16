#pragma once
/// @file List.h
/// @brief 双方向連結リスト (センチネルアンカー方式、循環リスト)

#include "pch_common.h"
#include "Container/Allocator.h"
#include "Container/CompressedPair.h"
#include <cassert>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace gx::container {

/// @brief 双方向連結リスト
/// @details センチネルノードによる循環リスト実装。安定ソート (マージソート O(n log n))、
///          splice、merge、reverse、unique 等の操作をサポート。
/// @tparam T 要素型
/// @tparam Alloc アロケータ型 (デフォルト: Allocator)
template <typename T, typename Alloc = Allocator>
class List
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

    /// @brief リストノード — prev/next を先頭に配置し、SentinelNode とレイアウト互換にする
    struct Node
    {
        Node* prev;
        Node* next;
        T     value;

        template <typename... Args>
        explicit Node(Args&&... args)
            : prev(nullptr)
            , next(nullptr)
            , value(std::forward<Args>(args)...)
        {}
    };

    /// @brief センチネルノード (値を持たないアンカー)
    /// @note Node と同じオフセットに prev/next を持つ
    struct SentinelNode
    {
        Node* prev;
        Node* next;
    };

public:
    // ========================================================================
    // Iterator
    // ========================================================================

    /// @brief 双方向イテレータ
    template <typename ValueType, typename NodePtr>
    class IteratorBase
    {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
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
        __forceinline IteratorBase& operator--() noexcept { m_node = m_node->prev; return *this; }
        __forceinline IteratorBase  operator--(int) noexcept { auto tmp = *this; m_node = m_node->prev; return tmp; }

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
    explicit List(Alloc alloc = *GetDefaultAllocator())
        : m_allocAndSize(std::move(alloc), 0)
    {
        InitSentinel();
    }

    /// @brief count 個のデフォルト値で構築
    explicit List(size_type count, Alloc alloc = *GetDefaultAllocator())
        : m_allocAndSize(std::move(alloc), 0)
    {
        InitSentinel();
        for (size_type i = 0; i < count; ++i)
            EmplaceBack();
    }

    /// @brief count 個の value で構築
    List(size_type count, const T& value, Alloc alloc = *GetDefaultAllocator())
        : m_allocAndSize(std::move(alloc), 0)
    {
        InitSentinel();
        for (size_type i = 0; i < count; ++i)
            PushBack(value);
    }

    /// @brief イテレータ範囲から構築
    template <typename InputIt, typename = std::enable_if_t<!std::is_integral_v<InputIt>>>
    List(InputIt first, InputIt last, Alloc alloc = *GetDefaultAllocator())
        : m_allocAndSize(std::move(alloc), 0)
    {
        InitSentinel();
        for (; first != last; ++first)
            PushBack(*first);
    }

    /// @brief 初期化子リストから構築
    List(std::initializer_list<T> init, Alloc alloc = *GetDefaultAllocator())
        : m_allocAndSize(std::move(alloc), 0)
    {
        InitSentinel();
        for (const auto& v : init)
            PushBack(v);
    }

    /// @brief コピーコンストラクタ
    List(const List& other)
        : m_allocAndSize(other.GetAllocator(), 0)
    {
        InitSentinel();
        for (const auto& v : other)
            PushBack(v);
    }

    /// @brief ムーブコンストラクタ
    List(List&& other) noexcept
        : m_allocAndSize(std::move(other.GetAllocator()), other.GetSize())
    {
        if (other.Empty())
        {
            InitSentinel();
        }
        else
        {
            // センチネルを自分に付け替え
            m_anchor.prev = other.m_anchor.prev;
            m_anchor.next = other.m_anchor.next;
            m_anchor.next->prev = AnchorAsNode();
            m_anchor.prev->next = AnchorAsNode();
        }
        other.InitSentinel();
        other.GetSize() = 0;
        SyncDebugVars();
        other.SyncDebugVars();
    }

    /// @brief デストラクタ
    ~List()
    {
        Clear();
    }

    /// @brief コピー代入
    List& operator=(const List& other)
    {
        if (this != &other)
        {
            Clear();
            for (const auto& v : other)
                PushBack(v);
        }
        return *this;
    }

    /// @brief ムーブ代入
    List& operator=(List&& other) noexcept
    {
        if (this != &other)
        {
            Clear();
            if (!other.Empty())
            {
                m_anchor.prev = other.m_anchor.prev;
                m_anchor.next = other.m_anchor.next;
                m_anchor.next->prev = AnchorAsNode();
                m_anchor.prev->next = AnchorAsNode();
                GetSize() = other.GetSize();
            }
            other.InitSentinel();
            other.GetSize() = 0;
            SyncDebugVars();
            other.SyncDebugVars();
        }
        return *this;
    }

    /// @brief 初期化子リスト代入
    List& operator=(std::initializer_list<T> init)
    {
        Clear();
        for (const auto& v : init)
            PushBack(v);
        return *this;
    }

    // ========================================================================
    // Element access
    // ========================================================================

    /// @brief 先頭要素
    __forceinline reference Front() noexcept
    {
        assert(!Empty());
        return m_anchor.next->value;
    }

    /// @brief 先頭要素 (const)
    __forceinline const_reference Front() const noexcept
    {
        assert(!Empty());
        return m_anchor.next->value;
    }

    /// @brief 末尾要素
    __forceinline reference Back() noexcept
    {
        assert(!Empty());
        return m_anchor.prev->value;
    }

    /// @brief 末尾要素 (const)
    __forceinline const_reference Back() const noexcept
    {
        assert(!Empty());
        return m_anchor.prev->value;
    }

    // ========================================================================
    // Iterators
    // ========================================================================

    __forceinline iterator       begin()        noexcept { return iterator(m_anchor.next); }
    __forceinline const_iterator begin()  const noexcept { return const_iterator(m_anchor.next); }
    __forceinline const_iterator cbegin() const noexcept { return const_iterator(m_anchor.next); }
    __forceinline iterator       end()          noexcept { return iterator(AnchorAsNode()); }
    __forceinline const_iterator end()    const noexcept { return const_iterator(AnchorAsConstNode()); }
    __forceinline const_iterator cend()   const noexcept { return const_iterator(AnchorAsConstNode()); }

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
        Node* cur = m_anchor.next;
        Node* sentinel = AnchorAsNode();
        while (cur != sentinel)
        {
            Node* next = cur->next;
            cur->value.~T();
            GetAllocator().deallocate(cur, sizeof(Node));
            cur = next;
        }
        InitSentinel();
        GetSize() = 0;
        SyncDebugVars();
    }

    /// @brief 末尾に要素を追加 (コピー)
    void PushBack(const T& value)
    {
        InsertNode(AnchorAsNode(), value);
    }

    /// @brief 末尾に要素を追加 (ムーブ)
    void PushBack(T&& value)
    {
        InsertNode(AnchorAsNode(), std::move(value));
    }

    /// @brief 末尾に要素を直接構築
    template <typename... Args>
    reference EmplaceBack(Args&&... args)
    {
        return EmplaceNode(AnchorAsNode(), std::forward<Args>(args)...);
    }

    /// @brief 先頭に要素を追加 (コピー)
    void PushFront(const T& value)
    {
        InsertNode(m_anchor.next, value);
    }

    /// @brief 先頭に要素を追加 (ムーブ)
    void PushFront(T&& value)
    {
        InsertNode(m_anchor.next, std::move(value));
    }

    /// @brief 先頭に要素を直接構築
    template <typename... Args>
    reference EmplaceFront(Args&&... args)
    {
        return EmplaceNode(m_anchor.next, std::forward<Args>(args)...);
    }

    /// @brief 末尾要素を削除
    void PopBack() noexcept
    {
        assert(!Empty());
        EraseNode(m_anchor.prev);
    }

    /// @brief 先頭要素を削除
    void PopFront() noexcept
    {
        assert(!Empty());
        EraseNode(m_anchor.next);
    }

    /// @brief pos の前に要素を挿入 (コピー)
    iterator Insert(const_iterator pos, const T& value)
    {
        Node* node = const_cast<Node*>(pos.GetNode());
        return iterator(InsertNode(node, value));
    }

    /// @brief pos の前に要素を挿入 (ムーブ)
    iterator Insert(const_iterator pos, T&& value)
    {
        Node* node = const_cast<Node*>(pos.GetNode());
        return iterator(InsertNode(node, std::move(value)));
    }

    /// @brief pos の前に要素を直接構築
    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args)
    {
        Node* node = const_cast<Node*>(pos.GetNode());
        Node* newNode = AllocNode(std::forward<Args>(args)...);
        LinkBefore(node, newNode);
        ++GetSize();
        SyncDebugVars();
        return iterator(newNode);
    }

    /// @brief 要素を削除
    iterator Erase(const_iterator pos) noexcept
    {
        Node* node = const_cast<Node*>(pos.GetNode());
        Node* next = node->next;
        EraseNode(node);
        return iterator(next);
    }

    /// @brief 範囲 [first, last) を削除
    iterator Erase(const_iterator first, const_iterator last) noexcept
    {
        Node* cur = const_cast<Node*>(first.GetNode());
        Node* end = const_cast<Node*>(last.GetNode());
        while (cur != end)
        {
            Node* next = cur->next;
            EraseNode(cur);
            cur = next;
        }
        return iterator(end);
    }

    // ========================================================================
    // Splice — ノードを他リストから移動 (ゼロコピー)
    // ========================================================================

    /// @brief other の全要素を pos の前に移動
    void Splice(const_iterator pos, List& other) noexcept
    {
        if (&other == this || other.Empty()) return;
        Node* posNode = const_cast<Node*>(pos.GetNode());
        Node* first = other.m_anchor.next;
        Node* last = other.m_anchor.prev;

        // other から切り離し
        other.InitSentinel();
        size_type count = other.GetSize();
        other.GetSize() = 0;

        // this に接続
        first->prev = posNode->prev;
        last->next = posNode;
        posNode->prev->next = first;
        posNode->prev = last;
        GetSize() += count;
        SyncDebugVars();
        other.SyncDebugVars();
    }

    /// @brief other の it が指す要素1つを pos の前に移動
    void Splice(const_iterator pos, List& other, const_iterator it) noexcept
    {
        Node* node = const_cast<Node*>(it.GetNode());
        Node* posNode = const_cast<Node*>(pos.GetNode());
        if (node == posNode || node->next == posNode) return;

        // other から切り離し
        Unlink(node);
        --other.GetSize();

        // this に接続
        LinkBefore(posNode, node);
        ++GetSize();
        SyncDebugVars();
        other.SyncDebugVars();
    }

    /// @brief other の [first, last) を pos の前に移動
    void Splice(const_iterator pos, List& other, const_iterator first, const_iterator last) noexcept
    {
        if (first == last) return;
        Node* posNode = const_cast<Node*>(pos.GetNode());
        Node* firstNode = const_cast<Node*>(first.GetNode());
        Node* lastNode = const_cast<Node*>(last.GetNode());
        Node* beforeLast = lastNode->prev;

        // 移動する要素数をカウント
        size_type count = 0;
        for (auto it = first; it != last; ++it)
            ++count;

        // other から切り離し
        firstNode->prev->next = lastNode;
        lastNode->prev = firstNode->prev;
        other.GetSize() -= count;

        // this に接続
        firstNode->prev = posNode->prev;
        beforeLast->next = posNode;
        posNode->prev->next = firstNode;
        posNode->prev = beforeLast;
        GetSize() += count;
        SyncDebugVars();
        other.SyncDebugVars();
    }

    // ========================================================================
    // List operations
    // ========================================================================

    /// @brief リストを反転
    void Reverse() noexcept
    {
        if (Size() <= 1) return;
        Node* cur = AnchorAsNode();
        do
        {
            Node* tmp = cur->next;
            cur->next = cur->prev;
            cur->prev = tmp;
            cur = tmp;
        } while (cur != AnchorAsNode());
    }

    /// @brief 連続する重複要素を削除
    void Unique()
    {
        if (Size() <= 1) return;
        Node* cur = m_anchor.next;
        Node* sentinel = AnchorAsNode();
        while (cur->next != sentinel)
        {
            if (cur->value == cur->next->value)
                EraseNode(cur->next);
            else
                cur = cur->next;
        }
    }

    /// @brief 述語による連続重複削除
    template <typename BinaryPredicate>
    void Unique(BinaryPredicate pred)
    {
        if (Size() <= 1) return;
        Node* cur = m_anchor.next;
        Node* sentinel = AnchorAsNode();
        while (cur->next != sentinel)
        {
            if (pred(cur->value, cur->next->value))
                EraseNode(cur->next);
            else
                cur = cur->next;
        }
    }

    /// @brief 値に一致する全要素を削除
    size_type Remove(const T& value)
    {
        size_type removed = 0;
        Node* cur = m_anchor.next;
        Node* sentinel = AnchorAsNode();
        while (cur != sentinel)
        {
            Node* next = cur->next;
            if (cur->value == value)
            {
                EraseNode(cur);
                ++removed;
            }
            cur = next;
        }
        return removed;
    }

    /// @brief 述語に一致する全要素を削除
    template <typename Predicate>
    size_type RemoveIf(Predicate pred)
    {
        size_type removed = 0;
        Node* cur = m_anchor.next;
        Node* sentinel = AnchorAsNode();
        while (cur != sentinel)
        {
            Node* next = cur->next;
            if (pred(cur->value))
            {
                EraseNode(cur);
                ++removed;
            }
            cur = next;
        }
        return removed;
    }

    /// @brief 2つのソート済みリストをマージ (other は空になる)
    void Merge(List& other)
    {
        Merge(other, [](const T& a, const T& b) { return a < b; });
    }

    /// @brief 比較関数指定マージ
    template <typename Compare>
    void Merge(List& other, Compare comp)
    {
        if (this == &other || other.Empty()) return;
        Node* sentinel = AnchorAsNode();
        Node* cur = m_anchor.next;
        Node* otherCur = other.m_anchor.next;
        Node* otherSentinel = other.AnchorAsNode();

        while (cur != sentinel && otherCur != otherSentinel)
        {
            if (comp(otherCur->value, cur->value))
            {
                Node* next = otherCur->next;
                Unlink(otherCur);
                LinkBefore(cur, otherCur);
                otherCur = next;
            }
            else
            {
                cur = cur->next;
            }
        }

        // other に残りがあれば末尾に繋ぐ
        if (otherCur != otherSentinel)
        {
            Node* otherFirst = otherCur;
            Node* otherLast = other.m_anchor.prev;

            otherFirst->prev = m_anchor.prev;
            otherLast->next = sentinel;
            m_anchor.prev->next = otherFirst;
            m_anchor.prev = otherLast;
        }

        GetSize() += other.GetSize();
        other.InitSentinel();
        other.GetSize() = 0;
        SyncDebugVars();
        other.SyncDebugVars();
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
        // マージソート: リストを半分に分割し、再帰的にソート後マージ
        List carry(GetAllocator());
        List buckets[64]{ List(GetAllocator()), List(GetAllocator()), List(GetAllocator()), List(GetAllocator()),
                          List(GetAllocator()), List(GetAllocator()), List(GetAllocator()), List(GetAllocator()),
                          List(GetAllocator()), List(GetAllocator()), List(GetAllocator()), List(GetAllocator()),
                          List(GetAllocator()), List(GetAllocator()), List(GetAllocator()), List(GetAllocator()),
                          List(GetAllocator()), List(GetAllocator()), List(GetAllocator()), List(GetAllocator()),
                          List(GetAllocator()), List(GetAllocator()), List(GetAllocator()), List(GetAllocator()),
                          List(GetAllocator()), List(GetAllocator()), List(GetAllocator()), List(GetAllocator()),
                          List(GetAllocator()), List(GetAllocator()), List(GetAllocator()), List(GetAllocator()),
                          List(GetAllocator()), List(GetAllocator()), List(GetAllocator()), List(GetAllocator()),
                          List(GetAllocator()), List(GetAllocator()), List(GetAllocator()), List(GetAllocator()),
                          List(GetAllocator()), List(GetAllocator()), List(GetAllocator()), List(GetAllocator()),
                          List(GetAllocator()), List(GetAllocator()), List(GetAllocator()), List(GetAllocator()),
                          List(GetAllocator()), List(GetAllocator()), List(GetAllocator()), List(GetAllocator()),
                          List(GetAllocator()), List(GetAllocator()), List(GetAllocator()), List(GetAllocator()),
                          List(GetAllocator()), List(GetAllocator()), List(GetAllocator()), List(GetAllocator()),
                          List(GetAllocator()), List(GetAllocator()), List(GetAllocator()), List(GetAllocator()) };
        int fill = 0;

        while (!Empty())
        {
            // carry に先頭要素1つを移動
            carry.Splice(carry.begin(), *this, begin());

            int i = 0;
            while (i < fill && !buckets[i].Empty())
            {
                buckets[i].Merge(carry, comp);
                carry.Splice(carry.begin(), buckets[i]);
                ++i;
            }
            buckets[i].Splice(buckets[i].begin(), carry);
            if (i == fill) ++fill;
        }

        for (int i = 1; i < fill; ++i)
            buckets[i].Merge(buckets[i - 1], comp);

        if (fill > 0)
            Splice(begin(), buckets[fill - 1]);
    }

    // ========================================================================
    // Validation (EASTL extension)
    // ========================================================================

    /// @brief 内部状態の整合性を検証
    bool Validate() const
    {
        const Node* sentinel = AnchorAsConstNode();
        size_type count = 0;
        const Node* cur = m_anchor.next;
        while (cur != sentinel)
        {
            if (cur->prev->next != cur) return false;
            if (cur->next->prev != cur) return false;
            ++count;
            cur = cur->next;
        }
        return count == GetSize();
    }

    /// @brief メモリをリークさせてリセット (プールアロケータ用)
    void ResetLoseMemory() noexcept
    {
        InitSentinel();
        GetSize() = 0;
        SyncDebugVars();
    }

    // ========================================================================
    // Allocator access
    // ========================================================================

    /// @brief アロケータ取得
    Alloc& GetAllocator() noexcept { return m_allocAndSize.First(); }
    const Alloc& GetAllocator() const noexcept { return m_allocAndSize.First(); }

    // ========================================================================
    // Comparison
    // ========================================================================

    friend bool operator==(const List& a, const List& b)
    {
        if (a.Size() != b.Size()) return false;
        auto itA = a.begin();
        auto itB = b.begin();
        for (; itA != a.end(); ++itA, ++itB)
            if (!(*itA == *itB)) return false;
        return true;
    }

    friend bool operator!=(const List& a, const List& b) { return !(a == b); }

private:
    // ========================================================================
    // Internal helpers
    // ========================================================================

    /// @brief センチネルを自己循環に初期化
    void InitSentinel() noexcept
    {
        m_anchor.prev = AnchorAsNode();
        m_anchor.next = AnchorAsNode();
    }

    /// @brief センチネルをNodeポインタとして取得
    __forceinline Node* AnchorAsNode() noexcept
    {
        return reinterpret_cast<Node*>(&m_anchor);
    }

    __forceinline const Node* AnchorAsConstNode() const noexcept
    {
        return reinterpret_cast<const Node*>(&m_anchor);
    }

    /// @brief サイズ参照
    __forceinline size_type& GetSize() noexcept { return m_allocAndSize.Second(); }
    __forceinline size_type GetSize() const noexcept { return m_allocAndSize.Second(); }

    /// @brief ノードを確保して構築
    template <typename... Args>
    Node* AllocNode(Args&&... args)
    {
        void* mem = GetAllocator().allocate(sizeof(Node));
        return ::new (mem) Node(std::forward<Args>(args)...);
    }

    /// @brief ノードを破棄して解放
    void FreeNode(Node* node) noexcept
    {
        node->value.~T();
        GetAllocator().deallocate(node, sizeof(Node));
    }

    /// @brief node を before の前にリンク
    static void LinkBefore(Node* before, Node* node) noexcept
    {
        node->prev = before->prev;
        node->next = before;
        before->prev->next = node;
        before->prev = node;
    }

    /// @brief node をリストから切り離す
    static void Unlink(Node* node) noexcept
    {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }

    /// @brief before の前にコピー挿入
    Node* InsertNode(Node* before, const T& value)
    {
        Node* node = AllocNode(value);
        LinkBefore(before, node);
        ++GetSize();
        SyncDebugVars();
        return node;
    }

    /// @brief before の前にムーブ挿入
    Node* InsertNode(Node* before, T&& value)
    {
        Node* node = AllocNode(std::move(value));
        LinkBefore(before, node);
        ++GetSize();
        SyncDebugVars();
        return node;
    }

    /// @brief before の前に直接構築
    template <typename... Args>
    T& EmplaceNode(Node* before, Args&&... args)
    {
        Node* node = AllocNode(std::forward<Args>(args)...);
        LinkBefore(before, node);
        ++GetSize();
        SyncDebugVars();
        return node->value;
    }

    /// @brief ノードを削除
    void EraseNode(Node* node) noexcept
    {
        Unlink(node);
        FreeNode(node);
        --GetSize();
        SyncDebugVars();
    }

    // ========================================================================
    // Data members
    // ========================================================================

    SentinelNode m_anchor{};                         ///< センチネルアンカー
    CompressedPair<Alloc, size_type> m_allocAndSize;  ///< アロケータ + 要素数

    size_type m_dbgSize = 0;  ///< [Debug] Size() のキャッシュ（デバッガ表示用）

    /// @brief デバッグ変数を同期
    __forceinline void SyncDebugVars() noexcept
    {
        m_dbgSize = GetSize();
    }
};

/// @brief swap (ADL用)
template <typename T, typename Alloc>
void swap(List<T, Alloc>& a, List<T, Alloc>& b) noexcept
{
    List<T, Alloc> tmp(std::move(a));
    a = std::move(b);
    b = std::move(tmp);
}

} // namespace gx::container
