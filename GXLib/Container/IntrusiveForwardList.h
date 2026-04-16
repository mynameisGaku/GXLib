#pragma once
/// @file IntrusiveForwardList.h
/// @brief ゼロアロケーション侵入型単方向連結リスト

#include "pch_common.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <type_traits>

namespace gx::container {

/// @brief 侵入型前方リストノード基底
/// @details ユーザー型がこの構造体をメンバーとして持ち、IntrusiveForwardList に登録する。
struct IntrusiveForwardListNode
{
    IntrusiveForwardListNode* next = nullptr;
};

/// @brief ポインタ-to-メンバーからオブジェクトを取得するヘルパー
template <typename T, IntrusiveForwardListNode T::*NodeMember>
struct IntrusiveForwardListTraits
{
    /// @brief オブジェクトからノードを取得
    __forceinline static IntrusiveForwardListNode& GetNode(T& obj) noexcept
    {
        return obj.*NodeMember;
    }

    /// @brief ノードからオブジェクトを取得
    __forceinline static T& GetObject(IntrusiveForwardListNode& node) noexcept
    {
        static const auto offset = reinterpret_cast<ptrdiff_t>(
            &(reinterpret_cast<T*>(0)->*NodeMember));
        return *reinterpret_cast<T*>(reinterpret_cast<char*>(&node) - offset);
    }

    __forceinline static const T& GetObject(const IntrusiveForwardListNode& node) noexcept
    {
        static const auto offset = reinterpret_cast<ptrdiff_t>(
            &(reinterpret_cast<const T*>(0)->*NodeMember));
        return *reinterpret_cast<const T*>(reinterpret_cast<const char*>(&node) - offset);
    }
};

/// @brief ゼロアロケーション侵入型単方向連結リスト
/// @details ノードはオブジェクト自身に埋め込まれ、メモリの割り当て/解放は行わない。
/// @tparam T オブジェクト型
/// @tparam NodeMember IntrusiveForwardListNode T::* メンバーポインタ
template <typename T, IntrusiveForwardListNode T::*NodeMember>
class IntrusiveForwardList
{
    using Traits = IntrusiveForwardListTraits<T, NodeMember>;

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
        using iterator_category = std::forward_iterator_tag;
        using value_type        = ValueType;
        using difference_type   = ptrdiff_t;
        using pointer           = ValueType*;
        using reference         = ValueType&;

        IteratorBase() noexcept = default;
        explicit IteratorBase(NodePtr node) noexcept : m_node(node) {}

        __forceinline reference operator*() const noexcept
        {
            return Traits::GetObject(*m_node);
        }

        __forceinline pointer operator->() const noexcept { return &(**this); }

        __forceinline IteratorBase& operator++() noexcept { m_node = m_node->next; return *this; }
        __forceinline IteratorBase  operator++(int) noexcept { auto tmp = *this; m_node = m_node->next; return tmp; }

        __forceinline friend bool operator==(const IteratorBase& a, const IteratorBase& b) noexcept { return a.m_node == b.m_node; }
        __forceinline friend bool operator!=(const IteratorBase& a, const IteratorBase& b) noexcept { return a.m_node != b.m_node; }

        NodePtr GetNode() const noexcept { return m_node; }

        /// @brief const_iterator への暗黙変換
        operator IteratorBase<const T, const IntrusiveForwardListNode*>() const noexcept
            requires (!std::is_const_v<ValueType>)
        {
            return IteratorBase<const T, const IntrusiveForwardListNode*>(m_node);
        }

    private:
        NodePtr m_node = nullptr;
    };

    using iterator       = IteratorBase<T, IntrusiveForwardListNode*>;
    using const_iterator = IteratorBase<const T, const IntrusiveForwardListNode*>;

    // ========================================================================
    // Constructors / Destructor
    // ========================================================================

    /// @brief デフォルトコンストラクタ (空リスト)
    IntrusiveForwardList() noexcept = default;

    /// @brief コピー禁止
    IntrusiveForwardList(const IntrusiveForwardList&) = delete;
    IntrusiveForwardList& operator=(const IntrusiveForwardList&) = delete;

    /// @brief ムーブコンストラクタ
    IntrusiveForwardList(IntrusiveForwardList&& other) noexcept
        : m_beforeHead(other.m_beforeHead)
        , m_size(other.m_size)
    {
        other.m_beforeHead.next = nullptr;
        other.m_size = 0;
    }

    /// @brief ムーブ代入
    IntrusiveForwardList& operator=(IntrusiveForwardList&& other) noexcept
    {
        if (this != &other)
        {
            Clear();
            m_beforeHead.next = other.m_beforeHead.next;
            m_size = other.m_size;
            other.m_beforeHead.next = nullptr;
            other.m_size = 0;
        }
        return *this;
    }

    /// @brief デストラクタ (リンクを外すだけ)
    ~IntrusiveForwardList()
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
        return Traits::GetObject(*m_beforeHead.next);
    }

    __forceinline const_reference Front() const noexcept
    {
        assert(!Empty());
        return Traits::GetObject(*m_beforeHead.next);
    }

    // ========================================================================
    // Iterators
    // ========================================================================

    /// @brief 先頭の前のイテレータ (insert_after/erase_after 用)
    __forceinline iterator BeforeBegin() noexcept
    {
        return iterator(&m_beforeHead);
    }

    __forceinline const_iterator BeforeBegin() const noexcept
    {
        return const_iterator(&m_beforeHead);
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

    __forceinline size_type Size() const noexcept { return m_size; }
    __forceinline bool Empty() const noexcept { return m_size == 0; }

    // ========================================================================
    // Modifiers
    // ========================================================================

    /// @brief 先頭にオブジェクトを追加
    void PushFront(T& obj) noexcept
    {
        IntrusiveForwardListNode& node = Traits::GetNode(obj);
        assert(node.next == nullptr);
        node.next = m_beforeHead.next;
        m_beforeHead.next = &node;
        ++m_size;
    }

    /// @brief 先頭要素をリストから外す
    void PopFront() noexcept
    {
        assert(!Empty());
        IntrusiveForwardListNode* head = m_beforeHead.next;
        m_beforeHead.next = head->next;
        head->next = nullptr;
        --m_size;
    }

    /// @brief pos の次にオブジェクトを挿入
    iterator InsertAfter(const_iterator pos, T& obj) noexcept
    {
        IntrusiveForwardListNode* posNode = const_cast<IntrusiveForwardListNode*>(pos.GetNode());
        IntrusiveForwardListNode& node = Traits::GetNode(obj);
        assert(node.next == nullptr);
        node.next = posNode->next;
        posNode->next = &node;
        ++m_size;
        return iterator(&node);
    }

    /// @brief pos の次の要素をリストから外す
    iterator EraseAfter(const_iterator pos) noexcept
    {
        IntrusiveForwardListNode* posNode = const_cast<IntrusiveForwardListNode*>(pos.GetNode());
        assert(posNode->next != nullptr);
        IntrusiveForwardListNode* target = posNode->next;
        posNode->next = target->next;
        target->next = nullptr;
        --m_size;
        return iterator(posNode->next);
    }

    /// @brief 指定オブジェクトをリストから外す (O(n) — 前ノードを検索)
    void Remove(T& obj) noexcept
    {
        IntrusiveForwardListNode& node = Traits::GetNode(obj);
        IntrusiveForwardListNode* prev = &m_beforeHead;
        while (prev->next != nullptr)
        {
            if (prev->next == &node)
            {
                prev->next = node.next;
                node.next = nullptr;
                --m_size;
                return;
            }
            prev = prev->next;
        }
        assert(false && "Object not found in list");
    }

    /// @brief 全要素をリストから外す (オブジェクトは破棄しない)
    void Clear() noexcept
    {
        IntrusiveForwardListNode* cur = m_beforeHead.next;
        while (cur)
        {
            IntrusiveForwardListNode* next = cur->next;
            cur->next = nullptr;
            cur = next;
        }
        m_beforeHead.next = nullptr;
        m_size = 0;
    }

    // ========================================================================
    // List operations
    // ========================================================================

    /// @brief リストを反転
    void Reverse() noexcept
    {
        IntrusiveForwardListNode* prev = nullptr;
        IntrusiveForwardListNode* cur = m_beforeHead.next;
        while (cur)
        {
            IntrusiveForwardListNode* next = cur->next;
            cur->next = prev;
            prev = cur;
            cur = next;
        }
        m_beforeHead.next = prev;
    }

    /// @brief 述語に一致する全要素を削除
    template <typename Predicate>
    size_type RemoveIf(Predicate pred)
    {
        size_type removed = 0;
        IntrusiveForwardListNode* prev = &m_beforeHead;
        while (prev->next)
        {
            T& obj = Traits::GetObject(*prev->next);
            if (pred(obj))
            {
                IntrusiveForwardListNode* target = prev->next;
                prev->next = target->next;
                target->next = nullptr;
                --m_size;
                ++removed;
            }
            else
            {
                prev = prev->next;
            }
        }
        return removed;
    }

    // ========================================================================
    // Validation
    // ========================================================================

    /// @brief 内部状態の整合性を検証
    bool Validate() const
    {
        size_type count = 0;
        const IntrusiveForwardListNode* cur = m_beforeHead.next;
        while (cur)
        {
            ++count;
            cur = cur->next;
        }
        return count == m_size;
    }

private:
    // ========================================================================
    // Data members
    // ========================================================================

    IntrusiveForwardListNode m_beforeHead{};  ///< before_begin センチネル
    size_type                m_size = 0;       ///< 要素数
};

} // namespace gx::container
