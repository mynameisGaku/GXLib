#pragma once
/// @file AudioDevice.h
/// @brief XAudio2オーディオデバイス管理
///
/// XAudio2はMicrosoftの低レイテンシオーディオAPIで、DxLibの内部サウンドエンジンに相当する。
/// XAudio2のサウンド再生は3層構造で動作する:
///   IXAudio2 (エンジン) → MasteringVoice (最終出力) → SourceVoice (個別の音声)
/// このクラスはエンジンとMasteringVoiceの生成・破棄を担当する。
/// 個別の音声再生はSoundPlayer / MusicPlayerが行う。

#include "pch.h"

namespace GX
{

/// @brief XAudio2エンジンとMasteringVoiceを管理するクラス
class AudioDevice
{
public:
    AudioDevice() = default;
    ~AudioDevice();

    /// @brief XAudio2エンジンとMasteringVoiceを初期化する
    /// @return 成功すればtrue
    bool Initialize();

    /// @brief XAudio2エンジンを破棄し、COMを解放する
    void Shutdown();

    /// @brief XAudio2エンジンのポインタを取得する（SourceVoice作成等に使う）
    /// @return IXAudio2ポインタ。未初期化ならnullptr
    IXAudio2* GetXAudio2() const { return m_xaudio2.Get(); }

    /// @brief MasteringVoiceを取得する（出力先の設定等に使う）
    /// @return MasteringVoiceポインタ。未初期化ならnullptr
    IXAudio2MasteringVoice* GetMasterVoice() const { return m_masterVoice; }

    /// @brief マスターボリュームを設定する。全サウンドに影響する
    /// @param volume 音量（0.0=無音 〜 1.0=最大）
    void SetMasterVolume(float volume);

private:
    ComPtr<IXAudio2>        m_xaudio2;
    IXAudio2MasteringVoice* m_masterVoice = nullptr;  ///< XAudio2が寿命を管理するのでComPtrは不要
    bool                    m_comInitialized = false;  ///< このインスタンスがCoInitializeExを呼んだか
};

} // namespace GX
