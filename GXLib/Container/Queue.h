#pragma once
/// @file Queue.h
/// @brief FIFO キュー アダプタ

#include "pch_common.h"
#include "Container/Deque.h"
#include <cstddef>
#include <utility>

namespace gx::container {

/// @brief FIFO キュー (コンテナアダプタ)
/// @details デフォルトの基底コンテナは Deque<T>。PushBack/PopFront/Front/Back を持つ
///          任意のコンテナを基底として使用可能。
/// @tparam T 要素型
/// @tparam Container 基底コンテナ型 (デフォルト: Deque<T>)
template <typename T, typename Container = Deque<T>>
class Queue
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
    Queue() = default;

    /// @brief コンテナを指定して構築
    explicit Queue(const Container& container)
        : m_container(container) {}

    /// @brief コンテナをムーブして構築
    explicit Queue(Container&& container)
        : m_container(std::move(container)) {}

    // ========================================================================
    // Element access
    // ========================================================================

    /// @brief キューの先頭 (最初に追加した要素)
    __forceinline reference Front() { return m_container.Front(); }

    /// @brief キューの先頭 (const)
    __forceinline const_reference Front() const { return m_container.Front(); }

    /// @brief キューの末尾 (最後に追加した要素)
    __forceinline reference Back() { return m_container.Back(); }

    /// @brief キューの末尾 (const)
    __forceinline const_reference Back() const { return m_container.Back(); }

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

    /// @brief 末尾に要素を追加 (コピー)
    void Push(const T& value) { m_container.PushBack(value); }

    /// @brief 末尾に要素を追加 (ムーブ)
    void Push(T&& value) { m_container.PushBack(std::move(value)); }

    /// @brief 末尾に要素を直接構築
    template <typename... Args>
    decltype(auto) Emplace(Args&&... args)
    {
        return m_container.EmplaceBack(std::forward<Args>(args)...);
    }

    /// @brief 先頭要素を削除
    void Pop() { m_container.PopFront(); }

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
    __forceinline reference front() { return Front(); }
    __forceinline const_reference front() const { return Front(); }
    __forceinline reference back() { return Back(); }
    __forceinline const_reference back() const { return Back(); }
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

    friend bool operator==(const Queue& a, const Queue& b) { return a.m_container == b.m_container; }
    friend bool operator!=(const Queue& a, const Queue& b) { return !(a == b); }

private:
    Container m_container;  ///< 基底コンテナ
};

} // namespace gx::container
