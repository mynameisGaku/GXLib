/// @file ArchiveFileProvider.cpp
/// @brief アーカイブプロバイダー実装 — Archive への委譲
#include "pch.h"
#include "IO/ArchiveFileProvider.h"

namespace GX {

bool ArchiveFileProvider::Open(const std::string& archivePath, const std::string& password)
{
    return m_archive.Open(archivePath, password);
}

bool ArchiveFileProvider::Exists(const std::string& path) const
{
    return m_archive.Contains(path);
}

FileData ArchiveFileProvider::Read(const std::string& path) const
{
    return m_archive.Read(path);
}

} // namespace GX
