#pragma once
/// @file Application.h
/// @brief アプリケーションのライフサイクル（初期化・メインループ・終了）を管理する
///
/// DxLib の DxLib_Init() / DxLib_End() に相当する機能を提供する。
/// DxLib ではメインループをユーザーが自前で書くが、GXLib では
/// Application::Run() にコールバックを渡す形式になっている。

#include "pch.h"
#include "Core/Window.h"
#include "Core/Timer.h"

namespace GX
{

/// @brief アプリケーション初期化設定
/// DxLib の SetGraphMode() / SetMainWindowText() に渡す情報をまとめた構造体
struct ApplicationDesc
{
    std::wstring title  = L"GXLib Application";  ///< ウィンドウタイトル
    uint32_t     width  = 1280;                   ///< クライアント領域の幅（ピクセル）
    uint32_t     height = 720;                    ///< クライアント領域の高さ（ピクセル）
};

/// @brief ゲームのメインループを管理するクラス
/// DxLib の DxLib_Init() 〜 DxLib_End() の一連の処理をまとめたもの。
/// 内部で Window と Timer を保持し、ウィンドウ作成からフレーム更新まで面倒を見る。
class Application
{
public:
    Application() = default;
    ~Application() = default;

    /// @brief アプリケーションを初期化する（ウィンドウ作成・タイマー開始）
    /// @param desc 初期化設定（タイトル・ウィンドウサイズ）
    /// @return 成功なら true
    bool Initialize(const ApplicationDesc& desc);

    /// @brief メインループを開始する
    /// ウィンドウが閉じられるまで毎フレーム updateCallback を呼び続ける。
    /// DxLib の ProcessMessage() + ScreenFlip() 相当のループを内部で回す。
    /// @param updateCallback 毎フレーム呼ばれる関数。引数はデルタタイム（秒）
    void Run(std::function<void(float)> updateCallback);

    /// @brief アプリケーションを終了する
    /// Run() のループを抜ける。DxLib の DxLib_End() に相当。
    void Shutdown();

    /// @brief 管理しているウィンドウを取得する
    /// @return Window への参照
    Window& GetWindow() { return m_window; }

    /// @brief 管理しているタイマーを取得する
    /// @return Timer への参照
    Timer& GetTimer() { return m_timer; }

private:
    Window m_window;
    Timer  m_timer;
    bool   m_running = false;  ///< Run() ループ中は true
};

} // namespace GX
