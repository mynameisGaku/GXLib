#pragma once
/// @file GX/App.h
/// @brief アプリケーション基底クラスとエントリポイントマクロ
///
/// 使用例:
/// @code
/// class MyGame : public gx::App {
///     gx::Config GetConfig() const override {
///         return { .title = "My Game", .width = 1280, .height = 720 };
///     }
///     void Draw() override {
///         gx::DrawCircle(400, 300, 50, gx::Color::Red);
///     }
/// };
/// GX_APP(MyGame)
/// @endcode

#include "GX/Config.h"

namespace gx {

class Renderer3D;
class Camera3D;
class PostEffectPipeline;
class InputManager;

/// @brief アプリケーション基底クラス
///
/// ライフサイクルメソッドをオーバーライドして使用する。
class App
{
public:
    virtual ~App() = default;

    /// @brief エンジン設定を返す（初期化前に呼ばれる）
    virtual Config GetConfig() const { return {}; }

    /// @brief 初期化前フック（SetGraphMode等を呼ぶ場合に使用）
    virtual void OnBoot() {}

    /// @brief 初期化完了後に1回呼ばれる
    virtual void Start() {}

    /// @brief 毎フレームの更新処理
    /// @param dt 前フレームからの経過時間（秒）
    virtual void Update(float dt) {}

    /// @brief 毎フレームの描画処理
    virtual void Draw() {}

    /// @brief 終了時のクリーンアップ
    virtual void Release() {}

    // --- サブシステムアクセス ---

    /// @brief 3Dレンダラーを取得する
    Renderer3D& GetRenderer3D();

    /// @brief 3Dカメラを取得する
    Camera3D& GetCamera3D();

    /// @brief ポストエフェクトパイプラインを取得する
    PostEffectPipeline& GetPostEffects();

    /// @brief 入力マネージャーを取得する
    InputManager& GetInput();
};

/// @brief アプリケーションを実行する
/// @param app アプリケーションインスタンス
/// @return 終了コード
int Run(App& app);

} // namespace gx

/// @brief WinMainエントリポイント生成マクロ
#define GX_APP(AppClass)                                          \
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)              \
{                                                                 \
    AppClass app;                                                 \
    return gx::Run(app);                                          \
}
