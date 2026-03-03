#pragma once
/// @file SoundBank.h
/// @brief キューベースの再生（ランダム/シーケンシャル/シャッフル）を持つサウンドバンク
///
/// 足音や銃声など複数バリエーションを持つ効果音を管理し、
/// 同じ音が繰り返されることによる不自然さを防ぐ。
/// ランダム、順番、シャッフルの3モードでエントリを選んで再生する。
/// @addtogroup grp_audio/// @{

#include "pch_audio.h"
#include <functional>
#include <vector>
#include <string>

namespace gx
{

class AudioManager;

/// @brief サウンドキューの再生モード
enum class SoundCueMode
{
    Random,      ///< 毎回ランダムにエントリを選択
    Sequential,  ///< 順番に再生し、末尾で先頭に戻る
    Shuffle,     ///< シャッフル順で各エントリを1回ずつ再生してから繰り返す
};

/// @brief サウンドキュー内の単一エントリ
struct SoundCueEntry
{
    int handle = -1;          ///< AudioManagerのサウンドハンドル
    float volumeMin = 1.0f;  ///< ランダム音量の最小値
    float volumeMax = 1.0f;  ///< ランダム音量の最大値
    float pitchMin = 1.0f;   ///< ランダムピッチの最小値（XAudio2未適用）
    float pitchMax = 1.0f;   ///< ランダムピッチの最大値
    float weight = 1.0f;     ///< 選択ウェイト（Randomモード用）
};

/// @brief 複数のサウンドバリエーションを管理するサウンドキュー
struct SoundCue
{
    gx::String name;
    SoundCueMode mode = SoundCueMode::Random;
    gx::Vector<SoundCueEntry> entries;
    float cooldown = 0.0f;            ///< 再生間の最小待機時間（秒）
    int maxConcurrent = 4;            ///< 最大同時再生数

    // ランタイム状態
    int sequentialIndex = 0;
    gx::Vector<int> shuffleOrder;
    int shuffleIndex = 0;
    float lastPlayTime = -1000.0f;    ///< 最後に再生した時刻
    int currentConcurrent = 0;
};

/// @brief サウンドキューのコレクションを管理するサウンドバンク
class SoundBank
{
public:
    SoundBank() = default;
    ~SoundBank() = default;

    /// @brief 新しいサウンドキューを作成する
    /// @return 後で参照するためのキューインデックス
    int CreateCue(const gx::String& name, SoundCueMode mode = SoundCueMode::Random);

    /// @brief キューにエントリを追加する
    void AddEntry(int cueIndex, const SoundCueEntry& entry);

    /// @brief キューのクールダウンを設定する
    void SetCooldown(int cueIndex, float seconds);

    /// @brief キューの最大同時再生数を設定する
    void SetMaxConcurrent(int cueIndex, int max);

    /// @brief キューを再生する（モードに基づいてエントリを選択）
    /// @param cueIndex キューインデックス
    /// @param audioManager 再生に使用するAudioManager
    /// @param currentTime 現在のゲーム時間（秒）
    /// @return 再生されたサウンドハンドル、スキップされた場合は-1（クールダウン/同時再生制限）
    int Play(int cueIndex, AudioManager& audioManager, float currentTime);

    /// @brief 3D位置指定でキューを再生する
    /// @note AudioEmitterが必要 -- 現在は簡易的にPlaySoundを使用
    int Play3D(int cueIndex, AudioManager& audioManager, float currentTime,
               float volume = 1.0f);

    /// @brief 名前でキューを検索する
    /// @return キューインデックス、見つからない場合は-1
    int FindCue(const gx::String& name) const;

    /// @brief キュー数を取得する
    int GetCueCount() const { return static_cast<int>(m_cues.size()); }

    /// @brief インデックスでキューを取得する
    const SoundCue& GetCue(int index) const { return m_cues[index]; }

    /// @brief 更新（同時再生カウントの減少など）
    void Update(float deltaTime);

    /// @brief すべてのキューをクリアする
    void Clear();

private:
    int SelectEntry(SoundCue& cue);
    float RandomFloat(float min, float max) const;
    void InitializeShuffle(SoundCue& cue);

    gx::Vector<SoundCue> m_cues;
};

} // namespace gx
/// @}
