/// @file TimelineEditor.cpp
/// @brief 対応する.hの実装
#include "pch_graphics.h"
#include "Editor/TimelineEditor.h"
#include "Core/Logger.h"
#include <algorithm>

namespace gx
{

// --- トラック管理 ---

uint32_t TimelineEditor::AddTrack(const std::string& name, EditorTrackType type)
{
    PushUndoState();
    EditorTrack track;
    track.name = name;
    track.type = type;
    m_tracks.push_back(std::move(track));
    return static_cast<uint32_t>(m_tracks.size() - 1);
}

void TimelineEditor::RemoveTrack(uint32_t trackIndex)
{
    if (trackIndex >= m_tracks.size()) return;
    PushUndoState();
    m_tracks.erase(m_tracks.begin() + trackIndex);
}

EditorTrack* TimelineEditor::GetTrack(uint32_t trackIndex)
{
    if (trackIndex >= m_tracks.size()) return nullptr;
    return &m_tracks[trackIndex];
}

const EditorTrack* TimelineEditor::GetTrack(uint32_t trackIndex) const
{
    if (trackIndex >= m_tracks.size()) return nullptr;
    return &m_tracks[trackIndex];
}

// --- キーフレーム操作 ---

uint32_t TimelineEditor::AddKeyframe(uint32_t trackIndex, float time, float v0, float v1, float v2)
{
    if (trackIndex >= m_tracks.size()) return UINT32_MAX;
    PushUndoState();

    TimelineKeyframe kf;
    kf.time = time;
    kf.value[0] = v0;
    kf.value[1] = v1;
    kf.value[2] = v2;

    auto& keyframes = m_tracks[trackIndex].keyframes;
    auto it = std::lower_bound(keyframes.begin(), keyframes.end(), kf,
        [](const TimelineKeyframe& a, const TimelineKeyframe& b) { return a.time < b.time; });
    auto inserted = keyframes.insert(it, kf);
    return static_cast<uint32_t>(std::distance(keyframes.begin(), inserted));
}

void TimelineEditor::RemoveKeyframe(uint32_t trackIndex, uint32_t keyframeIndex)
{
    if (trackIndex >= m_tracks.size()) return;
    auto& kfs = m_tracks[trackIndex].keyframes;
    if (keyframeIndex >= kfs.size()) return;
    PushUndoState();
    kfs.erase(kfs.begin() + keyframeIndex);
}

void TimelineEditor::MoveKeyframe(uint32_t trackIndex, uint32_t keyframeIndex, float newTime)
{
    if (trackIndex >= m_tracks.size()) return;
    auto& kfs = m_tracks[trackIndex].keyframes;
    if (keyframeIndex >= kfs.size()) return;
    PushUndoState();

    // Remove, update time, re-insert sorted
    TimelineKeyframe kf = kfs[keyframeIndex];
    kfs.erase(kfs.begin() + keyframeIndex);
    kf.time = newTime;
    auto it = std::lower_bound(kfs.begin(), kfs.end(), kf,
        [](const TimelineKeyframe& a, const TimelineKeyframe& b) { return a.time < b.time; });
    kfs.insert(it, kf);
}

void TimelineEditor::SetKeyframeValue(uint32_t trackIndex, uint32_t keyframeIndex, float v0, float v1, float v2)
{
    if (trackIndex >= m_tracks.size()) return;
    auto& kfs = m_tracks[trackIndex].keyframes;
    if (keyframeIndex >= kfs.size()) return;
    PushUndoState();
    kfs[keyframeIndex].value[0] = v0;
    kfs[keyframeIndex].value[1] = v1;
    kfs[keyframeIndex].value[2] = v2;
}

void TimelineEditor::SetKeyframeInterpolation(uint32_t trackIndex, uint32_t keyframeIndex, InterpolationMode mode)
{
    if (trackIndex >= m_tracks.size()) return;
    auto& kfs = m_tracks[trackIndex].keyframes;
    if (keyframeIndex >= kfs.size()) return;
    PushUndoState();
    kfs[keyframeIndex].interpolation = mode;
}

// --- 再生制御 ---

void TimelineEditor::Play()
{
    m_playing = true;
    m_paused = false;
}

void TimelineEditor::Pause()
{
    if (m_playing) m_paused = true;
}

void TimelineEditor::Stop()
{
    m_playing = false;
    m_paused = false;
    m_currentTime = 0.0f;
}

void TimelineEditor::SetCurrentTime(float time)
{
    m_currentTime = std::clamp(time, 0.0f, m_duration);
}

// --- デュレーション ---

void TimelineEditor::SetDuration(float duration)
{
    m_duration = std::max(0.01f, duration);
}

// --- 再生速度 ---

void TimelineEditor::SetPlaybackSpeed(float speed)
{
    m_playbackSpeed = std::max(0.0f, speed);
}

// --- トラックミュート/ロック ---

void TimelineEditor::MuteTrack(uint32_t trackIndex, bool muted)
{
    if (trackIndex >= m_tracks.size()) return;
    m_tracks[trackIndex].muted = muted;
}

void TimelineEditor::LockTrack(uint32_t trackIndex, bool locked)
{
    if (trackIndex >= m_tracks.size()) return;
    m_tracks[trackIndex].locked = locked;
}

bool TimelineEditor::IsTrackMuted(uint32_t trackIndex) const
{
    if (trackIndex >= m_tracks.size()) return false;
    return m_tracks[trackIndex].muted;
}

bool TimelineEditor::IsTrackLocked(uint32_t trackIndex) const
{
    if (trackIndex >= m_tracks.size()) return false;
    return m_tracks[trackIndex].locked;
}

// --- トラック評価 ---

bool TimelineEditor::EvaluateTrack(uint32_t trackIndex, float time, float outValue[3]) const
{
    if (trackIndex >= m_tracks.size()) return false;
    const auto& kfs = m_tracks[trackIndex].keyframes;
    if (kfs.empty()) return false;

    // Before first keyframe
    if (time <= kfs.front().time)
    {
        outValue[0] = kfs.front().value[0];
        outValue[1] = kfs.front().value[1];
        outValue[2] = kfs.front().value[2];
        return true;
    }

    // After last keyframe
    if (time >= kfs.back().time)
    {
        outValue[0] = kfs.back().value[0];
        outValue[1] = kfs.back().value[1];
        outValue[2] = kfs.back().value[2];
        return true;
    }

    // Find surrounding keyframes
    size_t lo = 0, hi = kfs.size() - 1;
    while (lo + 1 < hi)
    {
        size_t mid = (lo + hi) / 2;
        if (kfs[mid].time <= time) lo = mid; else hi = mid;
    }

    const auto& kfA = kfs[lo];
    const auto& kfB = kfs[hi];
    float dt = kfB.time - kfA.time;
    float t = (dt > 0.0f) ? (time - kfA.time) / dt : 0.0f;

    // Apply interpolation
    float mappedT = CurveEvaluator::EvaluateInterpolation(kfA.interpolation, t);

    outValue[0] = kfA.value[0] + (kfB.value[0] - kfA.value[0]) * mappedT;
    outValue[1] = kfA.value[1] + (kfB.value[1] - kfA.value[1]) * mappedT;
    outValue[2] = kfA.value[2] + (kfB.value[2] - kfA.value[2]) * mappedT;
    return true;
}

// --- コピー/ペースト ---

void TimelineEditor::CopyKeyframe(uint32_t trackIndex, uint32_t keyframeIndex)
{
    if (trackIndex >= m_tracks.size()) return;
    const auto& kfs = m_tracks[trackIndex].keyframes;
    if (keyframeIndex >= kfs.size()) return;
    m_clipboard = kfs[keyframeIndex];
}

uint32_t TimelineEditor::PasteKeyframe(uint32_t trackIndex, float time)
{
    if (!m_clipboard.has_value()) return UINT32_MAX;
    if (trackIndex >= m_tracks.size()) return UINT32_MAX;

    TimelineKeyframe kf = m_clipboard.value();
    kf.time = time;
    return AddKeyframe(trackIndex, kf.time, kf.value[0], kf.value[1], kf.value[2]);
}

// --- Undo/Redo ---

void TimelineEditor::PushUndoState()
{
    UndoState state;
    state.tracks = m_tracks;
    m_undoStack.push_back(std::move(state));
    if (m_undoStack.size() > k_MaxUndoSteps)
        m_undoStack.erase(m_undoStack.begin());
    m_redoStack.clear();
}

void TimelineEditor::Undo()
{
    if (m_undoStack.empty()) return;
    UndoState redo;
    redo.tracks = m_tracks;
    m_redoStack.push_back(std::move(redo));

    m_tracks = std::move(m_undoStack.back().tracks);
    m_undoStack.pop_back();
}

void TimelineEditor::Redo()
{
    if (m_redoStack.empty()) return;
    UndoState undo;
    undo.tracks = m_tracks;
    m_undoStack.push_back(std::move(undo));

    m_tracks = std::move(m_redoStack.back().tracks);
    m_redoStack.pop_back();
}

} // namespace gx
