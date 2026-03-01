/// @file SaveSystem.cpp
/// @brief セーブシステムの実装
#include "pch_common.h"
#include "Core/SaveSystem.h"
#include "Core/Logger.h"
#include <filesystem>
#include <sstream>
#include <chrono>
#include <iomanip>

namespace gx
{

SaveSystem::SaveSystem(const std::string& saveDirectory)
    : m_saveDirectory(saveDirectory)
{
}

void SaveSystem::SetInt(const std::string& key, int value)
{
    m_data[key] = std::to_string(value);
}

void SaveSystem::SetFloat(const std::string& key, float value)
{
    m_data[key] = std::to_string(value);
}

void SaveSystem::SetString(const std::string& key, const std::string& value)
{
    m_data[key] = value;
}

void SaveSystem::SetBool(const std::string& key, bool value)
{
    m_data[key] = value ? "1" : "0";
}

int SaveSystem::GetInt(const std::string& key, int defaultValue) const
{
    auto it = m_data.find(key);
    if (it == m_data.end()) return defaultValue;
    try { return std::stoi(it->second); }
    catch (...) { return defaultValue; }
}

float SaveSystem::GetFloat(const std::string& key, float defaultValue) const
{
    auto it = m_data.find(key);
    if (it == m_data.end()) return defaultValue;
    try { return std::stof(it->second); }
    catch (...) { return defaultValue; }
}

std::string SaveSystem::GetString(const std::string& key, const std::string& defaultValue) const
{
    auto it = m_data.find(key);
    if (it == m_data.end()) return defaultValue;
    return it->second;
}

bool SaveSystem::GetBool(const std::string& key, bool defaultValue) const
{
    auto it = m_data.find(key);
    if (it == m_data.end()) return defaultValue;
    return it->second == "1" || it->second == "true";
}

bool SaveSystem::HasKey(const std::string& key) const
{
    return m_data.count(key) > 0;
}

void SaveSystem::RemoveKey(const std::string& key)
{
    m_data.erase(key);
}

void SaveSystem::ClearAll()
{
    m_data.clear();
}

bool SaveSystem::SaveToSlot(uint32_t slotIndex, const std::string& label, float playTime)
{
    namespace fs = std::filesystem;
    try
    {
        fs::create_directories(m_saveDirectory);

        // データを保存
        std::ofstream dataFile(GetSlotPath(slotIndex));
        if (!dataFile.is_open()) return false;

        for (const auto& [key, value] : m_data)
        {
            // シンプルなkey=value形式（改行をエスケープ）
            std::string escaped = value;
            for (size_t pos = 0; (pos = escaped.find('\n', pos)) != std::string::npos; pos += 2)
                escaped.replace(pos, 1, "\\n");
            dataFile << key << "=" << escaped << "\n";
        }
        dataFile.close();

        // メタデータを保存
        std::ofstream metaFile(GetMetaPath(slotIndex));
        if (!metaFile.is_open()) return false;

        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        struct tm localTime;
        localtime_s(&localTime, &time);

        std::ostringstream dateStr;
        dateStr << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");

        metaFile << "label=" << label << "\n";
        metaFile << "dateTime=" << dateStr.str() << "\n";
        metaFile << "playTime=" << playTime << "\n";
        metaFile.close();

        return true;
    }
    catch (...)
    {
        GX_LOG_ERROR("SaveSystem: Failed to save to slot %u", slotIndex);
        return false;
    }
}

bool SaveSystem::LoadFromSlot(uint32_t slotIndex)
{
    try
    {
        std::ifstream dataFile(GetSlotPath(slotIndex));
        if (!dataFile.is_open()) return false;

        m_data.clear();
        std::string line;
        while (std::getline(dataFile, line))
        {
            size_t eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key = line.substr(0, eq);
            std::string value = line.substr(eq + 1);
            // 改行のアンエスケープ
            for (size_t pos = 0; (pos = value.find("\\n", pos)) != std::string::npos; pos += 1)
                value.replace(pos, 2, "\n");
            m_data[key] = value;
        }
        return true;
    }
    catch (...)
    {
        GX_LOG_ERROR("SaveSystem: Failed to load from slot %u", slotIndex);
        return false;
    }
}

bool SaveSystem::DeleteSlot(uint32_t slotIndex)
{
    namespace fs = std::filesystem;
    try
    {
        fs::remove(GetSlotPath(slotIndex));
        fs::remove(GetMetaPath(slotIndex));
        return true;
    }
    catch (...)
    {
        return false;
    }
}

std::vector<SaveSlotInfo> SaveSystem::GetSlotList(uint32_t maxSlots) const
{
    std::vector<SaveSlotInfo> slots;
    for (uint32_t i = 0; i < maxSlots; ++i)
    {
        SaveSlotInfo info;
        info.slotIndex = i;

        std::ifstream metaFile(GetMetaPath(i));
        if (metaFile.is_open())
        {
            info.exists = true;
            std::string line;
            while (std::getline(metaFile, line))
            {
                size_t eq = line.find('=');
                if (eq == std::string::npos) continue;
                std::string key = line.substr(0, eq);
                std::string value = line.substr(eq + 1);
                if (key == "label")    info.label = value;
                else if (key == "dateTime") info.dateTime = value;
                else if (key == "playTime") { try { info.playTime = std::stof(value); } catch (...) {} }
            }
        }
        slots.push_back(info);
    }
    return slots;
}

std::string SaveSystem::GetSlotPath(uint32_t slotIndex) const
{
    return m_saveDirectory + "/save_" + std::to_string(slotIndex) + ".gxsave";
}

std::string SaveSystem::GetMetaPath(uint32_t slotIndex) const
{
    return m_saveDirectory + "/save_" + std::to_string(slotIndex) + ".gxmeta";
}

} // namespace gx
