#pragma once
/// @file MatchmakingLobby.h
/// @brief マッチメイキング＆ロビーシステム
///
/// クイックマッチ、ランクマッチ、カスタム、プライベートの各モードを提供。
/// ロビー作成・参加・検索・招待コード・チャット・チーム分け・レディ管理を含む。
/// レーティングベースのマッチメイキングおよびロビー内ステートマシンを実装する。
/// @addtogroup grp_network/// @{

#include "pch_common.h"
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <cstdint>
#include <utility>

namespace gx
{

/// @brief マッチメイキング状態
enum class MatchmakingState
{
    Idle,       ///< 待機中
    Searching,  ///< マッチ検索中
    Found,      ///< マッチ発見
    Joining,    ///< ロビー参加中
    InLobby,    ///< ロビー内
    Starting,   ///< ゲーム開始中
    Failed      ///< 失敗
};

/// @brief マッチメイキングモード
enum class MatchmakingMode
{
    QuickMatch, ///< クイックマッチ
    Ranked,     ///< ランクマッチ
    Custom,     ///< カスタム
    Private     ///< プライベート
};

/// @brief プレイヤー情報
struct PlayerInfo
{
    uint64_t playerId = 0;       ///< プレイヤーID
    gx::String displayName;     ///< 表示名
    int rating = 1000;           ///< レーティング
    int teamId = -1;             ///< チームID（-1=未割当）
    bool isReady = false;        ///< レディ状態
    bool isHost = false;         ///< ホストか
    uint32_t latencyMs = 0;      ///< レイテンシ（ミリ秒）
    gx::HashMap<gx::String, gx::String> customData;  ///< カスタムデータ
};

/// @brief ロビー設定
struct LobbyConfig
{
    int maxPlayers = 8;              ///< 最大プレイヤー数
    int minPlayersToStart = 2;       ///< 開始に必要な最小人数
    gx::String lobbyName;           ///< ロビー名
    bool isPrivate = false;          ///< プライベートロビーか
    gx::String password;            ///< パスワード（空=パスワードなし）
    bool autoStart = true;           ///< 全員レディで自動開始するか
    int countdownSeconds = 5;        ///< 開始カウントダウン（秒）
    gx::String gameMode = "default"; ///< ゲームモード
    gx::String region = "auto";     ///< 地域（"auto"で自動選択）
    float matchmakingTimeout = 30.0f; ///< マッチメイキングタイムアウト（秒）
};

/// @brief マッチフィルタ
struct MatchFilter
{
    gx::String gameMode;              ///< ゲームモードフィルタ
    int minRating = -1;                ///< 最小レーティング（-1=制限なし）
    int maxRating = -1;                ///< 最大レーティング（-1=制限なし）
    int minPlayers = -1;               ///< 最小プレイヤー数（-1=制限なし）
    int maxPlayers = -1;               ///< 最大プレイヤー数（-1=制限なし）
    gx::String region;                ///< 地域フィルタ
    gx::Vector<gx::String> tags;     ///< タグフィルタ
};

/// @brief ロビー情報
struct LobbyInfo
{
    uint64_t lobbyId = 0;          ///< ロビーID
    uint64_t hostPlayer = 0;       ///< ホストのプレイヤーID
    LobbyConfig config;            ///< ロビー設定
    gx::Vector<PlayerInfo> players; ///< 参加プレイヤーリスト
    MatchmakingState state = MatchmakingState::Idle;  ///< 現在の状態
    uint64_t createdTime = 0;      ///< 作成時刻
    gx::String region;            ///< サーバー地域
};

/// @brief マッチ結果
struct MatchResult
{
    uint64_t lobbyId = 0;          ///< ロビーID
    bool success = false;          ///< 成功したか
    gx::String errorMessage;      ///< エラーメッセージ
    gx::String serverAddress;     ///< 接続先サーバーアドレス
    uint16_t serverPort = 0;       ///< 接続先ポート番号
};

/// @brief マッチメイキング＆ロビーシステム
class MatchmakingLobby
{
public:
    MatchmakingLobby();
    ~MatchmakingLobby() = default;

    /// @brief ローカルプレイヤーを設定する
    void SetLocalPlayer(const PlayerInfo& info);

    /// @brief ローカルプレイヤーを取得する
    const PlayerInfo& GetLocalPlayer() const { return m_localPlayer; }

    /// @brief 現在の状態を取得する
    MatchmakingState GetState() const { return m_state; }

    /// @brief ロビーを作成する
    /// @return ロビーID
    uint64_t CreateLobby(const LobbyConfig& config);

    /// @brief ロビーに参加する
    bool JoinLobby(uint64_t lobbyId);

    /// @brief 招待コードでロビーに参加する
    bool JoinByCode(const gx::String& inviteCode);

    /// @brief ロビーから退出する
    bool LeaveLobby();

    /// @brief 現在のロビー情報を取得する
    const LobbyInfo* GetCurrentLobby() const;

    /// @brief ロビーに参加中か判定する
    bool IsInLobby() const;

    /// @brief ホストか判定する
    bool IsHost() const;

    /// @brief ロビー内のプレイヤー数を取得する
    int GetPlayerCount() const;

    /// @brief 指定プレイヤーを取得する
    const PlayerInfo* GetPlayer(uint64_t playerId) const;

    /// @brief ロビー内の全プレイヤーを取得する
    gx::Vector<PlayerInfo> FindPlayers() const;

    /// @brief レディ状態を設定する
    void SetReady(bool ready);

    /// @brief チームを設定する
    void SetTeam(int teamId);

    /// @brief プレイヤーをキックする（ホスト専用）
    bool KickPlayer(uint64_t playerId);

    /// @brief マッチを開始する（ホスト専用、全員レディ時）
    bool StartMatch();

    /// @brief 開始可能か判定する
    bool CanStart() const;

    /// @brief マッチメイキング検索を開始する
    void StartSearch(const MatchFilter& filter);

    /// @brief 検索をキャンセルする
    void CancelSearch();

    /// @brief 検索経過時間を取得する（秒）
    float GetSearchTime() const { return m_searchTime; }

    /// @brief 招待コードを生成する
    gx::String GenerateInviteCode() const;

    /// @brief マッチフィルタを設定する
    void SetMatchFilter(const MatchFilter& filter);

    /// @brief マッチフィルタを取得する
    const MatchFilter& GetMatchFilter() const { return m_matchFilter; }

    /// @brief チャットメッセージを送信する
    void SendChatMessage(const gx::String& message);

    /// @brief チャット履歴を取得する
    gx::Vector<std::pair<gx::String, gx::String>> GetChatHistory() const { return m_chatHistory; }

    /// @brief 状態変化コールバック
    using StateCallback = std::function<void(MatchmakingState)>;
    StateCallback OnStateChanged;

    /// @brief プレイヤー参加コールバック
    using PlayerCallback = std::function<void(const PlayerInfo&)>;
    PlayerCallback OnPlayerJoined;

    /// @brief プレイヤー退出コールバック
    PlayerCallback OnPlayerLeft;

    /// @brief チャットメッセージコールバック
    using ChatCallback = std::function<void(const gx::String&, const gx::String&)>;
    ChatCallback OnChatMessage;

    /// @brief 更新処理（状態遷移・タイムアウト処理）
    void Update(float deltaTime);

private:
    void SetState(MatchmakingState newState);
    PlayerInfo* FindPlayerMutable(uint64_t playerId);
    uint64_t GenerateLobbyId() const;

    MatchmakingState m_state = MatchmakingState::Idle;  ///< 現在の状態
    PlayerInfo m_localPlayer;                            ///< ローカルプレイヤー情報
    LobbyInfo m_currentLobby;                            ///< 現在のロビー情報
    bool m_inLobby = false;                              ///< ロビー参加中か
    MatchFilter m_matchFilter;                           ///< マッチフィルタ
    float m_searchTime = 0.0f;                           ///< 検索経過時間（秒）
    uint64_t m_nextLobbyId = 1;                          ///< 次に割り当てるロビーID
    gx::Vector<std::pair<gx::String, gx::String>> m_chatHistory;  ///< チャット履歴（名前, メッセージ）
    gx::HashMap<gx::String, uint64_t> m_inviteCodes;         ///< 招待コード → ロビーIDマップ
};

} // namespace gx
/// @}
