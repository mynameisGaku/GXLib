#pragma once
/// @file pak_loader.h
/// @brief GXPAK bundle runtime loader

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include "gxpak.h"

namespace gxloader
{

class PakLoader
{
public:
    bool Open(const std::string& filePath);
    void Close();

    bool Contains(const std::string& path) const;
    std::vector<uint8_t> Read(const std::string& path) const;
    std::vector<gxfmt::GxpakEntry> GetEntries() const;
    std::vector<gxfmt::GxpakEntry> GetEntriesByType(gxfmt::GxpakAssetType type) const;

private:
    std::string m_filePath;
    std::vector<gxfmt::GxpakEntry> m_entries;
};

} // namespace gxloader
