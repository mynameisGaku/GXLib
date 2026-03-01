/// @file MatchmakingLobby.cpp
/// @brief マッチメイキング＆ロビーシステム実装
#include "pch_common.h"
#include "IO/Network/MatchmakingLobby.h"
#include <algorithm>
#include <random>
#include <chrono>
#include <cstring>

namespace gx
{

MatchmakingLobby::MatchmakingLobby()
{
    m_localPlayer.playerId = 0;
    m_localPlayer.displayName = "Player";
    m_inLobby = false;
    m_searchTime = 0.0f;
    m_nextLobbyId = 1;
}

void MatchmakingLobby::SetLocalPlayer(const PlayerInfo& info)
{
    m_localPlayer = info;

    // ロビー内なら自分の情報も更新する
    if (m_inLobby)
    {
        for (auto& p : m_currentLobby.players)
        {
            if (p.playerId == m_localPlayer.playerId)
            {
                p.displayName = info.displayName;
                p.rating = info.rating;
                p.customData = info.customData;
                break;
            }
        }
    }
}

uint64_t MatchmakingLobby::CreateLobby(const LobbyConfig& config)
{
    // ロビーを作成
    uint64_t lobbyId = GenerateLobbyId();

    m_currentLobby = {};
    m_currentLobby.lobbyId = lobbyId;
    m_currentLobby.hostPlayer = m_localPlayer.playerId;
    m_currentLobby.config = config;
    m_currentLobby.state = MatchmakingState::InLobby;
    m_currentLobby.region = config.region;

    auto now = std::chrono::steady_clock::now();
    m_currentLobby.createdTime = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count());

    // ローカルプレイヤーをホストとして追加
    PlayerInfo hostPlayer = m_localPlayer;
    hostPlayer.isHost = true;
    m_currentLobby.players.push_back(hostPlayer);

    m_inLobby = true;
    SetState(MatchmakingState::InLobby);

    return lobbyId;
}

bool MatchmakingLobby::JoinLobby(uint64_t lobbyId)
{
    if (m_inLobby) return false;

    // シミュレーション: ロビーIDが現在のロビーと一致するか確認
    // テスト用に、同一プロセス内でのロビー参加をシミュレート
    if (m_currentLobby.lobbyId != lobbyId)
    {
        // 新しいロビーとして作成（テスト環境用）
        m_currentLobby.lobbyId = lobbyId;
        m_currentLobby.state = MatchmakingState::InLobby;
    }

    // プレイヤー数チェック
    if (static_cast<int>(m_currentLobby.players.size()) >= m_currentLobby.config.maxPlayers)
    {
        SetState(MatchmakingState::Failed);
        return false;
    }

    // ローカルプレイヤーを追加
    PlayerInfo joiningPlayer = m_localPlayer;
    joiningPlayer.isHost = false;
    m_currentLobby.players.push_back(joiningPlayer);

    m_inLobby = true;
    SetState(MatchmakingState::InLobby);

    if (OnPlayerJoined) OnPlayerJoined(joiningPlayer);

    return true;
}

bool MatchmakingLobby::JoinByCode(const std::string& inviteCode)
{
    auto it = m_inviteCodes.find(inviteCode);
    if (it == m_inviteCodes.end())
    {
        SetState(MatchmakingState::Failed);
        return false;
    }

    return JoinLobby(it->second);
}

bool MatchmakingLobby::LeaveLobby()
{
    if (!m_inLobby) return false;

    PlayerInfo leavingPlayer = m_localPlayer;

    // ロビーからプレイヤーを削除
    auto& players = m_currentLobby.players;
    players.erase(
        std::remove_if(players.begin(), players.end(),
            [this](const PlayerInfo& p) { return p.playerId == m_localPlayer.playerId; }),
        players.end());

    m_inLobby = false;

    if (OnPlayerLeft) OnPlayerLeft(leavingPlayer);

    SetState(MatchmakingState::Idle);

    return true;
}

const LobbyInfo* MatchmakingLobby::GetCurrentLobby() const
{
    if (!m_inLobby) return nullptr;
    return &m_currentLobby;
}

bool MatchmakingLobby::IsInLobby() const
{
    return m_inLobby;
}

bool MatchmakingLobby::IsHost() const
{
    if (!m_inLobby) return false;
    return m_currentLobby.hostPlayer == m_localPlayer.playerId;
}

int MatchmakingLobby::GetPlayerCount() const
{
    if (!m_inLobby) return 0;
    return static_cast<int>(m_currentLobby.players.size());
}

const PlayerInfo* MatchmakingLobby::GetPlayer(uint64_t playerId) const
{
    if (!m_inLobby) return nullptr;

    for (const auto& p : m_currentLobby.players)
    {
        if (p.playerId == playerId) return &p;
    }
    return nullptr;
}

std::vector<PlayerInfo> MatchmakingLobby::FindPlayers() const
{
    if (!m_inLobby) return {};
    return m_currentLobby.players;
}

void MatchmakingLobby::SetReady(bool ready)
{
    if (!m_inLobby) return;

    m_localPlayer.isReady = ready;

    for (auto& p : m_currentLobby.players)
    {
        if (p.playerId == m_localPlayer.playerId)
        {
            p.isReady = ready;
            break;
        }
    }
}

void MatchmakingLobby::SetTeam(int teamId)
{
    if (!m_inLobby) return;

    m_localPlayer.teamId = teamId;

    for (auto& p : m_currentLobby.players)
    {
        if (p.playerId == m_localPlayer.playerId)
        {
            p.teamId = teamId;
            break;
        }
    }
}

bool MatchmakingLobby::KickPlayer(uint64_t playerId)
{
    if (!m_inLobby) return false;
    if (!IsHost()) return false;
    if (playerId == m_localPlayer.playerId) return false; // 自分をキックできない

    auto& players = m_currentLobby.players;
    PlayerInfo kickedPlayer;
    bool found = false;

    for (const auto& p : players)
    {
        if (p.playerId == playerId)
        {
            kickedPlayer = p;
            found = true;
            break;
        }
    }

    if (!found) return false;

    players.erase(
        std::remove_if(players.begin(), players.end(),
            [playerId](const PlayerInfo& p) { return p.playerId == playerId; }),
        players.end());

    if (OnPlayerLeft) OnPlayerLeft(kickedPlayer);

    return true;
}

bool MatchmakingLobby::StartMatch()
{
    if (!m_inLobby) return false;
    if (!IsHost()) return false;
    if (!CanStart()) return false;

    SetState(MatchmakingState::Starting);
    return true;
}

bool MatchmakingLobby::CanStart() const
{
    if (!m_inLobby) return false;

    int playerCount = static_cast<int>(m_currentLobby.players.size());
    if (playerCount < m_currentLobby.config.minPlayersToStart) return false;

    // 全プレイヤーがレディか確認
    for (const auto& p : m_currentLobby.players)
    {
        if (!p.isReady) return false;
    }

    return true;
}

void MatchmakingLobby::StartSearch(const MatchFilter& filter)
{
    m_matchFilter = filter;
    m_searchTime = 0.0f;
    SetState(MatchmakingState::Searching);
}

void MatchmakingLobby::CancelSearch()
{
    if (m_state == MatchmakingState::Searching)
    {
        m_searchTime = 0.0f;
        SetState(MatchmakingState::Idle);
    }
}

std::string MatchmakingLobby::GenerateInviteCode() const
{
    static const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static constexpr int kCodeLength = 6;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, static_cast<int>(sizeof(chars) - 2));

    std::string code;
    code.reserve(kCodeLength);
    for (int i = 0; i < kCodeLength; i++)
    {
        code += chars[dist(gen)];
    }

    return code;
}

void MatchmakingLobby::SetMatchFilter(const MatchFilter& filter)
{
    m_matchFilter = filter;
}

void MatchmakingLobby::SendChatMessage(const std::string& message)
{
    if (!m_inLobby) return;

    std::string senderName = m_localPlayer.displayName;
    m_chatHistory.emplace_back(senderName, message);

    if (OnChatMessage) OnChatMessage(senderName, message);
}

void MatchmakingLobby::Update(float deltaTime)
{
    switch (m_state)
    {
    case MatchmakingState::Searching:
    {
        m_searchTime += deltaTime;

        // タイムアウト判定
        if (m_searchTime >= m_currentLobby.config.matchmakingTimeout &&
            m_currentLobby.config.matchmakingTimeout > 0.0f)
        {
            SetState(MatchmakingState::Failed);
        }
        break;
    }
    case MatchmakingState::InLobby:
    {
        // autoStartが有効かつCanStartならカウントダウン処理
        // （実プロダクションではカウントダウンタイマーを管理）
        break;
    }
    default:
        break;
    }
}

void MatchmakingLobby::SetState(MatchmakingState newState)
{
    if (m_state == newState) return;
    m_state = newState;
    if (OnStateChanged) OnStateChanged(newState);
}

PlayerInfo* MatchmakingLobby::FindPlayerMutable(uint64_t playerId)
{
    for (auto& p : m_currentLobby.players)
    {
        if (p.playerId == playerId) return &p;
    }
    return nullptr;
}

uint64_t MatchmakingLobby::GenerateLobbyId() const
{
    // シンプルなインクリメントID + タイムスタンプベース
    auto now = std::chrono::steady_clock::now();
    uint64_t ts = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
    return (ts << 16) | (m_nextLobbyId & 0xFFFF);
}

} // namespace gx
