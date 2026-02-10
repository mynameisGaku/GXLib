#pragma once
/// @file Crypto.h
/// @brief 暗号化ユーティリティ — AES-256-CBC暗号化/復号 + SHA-256 ハッシュ
///
/// Windows BCrypt API を使用した暗号化機能を提供する。
/// Archive の暗号化アーカイブで内部的に使用される。

namespace GX {

/// @brief 暗号化ユーティリティクラス (全staticメソッド)
class Crypto
{
public:
    /// @brief AES-256-CBCで暗号化する
    /// @param data 暗号化するデータ
    /// @param size データサイズ (バイト)
    /// @param key 256ビット暗号鍵 (32バイト)
    /// @param iv 初期化ベクトル (16バイト)
    /// @return 暗号化されたデータ (失敗時は空)
    static std::vector<uint8_t> Encrypt(const void* data, size_t size,
                                         const uint8_t key[32], const uint8_t iv[16]);

    /// @brief AES-256-CBCで復号する
    /// @param data 復号するデータ
    /// @param size データサイズ (バイト)
    /// @param key 256ビット暗号鍵 (32バイト)
    /// @param iv 初期化ベクトル (16バイト)
    /// @return 復号されたデータ (失敗時は空)
    static std::vector<uint8_t> Decrypt(const void* data, size_t size,
                                         const uint8_t key[32], const uint8_t iv[16]);

    /// @brief SHA-256ハッシュを計算する
    /// @param data ハッシュ対象データ
    /// @param size データサイズ (バイト)
    /// @return 32バイトのSHA-256ハッシュ値
    static std::array<uint8_t, 32> SHA256(const void* data, size_t size);

    /// @brief 暗号学的に安全な乱数バイト列を生成する
    /// @param buffer 出力バッファ
    /// @param size 生成するバイト数
    static void GenerateRandomBytes(uint8_t* buffer, size_t size);
};

} // namespace GX
