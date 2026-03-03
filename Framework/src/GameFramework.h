#pragma once
/// @file GameFramework.h
/// @brief 高レベルゲームフレームワーク
///
/// GameBase を継承してゲームロジックだけ書けばよい。
/// エンジン初期化・メインループ・レンダリングパイプラインは
/// GameFramework が全て管理する。
///
/// @code
///   class MyGame : public gx::GameBase { ... };
///   GX_GAME_MAIN(MyGame)
/// @endcode

#include "pch.h"
#include "Math/MathConvert.h"
#include "Core/Application.h"
#include "Core/Logger.h"
#include "Graphics/Device/GraphicsDevice.h"
#include "Graphics/Device/CommandQueue.h"
#include "Graphics/Device/CommandList.h"
#include "Graphics/Device/SwapChain.h"
#include "Graphics/Rendering/SpriteBatch.h"
#include "Graphics/Rendering/PrimitiveBatch.h"
#include "Graphics/Rendering/FontManager.h"
#include "Graphics/Rendering/TextRenderer.h"
#include "Graphics/3D/Renderer3D.h"
#include "Graphics/3D/Camera3D.h"
#include "Graphics/3D/Light.h"
#include "Graphics/3D/SceneRenderer.h"
#include "Graphics/PostEffect/PostEffectPipeline.h"
#include "Graphics/RenderProfile.h"
#include "Graphics/QualitySettings.h"
#include "Input/InputManager.h"
#include "Audio/AudioManager.h"
#include "Core/Scene/Scene.h"
#include "Core/Scene/SceneManager.h"
#include "Core/Scene/Components.h"
#include "Graphics/3D/GraphicsComponents.h"
#include "Compat/DebugOverlay.h"
#include "Compat/ImGuiManager.h"

namespace gx
{

class GameFramework;

/// @brief ユーザーが継承してゲームロジックを書く基底クラス
///
/// GameFramework がエンジンを初期化し、デフォルトシーン
/// （カメラ + DirectionalLight + スカイ + 雲）を配置した状態で OnStart() を呼ぶ。
class GameBase
{
public:
    virtual ~GameBase() = default;

    /// @brief シーン初期化後に1度呼ばれる
    virtual void OnStart() {}

    /// @brief 毎フレーム更新
    virtual void OnUpdate(float dt) {}

    /// @brief Update後（カメラ追従等）
    virtual void OnLateUpdate(float dt) {}

    /// @brief カスタム3D描画（Renderer3D::Begin～End 内）
    virtual void OnDraw3D() {}

    /// @brief 2D/UI描画（SpriteBatch/PrimitiveBatch）
    virtual void OnDraw2D() {}

    /// @brief 終了時
    virtual void OnDestroy() {}

    // --- サブシステムアクセス ---

    Scene&               GetScene();
    Camera3D&            GetMainCamera();
    InputManager&        GetInput();
    Renderer3D&          GetRenderer();
    AudioManager&        GetAudio();
    PostEffectPipeline&  GetPostEffects();
    SpriteBatch&         GetSpriteBatch();
    PrimitiveBatch&      GetPrimitiveBatch();
    TextRenderer&        GetText();
    FontManager&         GetFontManager();
    SceneManager&        GetSceneManager();
    Window&              GetWindow();

    float GetDeltaTime() const;
    float GetTotalTime() const;
    uint32_t GetScreenWidth() const;
    uint32_t GetScreenHeight() const;

    /// @brief デバッグオーバーレイの表示/非表示を切り替える
    void ToggleDebugOverlay();

private:
    friend class GameFramework;
    GameFramework* m_framework = nullptr;
};

/// @brief エンジン初期化・メインループ・レンダリングパイプライン管理
///
/// Sandbox と同等のフルパイプラインを内包する。
/// SetupDefaultScene() でデフォルトシーンを構築する。
class GameFramework
{
public:
    struct Config
    {
        WString  title      = L"GXLib Framework";
        uint32_t width      = 1280;
        uint32_t height     = 720;
        bool     vsync      = true;
        bool     fullscreen = false;
    };

    /// @brief エンジンを起動してメインループを実行する
    /// @return 終了コード (0=正常)
    int Run(GameBase& game, const Config& config = {});

private:
    bool Initialize(const Config& config);
    bool InitializeGraphics();
    bool InitializeRenderers();
    void SetupDefaultScene();
    void SetupDefaultSky();
    void MainLoop(GameBase& game);
    void RenderFrame(GameBase& game, float deltaTime);
    void Shutdown(GameBase& game);

    // --- サブシステム ---
    Application          m_app;
    GraphicsDevice       m_graphicsDevice;
    CommandQueue         m_commandQueue;
    CommandList          m_commandList;
    SwapChain            m_swapChain;
    Renderer3D           m_renderer3D;
    PostEffectPipeline   m_postEffect;
    SpriteBatch          m_spriteBatch;
    PrimitiveBatch       m_primitiveBatch;
    FontManager          m_fontManager;
    TextRenderer         m_textRenderer;
    InputManager         m_inputManager;
    AudioManager         m_audioManager;
    SceneManager         m_sceneManager;
    SceneRenderer        m_sceneRenderer;
    Camera3D             m_mainCamera;
    Scene                m_defaultScene{"Main"};
    QualitySettings      m_qualitySettings;

    // --- デバッグオーバーレイ ---
    ImGuiManager         m_imguiManager;
    DebugOverlay         m_debugOverlay;

    // --- 状態 ---
    Vector3              m_ambientColor = { 0.15f, 0.15f, 0.2f };
    Vector3              m_sunDirection = { 0.3f, -1.0f, 0.4f };
    ID3D12Device*        m_device       = nullptr;
    uint32_t             m_screenWidth  = 0;
    uint32_t             m_screenHeight = 0;
    float                m_totalTime    = 0.0f;
    float                m_deltaTime    = 0.0f;
    bool                 m_vsync        = true;
    int                  m_defaultFont  = -1;

    uint64_t m_frameFenceValues[SwapChain::k_BufferCount] = {};
    uint32_t m_frameIndex = 0;

    friend class GameBase;
};

/// @brief WinMain マクロ — ユーザーはこれだけ書けば起動する
#define GX_GAME_MAIN(GameClass)                          \
    int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) \
    {                                                    \
        GameClass game;                                  \
        gx::GameFramework framework;                     \
        return framework.Run(game);                      \
    }

} // namespace gx
