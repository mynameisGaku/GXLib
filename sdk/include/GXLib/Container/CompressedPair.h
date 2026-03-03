#pragma once
/// @file CompressedPair.h
/// @brief EBO最適化ペア — アロケータをゼロバイトに圧縮

#include "pch_common.h"
#include <type_traits>

namespace gx::container {

/// @brief EBO活用の圧縮ペア (first がemptyクラスなら0バイト)
/// EASTL の compressed_pair に準拠
template <typename T1, typename T2, bool = std::is_empty_v<T1> && !std::is_final_v<T1>>
class CompressedPair;

/// @brief T1 がemptyクラス → EBO適用 (T1を継承、サイズ0)
template <typename T1, typename T2>
class CompressedPair<T1, T2, true> : private T1
{
public:
    using first_type = T1;
    using second_type = T2;

    CompressedPair() = default;

    template <typename U1, typename U2>
    CompressedPair(U1&& a, U2&& b)
        : T1(std::forward<U1>(a)), m_second(std::forward<U2>(b)) {}

    explicit CompressedPair(const T2& b) : T1(), m_second(b) {}

    __forceinline T1&       First()       noexcept { return *this; }
    __forceinline const T1& First() const noexcept { return *this; }
    __forceinline T2&       Second()       noexcept { return m_second; }
    __forceinline const T2& Second() const noexcept { return m_second; }

private:
    T2 m_second{};
};

/// @brief T1 が非empty → EBO不適用 (通常ペア)
template <typename T1, typename T2>
class CompressedPair<T1, T2, false>
{
public:
    using first_type = T1;
    using second_type = T2;

    CompressedPair() = default;

    template <typename U1, typename U2>
    CompressedPair(U1&& a, U2&& b)
        : m_first(std::forward<U1>(a)), m_second(std::forward<U2>(b)) {}

    explicit CompressedPair(const T2& b) : m_first(), m_second(b) {}

    __forceinline T1&       First()       noexcept { return m_first; }
    __forceinline const T1& First() const noexcept { return m_first; }
    __forceinline T2&       Second()       noexcept { return m_second; }
    __forceinline const T2& Second() const noexcept { return m_second; }

private:
    T1 m_first{};
    T2 m_second{};
};

} // namespace gx::container
