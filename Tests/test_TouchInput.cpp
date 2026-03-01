/// @file test_TouchInput.cpp
/// @brief Touch input and gesture detection unit tests

#include "pch.h"
#include <gtest/gtest.h>
#include "Input/TouchInput.h"

using namespace gx;

// ============================================================================
// Configuration
// ============================================================================

TEST(TouchInputTest, DefaultConfig)
{
    TouchInput touch;
    const auto& cfg = touch.GetConfig();
    EXPECT_EQ(cfg.maxTouches, 10);
    EXPECT_FLOAT_EQ(cfg.tapTimeout, 0.3f);
    EXPECT_FLOAT_EQ(cfg.doubleTapTimeout, 0.5f);
    EXPECT_FLOAT_EQ(cfg.longPressTime, 0.5f);
    EXPECT_FLOAT_EQ(cfg.swipeMinDistance, 50.0f);
}

TEST(TouchInputTest, SetConfig)
{
    TouchInput touch;
    TouchConfig cfg;
    cfg.maxTouches = 5;
    cfg.tapTimeout = 0.2f;
    touch.SetConfig(cfg);

    EXPECT_EQ(touch.GetConfig().maxTouches, 5);
    EXPECT_FLOAT_EQ(touch.GetConfig().tapTimeout, 0.2f);
}

// ============================================================================
// Touch lifecycle
// ============================================================================

TEST(TouchInputTest, BeginTouch)
{
    TouchInput touch;
    touch.BeginTouch(1, 100.0f, 200.0f, 0.8f);

    EXPECT_EQ(touch.GetTouchCount(), 1);
    const TouchPoint* tp = touch.GetTouchById(1);
    ASSERT_NE(tp, nullptr);
    EXPECT_FLOAT_EQ(tp->x, 100.0f);
    EXPECT_FLOAT_EQ(tp->y, 200.0f);
    EXPECT_FLOAT_EQ(tp->pressure, 0.8f);
    EXPECT_EQ(tp->phase, TouchPhase::Began);
}

TEST(TouchInputTest, MoveTouch)
{
    TouchInput touch;
    touch.BeginTouch(1, 100.0f, 200.0f);
    touch.MoveTouch(1, 150.0f, 250.0f);

    const TouchPoint* tp = touch.GetTouchById(1);
    ASSERT_NE(tp, nullptr);
    EXPECT_FLOAT_EQ(tp->x, 150.0f);
    EXPECT_FLOAT_EQ(tp->y, 250.0f);
    EXPECT_FLOAT_EQ(tp->deltaX, 50.0f);
    EXPECT_FLOAT_EQ(tp->deltaY, 50.0f);
    EXPECT_EQ(tp->phase, TouchPhase::Moved);
}

TEST(TouchInputTest, EndTouch)
{
    TouchInput touch;
    touch.BeginTouch(1, 100.0f, 200.0f);
    touch.EndTouch(1);

    const TouchPoint* tp = touch.GetTouchById(1);
    ASSERT_NE(tp, nullptr);
    EXPECT_EQ(tp->phase, TouchPhase::Ended);

    // After update, ended touch should be removed
    touch.Update(0.016f);
    EXPECT_EQ(touch.GetTouchCount(), 0);
}

TEST(TouchInputTest, CancelTouch)
{
    TouchInput touch;
    touch.BeginTouch(1, 100.0f, 200.0f);
    touch.CancelTouch(1);

    const TouchPoint* tp = touch.GetTouchById(1);
    ASSERT_NE(tp, nullptr);
    EXPECT_EQ(tp->phase, TouchPhase::Cancelled);

    touch.Update(0.016f);
    EXPECT_EQ(touch.GetTouchCount(), 0);
}

// ============================================================================
// Query
// ============================================================================

TEST(TouchInputTest, TouchCount)
{
    TouchInput touch;
    EXPECT_EQ(touch.GetTouchCount(), 0);

    touch.BeginTouch(1, 10.0f, 20.0f);
    EXPECT_EQ(touch.GetTouchCount(), 1);

    touch.BeginTouch(2, 30.0f, 40.0f);
    EXPECT_EQ(touch.GetTouchCount(), 2);
}

TEST(TouchInputTest, GetTouch)
{
    TouchInput touch;
    touch.BeginTouch(1, 10.0f, 20.0f);
    touch.BeginTouch(2, 30.0f, 40.0f);

    const TouchPoint* t0 = touch.GetTouch(0);
    const TouchPoint* t1 = touch.GetTouch(1);
    ASSERT_NE(t0, nullptr);
    ASSERT_NE(t1, nullptr);
    EXPECT_FLOAT_EQ(t0->x, 10.0f);
    EXPECT_FLOAT_EQ(t1->x, 30.0f);

    EXPECT_EQ(touch.GetTouch(2), nullptr);
}

TEST(TouchInputTest, GetTouchById)
{
    TouchInput touch;
    touch.BeginTouch(42, 100.0f, 200.0f);

    const TouchPoint* tp = touch.GetTouchById(42);
    ASSERT_NE(tp, nullptr);
    EXPECT_EQ(tp->id, 42u);

    EXPECT_EQ(touch.GetTouchById(99), nullptr);
}

TEST(TouchInputTest, IsTouching)
{
    TouchInput touch;
    EXPECT_FALSE(touch.IsTouching());

    touch.BeginTouch(1, 0.0f, 0.0f);
    EXPECT_TRUE(touch.IsTouching());
}

TEST(TouchInputTest, UpdatePhases)
{
    TouchInput touch;
    touch.BeginTouch(1, 100.0f, 200.0f);
    EXPECT_EQ(touch.GetTouchById(1)->phase, TouchPhase::Began);

    // After update, Began -> Stationary
    touch.Update(0.016f);
    const TouchPoint* tp = touch.GetTouchById(1);
    ASSERT_NE(tp, nullptr);
    EXPECT_EQ(tp->phase, TouchPhase::Stationary);
}

// ============================================================================
// Gesture detection
// ============================================================================

TEST(TouchInputTest, DetectTap)
{
    TouchInput touch;
    touch.BeginTouch(1, 100.0f, 100.0f);
    // Quick tap: begin and end within tap timeout
    touch.Update(0.01f);
    touch.EndTouch(1);
    touch.Update(0.01f);

    const auto& gestures = touch.GetGestures();
    bool foundTap = false;
    for (const auto& g : gestures)
    {
        if (g.type == GestureType::Tap)
        {
            foundTap = true;
            EXPECT_NEAR(g.x, 100.0f, 1.0f);
            EXPECT_NEAR(g.y, 100.0f, 1.0f);
        }
    }
    EXPECT_TRUE(foundTap);
}

TEST(TouchInputTest, DetectDoubleTap)
{
    TouchInput touch;

    // First tap
    touch.BeginTouch(1, 100.0f, 100.0f);
    touch.Update(0.01f);
    touch.EndTouch(1);
    touch.Update(0.01f);

    // Second tap (quick, same location)
    touch.BeginTouch(2, 100.0f, 100.0f);
    touch.Update(0.01f);
    touch.EndTouch(2);
    touch.Update(0.01f);

    const auto& gestures = touch.GetGestures();
    bool foundDoubleTap = false;
    for (const auto& g : gestures)
    {
        if (g.type == GestureType::DoubleTap)
            foundDoubleTap = true;
    }
    EXPECT_TRUE(foundDoubleTap);
}

TEST(TouchInputTest, DetectLongPress)
{
    TouchInput touch;
    bool foundLongPress = false;
    // Use callback to capture gesture when it fires during an intermediate Update
    touch.OnGestureDetected = [&](const GestureEvent& g) {
        if (g.type == GestureType::LongPress)
            foundLongPress = true;
    };

    touch.BeginTouch(1, 100.0f, 100.0f);

    // Hold for longer than longPressTime (0.5s default)
    for (int i = 0; i < 40; ++i)
        touch.Update(0.016f);  // ~640ms total

    EXPECT_TRUE(foundLongPress);
}

TEST(TouchInputTest, DetectSwipeRight)
{
    TouchInput touch;
    touch.BeginTouch(1, 100.0f, 100.0f);
    touch.Update(0.01f);
    // Move far enough to the right
    touch.MoveTouch(1, 200.0f, 100.0f);
    touch.Update(0.01f);
    touch.EndTouch(1);
    touch.Update(0.01f);

    bool foundSwipe = false;
    for (const auto& g : touch.GetGestures())
    {
        if (g.type == GestureType::Swipe)
        {
            foundSwipe = true;
            EXPECT_EQ(g.swipeDir, SwipeDirection::Right);
        }
    }
    EXPECT_TRUE(foundSwipe);
}

TEST(TouchInputTest, DetectSwipeUp)
{
    TouchInput touch;
    touch.BeginTouch(1, 100.0f, 200.0f);
    touch.Update(0.01f);
    // Move far enough upward (negative Y)
    touch.MoveTouch(1, 100.0f, 100.0f);
    touch.Update(0.01f);
    touch.EndTouch(1);
    touch.Update(0.01f);

    bool foundSwipe = false;
    for (const auto& g : touch.GetGestures())
    {
        if (g.type == GestureType::Swipe)
        {
            foundSwipe = true;
            EXPECT_EQ(g.swipeDir, SwipeDirection::Up);
        }
    }
    EXPECT_TRUE(foundSwipe);
}

TEST(TouchInputTest, DetectPinch)
{
    TouchInput touch;
    // Start two fingers close together
    touch.BeginTouch(1, 100.0f, 100.0f);
    touch.BeginTouch(2, 120.0f, 100.0f);
    touch.Update(0.016f);

    // Move fingers apart (pinch out)
    touch.MoveTouch(1, 50.0f, 100.0f);
    touch.MoveTouch(2, 200.0f, 100.0f);
    touch.Update(0.016f);

    bool foundPinch = false;
    for (const auto& g : touch.GetGestures())
    {
        if (g.type == GestureType::Pinch)
        {
            foundPinch = true;
            EXPECT_GT(g.scale, 1.0f);  // Pinch out = scale > 1
        }
    }
    EXPECT_TRUE(foundPinch);
}

TEST(TouchInputTest, DetectRotation)
{
    TouchInput touch;
    // Two fingers horizontal
    touch.BeginTouch(1, 100.0f, 100.0f);
    touch.BeginTouch(2, 200.0f, 100.0f);
    touch.Update(0.016f);

    // Rotate: move to diagonal position
    touch.MoveTouch(1, 100.0f, 50.0f);
    touch.MoveTouch(2, 200.0f, 150.0f);
    touch.Update(0.016f);

    bool foundRotation = false;
    for (const auto& g : touch.GetGestures())
    {
        if (g.type == GestureType::Rotate)
        {
            foundRotation = true;
            EXPECT_NE(g.rotation, 0.0f);
        }
    }
    EXPECT_TRUE(foundRotation);
}

// ============================================================================
// Gesture enable/disable
// ============================================================================

TEST(TouchInputTest, GestureEnabled)
{
    TouchInput touch;
    // All gestures enabled by default
    EXPECT_TRUE(touch.IsGestureEnabled(GestureType::Tap));
    EXPECT_TRUE(touch.IsGestureEnabled(GestureType::Swipe));

    touch.SetGestureEnabled(GestureType::Tap, false);
    EXPECT_FALSE(touch.IsGestureEnabled(GestureType::Tap));
    EXPECT_TRUE(touch.IsGestureEnabled(GestureType::Swipe));

    touch.SetGestureEnabled(GestureType::Tap, true);
    EXPECT_TRUE(touch.IsGestureEnabled(GestureType::Tap));
}

// ============================================================================
// Average position
// ============================================================================

TEST(TouchInputTest, AveragePosition)
{
    TouchInput touch;
    touch.BeginTouch(1, 100.0f, 200.0f);
    touch.BeginTouch(2, 300.0f, 400.0f);

    auto [avgX, avgY] = touch.GetAveragePosition();
    EXPECT_FLOAT_EQ(avgX, 200.0f);
    EXPECT_FLOAT_EQ(avgY, 300.0f);
}

// ============================================================================
// Clear
// ============================================================================

TEST(TouchInputTest, ClearTouches)
{
    TouchInput touch;
    touch.BeginTouch(1, 10.0f, 20.0f);
    touch.BeginTouch(2, 30.0f, 40.0f);
    EXPECT_EQ(touch.GetTouchCount(), 2);

    touch.Clear();
    EXPECT_EQ(touch.GetTouchCount(), 0);
    EXPECT_FALSE(touch.IsTouching());
}

// ============================================================================
// Multi-touch
// ============================================================================

TEST(TouchInputTest, MultiTouch)
{
    TouchInput touch;
    touch.BeginTouch(1, 10.0f, 20.0f);
    touch.BeginTouch(2, 30.0f, 40.0f);
    touch.BeginTouch(3, 50.0f, 60.0f);

    EXPECT_EQ(touch.GetTouchCount(), 3);

    touch.EndTouch(2);
    touch.Update(0.016f);
    EXPECT_EQ(touch.GetTouchCount(), 2);

    EXPECT_NE(touch.GetTouchById(1), nullptr);
    EXPECT_EQ(touch.GetTouchById(2), nullptr);
    EXPECT_NE(touch.GetTouchById(3), nullptr);
}

// ============================================================================
// Gesture callback
// ============================================================================

TEST(TouchInputTest, GestureCallback)
{
    TouchInput touch;
    GestureType receivedType = GestureType::None;
    touch.OnGestureDetected = [&](const GestureEvent& g) {
        receivedType = g.type;
    };

    // Generate a tap
    touch.BeginTouch(1, 100.0f, 100.0f);
    touch.Update(0.01f);
    touch.EndTouch(1);
    touch.Update(0.01f);

    EXPECT_EQ(receivedType, GestureType::Tap);
}

// ============================================================================
// Touch delta
// ============================================================================

TEST(TouchInputTest, TouchDelta)
{
    TouchInput touch;
    touch.BeginTouch(1, 100.0f, 100.0f);

    // Initial delta should be 0
    EXPECT_FLOAT_EQ(touch.GetTouchById(1)->deltaX, 0.0f);
    EXPECT_FLOAT_EQ(touch.GetTouchById(1)->deltaY, 0.0f);

    // Move and check delta
    touch.MoveTouch(1, 110.0f, 115.0f);
    EXPECT_FLOAT_EQ(touch.GetTouchById(1)->deltaX, 10.0f);
    EXPECT_FLOAT_EQ(touch.GetTouchById(1)->deltaY, 15.0f);
}

// ============================================================================
// Touch pressure
// ============================================================================

TEST(TouchInputTest, TouchPressure)
{
    TouchInput touch;
    touch.BeginTouch(1, 100.0f, 100.0f, 0.5f);

    const TouchPoint* tp = touch.GetTouchById(1);
    ASSERT_NE(tp, nullptr);
    EXPECT_FLOAT_EQ(tp->pressure, 0.5f);

    // Update pressure on move
    touch.MoveTouch(1, 110.0f, 110.0f, 0.9f);
    tp = touch.GetTouchById(1);
    ASSERT_NE(tp, nullptr);
    EXPECT_FLOAT_EQ(tp->pressure, 0.9f);
}
