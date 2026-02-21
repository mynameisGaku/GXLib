/// @file PakFileProvider.cpp
/// @brief GXPAK バンドルプロバイダー実装 — gxloader::PakLoader への委譲
#include "pch.h"
#include "IO/PakFileProvider.h"

namespace GX {

bool PakFileProvider::Open(const std::string& pakPath)
{
    return m_loader.Open(pakPath);
}

bool PakFileProvider::Exists(const std::string& path) const
{
    return m_loader.Contains(path);
}

FileData PakFileProvider::Read(const std::string& path) const
{
    FileData result;
    result.data = m_loader.Read(path);
    return result;
}

} // namespace GX
