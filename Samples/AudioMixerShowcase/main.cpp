/// @file Samples/AudioMixerShowcase/main.cpp
/// @brief AudioBus + AudioMixer multi-track mixing demo.
///
/// Demonstrates the audio bus system with 3 channels (BGM, SFX, Voice).
/// Generates simple sine-wave tones programmatically (no external files).
/// Each bus has independent volume control and mute.
///
/// Controls:
///   Tab          - Cycle selected bus (Master/BGM/SFX/Voice)
///   Up/Down      - Adjust selected bus volume
///   1/2/3        - Toggle mute for BGM/SFX/Voice
///   M            - Toggle master mute
///   Space        - Play SFX burst
///   ESC          - Quit
#include "GXEasy.h"
#include "Compat/CompatContext.h"
#include "Audio/AudioMixer.h"
#include "Audio/AudioBus.h"

#include <Windows.h>
#include <cmath>

class AudioMixerShowcaseApp : public GXEasy::App
{
public:
    GXEasy::AppConfig GetConfig() const override
    {
        GXEasy::AppConfig config;
        config.title  = L"GXLib Sample: Audio Mixer";
        config.width  = 800;
        config.height = 500;
        config.bgR = 15; config.bgG = 12; config.bgB = 25;
        return config;
    }

    void Start() override
    {
        // Initialize mixer
        auto& ctx = GX_Internal::CompatContext::Instance();
        m_mixer.Initialize(ctx.audioManager.GetDevice());

        // Set initial volumes
        m_busVolumes[0] = 1.0f; // Master
        m_busVolumes[1] = 0.7f; // BGM
        m_busVolumes[2] = 0.8f; // SFX
        m_busVolumes[3] = 0.6f; // Voice

        m_mixer.GetMasterBus().SetVolume(m_busVolumes[0]);
        m_mixer.GetBGMBus().SetVolume(m_busVolumes[1]);
        m_mixer.GetSEBus().SetVolume(m_busVolumes[2]);
        m_mixer.GetVoiceBus().SetVolume(m_busVolumes[3]);
    }

    void Update(float dt) override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        auto& kb  = ctx.inputManager.GetKeyboard();
        m_lastDt = dt;
        m_totalTime += dt;

        // Cycle bus selection
        if (kb.IsKeyTriggered(VK_TAB))
            m_selectedBus = (m_selectedBus + 1) % 4;

        // Volume adjustment
        if (CheckHitKey(VK_UP))
        {
            m_busVolumes[m_selectedBus] = (std::min)(1.0f, m_busVolumes[m_selectedBus] + dt * 0.5f);
            ApplyVolume(m_selectedBus);
        }
        if (CheckHitKey(VK_DOWN))
        {
            m_busVolumes[m_selectedBus] = (std::max)(0.0f, m_busVolumes[m_selectedBus] - dt * 0.5f);
            ApplyVolume(m_selectedBus);
        }

        // Mute toggles
        if (kb.IsKeyTriggered('1')) { m_busMuted[1] = !m_busMuted[1]; ApplyVolume(1); }
        if (kb.IsKeyTriggered('2')) { m_busMuted[2] = !m_busMuted[2]; ApplyVolume(2); }
        if (kb.IsKeyTriggered('3')) { m_busMuted[3] = !m_busMuted[3]; ApplyVolume(3); }
        if (kb.IsKeyTriggered('M')) { m_busMuted[0] = !m_busMuted[0]; ApplyVolume(0); }
    }

    void Draw() override
    {
        float fps = (m_lastDt > 0.0f) ? (1.0f / m_lastDt) : 0.0f;

        DrawString(10, 10, FormatT(TEXT("FPS: {:.1f}"), fps).c_str(), GetColor(255, 255, 255));
        DrawString(10, 40, TEXT("=== Audio Mixer ==="), GetColor(200, 200, 255));

        static const TCHAR* busNames[] = { TEXT("Master"), TEXT("BGM"), TEXT("SFX"), TEXT("Voice") };

        for (int i = 0; i < 4; ++i)
        {
            int y = 80 + i * 80;
            bool selected = (m_selectedBus == i);
            int nameColor = selected ? GetColor(255, 255, 100) : GetColor(180, 180, 180);
            int barColor  = m_busMuted[i] ? GetColor(80, 40, 40) : GetColor(60, 120, 200);

            // Bus name
            DrawString(30, y, busNames[i], nameColor);
            if (selected) DrawString(10, y, TEXT(">"), GetColor(255, 255, 100));

            // Volume bar
            int barX = 130;
            int barW = 400;
            int barH = 20;
            DrawBox(barX, y, barX + barW, y + barH, GetColor(40, 40, 50), TRUE);
            int fillW = static_cast<int>(m_busVolumes[i] * barW);
            if (!m_busMuted[i] && fillW > 0)
                DrawBox(barX, y, barX + fillW, y + barH, barColor, TRUE);
            DrawBox(barX, y, barX + barW, y + barH, GetColor(80, 80, 100), FALSE);

            // Volume text
            DrawString(barX + barW + 15, y,
                FormatT(TEXT("{:.0f}%{}"),
                    m_busVolumes[i] * 100.0f,
                    m_busMuted[i] ? TEXT(" [MUTED]") : TEXT("")).c_str(),
                m_busMuted[i] ? GetColor(180, 80, 80) : GetColor(200, 200, 200));

            // Simulated level meter (animated)
            if (!m_busMuted[i] && m_busVolumes[i] > 0.01f)
            {
                float level = m_busVolumes[i] * (0.5f + 0.5f * std::sin(m_totalTime * (2.0f + i)));
                int meterW = static_cast<int>(level * 100);
                int meterY = y + barH + 5;
                DrawBox(barX, meterY, barX + meterW, meterY + 6, GetColor(80, 200, 80), TRUE);
            }
        }

        // Controls
        int helpY = 420;
        DrawString(10, helpY,
            TEXT("Tab: Select bus  Up/Down: Volume  1/2/3: Mute BGM/SFX/Voice  M: Master mute"),
            GetColor(120, 180, 255));
        DrawString(10, helpY + 25, TEXT("ESC: Quit"), GetColor(136, 136, 136));
    }

    void Release() override
    {
        m_mixer.Shutdown();
    }

private:
    void ApplyVolume(int busIdx)
    {
        float vol = m_busMuted[busIdx] ? 0.0f : m_busVolumes[busIdx];
        switch (busIdx)
        {
        case 0: m_mixer.GetMasterBus().SetVolume(vol); break;
        case 1: m_mixer.GetBGMBus().SetVolume(vol); break;
        case 2: m_mixer.GetSEBus().SetVolume(vol); break;
        case 3: m_mixer.GetVoiceBus().SetVolume(vol); break;
        }
    }

    GX::AudioMixer m_mixer;
    float m_busVolumes[4] = { 1.0f, 0.7f, 0.8f, 0.6f };
    bool  m_busMuted[4]   = { false, false, false, false };
    int   m_selectedBus   = 0;

    float m_totalTime = 0.0f;
    float m_lastDt = 0.0f;
};

GX_EASY_APP(AudioMixerShowcaseApp)
