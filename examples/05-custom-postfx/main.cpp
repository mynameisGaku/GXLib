/// @file main.cpp
/// @brief 05-custom-postfx — カスタムポストエフェクト挿入 (ADR-0017 L2)
///
/// Layer 2 拡張ポイント: PostEffectPipeline::InsertCustomEffect で
/// 既存エフェクトチェーンの任意の位置に自作エフェクトを挿入できる。
///
/// このサンプルは空の「パススルー」カスタムエフェクトを AfterBloom に挿入し、
/// 登録と実行が動くことを確認する。実際の描画処理を追加するには
/// Execute() の中で PSO/RootSig をバインドして DrawFullscreen すればよい。
///
/// 学習ポイント / Learning points:
///   - ICustomEffect 派生クラスを作る
///   - PostFXInsertPoint で挿入位置を指定 (AfterSSAO/Bloom/DoF/MotionBlur等)
///   - InsertCustomEffect / RemoveCustomEffect で登録・解除
///   - SetPostFXMask で組み込みエフェクトを個別ON/OFF

#include "GXLib.h"
#include "Graphics/PostEffect/PostEffectPipeline.h"

/// @brief 簡易パススルーカスタムエフェクト
/// 実用的にはここで自作シェーダPSOをバインドして DrawFullscreen する。
/// このサンプルは登録フローのデモのみなのでログ出力のみ。
class PassthroughEffect : public gx::PostEffectPipeline::ICustomEffect
{
public:
    void Execute(ID3D12GraphicsCommandList* /*cmdList*/,
                 gx::RenderTarget& /*input*/, gx::RenderTarget& /*output*/,
                 uint32_t /*width*/, uint32_t /*height*/) override
    {
        // 実装例:
        //   cmdList->SetPipelineState(m_myPSO.Get());
        //   cmdList->SetGraphicsRootSignature(m_myRS.Get());
        //   // ...リソースバインド + Draw...
        //   DrawFullscreen(...);
        // ここでは何もしない (passthrough)
        m_executeCount++;
    }

    uint32_t GetExecuteCount() const { return m_executeCount; }

private:
    uint32_t m_executeCount = 0;
};

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    ChangeWindowMode(TRUE);
    SetGraphMode(1280, 720, 32);
    SetMainWindowText("05 - Custom PostFX (Layer 2)");

    if (GX_Init() == -1) return -1;
    SetDrawScreen(GX_SCREEN_BACK);

    // =========================================================================
    // Layer 2: カスタムポストエフェクトを登録
    // =========================================================================
    auto& pipe = GetPostEffects();

    auto effect = std::make_unique<PassthroughEffect>();
    PassthroughEffect* effectRaw = effect.get();  // 実行回数の監視用 (ownership は pipe に渡す)

    pipe.InsertCustomEffect(
        "MyPassthrough",
        gx::PostEffectPipeline::PostFXInsertPoint::AfterBloom,
        std::move(effect));

    unsigned int white  = GetColor(255, 255, 255);
    unsigned int yellow = GetColor(255, 220,  50);
    unsigned int gray   = GetColor(160, 160, 160);
    unsigned int green  = GetColor(100, 255, 100);

    bool fullFx = true;
    bool spaceWasDown = false;

    while (ProcessMessage() == 0)
    {
        if (CheckHitKey(KEY_INPUT_ESCAPE)) break;

        ClearDrawScreen();

        // SPACE で PostFX 全体 ON/OFF
        bool spaceDown = (CheckHitKey(KEY_INPUT_SPACE) != 0);
        if (spaceDown && !spaceWasDown)
        {
            fullFx = !fullFx;
            SetPostFXEnabled(fullFx);
        }
        spaceWasDown = spaceDown;

        DrawFormatString(10, 10, white, "FPS: %.1f", GetFPS());
        DrawFormatString(10, 40, fullFx ? white : yellow,
                         "PostFX : %s   (SPACE to toggle)",
                         fullFx ? "ON (all)" : "OFF");

        DrawString(10,  80, "--- Layer 2 Custom Effect ---", gray);
        DrawFormatString(10, 100, green,
                         "Registered  : MyPassthrough @ AfterBloom");
        DrawFormatString(10, 120, green,
                         "Executed    : %u times (ping-pong RT handled by pipeline)",
                         effectRaw->GetExecuteCount());
        DrawFormatString(10, 140, gray,
                         "Total custom: %zu", pipe.GetCustomEffectCount());

        DrawString(10, 180, "ESC: quit", white);
        DrawString(10, 210, "Execute() is called every frame while PostFX is ON.", gray);
        DrawString(10, 230, "Implement your own shader dispatch inside ICustomEffect::Execute.", gray);

        ScreenFlip();
    }

    // 明示的な解除 (省略しても GX_End で破棄される)
    pipe.RemoveCustomEffect("MyPassthrough");
    GX_End();
    return 0;
}
