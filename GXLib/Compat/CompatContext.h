#pragma once
/// @file CompatContext.h
/// @brief 簡易API用内部コンテキスト（シングルトン）
///
/// 全GXLibサブシステムへの参照を保持し、簡易API関数から使用されます。
/// ユーザーが直接触ることは想定しません。

#include "pch.h"
#include "Compat/CompatTypes.h"
#include "Core/Application.h"
#include "Core/ServiceLocator.h"
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
#include "Math/Transform3D.h"
#include "Graphics/3D/Model.h"
#include "Graphics/3D/Animator.h"
#include "Graphics/3D/ModelLoader.h"
#include "Graphics/PostEffect/PostEffectPipeline.h"
#include "Graphics/Layer/LayerStack.h"
#include "Graphics/Layer/LayerCompositor.h"
#include "Input/InputManager.h"
#include "Audio/AudioManager.h"
#include "Audio/AudioEmitter.h"
#include "Audio/AudioListener.h"
#include "Graphics/Rendering/ParticleSystem2D.h"
#include "Compat/DebugOverlay.h"
#include "Compat/ImGuiManager.h"
#include "Graphics/QualitySettings.h"
#include "Graphics/Rendering/LoadingScreen.h"
#include "IO/Network/NetworkManager.h"

#include <thread>
#include <atomic>

namespace gx_internal
{
/// @addtogroup grp_compat
/// @{

/// @brief 簡易APIの内部コンテキスト（シングルトン）
///
/// 全GXLibサブシステムのインスタンスを保持し、
/// 簡易API関数（GXLib.h）から内部的に使用されます。
class CompatContext
{
public:
    /// @brief シングルトンインスタンスを取得する
    /// @return CompatContext の唯一のインスタンスへの参照
    static CompatContext& Instance();

    /// @brief 全サブシステムを初期化する
    /// @return 成功時 true、失敗時 false
    bool Initialize();

    /// @brief 全サブシステムを終了し、リソースを解放する
    void Shutdown();

    /// @brief ウィンドウメッセージ処理と入力状態の更新を行う
    /// @return 正常時 0、ウィンドウが閉じられた場合 -1
    int ProcessMessage();

    /// @brief 垂直同期の有効/無効を設定する
    void SetVSync(bool enabled) { vsyncEnabled = enabled; }

    // --- バッチ管理 ---

    /// @brief アクティブな描画バッチの種類
    enum class ActiveBatch
    {
        None,       ///< バッチ未開始
        Sprite,     ///< SpriteBatch がアクティブ
        Primitive   ///< PrimitiveBatch がアクティブ
    };

    /// @brief SpriteBatch をアクティブにする（PrimitiveBatch は自動フラッシュされる）
    void EnsureSpriteBatch();

    /// @brief PrimitiveBatch をアクティブにする（SpriteBatch は自動フラッシュされる）
    void EnsurePrimitiveBatch();

    /// @brief アクティブな全バッチをフラッシュして終了する
    void FlushAll();

    // --- フレーム管理 ---

    /// @brief フレーム描画を開始する（ClearDrawScreen から呼ばれる）
    void BeginFrame();

    /// @brief フレーム描画を終了し、画面に表示する（ScreenFlip から呼ばれる）
    void EndFrame();

    // --- GXLib サブシステム ---
    gx::Application         app;              ///< アプリケーション管理
    gx::GraphicsDevice       graphicsDevice;   ///< D3D12グラフィックスデバイス
    gx::CommandQueue         commandQueue;     ///< コマンドキュー
    gx::CommandList          commandList;      ///< コマンドリスト
    gx::SwapChain            swapChain;        ///< スワップチェーン

    gx::SpriteBatch          spriteBatch;      ///< 2Dスプライト描画バッチ
    gx::PrimitiveBatch       primBatch;        ///< 2Dプリミティブ描画バッチ
    gx::FontManager          fontManager;      ///< フォント管理
    gx::TextRenderer         textRenderer;     ///< テキスト描画
    gx::InputManager         inputManager;     ///< 入力管理
    gx::AudioManager         audioManager;     ///< サウンド管理

    gx::Renderer3D           renderer3D;       ///< 3Dレンダラー
    gx::Camera3D             camera;           ///< 3Dカメラ
    gx::PostEffectPipeline   postEffect;       ///< ポストエフェクトパイプライン

    // --- 2Dパーティクル ---
    gx::ParticleSystem2D     particleSystem2D; ///< 2Dパーティクルシステム

    // --- 3Dオーディオ ---
    gx::AudioEmitter         audioEmitter3D;   ///< 3Dサウンド用エミッター（簡易API用）
    gx::AudioListener        audioListener3D;  ///< 3Dサウンド用リスナー（簡易API用）

    // --- ネットワーク (lazy init — 初回アクセス時に生成) ---
    std::unique_ptr<gx::NetworkManager> networkManager;
    gx::NetworkManager& EnsureNetwork() {
        if (!networkManager) networkManager = std::make_unique<gx::NetworkManager>();
        return *networkManager;
    }

    // Receive callback (stored for Compat layer)
    std::function<void(int, const void*, int)> onNetReceive;

    // --- デバッグオーバーレイ ---
    gx::ImGuiManager   imguiManager;                        ///< ImGui ライフサイクル管理
    gx::DebugOverlay   debugOverlay;                        ///< エンジン内蔵デバッグ表示
    gx::QualitySettings qualitySettings;                    ///< グラフィックス品質設定

    // --- ポストエフェクトマスク ---
    gx::PostFXFlag postFXMask = gx::PostFXFlag::All;   ///< 簡易APIでのポストエフェクト適用マスク

    // --- 状態 ---
    int          drawScreen     = GX_SCREEN_BACK;       ///< 現在の描画先スクリーン
    int          drawBlendMode  = GX_BLENDMODE_NOBLEND; ///< 現在のブレンドモード
    int          drawBlendParam = 255;                   ///< 現在のブレンドパラメータ（0〜255）
    uint32_t     drawBright_r   = 255;  ///< 描画輝度 赤成分（0〜255）
    uint32_t     drawBright_g   = 255;  ///< 描画輝度 緑成分（0〜255）
    uint32_t     drawBright_b   = 255;  ///< 描画輝度 青成分（0〜255）
    int          defaultFontHandle = -1; ///< デフォルトフォントのハンドル
    ActiveBatch  activeBatch    = ActiveBatch::None; ///< 現在アクティブなバッチ種別
    bool         frameActive    = false; ///< フレーム描画中フラグ
    bool         vsyncEnabled   = false; ///< 垂直同期を有効にするか
    float        fadeInAlpha    = 0.0f; ///< フェードイン用アルファ (1=黒, 0=完了)
    uint32_t     bgColor_r      = 0;    ///< 背景色 赤成分（0〜255）
    uint32_t     bgColor_g      = 0;    ///< 背景色 緑成分（0〜255）
    uint32_t     bgColor_b      = 0;    ///< 背景色 青成分（0〜255）

    // --- 3D モデルレジストリ ---
    static constexpr int k_MaxModels = 256;

    /// @brief 3Dモデルの管理エントリ
    struct ModelEntry
    {
        std::unique_ptr<gx::Model> model;  ///< モデルデータ
        gx::Transform3D transform;          ///< ワールド変換
        gx::Animator    animator;           ///< スキンメッシュ用アニメータ
        bool valid = false;                 ///< エントリが有効か

        ModelEntry() = default;
        ModelEntry(ModelEntry&&) = default;
        ModelEntry& operator=(ModelEntry&&) = default;
    };
    gx::Vector<ModelEntry> models;          ///< モデルエントリの配列
    gx::Vector<int>        modelFreeHandles; ///< 再利用可能なハンドルのフリーリスト
    int                     modelNextHandle = 0; ///< 次に割り当てるハンドル番号
    gx::ModelLoader          modelLoader;    ///< モデルローダー

    /// @brief 新しいモデルハンドルを割り当てる（フリーリストから再利用または新規作成）
    /// @return 割り当てられたモデルハンドル
    int AllocateModelHandle();

    // --- 3Dシーン管理 ---
    bool m_3dSceneActive = false;  ///< 3Dシーン描画中フラグ

    // --- 内部参照 ---
    ID3D12Device*              device     = nullptr; ///< D3D12デバイスへのポインタ
    ID3D12GraphicsCommandList* cmdList    = nullptr; ///< 現在のコマンドリストへのポインタ
    uint32_t                   frameIndex = 0;       ///< 現在のフレームインデックス
    uint32_t                   screenWidth  = 1280;  ///< スクリーン幅（ピクセル）
    uint32_t                   screenHeight = 720;   ///< スクリーン高さ（ピクセル）

    // --- ウィンドウ作成前の設定 ---
    bool     windowMode   = true;   ///< ウィンドウモードフラグ（true でウィンドウモード）
    int      graphWidth   = 1280;   ///< 画面幅の設定値
    int      graphHeight  = 720;    ///< 画面高さの設定値
    int      graphColorBit = 32;    ///< 色深度の設定値
    gx::String  windowTitle = "GXLib Application"; ///< ウィンドウタイトル

    // --- ロード画面用コマンドリスト ---
    gx::CommandList      m_loadingCmdList; ///< ロード画面専用コマンドリスト

private:
    CompatContext() = default;
    ~CompatContext() = default;
    CompatContext(const CompatContext&) = delete;
    CompatContext& operator=(const CompatContext&) = delete;

    /// @brief Phase 1: コアサブシステム初期化 (Window, Device, SwapChain)
    bool InitCore();

    /// @brief Phase 2: 重いサブシステムをバックグラウンドで初期化
    void InitHeavy(std::atomic<float>& progress, std::atomic<bool>& failed);

    /// @brief Phase 3: 初期化後のファイナライズ (カメラ, プロファイル, ImGui等)
    bool FinalizeInit();
};

/// @}
} // namespace gx_internal
