/// @file AssetMeta.cpp
/// @brief .meta サイドカーファイル 読み書き実装
#include "pch_common.h"
#include "Core/AssetMeta.h"
#include "Core/Logger.h"
#include <filesystem>
#include <fstream>

namespace gx
{

// ============================================================================
// GetMetaPath
// ============================================================================
gx::String AssetMeta::GetMetaPath(const gx::String& assetPath)
{
    return assetPath + ".meta";
}

// ============================================================================
// HasMeta
// ============================================================================
bool AssetMeta::HasMeta(const gx::String& assetPath)
{
    gx::String metaPath = GetMetaPath(assetPath);
    std::error_code ec;
    return std::filesystem::exists(std::filesystem::path(metaPath.c_str()), ec);
}

// ============================================================================
// Load — テキスト形式の .meta ファイルを解析する
// フォーマット:
//   guid: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
//   import:key=value
// ============================================================================
bool AssetMeta::Load(const gx::String& metaPath, AssetMetaData& outData)
{
    std::ifstream file(metaPath.c_str());
    if (!file.is_open())
        return false;

    outData = AssetMetaData{};

    gx::String line;
    while (gx::container::getline(file, line))
    {
        if (line.empty())
            continue;

        // guid: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
        if (line.size() > 6 && line.substr(0, 5) == "guid:")
        {
            gx::String guidStr = line.substr(5);
            // 先頭の空白をスキップ
            size_t start = 0;
            while (start < guidStr.size() && guidStr[start] == ' ')
                ++start;
            guidStr = guidStr.substr(start);
            outData.guid = GUID::FromString(guidStr);
        }
        // import:key=value
        else if (line.size() > 7 && line.substr(0, 7) == "import:")
        {
            gx::String kv = line.substr(7);
            auto eqPos = kv.find('=');
            if (eqPos != gx::String::npos)
            {
                gx::String key = kv.substr(0, eqPos);
                gx::String value = kv.substr(eqPos + 1);
                outData.importSettings[key] = value;
            }
        }
    }

    return outData.guid.IsValid();
}

// ============================================================================
// Save
// ============================================================================
bool AssetMeta::Save(const gx::String& metaPath, const AssetMetaData& data)
{
    std::ofstream file(metaPath.c_str());
    if (!file.is_open())
        return false;

    file << "guid: " << data.guid.ToString() << "\n";

    for (const auto& [key, value] : data.importSettings)
    {
        file << "import:" << key << "=" << value << "\n";
    }

    return file.good();
}

} // namespace gx
