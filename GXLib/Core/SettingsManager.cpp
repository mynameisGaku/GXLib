/// @file SettingsManager.cpp
/// @brief 設定マネージャーの実装
#include "pch_common.h"
#include "Core/SettingsManager.h"
#include "Core/Logger.h"
#include <sstream>

namespace gx
{

SettingsManager& SettingsManager::Instance()
{
    static SettingsManager instance;
    return instance;
}

void SettingsManager::SetInt(const std::string& section, const std::string& key, int value)
{
    SetValue(section, key, std::to_string(value));
}

void SettingsManager::SetFloat(const std::string& section, const std::string& key, float value)
{
    SetValue(section, key, std::to_string(value));
}

void SettingsManager::SetString(const std::string& section, const std::string& key, const std::string& value)
{
    SetValue(section, key, value);
}

void SettingsManager::SetBool(const std::string& section, const std::string& key, bool value)
{
    SetValue(section, key, value ? "true" : "false");
}

int SettingsManager::GetInt(const std::string& section, const std::string& key, int defaultValue) const
{
    auto v = GetValue(section, key);
    if (v.empty()) return defaultValue;
    try { return std::stoi(v); } catch (...) { return defaultValue; }
}

float SettingsManager::GetFloat(const std::string& section, const std::string& key, float defaultValue) const
{
    auto v = GetValue(section, key);
    if (v.empty()) return defaultValue;
    try { return std::stof(v); } catch (...) { return defaultValue; }
}

std::string SettingsManager::GetString(const std::string& section, const std::string& key, const std::string& defaultValue) const
{
    auto v = GetValue(section, key);
    return v.empty() ? defaultValue : v;
}

bool SettingsManager::GetBool(const std::string& section, const std::string& key, bool defaultValue) const
{
    auto v = GetValue(section, key);
    if (v.empty()) return defaultValue;
    return v == "true" || v == "1";
}

bool SettingsManager::SaveToFile(const std::string& filePath)
{
    try
    {
        std::ofstream file(filePath);
        if (!file.is_open()) return false;

        // シンプルなJSON風フォーマット: { "section": { "key": "value" } }
        file << "{\n";
        bool firstSection = true;
        for (const auto& [section, kvs] : m_data)
        {
            if (!firstSection) file << ",\n";
            firstSection = false;
            file << "  \"" << section << "\": {\n";
            bool firstKey = true;
            for (const auto& [key, value] : kvs)
            {
                if (!firstKey) file << ",\n";
                firstKey = false;
                // 値のクォートをエスケープ
                std::string escaped = value;
                for (size_t pos = 0; (pos = escaped.find('"', pos)) != std::string::npos; pos += 2)
                    escaped.insert(pos, "\\");
                file << "    \"" << key << "\": \"" << escaped << "\"";
            }
            file << "\n  }";
        }
        file << "\n}\n";
        return true;
    }
    catch (...)
    {
        GX_LOG_ERROR("SettingsManager: Failed to save to %s", filePath.c_str());
        return false;
    }
}

bool SettingsManager::LoadFromFile(const std::string& filePath)
{
    try
    {
        std::ifstream file(filePath);
        if (!file.is_open()) return false;

        m_data.clear();
        std::string content((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());

        // 独自JSONフォーマットのシンプルなパーサー
        std::string currentSection;
        size_t pos = 0;
        while (pos < content.size())
        {
            // 次のクォート文字列を検索
            size_t qStart = content.find('"', pos);
            if (qStart == std::string::npos) break;
            size_t qEnd = content.find('"', qStart + 1);
            if (qEnd == std::string::npos) break;

            std::string token = content.substr(qStart + 1, qEnd - qStart - 1);
            pos = qEnd + 1;

            // 空白をスキップ
            while (pos < content.size() && (content[pos] == ' ' || content[pos] == '\n' || content[pos] == '\r' || content[pos] == '\t'))
                pos++;

            if (pos < content.size() && content[pos] == ':')
            {
                pos++;
                // 空白をスキップ
                while (pos < content.size() && (content[pos] == ' ' || content[pos] == '\n' || content[pos] == '\r' || content[pos] == '\t'))
                    pos++;

                if (pos < content.size() && content[pos] == '{')
                {
                    // セクションの場合
                    currentSection = token;
                    pos++;
                }
                else if (pos < content.size() && content[pos] == '"')
                {
                    // キー値ペアの場合
                    size_t vStart = pos + 1;
                    size_t vEnd = content.find('"', vStart);
                    if (vEnd != std::string::npos)
                    {
                        std::string value = content.substr(vStart, vEnd - vStart);
                        // クォートのアンエスケープ
                        for (size_t p = 0; (p = value.find("\\\"", p)) != std::string::npos; p += 1)
                            value.erase(p, 1);
                        m_data[currentSection][token] = value;
                        pos = vEnd + 1;
                    }
                }
            }
        }
        return true;
    }
    catch (...)
    {
        GX_LOG_ERROR("SettingsManager: Failed to load from %s", filePath.c_str());
        return false;
    }
}

void SettingsManager::Clear()
{
    m_data.clear();
}

std::string SettingsManager::GetValue(const std::string& section, const std::string& key) const
{
    auto sit = m_data.find(section);
    if (sit == m_data.end()) return "";
    auto kit = sit->second.find(key);
    if (kit == sit->second.end()) return "";
    return kit->second;
}

void SettingsManager::SetValue(const std::string& section, const std::string& key, const std::string& value)
{
    m_data[section][key] = value;
}

} // namespace gx
