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
/// 2026-04-17 以降: AudioBus::AddEffect は XAPOBridge (Audio/XAPOBridge.h) を
/// 経由して XAudio2 SubmixVoice に SetEffectChain で配線される。
/// Process() は実際に audio callback thread で呼ばれる。

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
        // Diagnostic counter — increments each time XAudio2 dispatches
        // Process() on the audio thread. Visible from main via GetCallCount().
        m_callCount.fetch_add(1, std::memory_order_relaxed);
        m_totalFrames.fetch_add(sampleCount, std::memory_order_relaxed);
        m_lastChannels.store(channels, std::memory_order_relaxed);
        m_lastSampleRate.store(sampleRate, std::memory_order_relaxed);

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

    uint64_t GetCallCount()    const { return m_callCount.load(std::memory_order_relaxed); }
    uint64_t GetTotalFrames()  const { return m_totalFrames.load(std::memory_order_relaxed); }
    uint32_t GetLastChannels() const { return m_lastChannels.load(std::memory_order_relaxed); }
    uint32_t GetLastRate()     const { return m_lastSampleRate.load(std::memory_order_relaxed); }

private:
    std::atomic<float> m_rateHz { 5.0f };
    std::atomic<float> m_depth  { 0.7f };
    float m_phase = 0.0f;

    // 診断カウンタ (XAudio2 audio-thread → main thread via atomic)
    std::atomic<uint64_t> m_callCount { 0 };
    std::atomic<uint64_t> m_totalFrames { 0 };
    std::atomic<uint32_t> m_lastChannels { 0 };
    std::atomic<uint32_t> m_lastSampleRate { 0 };
};

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    ChangeWindowMode(TRUE);
    SetGraphMode(1280, 720, 32);
    SetMainWindowText("11 - Custom Audio DSP (Layer 2)");

    if (GX_Init() == -1) return -1;
    SetDrawScreen(GX_SCREEN_BACK);

    // =========================================================================
    // Layer 2: カスタム DSP エフェクトを BGM バスに登録
    // Tremolo は PlayMusic / PlaySoundMem(LOOP) がルートされる BGM バス
    // (AudioMixer.h GetBGMBus) に attach する。SE バスに付けると本サンプルの
    // ループ音が載らないため効果が聞こえない。
    // =========================================================================
    auto& audioMgr = gx::GetAudioManager();
    gx::AudioBus& bgmBus = audioMgr.GetMixer().GetBGMBus();

    auto tremolo = std::make_unique<TremoloEffect>();
    TremoloEffect* tremoloRaw = tremolo.get();
    int effectIdx = bgmBus.AddEffect(std::move(tremolo));

    // Sustained sine tone (2 sec, 440 Hz mono WAV, ~88 KB) as the test source.
    // Compat LoadSound は現状 WAV のみ対応 (ADR-0007 + Sound::LoadFromFile)。
    // PlaySoundMem(GX_PLAYTYPE_LOOP) は AudioManager::PlayMusic にルートされ BGM バスで再生される。
    int toneHandle = LoadSoundMem("Assets/audio/test_tone.wav");
    const bool hasBGM = (toneHandle >= 0);
    if (hasBGM)
    {
        PlaySoundMem(toneHandle, GX_PLAYTYPE_LOOP, TRUE);
    }

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
            DrawFormatString(10, 70,  green, "Registered on BGM bus @ index %d", effectIdx);
            DrawFormatString(10, 90,  white, "Rate  : %.2f Hz  (UP/DOWN to adjust)",
                             tremoloRaw->GetRateHz());
            DrawFormatString(10, 110, white, "Depth : %.2f     (LEFT/RIGHT to adjust)",
                             tremoloRaw->GetDepth());
            DrawFormatString(10, 130, white, "Bus effects total : %zu", bgmBus.GetEffectCount());
        }
        else
        {
            DrawString(10, 70, "[ERROR] Failed to register -- check log", red);
        }

        DrawString(10, 170, "--- Runtime diagnostics ---", cyan);
        if (tremoloRaw)
        {
            uint64_t calls  = tremoloRaw->GetCallCount();
            uint64_t frames = tremoloRaw->GetTotalFrames();
            DrawFormatString(10, 190, calls > 0 ? green : red,
                             "Process() calls : %llu (should grow if XAPO chain is wired)",
                             static_cast<unsigned long long>(calls));
            DrawFormatString(10, 210, white,
                             "Total frames    : %llu  (ch=%u, sr=%u Hz)",
                             static_cast<unsigned long long>(frames),
                             tremoloRaw->GetLastChannels(),
                             tremoloRaw->GetLastRate());
        }

        // BGM バスの SetEffectChain HRESULT を直読みして表示
        {
            long hr = bgmBus.GetLastEffectChainStatus();
            unsigned int outCh = bgmBus.GetLastEffectChainOutputChannels();
            DrawFormatString(10, 230, hr == 0 ? green : red,
                             "SetEffectChain HRESULT : 0x%08lX (OutputChannels=%u)",
                             static_cast<unsigned long>(hr), outCh);
        }

        DrawFormatString(10, 260, hasBGM ? green : red, "Test tone : %s",
                         hasBGM ? "playing 440 Hz sine on BGM bus" :
                                  "not found — launch from build/examples/11-custom-audio-dsp/Debug/");
        DrawString(10, 290, "HRESULT 0x00000000 = S_OK. Non-zero means engine registration failed.", white);
        DrawString(10, 310, "If Process() calls = 0 but HRESULT = S_OK: SourceVoice bypassed bus.", white);
        DrawString(10, 350, "ESC: quit", white);

        ScreenFlip();
    }

    if (effectIdx >= 0) bgmBus.RemoveEffect(effectIdx);
    StopMusic();
    if (toneHandle >= 0) DeleteSoundMem(toneHandle);
    GX_End();
    return 0;
}
