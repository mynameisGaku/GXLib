#pragma once
/// @file RBTree.h
/// @brief 赤黒木 — 順序付き連想コンテナの内部実装

#include "pch_common.h"
#include "Container/Allocator.h"
#include "Container/CompressedPair.h"
#include "Container/Hash.h"
#include "Container/TypeTraits.h"
#include <cassert>
#include <iterator>
#include <new>
#include <type_traits>
#include <utility>

namespace gx::container {

// ============================================================================
// ノード色
// ============================================================================

enum class RBColor : uint8_t
{
    Red   = 0,
    Black = 1,
};

// ============================================================================
// ノード基底 (値を持たない — アンカー用)
// ============================================================================

struct RBNodeBase
{
    RBNodeBase* parent = nullptr;
    RBNodeBase* left   = nullptr;
    RBNodeBase* right  = nullptr;
    RBColor     color  = RBColor::Red;
};

// ============================================================================
// 値ノード
// ============================================================================

template <typename Value>
struct RBNode : public RBNodeBase
{
    union {
        Value value;
    };

    RBNode() noexcept {}
    ~RBNode() {} // value のデストラクタは明示的に呼ぶ
};

// ============================================================================
// ツリー走査ヘルパー
// ============================================================================

/// @brief 部分木の最小ノード (最左)
__forceinline RBNodeBase* RBTreeMinimum(RBNodeBase* node) noexcept
{
    while (node->left)
        node = node->left;
    return node;
}

__forceinline const RBNodeBase* RBTreeMinimum(const RBNodeBase* node) noexcept
{
    while (node->left)
        node = node->left;
    return node;
}

/// @brief 部分木の最大ノード (最右)
__forceinline RBNodeBase* RBTreeMaximum(RBNodeBase* node) noexcept
{
    while (node->right)
        node = node->right;
    return node;
}

__forceinline const RBNodeBase* RBTreeMaximum(const RBNodeBase* node) noexcept
{
    while (node->right)
        node = node->right;
    return node;
}

/// @brief 中順序の次ノード (in-order successor)
__forceinline RBNodeBase* RBTreeIncrement(RBNodeBase* node) noexcept
{
    if (node->right)
    {
        return RBTreeMinimum(node->right);
    }
    else
    {
        RBNodeBase* p = node->parent;
        while (node == p->right)
        {
            node = p;
            p = p->parent;
        }
        // アンカーノード処理: root の右部分木が空の場合、
        // p がアンカーになりうる
        if (node->right != p)
            node = p;
        return node;
    }
}

__forceinline const RBNodeBase* RBTreeIncrement(const RBNodeBase* node) noexcept
{
    return RBTreeIncrement(const_cast<RBNodeBase*>(node));
}

/// @brief 中順序の前ノード (in-order predecessor)
__forceinline RBNodeBase* RBTreeDecrement(RBNodeBase* node) noexcept
{
    // アンカー (end()) の場合: 最右ノードへ
    if (node->color == RBColor::Red && node->parent->parent == node)
    {
        return node->right; // anchor->right = rightmost
    }
    else if (node->left)
    {
        return RBTreeMaximum(node->left);
    }
    else
    {
        RBNodeBase* p = node->parent;
        while (node == p->left)
        {
            node = p;
            p = p->parent;
        }
        return p;
    }
}

__forceinline const RBNodeBase* RBTreeDecrement(const RBNodeBase* node) noexcept
{
    return RBTreeDecrement(const_cast<RBNodeBase*>(node));
}

// ============================================================================
// 回転
// ============================================================================

/// @brief 左回転
inline void RBTreeRotateLeft(RBNodeBase* x, RBNodeBase*& root) noexcept
{
    RBNodeBase* y = x->right;
    x->right = y->left;
    if (y->left)
        y->left->parent = x;
    y->parent = x->parent;

    if (x == root)
        root = y;
    else if (x == x->parent->left)
        x->parent->left = y;
    else
        x->parent->right = y;

    y->left = x;
    x->parent = y;
}

/// @brief 右回転
inline void RBTreeRotateRight(RBNodeBase* x, RBNodeBase*& root) noexcept
{
    RBNodeBase* y = x->left;
    x->left = y->right;
    if (y->right)
        y->right->parent = x;
    y->parent = x->parent;

    if (x == root)
        root = y;
    else if (x == x->parent->right)
        x->parent->right = y;
    else
        x->parent->left = y;

    y->right = x;
    x->parent = y;
}

// ============================================================================
// 挿入フィックスアップ
// ============================================================================

inline void RBTreeInsertFixup(RBNodeBase* node, RBNodeBase*& root) noexcept
{
    node->color = RBColor::Red;

    while (node != root && node->parent->color == RBColor::Red)
    {
        RBNodeBase* grandparent = node->parent->parent;

        if (node->parent == grandparent->left)
        {
            RBNodeBase* uncle = grandparent->right;

            if (uncle && uncle->color == RBColor::Red)
            {
                // Case 1: 叔父が赤
                node->parent->color = RBColor::Black;
                uncle->color = RBColor::Black;
                grandparent->color = RBColor::Red;
                node = grandparent;
            }
            else
            {
                if (node == node->parent->right)
                {
                    // Case 2: 三角形 → 左回転で直線化
                    node = node->parent;
                    RBTreeRotateLeft(node, root);
                }
                // Case 3: 直線 → 右回転 + 色変更
                node->parent->color = RBColor::Black;
                node->parent->parent->color = RBColor::Red;
                RBTreeRotateRight(node->parent->parent, root);
            }
        }
        else
        {
            // 対称 (親が右側)
            RBNodeBase* uncle = grandparent->left;

            if (uncle && uncle->color == RBColor::Red)
            {
                node->parent->color = RBColor::Black;
                uncle->color = RBColor::Black;
                grandparent->color = RBColor::Red;
                node = grandparent;
            }
            else
            {
                if (node == node->parent->left)
                {
                    node = node->parent;
                    RBTreeRotateRight(node, root);
                }
                node->parent->color = RBColor::Black;
                node->parent->parent->color = RBColor::Red;
                RBTreeRotateLeft(node->parent->parent, root);
            }
        }
    }

    root->color = RBColor::Black;
}

// ============================================================================
// 削除フィックスアップ
// ============================================================================

inline void RBTreeEraseFixup(RBNodeBase* node, RBNodeBase* parent, RBNodeBase*& root) noexcept
{
    while (node != root && (node == nullptr || node->color == RBColor::Black))
    {
        if (node == parent->left)
        {
            RBNodeBase* sibling = parent->right;

            if (sibling->color == RBColor::Red)
            {
                // Case 1: 兄弟が赤
                sibling->color = RBColor::Black;
                parent->color = RBColor::Red;
                RBTreeRotateLeft(parent, root);
                sibling = parent->right;
            }

            if ((sibling->left == nullptr || sibling->left->color == RBColor::Black) &&
                (sibling->right == nullptr || sibling->right->color == RBColor::Black))
            {
                // Case 2: 兄弟の両子が黒
                sibling->color = RBColor::Red;
                node = parent;
                parent = parent->parent;
            }
            else
            {
                if (sibling->right == nullptr || sibling->right->color == RBColor::Black)
                {
                    // Case 3: 兄弟の右子が黒
                    if (sibling->left)
                        sibling->left->color = RBColor::Black;
                    sibling->color = RBColor::Red;
                    RBTreeRotateRight(sibling, root);
                    sibling = parent->right;
                }
                // Case 4: 兄弟の右子が赤
                sibling->color = parent->color;
                parent->color = RBColor::Black;
                if (sibling->right)
                    sibling->right->color = RBColor::Black;
                RBTreeRotateLeft(parent, root);
                node = root; // 終了
            }
        }
        else
        {
            // 対称
            RBNodeBase* sibling = parent->left;

            if (sibling->color == RBColor::Red)
            {
                sibling->color = RBColor::Black;
                parent->color = RBColor::Red;
                RBTreeRotateRight(parent, root);
                sibling = parent->left;
            }

            if ((sibling->right == nullptr || sibling->right->color == RBColor::Black) &&
                (sibling->left == nullptr || sibling->left->color == RBColor::Black))
            {
                sibling->color = RBColor::Red;
                node = parent;
                parent = parent->parent;
            }
            else
            {
                if (sibling->left == nullptr || sibling->left->color == RBColor::Black)
                {
                    if (sibling->right)
                        sibling->right->color = RBColor::Black;
                    sibling->color = RBColor::Red;
                    RBTreeRotateLeft(sibling, root);
                    sibling = parent->left;
                }
                sibling->color = parent->color;
                parent->color = RBColor::Black;
                if (sibling->left)
                    sibling->left->color = RBColor::Black;
                RBTreeRotateRight(parent, root);
                node = root;
            }
        }
    }

    if (node)
        node->color = RBColor::Black;
}

// ============================================================================
// RBTree 本体
// ============================================================================

/// @brief 赤黒木 — 順序付き連想コンテナの内部実装
///
/// ノードベース。sentinel アンカーパターン使用:
/// - m_anchor.parent = root
/// - m_anchor.left   = leftmost (begin)
/// - m_anchor.right  = rightmost
///
/// @tparam Value      格納値型
/// @tparam Key        キー型
/// @tparam ExtractKey Value から Key を取り出すファンクタ
/// @tparam Compare    比較関数 (Less<Key> など)
/// @tparam Alloc      アロケータ型
/// @tparam bMultiKey  true で重複キー許可
template <typename Value, typename Key, typename ExtractKey,
          typename Compare, typename Alloc, bool bMultiKey>
class RBTree
{
public:
    // ====================================================================
    // 型エイリアス
    // ====================================================================
    using key_type       = Key;
    using value_type     = Value;
    using size_type      = size_t;
    using difference_type = ptrdiff_t;
    using key_compare    = Compare;
    using allocator_type = Alloc;
    using reference      = value_type&;
    using const_reference = const value_type&;
    using pointer        = value_type*;
    using const_pointer  = const value_type*;

private:
    using Node = RBNode<Value>;

public:
    // ====================================================================
    // イテレータ
    // ====================================================================

    /// @brief 双方向イテレータ (中順序走査)
    template <typename ValueRef, typename NodeBasePtr>
    class IteratorImpl
    {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type        = Value;
        using difference_type   = ptrdiff_t;
        using pointer           = ValueRef*;
        using reference         = ValueRef&;

        IteratorImpl() noexcept = default;
        explicit IteratorImpl(NodeBasePtr node) noexcept : m_node(node) {}

        __forceinline reference operator*()  const noexcept
        {
            return static_cast<typename std::conditional_t<
                std::is_const_v<ValueRef>, const Node*, Node*>>(m_node)->value;
        }

        __forceinline pointer operator->() const noexcept
        {
            return &(static_cast<typename std::conditional_t<
                std::is_const_v<ValueRef>, const Node*, Node*>>(m_node)->value);
        }

        __forceinline IteratorImpl& operator++() noexcept
        {
            m_node = const_cast<RBNodeBase*>(RBTreeIncrement(static_cast<const RBNodeBase*>(m_node)));
            return *this;
        }

        __forceinline IteratorImpl operator++(int) noexcept
        {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        __forceinline IteratorImpl& operator--() noexcept
        {
            m_node = const_cast<RBNodeBase*>(RBTreeDecrement(static_cast<const RBNodeBase*>(m_node)));
            return *this;
        }

        __forceinline IteratorImpl operator--(int) noexcept
        {
            auto tmp = *this;
            --(*this);
            return tmp;
        }

        __forceinline friend bool operator==(const IteratorImpl& a, const IteratorImpl& b) noexcept
        {
            return a.m_node == b.m_node;
        }

        __forceinline friend bool operator!=(const IteratorImpl& a, const IteratorImpl& b) noexcept
        {
            return a.m_node != b.m_node;
        }

        // const_iterator への暗黙変換
        operator IteratorImpl<const Value, const RBNodeBase*>() const noexcept
        {
            return IteratorImpl<const Value, const RBNodeBase*>(m_node);
        }

        NodeBasePtr GetNode() const noexcept { return m_node; }

    private:
        friend class RBTree;
        NodeBasePtr m_node = nullptr;
    };

    using iterator       = IteratorImpl<Value, RBNodeBase*>;
    using const_iterator = IteratorImpl<const Value, const RBNodeBase*>;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    // ====================================================================
    // insert 戻り値
    // ====================================================================
    struct InsertResult
    {
        iterator first;
        bool     second; // true = 挿入された
    };

    // ====================================================================
    // コンストラクタ / デストラクタ
    // ====================================================================

    RBTree()
        : m_compare(), m_allocPair(Alloc(), size_type(0))
    {
        InitAnchor();
    }

    explicit RBTree(const Compare& comp, const Alloc& alloc = Alloc())
        : m_compare(comp), m_allocPair(alloc, size_type(0))
    {
        InitAnchor();
    }

    RBTree(const RBTree& other)
        : m_compare(other.m_compare),
          m_allocPair(other.GetAllocator(), size_type(0))
    {
        InitAnchor();
        if (other.GetRoot())
        {
            SetRoot(CopySubtree(static_cast<const Node*>(other.GetRoot()), &m_anchor));
            GetRoot()->color = RBColor::Black;
            m_anchor.left  = RBTreeMinimum(GetRoot());
            m_anchor.right = RBTreeMaximum(GetRoot());
            m_allocPair.Second() = other.m_allocPair.Second();
        }
    }

    RBTree(RBTree&& other) noexcept
        : m_compare(std::move(other.m_compare)),
          m_allocPair(std::move(other.m_allocPair))
    {
        InitAnchor();
        if (other.GetRoot())
        {
            // ルートを引き取る
            SetRoot(other.GetRoot());
            GetRoot()->parent = &m_anchor;
            m_anchor.left  = other.m_anchor.left;
            m_anchor.right = other.m_anchor.right;

            other.InitAnchor();
            other.m_allocPair.Second() = 0;
        }
    }

    ~RBTree()
    {
        Clear();
    }

    RBTree& operator=(const RBTree& other)
    {
        if (this != &other)
        {
            RBTree tmp(other);
            Swap(tmp);
        }
        return *this;
    }

    RBTree& operator=(RBTree&& other) noexcept
    {
        if (this != &other)
        {
            Clear();
            m_compare = std::move(other.m_compare);

            if (other.GetRoot())
            {
                SetRoot(other.GetRoot());
                GetRoot()->parent = &m_anchor;
                m_anchor.left  = other.m_anchor.left;
                m_anchor.right = other.m_anchor.right;
                m_allocPair = std::move(other.m_allocPair);

                other.InitAnchor();
                other.m_allocPair.Second() = 0;
            }
            else
            {
                InitAnchor();
                m_allocPair.Second() = 0;
            }
        }
        return *this;
    }

    // ====================================================================
    // 容量
    // ====================================================================

    __forceinline size_type Size()  const noexcept { return m_allocPair.Second(); }
    __forceinline bool      Empty() const noexcept { return m_allocPair.Second() == 0; }

    // ====================================================================
    // イテレータ
    // ====================================================================

    __forceinline iterator begin() noexcept
    {
        return iterator(m_anchor.left);
    }

    __forceinline iterator end() noexcept
    {
        return iterator(&m_anchor);
    }

    __forceinline const_iterator begin() const noexcept
    {
        return const_iterator(m_anchor.left);
    }

    __forceinline const_iterator end() const noexcept
    {
        return const_iterator(&m_anchor);
    }

    __forceinline const_iterator cbegin() const noexcept { return begin(); }
    __forceinline const_iterator cend()   const noexcept { return end(); }

    __forceinline reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
    __forceinline reverse_iterator rend()   noexcept { return reverse_iterator(begin()); }
    __forceinline const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
    __forceinline const_reverse_iterator rend()   const noexcept { return const_reverse_iterator(begin()); }

    // ====================================================================
    // 検索
    // ====================================================================

    /// @brief キーで検索 (最初の一致)
    iterator Find(const key_type& key) noexcept
    {
        RBNodeBase* node = GetRoot();
        RBNodeBase* result = &m_anchor; // end()

        while (node)
        {
            const key_type& nodeKey = ExtractKey()(static_cast<Node*>(node)->value);
            if (m_compare(key, nodeKey))
            {
                node = node->left;
            }
            else if (m_compare(nodeKey, key))
            {
                node = node->right;
            }
            else
            {
                // 一致 — bMultiKey なら最初の一致を探す
                result = node;
                if constexpr (bMultiKey)
                    node = node->left; // さらに左に同一キーがあるかもしれない
                else
                    return iterator(node);
            }
        }

        return iterator(result);
    }

    const_iterator Find(const key_type& key) const noexcept
    {
        return const_cast<RBTree*>(this)->Find(key);
    }

    /// @brief キーの存在判定
    __forceinline bool Contains(const key_type& key) const noexcept
    {
        return Find(key) != end();
    }

    /// @brief キーの出現数
    size_type Count(const key_type& key) const noexcept
    {
        if constexpr (!bMultiKey)
        {
            return Contains(key) ? 1 : 0;
        }
        else
        {
            auto [first, last] = EqualRange(key);
            size_type n = 0;
            for (auto it = first; it != last; ++it)
                ++n;
            return n;
        }
    }

    /// @brief key 以上の最初の要素
    iterator LowerBound(const key_type& key) noexcept
    {
        RBNodeBase* node = GetRoot();
        RBNodeBase* result = &m_anchor;

        while (node)
        {
            if (!m_compare(ExtractKey()(static_cast<Node*>(node)->value), key))
            {
                result = node;
                node = node->left;
            }
            else
            {
                node = node->right;
            }
        }

        return iterator(result);
    }

    const_iterator LowerBound(const key_type& key) const noexcept
    {
        return const_cast<RBTree*>(this)->LowerBound(key);
    }

    /// @brief key より大きい最初の要素
    iterator UpperBound(const key_type& key) noexcept
    {
        RBNodeBase* node = GetRoot();
        RBNodeBase* result = &m_anchor;

        while (node)
        {
            if (m_compare(key, ExtractKey()(static_cast<Node*>(node)->value)))
            {
                result = node;
                node = node->left;
            }
            else
            {
                node = node->right;
            }
        }

        return iterator(result);
    }

    const_iterator UpperBound(const key_type& key) const noexcept
    {
        return const_cast<RBTree*>(this)->UpperBound(key);
    }

    /// @brief 等範囲 [lower_bound, upper_bound)
    std::pair<iterator, iterator> EqualRange(const key_type& key) noexcept
    {
        return { LowerBound(key), UpperBound(key) };
    }

    std::pair<const_iterator, const_iterator> EqualRange(const key_type& key) const noexcept
    {
        return { LowerBound(key), UpperBound(key) };
    }

    // ====================================================================
    // 挿入
    // ====================================================================

    /// @brief 値をコピー挿入
    InsertResult Insert(const value_type& value)
    {
        return InsertValue(value);
    }

    /// @brief 値をムーブ挿入
    InsertResult Insert(value_type&& value)
    {
        return InsertValue(std::move(value));
    }

    /// @brief in-place 構築挿入
    template <typename... Args>
    InsertResult Emplace(Args&&... args)
    {
        Node* newNode = AllocateNode();
        ::new (static_cast<void*>(&newNode->value)) value_type(std::forward<Args>(args)...);

        auto result = InsertNode(newNode);
        if (!result.second)
        {
            // 重複で挿入されなかった場合は解放
            newNode->value.~value_type();
            DeallocateNode(newNode);
        }
        return result;
    }

    // ====================================================================
    // 削除
    // ====================================================================

    /// @brief イテレータで指定した要素を削除
    iterator Erase(const_iterator pos)
    {
        assert(pos != end());
        RBNodeBase* nodeBase = const_cast<RBNodeBase*>(pos.GetNode());
        Node* node = static_cast<Node*>(nodeBase);

        // 次のイテレータを先に取得
        const_iterator nextIt = pos;
        ++nextIt;

        EraseNode(nodeBase);

        node->value.~value_type();
        DeallocateNode(node);

        return iterator(const_cast<RBNodeBase*>(nextIt.GetNode()));
    }

    /// @brief イテレータ範囲の削除
    iterator Erase(const_iterator first, const_iterator last)
    {
        if (first == begin() && last == end())
        {
            Clear();
            return end();
        }
        while (first != last)
            first = Erase(first);
        return iterator(const_cast<RBNodeBase*>(last.GetNode()));
    }

    /// @brief キーで削除 (最初の一致のみ、或いは全一致)
    size_type Erase(const key_type& key)
    {
        if constexpr (!bMultiKey)
        {
            iterator it = Find(key);
            if (it == end())
                return 0;
            Erase(it);
            return 1;
        }
        else
        {
            auto [first, last] = EqualRange(key);
            size_type n = 0;
            while (first != last)
            {
                first = Erase(first);
                ++n;
            }
            return n;
        }
    }

    // ====================================================================
    // クリア
    // ====================================================================

    void Clear() noexcept
    {
        if (GetRoot())
        {
            DestroySubtree(static_cast<Node*>(GetRoot()));
            InitAnchor();
            m_allocPair.Second() = 0;
        }
    }

    // ====================================================================
    // スワップ
    // ====================================================================

    void Swap(RBTree& other) noexcept
    {
        using std::swap;

        // 両方空
        if (Empty() && other.Empty())
        {
            swap(m_compare, other.m_compare);
            return;
        }

        // ルートの parent ポインタはアンカーを指すので、
        // スワップ後に付け替えが必要
        swap(m_anchor.parent, other.m_anchor.parent);
        swap(m_anchor.left,   other.m_anchor.left);
        swap(m_anchor.right,  other.m_anchor.right);
        swap(m_anchor.color,  other.m_anchor.color);
        swap(m_compare,       other.m_compare);
        swap(m_allocPair,     other.m_allocPair);

        // root の parent をそれぞれのアンカーに再設定
        if (GetRoot())
            GetRoot()->parent = &m_anchor;
        else
            m_anchor.left = m_anchor.right = &m_anchor;

        if (other.GetRoot())
            other.GetRoot()->parent = &other.m_anchor;
        else
            other.m_anchor.left = other.m_anchor.right = &other.m_anchor;
    }

    // ====================================================================
    // アクセサ
    // ====================================================================

    __forceinline key_compare    KeyComp()      const noexcept { return m_compare; }
    __forceinline allocator_type GetAllocator() const noexcept { return m_allocPair.First(); }

    // ====================================================================
    // std compatibility aliases
    // ====================================================================

    __forceinline size_type size() const noexcept { return Size(); }
    __forceinline bool empty() const noexcept { return Empty(); }
    __forceinline iterator find(const key_type& key) noexcept { return Find(key); }
    __forceinline const_iterator find(const key_type& key) const noexcept { return Find(key); }
    __forceinline bool contains(const key_type& key) const noexcept { return Contains(key); }
    __forceinline size_type count(const key_type& key) const noexcept { return Count(key); }
    InsertResult insert(const value_type& value) { return Insert(value); }
    InsertResult insert(value_type&& value) { return Insert(std::move(value)); }
    template <typename... Args>
    InsertResult emplace(Args&&... args) { return Emplace(std::forward<Args>(args)...); }
    iterator erase(const_iterator pos) { return Erase(pos); }
    iterator erase(const_iterator first, const_iterator last) { return Erase(first, last); }
    size_type erase(const key_type& key) { return Erase(key); }
    void clear() noexcept { Clear(); }
    void swap(RBTree& other) noexcept { Swap(other); }
    iterator lower_bound(const key_type& key) noexcept { return LowerBound(key); }
    const_iterator lower_bound(const key_type& key) const noexcept { return LowerBound(key); }
    iterator upper_bound(const key_type& key) noexcept { return UpperBound(key); }
    const_iterator upper_bound(const key_type& key) const noexcept { return UpperBound(key); }
    std::pair<iterator, iterator> equal_range(const key_type& key) noexcept { return EqualRange(key); }
    std::pair<const_iterator, const_iterator> equal_range(const key_type& key) const noexcept { return EqualRange(key); }

    // ====================================================================
    // 検証
    // ====================================================================

    /// @brief RB木の不変量を検証 (デバッグ用)
    bool Validate() const noexcept
    {
        if (Empty())
        {
            return GetRoot() == nullptr &&
                   m_anchor.left == &m_anchor &&
                   m_anchor.right == &m_anchor;
        }

        // 1. root は黒
        if (GetRoot()->color != RBColor::Black)
            return false;

        // 2. root の parent はアンカー
        if (GetRoot()->parent != &m_anchor)
            return false;

        // 3. 全パスの黒ノード数が同一
        int blackCount = -1;
        size_type nodeCount = 0;
        if (!ValidateSubtree(GetRoot(), 0, blackCount, nodeCount))
            return false;

        // 4. ノード数一致
        if (nodeCount != Size())
            return false;

        // 5. leftmost / rightmost
        if (m_anchor.left != RBTreeMinimum(GetRoot()))
            return false;
        if (m_anchor.right != RBTreeMaximum(GetRoot()))
            return false;

        return true;
    }

    // ====================================================================
    // 比較演算子
    // ====================================================================

    friend bool operator==(const RBTree& a, const RBTree& b) noexcept
    {
        if (a.Size() != b.Size())
            return false;
        auto itA = a.begin();
        auto itB = b.begin();
        for (; itA != a.end(); ++itA, ++itB)
        {
            // 値の完全比較 (comp で < を使って == を判定)
            if (a.m_compare(ExtractKey()(*itA), ExtractKey()(*itB)) ||
                a.m_compare(ExtractKey()(*itB), ExtractKey()(*itA)))
                return false;
        }
        return true;
    }

    friend bool operator!=(const RBTree& a, const RBTree& b) noexcept
    {
        return !(a == b);
    }

protected:
    // ====================================================================
    // ルートアクセス
    // ====================================================================

    __forceinline RBNodeBase* GetRoot() noexcept { return m_anchor.parent; }
    __forceinline const RBNodeBase* GetRoot() const noexcept { return m_anchor.parent; }

    __forceinline void SetRoot(RBNodeBase* node) noexcept
    {
        m_anchor.parent = node;
    }

    // ====================================================================
    // アンカー初期化
    // ====================================================================

    void InitAnchor() noexcept
    {
        m_anchor.parent = nullptr;
        m_anchor.left   = &m_anchor;  // begin() == end()
        m_anchor.right  = &m_anchor;
        m_anchor.color  = RBColor::Red; // アンカーは赤 (end() の判別用)
    }

    // ====================================================================
    // ノードメモリ管理
    // ====================================================================

    Node* AllocateNode()
    {
        void* mem = m_allocPair.First().allocate(sizeof(Node), alignof(Node), 0, 0);
        Node* node = static_cast<Node*>(mem);
        node->parent = nullptr;
        node->left   = nullptr;
        node->right  = nullptr;
        node->color  = RBColor::Red;
        return node;
    }

    void DeallocateNode(Node* node)
    {
        m_allocPair.First().deallocate(node, sizeof(Node));
    }

    // ====================================================================
    // 挿入内部
    // ====================================================================

    template <typename V>
    InsertResult InsertValue(V&& value)
    {
        Node* newNode = AllocateNode();
        ::new (static_cast<void*>(&newNode->value)) value_type(std::forward<V>(value));

        auto result = InsertNode(newNode);
        if (!result.second)
        {
            newNode->value.~value_type();
            DeallocateNode(newNode);
        }
        return result;
    }

    InsertResult InsertNode(Node* newNode)
    {
        const key_type& key = ExtractKey()(newNode->value);
        RBNodeBase* parent = &m_anchor;
        RBNodeBase* current = GetRoot();
        bool insertLeft = true;

        while (current)
        {
            parent = current;
            const key_type& curKey = ExtractKey()(static_cast<Node*>(current)->value);

            if (m_compare(key, curKey))
            {
                current = current->left;
                insertLeft = true;
            }
            else if constexpr (!bMultiKey)
            {
                if (!m_compare(curKey, key))
                {
                    // キーが等しい → 重複
                    return { iterator(current), false };
                }
                current = current->right;
                insertLeft = false;
            }
            else
            {
                // bMultiKey: 等しいキーは右に配置 (安定な挿入順序)
                current = current->right;
                insertLeft = false;
            }
        }

        // 新ノードをリンク
        newNode->parent = parent;
        newNode->left   = nullptr;
        newNode->right  = nullptr;

        if (parent == &m_anchor)
        {
            // 空の木
            SetRoot(newNode);
            m_anchor.left  = newNode;
            m_anchor.right = newNode;
        }
        else if (insertLeft)
        {
            parent->left = newNode;
            if (parent == m_anchor.left)
                m_anchor.left = newNode; // leftmost 更新
        }
        else
        {
            parent->right = newNode;
            if (parent == m_anchor.right)
                m_anchor.right = newNode; // rightmost 更新
        }

        // root 参照を取得してフィックスアップ
        RBNodeBase*& rootRef = m_anchor.parent;
        RBTreeInsertFixup(newNode, rootRef);

        ++m_allocPair.Second();
        return { iterator(newNode), true };
    }

    // ====================================================================
    // 削除内部
    // ====================================================================

    void EraseNode(RBNodeBase* z)
    {
        RBNodeBase*& rootRef = m_anchor.parent;
        RBNodeBase* y = z;
        RBNodeBase* x = nullptr;
        RBNodeBase* xParent = nullptr;
        RBColor yOrigColor = y->color;

        if (z->left == nullptr)
        {
            x = z->right;
            xParent = z->parent;
            Transplant(z, z->right);
        }
        else if (z->right == nullptr)
        {
            x = z->left;
            xParent = z->parent;
            Transplant(z, z->left);
        }
        else
        {
            // 後続ノード (in-order successor)
            y = RBTreeMinimum(z->right);
            yOrigColor = y->color;
            x = y->right;

            if (y->parent == z)
            {
                xParent = y;
            }
            else
            {
                xParent = y->parent;
                Transplant(y, y->right);
                y->right = z->right;
                y->right->parent = y;
            }

            Transplant(z, y);
            y->left = z->left;
            y->left->parent = y;
            y->color = z->color;
        }

        // leftmost / rightmost の更新
        if (m_anchor.left == z)
        {
            if (z->right)
                m_anchor.left = RBTreeMinimum(z->right);
            else
                m_anchor.left = z->parent;
            // アンカーに戻ったら空
            if (m_anchor.left == nullptr)
                m_anchor.left = &m_anchor;
        }

        if (m_anchor.right == z)
        {
            if (z->left)
                m_anchor.right = RBTreeMaximum(z->left);
            else
                m_anchor.right = z->parent;
            if (m_anchor.right == nullptr)
                m_anchor.right = &m_anchor;
        }

        // フィックスアップ
        if (yOrigColor == RBColor::Black)
        {
            if (rootRef)
                RBTreeEraseFixup(x, xParent, rootRef);
        }

        --m_allocPair.Second();

        // 空になった場合
        if (m_allocPair.Second() == 0)
        {
            InitAnchor();
        }
    }

    /// @brief ノード u を v で置き換え (子のリンクは更新しない)
    void Transplant(RBNodeBase* u, RBNodeBase* v) noexcept
    {
        if (u->parent == &m_anchor)
        {
            SetRoot(v);
        }
        else if (u == u->parent->left)
        {
            u->parent->left = v;
        }
        else
        {
            u->parent->right = v;
        }

        if (v)
            v->parent = u->parent;
    }

    // ====================================================================
    // サブツリー操作
    // ====================================================================

    /// @brief サブツリーを再帰的にコピー
    Node* CopySubtree(const Node* src, RBNodeBase* parent)
    {
        if (!src)
            return nullptr;

        Node* dst = AllocateNode();
        ::new (static_cast<void*>(&dst->value)) value_type(src->value);
        dst->color  = src->color;
        dst->parent = parent;

        dst->left  = CopySubtree(static_cast<const Node*>(src->left), dst);
        dst->right = CopySubtree(static_cast<const Node*>(src->right), dst);

        return dst;
    }

    /// @brief サブツリーを再帰的に破棄
    void DestroySubtree(Node* node) noexcept
    {
        if (!node)
            return;
        DestroySubtree(static_cast<Node*>(node->left));
        DestroySubtree(static_cast<Node*>(node->right));
        node->value.~value_type();
        DeallocateNode(node);
    }

    // ====================================================================
    // 検証ヘルパー
    // ====================================================================

    bool ValidateSubtree(const RBNodeBase* node, int currentBlack,
                         int& expectedBlack, size_type& count) const noexcept
    {
        if (!node)
        {
            // null リーフ = 黒
            if (expectedBlack < 0)
                expectedBlack = currentBlack;
            return currentBlack == expectedBlack;
        }

        // 赤ノードの子は黒でなければならない
        if (node->color == RBColor::Red)
        {
            if ((node->left && node->left->color == RBColor::Red) ||
                (node->right && node->right->color == RBColor::Red))
                return false;
        }

        if (node->color == RBColor::Black)
            ++currentBlack;

        ++count;

        // 親リンクの整合性
        if (node->left && node->left->parent != node)
            return false;
        if (node->right && node->right->parent != node)
            return false;

        return ValidateSubtree(node->left, currentBlack, expectedBlack, count) &&
               ValidateSubtree(node->right, currentBlack, expectedBlack, count);
    }

    // ====================================================================
    // メンバ
    // ====================================================================

    mutable RBNodeBase m_anchor{};
    Compare m_compare{};
    CompressedPair<Alloc, size_type> m_allocPair;
};

} // namespace gx::container
