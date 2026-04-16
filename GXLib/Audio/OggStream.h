#pragma once
/// @file OggStream.h
/// @brief OGG Vorbisストリーミング再生
///
/// stb_vorbisを使用してOGGファイルをチャンク単位でデコードしながら再生する。
/// ダブルバッファリングによりメモリ使用量を抑えつつ途切れのない再生を実現する。
/// @addtogroup grp_audio/// @{

#include "pch_audio.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

struct stb_vorbis;

namespace gx
{

class AudioDevice;

/// @brief OGG Vorbisストリーミング再生クラス
class OggStream
{
public:
    static constexpr int k_StreamBufferCount = 2;      ///< ダブルバッファ数
    static constexpr int k_StreamBufferSamples = 16384; ///< 1バッファあたりのサンプル数

    OggStream() = default;
    ~OggStream();

    /// @brief OGGファイルを開く
    /// @param filePath ファイルパス
    /// @return 成功した場合true
    bool Open(const gx::WString& filePath);

    /// @brief ファイルを閉じる
    void Close();

    /// @brief ストリーミング再生を開始する
    /// @param xaudio2 XAudio2インターフェース
    /// @param output 出力先のサブミックスボイス（nullptrでマスターへ直接出力）
    /// @return 成功した場合true
    bool StartStreaming(IXAudio2* xaudio2, IXAudio2SubmixVoice* output = nullptr);

    /// @brief ストリーミング再生を停止する
    void StopStreaming();

    /// @brief 一時停止する
    void PauseStreaming();

    /// @brief 再開する
    void ResumeStreaming();

    /// @brief 音量を設定する
    /// @param vol ボリューム値（0.0〜1.0）
    void SetVolume(float vol);

    /// @brief ループ再生を設定する
    /// @param loop trueでループ有効
    void SetLoop(bool loop);

    /// @brief 毎フレーム更新（バッファ補充処理）
    void Update();

    /// @brief 再生中か判定する
    /// @return 再生中ならtrue
    bool IsPlaying() const;

private:
    void DecoderThread();
    bool FillBuffer(int bufIndex);

    stb_vorbis*  m_vorbis = nullptr;    ///< Vorbisデコーダーハンドル
    int          m_channels = 0;       ///< チャンネル数（1=モノ, 2=ステレオ）
    int          m_sampleRate = 0;     ///< サンプルレート（Hz）

    IXAudio2SourceVoice* m_voice = nullptr;  ///< XAudio2ソースボイス

    gx::Vector<int16_t> m_buffers[k_StreamBufferCount];                       ///< ダブルバッファ
    std::atomic<bool>    m_bufferReady[k_StreamBufferCount] = { false, false }; ///< バッファ準備完了フラグ
    int                  m_currentBuffer = 0;                                   ///< 現在の再生バッファインデックス

    std::thread              m_decoderThread;     ///< デコーダースレッド
    std::mutex               m_mutex;             ///< 同期用ミューテックス
    std::condition_variable  m_cv;                ///< デコーダー起床用条件変数
    std::atomic<bool>        m_running = false;   ///< デコーダースレッド実行中フラグ
    std::atomic<bool>        m_playing = false;   ///< 再生中フラグ
    std::atomic<bool>        m_loop = false;      ///< ループ再生フラグ
    std::atomic<bool>        m_needBuffer = false; ///< バッファ補充要求フラグ
    int                      m_decodeBufferIndex = 0; ///< デコード先バッファインデックス
};

} // namespace gx
/// @}
