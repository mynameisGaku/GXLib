#pragma once
/// @file AudioSnapshot.h
/// @brief オーディオスナップショット管理
///
/// ゲーム状態に応じたオーディオ設定のプリセットを管理する。
/// 例: 「通常」→「ポーズ」→「水中」のように、
/// バスのボリュームやミュート状態を一括で切り替えるスナップショット機能。
/// トランジション（時間経過によるブレンド遷移）もサポートする。
/// AudioBus/AudioMixerには依存せず、スナップショットのデータ管理と
/// 遷移状態の追跡のみ行う。
/// @addtogroup grp_audio/// @{

#include "pch_audio.h"

namespace gx
{

/// @brief バス単位のスナップショットデータ
struct BusSnapshot
{
    gx::String busName;     ///< バス名
    float volume = 1.0f;     ///< バス音量（0.0〜1.0）
    bool muted = false;      ///< ミュート状態
    float pan = 0.0f;        ///< パン（-1.0=左, 0.0=中央, 1.0=右）
};

/// @brief オーディオスナップショットデータ
struct AudioSnapshotData
{
    gx::String name;                ///< スナップショット名
    float masterVolume = 1.0f;       ///< マスター音量
    gx::Vector<BusSnapshot> buses;  ///< バス設定リスト
    float transitionTime = 0.5f;     ///< デフォルトトランジション時間（秒）
};

/// @brief オーディオスナップショットマネージャ
///
/// 名前付きスナップショットの登録・適用・ブレンド遷移を管理する。
class AudioSnapshotManager
{
public:
    AudioSnapshotManager() = default;
    ~AudioSnapshotManager() = default;

    /// @brief スナップショットを登録する
    /// @param snapshot スナップショットデータ
    void RegisterSnapshot(const AudioSnapshotData& snapshot);

    /// @brief スナップショットを登録解除する
    /// @param name スナップショット名
    void UnregisterSnapshot(const gx::String& name);

    /// @brief スナップショットを取得する
    /// @param name スナップショット名
    /// @return スナップショットデータへのポインタ（見つからなければnullptr）
    const AudioSnapshotData* GetSnapshot(const gx::String& name) const;

    /// @brief 登録済みスナップショット数を取得する
    /// @return スナップショット数
    size_t GetSnapshotCount() const;

    /// @brief スナップショットを即座に適用する（トランジションなし）
    /// @param name スナップショット名
    void ApplySnapshot(const gx::String& name);

    /// @brief スナップショットへ時間をかけてブレンド遷移する
    /// @param name 遷移先スナップショット名
    /// @param transitionTime 遷移時間（秒）
    void BlendToSnapshot(const gx::String& name, float transitionTime);

    /// @brief トランジション状態を更新する
    /// @param deltaTime 経過時間（秒）
    void Update(float deltaTime);

    /// @brief 現在のスナップショット名を取得する
    /// @return スナップショット名（未設定時は空文字列）
    gx::String GetCurrentSnapshotName() const;

    /// @brief トランジション中かどうか
    /// @return トランジション中ならtrue
    bool IsTransitioning() const;

    /// @brief トランジションの進行度を取得する
    /// @return 進行度（0.0〜1.0、トランジション中でなければ1.0）
    float GetTransitionProgress() const;

    /// @brief 状態をリセットする
    void Reset();

private:
    gx::HashMap<gx::String, AudioSnapshotData> m_snapshots;
    gx::String m_currentName;
    gx::String m_targetName;
    float m_transitionTime = 0.0f;
    float m_transitionElapsed = 0.0f;
    bool m_transitioning = false;
};

} // namespace gx
/// @}
