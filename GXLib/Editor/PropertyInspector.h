#pragma once
/// @file PropertyInspector.h
/// @brief エンティティのコンポーネントプロパティを一覧・編集するインスペクタ
///
/// 選択中エンティティのコンポーネントと各プロパティを収集して一覧表示し、
/// 値を変更すると onPropertyChanged コールバックで通知する。
/// Unityのインスペクタウィンドウに相当する機能。
/// @addtogroup grp_editor/// @{

#include "pch_graphics.h"
#include "Core/Scene/Entity.h"
#include <functional>

namespace gx
{

/// @brief プロパティ値が変更された際の通知
struct PropertyChange
{
    std::string componentName;   ///< コンポーネント型名（または"Transform"）
    std::string propertyName;    ///< プロパティ名
    std::string newValue;        ///< 文字列としての新しい値
};

/// @brief エンティティコンポーネント編集用プロパティインスペクタパネル
class PropertyInspector
{
public:
    PropertyInspector() = default;
    ~PropertyInspector() = default;

    /// @brief 検査対象のエンティティを設定する
    void SetTarget(Entity* entity);

    /// @brief 現在の対象エンティティを取得する
    Entity* GetTarget() const { return m_target; }

    /// @brief 現在の対象をクリアする
    void ClearTarget();

    /// @brief プロパティ変更時のコールバックを設定する
    using PropertyChangedCallback = std::function<void(const PropertyChange&)>;
    void SetOnPropertyChanged(PropertyChangedCallback cb) { m_onPropertyChanged = std::move(cb); }

    /// @brief 現在の対象のプロパティデータを収集する
    /// @return コンポーネント名とそのプロパティキー値ペアのリスト
    struct PropertyEntry
    {
        std::string name;
        std::string value;
        bool readOnly = false;
    };

    struct ComponentProperties
    {
        std::string componentName;
        std::vector<PropertyEntry> properties;
    };

    /// @brief 現在の対象の全プロパティを取得する
    std::vector<ComponentProperties> GetProperties() const;

    /// @brief 対象のプロパティ値を設定する
    /// @return プロパティが見つかり設定された場合true
    bool SetProperty(const std::string& componentName,
                     const std::string& propertyName,
                     const std::string& value);

    /// @brief アクティブな対象があるかチェックする
    bool HasTarget() const { return m_target != nullptr; }

private:
    Entity* m_target = nullptr;                     ///< 検査対象のエンティティ
    PropertyChangedCallback m_onPropertyChanged;    ///< プロパティ変更時コールバック
};

} // namespace gx
/// @}
