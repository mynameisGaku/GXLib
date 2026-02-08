#pragma once
/// @file Sound.h
/// @brief WAVファイル読み込み・PCMデータ管理
///
/// 【初学者向け解説】
/// WAV（Waveform Audio）は最も基本的な音声ファイル形式です。
/// 構造はシンプルで：
/// - RIFFヘッダー: ファイル識別子
/// - fmtチャンク: 音声フォーマット情報（サンプルレート、ビット数等）
/// - dataチャンク: 実際のPCM音声データ
///
/// PCM（Pulse Code Modulation）はデジタル音声の生データです。
/// 圧縮されていないので、そのままXAudio2のSourceVoiceに渡せます。

#include "pch.h"

namespace GX
{

/// @brief WAVファイルのPCMデータを保持するクラス
class Sound
{
public:
    Sound() = default;
    ~Sound() = default;

    /// WAVファイルを読み込む
    bool LoadFromFile(const std::wstring& filePath);

    /// PCMデータへのポインタ
    const uint8_t* GetData() const { return m_pcmData.data(); }

    /// PCMデータサイズ（バイト）
    uint32_t GetDataSize() const { return static_cast<uint32_t>(m_pcmData.size()); }

    /// WAVフォーマット情報
    const WAVEFORMATEX& GetFormat() const { return m_format; }

    /// 有効なデータを持っているか
    bool IsValid() const { return !m_pcmData.empty(); }

private:
    std::vector<uint8_t> m_pcmData;
    WAVEFORMATEX         m_format = {};
};

} // namespace GX
