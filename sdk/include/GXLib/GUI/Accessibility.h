#pragma once
/// @file Accessibility.h
/// @brief GUIシステムのアクセシビリティ対応 — スクリーンリーダー等への情報提供
///
/// ウィジェットにロール（ボタン・スライダー等）とアクセシブルラベルを付与する。
/// アクセシビリティツリーを構築し、タブ順序ナビゲーションもサポートする。
/// @addtogroup grp_gui/// @{

#include "pch_graphics.h"

namespace gx { namespace GUI {

/// @brief Semantic role of an accessible element
enum class AccessibilityRole
{
    None, Button, TextInput, Slider, Checkbox, RadioButton, ComboBox,
    Menu, MenuItem, Tab, Panel, Label, Image, ProgressBar, ScrollBar,
    Tooltip, Dialog, Alert
};

/// @brief Current state of an accessible element
struct AccessibilityState
{
    bool isEnabled = true;       ///< 有効か
    bool isFocused = false;      ///< フォーカス中か
    bool isSelected = false;     ///< 選択されているか
    bool isExpanded = false;     ///< 展開されているか（ツリー/ドロップダウン用）
    bool isChecked = false;      ///< チェック状態（チェックボックス用）
    float valueMin = 0.0f;       ///< 値の最小値（スライダー等用）
    float valueMax = 100.0f;     ///< 値の最大値
    float valueCurrent = 0.0f;   ///< 現在の値
    gx::String description;     ///< 説明テキスト（スクリーンリーダー用）
};

/// @brief An element in the accessibility tree
struct AccessibleElement
{
    uint32_t id = 0;                                   ///< 要素ID
    AccessibilityRole role = AccessibilityRole::None;  ///< 意味論的ロール
    gx::String label;                                 ///< アクセシブルラベル
    gx::String hint;                                  ///< 補助テキスト（ヒント）
    AccessibilityState state;                          ///< 現在の状態
    uint32_t parentId = 0;                             ///< 親要素ID
    gx::Vector<uint32_t> childIds;                    ///< 子要素IDリスト
    int tabOrder = 0;                                  ///< タブ順序
};

/// @brief Direction for keyboard focus navigation
enum class FocusNavigationDir
{
    Next, Previous, Up, Down, Left, Right
};

/// @brief Global accessibility configuration
struct AccessibilityConfig
{
    bool enableKeyboardNav = true;
    bool enableScreenReader = true;
    bool enableHighContrast = false;
    bool enableReducedMotion = false;
    float focusIndicatorWidth = 3.0f;
    uint32_t focusIndicatorColor = 0xFFFFFF00;  ///< Yellow
    float minimumTouchTarget = 44.0f;           ///< Pixels
    float fontScaleFactor = 1.0f;
    bool captionsEnabled = false;
};

/// @brief Priority levels for screen reader announcements
enum class AnnouncementPriority
{
    Low, Medium, High
};

/// @brief A screen reader announcement event
struct ScreenReaderEvent
{
    uint32_t elementId = 0;
    gx::String announcement;
    AnnouncementPriority priority = AnnouncementPriority::Medium;
};

/// @brief High contrast color palette
struct HighContrastColors
{
    uint32_t fg = 0xFFFFFFFF;      ///< Foreground (white)
    uint32_t bg = 0xFF000000;      ///< Background (black)
    uint32_t accent = 0xFF00FFFF;  ///< Accent (cyan)
    uint32_t focus = 0xFFFFFF00;   ///< Focus indicator (yellow)
};

/// @brief Manager for accessibility features including focus navigation,
/// screen reader announcements, and visual accessibility options
class AccessibilityManager
{
public:
    AccessibilityManager() = default;
    ~AccessibilityManager() = default;

    // --- Configuration ---

    /// @brief Set the accessibility configuration
    /// @param cfg The new configuration
    void SetConfig(const AccessibilityConfig& cfg) { m_config = cfg; }

    /// @brief Get the current configuration
    /// @return Reference to the current configuration
    const AccessibilityConfig& GetConfig() const { return m_config; }

    // --- Element management ---

    /// @brief Register a new accessible element
    /// @param element The element to register
    /// @return The element ID
    uint32_t RegisterElement(const AccessibleElement& element);

    /// @brief Unregister an element
    /// @param elementId The element to remove
    /// @return true if found and removed
    bool UnregisterElement(uint32_t elementId);

    /// @brief Get an element by ID
    /// @param elementId The element ID
    /// @return Pointer to the element, or nullptr
    const AccessibleElement* GetElement(uint32_t elementId) const;

    /// @brief Get the total number of registered elements
    /// @return Element count
    size_t GetElementCount() const;

    /// @brief Check if an element exists
    /// @param elementId The element ID
    /// @return true if the element exists
    bool HasElement(uint32_t elementId) const;

    /// @brief Update an element's state
    /// @param elementId The element ID
    /// @param state The new state
    /// @return true if the element was found and updated
    bool UpdateElement(uint32_t elementId, const AccessibilityState& state);

    // --- Focus management ---

    /// @brief Set focus to a specific element
    /// @param elementId The element to focus
    /// @return true if the element was found and focused
    bool SetFocus(uint32_t elementId);

    /// @brief Get the currently focused element ID
    /// @return The focused element ID, or 0 if none
    uint32_t GetFocusedElement() const;

    /// @brief Check if any element has focus
    /// @return true if an element is focused
    bool HasFocus() const;

    /// @brief Navigate focus in the specified direction
    /// @param direction The navigation direction
    /// @return true if focus was moved
    bool NavigateFocus(FocusNavigationDir direction);

    /// @brief Activate (click/press) the focused element
    /// @return true if an element was activated
    bool ActivateFocused();

    // --- Screen reader ---

    /// @brief Queue a screen reader announcement
    /// @param text The announcement text
    /// @param priority The announcement priority
    void Announce(const gx::String& text, AnnouncementPriority priority = AnnouncementPriority::Medium);

    /// @brief Get all pending announcements
    /// @return Vector of screen reader events
    gx::Vector<ScreenReaderEvent> GetPendingAnnouncements() const;

    /// @brief Clear all pending announcements
    void ClearAnnouncements();

    // --- Tab order ---

    /// @brief Get elements sorted by tab order
    /// @return Vector of element IDs sorted by tab order
    gx::Vector<uint32_t> GetTabOrder() const;

    /// @brief Set the tab order for an element
    /// @param elementId The element ID
    /// @param order The new tab order value
    /// @return true if the element was found and updated
    bool SetTabOrder(uint32_t elementId, int order);

    // --- Tree ---

    /// @brief Get all registered elements as a flat list
    /// @return Vector of all accessible elements
    gx::Vector<AccessibleElement> GetAccessibleTree() const;

    /// @brief Find elements by label text
    /// @param label The label to search for
    /// @return Vector of matching element IDs
    gx::Vector<uint32_t> FindByLabel(const gx::String& label) const;

    // --- Visual accessibility ---

    /// @brief Get high contrast color palette based on current config
    /// @return High contrast colors
    HighContrastColors GetHighContrastColors() const;

    /// @brief Check if motion should be reduced
    /// @return true if reduced motion is enabled
    bool ShouldReduceMotion() const;

    /// @brief Get the current font scale factor
    /// @return Font scale multiplier
    float GetFontScale() const;

    /// @brief Clear all elements and announcements
    void Clear();

private:
    AccessibilityConfig m_config;                                    ///< 設定
    gx::HashMap<uint32_t, AccessibleElement> m_elements;      ///< 登録済み要素マップ
    uint32_t m_nextElementId = 1;                                    ///< 次に割り当てる要素ID
    uint32_t m_focusedElementId = 0;                                 ///< フォーカス中の要素ID
    gx::Vector<ScreenReaderEvent> m_pendingAnnouncements;           ///< 未処理のアナウンスキュー
    bool m_activatedThisFrame = false;                               ///< 今フレームでActivateされたか
};

}} // namespace gx::GUI
/// @}
