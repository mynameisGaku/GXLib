/// @file Timeline.cpp
/// @brief タイムラインの実装
#include "pch_common.h"
#include "Core/Timeline.h"

namespace gx
{

// ============================================================================
// Float3Track
// ============================================================================

void Float3Track::AddKeyframe(float time, const XMFLOAT3& value)
{
    m_keyframes.push_back({ time, value });
    std::sort(m_keyframes.begin(), m_keyframes.end(),
        [](const Keyframe& a, const Keyframe& b) { return a.time < b.time; });
}

void Float3Track::Evaluate(float time)
{
    if (m_keyframes.empty() || !m_target) return;

    // 最初のキーフレームより前
    if (time <= m_keyframes.front().time)
    {
        m_target(m_keyframes.front().value);
        return;
    }

    // 最後のキーフレームより後
    if (time >= m_keyframes.back().time)
    {
        m_target(m_keyframes.back().value);
        return;
    }

    // 前後のキーフレームを見つけて補間
    for (size_t i = 0; i + 1 < m_keyframes.size(); ++i)
    {
        if (time >= m_keyframes[i].time && time <= m_keyframes[i + 1].time)
        {
            float t = (time - m_keyframes[i].time) /
                      (m_keyframes[i + 1].time - m_keyframes[i].time);
            XMFLOAT3 result = {
                m_keyframes[i].value.x + (m_keyframes[i + 1].value.x - m_keyframes[i].value.x) * t,
                m_keyframes[i].value.y + (m_keyframes[i + 1].value.y - m_keyframes[i].value.y) * t,
                m_keyframes[i].value.z + (m_keyframes[i + 1].value.z - m_keyframes[i].value.z) * t
            };
            m_target(result);
            return;
        }
    }
}

float Float3Track::GetDuration() const
{
    return m_keyframes.empty() ? 0.0f : m_keyframes.back().time;
}

// ============================================================================
// FloatTrack
// ============================================================================

void FloatTrack::AddKeyframe(float time, float value)
{
    m_keyframes.push_back({ time, value });
    std::sort(m_keyframes.begin(), m_keyframes.end(),
        [](const Keyframe& a, const Keyframe& b) { return a.time < b.time; });
}

void FloatTrack::Evaluate(float time)
{
    if (m_keyframes.empty() || !m_target) return;

    if (time <= m_keyframes.front().time)
    {
        m_target(m_keyframes.front().value);
        return;
    }

    if (time >= m_keyframes.back().time)
    {
        m_target(m_keyframes.back().value);
        return;
    }

    for (size_t i = 0; i + 1 < m_keyframes.size(); ++i)
    {
        if (time >= m_keyframes[i].time && time <= m_keyframes[i + 1].time)
        {
            float t = (time - m_keyframes[i].time) /
                      (m_keyframes[i + 1].time - m_keyframes[i].time);
            float result = m_keyframes[i].value + (m_keyframes[i + 1].value - m_keyframes[i].value) * t;
            m_target(result);
            return;
        }
    }
}

float FloatTrack::GetDuration() const
{
    return m_keyframes.empty() ? 0.0f : m_keyframes.back().time;
}

// ============================================================================
// EventTrack
// ============================================================================

void EventTrack::AddEvent(float time, std::function<void()> callback)
{
    m_events.push_back({ time, std::move(callback), false });
    std::sort(m_events.begin(), m_events.end(),
        [](const Event& a, const Event& b) { return a.time < b.time; });
}

void EventTrack::Evaluate(float time)
{
    for (auto& event : m_events)
    {
        if (!event.fired && time >= event.time)
        {
            event.fired = true;
            if (event.callback) event.callback();
        }
    }
    m_lastTime = time;
}

float EventTrack::GetDuration() const
{
    return m_events.empty() ? 0.0f : m_events.back().time;
}

void EventTrack::ResetFired()
{
    for (auto& event : m_events)
        event.fired = false;
    m_lastTime = -1.0f;
}

// ============================================================================
// Timeline
// ============================================================================

void Timeline::AddTrack(std::unique_ptr<TimelineTrack> track)
{
    m_tracks.push_back(std::move(track));
}

void Timeline::Play()
{
    m_playing = true;
}

void Timeline::Pause()
{
    m_playing = false;
}

void Timeline::Stop()
{
    m_playing = false;
    m_currentTime = 0.0f;
    // イベントトラックをリセット
    for (auto& track : m_tracks)
    {
        if (auto* et = dynamic_cast<EventTrack*>(track.get()))
            et->ResetFired();
    }
}

void Timeline::Seek(float time)
{
    m_currentTime = std::max(0.0f, time);
    for (auto& track : m_tracks)
        track->Evaluate(m_currentTime);
}

void Timeline::Update(float deltaTime)
{
    if (!m_playing) return;

    m_currentTime += deltaTime * m_speed;
    float duration = GetDuration();

    if (m_currentTime >= duration)
    {
        if (m_loop)
        {
            // 未発火イベントを処理するため、終端で評価
            for (auto& track : m_tracks)
                track->Evaluate(duration);

            // 時間を巻き戻してイベントフラグをリセット
            m_currentTime = std::fmod(m_currentTime, std::max(duration, 0.001f));
            for (auto& track : m_tracks)
            {
                if (auto* et = dynamic_cast<EventTrack*>(track.get()))
                    et->ResetFired();
            }

            // 巻き戻し後の時間で評価
            for (auto& track : m_tracks)
                track->Evaluate(m_currentTime);
        }
        else
        {
            m_currentTime = duration;
            m_playing = false;
            for (auto& track : m_tracks)
                track->Evaluate(m_currentTime);
        }
    }
    else
    {
        for (auto& track : m_tracks)
            track->Evaluate(m_currentTime);
    }
}

float Timeline::GetDuration() const
{
    float maxDuration = 0.0f;
    for (const auto& track : m_tracks)
        maxDuration = std::max(maxDuration, track->GetDuration());
    return maxDuration;
}

} // namespace gx
