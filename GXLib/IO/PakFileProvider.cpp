/// @file PakFileProvider.cpp
/// @brief GXPAK バンドルプロバイダー実装 — gxloader::PakLoader への委譲
#include "pch_common.h"
#include "IO/PakFileProvider.h"

namespace gx {

bool PakFileProvider::Open(const gx::String& pakPath)
{
    return m_loader.Open(pakPath.ToStdString());
}

bool PakFileProvider::Exists(const gx::String& path) const
{
    return m_loader.Contains(path.ToStdString());
}

FileData PakFileProvider::Read(const gx::String& path) const
{
    FileData result;
    auto stdvec = m_loader.Read(path.ToStdString());
    result.data = gx::Vector<uint8_t>(stdvec.data(), stdvec.data() + stdvec.size());
    return result;
}

} // namespace gx
