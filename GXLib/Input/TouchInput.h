#pragma once
/// @file TouchInput.h
/// @brief タッチ入力とジェスチャー検出
///
/// Win32のWM_TOUCHメッセージを受け取り、マルチタッチの座標・フェーズを管理する。
/// タップ、ダブルタップ、長押し、スワイプ、ピンチ、回転、パンの7種のジェスチャーを検出する。
/// スマートフォン向けUIやタッチパネルゲームに使う。
/// @addtogroup grp_input/// @{

#include "pch_common.h"

namespace gx
{

/// @brief Phase of a touch point lifecycle
enum class TouchPhase
{
    Began,       ///< Touch just started
    Moved,       ///< Touch position changed
    Stationary,  ///< Touch is active but not moving
    Ended,       ///< Touch was released
    Cancelled    ///< Touch was cancelled (e.g., system interrupt)
};

/// @brief Data for a single touch point
struct TouchPoint
{
    uint32_t id = 0;
    TouchPhase phase = TouchPhase::Began;
    float x = 0.0f, y = 0.0f;             ///< Current position
    float previousX = 0.0f, previousY = 0.0f;  ///< Position last frame
    float deltaX = 0.0f, deltaY = 0.0f;    ///< Position change this frame
    float pressure = 1.0f;                  ///< Pressure (0-1)
    float radius = 1.0f;                    ///< Touch radius
    double timestamp = 0.0;                 ///< Time when this event occurred (seconds)
    int tapCount = 0;                       ///< Number of taps at this location
};

/// @brief Type of recognized gesture
enum class GestureType
{
    None, Tap, DoubleTap, LongPress, Swipe, Pinch, Rotate, Pan
};

/// @brief Direction of a swipe gesture
enum class SwipeDirection
{
    Up, Down, Left, Right
};

/// @brief Data for a detected gesture event
struct GestureEvent
{
    GestureType type = GestureType::None;
    float x = 0.0f, y = 0.0f;              ///< Center position
    float deltaX = 0.0f, deltaY = 0.0f;    ///< Movement delta
    float scale = 1.0f;                     ///< Pinch scale factor
    float rotation = 0.0f;                  ///< Rotation angle (radians)
    float velocityX = 0.0f, velocityY = 0.0f;  ///< Velocity
    SwipeDirection swipeDir = SwipeDirection::Right;
    int touchCount = 0;
};

/// @brief Configuration for touch and gesture detection
struct TouchConfig
{
    int maxTouches = 10;
    float tapTimeout = 0.3f;          ///< Max time for a tap (seconds)
    float doubleTapTimeout = 0.5f;    ///< Max time between taps for double tap
    float longPressTime = 0.5f;       ///< Min time for long press
    float swipeMinDistance = 50.0f;   ///< Min distance for swipe (pixels)
    float swipeMaxTime = 0.5f;        ///< Max time for swipe
    float pinchMinDistance = 10.0f;   ///< Min distance change for pinch
};

/// @brief Touch input manager with multi-touch and gesture detection
class TouchInput
{
public:
    TouchInput() = default;
    ~TouchInput() = default;

    // --- Configuration ---

    /// @brief Set touch configuration
    /// @param cfg The new configuration
    void SetConfig(const TouchConfig& cfg) { m_config = cfg; }

    /// @brief Get the current configuration
    /// @return Reference to the configuration
    const TouchConfig& GetConfig() const { return m_config; }

    // --- Touch lifecycle ---

    /// @brief Register a new touch
    /// @param id Unique touch identifier
    /// @param x X position
    /// @param y Y position
    /// @param pressure Touch pressure (0-1)
    void BeginTouch(uint32_t id, float x, float y, float pressure = 1.0f);

    /// @brief Update a touch position
    /// @param id Touch identifier
    /// @param x New X position
    /// @param y New Y position
    /// @param pressure Touch pressure (0-1)
    void MoveTouch(uint32_t id, float x, float y, float pressure = 1.0f);

    /// @brief End a touch
    /// @param id Touch identifier
    void EndTouch(uint32_t id);

    /// @brief Cancel a touch
    /// @param id Touch identifier
    void CancelTouch(uint32_t id);

    /// @brief Process touch states and detect gestures
    /// @param deltaTime Frame delta time in seconds
    void Update(float deltaTime);

    // --- Query ---

    /// @brief Get the number of active touches
    /// @return Active touch count
    int GetTouchCount() const;

    /// @brief Get a touch by index
    /// @param index Touch index (0 to GetTouchCount()-1)
    /// @return Pointer to the touch point, or nullptr
    const TouchPoint* GetTouch(int index) const;

    /// @brief Get a touch by its unique ID
    /// @param id Touch identifier
    /// @return Pointer to the touch point, or nullptr
    const TouchPoint* GetTouchById(uint32_t id) const;

    /// @brief Check if any touch is active
    /// @return true if at least one touch is active
    bool IsTouching() const;

    /// @brief Get the maximum number of simultaneous touches
    /// @return Max touch count from config
    int GetMaxTouches() const { return m_config.maxTouches; }

    // --- Gestures ---

    /// @brief Get gestures detected this frame
    /// @return Vector of gesture events
    const std::vector<GestureEvent>& GetGestures() const { return m_gestures; }

    /// @brief Clear detected gestures
    void ClearGestures() { m_gestures.clear(); }

    /// @brief Detect a tap gesture for a touch that just ended
    /// @param touch The touch to check
    /// @return true if a tap was detected
    bool DetectTap(const TouchPoint& touch) const;

    /// @brief Detect a double tap gesture
    /// @param touch The touch to check
    /// @return true if a double tap was detected
    bool DetectDoubleTap(const TouchPoint& touch) const;

    /// @brief Detect a long press gesture
    /// @param touch The touch to check
    /// @return true if a long press was detected
    bool DetectLongPress(const TouchPoint& touch) const;

    /// @brief Detect a swipe gesture
    /// @param touch The touch to check
    /// @return Pair of (detected, direction)
    std::pair<bool, SwipeDirection> DetectSwipe(const TouchPoint& touch) const;

    /// @brief Detect a pinch gesture (requires 2+ touches)
    /// @return Pair of (detected, scale factor)
    std::pair<bool, float> DetectPinch() const;

    /// @brief Detect a rotation gesture (requires 2+ touches)
    /// @return Pair of (detected, rotation angle in radians)
    std::pair<bool, float> DetectRotation() const;

    /// @brief Enable or disable a gesture type
    /// @param type Gesture type
    /// @param enabled Whether to enable detection
    void SetGestureEnabled(GestureType type, bool enabled);

    /// @brief Check if a gesture type is enabled
    /// @param type Gesture type
    /// @return true if enabled
    bool IsGestureEnabled(GestureType type) const;

    /// @brief Get the centroid of all active touches
    /// @return Pair of (x, y) average position
    std::pair<float, float> GetAveragePosition() const;

    /// @brief Remove all active touches
    void Clear();

    /// @brief Callback invoked when a gesture is detected
    std::function<void(const GestureEvent&)> OnGestureDetected;

private:
    /// @brief Internal tracking info for gesture detection
    struct TouchTracker
    {
        float startX = 0.0f, startY = 0.0f;  ///< Where the touch began
        double startTime = 0.0;                ///< When the touch began
        bool longPressDetected = false;        ///< Already fired long press
        bool moved = false;                    ///< Whether the touch has moved significantly
    };

    /// @brief Record of a previous tap for double-tap detection
    struct TapRecord
    {
        float x = 0.0f, y = 0.0f;
        double time = 0.0;
    };

    TouchConfig m_config;
    std::vector<TouchPoint> m_touches;
    std::unordered_map<uint32_t, TouchTracker> m_trackers;
    std::vector<GestureEvent> m_gestures;
    double m_currentTime = 0.0;
    TapRecord m_lastTap;
    bool m_hasLastTap = false;

    // Previous frame state for pinch/rotate
    float m_prevPinchDistance = 0.0f;
    float m_prevRotationAngle = 0.0f;
    bool m_hasPrevPinchData = false;

    // Gesture enable flags (all enabled by default)
    std::unordered_map<int, bool> m_gestureEnabled;

    /// @brief Add a gesture event and invoke callback
    void EmitGesture(const GestureEvent& gesture);

    /// @brief Calculate distance between two points
    static float Distance(float x1, float y1, float x2, float y2);

    /// @brief Calculate angle between two points
    static float Angle(float x1, float y1, float x2, float y2);
};

} // namespace gx
/// @}
