#pragma once
/// @file Application.h
/// @brief アプリケーションライフサイクル管理
///
/// 【初学者向け解説】
/// ゲームアプリケーションには共通のライフサイクルがあります：
/// 1. 初期化（Initialize）: ウィンドウ作成、グラフィックス初期化
/// 2. メインループ（Run）: 毎フレーム更新と描画を繰り返す
/// 3. 終了処理（Shutdown）: リソース解放
///
/// このApplicationクラスは、WindowとTimerを統合し、
/// メインループの骨格を提供します。
/// ユーザーはコールバック関数を登録するだけでゲームロジックを実装できます。

#include "pch.h"
#include "Core/Window.h"
#include "Core/Timer.h"

namespace GX
{

/// @brief アプリケーション初期化設定
struct ApplicationDesc
{
    std::wstring title  = L"GXLib Application";
    uint32_t     width  = 1280;
    uint32_t     height = 720;
};

/// @brief アプリケーションライフサイクルを管理するクラス
class Application
{
public:
    Application() = default;
    ~Application() = default;

    /// アプリケーションを初期化（ウィンドウ作成）
    bool Initialize(const ApplicationDesc& desc);

    /// メインループを実行（ウィンドウが閉じるまで）
    void Run(std::function<void(float)> updateCallback);

    /// アプリケーションを終了
    void Shutdown();

    /// ウィンドウを取得
    Window& GetWindow() { return m_window; }

    /// タイマーを取得
    Timer& GetTimer() { return m_timer; }

private:
    Window m_window;
    Timer  m_timer;
    bool   m_running = false;
};

} // namespace GX
