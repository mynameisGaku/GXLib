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

/// @brief ウィジェットの種類を識別するための列挙型
/// DxLibにはGUI機能がないため、GXLib独自のウィジェットシステムで使う。
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

    /// @brief 指定座標がこの矩形内に含まれるか判定する
    /// @param px X座標
    /// @param py Y座標
    /// @return 矩形内なら true
    bool Contains(float px, float py) const
    {
        return px >= x && px <= x + width &&
               py >= y && py <= y + height;
    }
};

// ============================================================================
// イベント
// ============================================================================

/// @brief UIイベントの種類
/// マウス操作、キーボード入力、フォーカス変更、クリックなどを表す。
enum class UIEventType
{
    MouseDown, MouseUp, MouseMove, MouseWheel,
    MouseEnter, MouseLeave,
    KeyDown, KeyUp, CharInput,
    FocusGained, FocusLost,
    Click, ValueChanged, Submit
};

/// @brief イベント伝搬のフェーズ（DOMイベントモデル準拠）
/// Capture: ルートからターゲットへ下る、Target: 対象自身、Bubble: ターゲットからルートへ上る
enum class UIEventPhase { Capture, Target, Bubble };

// Widget の前方宣言（UIEvent が Widget* を持つため）
class Widget;

/// @brief UIイベントデータ
/// マウス座標やキーコードなどのイベント情報を格納する。
/// handled=true にするとイベント処理済みとしてマークできる。
/// stopPropagation=true にするとバブルフェーズの伝搬を停止する。
struct UIEvent
{
    UIEventType type = UIEventType::MouseMove;  ///< イベント種類
    UIEventPhase phase = UIEventPhase::Target;  ///< 現在の伝搬フェーズ
    float mouseX = 0.0f, mouseY = 0.0f;        ///< マウス座標（デザイン座標系）
    float localX = 0.0f, localY = 0.0f;        ///< ウィジェットローカル座標
    int mouseButton = 0;                        ///< マウスボタン番号
    int wheelDelta = 0;                         ///< ホイール回転量
    int keyCode = 0;                            ///< 仮想キーコード (VK_*)
    wchar_t charCode = 0;                       ///< 文字入力 (WM_CHAR由来)
    Widget* target = nullptr;                   ///< イベントの対象ウィジェット
    bool handled = false;                       ///< 処理済みフラグ
    bool stopPropagation = false;               ///< 伝搬停止フラグ
};

// ============================================================================
// Widget 基底クラス
// ============================================================================

/// @brief すべてのGUIウィジェットの基底クラス
class Widget
{
public:
    virtual ~Widget() = default;

    /// @brief ウィジェットの種類を返す（派生クラスが実装する）
    /// @return WidgetType 列挙値
    virtual WidgetType GetType() const = 0;

    // --- ツリー構造 ---

    /// @brief 子ウィジェットを追加する。所有権はこのウィジェットに移る
    /// @param child 追加する子ウィジェット
    void AddChild(std::unique_ptr<Widget> child);

    /// @brief 指定した子ウィジェットを取り除く（所有権が解放される）
    /// @param child 取り除く子ウィジェットのポインタ
    void RemoveChild(Widget* child);

    /// @brief 親ウィジェットを取得する
    /// @return 親ウィジェット。ルートの場合は nullptr
    Widget* GetParent() const { return m_parent; }

    /// @brief 子ウィジェット一覧を取得する（const版）
    /// @return 子ウィジェットの配列
    const std::vector<std::unique_ptr<Widget>>& GetChildren() const { return m_children; }

    /// @brief 子ウィジェット一覧を取得する
    /// @return 子ウィジェットの配列
    std::vector<std::unique_ptr<Widget>>& GetChildren() { return m_children; }

    /// @brief IDでウィジェットをツリー全体から再帰検索する
    /// @param id 検索するID文字列
    /// @return 見つかったウィジェット。なければ nullptr
    Widget* FindById(const std::string& id);

    // --- プロパティ ---
    std::string id;              ///< ウィジェットの一意識別子（FindByIdで使う）
    std::string className;       ///< CSSクラス名（StyleSheetのセレクタで使う）
    bool visible = true;         ///< false にすると描画・更新をスキップする
    bool enabled = true;         ///< false にするとイベントを受け付けなくなる
    float opacity = 1.0f;        ///< 不透明度（0.0=透明 ~ 1.0=不透明）
    int zIndex = 0;              ///< 描画順序（将来拡張用）

    // --- スタイル ---
    Style computedStyle;         ///< StyleSheetから計算されたスタイル

    // --- 状態 ---
    bool hovered = false;        ///< マウスカーソルが乗っているか
    bool pressed = false;        ///< マウスボタンが押されているか
    bool focused = false;        ///< キーボードフォーカスを持っているか

    // --- 描画用スタイル（アニメーション用） ---
    /// @brief 実際の描画に使うスタイルを返す
    /// @return アニメーション中は補間スタイル、そうでなければ computedStyle
    const Style& GetRenderStyle() const { return m_hasRenderStyle ? m_renderStyle : computedStyle; }

    // --- レイアウト ---
    LayoutRect layoutRect;     ///< 親からの相対位置
    LayoutRect globalRect;     ///< スクリーン座標
    bool layoutDirty = true;

    // --- スクロール ---
    float scrollOffsetX = 0.0f;
    float scrollOffsetY = 0.0f;

    /// @brief ウィジェット固有の自然な幅を返す（テキスト幅など）
    /// @return 固有幅（ピクセル）。レイアウトエンジンが width=auto の場合にこの値を使う
    virtual float GetIntrinsicWidth() const { return 0.0f; }

    /// @brief ウィジェット固有の自然な高さを返す（テキスト高さなど）
    /// @return 固有高さ（ピクセル）。レイアウトエンジンが height=auto の場合にこの値を使う
    virtual float GetIntrinsicHeight() const { return 0.0f; }

    // --- イベント ---

    /// @brief イベントを処理する。派生クラスでオーバーライドして固有処理を追加できる
    /// @param event 処理するUIイベント
    /// @return イベントを消費した場合 true
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

    /// @brief 毎フレームの更新処理。スタイルアニメーションと子ウィジェットの更新を行う
    /// @param deltaTime 前フレームからの経過時間（秒）
    virtual void Update(float deltaTime);

    /// @brief 描画処理。Transform/Opacityスタックを積んでからRenderSelf→RenderChildrenの順に呼ぶ
    /// @param renderer GUI描画用レンダラー
    virtual void Render(UIRenderer& renderer);

    /// @brief 自分自身の描画処理。派生クラスでオーバーライドして描画内容を実装する
    /// @param renderer GUI描画用レンダラー
    virtual void RenderSelf(UIRenderer& renderer) {}

    /// @brief 子ウィジェットの描画処理。visibleな子を順に描画する
    /// @param renderer GUI描画用レンダラー
    virtual void RenderChildren(UIRenderer& renderer);

    /// @brief アクティブなエフェクト（リップル等）があれば情報を返す
    /// @param style 現在のスタイル
    /// @param out エフェクト情報の出力先
    /// @return エフェクトがアクティブなら out のポインタ、なければ nullptr
    const UIRectEffect* GetActiveEffect(const Style& style, UIRectEffect& out) const;

protected:
    /// @brief スタイルのトランジション（遷移アニメーション）を更新する
    /// @param deltaTime 前フレームからの経過時間（秒）
    /// @param targetStyle 遷移先のターゲットスタイル
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
