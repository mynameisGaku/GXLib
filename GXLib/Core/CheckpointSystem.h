#pragma once
/// @file CheckpointSystem.h
/// @brief チェックポイントシステム — ゲーム中の通過地点を記録・ロードする
///
/// CreateCheckpoint() で現在地点を記録し、RestoreCheckpoint() で
/// その地点に戻る。メタデータを任意に付与できるので
/// 残機制やオートセーブのトリガーとして使う。
/// @addtogroup grp_core/// @{

#include "pch_common.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

namespace gx
{

/// @brief チェックポイントデータ
struct CheckpointData
{
    uint32_t id = 0;                ///< チェックポイントID
    std::string name;               ///< チェックポイント名
    uint64_t timestamp = 0;         ///< 作成時刻（エポック秒）
    std::string locationName;       ///< シーン/レベル名
    std::string screenshotPath;     ///< スクリーンショットファイルパス
    std::unordered_map<std::string, std::string> metadata; ///< 任意のメタデータ
};

/// @brief チェックポイントシステム
class CheckpointSystem
{
public:
    /// @brief コンストラクタ
    /// @param maxCheckpoints 保持できるチェックポイントの最大数
    explicit CheckpointSystem(uint32_t maxCheckpoints = 20);
    ~CheckpointSystem() = default;

    /// @brief チェックポイントを作成する
    /// @param name チェックポイント名
    /// @param location シーン/レベル名（省略可）
    /// @return 作成されたチェックポイントのID
    uint32_t CreateCheckpoint(const std::string& name, const std::string& location = "");

    /// @brief チェックポイントを取得する
    /// @param id チェックポイントID
    /// @return チェックポイントデータへのポインタ（見つからない場合 nullptr）
    const CheckpointData* GetCheckpoint(uint32_t id) const;

    /// @brief チェックポイントを削除する
    /// @param id チェックポイントID
    /// @return 削除に成功した場合 true
    bool RemoveCheckpoint(uint32_t id);

    /// @brief 現在のチェックポイント数を取得する
    /// @return チェックポイント数
    uint32_t GetCheckpointCount() const { return static_cast<uint32_t>(m_checkpoints.size()); }

    /// @brief 全チェックポイントを取得する
    /// @return チェックポイントデータの配列
    std::vector<CheckpointData> GetAllCheckpoints() const;

    /// @brief 最大チェックポイント数を設定する
    /// @param max 最大数
    void SetMaxCheckpoints(uint32_t max) { m_maxCheckpoints = max; }

    /// @brief 最大チェックポイント数を取得する
    /// @return 最大数
    uint32_t GetMaxCheckpoints() const { return m_maxCheckpoints; }

    /// @brief チェックポイントにメタデータを設定する
    /// @param checkpointId チェックポイントID
    /// @param key メタデータキー
    /// @param value メタデータ値
    void SetMetadata(uint32_t checkpointId, const std::string& key, const std::string& value);

    /// @brief チェックポイントのメタデータを取得する
    /// @param checkpointId チェックポイントID
    /// @param key メタデータキー
    /// @return メタデータ値（未設定時は空文字列）
    std::string GetMetadata(uint32_t checkpointId, const std::string& key) const;

    /// @brief クイックセーブする（"QuickSave"名でチェックポイント作成）
    /// @param location シーン/レベル名（省略可）
    /// @return 作成されたチェックポイントのID
    uint32_t QuickSave(const std::string& location = "");

    /// @brief クイックロードする（最後のクイックセーブを返す）
    /// @return チェックポイントデータへのポインタ（存在しない場合 nullptr）
    const CheckpointData* QuickLoad() const;

    /// @brief 自動チェックポイントの有効/無効を設定する
    /// @param enabled true で有効
    void SetAutoOnSceneChange(bool enabled) { m_autoOnSceneChange = enabled; }

    /// @brief 自動チェックポイントが有効かどうかを取得する
    /// @return 有効なら true
    bool IsAutoOnSceneChange() const { return m_autoOnSceneChange; }

    /// @brief シーン変更を通知する（自動チェックポイント有効時に自動作成）
    /// @param sceneName 新しいシーン名
    void NotifySceneChange(const std::string& sceneName);

    /// @brief 全チェックポイントをクリアする
    void ClearAll();

    /// @brief チェックポイント作成時コールバック型
    using CheckpointCallback = std::function<void(const CheckpointData&)>;

    /// @brief チェックポイント作成時のコールバックを設定する
    /// @param cb コールバック関数
    void SetOnCheckpointCreated(CheckpointCallback cb) { m_onCreate = std::move(cb); }

private:
    /// @brief 最大数を超えたチェックポイントを古い順に削除する
    void EnforceMaxCheckpoints();

    std::vector<CheckpointData> m_checkpoints;  ///< チェックポイント一覧
    uint32_t m_nextId = 1;                      ///< 次に割り当てるID
    uint32_t m_maxCheckpoints = 20;             ///< 最大保持数
    uint32_t m_quickSaveId = 0;                 ///< 最後のクイックセーブID
    bool m_autoOnSceneChange = false;           ///< シーン変更時の自動作成フラグ
    CheckpointCallback m_onCreate;              ///< 作成時コールバック
};

} // namespace gx
/// @}
