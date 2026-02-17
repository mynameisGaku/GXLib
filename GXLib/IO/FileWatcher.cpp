#include "pch.h"
#include "IO/FileWatcher.h"
#include "Core/Logger.h"

namespace GX {

FileWatcher::FileWatcher()
{
    m_stopEvent = CreateEventA(nullptr, TRUE, FALSE, nullptr);
    if (!m_stopEvent)
        GX_LOG_ERROR("FileWatcher: Failed to create stop event");
}

FileWatcher::~FileWatcher()
{
    Stop();
    if (m_stopEvent)
        CloseHandle(m_stopEvent);
}

void FileWatcher::Watch(const std::string& directory,
                         std::function<void(const std::string& path)> onChange)
{
    auto entry = std::make_unique<WatchEntry>();
    entry->directory = directory;
    entry->onChange = std::move(onChange);
    entry->buffer.resize(4096);

    entry->hDir = CreateFileA(
        directory.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        nullptr);

    if (entry->hDir == INVALID_HANDLE_VALUE)
    {
        GX_LOG_ERROR("FileWatcher: Failed to open directory: %s", directory.c_str());
        return;
    }

    entry->overlapped.hEvent = CreateEventA(nullptr, TRUE, FALSE, nullptr);
    if (!entry->overlapped.hEvent)
    {
        GX_LOG_ERROR("FileWatcher: Failed to create event for directory: %s", directory.c_str());
        CloseHandle(entry->hDir);
        return;
    }
    m_watches.push_back(std::move(entry));

    // 未起動なら監視スレッドを開始する (二重起動を防ぐ)
    if (!m_running.load())
    {
        m_running.store(true);
        ResetEvent(m_stopEvent);
        m_watchThread = std::thread(&FileWatcher::WatchLoop, this);
    }
}

void FileWatcher::StartRead(WatchEntry* entry)
{
    if (entry->overlapped.hEvent)
        CloseHandle(entry->overlapped.hEvent);
    memset(&entry->overlapped, 0, sizeof(OVERLAPPED));
    entry->overlapped.hEvent = CreateEventA(nullptr, TRUE, FALSE, nullptr);
    if (!entry->overlapped.hEvent)
    {
        GX_LOG_ERROR("FileWatcher: Failed to create event in StartRead");
        entry->active = false;
        return;
    }

    BOOL result = ReadDirectoryChangesW(
        entry->hDir,
        entry->buffer.data(),
        static_cast<DWORD>(entry->buffer.size()),
        TRUE, // サブディレクトリも監視する
        FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SIZE,
        nullptr,
        &entry->overlapped,
        nullptr);

    entry->active = (result != FALSE);
}

void FileWatcher::WatchLoop()
{
    // すべての監視エントリで非同期読み取りを開始する
    for (auto& entry : m_watches)
        StartRead(entry.get());

    while (m_running.load())
    {
        // 待機ハンドル配列を作る: 停止イベント + 各監視イベント
        std::vector<HANDLE> handles;
        handles.push_back(m_stopEvent);
        for (auto& entry : m_watches)
        {
            if (entry->active && entry->overlapped.hEvent)
                handles.push_back(entry->overlapped.hEvent);
        }

        // 初心者向け: 複数イベントを待ち、通知されたイベントの位置が返る
        DWORD waitResult = WaitForMultipleObjects(
            static_cast<DWORD>(handles.size()),
            handles.data(),
            FALSE,
            1000); // 定期的に抜けられるよう1秒タイムアウト

        if (waitResult == WAIT_OBJECT_0)
            break; // 停止イベント

        if (waitResult == WAIT_TIMEOUT)
            continue;

        // どの監視が通知されたか特定する
        DWORD idx = waitResult - WAIT_OBJECT_0 - 1;
        size_t watchIdx = 0;
        for (size_t i = 0; i < m_watches.size(); ++i)
        {
            if (m_watches[i]->active && m_watches[i]->overlapped.hEvent)
            {
                if (watchIdx == idx)
                {
                    auto* entry = m_watches[i].get();
                    DWORD bytesReturned = 0;
                    if (GetOverlappedResult(entry->hDir, &entry->overlapped, &bytesReturned, FALSE) && bytesReturned > 0)
                    {
                        auto* info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(entry->buffer.data());
                        while (info)
                        {
                            int nameLen = WideCharToMultiByte(CP_UTF8, 0,
                                info->FileName, info->FileNameLength / sizeof(WCHAR),
                                nullptr, 0, nullptr, nullptr);
                            std::string fileName(nameLen, '\0');
                            WideCharToMultiByte(CP_UTF8, 0,
                                info->FileName, info->FileNameLength / sizeof(WCHAR),
                                fileName.data(), nameLen, nullptr, nullptr);

                            std::string fullPath = entry->directory + "/" + fileName;

                            {
                                std::lock_guard<std::mutex> lock(m_mutex);
                                m_pendingNotifications.push_back({ fullPath, entry->onChange });
                            }

                            if (info->NextEntryOffset == 0)
                                break;
                            info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
                                reinterpret_cast<uint8_t*>(info) + info->NextEntryOffset);
                        }
                    }

                    // 次の通知に備えて再度読み取りを開始する
                    if (entry->overlapped.hEvent)
                        CloseHandle(entry->overlapped.hEvent);
                    StartRead(entry);
                    break;
                }
                ++watchIdx;
            }
        }
    }

    // 後片付け (非同期I/Oとイベントを解放)
    for (auto& entry : m_watches)
    {
        if (entry->active && entry->overlapped.hEvent)
        {
            CancelIo(entry->hDir);
            CloseHandle(entry->overlapped.hEvent);
            entry->overlapped.hEvent = nullptr;
        }
    }
}

void FileWatcher::Update()
{
    std::vector<std::pair<std::string, std::function<void(const std::string&)>>> notifications;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        notifications.swap(m_pendingNotifications);
    }

    for (auto& [path, callback] : notifications)
    {
        if (callback)
            callback(path);
    }
}

void FileWatcher::Stop()
{
    if (!m_running.load())
        return;

    m_running.store(false);
    if (m_stopEvent)
        SetEvent(m_stopEvent);

    if (m_watchThread.joinable())
        m_watchThread.join();

    for (auto& entry : m_watches)
    {
        if (entry->hDir != INVALID_HANDLE_VALUE)
        {
            CloseHandle(entry->hDir);
            entry->hDir = INVALID_HANDLE_VALUE;
        }
    }
    m_watches.clear();
}

} // namespace GX
