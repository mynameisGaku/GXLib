#pragma once
/// @file CloudSave.h
/// @brief クラウドセーブ＆リーダーボードシステム
///
/// クラウドストレージを介したセーブデータの保存・復元、
/// コンフリクト検出と解決、リーダーボード管理を提供する。
/// テスト環境ではインメモリストレージでネットワーク不要で動作する。
/// @addtogroup grp_network/// @{

#include "pch_common.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <functional>
#include <algorithm>

namespace gx
{

/// @brief クラウド接続状態
enum class CloudState
{
    Disconnected,   ///< 未接続
    Connecting,     ///< 接続中
    Connected,      ///< 接続済み
    Syncing,        ///< 同期中
    Error           ///< エラー
};

/// @brief クラウド設定
struct CloudConfig
{
    std::string serverUrl;                  ///< サーバーURL
    std::string appId;                      ///< アプリケーションID
    std::string apiKey;                     ///< APIキー
    bool syncOnSave = true;                 ///< 保存時に自動同期するか
    float autoSyncInterval = 300.0f;        ///< 自動同期間隔（秒）
    int maxRetries = 3;                     ///< 最大リトライ回数
    float timeoutSeconds = 10.0f;           ///< タイムアウト（秒）
    bool enableLeaderboard = true;          ///< リーダーボード機能を有効にするか
    bool enableCloudSave = true;            ///< クラウドセーブ機能を有効にするか
};

/// @brief クラウドセーブスロット
struct CloudSaveSlot
{
    std::string slotId;            ///< スロットID
    std::vector<uint8_t> data;     ///< セーブデータ
    uint64_t timestamp = 0;        ///< 最終更新タイムスタンプ
    uint32_t version = 0;          ///< データバージョン
    uint32_t checksum = 0;         ///< CRC32チェックサム
    std::unordered_map<std::string, std::string> metadata;  ///< メタデータ
    size_t sizeBytes = 0;          ///< データサイズ（バイト）
};

/// @brief リーダーボードエントリ
struct LeaderboardEntry
{
    uint64_t playerId = 0;         ///< プレイヤーID
    std::string playerName;        ///< プレイヤー名
    int64_t score = 0;             ///< スコア
    uint32_t rank = 0;             ///< ランク（1始まり）
    uint64_t timestamp = 0;        ///< 記録日時タイムスタンプ
    std::unordered_map<std::string, std::string> metadata;  ///< メタデータ
};

/// @brief リーダーボード更新ポリシー
enum class LeaderboardUpdatePolicy
{
    KeepBest,       ///< 最高スコアのみ保持
    AlwaysUpdate,   ///< 常に更新
    Increment       ///< 加算
};

/// @brief リーダーボードソート順
enum class LeaderboardSortOrder
{
    Ascending,  ///< 昇順（低いほど良い）
    Descending  ///< 降順（高いほど良い）
};

/// @brief リーダーボード設定
struct LeaderboardConfig
{
    std::string boardId;            ///< ボードID
    LeaderboardSortOrder sortOrder = LeaderboardSortOrder::Descending;  ///< ソート順
    uint32_t maxEntries = 100;      ///< 最大エントリ数
    bool allowDuplicates = false;   ///< 同一プレイヤーの重複登録を許可するか
    LeaderboardUpdatePolicy updatePolicy = LeaderboardUpdatePolicy::KeepBest;  ///< 更新ポリシー
};

/// @brief 同期結果
struct SyncResult
{
    bool success = false;              ///< 同期成功したか
    bool conflictDetected = false;     ///< コンフリクトが検出されたか
    uint64_t localTimestamp = 0;       ///< ローカル側のタイムスタンプ
    uint64_t remoteTimestamp = 0;      ///< リモート側のタイムスタンプ
    std::string errorMessage;          ///< エラーメッセージ
};

/// @brief クラウドセーブ＆リーダーボード管理
class CloudSaveManager
{
public:
    CloudSaveManager() = default;
    ~CloudSaveManager() = default;

    /// @brief 設定を設定する
    void SetConfig(const CloudConfig& cfg);

    /// @brief 設定を取得する
    const CloudConfig& GetConfig() const { return m_config; }

    /// @brief 現在の状態を取得する
    CloudState GetState() const { return m_state; }

    /// @brief クラウドに接続する（シミュレーション）
    bool Connect();

    /// @brief クラウドから切断する
    void Disconnect();

    /// @brief 接続中か判定する
    bool IsConnected() const { return m_state == CloudState::Connected; }

    /// @brief クラウドに保存する
    bool SaveToCloud(const std::string& slotId, const std::vector<uint8_t>& data,
                     const std::unordered_map<std::string, std::string>& metadata = {});

    /// @brief クラウドから読み込む
    CloudSaveSlot LoadFromCloud(const std::string& slotId) const;

    /// @brief クラウドセーブを削除する
    bool DeleteCloudSave(const std::string& slotId);

    /// @brief 利用可能なスロットIDリストを取得する
    std::vector<std::string> GetCloudSlots() const;

    /// @brief スロット情報を取得する（メタデータのみ）
    CloudSaveSlot GetSlotInfo(const std::string& slotId) const;

    /// @brief クラウドセーブが存在するか判定する
    bool HasCloudSave(const std::string& slotId) const;

    /// @brief 同期する
    SyncResult SyncSlot(const std::string& slotId);

    /// @brief 全スロットを同期する
    std::vector<SyncResult> SyncAll();

    /// @brief コンフリクトを解決する
    bool ResolveConflict(const std::string& slotId, bool useLocal);

    /// @brief スコアを送信する
    bool SubmitScore(const std::string& boardId, int64_t score,
                     const std::string& playerName,
                     const std::unordered_map<std::string, std::string>& metadata = {});

    /// @brief リーダーボードを取得する
    std::vector<LeaderboardEntry> GetLeaderboard(const std::string& boardId,
                                                  uint32_t offset = 0, uint32_t count = 10) const;

    /// @brief プレイヤーランクを取得する
    LeaderboardEntry GetPlayerRank(const std::string& boardId, uint64_t playerId) const;

    /// @brief リーダーボードのエントリ数を取得する
    uint32_t GetLeaderboardCount(const std::string& boardId) const;

    /// @brief リーダーボードを作成する
    bool CreateLeaderboard(const LeaderboardConfig& config);

    /// @brief リーダーボードを削除する
    bool DeleteLeaderboard(const std::string& boardId);

    /// @brief プレイヤー周辺のエントリを取得する
    std::vector<LeaderboardEntry> GetAroundPlayer(const std::string& boardId,
                                                   uint64_t playerId, uint32_t count) const;

    /// @brief 更新処理（同期キュー処理・タイムアウト）
    void Update(float deltaTime);

private:
    uint32_t ComputeChecksum(const std::vector<uint8_t>& data) const;
    uint64_t GetCurrentTimestamp() const;
    void SortLeaderboard(const std::string& boardId);

    CloudConfig m_config;
    CloudState m_state = CloudState::Disconnected;

    // インメモリクラウドストレージ（シミュレーション）
    std::unordered_map<std::string, CloudSaveSlot> m_cloudSlots;

    // ローカルキャッシュ
    std::unordered_map<std::string, CloudSaveSlot> m_localSlots;

    // コンフリクト中のスロット
    std::unordered_map<std::string, bool> m_conflicts; ///< slotId -> hasConflict

    // リーダーボード
    std::unordered_map<std::string, LeaderboardConfig> m_leaderboardConfigs;
    std::unordered_map<std::string, std::vector<LeaderboardEntry>> m_leaderboards;

    // 同期タイマー
    float m_syncTimer = 0.0f;
};

} // namespace gx
/// @}
