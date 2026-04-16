/// @file ParallelCommandRecorder.cpp
/// @brief マルチスレッドコマンド記録の実装
#include "pch_graphics.h"
#include "Graphics/Device/ParallelCommandRecorder.h"
#include "Graphics/Device/CommandQueue.h"
#include "Core/Logger.h"

namespace gx
{

ParallelCommandRecorder::~ParallelCommandRecorder()
{
    m_jobs.clear();
    m_workerCmdLists.clear();
}

bool ParallelCommandRecorder::Initialize(ID3D12Device* device, uint32_t workerCount)
{
    m_device = device;
    if (workerCount == 0)
    {
        uint32_t hw = std::thread::hardware_concurrency();
        m_workerCount = (hw > 1) ? (hw - 1) : 1;
    }
    else
    {
        m_workerCount = workerCount;
    }

    m_workerCmdLists.resize(m_workerCount);

    for (uint32_t i = 0; i < m_workerCount; ++i)
    {
        auto& w = m_workerCmdLists[i];

        for (uint32_t f = 0; f < 2; ++f)
        {
            HRESULT hr = device->CreateCommandAllocator(
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                IID_PPV_ARGS(&w.allocators[f]));
            if (FAILED(hr))
            {
                GX_LOG_ERROR("ParallelCommandRecorder: CreateCommandAllocator failed for worker {} frame {}", i, f);
                return false;
            }
        }

        HRESULT hr = device->CreateCommandList(0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            w.allocators[0].Get(), nullptr,
            IID_PPV_ARGS(&w.commandList));
        if (FAILED(hr))
        {
            GX_LOG_ERROR("ParallelCommandRecorder: CreateCommandList failed for worker {}", i);
            return false;
        }

        // 初回は閉じた状態にしておく（Reset前にClose必須）
        w.commandList->Close();
    }

    return true;
}

void ParallelCommandRecorder::AddRecordJob(RecordJob job)
{
    m_jobs.push_back(std::move(job));
}

void ParallelCommandRecorder::RecordAndExecute(CommandQueue& queue, uint32_t frameIndex,
                                                ID3D12PipelineState* initialPSO)
{
    if (m_jobs.empty())
        return;

    const uint32_t jobCount = static_cast<uint32_t>(m_jobs.size());
    const uint32_t usedWorkers = (std::min)(jobCount, m_workerCount);

    // ワーカーごとにジョブを割り当てて並列記録
    gx::Vector<std::thread> threads;
    threads.reserve(usedWorkers);

    for (uint32_t w = 0; w < usedWorkers; ++w)
    {
        threads.emplace_back([this, w, usedWorkers, jobCount, frameIndex, initialPSO]()
        {
            auto& wcl = m_workerCmdLists[w];
            auto* allocator = wcl.allocators[frameIndex % 2].Get();

            allocator->Reset();
            wcl.commandList->Reset(allocator, initialPSO);

            // このワーカーが担当するジョブ範囲
            uint32_t start = w * jobCount / usedWorkers;
            uint32_t end   = (w + 1) * jobCount / usedWorkers;

            for (uint32_t j = start; j < end; ++j)
                m_jobs[j](wcl.commandList.Get());

            wcl.commandList->Close();
        });
    }

    // 全ワーカーの完了を待つ
    for (auto& t : threads)
        t.join();

    // コマンドリストをまとめてキューに送信
    gx::Vector<ID3D12CommandList*> cmdLists;
    cmdLists.reserve(usedWorkers);
    for (uint32_t w = 0; w < usedWorkers; ++w)
        cmdLists.push_back(m_workerCmdLists[w].commandList.Get());

    queue.ExecuteCommandLists(cmdLists.data(), usedWorkers);

    m_jobs.clear();
}

} // namespace gx
