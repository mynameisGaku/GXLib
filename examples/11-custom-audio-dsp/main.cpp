/// @file main.cpp
/// @brief 11-custom-audio-dsp — カスタム DSP エフェクト (ADR-0017 L2)
///
/// Layer 2 拡張ポイント: IAudioEffect を派生して独自 DSP エフェクトを作成し、
/// AudioBus::AddEffect で登録する。
///
/// 学習ポイント / Learning points:
///   - IAudioEffect を派生して Process() を実装する
///   - オーディオコールバックスレッドでの制約 (ヒープ禁止・ロック禁止)
///   - std::atomic で main スレッド ⇔ audio スレッド間のパラメータ伝播
///   - GetAudioManager().GetMixer().GetSEBus() で SE バスを取得し AddEffect
///
/// CURRENT LIMITATION:
///   AudioBus::AddEffect は登録のみ動作し、Process() は現状まだ呼ばれない。
///   XAudio2 SubmixVoice の PCM 処理を行うには IXAPO ラッパー実装が必要。
///   このサンプルは API 形状のデモのみ。

#include "GXLib.h"
#include "Audio/AudioEffect.h"
#include "Audio/AudioBus.h"
#include "Audio/AudioManager.h"
#include "Audio/AudioMixer.h"
#include <atomic>
#include <cmath>

// =========================================================================
// カスタム DSP: トレモロ (音量を LFO で周期変調)
// =========================================================================
class TremoloEffect : public gx::IAudioEffect
{
public:
    void Process(float* buffer, uint32_t sampleCount,
                 uint32_t channels, uint32_t sampleRate) override
    {
        const float rateHz = m_rateHz.load(std::memory_order_relaxed);
        const float depth  = m_depth.load(std::memory_order_relaxed);
        const float phaseInc = 2.0f * 3.14159265f * rateHz / static_cast<float>(sampleRate);

        for (uint32_t i = 0; i < sampleCount; ++i)
        {
            const float lfo = 0.5f + 0.5f * std::sinf(m_phase);
            const float gain = (1.0f - depth) + depth * lfo;
            for (uint32_t c = 0; c < channels; ++c)
                buffer[i * channels + c] *= gain;

            m_phase += phaseInc;
            if (m_phase > 6.28318530f) m_phase -= 6.28318530f;
        }
    }

    void Reset() override { m_phase = 0.0f; }
    const char* GetName() const override { return "Tremolo"; }

    void SetRateHz(float hz)  { m_rateHz.store(hz,  std::memory_order_relaxed); }
    void SetDepth(float d)    { m_depth.store(d,    std::memory_order_relaxed); }
    float GetRateHz() const   { return m_rateHz.load(std::memory_order_relaxed); }
    float GetDepth() const    { return m_depth.load(std::memory_order_relaxed); }

private:
    std::atomic<float> m_rateHz { 5.0f };
    std::atomic<float> m_depth  { 0.7f };
    float m_phase = 0.0f;
};

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    ChangeWindowMode(TRUE);
    SetGraphMode(1280, 720, 32);
    SetMainWindowText("11 - Custom Audio DSP (Layer 2)");

    if (GX_Init() == -1) return -1;
    SetDrawScreen(GX_SCREEN_BACK);

    // =========================================================================
    // Layer 2: カスタム DSP エフェクトを SE バスに登録
    // =========================================================================
    auto& audioMgr = gx::GetAudioManager();
    gx::AudioBus& seBus = audioMgr.GetMixer().GetSEBus();

    auto tremolo = std::make_unique<TremoloEffect>();
    TremoloEffect* tremoloRaw = tremolo.get();
    int effectIdx = seBus.AddEffect(std::move(tremolo));

    const bool hasBGM = (PlayMusic("Assets/audio/bgm_main.ogg", GX_PLAYTYPE_LOOP) == 0);

    unsigned int white = GetColor(255, 255, 255);
    unsigned int green = GetColor(80, 220, 80);
    unsigned int red   = GetColor(220, 80, 80);
    unsigned int cyan  = GetColor(100, 220, 255);

    while (ProcessMessage() == 0)
    {
        if (CheckHitKey(KEY_INPUT_ESCAPE)) break;

        ClearDrawScreen();

        if (tremoloRaw)
        {
            float r = tremoloRaw->GetRateHz();
            float d = tremoloRaw->GetDepth();
            if (CheckHitKey(KEY_INPUT_UP))    tremoloRaw->SetRateHz(r + 0.05f);
            if (CheckHitKey(KEY_INPUT_DOWN))  tremoloRaw->SetRateHz((r > 0.05f) ? r - 0.05f : 0.0f);
            if (CheckHitKey(KEY_INPUT_RIGHT)) tremoloRaw->SetDepth((d < 0.99f) ? d + 0.01f : 1.0f);
            if (CheckHitKey(KEY_INPUT_LEFT))  tremoloRaw->SetDepth((d > 0.01f) ? d - 0.01f : 0.0f);
        }

        DrawFormatString(10, 10, white, "FPS: %.1f", GetFPS());

        DrawString(10, 50, "--- IAudioEffect (Tremolo) ---", cyan);
        if (effectIdx >= 0)
        {
            DrawFormatString(10, 70,  green, "Registered on SE bus @ index %d", effectIdx);
            DrawFormatString(10, 90,  white, "Rate  : %.2f Hz  (UP/DOWN to adjust)",
                             tremoloRaw->GetRateHz());
            DrawFormatString(10, 110, white, "Depth : %.2f     (LEFT/RIGHT to adjust)",
                             tremoloRaw->GetDepth());
            DrawFormatString(10, 130, white, "Bus effects total : %zu", seBus.GetEffectCount());
        }
        else
        {
            DrawString(10, 70, "[ERROR] Failed to register -- check log", red);
        }

        DrawString(10, 170, "--- Current limitation ---", cyan);
        DrawString(10, 190, "AudioBus::AddEffect stores IAudioEffect but Process() is not", white);
        DrawString(10, 210, "yet dispatched on the audio thread. IXAPO wrapper integration", white);
        DrawString(10, 230, "is a pending sprint (see ADR-0010 + gap analysis).", white);
        DrawString(10, 250, "The pattern (derive + atomic params + Process) is final.", white);
        DrawFormatString(10, 280, hasBGM ? white : red, "BGM : %s",
                         hasBGM ? "playing" : "not found (check Assets/audio/bgm_main.ogg)");
        DrawString(10, 320, "ESC: quit", white);

        ScreenFlip();
    }

    if (effectIdx >= 0) seBus.RemoveEffect(effectIdx);
    StopMusic();
    GX_End();
    return 0;
}
