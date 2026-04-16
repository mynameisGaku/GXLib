/// @file test_MatchmakingLobby.cpp
#include <gtest/gtest.h>
#include "IO/Network/MatchmakingLobby.h"
using namespace gx;

TEST(MatchmakingLobbyTest, DefaultState) {
    MatchmakingLobby mm;
    EXPECT_EQ(mm.GetState(), MatchmakingState::Idle);
    EXPECT_FALSE(mm.IsInLobby());
    EXPECT_FALSE(mm.IsHost());
    EXPECT_EQ(mm.GetPlayerCount(), 0);
}

TEST(MatchmakingLobbyTest, SetLocalPlayer) {
    MatchmakingLobby mm;
    PlayerInfo info;
    info.playerId = 42;
    info.displayName = "TestPlayer";
    info.rating = 1500;
    mm.SetLocalPlayer(info);
    EXPECT_EQ(mm.GetLocalPlayer().playerId, 42u);
    EXPECT_EQ(mm.GetLocalPlayer().displayName, "TestPlayer");
    EXPECT_EQ(mm.GetLocalPlayer().rating, 1500);
}

TEST(MatchmakingLobbyTest, CreateLobby) {
    MatchmakingLobby mm;
    PlayerInfo info;
    info.playerId = 1;
    info.displayName = "Host";
    mm.SetLocalPlayer(info);

    LobbyConfig cfg;
    cfg.maxPlayers = 4;
    cfg.lobbyName = "TestLobby";
    uint64_t lobbyId = mm.CreateLobby(cfg);
    EXPECT_NE(lobbyId, 0u);
    EXPECT_TRUE(mm.IsInLobby());
    EXPECT_TRUE(mm.IsHost());
    EXPECT_EQ(mm.GetState(), MatchmakingState::InLobby);
}

TEST(MatchmakingLobbyTest, LobbyConfig) {
    MatchmakingLobby mm;
    PlayerInfo info;
    info.playerId = 1;
    mm.SetLocalPlayer(info);

    LobbyConfig cfg;
    cfg.maxPlayers = 6;
    cfg.minPlayersToStart = 3;
    cfg.gameMode = "deathmatch";
    cfg.isPrivate = true;
    mm.CreateLobby(cfg);

    const LobbyInfo* lobby = mm.GetCurrentLobby();
    ASSERT_NE(lobby, nullptr);
    EXPECT_EQ(lobby->config.maxPlayers, 6);
    EXPECT_EQ(lobby->config.minPlayersToStart, 3);
    EXPECT_EQ(lobby->config.gameMode, "deathmatch");
    EXPECT_TRUE(lobby->config.isPrivate);
}

TEST(MatchmakingLobbyTest, JoinLobby) {
    MatchmakingLobby host;
    PlayerInfo hostInfo;
    hostInfo.playerId = 1;
    hostInfo.displayName = "Host";
    host.SetLocalPlayer(hostInfo);

    LobbyConfig cfg;
    cfg.maxPlayers = 4;
    uint64_t lobbyId = host.CreateLobby(cfg);

    // 別のプレイヤーが参加をシミュレート
    MatchmakingLobby client;
    PlayerInfo clientInfo;
    clientInfo.playerId = 2;
    clientInfo.displayName = "Client";
    client.SetLocalPlayer(clientInfo);

    // クライアント側でロビーIDを使って参加
    bool joined = client.JoinLobby(lobbyId);
    EXPECT_TRUE(joined);
    EXPECT_TRUE(client.IsInLobby());
    EXPECT_EQ(client.GetState(), MatchmakingState::InLobby);
}

TEST(MatchmakingLobbyTest, LeaveLobby) {
    MatchmakingLobby mm;
    PlayerInfo info;
    info.playerId = 1;
    mm.SetLocalPlayer(info);
    mm.CreateLobby({});
    EXPECT_TRUE(mm.IsInLobby());

    bool left = mm.LeaveLobby();
    EXPECT_TRUE(left);
    EXPECT_FALSE(mm.IsInLobby());
    EXPECT_EQ(mm.GetState(), MatchmakingState::Idle);
}

TEST(MatchmakingLobbyTest, IsInLobby) {
    MatchmakingLobby mm;
    EXPECT_FALSE(mm.IsInLobby());

    PlayerInfo info;
    info.playerId = 1;
    mm.SetLocalPlayer(info);
    mm.CreateLobby({});
    EXPECT_TRUE(mm.IsInLobby());
}

TEST(MatchmakingLobbyTest, IsHost) {
    MatchmakingLobby mm;
    PlayerInfo info;
    info.playerId = 1;
    mm.SetLocalPlayer(info);
    mm.CreateLobby({});
    EXPECT_TRUE(mm.IsHost());
}

TEST(MatchmakingLobbyTest, PlayerCount) {
    MatchmakingLobby mm;
    EXPECT_EQ(mm.GetPlayerCount(), 0);

    PlayerInfo info;
    info.playerId = 1;
    mm.SetLocalPlayer(info);
    mm.CreateLobby({});
    EXPECT_EQ(mm.GetPlayerCount(), 1);
}

TEST(MatchmakingLobbyTest, GetPlayer) {
    MatchmakingLobby mm;
    PlayerInfo info;
    info.playerId = 42;
    info.displayName = "Hero";
    mm.SetLocalPlayer(info);
    mm.CreateLobby({});

    const PlayerInfo* p = mm.GetPlayer(42);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->displayName, "Hero");
    EXPECT_TRUE(p->isHost);

    EXPECT_EQ(mm.GetPlayer(999), nullptr);
}

TEST(MatchmakingLobbyTest, SetReady) {
    MatchmakingLobby mm;
    PlayerInfo info;
    info.playerId = 1;
    mm.SetLocalPlayer(info);
    mm.CreateLobby({});

    mm.SetReady(true);
    const PlayerInfo* p = mm.GetPlayer(1);
    ASSERT_NE(p, nullptr);
    EXPECT_TRUE(p->isReady);

    mm.SetReady(false);
    p = mm.GetPlayer(1);
    ASSERT_NE(p, nullptr);
    EXPECT_FALSE(p->isReady);
}

TEST(MatchmakingLobbyTest, SetTeam) {
    MatchmakingLobby mm;
    PlayerInfo info;
    info.playerId = 1;
    mm.SetLocalPlayer(info);
    mm.CreateLobby({});

    mm.SetTeam(2);
    const PlayerInfo* p = mm.GetPlayer(1);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->teamId, 2);
}

TEST(MatchmakingLobbyTest, CanStartNotEnoughPlayers) {
    MatchmakingLobby mm;
    PlayerInfo info;
    info.playerId = 1;
    mm.SetLocalPlayer(info);

    LobbyConfig cfg;
    cfg.minPlayersToStart = 2;
    mm.CreateLobby(cfg);
    mm.SetReady(true);

    // 1人しかいないので開始不可
    EXPECT_FALSE(mm.CanStart());
}

TEST(MatchmakingLobbyTest, CanStartNotAllReady) {
    MatchmakingLobby mm;
    PlayerInfo info;
    info.playerId = 1;
    mm.SetLocalPlayer(info);

    LobbyConfig cfg;
    cfg.minPlayersToStart = 1;
    mm.CreateLobby(cfg);

    // レディでない
    EXPECT_FALSE(mm.CanStart());
}

TEST(MatchmakingLobbyTest, CanStartSuccess) {
    MatchmakingLobby mm;
    PlayerInfo info;
    info.playerId = 1;
    mm.SetLocalPlayer(info);

    LobbyConfig cfg;
    cfg.minPlayersToStart = 1;
    mm.CreateLobby(cfg);
    mm.SetReady(true);

    EXPECT_TRUE(mm.CanStart());
}

TEST(MatchmakingLobbyTest, StartMatch) {
    MatchmakingLobby mm;
    PlayerInfo info;
    info.playerId = 1;
    mm.SetLocalPlayer(info);

    LobbyConfig cfg;
    cfg.minPlayersToStart = 1;
    mm.CreateLobby(cfg);
    mm.SetReady(true);

    EXPECT_TRUE(mm.StartMatch());
    EXPECT_EQ(mm.GetState(), MatchmakingState::Starting);
}

TEST(MatchmakingLobbyTest, KickPlayer) {
    MatchmakingLobby mm;
    PlayerInfo hostInfo;
    hostInfo.playerId = 1;
    hostInfo.displayName = "Host";
    mm.SetLocalPlayer(hostInfo);

    LobbyConfig cfg;
    cfg.maxPlayers = 4;
    mm.CreateLobby(cfg);

    // 手動でプレイヤーを追加（テスト用）
    // ホストなので別プレイヤーをシミュレート追加するためにJoinを使用
    // 実際のテストでは、MatchmakingLobbyのデータを直接操作はできないので、
    // KickPlayerが存在しないプレイヤーに対してfalseを返すことを確認
    EXPECT_FALSE(mm.KickPlayer(999)); // 存在しないプレイヤー
    EXPECT_FALSE(mm.KickPlayer(1));   // 自分自身はキックできない
}

TEST(MatchmakingLobbyTest, StartSearch) {
    MatchmakingLobby mm;
    MatchFilter filter;
    filter.gameMode = "ranked";
    filter.minRating = 1000;
    filter.maxRating = 2000;
    mm.StartSearch(filter);
    EXPECT_EQ(mm.GetState(), MatchmakingState::Searching);
}

TEST(MatchmakingLobbyTest, CancelSearch) {
    MatchmakingLobby mm;
    MatchFilter filter;
    filter.gameMode = "quick";
    mm.StartSearch(filter);
    EXPECT_EQ(mm.GetState(), MatchmakingState::Searching);

    mm.CancelSearch();
    EXPECT_EQ(mm.GetState(), MatchmakingState::Idle);
}

TEST(MatchmakingLobbyTest, SearchTime) {
    MatchmakingLobby mm;
    EXPECT_FLOAT_EQ(mm.GetSearchTime(), 0.0f);

    MatchFilter filter;
    mm.StartSearch(filter);
    mm.Update(1.5f);
    EXPECT_FLOAT_EQ(mm.GetSearchTime(), 1.5f);

    mm.Update(2.0f);
    EXPECT_FLOAT_EQ(mm.GetSearchTime(), 3.5f);
}

TEST(MatchmakingLobbyTest, GenerateInviteCode) {
    MatchmakingLobby mm;
    gx::String code = mm.GenerateInviteCode();
    EXPECT_FALSE(code.empty());
}

TEST(MatchmakingLobbyTest, InviteCodeLength) {
    MatchmakingLobby mm;
    gx::String code = mm.GenerateInviteCode();
    EXPECT_EQ(code.size(), 6u);

    // 全文字が英数字であることを確認
    for (char c : code) {
        EXPECT_TRUE((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'));
    }
}

TEST(MatchmakingLobbyTest, ChatMessage) {
    MatchmakingLobby mm;
    PlayerInfo info;
    info.playerId = 1;
    info.displayName = "Alice";
    mm.SetLocalPlayer(info);
    mm.CreateLobby({});

    gx::String receivedPlayer, receivedMsg;
    mm.OnChatMessage = [&](const gx::String& player, const gx::String& msg) {
        receivedPlayer = player;
        receivedMsg = msg;
    };

    mm.SendChatMessage("Hello!");
    EXPECT_EQ(receivedPlayer, "Alice");
    EXPECT_EQ(receivedMsg, "Hello!");
}

TEST(MatchmakingLobbyTest, ChatHistory) {
    MatchmakingLobby mm;
    PlayerInfo info;
    info.playerId = 1;
    info.displayName = "Bob";
    mm.SetLocalPlayer(info);
    mm.CreateLobby({});

    mm.SendChatMessage("First");
    mm.SendChatMessage("Second");

    auto history = mm.GetChatHistory();
    ASSERT_EQ(history.size(), 2u);
    EXPECT_EQ(history[0].first, "Bob");
    EXPECT_EQ(history[0].second, "First");
    EXPECT_EQ(history[1].second, "Second");
}

TEST(MatchmakingLobbyTest, MatchFilter) {
    MatchmakingLobby mm;
    MatchFilter filter;
    filter.gameMode = "ctf";
    filter.minRating = 500;
    filter.maxRating = 1500;
    filter.region = "asia";
    filter.tags = {"competitive", "5v5"};
    mm.SetMatchFilter(filter);

    const auto& f = mm.GetMatchFilter();
    EXPECT_EQ(f.gameMode, "ctf");
    EXPECT_EQ(f.minRating, 500);
    EXPECT_EQ(f.maxRating, 1500);
    EXPECT_EQ(f.region, "asia");
    EXPECT_EQ(f.tags.size(), 2u);
}

TEST(MatchmakingLobbyTest, StateTransitions) {
    MatchmakingLobby mm;
    gx::Vector<MatchmakingState> states;
    mm.OnStateChanged = [&](MatchmakingState s) { states.push_back(s); };

    PlayerInfo info;
    info.playerId = 1;
    mm.SetLocalPlayer(info);

    // Idle -> InLobby
    LobbyConfig cfg;
    cfg.minPlayersToStart = 1;
    mm.CreateLobby(cfg);
    EXPECT_EQ(states.back(), MatchmakingState::InLobby);

    // InLobby -> Starting
    mm.SetReady(true);
    mm.StartMatch();
    EXPECT_EQ(states.back(), MatchmakingState::Starting);

    EXPECT_GE(states.size(), 2u);
}

TEST(MatchmakingLobbyTest, FindPlayers) {
    MatchmakingLobby mm;
    PlayerInfo info;
    info.playerId = 1;
    info.displayName = "Player1";
    mm.SetLocalPlayer(info);
    mm.CreateLobby({});

    auto players = mm.FindPlayers();
    ASSERT_EQ(players.size(), 1u);
    EXPECT_EQ(players[0].displayName, "Player1");
}

TEST(MatchmakingLobbyTest, LeaveNotInLobby) {
    MatchmakingLobby mm;
    EXPECT_FALSE(mm.LeaveLobby());
}

TEST(MatchmakingLobbyTest, GetCurrentLobbyNotInLobby) {
    MatchmakingLobby mm;
    EXPECT_EQ(mm.GetCurrentLobby(), nullptr);
}
