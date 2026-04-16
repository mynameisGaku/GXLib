/// @file TouchInput.cpp
/// @brief タッチ入力とジェスチャー検出の実装
#include "pch_common.h"
#include "Input/TouchInput.h"

#include <cmath>
#include <algorithm>

namespace gx
{

// ============================================================================
// Touch lifecycle
// ============================================================================

void TouchInput::BeginTouch(uint32_t id, float x, float y, float pressure)
{
    // Check if we've hit the max touch limit
    int activeCount = GetTouchCount();
    if (activeCount >= m_config.maxTouches)
        return;

    // Check if this ID already exists
    for (auto& touch : m_touches)
    {
        if (touch.id == id)
            return;  // Already tracking
    }

    TouchPoint point;
    point.id = id;
    point.phase = TouchPhase::Began;
    point.x = x;
    point.y = y;
    point.previousX = x;
    point.previousY = y;
    point.deltaX = 0.0f;
    point.deltaY = 0.0f;
    point.pressure = pressure;
    point.timestamp = m_currentTime;
    point.tapCount = 0;
    m_touches.push_back(point);

    // Set up tracker for gesture detection
    TouchTracker tracker;
    tracker.startX = x;
    tracker.startY = y;
    tracker.startTime = m_currentTime;
    tracker.longPressDetected = false;
    tracker.moved = false;
    m_trackers[id] = tracker;
}

void TouchInput::MoveTouch(uint32_t id, float x, float y, float pressure)
{
    for (auto& touch : m_touches)
    {
        if (touch.id == id)
        {
            touch.previousX = touch.x;
            touch.previousY = touch.y;
            touch.x = x;
            touch.y = y;
            touch.deltaX = x - touch.previousX;
            touch.deltaY = y - touch.previousY;
            touch.pressure = pressure;
            touch.phase = TouchPhase::Moved;
            touch.timestamp = m_currentTime;

            // Update tracker movement flag
            auto trackerIt = m_trackers.find(id);
            if (trackerIt != m_trackers.end())
            {
                float dist = Distance(trackerIt->second.startX, trackerIt->second.startY, x, y);
                if (dist > 10.0f)  // Small threshold to avoid jitter
                    trackerIt->second.moved = true;
            }
            return;
        }
    }
}

void TouchInput::EndTouch(uint32_t id)
{
    for (auto& touch : m_touches)
    {
        if (touch.id == id)
        {
            touch.phase = TouchPhase::Ended;
            touch.timestamp = m_currentTime;
            return;
        }
    }
}

void TouchInput::CancelTouch(uint32_t id)
{
    for (auto& touch : m_touches)
    {
        if (touch.id == id)
        {
            touch.phase = TouchPhase::Cancelled;
            touch.timestamp = m_currentTime;
            return;
        }
    }
}

// ============================================================================
// Update
// ============================================================================

void TouchInput::Update(float deltaTime)
{
    m_currentTime += static_cast<double>(deltaTime);
    m_gestures.clear();

    // Process gesture detection for touches that have ended
    for (const auto& touch : m_touches)
    {
        if (touch.phase == TouchPhase::Ended)
        {
            auto trackerIt = m_trackers.find(touch.id);
            if (trackerIt == m_trackers.end())
                continue;

            const auto& tracker = trackerIt->second;

            // Check for swipe
            auto [swipeDetected, swipeDir] = DetectSwipe(touch);
            if (swipeDetected && IsGestureEnabled(GestureType::Swipe))
            {
                GestureEvent gesture;
                gesture.type = GestureType::Swipe;
                gesture.x = touch.x;
                gesture.y = touch.y;
                gesture.deltaX = touch.x - tracker.startX;
                gesture.deltaY = touch.y - tracker.startY;
                gesture.swipeDir = swipeDir;
                gesture.touchCount = 1;
                double duration = m_currentTime - tracker.startTime;
                if (duration > 0.0)
                {
                    gesture.velocityX = gesture.deltaX / static_cast<float>(duration);
                    gesture.velocityY = gesture.deltaY / static_cast<float>(duration);
                }
                EmitGesture(gesture);
            }
            // Check for double tap
            else if (DetectDoubleTap(touch) && IsGestureEnabled(GestureType::DoubleTap))
            {
                GestureEvent gesture;
                gesture.type = GestureType::DoubleTap;
                gesture.x = touch.x;
                gesture.y = touch.y;
                gesture.touchCount = 1;
                EmitGesture(gesture);
                m_hasLastTap = false;  // Reset so we don't triple-detect
            }
            // Check for tap
            else if (DetectTap(touch) && IsGestureEnabled(GestureType::Tap))
            {
                GestureEvent gesture;
                gesture.type = GestureType::Tap;
                gesture.x = touch.x;
                gesture.y = touch.y;
                gesture.touchCount = 1;
                EmitGesture(gesture);

                // Record for potential double tap
                m_lastTap.x = touch.x;
                m_lastTap.y = touch.y;
                m_lastTap.time = m_currentTime;
                m_hasLastTap = true;
            }
        }
    }

    // Check for long press on active touches
    for (auto& touch : m_touches)
    {
        if (touch.phase == TouchPhase::Began || touch.phase == TouchPhase::Stationary ||
            touch.phase == TouchPhase::Moved)
        {
            auto trackerIt = m_trackers.find(touch.id);
            if (trackerIt != m_trackers.end())
            {
                auto& tracker = trackerIt->second;
                double duration = m_currentTime - tracker.startTime;
                if (!tracker.longPressDetected && !tracker.moved &&
                    duration >= static_cast<double>(m_config.longPressTime) &&
                    IsGestureEnabled(GestureType::LongPress))
                {
                    tracker.longPressDetected = true;
                    GestureEvent gesture;
                    gesture.type = GestureType::LongPress;
                    gesture.x = touch.x;
                    gesture.y = touch.y;
                    gesture.touchCount = 1;
                    EmitGesture(gesture);
                }
            }
        }
    }

    // Detect pinch and rotation (requires 2+ active touches)
    int activeCount = GetTouchCount();
    if (activeCount >= 2)
    {
        // Find two active touches
        const TouchPoint* t0 = nullptr;
        const TouchPoint* t1 = nullptr;
        for (const auto& touch : m_touches)
        {
            if (touch.phase == TouchPhase::Ended || touch.phase == TouchPhase::Cancelled)
                continue;
            if (!t0) { t0 = &touch; continue; }
            if (!t1) { t1 = &touch; break; }
        }

        if (t0 && t1)
        {
            float currentDist = Distance(t0->x, t0->y, t1->x, t1->y);
            float currentAngle = Angle(t0->x, t0->y, t1->x, t1->y);

            if (m_hasPrevPinchData)
            {
                // Pinch detection
                float distDelta = currentDist - m_prevPinchDistance;
                if (std::abs(distDelta) >= m_config.pinchMinDistance &&
                    IsGestureEnabled(GestureType::Pinch))
                {
                    float scale = (m_prevPinchDistance > 0.001f) ?
                        currentDist / m_prevPinchDistance : 1.0f;
                    GestureEvent gesture;
                    gesture.type = GestureType::Pinch;
                    gesture.x = (t0->x + t1->x) * 0.5f;
                    gesture.y = (t0->y + t1->y) * 0.5f;
                    gesture.scale = scale;
                    gesture.touchCount = 2;
                    EmitGesture(gesture);
                }

                // Rotation detection
                float angleDelta = currentAngle - m_prevRotationAngle;
                // Normalize to -PI..PI
                const float PI = 3.14159265f;
                while (angleDelta > PI) angleDelta -= 2.0f * PI;
                while (angleDelta < -PI) angleDelta += 2.0f * PI;

                if (std::abs(angleDelta) > 0.01f && IsGestureEnabled(GestureType::Rotate))
                {
                    GestureEvent gesture;
                    gesture.type = GestureType::Rotate;
                    gesture.x = (t0->x + t1->x) * 0.5f;
                    gesture.y = (t0->y + t1->y) * 0.5f;
                    gesture.rotation = angleDelta;
                    gesture.touchCount = 2;
                    EmitGesture(gesture);
                }
            }

            m_prevPinchDistance = currentDist;
            m_prevRotationAngle = currentAngle;
            m_hasPrevPinchData = true;
        }
    }
    else
    {
        m_hasPrevPinchData = false;
    }

    // Transition Began -> Stationary, and remove Ended/Cancelled touches
    for (auto& touch : m_touches)
    {
        if (touch.phase == TouchPhase::Began)
            touch.phase = TouchPhase::Stationary;
        else if (touch.phase == TouchPhase::Moved)
            touch.phase = TouchPhase::Stationary;
    }

    // Remove ended/cancelled touches and their trackers
    for (auto it = m_touches.begin(); it != m_touches.end(); )
    {
        if (it->phase == TouchPhase::Ended || it->phase == TouchPhase::Cancelled)
        {
            m_trackers.erase(it->id);
            it = m_touches.erase(it);
        }
        else
        {
            // Reset deltas for stationary touches
            it->deltaX = 0.0f;
            it->deltaY = 0.0f;
            ++it;
        }
    }
}

// ============================================================================
// Query
// ============================================================================

int TouchInput::GetTouchCount() const
{
    int count = 0;
    for (const auto& touch : m_touches)
    {
        if (touch.phase != TouchPhase::Ended && touch.phase != TouchPhase::Cancelled)
            ++count;
    }
    return count;
}

const TouchPoint* TouchInput::GetTouch(int index) const
{
    int current = 0;
    for (const auto& touch : m_touches)
    {
        if (touch.phase != TouchPhase::Ended && touch.phase != TouchPhase::Cancelled)
        {
            if (current == index)
                return &touch;
            ++current;
        }
    }
    return nullptr;
}

const TouchPoint* TouchInput::GetTouchById(uint32_t id) const
{
    for (const auto& touch : m_touches)
    {
        if (touch.id == id)
            return &touch;
    }
    return nullptr;
}

bool TouchInput::IsTouching() const
{
    return GetTouchCount() > 0;
}

// ============================================================================
// Gesture detection
// ============================================================================

bool TouchInput::DetectTap(const TouchPoint& touch) const
{
    auto trackerIt = m_trackers.find(touch.id);
    if (trackerIt == m_trackers.end())
        return false;

    const auto& tracker = trackerIt->second;

    // Must not have moved
    if (tracker.moved)
        return false;

    // Must be within tap timeout
    double duration = m_currentTime - tracker.startTime;
    if (duration > static_cast<double>(m_config.tapTimeout))
        return false;

    return true;
}

bool TouchInput::DetectDoubleTap(const TouchPoint& touch) const
{
    if (!m_hasLastTap)
        return false;

    if (!DetectTap(touch))
        return false;

    // Check time between taps
    double timeSinceLastTap = m_currentTime - m_lastTap.time;
    if (timeSinceLastTap > static_cast<double>(m_config.doubleTapTimeout))
        return false;

    // Check distance between taps
    float dist = Distance(touch.x, touch.y, m_lastTap.x, m_lastTap.y);
    if (dist > 30.0f)  // Tolerance for double tap position
        return false;

    return true;
}

bool TouchInput::DetectLongPress(const TouchPoint& touch) const
{
    auto trackerIt = m_trackers.find(touch.id);
    if (trackerIt == m_trackers.end())
        return false;

    const auto& tracker = trackerIt->second;

    if (tracker.moved)
        return false;

    double duration = m_currentTime - tracker.startTime;
    return duration >= static_cast<double>(m_config.longPressTime);
}

std::pair<bool, SwipeDirection> TouchInput::DetectSwipe(const TouchPoint& touch) const
{
    auto trackerIt = m_trackers.find(touch.id);
    if (trackerIt == m_trackers.end())
        return { false, SwipeDirection::Right };

    const auto& tracker = trackerIt->second;

    // Check duration
    double duration = m_currentTime - tracker.startTime;
    if (duration > static_cast<double>(m_config.swipeMaxTime))
        return { false, SwipeDirection::Right };

    float dx = touch.x - tracker.startX;
    float dy = touch.y - tracker.startY;
    float dist = Distance(tracker.startX, tracker.startY, touch.x, touch.y);

    if (dist < m_config.swipeMinDistance)
        return { false, SwipeDirection::Right };

    // Determine direction based on dominant axis
    SwipeDirection dir;
    if (std::abs(dx) > std::abs(dy))
    {
        dir = (dx > 0) ? SwipeDirection::Right : SwipeDirection::Left;
    }
    else
    {
        dir = (dy > 0) ? SwipeDirection::Down : SwipeDirection::Up;
    }

    return { true, dir };
}

std::pair<bool, float> TouchInput::DetectPinch() const
{
    if (!m_hasPrevPinchData)
        return { false, 1.0f };

    // Need at least 2 active touches
    const TouchPoint* t0 = nullptr;
    const TouchPoint* t1 = nullptr;
    for (const auto& touch : m_touches)
    {
        if (touch.phase == TouchPhase::Ended || touch.phase == TouchPhase::Cancelled)
            continue;
        if (!t0) { t0 = &touch; continue; }
        if (!t1) { t1 = &touch; break; }
    }

    if (!t0 || !t1)
        return { false, 1.0f };

    float currentDist = Distance(t0->x, t0->y, t1->x, t1->y);
    if (m_prevPinchDistance < 0.001f)
        return { false, 1.0f };

    float scale = currentDist / m_prevPinchDistance;
    float distDelta = std::abs(currentDist - m_prevPinchDistance);

    if (distDelta < m_config.pinchMinDistance)
        return { false, 1.0f };

    return { true, scale };
}

std::pair<bool, float> TouchInput::DetectRotation() const
{
    if (!m_hasPrevPinchData)
        return { false, 0.0f };

    const TouchPoint* t0 = nullptr;
    const TouchPoint* t1 = nullptr;
    for (const auto& touch : m_touches)
    {
        if (touch.phase == TouchPhase::Ended || touch.phase == TouchPhase::Cancelled)
            continue;
        if (!t0) { t0 = &touch; continue; }
        if (!t1) { t1 = &touch; break; }
    }

    if (!t0 || !t1)
        return { false, 0.0f };

    float currentAngle = Angle(t0->x, t0->y, t1->x, t1->y);
    float angleDelta = currentAngle - m_prevRotationAngle;

    // Normalize
    const float PI = 3.14159265f;
    while (angleDelta > PI) angleDelta -= 2.0f * PI;
    while (angleDelta < -PI) angleDelta += 2.0f * PI;

    if (std::abs(angleDelta) < 0.01f)
        return { false, 0.0f };

    return { true, angleDelta };
}

void TouchInput::SetGestureEnabled(GestureType type, bool enabled)
{
    m_gestureEnabled[static_cast<int>(type)] = enabled;
}

bool TouchInput::IsGestureEnabled(GestureType type) const
{
    auto it = m_gestureEnabled.find(static_cast<int>(type));
    if (it == m_gestureEnabled.end())
        return true;  // Enabled by default
    return it->second;
}

std::pair<float, float> TouchInput::GetAveragePosition() const
{
    float sumX = 0.0f, sumY = 0.0f;
    int count = 0;
    for (const auto& touch : m_touches)
    {
        if (touch.phase != TouchPhase::Ended && touch.phase != TouchPhase::Cancelled)
        {
            sumX += touch.x;
            sumY += touch.y;
            ++count;
        }
    }

    if (count == 0)
        return { 0.0f, 0.0f };

    return { sumX / static_cast<float>(count), sumY / static_cast<float>(count) };
}

void TouchInput::Clear()
{
    m_touches.clear();
    m_trackers.clear();
    m_gestures.clear();
    m_hasPrevPinchData = false;
    m_hasLastTap = false;
}

// ============================================================================
// Internal helpers
// ============================================================================

void TouchInput::EmitGesture(const GestureEvent& gesture)
{
    m_gestures.push_back(gesture);
    if (OnGestureDetected)
        OnGestureDetected(gesture);
}

float TouchInput::Distance(float x1, float y1, float x2, float y2)
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    return std::sqrt(dx * dx + dy * dy);
}

float TouchInput::Angle(float x1, float y1, float x2, float y2)
{
    return std::atan2(y2 - y1, x2 - x1);
}

} // namespace gx
