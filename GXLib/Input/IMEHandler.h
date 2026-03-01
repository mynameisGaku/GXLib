#pragma once
/// @file IMEHandler.h
/// @brief IME入力ハンドラー（Win32 IMM32 APIラッパー）
///
/// Windows IMM32 (Input Method Manager) APIをラップし、
/// 日本語・中国語・韓国語等のIME入力をゲームアプリケーションで扱うための機能を提供する。
/// DxLibのKeyInputString系関数に相当するが、より細かくIMEの状態を取得できる。
/// - 変換中文字列（コンポジション）の取得
/// - 確定文字列の取得
/// - 候補リストの取得
/// - カーソル位置の取得と変換ウィンドウ位置の設定
/// @addtogroup grp_input/// @{

#include "pch_common.h"

namespace gx
{

/// @brief IMEの現在の入力状態
enum class IMEState
{
    Inactive,   ///< IME未使用または変換完了後
    Composing,  ///< 変換中（未確定文字列あり）
    Committed   ///< 確定直後（CommittedTextが有効）
};

/// @brief IME候補リストの情報
struct IMECandidate
{
    std::vector<std::wstring> candidates; ///< 候補文字列のリスト
    uint32_t selectedIndex = 0;           ///< 現在選択中の候補インデックス
    uint32_t pageStart     = 0;           ///< 表示ページの先頭インデックス
    uint32_t pageSize      = 0;           ///< 1ページあたりの表示数
};

/// @brief Win32 IMM32 APIをラップしたIME入力ハンドラー
///
/// Window のメッセージループから WM_IME_* メッセージを受け取り、
/// IMEの状態をリアルタイムで追跡する。
/// UIContextやテキスト入力ウィジェットと連携して使用する。
///
/// @code
/// // 初期化
/// m_imeHandler.Initialize(hwnd);
///
/// // メッセージループ（Window::AddMessageCallback内）
/// if (m_imeHandler.ProcessMessage(msg, wParam, lParam)) return true;
///
/// // テキストフィールドでの使用
/// if (m_imeHandler.GetState() == IMEState::Committed) {
///     textField.Append(m_imeHandler.GetCommittedText());
///     m_imeHandler.ClearCommitted();
/// }
/// @endcode
class IMEHandler
{
public:
    IMEHandler();
    ~IMEHandler();

    /// @brief IMEハンドラーを初期化する
    /// @param hwnd ウィンドウハンドル
    /// @return 成功でtrue
    bool Initialize(HWND hwnd);

    /// @brief IMEハンドラーを終了する
    void Shutdown();

    /// @brief ウィンドウメッセージを処理する
    /// @param msg メッセージID
    /// @param wParam WPARAM
    /// @param lParam LPARAM
    /// @return メッセージを処理した場合true（DefWindowProcを呼ばない）
    bool ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    /// @brief 現在のIME状態を取得する
    /// @return IMEState列挙値
    IMEState GetState() const { return m_state; }

    /// @brief 変換中の文字列（コンポジション）を取得する
    /// @return 未確定文字列への参照。変換中でなければ空文字列
    const std::wstring& GetCompositionString() const { return m_compositionString; }

    /// @brief 確定された文字列を取得する
    /// @return 確定文字列への参照。確定直後でなければ空文字列
    const std::wstring& GetCommittedText() const { return m_committedText; }

    /// @brief コンポジション文字列内のカーソル位置を取得する
    /// @return カーソル位置（文字単位、0始まり）
    uint32_t GetCursorPosition() const { return m_cursorPos; }

    /// @brief 候補リスト情報を取得する
    /// @return IMECandidate構造体への参照
    const IMECandidate& GetCandidates() const { return m_candidates; }

    /// @brief 確定文字列をクリアし、状態をInactiveに戻す
    ///
    /// GetCommittedText()でテキストを受け取った後に呼び出す。
    void ClearCommitted();

    /// @brief IME変換ウィンドウの表示位置を設定する
    /// @param x ウィンドウクライアント座標のX
    /// @param y ウィンドウクライアント座標のY
    void SetCompositionPosition(int x, int y);

    /// @brief 初期化済みかどうかを取得する
    /// @return 初期化済みならtrue
    bool IsInitialized() const { return m_initialized; }

private:
    /// @brief ImmGetCompositionStringW でコンポジション文字列を取得・更新する
    void UpdateCompositionString();

    /// @brief ImmGetCandidateListW で候補リストを取得・更新する
    void UpdateCandidateList();

    HWND         m_hwnd              = nullptr;
    IMEState     m_state             = IMEState::Inactive;
    std::wstring m_compositionString;
    std::wstring m_committedText;
    uint32_t     m_cursorPos         = 0;
    IMECandidate m_candidates;
    bool         m_initialized       = false;
};

} // namespace gx
/// @}
