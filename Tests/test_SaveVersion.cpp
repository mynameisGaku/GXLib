/// @file test_SaveVersion.cpp
/// @brief SaveVersionManager のテスト — ヘッダー、マイグレーション、チェックサム

#include <gtest/gtest.h>
#include "Core/SaveVersion.h"

using namespace gx;

// ============================================================================
// 定数テスト
// ============================================================================

TEST(SaveVersionTest, MagicConstant)
{
    EXPECT_EQ(SaveVersionManager::k_Magic, 0x47585356u);
}

TEST(SaveVersionTest, DefaultVersion)
{
    SaveVersionManager mgr;
    EXPECT_EQ(mgr.GetCurrentVersion(), SaveVersionManager::k_DefaultVersion);
    EXPECT_EQ(mgr.GetCurrentVersion(), 1u);
}

// ============================================================================
// ヘッダー読み書きテスト
// ============================================================================

TEST(SaveVersionTest, WriteReadHeader)
{
    SaveVersionManager mgr;

    std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04};
    mgr.WriteHeader(data);

    SaveFormatHeader header;
    EXPECT_TRUE(mgr.ReadHeader(data, header));
    EXPECT_EQ(header.magic, SaveVersionManager::k_Magic);
    EXPECT_EQ(header.version, 1u);
    EXPECT_EQ(header.dataSize, 4u);
}

TEST(SaveVersionTest, HeaderRoundTrip)
{
    SaveVersionManager mgr;

    std::vector<uint8_t> originalPayload = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
    std::vector<uint8_t> data = originalPayload;

    mgr.WriteHeader(data, 1, 0x42);

    SaveFormatHeader header;
    ASSERT_TRUE(mgr.ReadHeader(data, header));
    EXPECT_EQ(header.magic, SaveVersionManager::k_Magic);
    EXPECT_EQ(header.version, 1u);
    EXPECT_EQ(header.flags, 0x42u);
    EXPECT_EQ(header.dataSize, 5u);
    EXPECT_GT(header.timestamp, 0u);

    // ペイロードが保持されていることを確認
    std::vector<uint8_t> extractedPayload(data.begin() + SaveVersionManager::k_HeaderSize, data.end());
    EXPECT_EQ(extractedPayload, originalPayload);
}

TEST(SaveVersionTest, InvalidMagic)
{
    SaveVersionManager mgr;

    // 不正なマジックナンバーのヘッダーを作成
    SaveFormatHeader header;
    header.magic = 0xDEADBEEF;
    header.version = 1;

    EXPECT_FALSE(mgr.ValidateHeader(header));
}

// ============================================================================
// ヘッダー検証テスト
// ============================================================================

TEST(SaveVersionTest, ValidateGoodHeader)
{
    SaveVersionManager mgr;

    SaveFormatHeader header;
    header.magic   = SaveVersionManager::k_Magic;
    header.version = 1;

    EXPECT_TRUE(mgr.ValidateHeader(header));
}

TEST(SaveVersionTest, ValidateBadVersion)
{
    SaveVersionManager mgr;
    mgr.SetCurrentVersion(3);

    SaveFormatHeader header;
    header.magic   = SaveVersionManager::k_Magic;
    header.version = 0; // 無効

    EXPECT_FALSE(mgr.ValidateHeader(header));

    header.version = 99; // 現在バージョンを超過
    EXPECT_FALSE(mgr.ValidateHeader(header));
}

// ============================================================================
// マイグレーション判定テスト
// ============================================================================

TEST(SaveVersionTest, NeedsMigrationTrue)
{
    SaveVersionManager mgr;
    mgr.SetCurrentVersion(3);

    SaveFormatHeader header;
    header.magic   = SaveVersionManager::k_Magic;
    header.version = 1;

    EXPECT_TRUE(mgr.NeedsMigration(header));
}

TEST(SaveVersionTest, NeedsMigrationFalse)
{
    SaveVersionManager mgr;
    mgr.SetCurrentVersion(1);

    SaveFormatHeader header;
    header.magic   = SaveVersionManager::k_Magic;
    header.version = 1;

    EXPECT_FALSE(mgr.NeedsMigration(header));
}

// ============================================================================
// マイグレーション登録・実行テスト
// ============================================================================

TEST(SaveVersionTest, RegisterMigration)
{
    SaveVersionManager mgr;

    mgr.RegisterMigration(1, 2, "Add score field", [](std::vector<uint8_t>& data) {
        data.push_back(0x00); // ダミーフィールド追加
        return true;
    });

    EXPECT_EQ(mgr.GetMigrationCount(), 1u);
}

TEST(SaveVersionTest, MigrationCount)
{
    SaveVersionManager mgr;

    EXPECT_EQ(mgr.GetMigrationCount(), 0u);

    mgr.RegisterMigration(1, 2, "v1->v2", [](std::vector<uint8_t>&) { return true; });
    mgr.RegisterMigration(2, 3, "v2->v3", [](std::vector<uint8_t>&) { return true; });
    mgr.RegisterMigration(3, 4, "v3->v4", [](std::vector<uint8_t>&) { return true; });

    EXPECT_EQ(mgr.GetMigrationCount(), 3u);
}

TEST(SaveVersionTest, MigrateSimple)
{
    SaveVersionManager mgr;
    mgr.SetCurrentVersion(2);

    bool migrationCalled = false;
    mgr.RegisterMigration(1, 2, "Add field", [&](std::vector<uint8_t>& data) {
        migrationCalled = true;
        data.push_back(0xFF);
        return true;
    });

    // v1のデータを作成
    std::vector<uint8_t> payload = {0x01, 0x02, 0x03};
    std::vector<uint8_t> data = payload;

    // v1ヘッダーを書き込み（一時的にcurrentVersionを1に設定）
    mgr.SetCurrentVersion(1);
    mgr.WriteHeader(data, 1);
    mgr.SetCurrentVersion(2);

    // マイグレーション実行
    EXPECT_TRUE(mgr.Migrate(data));
    EXPECT_TRUE(migrationCalled);

    // マイグレーション後のヘッダー検証
    SaveFormatHeader header;
    ASSERT_TRUE(mgr.ReadHeader(data, header));
    EXPECT_EQ(header.version, 2u);
}

TEST(SaveVersionTest, MigrateChain)
{
    SaveVersionManager mgr;
    mgr.SetCurrentVersion(4);

    int callOrder = 0;
    int call1 = 0, call2 = 0, call3 = 0;

    mgr.RegisterMigration(1, 2, "v1->v2", [&](std::vector<uint8_t>&) {
        call1 = ++callOrder;
        return true;
    });
    mgr.RegisterMigration(2, 3, "v2->v3", [&](std::vector<uint8_t>&) {
        call2 = ++callOrder;
        return true;
    });
    mgr.RegisterMigration(3, 4, "v3->v4", [&](std::vector<uint8_t>&) {
        call3 = ++callOrder;
        return true;
    });

    // v1データを作成
    std::vector<uint8_t> payload = {0xAA};
    std::vector<uint8_t> data = payload;
    mgr.SetCurrentVersion(1);
    mgr.WriteHeader(data, 1);
    mgr.SetCurrentVersion(4);

    EXPECT_TRUE(mgr.Migrate(data));

    // 正しい順序で呼ばれたか
    EXPECT_EQ(call1, 1);
    EXPECT_EQ(call2, 2);
    EXPECT_EQ(call3, 3);

    // ヘッダーバージョン確認
    SaveFormatHeader header;
    ASSERT_TRUE(mgr.ReadHeader(data, header));
    EXPECT_EQ(header.version, 4u);
}

TEST(SaveVersionTest, CanMigrate)
{
    SaveVersionManager mgr;
    mgr.SetCurrentVersion(3);

    mgr.RegisterMigration(1, 2, "v1->v2", [](std::vector<uint8_t>&) { return true; });
    mgr.RegisterMigration(2, 3, "v2->v3", [](std::vector<uint8_t>&) { return true; });

    EXPECT_TRUE(mgr.CanMigrate(1));
    EXPECT_TRUE(mgr.CanMigrate(2));
    EXPECT_TRUE(mgr.CanMigrate(3)); // 既に最新

    // パスが存在しない場合
    SaveVersionManager mgr2;
    mgr2.SetCurrentVersion(3);
    mgr2.RegisterMigration(1, 2, "v1->v2", [](std::vector<uint8_t>&) { return true; });
    // v2->v3が未登録
    EXPECT_FALSE(mgr2.CanMigrate(1));
}

// ============================================================================
// チェックサムテスト
// ============================================================================

TEST(SaveVersionTest, ChecksumCompute)
{
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    uint32_t crc = SaveVersionManager::ComputeChecksum(data, 4);

    // 同じデータには同じチェックサムが返る
    uint32_t crc2 = SaveVersionManager::ComputeChecksum(data, 4);
    EXPECT_EQ(crc, crc2);

    // 異なるデータには異なるチェックサムが返る
    uint8_t data2[] = {0x04, 0x03, 0x02, 0x01};
    uint32_t crc3 = SaveVersionManager::ComputeChecksum(data2, 4);
    EXPECT_NE(crc, crc3);
}

TEST(SaveVersionTest, ChecksumVerify)
{
    SaveVersionManager mgr;

    std::vector<uint8_t> payload = {0x10, 0x20, 0x30};
    uint32_t checksum = SaveVersionManager::ComputeChecksum(payload.data(), payload.size());

    SaveFormatHeader header;
    header.checksum = checksum;

    EXPECT_TRUE(mgr.VerifyChecksum(header, payload));
}

TEST(SaveVersionTest, ChecksumMismatch)
{
    SaveVersionManager mgr;

    std::vector<uint8_t> payload = {0x10, 0x20, 0x30};

    SaveFormatHeader header;
    header.checksum = 0xDEADBEEF; // 不正なチェックサム

    EXPECT_FALSE(mgr.VerifyChecksum(header, payload));
}

// ============================================================================
// クリアテスト
// ============================================================================

TEST(SaveVersionTest, ClearMigrations)
{
    SaveVersionManager mgr;
    mgr.RegisterMigration(1, 2, "test", [](std::vector<uint8_t>&) { return true; });
    EXPECT_EQ(mgr.GetMigrationCount(), 1u);

    mgr.Clear();
    EXPECT_EQ(mgr.GetMigrationCount(), 0u);
}
