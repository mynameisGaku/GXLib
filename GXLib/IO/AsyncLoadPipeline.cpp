/// @file AsyncLoadPipeline.cpp
/// @brief 2スレッド非同期ロードパイプライン実装
#include "pch_common.h"
#include "IO/AsyncLoadPipeline.h"
#include "Core/Logger.h"

namespace gx
{

// ============================================================================
// コンストラクタ / デストラクタ
// ============================================================================

AsyncLoadPipeline::AsyncLoadPipeline()
{
    m_ioThread = std::thread(&AsyncLoadPipeline::IOWorkerLoop, this);
    m_parseThread = std::thread(&AsyncLoadPipeline::ParseWorkerLoop, this);
}

AsyncLoadPipeline::~AsyncLoadPipeline()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_running.store(false);
    }
    m_ioCv.notify_all();
    m_parseCv.notify_all();

    if (m_ioThread.joinable()) m_ioThread.join();
    if (m_parseThread.joinable()) m_parseThread.join();
}

// ============================================================================
// Submit
// ============================================================================

uint32_t AsyncLoadPipeline::Submit(const gx::String& path,
                                    ParseFunc parseFunc,
                                    IntegrateFunc integrateFunc,
                                    LoadPriority priority)
{
    auto task = std::make_shared<LoadTask>();
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        task->id = m_nextTaskId++;
        task->path = path;
        task->parseFunc = std::move(parseFunc);
        task->integrateFunc = std::move(integrateFunc);
        task->priority = priority;
        task->status = PipelineLoadStatus::Pending;
        m_tasks[task->id] = task;
    }

    EnqueueForIO(task);
    return task->id;
}

// ============================================================================
// SubmitAfter
// ============================================================================

uint32_t AsyncLoadPipeline::SubmitAfter(uint32_t dependsOn,
                                         const gx::String& path,
                                         ParseFunc parseFunc,
                                         IntegrateFunc integrateFunc,
                                         LoadPriority priority)
{
    auto task = std::make_shared<LoadTask>();
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        task->id = m_nextTaskId++;
        task->path = path;
        task->parseFunc = std::move(parseFunc);
        task->integrateFunc = std::move(integrateFunc);
        task->priority = priority;
        task->status = PipelineLoadStatus::Pending;
        task->dependsOn = dependsOn;
        m_tasks[task->id] = task;

        // 先行タスクがまだ完了していない場合はpendingに入れる
        auto depIt = m_tasks.find(dependsOn);
        if (depIt != m_tasks.end() &&
            depIt->second->status != PipelineLoadStatus::Complete &&
            depIt->second->status != PipelineLoadStatus::Error &&
            depIt->second->status != PipelineLoadStatus::Cancelled)
        {
            m_pendingDeps.push_back(task);
            return task->id;
        }
    }

    // 先行タスクが既に完了している場合は即座にIOキューへ
    EnqueueForIO(task);
    return task->id;
}

// ============================================================================
// SubmitBatch
// ============================================================================

BatchHandle AsyncLoadPipeline::SubmitBatch(const gx::Vector<gx::String>& paths,
                                            ParseFunc parseFunc,
                                            std::function<void()> onAllComplete,
                                            LoadPriority priority)
{
    BatchHandle handle;
    if (paths.empty())
        return handle;

    BatchInfo batch;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        batch.id = m_nextBatchId++;
        handle.id = batch.id;
        batch.onAllComplete = std::move(onAllComplete);
    }

    for (const auto& path : paths)
    {
        auto task = std::make_shared<LoadTask>();
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            task->id = m_nextTaskId++;
            task->path = path;
            task->parseFunc = parseFunc;
            task->priority = priority;
            task->status = PipelineLoadStatus::Pending;
            task->batchId = batch.id;
            m_tasks[task->id] = task;
            batch.taskIds.push_back(task->id);
        }
        EnqueueForIO(task);
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_batches[batch.id] = std::move(batch);
    }

    return handle;
}

// ============================================================================
// EnqueueForIO — 優先度降順でIOキューに挿入
// ============================================================================

void AsyncLoadPipeline::EnqueueForIO(std::shared_ptr<LoadTask> task)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        task->status = PipelineLoadStatus::Pending;

        // 優先度降順で挿入（Critical が先頭）
        auto it = m_ioQueue.begin();
        while (it != m_ioQueue.end() &&
               static_cast<uint8_t>((*it)->priority) >= static_cast<uint8_t>(task->priority))
        {
            ++it;
        }
        m_ioQueue.insert(it, std::move(task));
    }
    m_ioCv.notify_one();
}

// ============================================================================
// IOWorkerLoop
// ============================================================================

void AsyncLoadPipeline::IOWorkerLoop()
{
    while (true)
    {
        std::shared_ptr<LoadTask> task;

        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_ioCv.wait(lock, [this]() {
                return !m_running.load() || !m_ioQueue.empty();
            });

            if (!m_running.load() && m_ioQueue.empty())
                return;

            if (m_ioQueue.empty())
                continue;

            task = std::move(m_ioQueue.front());
            m_ioQueue.erase(m_ioQueue.begin());
            task->status = PipelineLoadStatus::IO;
        }

        if (task->cancelled.load())
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            task->status = PipelineLoadStatus::Cancelled;
            continue;
        }

        // IO: VFS読み込み（ロックの外で実行）
        task->ioResult = FileSystem::Instance().ReadFile(task->path);

        if (!task->ioResult.IsValid())
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            task->status = PipelineLoadStatus::Error;
            // バッチ/依存の解決のためintegrateキューに入れる
            m_integrateQueue.push_back(task);
            continue;
        }

        // Parseキューへ
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_parseQueue.push_back(std::move(task));
        }
        m_parseCv.notify_one();
    }
}

// ============================================================================
// ParseWorkerLoop
// ============================================================================

void AsyncLoadPipeline::ParseWorkerLoop()
{
    while (true)
    {
        std::shared_ptr<LoadTask> task;

        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_parseCv.wait(lock, [this]() {
                return !m_running.load() || !m_parseQueue.empty();
            });

            if (!m_running.load() && m_parseQueue.empty())
                return;

            if (m_parseQueue.empty())
                continue;

            task = std::move(m_parseQueue.front());
            m_parseQueue.erase(m_parseQueue.begin());
            task->status = PipelineLoadStatus::Parsing;
        }

        if (task->cancelled.load())
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            task->status = PipelineLoadStatus::Cancelled;
            continue;
        }

        // Parse: 展開（ロックの外で実行）
        if (task->parseFunc)
        {
            try
            {
                task->parseResult = task->parseFunc(task->ioResult);
            }
            catch (...)
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                task->status = PipelineLoadStatus::Error;
                m_integrateQueue.push_back(task);
                continue;
            }
        }

        // IOデータはもう不要なので解放
        task->ioResult.data.clear();

        // Integrateキューへ
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            task->status = PipelineLoadStatus::WaitIntegrate;
            m_integrateQueue.push_back(std::move(task));
        }
    }
}

// ============================================================================
// Update — メインスレッドで毎フレーム呼ぶ
// ============================================================================

void AsyncLoadPipeline::Update()
{
    gx::Vector<std::shared_ptr<LoadTask>> toProcess;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        toProcess.swap(m_integrateQueue);
    }

    for (auto& task : toProcess)
    {
        if (task->cancelled.load())
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            task->status = PipelineLoadStatus::Cancelled;
            continue;
        }

        // Integrate: メインスレッドで実行
        if (task->status == PipelineLoadStatus::WaitIntegrate && task->integrateFunc)
        {
            task->integrateFunc(task->parseResult);
        }

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (task->status != PipelineLoadStatus::Error)
                task->status = PipelineLoadStatus::Complete;

            // バッチ完了チェック
            if (task->batchId != 0)
            {
                auto batchIt = m_batches.find(task->batchId);
                if (batchIt != m_batches.end())
                {
                    batchIt->second.completedCount++;
                    if (!batchIt->second.fired &&
                        batchIt->second.completedCount >= batchIt->second.taskIds.size())
                    {
                        batchIt->second.fired = true;
                        if (batchIt->second.onAllComplete)
                            batchIt->second.onAllComplete();
                    }
                }
            }

            // 依存待ちタスクの解放
            CheckPendingDependencies();
        }
    }
}

// ============================================================================
// CheckPendingDependencies (ロック取得済みで呼ぶ)
// ============================================================================

void AsyncLoadPipeline::CheckPendingDependencies()
{
    gx::Vector<std::shared_ptr<LoadTask>> ready;

    auto it = m_pendingDeps.begin();
    while (it != m_pendingDeps.end())
    {
        auto& task = *it;
        auto depIt = m_tasks.find(task->dependsOn);
        if (depIt == m_tasks.end() ||
            depIt->second->status == PipelineLoadStatus::Complete ||
            depIt->second->status == PipelineLoadStatus::Error ||
            depIt->second->status == PipelineLoadStatus::Cancelled)
        {
            ready.push_back(task);
            it = m_pendingDeps.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // IOキューに追加（ロックはすでに保持している → 直接操作）
    for (auto& task : ready)
    {
        auto insertIt = m_ioQueue.begin();
        while (insertIt != m_ioQueue.end() &&
               static_cast<uint8_t>((*insertIt)->priority) >= static_cast<uint8_t>(task->priority))
        {
            ++insertIt;
        }
        m_ioQueue.insert(insertIt, std::move(task));
    }

    if (!ready.empty())
        m_ioCv.notify_all();
}

// ============================================================================
// GetStatus
// ============================================================================

PipelineLoadStatus AsyncLoadPipeline::GetStatus(uint32_t taskId) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_tasks.find(taskId);
    if (it != m_tasks.end())
        return it->second->status;
    return PipelineLoadStatus::Error;
}

// ============================================================================
// GetProgress
// ============================================================================

float AsyncLoadPipeline::GetProgress(BatchHandle batch) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_batches.find(batch.id);
    if (it == m_batches.end() || it->second.taskIds.empty())
        return 0.0f;

    uint32_t completed = 0;
    for (uint32_t taskId : it->second.taskIds)
    {
        auto taskIt = m_tasks.find(taskId);
        if (taskIt != m_tasks.end())
        {
            auto s = taskIt->second->status;
            if (s == PipelineLoadStatus::Complete ||
                s == PipelineLoadStatus::Error ||
                s == PipelineLoadStatus::Cancelled)
            {
                ++completed;
            }
        }
    }

    return static_cast<float>(completed) / static_cast<float>(it->second.taskIds.size());
}

// ============================================================================
// Cancel
// ============================================================================

void AsyncLoadPipeline::Cancel(uint32_t taskId)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_tasks.find(taskId);
    if (it != m_tasks.end())
    {
        it->second->cancelled.store(true);
        // まだIOキューにいる場合は除去
        auto qIt = std::find_if(m_ioQueue.begin(), m_ioQueue.end(),
                                [taskId](const auto& t) { return t->id == taskId; });
        if (qIt != m_ioQueue.end())
        {
            (*qIt)->status = PipelineLoadStatus::Cancelled;
            m_ioQueue.erase(qIt);
        }
    }
}

// ============================================================================
// CancelAll
// ============================================================================

void AsyncLoadPipeline::CancelAll()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& [id, task] : m_tasks)
    {
        task->cancelled.store(true);
        if (task->status != PipelineLoadStatus::Complete)
            task->status = PipelineLoadStatus::Cancelled;
    }
    m_ioQueue.clear();
    m_parseQueue.clear();
    m_pendingDeps.clear();
}

// ============================================================================
// GetActiveCount
// ============================================================================

uint32_t AsyncLoadPipeline::GetActiveCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    uint32_t count = 0;
    for (const auto& [id, task] : m_tasks)
    {
        auto s = task->status;
        if (s != PipelineLoadStatus::Complete &&
            s != PipelineLoadStatus::Error &&
            s != PipelineLoadStatus::Cancelled)
        {
            ++count;
        }
    }
    return count;
}

} // namespace gx
