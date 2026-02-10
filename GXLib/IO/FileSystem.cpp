#include "pch.h"
#include "IO/FileSystem.h"
#include "Core/Logger.h"

namespace GX {

FileSystem& FileSystem::Instance()
{
    static FileSystem s_instance;
    return s_instance;
}

std::string FileSystem::NormalizePath(const std::string& path)
{
    std::string result = path;
    // バックスラッシュをスラッシュに統一する (Windows表記→共通表記)
    for (auto& c : result)
    {
        if (c == '\\') c = '/';
    }
    // 先頭スラッシュを除去する
    while (!result.empty() && result[0] == '/')
        result.erase(result.begin());
    return result;
}

void FileSystem::Mount(const std::string& mountPoint, std::shared_ptr<IFileProvider> provider)
{
    MountEntry entry;
    entry.mountPoint = NormalizePath(mountPoint);
    entry.provider = std::move(provider);
    m_mounts.push_back(std::move(entry));

    // 優先度の高い順に並べ替える
    std::sort(m_mounts.begin(), m_mounts.end(),
        [](const MountEntry& a, const MountEntry& b) {
            return a.provider->Priority() > b.provider->Priority();
        });
}

void FileSystem::Unmount(const std::string& mountPoint)
{
    std::string normalized = NormalizePath(mountPoint);
    m_mounts.erase(
        std::remove_if(m_mounts.begin(), m_mounts.end(),
            [&](const MountEntry& e) { return e.mountPoint == normalized; }),
        m_mounts.end());
}

bool FileSystem::Exists(const std::string& path) const
{
    std::string normalized = NormalizePath(path);

    for (const auto& mount : m_mounts)
    {
        // パスがマウントポイントで始まるか確認する
        if (mount.mountPoint.empty() ||
            (normalized.size() >= mount.mountPoint.size() &&
             normalized.compare(0, mount.mountPoint.size(), mount.mountPoint) == 0))
        {
            std::string relativePath = mount.mountPoint.empty()
                ? normalized
                : normalized.substr(mount.mountPoint.size());
            // 相対パスの先頭スラッシュを除去する
            while (!relativePath.empty() && relativePath[0] == '/')
                relativePath.erase(relativePath.begin());

            if (mount.provider->Exists(relativePath.empty() ? normalized : relativePath))
                return true;
        }
    }
    return false;
}

FileData FileSystem::ReadFile(const std::string& path) const
{
    std::string normalized = NormalizePath(path);

    for (const auto& mount : m_mounts)
    {
        if (mount.mountPoint.empty() ||
            (normalized.size() >= mount.mountPoint.size() &&
             normalized.compare(0, mount.mountPoint.size(), mount.mountPoint) == 0))
        {
            std::string relativePath = mount.mountPoint.empty()
                ? normalized
                : normalized.substr(mount.mountPoint.size());
            while (!relativePath.empty() && relativePath[0] == '/')
                relativePath.erase(relativePath.begin());

            const std::string& lookupPath = relativePath.empty() ? normalized : relativePath;
            if (mount.provider->Exists(lookupPath))
                return mount.provider->Read(lookupPath);
        }
    }

    return FileData{};
}

bool FileSystem::WriteFile(const std::string& path, const void* data, size_t size)
{
    std::string normalized = NormalizePath(path);

    for (auto& mount : m_mounts)
    {
        if (mount.mountPoint.empty() ||
            (normalized.size() >= mount.mountPoint.size() &&
             normalized.compare(0, mount.mountPoint.size(), mount.mountPoint) == 0))
        {
            std::string relativePath = mount.mountPoint.empty()
                ? normalized
                : normalized.substr(mount.mountPoint.size());
            while (!relativePath.empty() && relativePath[0] == '/')
                relativePath.erase(relativePath.begin());

            const std::string& writePath = relativePath.empty() ? normalized : relativePath;
            if (mount.provider->Write(writePath, data, size))
                return true;
        }
    }

    return false;
}

void FileSystem::Clear()
{
    m_mounts.clear();
}

} // namespace GX
