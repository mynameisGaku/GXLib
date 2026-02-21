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

/// @brief GUIシステムの中心となるコンテキストクラス
/// ウィジェットツリーの管理、Flexboxレイアウト計算、入力イベントのディスパッチ、
/// フォーカス管理を一括して行う。DxLibにはGUI機能がないため独自実装。
class UIContext
{
public:
    UIContext() = default;
    ~UIContext() = default;

    /// @brief 初期化する
    /// @param renderer GUI描画用レンダラー
    /// @param screenWidth スクリーン幅（ピクセル）
    /// @param screenHeight スクリーン高さ（ピクセル）
    /// @return 成功なら true
    bool Initialize(UIRenderer* renderer, uint32_t screenWidth, uint32_t screenHeight);

    /// @brief ルートウィジェットを設定する。以前のルートは破棄される
    /// @param root ルートとなるウィジェットツリー
    void SetRoot(std::unique_ptr<Widget> root);

    /// @brief ルートウィジェットを取得する
    /// @return ルートウィジェット。未設定なら nullptr
    Widget* GetRoot() const { return m_root.get(); }

    /// @brief IDでウィジェットをツリー全体から検索する
    /// @param id 検索するID文字列
    /// @return 見つかったウィジェット。なければ nullptr
    Widget* FindById(const std::string& id);

    /// @brief フレーム更新。入力処理→イベントディスパッチ→レイアウト計算→アニメーション更新の順に実行
    /// @param deltaTime 前フレームからの経過時間（秒）
    /// @param input 入力マネージャー
    void Update(float deltaTime, InputManager& input);

    /// @brief ウィジェットツリーを描画する
    void Render();

    /// @brief WM_CHARメッセージを処理する（Window::AddMessageCallbackから呼ぶ）
    /// @param ch 入力された文字
    /// @return 処理された場合 true
    bool ProcessCharMessage(wchar_t ch);

    /// @brief キーボードフォーカスを指定ウィジェットに設定する
    /// @param widget フォーカス先。nullptr でフォーカス解除
    void SetFocus(Widget* widget);

    /// @brief 現在フォーカスを持つウィジェットを取得する
    /// @return フォーカス中のウィジェット。なければ nullptr
    Widget* GetFocusedWidget() const { return m_focusedWidget; }

    /// @brief スタイルシートを設定する。Update時にツリー全体へ自動適用される
    /// @param sheet 適用するスタイルシート
    void SetStyleSheet(StyleSheet* sheet);

    /// @brief スクリーンサイズ変更時に呼ぶ。レイアウトの再計算をトリガーする
    /// @param width 新しいスクリーン幅
    /// @param height 新しいスクリーン高さ
    void OnResize(uint32_t width, uint32_t height);

    /// @brief デザイン解像度を設定する。GUI座標空間の基準となる仮想解像度で、
    ///        実際のスクリーンサイズとの比率でスケーリングされる（レターボックス対応）
    /// @param width デザイン幅（0で無効化）
    /// @param height デザイン高さ（0で無効化）
    void SetDesignResolution(uint32_t width, uint32_t height);

    /// @brief UIRendererを取得する
    /// @return UIRenderer のポインタ
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
