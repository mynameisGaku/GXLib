#pragma once
/// @file Window.h
/// @brief Win32ウィンドウ管理クラス
///
/// 【初学者向け解説】
/// Windows上でグラフィックスを表示するには、まず「ウィンドウ」を作る必要があります。
/// Win32 APIを使ってウィンドウを作成し、ユーザーの操作（クリック、キー入力、
/// ウィンドウの移動やリサイズなど）を処理します。
///
/// ウィンドウプロシージャ（WndProc）は、Windowsがウィンドウに送るメッセージ
/// （イベント）を処理するコールバック関数です。例えば：
/// - WM_SIZE: ウィンドウがリサイズされた
/// - WM_DESTROY: ウィンドウが閉じられた
/// - WM_KEYDOWN: キーが押された

#include "pch.h"

namespace GX
{

/// @brief ウィンドウ作成設定
struct WindowDesc
{
    std::wstring title  = L"GXLib Application";   ///< ウィンドウタイトル
    uint32_t     width  = 1280;                    ///< クライアント領域の幅
    uint32_t     height = 720;                     ///< クライアント領域の高さ
};

/// @brief Win32ウィンドウを管理するクラス
class Window
{
public:
    /// @brief デフォルトコンストラクタ
    Window() = default;

    /// @brief デストラクタ（ウィンドウを破棄する）
    ~Window();

    /// @brief ウィンドウを作成して表示する
    /// @param desc ウィンドウの作成設定（タイトル、幅、高さ）
    /// @return 作成に成功した場合true、失敗した場合false
    bool Initialize(const WindowDesc& desc);

    /// @brief ウィンドウメッセージを処理する
    /// @return ウィンドウが有効な場合true、閉じられた場合false（アプリ終了）
    bool ProcessMessages();

    /// @brief ウィンドウハンドル（HWND）を取得する
    /// @return Win32ウィンドウハンドル
    HWND GetHWND() const { return m_hwnd; }

    /// @brief クライアント領域の幅を取得する
    /// @return クライアント領域の幅（ピクセル）
    uint32_t GetWidth() const { return m_width; }

    /// @brief クライアント領域の高さを取得する
    /// @return クライアント領域の高さ（ピクセル）
    uint32_t GetHeight() const { return m_height; }

    /// @brief リサイズ時のコールバックを設定する
    /// @param callback ウィンドウリサイズ時に呼び出される関数（引数は新しい幅と高さ）
    void SetResizeCallback(std::function<void(uint32_t, uint32_t)> callback);

    /// @brief メッセージコールバックを追加する（Input等が使用）
    /// @param callback Windowsメッセージを受け取るコールバック関数。trueを返すとメッセージは処理済みとみなされる
    void AddMessageCallback(std::function<bool(HWND, UINT, WPARAM, LPARAM)> callback);

    /// @brief ウィンドウタイトルを設定する
    /// @param title 新しいウィンドウタイトル文字列
    void SetTitle(const std::wstring& title);

private:
    /// ウィンドウプロシージャ（Windowsからのメッセージを処理）
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HWND     m_hwnd   = nullptr;
    uint32_t m_width  = 0;
    uint32_t m_height = 0;

    /// リサイズ時に呼ばれるコールバック関数
    std::function<void(uint32_t, uint32_t)> m_resizeCallback;

    /// 汎用メッセージコールバックリスト
    std::vector<std::function<bool(HWND, UINT, WPARAM, LPARAM)>> m_messageCallbacks;
};

} // namespace GX
