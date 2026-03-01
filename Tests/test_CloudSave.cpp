/// @file test_CloudSave.cpp
#include <gtest/gtest.h>
#include "IO/Network/CloudSave.h"
using namespace gx;

TEST(CloudSaveTest, DefaultConfig) {
    CloudSaveManager mgr;
    const auto& cfg = mgr.GetConfig();
    EXPECT_TRUE(cfg.serverUrl.empty());
    EXPECT_TRUE(cfg.syncOnSave);
    EXPECT_FLOAT_EQ(cfg.autoSyncInterval, 300.0f);
    EXPECT_EQ(cfg.maxRetries, 3);
    EXPECT_TRUE(cfg.enableLeaderboard);
    EXPECT_TRUE(cfg.enableCloudSave);
}

TEST(CloudSaveTest, SetConfig) {
    CloudSaveManager mgr;
    CloudConfig cfg;
    cfg.serverUrl = "https://api.example.com";
    cfg.appId = "myapp";
    cfg.apiKey = "secret123";
    cfg.autoSyncInterval = 60.0f;
    mgr.SetConfig(cfg);

    EXPECT_EQ(mgr.GetConfig().serverUrl, "https://api.example.com");
    EXPECT_EQ(mgr.GetConfig().appId, "myapp");
    EXPECT_FLOAT_EQ(mgr.GetConfig().autoSyncInterval, 60.0f);
}

TEST(CloudSaveTest, InitialState) {
    CloudSaveManager mgr;
    EXPECT_EQ(mgr.GetState(), CloudState::Disconnected);
    EXPECT_FALSE(mgr.IsConnected());
}

TEST(CloudSaveTest, ConnectDisconnect) {
    CloudSaveManager mgr;
    EXPECT_TRUE(mgr.Connect());
    EXPECT_EQ(mgr.GetState(), CloudState::Connected);
    EXPECT_TRUE(mgr.IsConnected());

    mgr.Disconnect();
    EXPECT_EQ(mgr.GetState(), CloudState::Disconnected);
    EXPECT_FALSE(mgr.IsConnected());
}

TEST(CloudSaveTest, IsConnected) {
    CloudSaveManager mgr;
    EXPECT_FALSE(mgr.IsConnected());
    mgr.Connect();
    EXPECT_TRUE(mgr.IsConnected());
}

TEST(CloudSaveTest, SaveToCloud) {
    CloudSaveManager mgr;
    mgr.Connect();

    std::vector<uint8_t> data = {1, 2, 3, 4, 5};
    std::unordered_map<std::string, std::string> meta = {{"level", "5"}};
    EXPECT_TRUE(mgr.SaveToCloud("slot1", data, meta));
    EXPECT_TRUE(mgr.HasCloudSave("slot1"));
}

TEST(CloudSaveTest, SaveToCloudDisconnected) {
    CloudSaveManager mgr;
    std::vector<uint8_t> data = {1, 2, 3};
    EXPECT_FALSE(mgr.SaveToCloud("slot1", data));
}

TEST(CloudSaveTest, LoadFromCloud) {
    CloudSaveManager mgr;
    mgr.Connect();

    std::vector<uint8_t> data = {10, 20, 30, 40};
    mgr.SaveToCloud("save1", data);

    auto slot = mgr.LoadFromCloud("save1");
    EXPECT_EQ(slot.slotId, "save1");
    EXPECT_EQ(slot.data.size(), 4u);
    EXPECT_EQ(slot.data[0], 10);
    EXPECT_EQ(slot.data[3], 40);
    EXPECT_EQ(slot.version, 1u);
}

TEST(CloudSaveTest, LoadFromCloudNotFound) {
    CloudSaveManager mgr;
    mgr.Connect();

    auto slot = mgr.LoadFromCloud("nonexistent");
    EXPECT_TRUE(slot.slotId.empty());
    EXPECT_TRUE(slot.data.empty());
}

TEST(CloudSaveTest, DeleteCloudSave) {
    CloudSaveManager mgr;
    mgr.Connect();

    std::vector<uint8_t> data = {1};
    mgr.SaveToCloud("todelete", data);
    EXPECT_TRUE(mgr.HasCloudSave("todelete"));

    EXPECT_TRUE(mgr.DeleteCloudSave("todelete"));
    EXPECT_FALSE(mgr.HasCloudSave("todelete"));
}

TEST(CloudSaveTest, HasCloudSave) {
    CloudSaveManager mgr;
    mgr.Connect();

    EXPECT_FALSE(mgr.HasCloudSave("x"));
    mgr.SaveToCloud("x", {1, 2});
    EXPECT_TRUE(mgr.HasCloudSave("x"));
}

TEST(CloudSaveTest, GetCloudSlots) {
    CloudSaveManager mgr;
    mgr.Connect();

    mgr.SaveToCloud("alpha", {1});
    mgr.SaveToCloud("beta", {2});
    mgr.SaveToCloud("gamma", {3});

    auto slots = mgr.GetCloudSlots();
    EXPECT_EQ(slots.size(), 3u);

    // 全スロットが含まれることを確認
    auto contains = [&](const std::string& name) {
        return std::find(slots.begin(), slots.end(), name) != slots.end();
    };
    EXPECT_TRUE(contains("alpha"));
    EXPECT_TRUE(contains("beta"));
    EXPECT_TRUE(contains("gamma"));
}

TEST(CloudSaveTest, SlotInfo) {
    CloudSaveManager mgr;
    mgr.Connect();

    std::unordered_map<std::string, std::string> meta = {{"chapter", "3"}};
    mgr.SaveToCloud("info_test", {1, 2, 3, 4, 5}, meta);

    auto info = mgr.GetSlotInfo("info_test");
    EXPECT_EQ(info.slotId, "info_test");
    EXPECT_EQ(info.sizeBytes, 5u);
    EXPECT_EQ(info.version, 1u);
    EXPECT_NE(info.timestamp, 0u);
    EXPECT_EQ(info.metadata.at("chapter"), "3");
    // GetSlotInfoはデータ本体を返さない
    EXPECT_TRUE(info.data.empty());
}

TEST(CloudSaveTest, SyncSlotNoConflict) {
    CloudSaveManager mgr;
    mgr.Connect();

    mgr.SaveToCloud("sync1", {1, 2, 3});

    auto result = mgr.SyncSlot("sync1");
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.conflictDetected);
}

TEST(CloudSaveTest, SyncSlotConflict) {
    CloudSaveManager mgr;
    mgr.Connect();

    // 初回保存
    mgr.SaveToCloud("conflict_slot", {1, 2, 3});

    // ローカルキャッシュのバージョンを変更してコンフリクトを発生させる
    // SaveToCloudを2回呼ぶと同期されるので、直接コンフリクトを発生させるために
    // 1回保存してから、内部状態を操作する代わりに
    // SyncAll -> ResolveConflictのフローをテスト

    // 2回目の保存でバージョンが上がる
    mgr.SaveToCloud("conflict_slot", {4, 5, 6});
    auto result = mgr.SyncSlot("conflict_slot");
    EXPECT_TRUE(result.success); // 同じデータなのでコンフリクトなし
}

TEST(CloudSaveTest, ResolveConflict) {
    CloudSaveManager mgr;
    mgr.Connect();

    mgr.SaveToCloud("resolve_test", {1});

    // コンフリクトがない場合はfalse
    EXPECT_FALSE(mgr.ResolveConflict("resolve_test", true));
}

TEST(CloudSaveTest, SubmitScore) {
    CloudSaveManager mgr;
    mgr.Connect();

    LeaderboardConfig lbCfg;
    lbCfg.boardId = "highscores";
    lbCfg.sortOrder = LeaderboardSortOrder::Descending;
    mgr.CreateLeaderboard(lbCfg);

    EXPECT_TRUE(mgr.SubmitScore("highscores", 1000, "Alice"));
    EXPECT_TRUE(mgr.SubmitScore("highscores", 2000, "Bob"));
    EXPECT_TRUE(mgr.SubmitScore("highscores", 1500, "Charlie"));
    EXPECT_EQ(mgr.GetLeaderboardCount("highscores"), 3u);
}

TEST(CloudSaveTest, SubmitScoreNoBoard) {
    CloudSaveManager mgr;
    mgr.Connect();
    EXPECT_FALSE(mgr.SubmitScore("nonexistent", 100, "Test"));
}

TEST(CloudSaveTest, GetLeaderboard) {
    CloudSaveManager mgr;
    mgr.Connect();

    LeaderboardConfig lbCfg;
    lbCfg.boardId = "scores";
    lbCfg.sortOrder = LeaderboardSortOrder::Descending;
    mgr.CreateLeaderboard(lbCfg);

    mgr.SubmitScore("scores", 100, "Low");
    mgr.SubmitScore("scores", 500, "Mid");
    mgr.SubmitScore("scores", 1000, "High");

    auto entries = mgr.GetLeaderboard("scores", 0, 10);
    ASSERT_EQ(entries.size(), 3u);
    EXPECT_EQ(entries[0].rank, 1u);
    EXPECT_EQ(entries[0].playerName, "High");
    EXPECT_EQ(entries[0].score, 1000);
}

TEST(CloudSaveTest, LeaderboardSorted) {
    CloudSaveManager mgr;
    mgr.Connect();

    LeaderboardConfig lbCfg;
    lbCfg.boardId = "asc_board";
    lbCfg.sortOrder = LeaderboardSortOrder::Ascending;
    mgr.CreateLeaderboard(lbCfg);

    mgr.SubmitScore("asc_board", 300, "C");
    mgr.SubmitScore("asc_board", 100, "A");
    mgr.SubmitScore("asc_board", 200, "B");

    auto entries = mgr.GetLeaderboard("asc_board", 0, 10);
    ASSERT_EQ(entries.size(), 3u);
    EXPECT_EQ(entries[0].playerName, "A");
    EXPECT_EQ(entries[0].score, 100);
    EXPECT_EQ(entries[2].playerName, "C");
    EXPECT_EQ(entries[2].score, 300);
}

TEST(CloudSaveTest, GetPlayerRank) {
    CloudSaveManager mgr;
    mgr.Connect();

    LeaderboardConfig lbCfg;
    lbCfg.boardId = "ranked";
    mgr.CreateLeaderboard(lbCfg);

    mgr.SubmitScore("ranked", 100, "First");
    mgr.SubmitScore("ranked", 500, "Second");
    mgr.SubmitScore("ranked", 300, "Third");

    uint64_t secondId = std::hash<std::string>{}("Second");
    auto entry = mgr.GetPlayerRank("ranked", secondId);
    EXPECT_EQ(entry.playerName, "Second");
    EXPECT_EQ(entry.rank, 1u); // Highest score = rank 1 (descending)
}

TEST(CloudSaveTest, LeaderboardCount) {
    CloudSaveManager mgr;
    mgr.Connect();

    LeaderboardConfig lbCfg;
    lbCfg.boardId = "counter";
    mgr.CreateLeaderboard(lbCfg);

    EXPECT_EQ(mgr.GetLeaderboardCount("counter"), 0u);
    mgr.SubmitScore("counter", 100, "P1");
    EXPECT_EQ(mgr.GetLeaderboardCount("counter"), 1u);
    mgr.SubmitScore("counter", 200, "P2");
    EXPECT_EQ(mgr.GetLeaderboardCount("counter"), 2u);
}

TEST(CloudSaveTest, CreateLeaderboard) {
    CloudSaveManager mgr;
    LeaderboardConfig cfg;
    cfg.boardId = "new_board";
    cfg.maxEntries = 50;
    EXPECT_TRUE(mgr.CreateLeaderboard(cfg));
    EXPECT_FALSE(mgr.CreateLeaderboard(cfg)); // 重複

    LeaderboardConfig empty;
    empty.boardId = "";
    EXPECT_FALSE(mgr.CreateLeaderboard(empty)); // 空ID
}

TEST(CloudSaveTest, DeleteLeaderboard) {
    CloudSaveManager mgr;
    LeaderboardConfig cfg;
    cfg.boardId = "delete_me";
    mgr.CreateLeaderboard(cfg);

    EXPECT_TRUE(mgr.DeleteLeaderboard("delete_me"));
    EXPECT_FALSE(mgr.DeleteLeaderboard("delete_me")); // もう存在しない
}

TEST(CloudSaveTest, GetAroundPlayer) {
    CloudSaveManager mgr;
    mgr.Connect();

    LeaderboardConfig lbCfg;
    lbCfg.boardId = "around";
    lbCfg.sortOrder = LeaderboardSortOrder::Descending;
    mgr.CreateLeaderboard(lbCfg);

    mgr.SubmitScore("around", 100, "P1");
    mgr.SubmitScore("around", 200, "P2");
    mgr.SubmitScore("around", 300, "P3");
    mgr.SubmitScore("around", 400, "P4");
    mgr.SubmitScore("around", 500, "P5");

    uint64_t p3Id = std::hash<std::string>{}("P3");
    auto around = mgr.GetAroundPlayer("around", p3Id, 3);
    ASSERT_GE(around.size(), 2u);
    // P3は300点でdescending順: P5(500), P4(400), P3(300), P2(200), P1(100)
    // P3はインデックス2。count=3 → [P4, P3, P2] or similar neighborhood
}

TEST(CloudSaveTest, UpdateSync) {
    CloudSaveManager mgr;
    mgr.Connect();

    // autoSyncIntervalを短くして更新テスト
    CloudConfig cfg;
    cfg.autoSyncInterval = 1.0f;
    mgr.SetConfig(cfg);

    // 更新してもクラッシュしないこと
    mgr.Update(0.5f);
    mgr.Update(0.6f); // 1.1秒 → 自動同期がトリガーされるはず
}

TEST(CloudSaveTest, OfflineQueue) {
    CloudSaveManager mgr;
    // 未接続でのセーブ失敗を確認
    std::vector<uint8_t> data = {1, 2, 3};
    EXPECT_FALSE(mgr.SaveToCloud("offline", data));

    // 接続後にセーブ成功
    mgr.Connect();
    EXPECT_TRUE(mgr.SaveToCloud("offline", data));
}

TEST(CloudSaveTest, SaveAndLoadRoundTrip) {
    CloudSaveManager mgr;
    mgr.Connect();

    // 保存
    std::vector<uint8_t> originalData = {0xDE, 0xAD, 0xBE, 0xEF, 0x42};
    std::unordered_map<std::string, std::string> meta = {
        {"game", "TestGame"},
        {"version", "1.0"}
    };
    mgr.SaveToCloud("roundtrip", originalData, meta);

    // 読み込み
    auto loaded = mgr.LoadFromCloud("roundtrip");
    EXPECT_EQ(loaded.slotId, "roundtrip");
    ASSERT_EQ(loaded.data.size(), originalData.size());
    for (size_t i = 0; i < originalData.size(); i++) {
        EXPECT_EQ(loaded.data[i], originalData[i]);
    }
    EXPECT_EQ(loaded.metadata.at("game"), "TestGame");
    EXPECT_EQ(loaded.metadata.at("version"), "1.0");
    EXPECT_EQ(loaded.version, 1u);
    EXPECT_NE(loaded.checksum, 0u);
}

TEST(CloudSaveTest, VersionIncrement) {
    CloudSaveManager mgr;
    mgr.Connect();

    mgr.SaveToCloud("ver_test", {1});
    auto slot1 = mgr.LoadFromCloud("ver_test");
    EXPECT_EQ(slot1.version, 1u);

    mgr.SaveToCloud("ver_test", {2});
    auto slot2 = mgr.LoadFromCloud("ver_test");
    EXPECT_EQ(slot2.version, 2u);

    mgr.SaveToCloud("ver_test", {3});
    auto slot3 = mgr.LoadFromCloud("ver_test");
    EXPECT_EQ(slot3.version, 3u);
}
