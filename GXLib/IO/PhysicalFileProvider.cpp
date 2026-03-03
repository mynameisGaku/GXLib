#include "pch_common.h"
#include "IO/PhysicalFileProvider.h"
#include "Core/Logger.h"

namespace gx {

PhysicalFileProvider::PhysicalFileProvider(const gx::String& rootDir)
    : m_rootDir(rootDir)
{
    // 末尾スラッシュを整える (なければ追加)
    if (!m_rootDir.empty() && m_rootDir.back() != '/' && m_rootDir.back() != '\\')
        m_rootDir += '/';
}

gx::String PhysicalFileProvider::ResolvePath(const gx::String& path) const
{
    return m_rootDir + path;
}

bool PhysicalFileProvider::Exists(const gx::String& path) const
{
    gx::String fullPath = ResolvePath(path);
    DWORD attrib = GetFileAttributesA(fullPath.c_str());
    return (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY));
}

FileData PhysicalFileProvider::Read(const gx::String& path) const
{
    gx::String fullPath = ResolvePath(path);
    std::ifstream file(fullPath, std::ios::binary | std::ios::ate);
    if (!file.is_open())
        return FileData{};

    auto fileSize = file.tellg();
    if (fileSize <= 0)
        return FileData{};

    FileData result;
    result.data.resize(static_cast<size_t>(fileSize));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(result.data.data()), fileSize);

    return result;
}

bool PhysicalFileProvider::Write(const gx::String& path, const void* data, size_t size)
{
    gx::String fullPath = ResolvePath(path);
    std::ofstream file(fullPath, std::ios::binary);
    if (!file.is_open())
        return false;

    file.write(static_cast<const char*>(data), size);
    return file.good();
}

} // namespace gx
