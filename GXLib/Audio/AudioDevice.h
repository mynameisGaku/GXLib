#pragma once
/// @file AudioDevice.h
/// @brief XAudio2オーディオデバイス管理
///
/// 【初学者向け解説】
/// XAudio2はMicrosoftが提供する低レイテンシオーディオAPIです。
/// サウンドの再生には以下の3層構造があります：
///
/// 1. IXAudio2 — オーディオエンジン本体
/// 2. MasteringVoice — 最終出力（スピーカーに繋がる）
/// 3. SourceVoice — 個別の音声（SE、BGMなど）
///
/// AudioDeviceクラスは1と2を管理します。

#include "pch.h"

namespace GX
{

/// @brief XAudio2オーディオエンジン管理クラス
class AudioDevice
{
public:
    AudioDevice() = default;
    ~AudioDevice();

    /// オーディオデバイスを初期化
    bool Initialize();

    /// 終了処理
    void Shutdown();

    /// XAudio2エンジンを取得
    IXAudio2* GetXAudio2() const { return m_xaudio2.Get(); }

    /// マスターボイスを取得
    IXAudio2MasteringVoice* GetMasterVoice() const { return m_masterVoice; }

    /// マスターボリュームを設定（0.0〜1.0）
    void SetMasterVolume(float volume);

private:
    ComPtr<IXAudio2>        m_xaudio2;
    IXAudio2MasteringVoice* m_masterVoice = nullptr;  ///< XAudio2が所有（ComPtr不要）
    bool                    m_comInitialized = false;
};

} // namespace GX
