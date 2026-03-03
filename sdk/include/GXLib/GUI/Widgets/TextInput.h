#pragma once
/// @file TextInput.h
/// @brief テキスト入力ウィジェット

#include "pch_graphics.h"
#include "GUI/Widget.h"

namespace gx { namespace GUI {
/// @addtogroup grp_gui_widgets
/// @{

class UIRenderer;

/// @brief 単一行テキスト入力ウィジェット
/// キーボードフォーカスを受け取り、文字入力・カーソル移動・選択・コピー&ペーストに対応する。
/// パスワードモード、最大文字数制限、プレースホルダーテキストも設定できる。
class TextInput : public Widget
{
public:
    TextInput() = default;
    ~TextInput() override = default;

    WidgetType GetType() const override { return WidgetType::TextInput; }
    float GetIntrinsicWidth() const override;
    float GetIntrinsicHeight() const override;
    bool OnEvent(const UIEvent& event) override;
    void Update(float deltaTime) override;
    void RenderSelf(UIRenderer& renderer) override;

    // 公開API

    /// @brief テキストを設定する（カーソルは末尾に移動する）
    /// @param text 設定するテキスト
    void SetText(const gx::WString& text);

    /// @brief 現在のテキストを取得する
    /// @return テキスト文字列
    const gx::WString& GetText() const { return m_text; }

    /// @brief プレースホルダーテキストを設定する（未入力時に薄く表示される）
    /// @param text プレースホルダー文字列
    void SetPlaceholder(const gx::WString& text) { m_placeholder = text; }

    /// @brief フォントハンドルを設定する
    /// @param handle FontManagerで取得したハンドル
    void SetFontHandle(int handle) { m_fontHandle = handle; }

    /// @brief フォントハンドルを取得する
    /// @return フォントハンドル
    int GetFontHandle() const { return m_fontHandle; }

    /// @brief UIRendererを設定する（テキスト幅の計測に必要）
    /// @param renderer GUI描画用レンダラー
    void SetRenderer(UIRenderer* renderer) { m_renderer = renderer; }

    /// @brief 最大文字数を設定する（0で無制限）
    /// @param maxLen 最大文字数
    void SetMaxLength(int maxLen) { m_maxLength = maxLen; }

    /// @brief パスワードモードを設定する（trueで入力文字が * で表示される）
    /// @param pw true=パスワードモード
    void SetPasswordMode(bool pw) { m_passwordMode = pw; }

    // コールバック
    std::function<void()> onSubmit;

private:
    // テキスト状態
    gx::WString m_text;                ///< 入力テキスト（Unicode）
    gx::WString m_placeholder;         ///< プレースホルダーテキスト（未入力時に薄く表示）
    int          m_cursorPos = 0;       ///< カーソル位置（文字インデックス）
    int          m_selStart = -1;       ///< 選択範囲の開始インデックス（-1=選択なし）
    int          m_selEnd = -1;         ///< 選択範囲の終了インデックス（-1=選択なし）
    bool         m_passwordMode = false; ///< パスワードモード（true で * 表示）
    int          m_maxLength = 0;       ///< 最大文字数（0=無制限）

    // 表示状態
    float m_scrollOffsetX = 0.0f;      ///< テキストの水平スクロールオフセット（ピクセル）
    float m_blinkTimer = 0.0f;         ///< カーソル点滅タイマー（秒）
    bool  m_cursorVisible = true;      ///< カーソルの表示状態（点滅切替用）

    // ドラッグ選択
    bool  m_selecting = false;         ///< マウスドラッグによる選択操作中か

    // レンダラー・フォント
    UIRenderer* m_renderer = nullptr;  ///< GUI描画レンダラー（テキスト幅計算用）
    int         m_fontHandle = -1;     ///< フォントハンドル

    // ヘルパー
    int   HitTestCursor(float localX) const;
    void  DeleteSelection();
    void  InsertText(const gx::WString& str);
    void  EnsureCursorVisible();
    float GetCursorX() const;
    gx::WString GetDisplayText() const;
    bool  HasSelection() const;
    void  ClearSelection();
    void  SelectAll();
    void  CopyToClipboard();
    void  PasteFromClipboard();
    void  CutToClipboard();
};

/// @}
}} // namespace gx::GUI
