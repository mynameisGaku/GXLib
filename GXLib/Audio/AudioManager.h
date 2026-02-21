#pragma once
/// @file AudioManager.h
/// @brief オーディオマネージャー -- ハンドルベースのサウンド管理
///
/// DxLibのLoadSoundMem / PlaySoundMem / DeleteSoundMemに相当するハンドル管理を提供する。
/// TextureManagerと同じパターンで、LoadSound()が整数ハンドルを返し、
/// 以降はそのハンドルでSE再生・BGM再生・解放を行う。
/// 同一パスの二重読み込みはキャッシュで防止し、解放済みハンドルはフリーリストで再利用する。

#include "pch.h"
#include "Audio/AudioDevice.h"
#include "Audio/Sound.h"
#include "Audio/SoundPlayer.h"
#include "Audio/MusicPlayer.h"
#include "Audio/AudioMixer.h"

namespace GX
{

class AudioListener;
class AudioEmitter;

/// @brief ハンドルベースでサウンドを管理するクラス（DxLibのLoadSoundMem/PlaySoundMem相当）
///
/// 3D空間音響（X3DAudio）とオーディオミキサー（SubmixVoice）にも対応。
/// PlaySound3D()で3Dサウンドを再生し、Update()でリスナーとの空間計算を行う。
/// ミキサーのBGM/SE/Voiceバスで、カテゴリ別のボリューム制御ができる。
class AudioManager
{
public:
    static constexpr uint32_t k_MaxSounds = 256;  ///< 同時管理可能なサウンド数の上限

    AudioManager() = default;
    ~AudioManager() = default;

    /// @brief オーディオシステム全体を初期化する（AudioDevice + SoundPlayer + MusicPlayer + Mixer）
    /// @return 成功すればtrue
    bool Initialize();

    /// @brief WAVファイルを読み込み、サウンドハンドルを返す（DxLibのLoadSoundMem相当）
    /// @param filePath WAVファイルのパス（ワイド文字列）
    /// @return サウンドハンドル（0以上）。失敗時は-1
    int LoadSound(const std::wstring& filePath);

    /// @brief 効果音を再生する（DxLibのPlaySoundMem相当）
    /// @param handle LoadSound()で取得したハンドル
    /// @param volume 音量（0.0〜1.0、デフォルト1.0）
    /// @param pan 左右パン（-1.0〜1.0、デフォルト0.0=中央）
    void PlaySound(int handle, float volume = 1.0f, float pan = 0.0f);

    /// @brief 効果音を指定バスに出力して再生する
    /// @param handle サウンドハンドル
    /// @param bus 出力先のバス
    /// @param volume 音量（0.0〜1.0、デフォルト1.0）
    void PlaySoundOnBus(int handle, AudioBus& bus, float volume = 1.0f);

    /// @brief 3D空間内で効果音を再生する
    /// @param handle サウンドハンドル
    /// @param emitter 音源位置情報
    /// @param volume 音量（0.0〜1.0、デフォルト1.0）
    /// @return ボイスID（停止用、-1で失敗）
    int PlaySound3D(int handle, AudioEmitter& emitter, float volume = 1.0f);

    /// @brief 3Dサウンドを停止する
    /// @param voiceId PlaySound3Dの戻り値
    void StopSound3D(int voiceId);

    /// @brief BGMを再生する（DxLibのPlayMusic相当）
    /// @param handle LoadSound()で取得したハンドル
    /// @param loop ループ再生するか（デフォルトtrue）
    /// @param volume 音量（0.0〜1.0、デフォルト1.0）
    void PlayMusic(int handle, bool loop = true, float volume = 1.0f);

    /// @brief BGMを停止する（DxLibのStopMusic相当）
    void StopMusic();

    /// @brief BGMを一時停止する
    void PauseMusic();

    /// @brief 一時停止中のBGMを再開する
    void ResumeMusic();

    /// @brief BGMのフェードインを開始する
    /// @param seconds フェードにかける秒数
    void FadeInMusic(float seconds);

    /// @brief BGMのフェードアウトを開始する。完了後に自動停止する
    /// @param seconds フェードにかける秒数
    void FadeOutMusic(float seconds);

    /// @brief BGMが再生中か判定する
    /// @return 再生中ならtrue
    bool IsMusicPlaying() const { return m_musicPlayer.IsPlaying(); }

    /// @brief 指定ハンドルの音量を設定する（現状はPlay時のvolumeで代用）
    /// @param handle サウンドハンドル
    /// @param volume 音量（0.0〜1.0）
    void SetSoundVolume(int handle, float volume);

    /// @brief マスターボリュームを設定する。全サウンドに影響する
    /// @param volume 音量（0.0〜1.0）
    void SetMasterVolume(float volume);

    /// @brief 3Dオーディオのリスナーを設定する（毎フレーム更新推奨）
    /// @param listener リスナー位置情報
    void SetListener(const AudioListener& listener);

    /// @brief フレーム更新。BGMのフェード処理、3D計算、SEクリーンアップを行う
    /// @param deltaTime 前フレームからの経過秒数
    void Update(float deltaTime);

    /// @brief オーディオシステムを終了する。全サウンドを解放してデバイスを破棄する
    void Shutdown();

    /// @brief サウンドハンドルを解放する（DxLibのDeleteSoundMem相当）
    /// @param handle 解放するサウンドハンドル
    void ReleaseSound(int handle);

    /// @brief AudioDeviceへの参照を取得する
    /// @return AudioDeviceへの参照
    AudioDevice& GetDevice() { return m_device; }

    /// @brief SoundPlayerへの参照を取得する
    /// @return SoundPlayerへの参照
    SoundPlayer& GetSoundPlayer() { return m_soundPlayer; }

    /// @brief MusicPlayerへの参照を取得する
    /// @return MusicPlayerへの参照
    MusicPlayer& GetMusicPlayer() { return m_musicPlayer; }

    /// @brief オーディオミキサーへの参照を取得する
    /// @return AudioMixerへの参照
    AudioMixer& GetMixer() { return m_mixer; }

private:
    /// @brief 新しいハンドルを割り当てる。フリーリストがあればそこから再利用する
    /// @return 割り当てられたハンドル番号
    int AllocateHandle();

    AudioDevice  m_device;
    SoundPlayer  m_soundPlayer;
    MusicPlayer  m_musicPlayer;
    AudioMixer   m_mixer;

    /// @brief ハンドルに対応するサウンドデータとファイルパスのペア
    struct SoundEntry
    {
        std::unique_ptr<Sound> sound;   ///< PCMデータ本体
        std::wstring filePath;          ///< 読み込み元パス（キャッシュ用）
    };

    std::vector<SoundEntry>                m_entries;       ///< ハンドル→エントリのマッピング
    std::unordered_map<std::wstring, int>  m_pathCache;     ///< パス→ハンドルの重複読み込み防止キャッシュ
    std::vector<int>                       m_freeHandles;   ///< 解放済みハンドルの再利用リスト
    int                                    m_nextHandle = 0; ///< 次に割り当てる新規ハンドル番号

    // 3Dオーディオ: 最後に設定されたリスナー位置を保持
    const AudioListener* m_currentListener = nullptr;
};

} // namespace GX
