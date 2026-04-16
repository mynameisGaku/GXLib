#pragma once
/// @file Stack.h
/// @brief LIFO スタック アダプタ

#include "pch_common.h"
#include "Container/Deque.h"
#include <cstddef>
#include <utility>

namespace gx::container {

/// @brief LIFO スタック (コンテナアダプタ)
/// @details デフォルトの基底コンテナは Deque<T>。PushBack/PopBack/Back を持つ
///          任意のコンテナを基底として使用可能。
/// @tparam T 要素型
/// @tparam Container 基底コンテナ型 (デフォルト: Deque<T>)
template <typename T, typename Container = Deque<T>>
class Stack
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
    Stack() = default;

    /// @brief コンテナを指定して構築
    explicit Stack(const Container& container)
        : m_container(container) {}

    /// @brief コンテナをムーブして構築
    explicit Stack(Container&& container)
        : m_container(std::move(container)) {}

    // ========================================================================
    // Element access
    // ========================================================================

    /// @brief スタックの先頭 (最後に追加した要素)
    __forceinline reference Top() { return m_container.Back(); }

    /// @brief スタックの先頭 (const)
    __forceinline const_reference Top() const { return m_container.Back(); }

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

    /// @brief 要素をプッシュ (コピー)
    void Push(const T& value) { m_container.PushBack(value); }

    /// @brief 要素をプッシュ (ムーブ)
    void Push(T&& value) { m_container.PushBack(std::move(value)); }

    /// @brief 要素を直接構築してプッシュ
    template <typename... Args>
    decltype(auto) Emplace(Args&&... args)
    {
        return m_container.EmplaceBack(std::forward<Args>(args)...);
    }

    /// @brief 先頭要素をポップ
    void Pop() { m_container.PopBack(); }

    /// @brief 全要素を削除
    void Clear() { m_container.Clear(); }

    // ========================================================================
    // Container access
    // ========================================================================

    /// @brief 基底コンテナへのアクセス
    Container& GetContainer() { return m_container; }
    const Container& GetContainer() const { return m_container; }

    // ========================================================================
    // std compatibility aliases
    // ========================================================================

    __forceinline size_type size() const { return Size(); }
    __forceinline bool empty() const { return Empty(); }
    __forceinline reference top() { return Top(); }
    __forceinline const_reference top() const { return Top(); }
    void push(const T& value) { Push(value); }
    void push(T&& value) { Push(std::move(value)); }
    template <typename... Args> decltype(auto) emplace(Args&&... args) { return Emplace(std::forward<Args>(args)...); }
    void pop() { Pop(); }

    // ========================================================================
    // Validation
    // ========================================================================

    /// @brief 基底コンテナの検証を委譲
    bool Validate() const { return m_container.Validate(); }

    // ========================================================================
    // Comparison
    // ========================================================================

    friend bool operator==(const Stack& a, const Stack& b) { return a.m_container == b.m_container; }
    friend bool operator!=(const Stack& a, const Stack& b) { return !(a == b); }

private:
    Container m_container;  ///< 基底コンテナ
};

} // namespace gx::container
