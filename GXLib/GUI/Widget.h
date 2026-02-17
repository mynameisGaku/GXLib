#pragma once
/// @file Widget.h
/// @brief GUI ウィジェット基底クラス
///
/// すべてのGUIウィジェットの基底クラス。
/// ツリー構造管理、スタイル、レイアウト矩形、イベント処理を提供する。

#include "pch.h"
#include "GUI/Style.h"

namespace GX { namespace GUI {

// 前方宣言
class UIRenderer;
struct UIRectEffect;

// ============================================================================
// ウィジェットタイプ
// ============================================================================

enum class WidgetType
{
    Panel, Text, Button, Image, TextInput, Slider,
    CheckBox, RadioButton, DropDown, ListView,
    ScrollView, ProgressBar, TabView, Dialog, Canvas, Spacer
};

// ============================================================================
// レイアウト矩形
// ============================================================================

/// @brief レイアウト計算済みの矩形
struct LayoutRect
{
    float x = 0.0f, y = 0.0f;
    float width = 0.0f, height = 0.0f;

    bool Contains(float px, float py) const
    {
        return px >= x && px <= x + width &&
               py >= y && py <= y + height;
    }
};

// ============================================================================
// イベント
// ============================================================================

enum class UIEventType
{
    MouseDown, MouseUp, MouseMove, MouseWheel,
    MouseEnter, MouseLeave,
    KeyDown, KeyUp, CharInput,
    FocusGained, FocusLost,
    Click, ValueChanged, Submit
};

enum class UIEventPhase { Capture, Target, Bubble };

// Widget の前方宣言（UIEvent が Widget* を持つため）
class Widget;

struct UIEvent
{
    UIEventType type = UIEventType::MouseMove;
    UIEventPhase phase = UIEventPhase::Target;
    float mouseX = 0.0f, mouseY = 0.0f;
    float localX = 0.0f, localY = 0.0f;
    int mouseButton = 0;
    int wheelDelta = 0;
    int keyCode = 0;
    wchar_t charCode = 0;
    Widget* target = nullptr;
    bool handled = false;
    bool stopPropagation = false;
};

// ============================================================================
// Widget 基底クラス
// ============================================================================

/// @brief すべてのGUIウィジェットの基底クラス
class Widget
{
public:
    virtual ~Widget() = default;

    /// ウィジェットの種類を返す
    virtual WidgetType GetType() const = 0;

    // --- ツリー構造 ---
    void AddChild(std::unique_ptr<Widget> child);
    void RemoveChild(Widget* child);
    Widget* GetParent() const { return m_parent; }
    const std::vector<std::unique_ptr<Widget>>& GetChildren() const { return m_children; }
    std::vector<std::unique_ptr<Widget>>& GetChildren() { return m_children; }

    /// IDでウィジェットを再帰検索
    Widget* FindById(const std::string& id);

    // --- プロパティ ---
    std::string id;
    std::string className;
    bool visible = true;
    bool enabled = true;
    float opacity = 1.0f;
    int zIndex = 0;

    // --- スタイル ---
    Style computedStyle;

    // --- 状態 ---
    bool hovered = false;
    bool pressed = false;
    bool focused = false;

    // --- 描画用スタイル（アニメーション用） ---
    const Style& GetRenderStyle() const { return m_hasRenderStyle ? m_renderStyle : computedStyle; }

    // --- レイアウト ---
    LayoutRect layoutRect;     ///< 親からの相対位置
    LayoutRect globalRect;     ///< スクリーン座標
    bool layoutDirty = true;

    // --- スクロール ---
    float scrollOffsetX = 0.0f;
    float scrollOffsetY = 0.0f;

    /// ウィジェット固有の intrinsic size（テキスト幅等）
    virtual float GetIntrinsicWidth() const { return 0.0f; }
    virtual float GetIntrinsicHeight() const { return 0.0f; }

    // --- イベント ---
    virtual bool OnEvent(const UIEvent& event);
    std::function<void()> onClick;
    std::function<void()> onHover;
    std::function<void()> onLeave;
    std::function<void()> onPress;
    std::function<void()> onRelease;
    std::function<void()> onFocus;
    std::function<void()> onBlur;
    std::function<void()> onSubmit;
    std::function<void(const UIEvent&)> onEvent;
    /// 値変更コールバック。各ウィジェットの値形式:
    /// - CheckBox: "true" / "false"
    /// - Slider: "0.5" (float文字列)
    /// - RadioButton: 選択されたラジオボタンのテキスト
    /// - TextInput: 入力テキスト (UTF-8)
    /// - DropDown/ListView: 選択されたアイテムのテキスト
    std::function<void(const std::string&)> onValueChanged;

    // --- ライフサイクル ---
    virtual void Update(float deltaTime);
    virtual void Render(UIRenderer& renderer);
    virtual void RenderSelf(UIRenderer& renderer) {}
    virtual void RenderChildren(UIRenderer& renderer);
    const UIRectEffect* GetActiveEffect(const Style& style, UIRectEffect& out) const;

protected:
    void UpdateStyleTransition(float deltaTime, const Style& targetStyle);

    Widget* m_parent = nullptr;
    std::vector<std::unique_ptr<Widget>> m_children;

    Style m_renderStyle;
    Style m_targetStyle;
    Style m_startStyle;
    float m_transitionTime = 0.0f;
    float m_transitionDuration = 0.0f;
    bool m_hasRenderStyle = false;

    bool m_effectActive = false;
    float m_effectTime = 0.0f;
    float m_effectCenterX = 0.5f;
    float m_effectCenterY = 0.5f;
};

}} // namespace GX::GUI
