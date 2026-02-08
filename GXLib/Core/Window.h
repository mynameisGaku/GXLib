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
    Window() = default;
    ~Window();

    /// ウィンドウを作成して表示する
    bool Initialize(const WindowDesc& desc);

    /// ウィンドウメッセージを処理する（falseを返したらアプリ終了）
    bool ProcessMessages();

    /// ウィンドウハンドルを取得
    HWND GetHWND() const { return m_hwnd; }

    /// クライアント領域の幅を取得
    uint32_t GetWidth() const { return m_width; }

    /// クライアント領域の高さを取得
    uint32_t GetHeight() const { return m_height; }

    /// リサイズ時のコールバックを設定
    void SetResizeCallback(std::function<void(uint32_t, uint32_t)> callback);

    /// メッセージコールバックを追加（Input等が使用）
    /// コールバックがtrueを返したら、そのメッセージは処理済みとみなす
    void AddMessageCallback(std::function<bool(HWND, UINT, WPARAM, LPARAM)> callback);

    /// ウィンドウタイトルを設定
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
