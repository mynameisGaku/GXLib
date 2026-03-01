/// @file JobSystem.cpp
/// @brief 対応する.hの実装
#include "pch_common.h"
#include <unordered_set>
#include "Core/JobSystem.h"
#include "Core/Logger.h"

namespace gx {

JobSystem::~JobSystem()
{
    Shutdown();
}

JobSystem& JobSystem::Instance()
{
    static JobSystem instance;
    return instance;
}

bool JobSystem::Initialize(uint32_t workerCount)
{
    if (m_initialized)
    {
        GX_LOG_WARN("JobSystem: Already initialized");
        return false;
    }

    if (workerCount == 0)
    {
        uint32_t hw = std::thread::hardware_concurrency();
        workerCount = (hw > 1) ? (hw - 1) : 1;
    }

    m_workerCount = workerCount;
    m_running.store(true);
    m_initialized = true;

    m_workers.reserve(workerCount);
    for (uint32_t i = 0; i < workerCount; ++i)
    {
        m_workers.emplace_back(&JobSystem::WorkerThread, this);
    }

    GX_LOG_INFO("JobSystem: Initialized with %u worker threads", workerCount);
    return true;
}

void JobSystem::Shutdown()
{
    if (!m_initialized)
        return;

    // 実行中フラグをオフにし、全ワーカーを起床させる
    m_running.store(false);
    m_condition.notify_all();

    for (auto& worker : m_workers)
    {
        if (worker.joinable())
            worker.join();
    }

    m_workers.clear();

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_jobQueue.clear();
        m_completedJobs.clear();
    }

    m_workerCount = 0;
    m_initialized = false;
}

JobHandle JobSystem::Submit(JobFunction job, JobPriority priority)
{
    JobHandle handle;
    handle.id = m_nextJobId.fetch_add(1);

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        Job j;
        j.function = std::move(job);
        j.priority = priority;
        j.id = handle.id;
        j.dependencyId = 0;
        j.completed = false;
        m_jobQueue.push_back(std::move(j));
    }

    m_condition.notify_one();
    return handle;
}

JobHandle JobSystem::SubmitAfter(JobHandle dependency, JobFunction job, JobPriority priority)
{
    JobHandle handle;
    handle.id = m_nextJobId.fetch_add(1);

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        Job j;
        j.function = std::move(job);
        j.priority = priority;
        j.id = handle.id;
        j.dependencyId = dependency.id;
        j.completed = false;
        m_jobQueue.push_back(std::move(j));
    }

    m_condition.notify_one();
    return handle;
}

void JobSystem::Wait(JobHandle handle)
{
    if (!handle.IsValid())
        return;

    std::unique_lock<std::mutex> lock(m_mutex);
    m_completionCondition.wait(lock, [this, &handle]() {
        return m_completedJobs.count(handle.id) > 0;
    });
}

void JobSystem::WaitAll()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_completionCondition.wait(lock, [this]() {
        return m_jobQueue.empty();
    });
}

bool JobSystem::IsComplete(JobHandle handle) const
{
    if (!handle.IsValid())
        return true;

    std::lock_guard<std::mutex> lock(m_mutex);
    return m_completedJobs.count(handle.id) > 0;
}

void JobSystem::ParallelFor(uint32_t count, const std::function<void(uint32_t)>& body, uint32_t minBatchSize)
{
    if (count == 0)
        return;

    if (!m_initialized || count <= minBatchSize)
    {
        // シングルスレッドフォールバック
        for (uint32_t i = 0; i < count; ++i)
            body(i);
        return;
    }

    // バッチに分割
    uint32_t batchCount = (count + minBatchSize - 1) / minBatchSize;
    std::vector<JobHandle> handles;
    handles.reserve(batchCount);

    for (uint32_t batch = 0; batch < batchCount; ++batch)
    {
        uint32_t start = batch * minBatchSize;
        uint32_t end = (std::min)(start + minBatchSize, count);

        handles.push_back(Submit([&body, start, end]() {
            for (uint32_t i = start; i < end; ++i)
                body(i);
        }, JobPriority::Normal));
    }

    // 全バッチの完了を待機
    for (auto& h : handles)
        Wait(h);
}

void JobSystem::WorkerThread()
{
    while (m_running.load())
    {
        Job jobToExecute;
        bool hasJob = false;

        {
            std::unique_lock<std::mutex> lock(m_mutex);

            // ジョブが利用可能になるまで待機
            m_condition.wait(lock, [this]() {
                if (!m_running.load())
                    return true;

                // 実行可能なジョブがあるかチェック
                for (const auto& job : m_jobQueue)
                {
                    if (job.dependencyId == 0 || m_completedJobs.count(job.dependencyId) > 0)
                        return true;
                }
                return false;
            });

            if (!m_running.load() && m_jobQueue.empty())
                break;

            // 最も優先度が高い実行可能ジョブを探す
            int bestIndex = -1;
            int bestPriority = -1;

            for (int i = 0; i < static_cast<int>(m_jobQueue.size()); ++i)
            {
                const auto& job = m_jobQueue[i];
                // 依存ジョブが完了しているかチェック
                if (job.dependencyId != 0 && m_completedJobs.count(job.dependencyId) == 0)
                    continue;

                int prio = static_cast<int>(job.priority);
                if (prio > bestPriority)
                {
                    bestPriority = prio;
                    bestIndex = i;
                }
            }

            if (bestIndex >= 0)
            {
                jobToExecute = std::move(m_jobQueue[bestIndex]);
                m_jobQueue.erase(m_jobQueue.begin() + bestIndex);
                hasJob = true;
            }
        }

        if (hasJob)
        {
            // ロック外でジョブを実行
            if (jobToExecute.function)
            {
                jobToExecute.function();
            }

            // 完了を記録
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_completedJobs.insert(jobToExecute.id);
            }

            // 依存待ちジョブや WaitAll を起床させる
            m_completionCondition.notify_all();
            m_condition.notify_all();
        }
    }
}

} // namespace gx
