#pragma once
/// @file IntrusiveList.h
/// @brief ゼロアロケーション侵入型双方向連結リスト

#include "pch_common.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <type_traits>

namespace gx::container {

/// @brief 侵入型リストノード基底
/// @details ユーザー型がこの構造体をメンバーとして持ち、IntrusiveList に登録する。
///          メモリ割り当ては一切行わず、ノードはオブジェクト自身に埋め込まれる。
struct IntrusiveListNode
{
    IntrusiveListNode* prev = nullptr;
    IntrusiveListNode* next = nullptr;
};

/// @brief ポインタ-to-メンバーからオフセットを取得するヘルパー
/// @tparam T オブジェクト型
/// @tparam NodeMember IntrusiveListNode T::* メンバーポインタ
template <typename T, IntrusiveListNode T::*NodeMember>
struct IntrusiveListTraits
{
    /// @brief オブジェクトからノードを取得
    __forceinline static IntrusiveListNode& GetNode(T& obj) noexcept
    {
        return obj.*NodeMember;
    }

    /// @brief ノードからオブジェクトを取得
    __forceinline static T& GetObject(IntrusiveListNode& node) noexcept
    {
        // メンバーポインタのオフセットを算出
        // null オブジェクトのメンバーアドレスから基底アドレスとの差分を使う
        static const auto offset = reinterpret_cast<ptrdiff_t>(
            &(reinterpret_cast<T*>(0)->*NodeMember));
        return *reinterpret_cast<T*>(reinterpret_cast<char*>(&node) - offset);
    }

    __forceinline static const T& GetObject(const IntrusiveListNode& node) noexcept
    {
        static const auto offset = reinterpret_cast<ptrdiff_t>(
            &(reinterpret_cast<const T*>(0)->*NodeMember));
        return *reinterpret_cast<const T*>(reinterpret_cast<const char*>(&node) - offset);
    }
};

/// @brief ゼロアロケーション侵入型双方向連結リスト
/// @details ノードはオブジェクト自身に埋め込まれ、メモリの割り当て/解放は行わない。
///          clear() はリンクを外すだけでオブジェクトは破棄しない。
/// @tparam T オブジェクト型
/// @tparam NodeMember IntrusiveListNode T::* メンバーポインタ
template <typename T, IntrusiveListNode T::*NodeMember>
class IntrusiveList
{
    using Traits = IntrusiveListTraits<T, NodeMember>;

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
    // Iterator
    // ========================================================================

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

        __forceinline reference operator*() const noexcept
        {
            if constexpr (std::is_const_v<ValueType>)
                return Traits::GetObject(*m_node);
            else
                return Traits::GetObject(*m_node);
        }

        __forceinline pointer operator->() const noexcept { return &(**this); }

        __forceinline IteratorBase& operator++() noexcept { m_node = m_node->next; return *this; }
        __forceinline IteratorBase  operator++(int) noexcept { auto tmp = *this; m_node = m_node->next; return tmp; }
        __forceinline IteratorBase& operator--() noexcept { m_node = m_node->prev; return *this; }
        __forceinline IteratorBase  operator--(int) noexcept { auto tmp = *this; m_node = m_node->prev; return tmp; }

        __forceinline friend bool operator==(const IteratorBase& a, const IteratorBase& b) noexcept { return a.m_node == b.m_node; }
        __forceinline friend bool operator!=(const IteratorBase& a, const IteratorBase& b) noexcept { return a.m_node != b.m_node; }

        NodePtr GetNode() const noexcept { return m_node; }

        /// @brief const_iterator への暗黙変換
        operator IteratorBase<const T, const IntrusiveListNode*>() const noexcept
            requires (!std::is_const_v<ValueType>)
        {
            return IteratorBase<const T, const IntrusiveListNode*>(m_node);
        }

    private:
        NodePtr m_node = nullptr;
    };

    using iterator       = IteratorBase<T, IntrusiveListNode*>;
    using const_iterator = IteratorBase<const T, const IntrusiveListNode*>;

    // ========================================================================
    // Constructors / Destructor
    // ========================================================================

    /// @brief デフォルトコンストラクタ (空リスト)
    IntrusiveList() noexcept
    {
        InitSentinel();
    }

    /// @brief コピー禁止 (侵入型なのでコピーに意味がない)
    IntrusiveList(const IntrusiveList&) = delete;
    IntrusiveList& operator=(const IntrusiveList&) = delete;

    /// @brief ムーブコンストラクタ
    IntrusiveList(IntrusiveList&& other) noexcept
        : m_size(other.m_size)
    {
        if (other.Empty())
        {
            InitSentinel();
        }
        else
        {
            m_anchor.prev = other.m_anchor.prev;
            m_anchor.next = other.m_anchor.next;
            m_anchor.next->prev = &m_anchor;
            m_anchor.prev->next = &m_anchor;
        }
        other.InitSentinel();
        other.m_size = 0;
    }

    /// @brief ムーブ代入
    IntrusiveList& operator=(IntrusiveList&& other) noexcept
    {
        if (this != &other)
        {
            Clear();
            if (!other.Empty())
            {
                m_anchor.prev = other.m_anchor.prev;
                m_anchor.next = other.m_anchor.next;
                m_anchor.next->prev = &m_anchor;
                m_anchor.prev->next = &m_anchor;
                m_size = other.m_size;
            }
            other.InitSentinel();
            other.m_size = 0;
        }
        return *this;
    }

    /// @brief デストラクタ (リンクを外すだけ、オブジェクトは破棄しない)
    ~IntrusiveList()
    {
        Clear();
    }

    // ========================================================================
    // Element access
    // ========================================================================

    /// @brief 先頭要素
    __forceinline reference Front() noexcept
    {
        assert(!Empty());
        return Traits::GetObject(*m_anchor.next);
    }

    __forceinline const_reference Front() const noexcept
    {
        assert(!Empty());
        return Traits::GetObject(*m_anchor.next);
    }

    /// @brief 末尾要素
    __forceinline reference Back() noexcept
    {
        assert(!Empty());
        return Traits::GetObject(*m_anchor.prev);
    }

    __forceinline const_reference Back() const noexcept
    {
        assert(!Empty());
        return Traits::GetObject(*m_anchor.prev);
    }

    // ========================================================================
    // Iterators
    // ========================================================================

    __forceinline iterator       begin()        noexcept { return iterator(m_anchor.next); }
    __forceinline const_iterator begin()  const noexcept { return const_iterator(m_anchor.next); }
    __forceinline const_iterator cbegin() const noexcept { return const_iterator(m_anchor.next); }
    __forceinline iterator       end()          noexcept { return iterator(&m_anchor); }
    __forceinline const_iterator end()    const noexcept { return const_iterator(&m_anchor); }
    __forceinline const_iterator cend()   const noexcept { return const_iterator(&m_anchor); }

    // ========================================================================
    // Capacity
    // ========================================================================

    __forceinline size_type Size() const noexcept { return m_size; }
    __forceinline bool Empty() const noexcept { return m_size == 0; }

    // ========================================================================
    // Modifiers
    // ========================================================================

    /// @brief 末尾にオブジェクトを追加 (既にリストに入っていないこと)
    void PushBack(T& obj) noexcept
    {
        IntrusiveListNode& node = Traits::GetNode(obj);
        assert(node.prev == nullptr && node.next == nullptr);
        LinkBefore(&m_anchor, &node);
        ++m_size;
    }

    /// @brief 先頭にオブジェクトを追加
    void PushFront(T& obj) noexcept
    {
        IntrusiveListNode& node = Traits::GetNode(obj);
        assert(node.prev == nullptr && node.next == nullptr);
        LinkBefore(m_anchor.next, &node);
        ++m_size;
    }

    /// @brief 末尾要素をリストから外す
    void PopBack() noexcept
    {
        assert(!Empty());
        IntrusiveListNode* node = m_anchor.prev;
        Unlink(node);
        node->prev = nullptr;
        node->next = nullptr;
        --m_size;
    }

    /// @brief 先頭要素をリストから外す
    void PopFront() noexcept
    {
        assert(!Empty());
        IntrusiveListNode* node = m_anchor.next;
        Unlink(node);
        node->prev = nullptr;
        node->next = nullptr;
        --m_size;
    }

    /// @brief pos の前にオブジェクトを挿入
    iterator Insert(const_iterator pos, T& obj) noexcept
    {
        IntrusiveListNode& node = Traits::GetNode(obj);
        assert(node.prev == nullptr && node.next == nullptr);
        IntrusiveListNode* posNode = const_cast<IntrusiveListNode*>(pos.GetNode());
        LinkBefore(posNode, &node);
        ++m_size;
        return iterator(&node);
    }

    /// @brief 指定オブジェクトをリストから外す
    void Remove(T& obj) noexcept
    {
        IntrusiveListNode& node = Traits::GetNode(obj);
        assert(node.prev != nullptr && node.next != nullptr);
        Unlink(&node);
        node.prev = nullptr;
        node.next = nullptr;
        --m_size;
    }

    /// @brief イテレータで指す要素をリストから外す
    iterator Erase(const_iterator pos) noexcept
    {
        IntrusiveListNode* node = const_cast<IntrusiveListNode*>(pos.GetNode());
        IntrusiveListNode* next = node->next;
        Unlink(node);
        node->prev = nullptr;
        node->next = nullptr;
        --m_size;
        return iterator(next);
    }

    /// @brief 全要素をリストから外す (オブジェクトは破棄しない)
    void Clear() noexcept
    {
        IntrusiveListNode* cur = m_anchor.next;
        while (cur != &m_anchor)
        {
            IntrusiveListNode* next = cur->next;
            cur->prev = nullptr;
            cur->next = nullptr;
            cur = next;
        }
        InitSentinel();
        m_size = 0;
    }

    // ========================================================================
    // Splice
    // ========================================================================

    /// @brief other の全要素を pos の前に移動
    void Splice(const_iterator pos, IntrusiveList& other) noexcept
    {
        if (other.Empty()) return;
        IntrusiveListNode* posNode = const_cast<IntrusiveListNode*>(pos.GetNode());
        IntrusiveListNode* first = other.m_anchor.next;
        IntrusiveListNode* last = other.m_anchor.prev;

        other.InitSentinel();
        size_type count = other.m_size;
        other.m_size = 0;

        first->prev = posNode->prev;
        last->next = posNode;
        posNode->prev->next = first;
        posNode->prev = last;
        m_size += count;
    }

    /// @brief other の it が指す要素1つを pos の前に移動
    void Splice(const_iterator pos, IntrusiveList& other, const_iterator it) noexcept
    {
        IntrusiveListNode* node = const_cast<IntrusiveListNode*>(it.GetNode());
        IntrusiveListNode* posNode = const_cast<IntrusiveListNode*>(pos.GetNode());
        if (node == posNode || node->next == posNode) return;

        Unlink(node);
        --other.m_size;

        LinkBefore(posNode, node);
        ++m_size;
    }

    // ========================================================================
    // List operations
    // ========================================================================

    /// @brief リストを反転
    void Reverse() noexcept
    {
        if (m_size <= 1) return;
        IntrusiveListNode* cur = &m_anchor;
        do
        {
            IntrusiveListNode* tmp = cur->next;
            cur->next = cur->prev;
            cur->prev = tmp;
            cur = tmp;
        } while (cur != &m_anchor);
    }

    /// @brief 安定ソート (マージソート)
    template <typename Compare>
    void Sort(Compare comp)
    {
        if (m_size <= 1) return;

        // リストをフラット配列に取り出してソートし、再リンク
        // 侵入型リストなのでノードを付け替えるだけ

        // ボトムアップ マージソート
        IntrusiveList carry;
        IntrusiveList buckets[64];
        int fill = 0;

        while (!Empty())
        {
            // 先頭1要素を carry に移動
            IntrusiveListNode* node = m_anchor.next;
            Unlink(node);
            --m_size;
            carry.LinkBefore(&carry.m_anchor, node);
            carry.m_size = 1;

            int i = 0;
            while (i < fill && !buckets[i].Empty())
            {
                buckets[i].MergeInternal(carry, comp);
                // carry ← buckets[i] の結果を carry に
                IntrusiveList tmp;
                tmp = std::move(buckets[i]);
                carry = std::move(tmp);
                ++i;
            }
            buckets[i] = std::move(carry);
            if (i == fill) ++fill;
        }

        for (int i = 1; i < fill; ++i)
            buckets[i].MergeInternal(buckets[i - 1], comp);

        if (fill > 0)
            *this = std::move(buckets[fill - 1]);
    }

    /// @brief デフォルト比較ソート
    void Sort()
    {
        Sort([](const T& a, const T& b) { return a < b; });
    }

    /// @brief 2つのソート済みリストをマージ
    template <typename Compare>
    void Merge(IntrusiveList& other, Compare comp)
    {
        MergeInternal(other, comp);
    }

    void Merge(IntrusiveList& other)
    {
        Merge(other, [](const T& a, const T& b) { return a < b; });
    }

    // ========================================================================
    // Validation
    // ========================================================================

    /// @brief 内部状態の整合性を検証
    bool Validate() const
    {
        size_type count = 0;
        const IntrusiveListNode* cur = m_anchor.next;
        while (cur != &m_anchor)
        {
            if (cur->prev->next != cur) return false;
            if (cur->next->prev != cur) return false;
            ++count;
            cur = cur->next;
        }
        return count == m_size;
    }

private:
    // ========================================================================
    // Internal helpers
    // ========================================================================

    void InitSentinel() noexcept
    {
        m_anchor.prev = &m_anchor;
        m_anchor.next = &m_anchor;
    }

    static void LinkBefore(IntrusiveListNode* before, IntrusiveListNode* node) noexcept
    {
        node->prev = before->prev;
        node->next = before;
        before->prev->next = node;
        before->prev = node;
    }

    static void Unlink(IntrusiveListNode* node) noexcept
    {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }

    /// @brief 内部マージ (other を this にマージ)
    template <typename Compare>
    void MergeInternal(IntrusiveList& other, Compare& comp)
    {
        if (this == &other || other.Empty()) return;
        if (Empty())
        {
            *this = std::move(other);
            return;
        }

        IntrusiveListNode* cur = m_anchor.next;
        IntrusiveListNode* otherCur = other.m_anchor.next;

        while (cur != &m_anchor && otherCur != &other.m_anchor)
        {
            if (comp(Traits::GetObject(*otherCur), Traits::GetObject(*cur)))
            {
                IntrusiveListNode* next = otherCur->next;
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
        if (otherCur != &other.m_anchor)
        {
            IntrusiveListNode* otherFirst = otherCur;
            IntrusiveListNode* otherLast = other.m_anchor.prev;

            otherFirst->prev = m_anchor.prev;
            otherLast->next = &m_anchor;
            m_anchor.prev->next = otherFirst;
            m_anchor.prev = otherLast;
        }

        m_size += other.m_size;
        other.InitSentinel();
        other.m_size = 0;
    }

    // ========================================================================
    // Data members
    // ========================================================================

    IntrusiveListNode m_anchor{};  ///< センチネルアンカー
    size_type         m_size = 0;  ///< 要素数
};

} // namespace gx::container
