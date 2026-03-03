/// @file AudioSnapshot.cpp
/// @brief オーディオスナップショット管理の実装
#include "pch_audio.h"
#include "Audio/AudioSnapshot.h"

#include <algorithm>

namespace gx
{

// ============================================================================
// RegisterSnapshot
// ============================================================================

void AudioSnapshotManager::RegisterSnapshot(const AudioSnapshotData& snapshot)
{
    m_snapshots[snapshot.name] = snapshot;
}

// ============================================================================
// UnregisterSnapshot
// ============================================================================

void AudioSnapshotManager::UnregisterSnapshot(const gx::String& name)
{
    m_snapshots.erase(name);

    // 削除されたスナップショットが現在または遷移先の場合はリセット
    if (m_currentName == name)
    {
        m_currentName.clear();
    }
    if (m_targetName == name)
    {
        m_targetName.clear();
        m_transitioning = false;
        m_transitionElapsed = 0.0f;
        m_transitionTime = 0.0f;
    }
}

// ============================================================================
// GetSnapshot
// ============================================================================

const AudioSnapshotData* AudioSnapshotManager::GetSnapshot(const gx::String& name) const
{
    auto it = m_snapshots.find(name);
    if (it != m_snapshots.end())
    {
        return &it->second;
    }
    return nullptr;
}

// ============================================================================
// GetSnapshotCount
// ============================================================================

size_t AudioSnapshotManager::GetSnapshotCount() const
{
    return m_snapshots.size();
}

// ============================================================================
// ApplySnapshot — 即座に適用
// ============================================================================

void AudioSnapshotManager::ApplySnapshot(const gx::String& name)
{
    auto it = m_snapshots.find(name);
    if (it == m_snapshots.end())
    {
        return;
    }

    // トランジションを中断して即座に適用
    m_transitioning = false;
    m_transitionElapsed = 0.0f;
    m_transitionTime = 0.0f;
    m_targetName.clear();
    m_currentName = name;
}

// ============================================================================
// BlendToSnapshot — 時間をかけて遷移
// ============================================================================

void AudioSnapshotManager::BlendToSnapshot(const gx::String& name, float transitionTime)
{
    auto it = m_snapshots.find(name);
    if (it == m_snapshots.end())
    {
        return;
    }

    // 遷移時間が0以下の場合は即座に適用
    if (transitionTime <= 0.0f)
    {
        ApplySnapshot(name);
        return;
    }

    m_targetName = name;
    m_transitionTime = transitionTime;
    m_transitionElapsed = 0.0f;
    m_transitioning = true;
}

// ============================================================================
// Update — トランジション進行
// ============================================================================

void AudioSnapshotManager::Update(float deltaTime)
{
    if (!m_transitioning)
    {
        return;
    }

    m_transitionElapsed += deltaTime;

    // トランジション完了チェック
    if (m_transitionElapsed >= m_transitionTime)
    {
        m_transitionElapsed = m_transitionTime;
        m_currentName = m_targetName;
        m_targetName.clear();
        m_transitioning = false;
    }
}

// ============================================================================
// GetCurrentSnapshotName
// ============================================================================

gx::String AudioSnapshotManager::GetCurrentSnapshotName() const
{
    return m_currentName;
}

// ============================================================================
// IsTransitioning
// ============================================================================

bool AudioSnapshotManager::IsTransitioning() const
{
    return m_transitioning;
}

// ============================================================================
// GetTransitionProgress
// ============================================================================

float AudioSnapshotManager::GetTransitionProgress() const
{
    if (!m_transitioning)
    {
        return 1.0f;
    }

    if (m_transitionTime <= 0.0f)
    {
        return 1.0f;
    }

    float progress = m_transitionElapsed / m_transitionTime;
    return (std::min)(progress, 1.0f);
}

// ============================================================================
// Reset
// ============================================================================

void AudioSnapshotManager::Reset()
{
    m_currentName.clear();
    m_targetName.clear();
    m_transitionTime = 0.0f;
    m_transitionElapsed = 0.0f;
    m_transitioning = false;
}

} // namespace gx
