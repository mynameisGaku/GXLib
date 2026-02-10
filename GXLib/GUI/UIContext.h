#pragma once
/// @file UIContext.h
/// @brief GUI コンテキスト — ウィジェットツリー管理・更新・イベントディスパッチ
///
/// UIContext はGUIシステムの中心となるクラス。
/// ウィジェットツリーの管理、レイアウト計算、イベントの3フェーズディスパッチ
/// （キャプチャ→ターゲット→バブル）、フォーカス管理を行う。

#include "pch.h"
#include "GUI/Widget.h"
#include "GUI/UIRenderer.h"
#include "GUI/StyleSheet.h"

namespace GX
{
class InputManager;
}

namespace GX { namespace GUI {

/// @brief GUI コンテキスト
class UIContext
{
public:
    UIContext() = default;
    ~UIContext() = default;

    /// 初期化
    bool Initialize(UIRenderer* renderer, uint32_t screenWidth, uint32_t screenHeight);

    /// ルートウィジェットを設定
    void SetRoot(std::unique_ptr<Widget> root);

    /// ルートウィジェットを取得
    Widget* GetRoot() const { return m_root.get(); }

    /// IDでウィジェットを検索
    Widget* FindById(const std::string& id);

    /// フレーム更新（入力→イベント→レイアウト→アニメーション）
    void Update(float deltaTime, InputManager& input);

    /// 描画
    void Render();

    /// WM_CHARメッセージ処理（Window::AddMessageCallbackから呼ばれる）
    bool ProcessCharMessage(wchar_t ch);

    /// フォーカスウィジェットを設定
    void SetFocus(Widget* widget);

    /// フォーカスウィジェットを取得
    Widget* GetFocusedWidget() const { return m_focusedWidget; }

    /// スタイルシートを設定（ツリー全体に自動適用）
    void SetStyleSheet(StyleSheet* sheet);

    /// スクリーンサイズ更新
    void OnResize(uint32_t width, uint32_t height);

    /// デザイン解像度を設定（GUI座標空間の基準解像度）
    void SetDesignResolution(uint32_t width, uint32_t height);

    /// UIRendererを取得
    UIRenderer* GetRenderer() const { return m_renderer; }

private:
    /// 入力イベントを生成してディスパッチ
    void ProcessInputEvents(InputManager& input);

    /// 3フェーズイベントディスパッチ（キャプチャ→ターゲット→バブル）
    void DispatchEvent(UIEvent& event);

    /// ヒットテスト（スクリーン座標→ウィジェット）
    Widget* HitTest(Widget* widget, float x, float y);

    /// レイアウトを計算（ダーティな場合のみ）
    void ComputeLayout();

    /// ウィジェットのレイアウト矩形を計算（再帰）
    void LayoutWidget(Widget* widget, float posX, float posY,
                      float allocWidth, float allocHeight);

    /// ウィジェットの希望サイズを再帰的に計測（ボトムアップ）
    struct WidgetSize { float width, height; };
    WidgetSize MeasureWidget(Widget* widget, float maxWidth, float maxHeight);

    /// キャプチャパスのパスを収集
    void CollectAncestors(Widget* widget, std::vector<Widget*>& path);

    UIRenderer*              m_renderer = nullptr;
    StyleSheet*              m_styleSheet = nullptr;
    std::unique_ptr<Widget>  m_root;
    Widget*                  m_focusedWidget = nullptr;
    Widget*                  m_hoveredWidget = nullptr;
    Widget*                  m_pressedWidget = nullptr;
    uint32_t                 m_screenWidth  = 1280;
    uint32_t                 m_screenHeight = 720;
    uint32_t                 m_designWidth  = 0;   // 0 = スケーリング無効
    uint32_t                 m_designHeight = 0;
    float                    m_prevMouseX = 0.0f;
    float                    m_prevMouseY = 0.0f;
    bool                     m_prevMouseDown = false;
};

}} // namespace GX::GUI
