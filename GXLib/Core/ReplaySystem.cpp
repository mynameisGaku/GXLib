/// @file ReplaySystem.cpp
/// @brief リプレイ録画・再生システムの実装

#include "pch_common.h"
#include "Core/ReplaySystem.h"

#include <algorithm>

namespace gx
{

// =============================================================================
// ReplayRecorder
// =============================================================================

void ReplayRecorder::StartRecording(const std::string& name)
{
    m_recording     = true;
    m_lastRecordTime = -1.0f;
    m_frames.clear();
    m_currentVars.clear();
    m_header.name       = name;
    m_header.duration   = 0.0f;
    m_header.frameCount = 0;
}

void ReplayRecorder::StopRecording()
{
    if (!m_recording)
    {
        return;
    }
    m_recording = false;

    // ヘッダーを最終更新
    m_header.frameCount = static_cast<uint32_t>(m_frames.size());
    if (!m_frames.empty())
    {
        m_header.duration = m_frames.back().timestamp - m_frames.front().timestamp;
    }
}

bool ReplayRecorder::IsRecording() const
{
    return m_recording;
}

void ReplayRecorder::RecordFrame(float timestamp, const std::vector<ReplayEntityState>& entities)
{
    if (!m_recording)
    {
        return;
    }

    // 録画間隔チェック
    if (m_lastRecordTime >= 0.0f)
    {
        float elapsed = timestamp - m_lastRecordTime;
        if (elapsed < m_recordInterval)
        {
            return; // まだ間隔に達していない
        }
    }

    // 最大フレーム数チェック
    if (m_frames.size() >= m_maxFrames)
    {
        return; // 上限に達した
    }

    ReplayFrame frame;
    frame.timestamp   = timestamp;
    frame.frameNumber = static_cast<uint32_t>(m_frames.size());
    frame.entities    = entities;
    frame.variables   = m_currentVars;

    m_frames.push_back(std::move(frame));
    m_lastRecordTime = timestamp;

    // ヘッダー更新
    m_header.frameCount = static_cast<uint32_t>(m_frames.size());
    if (m_frames.size() >= 2)
    {
        m_header.duration = m_frames.back().timestamp - m_frames.front().timestamp;
    }
}

void ReplayRecorder::SetVariable(const std::string& name, float value)
{
    m_currentVars[name] = value;
}

const ReplayHeader& ReplayRecorder::GetHeader() const
{
    return m_header;
}

const std::vector<ReplayFrame>& ReplayRecorder::GetFrames() const
{
    return m_frames;
}

size_t ReplayRecorder::GetFrameCount() const
{
    return m_frames.size();
}

float ReplayRecorder::GetDuration() const
{
    return m_header.duration;
}

void ReplayRecorder::Clear()
{
    m_frames.clear();
    m_currentVars.clear();
    m_header         = {};
    m_recording      = false;
    m_lastRecordTime = -1.0f;
}

void ReplayRecorder::SetMaxFrames(size_t max)
{
    m_maxFrames = max;
}

void ReplayRecorder::SetRecordInterval(float interval)
{
    m_recordInterval = interval;
}

// =============================================================================
// ReplayPlayer
// =============================================================================

void ReplayPlayer::SetReplay(const std::vector<ReplayFrame>& frames, const ReplayHeader& header)
{
    m_frames = frames;
    m_header = header;
    m_currentTime     = 0.0f;
    m_currentFrameIdx = 0;
    m_playing         = false;
    m_paused          = false;
}

bool ReplayPlayer::HasReplay() const
{
    return !m_frames.empty();
}

void ReplayPlayer::Play()
{
    if (m_frames.empty())
    {
        return;
    }

    if (m_paused)
    {
        // 一時停止解除
        m_paused  = false;
        m_playing = true;
        return;
    }

    m_playing         = true;
    m_paused          = false;
    m_currentFrameIdx = 0;

    // 先頭フレームのタイムスタンプから開始
    if (!m_frames.empty())
    {
        m_currentTime = m_frames.front().timestamp;
    }
}

void ReplayPlayer::Pause()
{
    if (m_playing)
    {
        m_playing = false;
        m_paused  = true;
    }
}

void ReplayPlayer::Stop()
{
    m_playing         = false;
    m_paused          = false;
    m_currentTime     = 0.0f;
    m_currentFrameIdx = 0;

    if (!m_frames.empty())
    {
        m_currentTime = m_frames.front().timestamp;
    }
}

void ReplayPlayer::Seek(float timestamp)
{
    if (m_frames.empty())
    {
        return;
    }

    // タイムスタンプをクランプ
    float minTime = m_frames.front().timestamp;
    float maxTime = m_frames.back().timestamp;

    m_currentTime = std::max(minTime, std::min(timestamp, maxTime));
    UpdateFrameIndex();
}

void ReplayPlayer::Update(float deltaTime)
{
    if (!m_playing || m_frames.empty())
    {
        return;
    }

    m_currentTime += deltaTime * m_playbackSpeed;

    float endTime = m_frames.back().timestamp;

    if (m_currentTime > endTime)
    {
        if (m_looping)
        {
            // ループ — 先頭に戻る
            float startTime = m_frames.front().timestamp;
            float duration  = endTime - startTime;
            if (duration > 0.0f)
            {
                m_currentTime = startTime + std::fmod(m_currentTime - startTime, duration);
            }
            else
            {
                m_currentTime = startTime;
            }
        }
        else
        {
            // 終了
            m_currentTime = endTime;
            m_playing     = false;
        }
    }

    UpdateFrameIndex();
}

bool ReplayPlayer::IsPlaying() const
{
    return m_playing;
}

bool ReplayPlayer::IsPaused() const
{
    return m_paused;
}

bool ReplayPlayer::IsFinished() const
{
    if (m_frames.empty())
    {
        return true;
    }
    return !m_playing && !m_paused && m_currentTime >= m_frames.back().timestamp;
}

float ReplayPlayer::GetCurrentTime() const
{
    return m_currentTime;
}

float ReplayPlayer::GetDuration() const
{
    return m_header.duration;
}

uint32_t ReplayPlayer::GetCurrentFrameIndex() const
{
    return m_currentFrameIdx;
}

float ReplayPlayer::GetPlaybackSpeed() const
{
    return m_playbackSpeed;
}

void ReplayPlayer::SetPlaybackSpeed(float speed)
{
    m_playbackSpeed = speed;
}

void ReplayPlayer::SetLooping(bool loop)
{
    m_looping = loop;
}

bool ReplayPlayer::IsLooping() const
{
    return m_looping;
}

const ReplayFrame* ReplayPlayer::GetCurrentFrame() const
{
    if (m_frames.empty())
    {
        return nullptr;
    }
    if (m_currentFrameIdx >= m_frames.size())
    {
        return nullptr;
    }
    return &m_frames[m_currentFrameIdx];
}

void ReplayPlayer::UpdateFrameIndex()
{
    if (m_frames.empty())
    {
        m_currentFrameIdx = 0;
        return;
    }

    // 現在時刻以下の最も近いフレームを線形探索
    // m_frames はタイムスタンプ順にソートされていると仮定
    uint32_t bestIdx = 0;
    for (uint32_t i = 0; i < static_cast<uint32_t>(m_frames.size()); ++i)
    {
        if (m_frames[i].timestamp <= m_currentTime)
        {
            bestIdx = i;
        }
        else
        {
            break;
        }
    }
    m_currentFrameIdx = bestIdx;
}

} // namespace gx
