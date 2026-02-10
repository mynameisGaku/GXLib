#include "pch.h"
#include "IO/Crypto.h"
#include "Core/Logger.h"
#include <bcrypt.h>

#pragma comment(lib, "bcrypt.lib")

namespace GX {

std::vector<uint8_t> Crypto::Encrypt(const void* data, size_t size,
                                      const uint8_t key[32], const uint8_t iv[16])
{
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    BCRYPT_KEY_HANDLE hKey = nullptr;
    std::vector<uint8_t> result;

    NTSTATUS status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, nullptr, 0);
    if (!BCRYPT_SUCCESS(status))
    {
        GX_LOG_ERROR("Crypto::Encrypt: BCryptOpenAlgorithmProvider failed: 0x%08X", status);
        return result;
    }

    // CBCモードを設定する (ブロック暗号の連鎖方式)
    status = BCryptSetProperty(hAlg, BCRYPT_CHAINING_MODE,
        (PUCHAR)BCRYPT_CHAIN_MODE_CBC, sizeof(BCRYPT_CHAIN_MODE_CBC), 0);
    if (!BCRYPT_SUCCESS(status))
    {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return result;
    }

    // 鍵ハンドルを生成する
    status = BCryptGenerateSymmetricKey(hAlg, &hKey, nullptr, 0,
        (PUCHAR)key, 32, 0);
    if (!BCRYPT_SUCCESS(status))
    {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return result;
    }

    // IVをコピーする (BCryptEncryptが内容を更新するため)
    uint8_t ivCopy[16];
    memcpy(ivCopy, iv, 16);

    // 出力サイズを取得する
    ULONG cbOutput = 0;
    status = BCryptEncrypt(hKey, (PUCHAR)data, (ULONG)size, nullptr,
        ivCopy, 16, nullptr, 0, &cbOutput, BCRYPT_BLOCK_PADDING);
    if (!BCRYPT_SUCCESS(status))
    {
        BCryptDestroyKey(hKey);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return result;
    }

    result.resize(cbOutput);
    memcpy(ivCopy, iv, 16);

    status = BCryptEncrypt(hKey, (PUCHAR)data, (ULONG)size, nullptr,
        ivCopy, 16, result.data(), cbOutput, &cbOutput, BCRYPT_BLOCK_PADDING);
    if (!BCRYPT_SUCCESS(status))
    {
        result.clear();
    }
    else
    {
        result.resize(cbOutput);
    }

    BCryptDestroyKey(hKey);
    BCryptCloseAlgorithmProvider(hAlg, 0);
    return result;
}

std::vector<uint8_t> Crypto::Decrypt(const void* data, size_t size,
                                      const uint8_t key[32], const uint8_t iv[16])
{
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    BCRYPT_KEY_HANDLE hKey = nullptr;
    std::vector<uint8_t> result;

    NTSTATUS status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, nullptr, 0);
    if (!BCRYPT_SUCCESS(status))
        return result;

    status = BCryptSetProperty(hAlg, BCRYPT_CHAINING_MODE,
        (PUCHAR)BCRYPT_CHAIN_MODE_CBC, sizeof(BCRYPT_CHAIN_MODE_CBC), 0);
    if (!BCRYPT_SUCCESS(status))
    {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return result;
    }

    status = BCryptGenerateSymmetricKey(hAlg, &hKey, nullptr, 0,
        (PUCHAR)key, 32, 0);
    if (!BCRYPT_SUCCESS(status))
    {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return result;
    }

    uint8_t ivCopy[16];
    memcpy(ivCopy, iv, 16);

    ULONG cbOutput = 0;
    status = BCryptDecrypt(hKey, (PUCHAR)data, (ULONG)size, nullptr,
        ivCopy, 16, nullptr, 0, &cbOutput, BCRYPT_BLOCK_PADDING);
    if (!BCRYPT_SUCCESS(status))
    {
        BCryptDestroyKey(hKey);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return result;
    }

    result.resize(cbOutput);
    memcpy(ivCopy, iv, 16);

    status = BCryptDecrypt(hKey, (PUCHAR)data, (ULONG)size, nullptr,
        ivCopy, 16, result.data(), cbOutput, &cbOutput, BCRYPT_BLOCK_PADDING);
    if (!BCRYPT_SUCCESS(status))
    {
        result.clear();
    }
    else
    {
        result.resize(cbOutput);
    }

    BCryptDestroyKey(hKey);
    BCryptCloseAlgorithmProvider(hAlg, 0);
    return result;
}

std::array<uint8_t, 32> Crypto::SHA256(const void* data, size_t size)
{
    std::array<uint8_t, 32> hash = {};
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    BCRYPT_HASH_HANDLE hHash = nullptr;

    NTSTATUS status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
    if (!BCRYPT_SUCCESS(status))
        return hash;

    status = BCryptCreateHash(hAlg, &hHash, nullptr, 0, nullptr, 0, 0);
    if (!BCRYPT_SUCCESS(status))
    {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return hash;
    }

    status = BCryptHashData(hHash, (PUCHAR)data, (ULONG)size, 0);
    if (BCRYPT_SUCCESS(status))
    {
        BCryptFinishHash(hHash, hash.data(), 32, 0);
    }

    BCryptDestroyHash(hHash);
    BCryptCloseAlgorithmProvider(hAlg, 0);
    return hash;
}

void Crypto::GenerateRandomBytes(uint8_t* buffer, size_t size)
{
    BCryptGenRandom(nullptr, buffer, (ULONG)size, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
}

} // namespace GX
