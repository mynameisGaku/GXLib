#pragma once
/// @file DataBinding.h
/// @brief データモデルとウィジェットを繋ぐデータバインディングシステム
///
/// DataProperty（観測可能な変数）の値が変わると、対応するウィジェットが
/// 自動で更新される。一方向・双方向・初回のみの3モードを選べる。
/// @addtogroup grp_gui/// @{

#include "pch_graphics.h"

namespace gx { namespace GUI {

/// @brief Binding propagation mode
enum class BindingMode
{
    OneWay,    ///< Source to target only
    TwoWay,    ///< Both directions
    OneTime    ///< Initial value only, no subsequent updates
};

/// @brief Converter functions for transforming values between source and target
struct BindingConverter
{
    std::function<std::string(const std::string&)> toUI;     ///< Convert source value for UI display
    std::function<std::string(const std::string&)> fromUI;   ///< Convert UI value back to source format
};

/// @brief An observable property that notifies on value changes
class DataProperty
{
public:
    DataProperty() = default;
    explicit DataProperty(const std::string& name) : m_name(name) {}
    ~DataProperty() = default;

    /// @brief Set the property value and notify observers
    /// @param value The new value
    void Set(const std::string& value)
    {
        std::string oldValue = m_value;
        m_value = value;
        if (OnChanged && oldValue != value)
            OnChanged(oldValue, value);
    }

    /// @brief Get the current value
    /// @return Current string value
    const std::string& Get() const { return m_value; }

    /// @brief Set value without triggering notifications
    /// @param value The new value
    void SetSilent(const std::string& value) { m_value = value; }

    /// @brief Get the property name
    /// @return The property name
    const std::string& GetName() const { return m_name; }

    /// @brief Set an integer value
    /// @param v The integer value
    void SetInt(int v) { Set(std::to_string(v)); }

    /// @brief Get as integer
    /// @return The integer value, or 0 if parsing fails
    int GetInt() const
    {
        try { return std::stoi(m_value); }
        catch (...) { return 0; }
    }

    /// @brief Set a float value
    /// @param v The float value
    void SetFloat(float v) { Set(std::to_string(v)); }

    /// @brief Get as float
    /// @return The float value, or 0.0f if parsing fails
    float GetFloat() const
    {
        try { return std::stof(m_value); }
        catch (...) { return 0.0f; }
    }

    /// @brief Set a boolean value
    /// @param v The boolean value
    void SetBool(bool v) { Set(v ? "true" : "false"); }

    /// @brief Get as boolean
    /// @return true if value is "true" or "1"
    bool GetBool() const { return m_value == "true" || m_value == "1"; }

    /// @brief Callback invoked when the value changes. Parameters: (oldValue, newValue)
    std::function<void(const std::string&, const std::string&)> OnChanged;

private:
    std::string m_name;   ///< プロパティ名
    std::string m_value;  ///< 現在の値（文字列）
};

/// @brief A collection of named properties that can be observed for changes
class DataContext
{
public:
    DataContext() = default;
    ~DataContext() = default;

    /// @brief Set a property value (creates if not present)
    /// @param name Property name
    /// @param value Property value
    void SetProperty(const std::string& name, const std::string& value);

    /// @brief Get a property value
    /// @param name Property name
    /// @return The property value, or empty string if not found
    std::string GetProperty(const std::string& name) const;

    /// @brief Check if a property exists
    /// @param name Property name
    /// @return true if the property exists
    bool HasProperty(const std::string& name) const;

    /// @brief Remove a property
    /// @param name Property name
    /// @return true if the property was found and removed
    bool RemoveProperty(const std::string& name);

    /// @brief Get the number of properties
    /// @return Property count
    size_t GetPropertyCount() const;

    /// @brief Get all property names
    /// @return Vector of property names
    std::vector<std::string> GetPropertyNames() const;

    /// @brief Get direct access to a DataProperty object
    /// @param name Property name
    /// @return Pointer to the DataProperty, or nullptr if not found
    DataProperty* GetDataProperty(const std::string& name);

    /// @brief Callback invoked when any property changes
    std::function<void(const std::string& name, const std::string& value)> OnPropertyChanged;

private:
    std::unordered_map<std::string, DataProperty> m_properties; ///< 名前→プロパティマップ
};

/// @brief A binding definition connecting a source property to a target
struct Binding
{
    std::string sourceProperty;                ///< ソースプロパティ名
    std::string targetId;                      ///< ターゲットウィジェットID
    std::string targetProperty;                ///< ターゲット上のプロパティ名
    BindingMode mode = BindingMode::OneWay;    ///< バインディングモード
    BindingConverter converter;                ///< 値変換関数
    bool active = true;                        ///< アクティブ状態
};

/// @brief Manages data bindings between a DataContext and UI targets
class DataBindingManager
{
public:
    DataBindingManager() = default;
    ~DataBindingManager() = default;

    /// @brief Create a new binding
    /// @param sourceProperty Source property name in the DataContext
    /// @param targetId ID of the target widget
    /// @param targetProperty Property name on the target
    /// @param mode Binding mode (default OneWay)
    /// @return Binding ID
    uint32_t Bind(const std::string& sourceProperty, const std::string& targetId,
                  const std::string& targetProperty, BindingMode mode = BindingMode::OneWay);

    /// @brief Create a binding with value converter
    /// @param sourceProperty Source property name
    /// @param targetId Target widget ID
    /// @param targetProperty Target property name
    /// @param converter Value converter functions
    /// @param mode Binding mode
    /// @return Binding ID
    uint32_t BindWithConverter(const std::string& sourceProperty, const std::string& targetId,
                               const std::string& targetProperty, const BindingConverter& converter,
                               BindingMode mode = BindingMode::OneWay);

    /// @brief Remove a binding
    /// @param bindingId The binding ID to remove
    /// @return true if found and removed
    bool Unbind(uint32_t bindingId);

    /// @brief Remove all bindings for a specific target
    /// @param targetId The target widget ID
    void UnbindAll(const std::string& targetId);

    /// @brief Get the total number of bindings
    /// @return Binding count
    size_t GetBindingCount() const;

    /// @brief Get a binding by ID
    /// @param bindingId The binding ID
    /// @return Pointer to the Binding, or nullptr
    const Binding* GetBinding(uint32_t bindingId) const;

    /// @brief Set the data context for bindings
    /// @param context Pointer to the data context
    void SetDataContext(DataContext* context);

    /// @brief Get the current data context
    /// @return Pointer to the data context
    DataContext* GetDataContext() const;

    /// @brief Enable or disable a binding
    /// @param bindingId The binding ID
    /// @param active Whether the binding should be active
    void SetActive(uint32_t bindingId, bool active);

    /// @brief Check if a binding is active
    /// @param bindingId The binding ID
    /// @return true if the binding is active
    bool IsActive(uint32_t bindingId) const;

    /// @brief Push all source values to targets (process pending changes)
    void UpdateBindings();

    /// @brief Notify that a specific property has changed
    /// @param propertyName The property name that changed
    void NotifyPropertyChanged(const std::string& propertyName);

    /// @brief Get all binding IDs for a specific source property
    /// @param propertyName The source property name
    /// @return Vector of binding IDs
    std::vector<uint32_t> GetBindingsForProperty(const std::string& propertyName) const;

    /// @brief Get all binding IDs for a specific target
    /// @param targetId The target widget ID
    /// @return Vector of binding IDs
    std::vector<uint32_t> GetBindingsForTarget(const std::string& targetId) const;

    /// @brief Remove all bindings
    void Clear();

private:
    std::unordered_map<uint32_t, Binding> m_bindings;  ///< バインディングID→定義マップ
    DataContext* m_dataContext = nullptr;               ///< データコンテキスト
    uint32_t m_nextBindingId = 1;                      ///< 次に割り当てるバインディングID
    std::vector<std::string> m_dirtyProperties;        ///< 更新が必要なプロパティ名
};

}} // namespace gx::GUI
/// @}
