#include "pch.h"
#include "GUI/Widgets/TextInput.h"
#include "GUI/UIRenderer.h"

namespace GX { namespace GUI {

// ============================================================================
// wstring → string（UTF-8変換）
// ============================================================================

static std::string WstringToUtf8(const std::wstring& ws)
{
    if (ws.empty()) return "";
    int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(),
                                   static_cast<int>(ws.size()), nullptr, 0, nullptr, nullptr);
    if (len <= 0) return "";
    std::string result(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(),
                         static_cast<int>(ws.size()), &result[0], len, nullptr, nullptr);
    return result;
}

// ============================================================================
// 内容サイズ（Intrinsic Size）
// ============================================================================

float TextInput::GetIntrinsicWidth() const
{
    return 200.0f;
}

float TextInput::GetIntrinsicHeight() const
{
    if (m_renderer && m_fontHandle >= 0)
    {
        float lineH = static_cast<float>(m_renderer->GetLineHeight(m_fontHandle));
        return (std::max)(30.0f, lineH + 8.0f);
    }
    return 30.0f;
}

// ============================================================================
// テキスト設定
// ============================================================================

void TextInput::SetText(const std::wstring& text)
{
    m_text = text;
    m_cursorPos = static_cast<int>(text.size());
    ClearSelection();
    layoutDirty = true;
}

// ============================================================================
// 表示用テキスト（パスワードモード）
// ============================================================================

std::wstring TextInput::GetDisplayText() const
{
    if (m_passwordMode)
        return std::wstring(m_text.size(), L'*');
    return m_text;
}

// ============================================================================
// 選択操作のヘルパー
// ============================================================================

bool TextInput::HasSelection() const
{
    return m_selStart >= 0 && m_selEnd >= 0 && m_selStart != m_selEnd;
}

void TextInput::ClearSelection()
{
    m_selStart = -1;
    m_selEnd = -1;
}

void TextInput::SelectAll()
{
    m_selStart = 0;
    m_selEnd = static_cast<int>(m_text.size());
    m_cursorPos = m_selEnd;
}

void TextInput::DeleteSelection()
{
    if (!HasSelection()) return;

    int s = (std::min)(m_selStart, m_selEnd);
    int e = (std::max)(m_selStart, m_selEnd);
    m_text.erase(s, e - s);
    m_cursorPos = s;
    ClearSelection();

    if (onValueChanged)
        onValueChanged(WstringToUtf8(m_text));
}

void TextInput::InsertText(const std::wstring& str)
{
    if (str.empty()) return;

    if (HasSelection())
        DeleteSelection();

    // 最大文字数チェック
    if (m_maxLength > 0)
    {
        int remaining = m_maxLength - static_cast<int>(m_text.size());
        if (remaining <= 0) return;
        std::wstring toInsert = str.substr(0, remaining);
        m_text.insert(m_cursorPos, toInsert);
        m_cursorPos += static_cast<int>(toInsert.size());
    }
    else
    {
        m_text.insert(m_cursorPos, str);
        m_cursorPos += static_cast<int>(str.size());
    }

    if (onValueChanged)
        onValueChanged(WstringToUtf8(m_text));
}

// ============================================================================
// クリップボード
// ============================================================================

void TextInput::CopyToClipboard()
{
    if (!HasSelection()) return;

    int s = (std::min)(m_selStart, m_selEnd);
    int e = (std::max)(m_selStart, m_selEnd);
    std::wstring selected = m_text.substr(s, e - s);

    if (!OpenClipboard(nullptr)) return;
    EmptyClipboard();

    size_t bytes = (selected.size() + 1) * sizeof(wchar_t);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (hMem)
    {
        void* pMem = GlobalLock(hMem);
        memcpy(pMem, selected.c_str(), bytes);
        GlobalUnlock(hMem);
        SetClipboardData(CF_UNICODETEXT, hMem);
    }
    CloseClipboard();
}

void TextInput::PasteFromClipboard()
{
    if (!OpenClipboard(nullptr)) return;

    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (hData)
    {
        wchar_t* pData = static_cast<wchar_t*>(GlobalLock(hData));
        if (pData)
        {
            std::wstring pasteText = pData;
            GlobalUnlock(hData);

            // 改行を除去
            std::wstring filtered;
            filtered.reserve(pasteText.size());
            for (wchar_t ch : pasteText)
            {
                if (ch != L'\r' && ch != L'\n')
                    filtered += ch;
            }

            InsertText(filtered);
        }
    }
    CloseClipboard();
}

void TextInput::CutToClipboard()
{
    CopyToClipboard();
    DeleteSelection();
}

// ============================================================================
// HitTestCursor — X座標 → テキスト位置
// ============================================================================

int TextInput::HitTestCursor(float localX) const
{
    if (!m_renderer || m_fontHandle < 0) return 0;

    std::wstring display = GetDisplayText();
    if (display.empty()) return 0;

    float padLeft = computedStyle.padding.left;
    float adjustedX = localX - padLeft + m_scrollOffsetX;

    float prevWidth = 0.0f;
    for (int i = 1; i <= static_cast<int>(display.size()); ++i)
    {
        float w = static_cast<float>(m_renderer->GetTextWidth(m_fontHandle, display.substr(0, i)));
        float mid = (prevWidth + w) * 0.5f;
        if (adjustedX < mid)
            return i - 1;
        prevWidth = w;
    }
    return static_cast<int>(display.size());
}

// ============================================================================
// GetCursorX — カーソルの描画X位置（テキスト先頭からの距離）
// ============================================================================

float TextInput::GetCursorX() const
{
    if (!m_renderer || m_fontHandle < 0 || m_cursorPos == 0) return 0.0f;

    std::wstring display = GetDisplayText();
    return static_cast<float>(m_renderer->GetTextWidth(m_fontHandle,
        display.substr(0, m_cursorPos)));
}

// ============================================================================
// EnsureCursorVisible — スクロール調整
// ============================================================================

void TextInput::EnsureCursorVisible()
{
    float cursorX = GetCursorX();
    float padLeft = computedStyle.padding.left;
    float padRight = computedStyle.padding.right;
    float viewWidth = globalRect.width - padLeft - padRight;
    if (viewWidth <= 0.0f) return;

    if (cursorX - m_scrollOffsetX < 0.0f)
        m_scrollOffsetX = cursorX;
    else if (cursorX - m_scrollOffsetX > viewWidth)
        m_scrollOffsetX = cursorX - viewWidth;
}

// ============================================================================
// イベント処理
// ============================================================================

bool TextInput::OnEvent(const UIEvent& event)
{
    Widget::OnEvent(event);

    if (!enabled) return false;

    bool shiftHeld = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    bool ctrlHeld = (GetKeyState(VK_CONTROL) & 0x8000) != 0;

    switch (event.type)
    {
    case UIEventType::CharInput:
    {
        // 制御文字は無視
        if (event.charCode < 0x20) return false;

        std::wstring ch(1, event.charCode);
        InsertText(ch);
        EnsureCursorVisible();
        m_blinkTimer = 0.0f;
        m_cursorVisible = true;
        return true;
    }

    case UIEventType::KeyDown:
    {
        int vk = event.keyCode;

        // Ctrl 組み合わせ
        if (ctrlHeld)
        {
            if (vk == 'A') { SelectAll(); return true; }
            if (vk == 'C') { CopyToClipboard(); return true; }
            if (vk == 'V') { PasteFromClipboard(); EnsureCursorVisible(); m_blinkTimer = 0.0f; m_cursorVisible = true; return true; }
            if (vk == 'X') { CutToClipboard(); EnsureCursorVisible(); m_blinkTimer = 0.0f; m_cursorVisible = true; return true; }
        }

        if (vk == VK_LEFT)
        {
            if (HasSelection() && !shiftHeld)
            {
                m_cursorPos = (std::min)(m_selStart, m_selEnd);
                ClearSelection();
            }
            else
            {
                if (shiftHeld && !HasSelection())
                    m_selStart = m_cursorPos;
                if (m_cursorPos > 0) m_cursorPos--;
                if (shiftHeld)
                    m_selEnd = m_cursorPos;
                else
                    ClearSelection();
            }
            EnsureCursorVisible();
            m_blinkTimer = 0.0f;
            m_cursorVisible = true;
            return true;
        }

        if (vk == VK_RIGHT)
        {
            if (HasSelection() && !shiftHeld)
            {
                m_cursorPos = (std::max)(m_selStart, m_selEnd);
                ClearSelection();
            }
            else
            {
                if (shiftHeld && !HasSelection())
                    m_selStart = m_cursorPos;
                if (m_cursorPos < static_cast<int>(m_text.size())) m_cursorPos++;
                if (shiftHeld)
                    m_selEnd = m_cursorPos;
                else
                    ClearSelection();
            }
            EnsureCursorVisible();
            m_blinkTimer = 0.0f;
            m_cursorVisible = true;
            return true;
        }

        if (vk == VK_HOME)
        {
            if (shiftHeld && !HasSelection())
                m_selStart = m_cursorPos;
            m_cursorPos = 0;
            if (shiftHeld)
                m_selEnd = m_cursorPos;
            else
                ClearSelection();
            EnsureCursorVisible();
            m_blinkTimer = 0.0f;
            m_cursorVisible = true;
            return true;
        }

        if (vk == VK_END)
        {
            if (shiftHeld && !HasSelection())
                m_selStart = m_cursorPos;
            m_cursorPos = static_cast<int>(m_text.size());
            if (shiftHeld)
                m_selEnd = m_cursorPos;
            else
                ClearSelection();
            EnsureCursorVisible();
            m_blinkTimer = 0.0f;
            m_cursorVisible = true;
            return true;
        }

        if (vk == VK_BACK)
        {
            if (HasSelection())
            {
                DeleteSelection();
            }
            else if (m_cursorPos > 0)
            {
                m_text.erase(m_cursorPos - 1, 1);
                m_cursorPos--;
                if (onValueChanged)
                    onValueChanged(WstringToUtf8(m_text));
            }
            EnsureCursorVisible();
            m_blinkTimer = 0.0f;
            m_cursorVisible = true;
            return true;
        }

        if (vk == VK_DELETE)
        {
            if (HasSelection())
            {
                DeleteSelection();
            }
            else if (m_cursorPos < static_cast<int>(m_text.size()))
            {
                m_text.erase(m_cursorPos, 1);
                if (onValueChanged)
                    onValueChanged(WstringToUtf8(m_text));
            }
            EnsureCursorVisible();
            m_blinkTimer = 0.0f;
            m_cursorVisible = true;
            return true;
        }

        if (vk == VK_RETURN)
        {
            if (onSubmit)
                onSubmit();
            return true;
        }

        if (vk == VK_ESCAPE)
        {
            // フォーカス解除は UIContext に任せる
            return false;
        }

        break;
    }

    case UIEventType::MouseDown:
    {
        float localX = event.mouseX - globalRect.x;
        int pos = HitTestCursor(localX);

        if (shiftHeld)
        {
            if (!HasSelection())
                m_selStart = m_cursorPos;
            m_cursorPos = pos;
            m_selEnd = m_cursorPos;
        }
        else
        {
            m_cursorPos = pos;
            ClearSelection();
            m_selStart = pos;
        }
        m_selecting = true;
        m_blinkTimer = 0.0f;
        m_cursorVisible = true;
        EnsureCursorVisible();
        return true;
    }

    case UIEventType::MouseMove:
    {
        if (m_selecting)
        {
            float localX = event.mouseX - globalRect.x;
            int pos = HitTestCursor(localX);
            m_cursorPos = pos;
            m_selEnd = pos;
            EnsureCursorVisible();
            return true;
        }
        break;
    }

    case UIEventType::MouseUp:
    {
        if (m_selecting)
        {
            m_selecting = false;
            // 選択が同じ位置ならクリア
            if (m_selStart == m_selEnd)
                ClearSelection();
            return true;
        }
        break;
    }

    case UIEventType::FocusGained:
    {
        m_blinkTimer = 0.0f;
        m_cursorVisible = true;
        return true;
    }

    case UIEventType::FocusLost:
    {
        ClearSelection();
        m_selecting = false;
        return true;
    }

    default:
        break;
    }

    return false;
}

// ============================================================================
// 更新
// ============================================================================

void TextInput::Update(float deltaTime)
{
    Widget::Update(deltaTime);

    if (focused)
    {
        m_blinkTimer += deltaTime;
        if (m_blinkTimer >= 0.53f)
        {
            m_blinkTimer -= 0.53f;
            m_cursorVisible = !m_cursorVisible;
        }
    }
}

// ============================================================================
// 描画
// ============================================================================

void TextInput::Render(UIRenderer& renderer)
{
    const Style& drawStyle = GetRenderStyle();
    // 背景 + 枠線
    renderer.DrawRect(globalRect, drawStyle, opacity);

    // クリッピング
    renderer.PushScissor(globalRect);

    float padLeft = computedStyle.padding.left;
    float padRight = computedStyle.padding.right;
    float textX = globalRect.x + padLeft - m_scrollOffsetX;
    float lineH = (m_renderer && m_fontHandle >= 0)
        ? static_cast<float>(m_renderer->GetLineHeight(m_fontHandle))
        : 16.0f;
    float textY = globalRect.y + (globalRect.height - lineH) * 0.5f;

    if (m_text.empty() && !focused)
    {
        // プレースホルダー
        if (!m_placeholder.empty() && m_fontHandle >= 0)
        {
            StyleColor placeholderColor = { 0.5f, 0.5f, 0.55f, 0.6f };
        renderer.DrawText(textX, textY, m_fontHandle, m_placeholder, placeholderColor, opacity);
        }
    }
    else
    {
        std::wstring display = GetDisplayText();

        // 選択ハイライト
        if (HasSelection() && m_fontHandle >= 0)
        {
            int s = (std::min)(m_selStart, m_selEnd);
            int e = (std::max)(m_selStart, m_selEnd);
            float selStartX = (s > 0)
                ? static_cast<float>(renderer.GetTextWidth(m_fontHandle, display.substr(0, s)))
                : 0.0f;
            float selEndX = static_cast<float>(renderer.GetTextWidth(m_fontHandle, display.substr(0, e)));

            float highlightX = globalRect.x + padLeft + selStartX - m_scrollOffsetX;
            float highlightW = selEndX - selStartX;
            StyleColor selColor = { 0.3f, 0.5f, 0.8f, 0.5f };
            renderer.DrawSolidRect(highlightX, globalRect.y + 2.0f,
                                    highlightW, globalRect.height - 4.0f, selColor);
        }

        // テキスト描画
        if (!display.empty() && m_fontHandle >= 0)
        {
            StyleColor textColor = drawStyle.color;
            renderer.DrawText(textX, textY, m_fontHandle, display, textColor, opacity);
        }
    }

    // カーソル
    if (focused && m_cursorVisible)
    {
        float cursorScreenX = globalRect.x + padLeft + GetCursorX() - m_scrollOffsetX;
        StyleColor cursorColor = drawStyle.color;
        renderer.DrawSolidRect(cursorScreenX, globalRect.y + 4.0f,
                                1.5f, globalRect.height - 8.0f, cursorColor);
    }

    renderer.PopScissor();
}

}} // namespace GX::GUI
