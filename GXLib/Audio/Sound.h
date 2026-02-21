#pragma once
/// @file Sound.h
/// @brief WAVファイル読み込み・PCMデータ管理
///
/// DxLibのLoadSoundMemで読み込まれるサウンドデータに相当する。
/// WAVファイルを読み込み、非圧縮のPCMデータとフォーマット情報をメモリに保持する。
/// 保持したデータはSoundPlayer（SE再生）やMusicPlayer（BGM再生）から参照される。

#include "pch.h"

namespace GX
{

/// @brief WAVファイルから読み込んだPCMデータを保持するクラス（DxLibのサウンドハンドルの実体）
class Sound
{
public:
    Sound() = default;
    ~Sound() = default;

    /// @brief WAVファイルを読み込み、PCMデータとフォーマット情報を取得する
    /// @param filePath WAVファイルのパス（ワイド文字列）
    /// @return 読み込み成功ならtrue
    bool LoadFromFile(const std::wstring& filePath);

    /// @brief PCMデータの先頭ポインタを取得する
    /// @return PCMバイト列へのポインタ
    const uint8_t* GetData() const { return m_pcmData.data(); }

    /// @brief PCMデータのサイズをバイト単位で取得する
    /// @return データサイズ（バイト）
    uint32_t GetDataSize() const { return static_cast<uint32_t>(m_pcmData.size()); }

    /// @brief WAVのフォーマット情報を取得する（サンプルレート、チャンネル数、ビット数等）
    /// @return WAVEFORMATEX構造体への参照
    const WAVEFORMATEX& GetFormat() const { return m_format; }

    /// @brief 有効なPCMデータを保持しているか判定する
    /// @return データが読み込み済みならtrue
    bool IsValid() const { return !m_pcmData.empty(); }

private:
    std::vector<uint8_t> m_pcmData;   ///< WAVから読み込んだ非圧縮PCMデータ
    WAVEFORMATEX         m_format = {}; ///< サンプルレート・チャンネル数・ビット深度等
};

} // namespace GX
