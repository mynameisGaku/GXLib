#pragma once
/// @file TextInput.h
/// @brief テキスト入力ウィジェット

#include "pch.h"
#include "GUI/Widget.h"

namespace GX { namespace GUI {

class UIRenderer;

/// @brief 単一行テキスト入力ウィジェット
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
    void SetText(const std::wstring& text);
    const std::wstring& GetText() const { return m_text; }
    void SetPlaceholder(const std::wstring& text) { m_placeholder = text; }
    void SetFontHandle(int handle) { m_fontHandle = handle; }
    int GetFontHandle() const { return m_fontHandle; }
    void SetRenderer(UIRenderer* renderer) { m_renderer = renderer; }
    void SetMaxLength(int maxLen) { m_maxLength = maxLen; }
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
