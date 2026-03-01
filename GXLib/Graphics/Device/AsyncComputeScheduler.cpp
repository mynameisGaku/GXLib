/// @file AsyncComputeScheduler.cpp
/// @brief 非同期コンピュートスケジューラの実装
#include "pch_graphics.h"
#include "Graphics/Device/AsyncComputeScheduler.h"
#include "Core/Logger.h"

namespace gx
{

bool AsyncComputeScheduler::Initialize(ID3D12Device* device)
{
    // device can be null for CPU-only testing
    for (auto& t : m_tasks)
    {
        t.asyncEnabled = false;
        t.executing = false;
        t.fenceValue = 0;
    }
    m_scheduledCount = 0;
    m_frameIndex = 0;
    m_initialized = true;
    return true;
}

void AsyncComputeScheduler::SetTaskAsync(AsyncComputeTask task, bool enabled)
{
    size_t idx = static_cast<size_t>(task);
    if (idx < static_cast<size_t>(AsyncComputeTask::Count))
    {
        m_tasks[idx].task = task;
        m_tasks[idx].asyncEnabled = enabled;
    }
}

bool AsyncComputeScheduler::IsTaskAsync(AsyncComputeTask task) const
{
    size_t idx = static_cast<size_t>(task);
    if (idx < static_cast<size_t>(AsyncComputeTask::Count))
        return m_tasks[idx].asyncEnabled;
    return false;
}

void AsyncComputeScheduler::BeginFrame()
{
    m_scheduledCount = 0;
    for (auto& t : m_tasks)
        t.executing = false;
}

void AsyncComputeScheduler::EndFrame()
{
    // Wait for all async tasks to complete
    for (auto& t : m_tasks)
    {
        if (t.executing)
            t.executing = false;
    }
    m_frameIndex++;
}

void AsyncComputeScheduler::ScheduleTask(AsyncComputeTask task)
{
    size_t idx = static_cast<size_t>(task);
    if (idx >= static_cast<size_t>(AsyncComputeTask::Count)) return;

    m_tasks[idx].executing = true;
    m_tasks[idx].fenceValue = m_frameIndex;
    m_scheduledCount++;
}

void AsyncComputeScheduler::WaitForTask(AsyncComputeTask task)
{
    size_t idx = static_cast<size_t>(task);
    if (idx >= static_cast<size_t>(AsyncComputeTask::Count)) return;
    m_tasks[idx].executing = false;
}

uint32_t AsyncComputeScheduler::GetAsyncEnabledCount() const
{
    uint32_t count = 0;
    for (const auto& t : m_tasks)
        if (t.asyncEnabled) count++;
    return count;
}

std::string AsyncComputeScheduler::GetTaskName(AsyncComputeTask task)
{
    switch (task)
    {
    case AsyncComputeTask::ParticleSimulation: return "ParticleSimulation";
    case AsyncComputeTask::LightCulling:       return "LightCulling";
    case AsyncComputeTask::SSAOGenerate:       return "SSAOGenerate";
    case AsyncComputeTask::HiZGenerate:        return "HiZGenerate";
    default: return "Unknown";
    }
}

void AsyncComputeScheduler::DisableAllAsync()
{
    for (auto& t : m_tasks)
        t.asyncEnabled = false;
}

void AsyncComputeScheduler::Reset()
{
    for (auto& t : m_tasks)
    {
        t.asyncEnabled = false;
        t.executing = false;
        t.fenceValue = 0;
    }
    m_scheduledCount = 0;
    m_frameIndex = 0;
}

} // namespace gx
