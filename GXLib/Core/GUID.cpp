/// @file GUID.cpp
/// @brief GUID 実装 — UUID v4 生成と文字列変換
#include "pch_common.h"
#include "Core/GUID.h"
#include <objbase.h>
#pragma comment(lib, "ole32.lib")

namespace gx
{

// ============================================================================
// ToString — "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx"
// ============================================================================
gx::String GUID::ToString() const
{
    // hi = [time_low(32) | time_mid(16) | time_hi_ver(16)]
    // lo = [clk_seq(16) | node(48)]
    uint32_t a = static_cast<uint32_t>(hi >> 32);
    uint16_t b = static_cast<uint16_t>(hi >> 16);
    uint16_t c = static_cast<uint16_t>(hi);
    uint16_t d = static_cast<uint16_t>(lo >> 48);
    uint64_t e = lo & 0x0000FFFFFFFFFFFF;

    char buf[64];
    snprintf(buf, sizeof(buf),
             "%08x-%04x-%04x-%04x-%012llx",
             a, b, c, d,
             static_cast<unsigned long long>(e));
    return gx::String(buf);
}

// ============================================================================
// FromString
// ============================================================================
static bool HexToByte(char ch, uint8_t& out)
{
    if (ch >= '0' && ch <= '9') { out = static_cast<uint8_t>(ch - '0'); return true; }
    if (ch >= 'a' && ch <= 'f') { out = static_cast<uint8_t>(ch - 'a' + 10); return true; }
    if (ch >= 'A' && ch <= 'F') { out = static_cast<uint8_t>(ch - 'A' + 10); return true; }
    return false;
}

GUID GUID::FromString(const gx::String& str)
{
    // "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx" → 32 hex digits + 4 dashes = 36 chars
    if (str.size() != 36)
        return GUID{};

    // 除去ダッシュ位置: 8, 13, 18, 23
    gx::String hex;
    hex.reserve(32);
    for (size_t i = 0; i < str.size(); ++i)
    {
        if (i == 8 || i == 13 || i == 18 || i == 23)
        {
            if (str[i] != '-')
                return GUID{};
            continue;
        }
        hex.push_back(str[i]);
    }

    if (hex.size() != 32)
        return GUID{};

    // 32 hex chars → 16 bytes → hi(8) + lo(8)
    uint64_t hi_val = 0, lo_val = 0;
    for (int i = 0; i < 16; ++i)
    {
        uint8_t h, l;
        if (!HexToByte(hex[i * 2], h) || !HexToByte(hex[i * 2 + 1], l))
            return GUID{};
        uint8_t byte = static_cast<uint8_t>((h << 4) | l);
        if (i < 8)
            hi_val = (hi_val << 8) | byte;
        else
            lo_val = (lo_val << 8) | byte;
    }

    return GUID{ hi_val, lo_val };
}

// ============================================================================
// Generate — CoCreateGuid (UUID v4)
// ============================================================================
GUID GUID::Generate()
{
    ::GUID winGuid;
    HRESULT hr = ::CoCreateGuid(&winGuid);
    if (FAILED(hr))
        return GUID{};

    // Windows GUID layout: Data1(4) Data2(2) Data3(2) Data4(8)
    // Pack into hi/lo
    uint64_t hi_val = 0;
    hi_val |= static_cast<uint64_t>(winGuid.Data1) << 32;
    hi_val |= static_cast<uint64_t>(winGuid.Data2) << 16;
    hi_val |= static_cast<uint64_t>(winGuid.Data3);

    uint64_t lo_val = 0;
    for (int i = 0; i < 8; ++i)
        lo_val = (lo_val << 8) | winGuid.Data4[i];

    return GUID{ hi_val, lo_val };
}

} // namespace gx
