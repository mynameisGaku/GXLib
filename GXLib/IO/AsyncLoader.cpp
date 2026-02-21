#include "pch.h"
#include "IO/AsyncLoader.h"
#include "Core/Logger.h"

namespace GX {

AsyncLoader::AsyncLoader()
{
    m_workerThread = std::thread(&AsyncLoader::WorkerLoop, this);
}

AsyncLoader::~AsyncLoader()
{
    m_running.store(false);
    m_cv.notify_one();
    if (m_workerThread.joinable())
        m_workerThread.join();
}

uint32_t AsyncLoader::Load(const std::string& path,
                            std::function<void(FileData&)> onComplete)
{
    auto req = std::make_shared<LoadRequest>();
    req->path = path;
    req->onComplete = std::move(onComplete);

    uint32_t id;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        id = m_nextId++;
        m_statusMap[id] = LoadStatus::Pending;
        m_pendingQueue.push_back({ id, req });
    }
    m_cv.notify_one();
    return id;
}

// 完了キューをswapで一括取得し、ロックの外でコールバックを発火する。
// これによりコールバック内からLoad()を呼んでもデッドロックしない。
void AsyncLoader::Update()
{
    std::vector<std::shared_ptr<LoadRequest>> completed;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        completed.swap(m_completedQueue);
    }

    for (auto& req : completed)
    {
        if (req->onComplete)
            req->onComplete(req->result);
    }
}

void AsyncLoader::CancelAll()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& [id, req] : m_pendingQueue)
        m_statusMap[id] = LoadStatus::Error;
    m_pendingQueue.clear();
}

LoadStatus AsyncLoader::GetStatus(uint32_t requestId) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_statusMap.find(requestId);
    if (it != m_statusMap.end())
        return it->second;
    return LoadStatus::Error;
}

void AsyncLoader::WorkerLoop()
{
    while (true)
    {
        std::pair<uint32_t, std::shared_ptr<LoadRequest>> work;

        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this]() { return !m_running.load() || !m_pendingQueue.empty(); });

            if (!m_running.load() && m_pendingQueue.empty())
                return;

            if (m_pendingQueue.empty())
                continue;

            work = std::move(m_pendingQueue.front());
            m_pendingQueue.erase(m_pendingQueue.begin());
            m_statusMap[work.first] = LoadStatus::Loading;
        }

        // ロックの外で読み込みを行う (I/Oで他スレッドを止めないため)
        work.second->result = FileSystem::Instance().ReadFile(work.second->path);
        work.second->status = work.second->result.IsValid()
            ? LoadStatus::Complete : LoadStatus::Error;

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_statusMap[work.first] = work.second->status;
            m_completedQueue.push_back(std::move(work.second));
        }
    }
}

} // namespace GX
