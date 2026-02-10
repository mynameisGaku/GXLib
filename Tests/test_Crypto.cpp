/// @file test_Crypto.cpp
/// @brief AES-256-CBC / SHA-256 / GenerateRandomBytes 単体テスト

#include "pch.h"
#include <gtest/gtest.h>
#include "IO/Crypto.h"

using namespace GX;

TEST(CryptoTest, AES256_EncryptDecrypt)
{
    // テスト用データ
    const std::string plaintext = "Hello, GXLib Crypto! This is a test message.";
    const auto* data = reinterpret_cast<const uint8_t*>(plaintext.data());
    size_t size = plaintext.size();

    // 鍵とIVを生成（IVは初期化ベクトル。暗号化の揺らぎに使う）
    uint8_t key[32], iv[16];
    Crypto::GenerateRandomBytes(key, 32);
    Crypto::GenerateRandomBytes(iv, 16);

    // 暗号化
    auto encrypted = Crypto::Encrypt(data, size, key, iv);
    EXPECT_FALSE(encrypted.empty());
    EXPECT_NE(encrypted.size(), size); // Encrypted data differs in size (padding)

    // 復号
    auto decrypted = Crypto::Decrypt(encrypted.data(), encrypted.size(), key, iv);
    EXPECT_EQ(decrypted.size(), size);

    // 復号結果が元と一致するか確認
    std::string result(decrypted.begin(), decrypted.end());
    EXPECT_EQ(result, plaintext);
}

TEST(CryptoTest, AES256_WrongKey)
{
    const std::string plaintext = "Secret data";
    const auto* data = reinterpret_cast<const uint8_t*>(plaintext.data());

    uint8_t key[32], wrongKey[32], iv[16];
    Crypto::GenerateRandomBytes(key, 32);
    Crypto::GenerateRandomBytes(wrongKey, 32);
    Crypto::GenerateRandomBytes(iv, 16);

    auto encrypted = Crypto::Encrypt(data, plaintext.size(), key, iv);

    // 間違った鍵なら内容が一致しない（または失敗）するはず
    auto decrypted = Crypto::Decrypt(encrypted.data(), encrypted.size(), wrongKey, iv);
    if (!decrypted.empty())
    {
        std::string result(decrypted.begin(), decrypted.end());
        EXPECT_NE(result, plaintext);
    }
}

TEST(CryptoTest, SHA256_KnownHash)
{
    // 空文字列のSHA-256
    const char* empty = "";
    auto hash = Crypto::SHA256(empty, 0);

    // 期待値: e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
    EXPECT_EQ(hash[0], 0xe3);
    EXPECT_EQ(hash[1], 0xb0);
    EXPECT_EQ(hash[2], 0xc4);
    EXPECT_EQ(hash[3], 0x42);
    EXPECT_EQ(hash[31], 0x55);
}

TEST(CryptoTest, SHA256_Deterministic)
{
    const std::string data = "test data for hashing";
    auto hash1 = Crypto::SHA256(data.data(), data.size());
    auto hash2 = Crypto::SHA256(data.data(), data.size());

    EXPECT_EQ(hash1, hash2);
}

TEST(CryptoTest, GenerateRandomBytes_Length)
{
    uint8_t buffer[64] = {};
    Crypto::GenerateRandomBytes(buffer, 64);

    // 全て0になるのは極めて低確率（2^-512）
    bool allZero = true;
    for (auto byte : buffer)
    {
        if (byte != 0) { allZero = false; break; }
    }
    EXPECT_FALSE(allZero);
}

TEST(CryptoTest, GenerateRandomBytes_Unique)
{
    uint8_t buf1[32], buf2[32];
    Crypto::GenerateRandomBytes(buf1, 32);
    Crypto::GenerateRandomBytes(buf2, 32);

    // 256bitの乱数2つが同じになる確率は極めて低い
    bool same = (memcmp(buf1, buf2, 32) == 0);
    EXPECT_FALSE(same);
}
