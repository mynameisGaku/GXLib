#pragma once
/// @file SoundPlayer.h
/// @brief SE（効果音）再生
///
/// DxLibのPlaySoundMem / StopSoundMemに相当するSE再生機能を提供する。
/// 同じ効果音を重ねて再生できるよう、Play()のたびに新しいSourceVoiceを作成する。
/// 再生が終わったVoiceはXAudio2のコールバック通知で検出し、
/// CleanupFinishedVoices()でまとめて解放する。

#include "pch.h"
#include <x3daudio.h>

namespace GX
{

class Sound;
class AudioDevice;
class AudioListener;
class AudioEmitter;
class AudioBus;

/// @brief 効果音の再生を管理するクラス（DxLibのPlaySoundMem相当）
///
/// 通常の2D再生に加え、3D空間音響に対応したPlay3D()を提供する。
/// 3Dサウンドは毎フレームUpdate3D()を呼ぶことで、リスナーとの相対位置から
/// パンニング・距離減衰・ドップラー効果が自動計算される。
class SoundPlayer
{
public:
    SoundPlayer() = default;
    ~SoundPlayer();

    /// @brief AudioDeviceを関連付けて初期化する
    /// @param audioDevice XAudio2エンジンを持つAudioDeviceへのポインタ
    void Initialize(AudioDevice* audioDevice);

    /// @brief 効果音を再生する。同じSoundを複数同時に鳴らせる
    /// @param sound 再生するPCMデータ
    /// @param volume 音量（0.0〜1.0、デフォルト1.0）
    /// @param pan 左右パン（-1.0=左 〜 0.0=中央 〜 1.0=右、デフォルト0.0）
    void Play(const Sound& sound, float volume = 1.0f, float pan = 0.0f);

    /// @brief 効果音を指定バスに出力して再生する
    /// @param sound 再生するPCMデータ
    /// @param bus 出力先のバス
    /// @param volume 音量（0.0〜1.0、デフォルト1.0）
    void PlayOnBus(const Sound& sound, AudioBus& bus, float volume = 1.0f);

    /// @brief 3D空間内で効果音を再生する
    /// @param sound 再生するPCMデータ
    /// @param emitter 音源位置・方向・減衰設定
    /// @param volume 音量（0.0〜1.0、デフォルト1.0）
    /// @return ボイスID（停止用、-1で失敗）
    int Play3D(const Sound& sound, AudioEmitter& emitter, float volume = 1.0f);

    /// @brief 3Dサウンドのパラメータを更新する（毎フレーム呼び出し）
    /// @param listener リスナー位置情報
    void Update3D(const AudioListener& listener);

    /// @brief 指定IDの3Dサウンドを停止する
    /// @param voiceId Play3Dの戻り値
    void Stop3D(int voiceId);

    /// @brief 現在再生中のVoice数を取得する
    /// @return アクティブなVoice数
    int GetActiveVoiceCount() const { return m_activeVoiceCount; }

    /// @brief 再生が完了したVoiceを検出して解放する。Update等から定期的に呼ぶこと
    void CleanupFinishedVoices();

    /// @brief 全Voiceを停止・破棄する（シャットダウン用）
    void StopAll();

private:
    /// XAudio2のSourceVoice再生完了を検知するコールバック。
    /// OnStreamEnd()でフラグを立て、CleanupFinishedVoices()でチェックする。
    struct VoiceCallback : public IXAudio2VoiceCallback
    {
        bool isFinished = false;

        void STDMETHODCALLTYPE OnStreamEnd() override { isFinished = true; }
        void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() override {}
        void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32) override {}
        void STDMETHODCALLTYPE OnBufferEnd(void*) override {}
        void STDMETHODCALLTYPE OnBufferStart(void*) override {}
        void STDMETHODCALLTYPE OnLoopEnd(void*) override {}
        void STDMETHODCALLTYPE OnVoiceError(void*, HRESULT) override {}
    };

    /// @brief 1つの再生中Voice（SourceVoice + コールバック）のペア
    struct ActiveVoice
    {
        IXAudio2SourceVoice*           voice = nullptr;
        std::unique_ptr<VoiceCallback> callback;
        bool is3D = false;   ///< 3Dサウンドか
        int  voiceId = -1;   ///< 3DボイスID（Play3Dの戻り値）
    };

    /// @brief 3Dサウンド用の追加情報
    struct Voice3DInfo
    {
        int activeVoiceIndex = -1;   ///< m_activeVoices内のインデックス
        AudioEmitter* emitter = nullptr;  ///< 音源位置情報（呼び出し元が所有）
        uint32_t srcChannels = 1;    ///< ソースチャンネル数
    };

    AudioDevice* m_audioDevice = nullptr;
    std::vector<ActiveVoice> m_activeVoices;  ///< 現在再生中のVoiceリスト
    int m_activeVoiceCount = 0;               ///< m_activeVoices.size()のキャッシュ

    // 3Dサウンド管理
    std::vector<Voice3DInfo> m_3dVoices;      ///< 3DボイスIDでインデックス
    int m_nextVoiceId = 0;                    ///< 次に割り当てる3DボイスID

    /// @brief X3DAudioCalculate用の一時バッファ（最大8ch出力対応）
    static constexpr uint32_t k_MaxOutputChannels = 8;
    float m_matrixCoefficients[k_MaxOutputChannels] = {};
};

} // namespace GX
