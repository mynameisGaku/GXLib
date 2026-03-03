/// @file ParallelRenderQueue.cpp
/// @brief 並列描画キューの実装
#include "pch_graphics.h"
#include "Graphics/ParallelRenderQueue.h"
#include "Core/Logger.h"

namespace gx
{

ParallelRenderQueue::~ParallelRenderQueue()
{
    Shutdown();
}

bool ParallelRenderQueue::Initialize(ID3D12Device* device, uint32_t workerCount)
{
    if (!device) return false;
    m_device = device;

    if (workerCount == 0)
    {
        workerCount = std::max(1u, std::thread::hardware_concurrency() - 1);
    }

    m_workerData.resize(workerCount);

    for (uint32_t i = 0; i < workerCount; ++i)
    {
        auto& wd = m_workerData[i];
        HRESULT hr = device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&wd.allocator));
        if (FAILED(hr))
        {
            GX_LOG_ERROR("ParallelRenderQueue: CreateCommandAllocator failed for worker %u", i);
            return false;
        }

        hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
            wd.allocator.Get(), nullptr, IID_PPV_ARGS(&wd.commandList));
        if (FAILED(hr))
        {
            GX_LOG_ERROR("ParallelRenderQueue: CreateCommandList failed for worker %u", i);
            return false;
        }

        // 初期状態でクローズする
        wd.commandList->Close();
    }

    m_shutdown.store(false);
    for (uint32_t i = 0; i < workerCount; ++i)
    {
        m_workers.emplace_back(&ParallelRenderQueue::WorkerThread, this);
    }

    GX_LOG_INFO("ParallelRenderQueue initialized with %u workers", workerCount);
    return true;
}

void ParallelRenderQueue::Submit(std::function<void(ID3D12GraphicsCommandList*)> recordFunc)
{
    {
        std::lock_guard<std::mutex> lock(m_taskMutex);
        m_tasks.push(std::move(recordFunc));
        m_pendingTasks.fetch_add(1);
    }
    m_taskCV.notify_one();
}

void ParallelRenderQueue::Execute(ID3D12CommandQueue* commandQueue)
{
    if (!commandQueue) return;

    // 全保留中タスクの完了を待つ
    {
        std::unique_lock<std::mutex> lock(m_completionMutex);
        m_completionCV.wait(lock, [this] { return m_pendingTasks.load() == 0; });
    }

    // 使用済みの全コマンドリストを収集する
    gx::Vector<ID3D12CommandList*> cmdLists;
    for (auto& wd : m_workerData)
    {
        if (wd.used)
        {
            cmdLists.push_back(wd.commandList.Get());
            wd.used = false;
        }
    }

    if (!cmdLists.empty())
    {
        commandQueue->ExecuteCommandLists(static_cast<UINT>(cmdLists.size()), cmdLists.data());
    }
}

void ParallelRenderQueue::Shutdown()
{
    m_shutdown.store(true);
    m_taskCV.notify_all();

    for (auto& worker : m_workers)
    {
        if (worker.joinable()) worker.join();
    }
    m_workers.clear();
    m_workerData.clear();
}

void ParallelRenderQueue::WorkerThread()
{
    // このスレッドのインデックスを見つける
    auto threadId = std::this_thread::get_id();
    int workerIndex = -1;
    for (int i = 0; i < static_cast<int>(m_workers.size()); ++i)
    {
        if (m_workers[i].get_id() == threadId)
        {
            workerIndex = i;
            break;
        }
    }
    if (workerIndex < 0) return;

    while (!m_shutdown.load())
    {
        std::function<void(ID3D12GraphicsCommandList*)> task;
        {
            std::unique_lock<std::mutex> lock(m_taskMutex);
            m_taskCV.wait(lock, [this] { return m_shutdown.load() || !m_tasks.empty(); });

            if (m_shutdown.load()) break;
            if (m_tasks.empty()) continue;

            task = std::move(m_tasks.front());
            m_tasks.pop();
        }

        if (task)
        {
            auto& wd = m_workerData[workerIndex];
            wd.allocator->Reset();
            wd.commandList->Reset(wd.allocator.Get(), nullptr);

            task(wd.commandList.Get());

            wd.commandList->Close();
            wd.used = true;

            m_pendingTasks.fetch_sub(1);
            m_completionCV.notify_all();
        }
    }
}

} // namespace gx
