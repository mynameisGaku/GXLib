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
    std::wstring title  = L"GXLib Application";  ///< ウィンドウタイトル
    uint32_t     width  = 1280;                   ///< クライアント領域の幅（ピクセル）
    uint32_t     height = 720;                    ///< クライアント領域の高さ（ピクセル）
};

/// @brief アプリケーションライフサイクルを管理するクラス
class Application
{
public:
    /// @brief デフォルトコンストラクタ
    Application() = default;

    /// @brief デストラクタ
    ~Application() = default;

    /// @brief アプリケーションを初期化する（ウィンドウ作成）
    /// @param desc アプリケーションの初期化設定（タイトル、幅、高さ）
    /// @return 初期化に成功した場合true、失敗した場合false
    bool Initialize(const ApplicationDesc& desc);

    /// @brief メインループを実行する（ウィンドウが閉じるまで繰り返す）
    /// @param updateCallback 毎フレーム呼び出されるコールバック関数（引数はデルタタイム秒）
    void Run(std::function<void(float)> updateCallback);

    /// @brief アプリケーションを終了し、リソースを解放する
    void Shutdown();

    /// @brief ウィンドウオブジェクトへの参照を取得する
    /// @return Windowオブジェクトの参照
    Window& GetWindow() { return m_window; }

    /// @brief タイマーオブジェクトへの参照を取得する
    /// @return Timerオブジェクトの参照
    Timer& GetTimer() { return m_timer; }

private:
    Window m_window;
    Timer  m_timer;
    bool   m_running = false;
};

} // namespace GX
