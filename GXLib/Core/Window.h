#pragma once
/// @file Window.h
/// @brief Win32 ウィンドウの作成・メッセージ処理を担当する
///
/// DxLib ではウィンドウ生成は内部で自動的に行われるが、
/// GXLib では Window クラスを通じて明示的に管理する。
/// DxLib の SetGraphMode() / ChangeWindowMode() に相当する設定は
/// WindowDesc 構造体で指定する。

#include "pch.h"

namespace GX
{

/// @brief ウィンドウ作成時の設定
/// DxLib の SetGraphMode(width, height, 32) + SetMainWindowText() に対応する
struct WindowDesc
{
    std::wstring title  = L"GXLib Application";   ///< ウィンドウタイトル
    uint32_t     width  = 1280;                    ///< クライアント領域の幅（ピクセル）
    uint32_t     height = 720;                     ///< クライアント領域の高さ（ピクセル）
};

/// @brief Win32 ウィンドウの生成・破棄・メッセージポンプを管理するクラス
/// DxLib では裏側に隠れているウィンドウ処理を、ここで一手に引き受ける。
class Window
{
public:
    Window() = default;

    /// @brief デストラクタ。HWND が残っていれば DestroyWindow() で破棄する
    ~Window();

    /// @brief ウィンドウを作成して画面中央に表示する
    /// @param desc 作成設定（タイトル・幅・高さ）
    /// @return 成功なら true
    bool Initialize(const WindowDesc& desc);

    /// @brief 溜まっている Win32 メッセージを処理する
    /// DxLib の ProcessMessage() に相当。毎フレーム呼ぶ必要がある。
    /// @return ウィンドウが生きていれば true、閉じられたら false
    bool ProcessMessages();

    /// @brief ウィンドウハンドルを取得する
    /// @return HWND（DirectX デバイス作成時などに必要）
    HWND GetHWND() const { return m_hwnd; }

    /// @brief クライアント領域の幅を取得する
    /// @return 幅（ピクセル）
    uint32_t GetWidth() const { return m_width; }

    /// @brief クライアント領域の高さを取得する
    /// @return 高さ（ピクセル）
    uint32_t GetHeight() const { return m_height; }

    /// @brief ウィンドウリサイズ時に呼ばれるコールバックを登録する
    /// SwapChain やレンダーターゲットの再生成に使う。
    /// @param callback 新しい幅・高さを受け取る関数
    void SetResizeCallback(std::function<void(uint32_t, uint32_t)> callback);

    /// @brief Win32 メッセージを横取りするコールバックを追加する
    /// InputManager などが WM_KEYDOWN 等を受け取るために使う。
    /// @param callback true を返すとそのメッセージは処理済み扱い
    void AddMessageCallback(std::function<bool(HWND, UINT, WPARAM, LPARAM)> callback);

    /// @brief ウィンドウタイトルを変更する
    /// @param title 新しいタイトル文字列
    void SetTitle(const std::wstring& title);

private:
    /// Win32 ウィンドウプロシージャ。GWLP_USERDATA に this を格納して
    /// static 関数からメンバにアクセスする定番パターン。
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HWND     m_hwnd   = nullptr;
    uint32_t m_width  = 0;
    uint32_t m_height = 0;

    std::function<void(uint32_t, uint32_t)> m_resizeCallback;                           ///< リサイズコールバック
    std::vector<std::function<bool(HWND, UINT, WPARAM, LPARAM)>> m_messageCallbacks;    ///< メッセージフックリスト
};

} // namespace GX
