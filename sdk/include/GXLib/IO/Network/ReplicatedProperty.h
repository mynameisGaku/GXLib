#pragma once
/// @file ReplicatedProperty.h
/// @brief ネットワーク同期プロパティ（ダーティフラグ・バージョン管理・補間）
///
/// サーバー/クライアント間で同期するプロパティをラップするテンプレートクラス。
/// 値の変更を検出し（ダーティフラグ）、バージョン管理と型に応じた補間を提供する。
/// @addtogroup grp_network/// @{

#include "pch_common.h"

namespace gx
{

/// @brief ネットワーク同期プロパティ
/// @tparam T プロパティの値型
///
/// 値の変更を追跡し、ダーティフラグとバージョン番号を管理する。
/// 数値型には線形補間を、非数値型にはスナップ補間を提供する。
template<typename T>
class ReplicatedProperty
{
public:
    ReplicatedProperty() = default;

    /// @brief 初期値を指定するコンストラクタ
    explicit ReplicatedProperty(const T& value) : m_value(value), m_prevValue(value) {}

    /// @brief 値を設定する（変更があればダーティフラグを立てる）
    void Set(const T& value)
    {
        if (!(m_value == value))
        {
            m_prevValue = m_value;
            m_value = value;
            m_dirty = true;
            m_version++;
        }
    }

    /// @brief 現在の値を取得する
    const T& Get() const { return m_value; }

    /// @brief 前回の値を取得する
    const T& GetPrevious() const { return m_prevValue; }

    /// @brief ダーティフラグを取得する
    bool IsDirty() const { return m_dirty; }

    /// @brief ダーティフラグをクリアする
    void ClearDirty() { m_dirty = false; }

    /// @brief バージョン番号を取得する
    uint32_t GetVersion() const { return m_version; }

    /// @brief 現在値とターゲット値を補間する
    /// @param target ターゲット値
    /// @param t 補間係数 (0.0〜1.0)
    /// @return 補間された値
    T Interpolate(const T& target, float t) const
    {
        return m_value + (target - m_value) * t;
    }

private:
    T m_value{};        ///< 現在値
    T m_prevValue{};    ///< 前回値
    bool m_dirty = false;    ///< 変更フラグ
    uint32_t m_version = 0;  ///< バージョン番号
};

// ---------------------------------------------------------------------------
// 算術演算を持たない型の特殊化
// ---------------------------------------------------------------------------

/// @brief gx::Stringの補間特殊化（スナップ）
template<>
inline gx::String ReplicatedProperty<gx::String>::Interpolate(const gx::String& target, float) const
{
    return target;
}

/// @brief boolの補間特殊化（スナップ）
template<>
inline bool ReplicatedProperty<bool>::Interpolate(const bool& target, float) const
{
    return target;
}

/// @brief intの補間特殊化（整数切り捨て線形補間）
template<>
inline int ReplicatedProperty<int>::Interpolate(const int& target, float t) const
{
    return static_cast<int>(m_value + static_cast<int>(static_cast<float>(target - m_value) * t));
}

} // namespace gx
/// @}
