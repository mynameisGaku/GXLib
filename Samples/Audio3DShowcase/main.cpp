/// @file Samples/Audio3DShowcase/main.cpp
/// @brief 3D空間音響デモ
///
/// 音源オブジェクト（球体）が円運動し、カメラ位置に応じたパンニング・距離減衰を体感する。
/// 起動時にサイン波トーンを生成してWAVファイルとして書き出し、3Dサウンドとして再生する。
///
/// 使用API:
///   - AudioListener::UpdateFromCamera()
///   - AudioEmitter::SetPosition()
///   - AudioManager::LoadSound() / PlaySound3D() / SetListener() / Update()
///   - AudioMixer::GetSEBus().SetVolume()
#include "GXEasy.h"
#include "Compat/CompatContext.h"
#include "Graphics/3D/MeshData.h"
#include "Graphics/3D/Light.h"
#include "Graphics/3D/Material.h"
#include "Audio/AudioEmitter.h"
#include "Audio/AudioListener.h"

#include <string>
#include <cmath>
#include <fstream>
#include <Windows.h>

/// @brief サイン波のWAVファイルを生成する
/// @param filePath 出力先パス
/// @param frequency 周波数（Hz）
/// @param durationSec 長さ（秒）
/// @param sampleRate サンプルレート
static bool GenerateSineWAV(const std::wstring& filePath, float frequency,
                            float durationSec, uint32_t sampleRate = 44100)
{
    const uint16_t numChannels = 1;
    const uint16_t bitsPerSample = 16;
    const uint32_t numSamples = static_cast<uint32_t>(sampleRate * durationSec);
    const uint32_t dataSize = numSamples * numChannels * (bitsPerSample / 8);
    const uint32_t byteRate = sampleRate * numChannels * (bitsPerSample / 8);
    const uint16_t blockAlign = numChannels * (bitsPerSample / 8);

    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) return false;

    // RIFF header
    file.write("RIFF", 4);
    uint32_t chunkSize = 36 + dataSize;
    file.write(reinterpret_cast<const char*>(&chunkSize), 4);
    file.write("WAVE", 4);

    // fmt chunk
    file.write("fmt ", 4);
    uint32_t fmtSize = 16;
    file.write(reinterpret_cast<const char*>(&fmtSize), 4);
    uint16_t audioFormat = 1; // PCM
    file.write(reinterpret_cast<const char*>(&audioFormat), 2);
    file.write(reinterpret_cast<const char*>(&numChannels), 2);
    file.write(reinterpret_cast<const char*>(&sampleRate), 4);
    file.write(reinterpret_cast<const char*>(&byteRate), 4);
    file.write(reinterpret_cast<const char*>(&blockAlign), 2);
    file.write(reinterpret_cast<const char*>(&bitsPerSample), 2);

    // data chunk
    file.write("data", 4);
    file.write(reinterpret_cast<const char*>(&dataSize), 4);

    const float PI2 = 6.283185307f;
    // フェードイン/アウトで再生時のクリックノイズを防止
    const uint32_t fadeSamples = (std::min)(sampleRate / 20, numSamples / 4); // 50ms or 1/4

    for (uint32_t i = 0; i < numSamples; ++i)
    {
        float t = static_cast<float>(i) / sampleRate;
        float sample = std::sin(PI2 * frequency * t) * 0.4f; // 振幅0.4

        // フェードイン/アウト
        if (i < fadeSamples)
            sample *= static_cast<float>(i) / fadeSamples;
        else if (i > numSamples - fadeSamples)
            sample *= static_cast<float>(numSamples - i) / fadeSamples;

        int16_t pcm = static_cast<int16_t>(sample * 32767.0f);
        file.write(reinterpret_cast<const char*>(&pcm), 2);
    }

    return true;
}

class Audio3DShowcaseApp : public GXEasy::App
{
public:
    GXEasy::AppConfig GetConfig() const override
    {
        GXEasy::AppConfig config;
        config.title  = L"GXLib Sample: 3D Spatial Audio";
        config.width  = 1280;
        config.height = 720;
        config.bgR = 6; config.bgG = 8; config.bgB = 18;
        return config;
    }

    void Start() override
    {
        auto& ctx      = GX_Internal::CompatContext::Instance();
        auto& renderer  = ctx.renderer3D;
        auto& camera    = ctx.camera;
        auto& postFX    = ctx.postEffect;

        renderer.SetShadowEnabled(false);

        postFX.SetTonemapMode(GX::TonemapMode::ACES);
        postFX.GetBloom().SetEnabled(true);
        postFX.SetFXAAEnabled(true);

        // メッシュ
        m_sphereMesh   = renderer.CreateGPUMesh(GX::MeshGenerator::CreateSphere(0.5f, 16, 8));
        m_floorMesh    = renderer.CreateGPUMesh(GX::MeshGenerator::CreatePlane(40.0f, 40.0f, 20, 20));
        m_listenerMesh = renderer.CreateGPUMesh(GX::MeshGenerator::CreateBox(0.3f, 1.5f, 0.3f));

        m_floorTransform.SetPosition(0, 0, 0);
        m_floorMat.constants.albedoFactor    = { 0.4f, 0.4f, 0.42f, 1.0f };
        m_floorMat.constants.roughnessFactor = 0.8f;

        // 音源球マテリアル（3色）+ エミッター設定
        float colors[][3] = { {1.0f, 0.2f, 0.1f}, {0.1f, 0.8f, 0.2f}, {0.2f, 0.3f, 1.0f} };
        for (int i = 0; i < k_NumEmitters; ++i)
        {
            m_emitterMats[i].constants.albedoFactor = { colors[i][0], colors[i][1], colors[i][2], 1.0f };
            m_emitterMats[i].constants.roughnessFactor = 0.3f;
            m_emitterMats[i].constants.metallicFactor  = 0.8f;

            m_emitters[i].SetMaxDistance(30.0f);
            m_emitters[i].SetInnerRadius(2.0f);
        }

        // リスナーマテリアル
        m_listenerMat.constants.albedoFactor    = { 1.0f, 1.0f, 0.2f, 1.0f };
        m_listenerMat.constants.roughnessFactor = 0.4f;

        // ライト
        GX::LightData lights[2];
        lights[0] = GX::Light::CreateDirectional({ 0.3f, -1.0f, 0.5f }, { 1.0f, 0.98f, 0.95f }, 3.0f);
        lights[1] = GX::Light::CreatePoint({ 0.0f, 8.0f, 0.0f }, 30.0f, { 1.0f, 0.95f, 0.9f }, 4.0f);
        renderer.SetLights(lights, 2, { 0.08f, 0.08f, 0.1f });

        renderer.GetSkybox().SetSun({ 0.3f, -1.0f, 0.5f }, 5.0f);
        renderer.GetSkybox().SetColors({ 0.4f, 0.5f, 0.7f }, { 0.7f, 0.75f, 0.8f });

        // カメラ（固定俯瞰）
        float aspect = static_cast<float>(ctx.swapChain.GetWidth()) / ctx.swapChain.GetHeight();
        camera.SetPerspective(XM_PIDIV4, aspect, 0.1f, 500.0f);
        camera.SetPosition(0.0f, 15.0f, -20.0f);
        camera.SetPitch(-0.5f);
        camera.SetYaw(0.0f);

        // サイン波トーンを生成してWAVファイルとして書き出す
        // 3つの異なる周波数でエミッターを区別できるようにする
        float frequencies[] = { 440.0f, 554.37f, 659.26f }; // A4, C#5, E5（メジャーコード）
        std::wstring toneFiles[] = { L"_tone_a4.wav", L"_tone_cs5.wav", L"_tone_e5.wav" };
        for (int i = 0; i < k_NumEmitters; ++i)
        {
            GenerateSineWAV(toneFiles[i], frequencies[i], k_ToneDuration);
            m_soundHandles[i] = ctx.audioManager.LoadSound(toneFiles[i]);
        }

        // SE音量初期値
        m_seVolume = 1.0f;
    }

    void Release() override
    {
        // Start()で生成した一時WAVファイルを削除
        std::remove("_tone_a4.wav");
        std::remove("_tone_cs5.wav");
        std::remove("_tone_e5.wav");
    }

    void Update(float dt) override
    {
        auto& ctx    = GX_Internal::CompatContext::Instance();
        auto& camera = ctx.camera;

        m_totalTime += dt;
        m_lastDt = dt;

        // 3つのエミッターが異なる半径・高さ・速度で円運動
        for (int i = 0; i < k_NumEmitters; ++i)
        {
            float radius = 6.0f + i * 3.0f;
            float speed  = 1.0f + i * 0.3f;
            float height = 1.0f + i * 1.5f;
            float angle  = m_totalTime * speed + i * XM_2PI / k_NumEmitters;

            XMFLOAT3 pos = {
                std::cos(angle) * radius,
                height,
                std::sin(angle) * radius
            };

            m_emitters[i].SetPosition(pos);
            m_emitterTransforms[i].SetPosition(pos.x, pos.y, pos.z);

            // 速度を設定（ドップラー効果用）
            XMFLOAT3 vel = {
                -std::sin(angle) * radius * speed,
                0.0f,
                std::cos(angle) * radius * speed
            };
            m_emitters[i].SetVelocity(vel);

            // サウンドを定期的に再トリガー（ループ再生の代替）
            m_retriggerTimers[i] -= dt;
            if (m_retriggerTimers[i] <= 0.0f && m_soundHandles[i] >= 0)
            {
                ctx.audioManager.PlaySound3D(m_soundHandles[i], m_emitters[i]);
                m_retriggerTimers[i] = k_ToneDuration - 0.05f; // フェードアウト前に再トリガー
            }
        }

        // リスナーをカメラに追従
        m_listener.UpdateFromCamera(camera, dt);

        // AudioManagerにリスナーを設定し、3Dサウンド計算を更新
        ctx.audioManager.SetListener(m_listener);
        ctx.audioManager.Update(dt);

        // リスナー位置の可視化用
        auto camPos = camera.GetPosition();
        m_listenerTransform.SetPosition(camPos.x, 0.5f, camPos.z);

        // 上下キーでSE音量変更
        auto& mixer = ctx.audioManager.GetMixer();
        if (CheckHitKey(KEY_INPUT_UP))
        {
            m_seVolume = (std::min)(m_seVolume + dt * 0.5f, 1.0f);
            mixer.GetSEBus().SetVolume(m_seVolume);
        }
        if (CheckHitKey(KEY_INPUT_DOWN))
        {
            m_seVolume = (std::max)(m_seVolume - dt * 0.5f, 0.0f);
            mixer.GetSEBus().SetVolume(m_seVolume);
        }

        // WASD カメラ移動
        float speed = 8.0f * dt;
        if (CheckHitKey(KEY_INPUT_W)) camera.MoveForward(speed);
        if (CheckHitKey(KEY_INPUT_S)) camera.MoveForward(-speed);
        if (CheckHitKey(KEY_INPUT_D)) camera.MoveRight(speed);
        if (CheckHitKey(KEY_INPUT_A)) camera.MoveRight(-speed);
    }

    void Draw() override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        auto* cmd = ctx.cmdList;
        const uint32_t frameIndex = ctx.frameIndex;

        ctx.FlushAll();

        ctx.postEffect.BeginScene(cmd, frameIndex,
                                  ctx.renderer3D.GetDepthBuffer().GetDSVHandle(),
                                  ctx.camera);
        ctx.renderer3D.Begin(cmd, frameIndex, ctx.camera, m_totalTime);

        // 床
        ctx.renderer3D.SetMaterial(m_floorMat);
        ctx.renderer3D.DrawMesh(m_floorMesh, m_floorTransform);

        // エミッター球
        for (int i = 0; i < k_NumEmitters; ++i)
        {
            ctx.renderer3D.SetMaterial(m_emitterMats[i]);
            ctx.renderer3D.DrawMesh(m_sphereMesh, m_emitterTransforms[i]);
        }

        // リスナー位置表示
        ctx.renderer3D.SetMaterial(m_listenerMat);
        ctx.renderer3D.DrawMesh(m_listenerMesh, m_listenerTransform);

        ctx.renderer3D.End();
        ctx.postEffect.EndScene();

        auto& depthBuffer = ctx.renderer3D.GetDepthBuffer();
        depthBuffer.TransitionTo(cmd, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ctx.postEffect.Resolve(ctx.swapChain.GetCurrentRTVHandle(),
                               depthBuffer, ctx.camera, m_lastDt);
        depthBuffer.TransitionTo(cmd, D3D12_RESOURCE_STATE_DEPTH_WRITE);

        // HUD
        const float fps = (m_lastDt > 0.0f) ? (1.0f / m_lastDt) : 0.0f;
        DrawString(10, 10, FormatT(TEXT("FPS: {:.1f}"), fps).c_str(), GetColor(255, 255, 255));
        DrawString(10, 35, TEXT("3 emitters orbiting with 3D spatial audio"),
                   GetColor(120, 180, 255));

        // エミッター距離表示
        auto camPos = GX_Internal::CompatContext::Instance().camera.GetPosition();
        const TChar* noteNames[] = { TEXT("A4"), TEXT("C#5"), TEXT("E5") };
        for (int i = 0; i < k_NumEmitters; ++i)
        {
            auto ePos = m_emitters[i].GetPosition();
            float dx = ePos.x - camPos.x;
            float dy = ePos.y - camPos.y;
            float dz = ePos.z - camPos.z;
            float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
            DrawString(10, 60 + i * 25,
                       FormatT(TEXT("Emitter {} ({}): dist={:.1f}m"), i + 1, noteNames[i], dist).c_str(),
                       GetColor(200, 200, 200));
        }

        DrawString(10, 60 + k_NumEmitters * 25,
                   FormatT(TEXT("SE Volume: {:.0f}%%  (Up/Down to adjust)"), m_seVolume * 100.0f).c_str(),
                   GetColor(255, 200, 100));
        DrawString(10, 85 + k_NumEmitters * 25,
                   TEXT("WASD: Move camera  ESC: Quit"),
                   GetColor(136, 136, 136));
    }

private:
    static constexpr int   k_NumEmitters  = 3;
    static constexpr float k_ToneDuration = 2.0f; // トーンの長さ（秒）

    float m_totalTime = 0.0f;
    float m_lastDt    = 0.0f;
    float m_seVolume  = 1.0f;

    // メッシュ
    GX::GPUMesh m_sphereMesh;
    GX::GPUMesh m_floorMesh;
    GX::GPUMesh m_listenerMesh;

    // 床
    GX::Transform3D m_floorTransform;
    GX::Material    m_floorMat;

    // エミッター
    GX::AudioEmitter  m_emitters[k_NumEmitters];
    GX::Transform3D   m_emitterTransforms[k_NumEmitters];
    GX::Material      m_emitterMats[k_NumEmitters];

    // サウンド
    int   m_soundHandles[k_NumEmitters] = { -1, -1, -1 };
    float m_retriggerTimers[k_NumEmitters] = { 0.0f, 0.0f, 0.0f };

    // リスナー
    GX::AudioListener m_listener;
    GX::Transform3D   m_listenerTransform;
    GX::Material      m_listenerMat;
};

GX_EASY_APP(Audio3DShowcaseApp)
