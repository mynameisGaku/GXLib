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

SaveSystem::SaveSystem(const gx::String& saveDirectory)
    : m_saveDirectory(saveDirectory)
{
}

void SaveSystem::SetInt(const gx::String& key, int value)
{
    m_data[key] = gx::String(std::to_string(value));
}

void SaveSystem::SetFloat(const gx::String& key, float value)
{
    m_data[key] = gx::String(std::to_string(value));
}

void SaveSystem::SetString(const gx::String& key, const gx::String& value)
{
    m_data[key] = value;
}

void SaveSystem::SetBool(const gx::String& key, bool value)
{
    m_data[key] = value ? "1" : "0";
}

int SaveSystem::GetInt(const gx::String& key, int defaultValue) const
{
    auto it = m_data.find(key);
    if (it == m_data.end()) return defaultValue;
    try { return std::stoi(it->second.ToStdString()); }
    catch (...) { return defaultValue; }
}

float SaveSystem::GetFloat(const gx::String& key, float defaultValue) const
{
    auto it = m_data.find(key);
    if (it == m_data.end()) return defaultValue;
    try { return std::stof(it->second.ToStdString()); }
    catch (...) { return defaultValue; }
}

gx::String SaveSystem::GetString(const gx::String& key, const gx::String& defaultValue) const
{
    auto it = m_data.find(key);
    if (it == m_data.end()) return defaultValue;
    return it->second;
}

bool SaveSystem::GetBool(const gx::String& key, bool defaultValue) const
{
    auto it = m_data.find(key);
    if (it == m_data.end()) return defaultValue;
    return it->second == "1" || it->second == "true";
}

bool SaveSystem::HasKey(const gx::String& key) const
{
    return m_data.count(key) > 0;
}

void SaveSystem::RemoveKey(const gx::String& key)
{
    m_data.erase(key);
}

void SaveSystem::ClearAll()
{
    m_data.clear();
}

bool SaveSystem::SaveToSlot(uint32_t slotIndex, const gx::String& label, float playTime)
{
    namespace fs = std::filesystem;
    try
    {
        fs::create_directories(fs::path(m_saveDirectory.c_str()));

        // データを保存
        std::ofstream dataFile(GetSlotPath(slotIndex).c_str());
        if (!dataFile.is_open()) return false;

        for (const auto& [key, value] : m_data)
        {
            // シンプルなkey=value形式（改行をエスケープ）
            gx::String escaped = value;
            for (size_t pos = 0; (pos = escaped.find('\n', pos)) != gx::String::npos; pos += 2)
                escaped.replace(pos, 1, "\\n");
            dataFile << key << "=" << escaped << "\n";
        }
        dataFile.close();

        // メタデータを保存
        std::ofstream metaFile(GetMetaPath(slotIndex).c_str());
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
        std::ifstream dataFile(GetSlotPath(slotIndex).c_str());
        if (!dataFile.is_open()) return false;

        m_data.clear();
        gx::String line;
        while (gx::container::getline(dataFile, line))
        {
            size_t eq = line.find('=');
            if (eq == gx::String::npos) continue;
            gx::String key = line.substr(0, eq);
            gx::String value = line.substr(eq + 1);
            // 改行のアンエスケープ
            for (size_t pos = 0; (pos = value.find("\\n", pos)) != gx::String::npos; pos += 1)
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
        fs::remove(fs::path(GetSlotPath(slotIndex).c_str()));
        fs::remove(fs::path(GetMetaPath(slotIndex).c_str()));
        return true;
    }
    catch (...)
    {
        return false;
    }
}

gx::Vector<SaveSlotInfo> SaveSystem::GetSlotList(uint32_t maxSlots) const
{
    gx::Vector<SaveSlotInfo> slots;
    for (uint32_t i = 0; i < maxSlots; ++i)
    {
        SaveSlotInfo info;
        info.slotIndex = i;

        std::ifstream metaFile(GetMetaPath(i).c_str());
        if (metaFile.is_open())
        {
            info.exists = true;
            gx::String line;
            while (gx::container::getline(metaFile, line))
            {
                size_t eq = line.find('=');
                if (eq == gx::String::npos) continue;
                gx::String key = line.substr(0, eq);
                gx::String value = line.substr(eq + 1);
                if (key == "label")    info.label = value;
                else if (key == "dateTime") info.dateTime = value;
                else if (key == "playTime") { try { info.playTime = std::stof(value.ToStdString()); } catch (...) {} }
            }
        }
        slots.push_back(info);
    }
    return slots;
}

gx::String SaveSystem::GetSlotPath(uint32_t slotIndex) const
{
    return m_saveDirectory + "/save_" + gx::String(std::to_string(slotIndex)) + ".gxsave";
}

gx::String SaveSystem::GetMetaPath(uint32_t slotIndex) const
{
    return m_saveDirectory + "/save_" + gx::String(std::to_string(slotIndex)) + ".gxmeta";
}

} // namespace gx
