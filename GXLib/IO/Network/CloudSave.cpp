/// @file CloudSave.cpp
/// @brief クラウドセーブ＆リーダーボードシステム実装
#include "pch_common.h"
#include "IO/Network/CloudSave.h"
#include <chrono>
#include <algorithm>
#include <numeric>

namespace gx
{

void CloudSaveManager::SetConfig(const CloudConfig& cfg)
{
    m_config = cfg;
}

bool CloudSaveManager::Connect()
{
    if (m_state == CloudState::Connected) return true;

    // シミュレーション: 常に接続成功
    m_state = CloudState::Connected;
    return true;
}

void CloudSaveManager::Disconnect()
{
    m_state = CloudState::Disconnected;
}

bool CloudSaveManager::SaveToCloud(const std::string& slotId, const std::vector<uint8_t>& data,
                                    const std::unordered_map<std::string, std::string>& metadata)
{
    if (m_state != CloudState::Connected) return false;

    CloudSaveSlot slot;
    slot.slotId = slotId;
    slot.data = data;
    slot.timestamp = GetCurrentTimestamp();
    slot.metadata = metadata;
    slot.sizeBytes = data.size();
    slot.checksum = ComputeChecksum(data);

    // バージョンをインクリメント
    auto existIt = m_cloudSlots.find(slotId);
    if (existIt != m_cloudSlots.end())
    {
        slot.version = existIt->second.version + 1;
    }
    else
    {
        slot.version = 1;
    }

    // クラウドとローカル両方に保存
    m_cloudSlots[slotId] = slot;
    m_localSlots[slotId] = slot;

    // コンフリクトをクリア
    m_conflicts.erase(slotId);

    return true;
}

CloudSaveSlot CloudSaveManager::LoadFromCloud(const std::string& slotId) const
{
    auto it = m_cloudSlots.find(slotId);
    if (it != m_cloudSlots.end()) return it->second;
    return {};
}

bool CloudSaveManager::DeleteCloudSave(const std::string& slotId)
{
    if (m_state != CloudState::Connected) return false;

    bool erased = m_cloudSlots.erase(slotId) > 0;
    m_localSlots.erase(slotId);
    m_conflicts.erase(slotId);
    return erased;
}

std::vector<std::string> CloudSaveManager::GetCloudSlots() const
{
    std::vector<std::string> result;
    result.reserve(m_cloudSlots.size());
    for (const auto& [id, _] : m_cloudSlots)
    {
        result.push_back(id);
    }
    return result;
}

CloudSaveSlot CloudSaveManager::GetSlotInfo(const std::string& slotId) const
{
    auto it = m_cloudSlots.find(slotId);
    if (it != m_cloudSlots.end())
    {
        // メタデータのみ返す（データ本体は空）
        CloudSaveSlot info;
        info.slotId = it->second.slotId;
        info.timestamp = it->second.timestamp;
        info.version = it->second.version;
        info.checksum = it->second.checksum;
        info.metadata = it->second.metadata;
        info.sizeBytes = it->second.sizeBytes;
        return info;
    }
    return {};
}

bool CloudSaveManager::HasCloudSave(const std::string& slotId) const
{
    return m_cloudSlots.find(slotId) != m_cloudSlots.end();
}

SyncResult CloudSaveManager::SyncSlot(const std::string& slotId)
{
    SyncResult result;

    if (m_state != CloudState::Connected)
    {
        result.errorMessage = "Not connected";
        return result;
    }

    auto localIt = m_localSlots.find(slotId);
    auto cloudIt = m_cloudSlots.find(slotId);

    // ローカルもクラウドもない場合
    if (localIt == m_localSlots.end() && cloudIt == m_cloudSlots.end())
    {
        result.success = true;
        return result;
    }

    // ローカルのみ → アップロード
    if (localIt != m_localSlots.end() && cloudIt == m_cloudSlots.end())
    {
        m_cloudSlots[slotId] = localIt->second;
        result.success = true;
        result.localTimestamp = localIt->second.timestamp;
        return result;
    }

    // クラウドのみ → ダウンロード
    if (localIt == m_localSlots.end() && cloudIt != m_cloudSlots.end())
    {
        m_localSlots[slotId] = cloudIt->second;
        result.success = true;
        result.remoteTimestamp = cloudIt->second.timestamp;
        return result;
    }

    // 両方ある場合 → コンフリクト検出
    result.localTimestamp = localIt->second.timestamp;
    result.remoteTimestamp = cloudIt->second.timestamp;

    if (localIt->second.version != cloudIt->second.version ||
        localIt->second.checksum != cloudIt->second.checksum)
    {
        // コンフリクト
        result.conflictDetected = true;
        m_conflicts[slotId] = true;
        result.success = false;
        result.errorMessage = "Conflict detected";
    }
    else
    {
        result.success = true;
    }

    return result;
}

std::vector<SyncResult> CloudSaveManager::SyncAll()
{
    std::vector<SyncResult> results;

    // ローカルとクラウドの全スロットIDを収集
    std::unordered_map<std::string, bool> allSlots;
    for (const auto& [id, _] : m_localSlots) allSlots[id] = true;
    for (const auto& [id, _] : m_cloudSlots) allSlots[id] = true;

    for (const auto& [id, _] : allSlots)
    {
        results.push_back(SyncSlot(id));
    }

    return results;
}

bool CloudSaveManager::ResolveConflict(const std::string& slotId, bool useLocal)
{
    auto conflictIt = m_conflicts.find(slotId);
    if (conflictIt == m_conflicts.end()) return false;

    if (useLocal)
    {
        auto localIt = m_localSlots.find(slotId);
        if (localIt != m_localSlots.end())
        {
            m_cloudSlots[slotId] = localIt->second;
        }
    }
    else
    {
        auto cloudIt = m_cloudSlots.find(slotId);
        if (cloudIt != m_cloudSlots.end())
        {
            m_localSlots[slotId] = cloudIt->second;
        }
    }

    m_conflicts.erase(slotId);
    return true;
}

bool CloudSaveManager::SubmitScore(const std::string& boardId, int64_t score,
                                    const std::string& playerName,
                                    const std::unordered_map<std::string, std::string>& metadata)
{
    if (m_state != CloudState::Connected) return false;

    // リーダーボードが存在するか確認
    auto cfgIt = m_leaderboardConfigs.find(boardId);
    if (cfgIt == m_leaderboardConfigs.end()) return false;

    const auto& cfg = cfgIt->second;
    auto& entries = m_leaderboards[boardId];

    // プレイヤーIDはplayerNameのハッシュで簡易生成
    uint64_t playerId = std::hash<std::string>{}(playerName);

    // 既存エントリを検索
    auto existIt = std::find_if(entries.begin(), entries.end(),
        [playerId](const LeaderboardEntry& e) { return e.playerId == playerId; });

    if (existIt != entries.end() && !cfg.allowDuplicates)
    {
        // 更新ポリシーに従う
        switch (cfg.updatePolicy)
        {
        case LeaderboardUpdatePolicy::KeepBest:
            if (cfg.sortOrder == LeaderboardSortOrder::Descending)
            {
                if (score <= existIt->score) return true; // 既存が良い
            }
            else
            {
                if (score >= existIt->score) return true; // 既存が良い
            }
            existIt->score = score;
            existIt->timestamp = GetCurrentTimestamp();
            existIt->metadata = metadata;
            break;
        case LeaderboardUpdatePolicy::AlwaysUpdate:
            existIt->score = score;
            existIt->timestamp = GetCurrentTimestamp();
            existIt->metadata = metadata;
            break;
        case LeaderboardUpdatePolicy::Increment:
            existIt->score += score;
            existIt->timestamp = GetCurrentTimestamp();
            existIt->metadata = metadata;
            break;
        }
    }
    else
    {
        // 新規エントリ
        LeaderboardEntry entry;
        entry.playerId = playerId;
        entry.playerName = playerName;
        entry.score = score;
        entry.timestamp = GetCurrentTimestamp();
        entry.metadata = metadata;
        entries.push_back(entry);

        // maxEntries制限
        if (entries.size() > cfg.maxEntries)
        {
            SortLeaderboard(boardId);
            entries.resize(cfg.maxEntries);
        }
    }

    SortLeaderboard(boardId);
    return true;
}

std::vector<LeaderboardEntry> CloudSaveManager::GetLeaderboard(const std::string& boardId,
                                                                uint32_t offset, uint32_t count) const
{
    auto it = m_leaderboards.find(boardId);
    if (it == m_leaderboards.end()) return {};

    const auto& entries = it->second;
    if (offset >= entries.size()) return {};

    uint32_t end = std::min(offset + count, static_cast<uint32_t>(entries.size()));
    std::vector<LeaderboardEntry> result(entries.begin() + offset, entries.begin() + end);

    // ランクを設定
    for (uint32_t i = 0; i < result.size(); i++)
    {
        result[i].rank = offset + i + 1;
    }

    return result;
}

LeaderboardEntry CloudSaveManager::GetPlayerRank(const std::string& boardId, uint64_t playerId) const
{
    auto it = m_leaderboards.find(boardId);
    if (it == m_leaderboards.end()) return {};

    const auto& entries = it->second;
    for (uint32_t i = 0; i < entries.size(); i++)
    {
        if (entries[i].playerId == playerId)
        {
            LeaderboardEntry result = entries[i];
            result.rank = i + 1;
            return result;
        }
    }

    return {};
}

uint32_t CloudSaveManager::GetLeaderboardCount(const std::string& boardId) const
{
    auto it = m_leaderboards.find(boardId);
    if (it == m_leaderboards.end()) return 0;
    return static_cast<uint32_t>(it->second.size());
}

bool CloudSaveManager::CreateLeaderboard(const LeaderboardConfig& config)
{
    if (config.boardId.empty()) return false;

    // 既に存在する場合は失敗
    if (m_leaderboardConfigs.find(config.boardId) != m_leaderboardConfigs.end()) return false;

    m_leaderboardConfigs[config.boardId] = config;
    m_leaderboards[config.boardId] = {};
    return true;
}

bool CloudSaveManager::DeleteLeaderboard(const std::string& boardId)
{
    bool erased = m_leaderboardConfigs.erase(boardId) > 0;
    m_leaderboards.erase(boardId);
    return erased;
}

std::vector<LeaderboardEntry> CloudSaveManager::GetAroundPlayer(const std::string& boardId,
                                                                  uint64_t playerId, uint32_t count) const
{
    auto it = m_leaderboards.find(boardId);
    if (it == m_leaderboards.end()) return {};

    const auto& entries = it->second;
    int playerIndex = -1;
    for (int i = 0; i < static_cast<int>(entries.size()); i++)
    {
        if (entries[i].playerId == playerId)
        {
            playerIndex = i;
            break;
        }
    }

    if (playerIndex < 0) return {};

    int half = static_cast<int>(count) / 2;
    int start = std::max(0, playerIndex - half);
    int end = std::min(static_cast<int>(entries.size()), start + static_cast<int>(count));
    start = std::max(0, end - static_cast<int>(count));

    std::vector<LeaderboardEntry> result;
    for (int i = start; i < end; i++)
    {
        LeaderboardEntry entry = entries[i];
        entry.rank = static_cast<uint32_t>(i + 1);
        result.push_back(entry);
    }

    return result;
}

void CloudSaveManager::Update(float deltaTime)
{
    if (m_state != CloudState::Connected) return;

    m_syncTimer += deltaTime;

    // 自動同期
    if (m_config.autoSyncInterval > 0.0f && m_syncTimer >= m_config.autoSyncInterval)
    {
        m_syncTimer = 0.0f;
        SyncAll();
    }
}

uint32_t CloudSaveManager::ComputeChecksum(const std::vector<uint8_t>& data) const
{
    // シンプルなCRC-like checksum
    uint32_t checksum = 0;
    for (size_t i = 0; i < data.size(); i++)
    {
        checksum = (checksum >> 1) | (checksum << 31);
        checksum += data[i];
        checksum ^= 0xDEADBEEF;
    }
    return checksum;
}

uint64_t CloudSaveManager::GetCurrentTimestamp() const
{
    auto now = std::chrono::steady_clock::now();
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
}

void CloudSaveManager::SortLeaderboard(const std::string& boardId)
{
    auto cfgIt = m_leaderboardConfigs.find(boardId);
    if (cfgIt == m_leaderboardConfigs.end()) return;

    auto& entries = m_leaderboards[boardId];
    if (cfgIt->second.sortOrder == LeaderboardSortOrder::Descending)
    {
        std::sort(entries.begin(), entries.end(),
            [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
                return a.score > b.score;
            });
    }
    else
    {
        std::sort(entries.begin(), entries.end(),
            [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
                return a.score < b.score;
            });
    }

    // ランクを更新
    for (uint32_t i = 0; i < entries.size(); i++)
    {
        entries[i].rank = i + 1;
    }
}

} // namespace gx
