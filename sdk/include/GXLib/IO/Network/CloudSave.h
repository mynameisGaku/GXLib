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
    gx::String serverUrl;                  ///< サーバーURL
    gx::String appId;                      ///< アプリケーションID
    gx::String apiKey;                     ///< APIキー
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
    gx::String slotId;            ///< スロットID
    gx::Vector<uint8_t> data;     ///< セーブデータ
    uint64_t timestamp = 0;        ///< 最終更新タイムスタンプ
    uint32_t version = 0;          ///< データバージョン
    uint32_t checksum = 0;         ///< CRC32チェックサム
    gx::HashMap<gx::String, gx::String> metadata;  ///< メタデータ
    size_t sizeBytes = 0;          ///< データサイズ（バイト）
};

/// @brief リーダーボードエントリ
struct LeaderboardEntry
{
    uint64_t playerId = 0;         ///< プレイヤーID
    gx::String playerName;        ///< プレイヤー名
    int64_t score = 0;             ///< スコア
    uint32_t rank = 0;             ///< ランク（1始まり）
    uint64_t timestamp = 0;        ///< 記録日時タイムスタンプ
    gx::HashMap<gx::String, gx::String> metadata;  ///< メタデータ
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
    gx::String boardId;            ///< ボードID
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
    gx::String errorMessage;          ///< エラーメッセージ
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
    bool SaveToCloud(const gx::String& slotId, const gx::Vector<uint8_t>& data,
                     const gx::HashMap<gx::String, gx::String>& metadata = {});

    /// @brief クラウドから読み込む
    CloudSaveSlot LoadFromCloud(const gx::String& slotId) const;

    /// @brief クラウドセーブを削除する
    bool DeleteCloudSave(const gx::String& slotId);

    /// @brief 利用可能なスロットIDリストを取得する
    gx::Vector<gx::String> GetCloudSlots() const;

    /// @brief スロット情報を取得する（メタデータのみ）
    CloudSaveSlot GetSlotInfo(const gx::String& slotId) const;

    /// @brief クラウドセーブが存在するか判定する
    bool HasCloudSave(const gx::String& slotId) const;

    /// @brief 同期する
    SyncResult SyncSlot(const gx::String& slotId);

    /// @brief 全スロットを同期する
    gx::Vector<SyncResult> SyncAll();

    /// @brief コンフリクトを解決する
    bool ResolveConflict(const gx::String& slotId, bool useLocal);

    /// @brief スコアを送信する
    bool SubmitScore(const gx::String& boardId, int64_t score,
                     const gx::String& playerName,
                     const gx::HashMap<gx::String, gx::String>& metadata = {});

    /// @brief リーダーボードを取得する
    gx::Vector<LeaderboardEntry> GetLeaderboard(const gx::String& boardId,
                                                  uint32_t offset = 0, uint32_t count = 10) const;

    /// @brief プレイヤーランクを取得する
    LeaderboardEntry GetPlayerRank(const gx::String& boardId, uint64_t playerId) const;

    /// @brief リーダーボードのエントリ数を取得する
    uint32_t GetLeaderboardCount(const gx::String& boardId) const;

    /// @brief リーダーボードを作成する
    bool CreateLeaderboard(const LeaderboardConfig& config);

    /// @brief リーダーボードを削除する
    bool DeleteLeaderboard(const gx::String& boardId);

    /// @brief プレイヤー周辺のエントリを取得する
    gx::Vector<LeaderboardEntry> GetAroundPlayer(const gx::String& boardId,
                                                   uint64_t playerId, uint32_t count) const;

    /// @brief 更新処理（同期キュー処理・タイムアウト）
    void Update(float deltaTime);

private:
    uint32_t ComputeChecksum(const gx::Vector<uint8_t>& data) const;
    uint64_t GetCurrentTimestamp() const;
    void SortLeaderboard(const gx::String& boardId);

    CloudConfig m_config;
    CloudState m_state = CloudState::Disconnected;

    // インメモリクラウドストレージ（シミュレーション）
    gx::HashMap<gx::String, CloudSaveSlot> m_cloudSlots;

    // ローカルキャッシュ
    gx::HashMap<gx::String, CloudSaveSlot> m_localSlots;

    // コンフリクト中のスロット
    gx::HashMap<gx::String, bool> m_conflicts; ///< slotId -> hasConflict

    // リーダーボード
    gx::HashMap<gx::String, LeaderboardConfig> m_leaderboardConfigs;
    gx::HashMap<gx::String, gx::Vector<LeaderboardEntry>> m_leaderboards;

    // 同期タイマー
    float m_syncTimer = 0.0f;
};

} // namespace gx
/// @}
