#pragma once
/// @file TextInput.h
/// @brief テキスト入力ウィジェット

#include "pch.h"
#include "GUI/Widget.h"

namespace GX { namespace GUI {

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
    void SetText(const std::wstring& text);

    /// @brief 現在のテキストを取得する
    /// @return テキスト文字列
    const std::wstring& GetText() const { return m_text; }

    /// @brief プレースホルダーテキストを設定する（未入力時に薄く表示される）
    /// @param text プレースホルダー文字列
    void SetPlaceholder(const std::wstring& text) { m_placeholder = text; }

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
    std::wstring m_text;
    std::wstring m_placeholder;
    int          m_cursorPos = 0;
    int          m_selStart = -1;
    int          m_selEnd = -1;
    bool         m_passwordMode = false;
    int          m_maxLength = 0;

    // 表示状態
    float m_scrollOffsetX = 0.0f;
    float m_blinkTimer = 0.0f;
    bool  m_cursorVisible = true;

    // ドラッグ選択
    bool  m_selecting = false;

    // レンダラー・フォント
    UIRenderer* m_renderer = nullptr;
    int         m_fontHandle = -1;

    // ヘルパー
    int   HitTestCursor(float localX) const;
    void  DeleteSelection();
    void  InsertText(const std::wstring& str);
    void  EnsureCursorVisible();
    float GetCursorX() const;
    std::wstring GetDisplayText() const;
    bool  HasSelection() const;
    void  ClearSelection();
    void  SelectAll();
    void  CopyToClipboard();
    void  PasteFromClipboard();
    void  CutToClipboard();
};

}} // namespace GX::GUI
