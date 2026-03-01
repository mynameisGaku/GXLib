/// @file IMEHandler.cpp
/// @brief IME入力ハンドラーの実装（Win32 IMM32 API）
#include "pch_common.h"
#include "Input/IMEHandler.h"
#include <imm.h>

#pragma comment(lib, "imm32.lib")

namespace gx
{

// ============================================================
// コンストラクタ / デストラクタ
// ============================================================
IMEHandler::IMEHandler()  = default;
IMEHandler::~IMEHandler() { Shutdown(); }

// ============================================================
// Initialize / Shutdown
// ============================================================
bool IMEHandler::Initialize(HWND hwnd)
{
    if (!hwnd) return false;

    m_hwnd        = hwnd;
    m_initialized = true;
    m_state       = IMEState::Inactive;
    m_compositionString.clear();
    m_committedText.clear();
    m_cursorPos = 0;
    m_candidates = {};

    return true;
}

void IMEHandler::Shutdown()
{
    m_initialized = false;
    m_hwnd        = nullptr;
    m_state       = IMEState::Inactive;
    m_compositionString.clear();
    m_committedText.clear();
    m_cursorPos = 0;
    m_candidates = {};
}

// ============================================================
// ProcessMessage
// ============================================================
bool IMEHandler::ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (!m_initialized || !m_hwnd) return false;

    switch (msg)
    {
    case WM_IME_STARTCOMPOSITION:
        m_state = IMEState::Composing;
        m_compositionString.clear();
        m_cursorPos = 0;
        return true;

    case WM_IME_COMPOSITION:
    {
        HIMC himc = ImmGetContext(m_hwnd);
        if (!himc) return false;

        // 確定文字列の取得（GCS_RESULTSTR は変換確定時にセットされる）
        if (lParam & GCS_RESULTSTR)
        {
            const int byteLen = ImmGetCompositionStringW(himc, GCS_RESULTSTR, nullptr, 0);
            if (byteLen > 0)
            {
                std::wstring result;
                result.resize(byteLen / sizeof(wchar_t));
                ImmGetCompositionStringW(himc, GCS_RESULTSTR, result.data(), byteLen);
                m_committedText += result;
                m_state = IMEState::Committed;
            }
            // 確定後はコンポジション文字列をクリア
            m_compositionString.clear();
            m_cursorPos = 0;
        }

        // コンポジション文字列の更新
        if (lParam & GCS_COMPSTR)
        {
            UpdateCompositionString();
        }

        // カーソル位置の更新
        if (lParam & GCS_CURSORPOS)
        {
            m_cursorPos = static_cast<uint32_t>(
                ImmGetCompositionStringW(himc, GCS_CURSORPOS, nullptr, 0));
        }

        ImmReleaseContext(m_hwnd, himc);
        return true;
    }

    case WM_IME_ENDCOMPOSITION:
        m_state = IMEState::Inactive;
        m_compositionString.clear();
        m_cursorPos = 0;
        return true;

    case WM_IME_NOTIFY:
        if (wParam == IMN_CHANGECANDIDATE || wParam == IMN_OPENCANDIDATE)
        {
            UpdateCandidateList();
            return true;
        }
        if (wParam == IMN_CLOSECANDIDATE)
        {
            m_candidates = {};
            return true;
        }
        return false;

    default:
        return false;
    }
}

// ============================================================
// ClearCommitted
// ============================================================
void IMEHandler::ClearCommitted()
{
    m_committedText.clear();
    if (m_state == IMEState::Committed)
        m_state = IMEState::Inactive;
}

// ============================================================
// SetCompositionPosition
// ============================================================
void IMEHandler::SetCompositionPosition(int x, int y)
{
    if (!m_initialized || !m_hwnd) return;

    HIMC himc = ImmGetContext(m_hwnd);
    if (!himc) return;

    COMPOSITIONFORM cf = {};
    cf.dwStyle        = CFS_POINT;
    cf.ptCurrentPos.x = static_cast<LONG>(x);
    cf.ptCurrentPos.y = static_cast<LONG>(y);
    ImmSetCompositionWindow(himc, &cf);

    ImmReleaseContext(m_hwnd, himc);
}

// ============================================================
// UpdateCompositionString（private）
// ============================================================
void IMEHandler::UpdateCompositionString()
{
    HIMC himc = ImmGetContext(m_hwnd);
    if (!himc) return;

    const int byteLen = ImmGetCompositionStringW(himc, GCS_COMPSTR, nullptr, 0);
    if (byteLen > 0)
    {
        m_compositionString.resize(byteLen / sizeof(wchar_t));
        ImmGetCompositionStringW(himc, GCS_COMPSTR, m_compositionString.data(), byteLen);
    }
    else
    {
        m_compositionString.clear();
    }

    // カーソル位置も同時に更新
    m_cursorPos = static_cast<uint32_t>(
        ImmGetCompositionStringW(himc, GCS_CURSORPOS, nullptr, 0));

    ImmReleaseContext(m_hwnd, himc);
}

// ============================================================
// UpdateCandidateList（private）
// ============================================================
void IMEHandler::UpdateCandidateList()
{
    HIMC himc = ImmGetContext(m_hwnd);
    if (!himc) return;

    const DWORD size = ImmGetCandidateListW(himc, 0, nullptr, 0);
    if (size > 0)
    {
        std::vector<uint8_t> buf(size);
        auto* list = reinterpret_cast<CANDIDATELIST*>(buf.data());
        ImmGetCandidateListW(himc, 0, list, size);

        m_candidates.candidates.clear();
        m_candidates.candidates.reserve(list->dwCount);
        for (DWORD i = 0; i < list->dwCount; ++i)
        {
            const auto* str = reinterpret_cast<const wchar_t*>(buf.data() + list->dwOffset[i]);
            m_candidates.candidates.emplace_back(str);
        }

        m_candidates.selectedIndex = static_cast<uint32_t>(list->dwSelection);
        m_candidates.pageStart     = static_cast<uint32_t>(list->dwPageStart);
        m_candidates.pageSize      = static_cast<uint32_t>(list->dwPageSize);
    }
    else
    {
        m_candidates = {};
    }

    ImmReleaseContext(m_hwnd, himc);
}

} // namespace gx
