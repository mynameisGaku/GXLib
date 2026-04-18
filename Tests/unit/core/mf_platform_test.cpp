/// @file mf_platform_test.cpp
/// @brief MFPlatform refcount wrapper — E4 regression tests (ADR-0020).
///
/// Added 2026-04-19 as part of sprint-002 Task 6 (deferred test hardening).
/// Covers the contract added by the 2026-04-18 E4 fix: process-wide Media
/// Foundation refcount, Acquire/Release paired transitions, idempotent
/// multi-consumer scenario (MoviePlayer + VideoRecorder cohabitation).
#include <gtest/gtest.h>
#include "Core/MFPlatform.h"

namespace {

class MFPlatformTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Ensure a clean baseline by draining any leaked refcount from
        // prior tests. Safe because GetRefCount + Release are no-op when 0.
        while (gx::MFPlatform::GetRefCount() > 0)
        {
            gx::MFPlatform::Release();
        }
    }

    void TearDown() override
    {
        // Symmetric drain — if a test leaks an Acquire, surface via this
        // teardown rather than letting state bleed into the next case.
        while (gx::MFPlatform::GetRefCount() > 0)
        {
            gx::MFPlatform::Release();
        }
    }
};

TEST_F(MFPlatformTest, StartsUninitialized)
{
    EXPECT_EQ(gx::MFPlatform::GetRefCount(), 0);
    EXPECT_FALSE(gx::MFPlatform::IsInitialized());
}

TEST_F(MFPlatformTest, AcquireInitializesOnZeroToOneTransition)
{
    // Arrange
    ASSERT_EQ(gx::MFPlatform::GetRefCount(), 0);

    // Act
    bool ok = gx::MFPlatform::Acquire();

    // Assert
    EXPECT_TRUE(ok);
    EXPECT_TRUE(gx::MFPlatform::IsInitialized());
    EXPECT_EQ(gx::MFPlatform::GetRefCount(), 1);
}

TEST_F(MFPlatformTest, ReleaseShutsDownOnOneToZeroTransition)
{
    // Arrange
    ASSERT_TRUE(gx::MFPlatform::Acquire());
    ASSERT_EQ(gx::MFPlatform::GetRefCount(), 1);

    // Act
    gx::MFPlatform::Release();

    // Assert
    EXPECT_FALSE(gx::MFPlatform::IsInitialized());
    EXPECT_EQ(gx::MFPlatform::GetRefCount(), 0);
}

TEST_F(MFPlatformTest, NestedAcquireReleasePairs)
{
    // Arrange (no-op)

    // Act — simulate MoviePlayer::Open followed by VideoRecorder::Initialize
    ASSERT_TRUE(gx::MFPlatform::Acquire());
    EXPECT_EQ(gx::MFPlatform::GetRefCount(), 1);

    ASSERT_TRUE(gx::MFPlatform::Acquire());
    EXPECT_EQ(gx::MFPlatform::GetRefCount(), 2);

    // Assert — still initialised after one Release
    gx::MFPlatform::Release();
    EXPECT_TRUE(gx::MFPlatform::IsInitialized());
    EXPECT_EQ(gx::MFPlatform::GetRefCount(), 1);

    // Assert — shuts down after matched final Release
    gx::MFPlatform::Release();
    EXPECT_FALSE(gx::MFPlatform::IsInitialized());
    EXPECT_EQ(gx::MFPlatform::GetRefCount(), 0);
}

TEST_F(MFPlatformTest, MoviePlayerVideoRecorderCohabitationScenario)
{
    // This is the E4 bug scenario: MoviePlayer (cutscene) and VideoRecorder
    // (session recording) coexist; one closes first and the other must still
    // observe a live MF platform.

    // Arrange — MoviePlayer opens a cutscene
    ASSERT_TRUE(gx::MFPlatform::Acquire());
    const int afterPlayer = gx::MFPlatform::GetRefCount();

    // Act — VideoRecorder starts recording mid-cutscene
    ASSERT_TRUE(gx::MFPlatform::Acquire());
    EXPECT_EQ(gx::MFPlatform::GetRefCount(), afterPlayer + 1);
    EXPECT_TRUE(gx::MFPlatform::IsInitialized());

    // Act — MoviePlayer closes (cutscene finishes)
    gx::MFPlatform::Release();

    // Assert — VideoRecorder's MF session is still live (was the E4 bug)
    EXPECT_TRUE(gx::MFPlatform::IsInitialized());
    EXPECT_EQ(gx::MFPlatform::GetRefCount(), afterPlayer);

    // Act — VideoRecorder shuts down
    gx::MFPlatform::Release();

    // Assert — MF platform teardown
    EXPECT_FALSE(gx::MFPlatform::IsInitialized());
    EXPECT_EQ(gx::MFPlatform::GetRefCount(), 0);
}

TEST_F(MFPlatformTest, UnbalancedReleaseIsNoOpNotCrash)
{
    // Arrange — refcount starts at 0 per SetUp

    // Act — unbalanced Release (the forbidden-pattern guard)
    gx::MFPlatform::Release(); // should log error, not crash

    // Assert — state unchanged
    EXPECT_EQ(gx::MFPlatform::GetRefCount(), 0);
    EXPECT_FALSE(gx::MFPlatform::IsInitialized());
}

TEST_F(MFPlatformTest, RecoversAfterFullTeardownCycle)
{
    // Simulates two independent MoviePlayer instances (open, close, open, close).

    // Cycle 1
    ASSERT_TRUE(gx::MFPlatform::Acquire());
    EXPECT_EQ(gx::MFPlatform::GetRefCount(), 1);
    gx::MFPlatform::Release();
    EXPECT_EQ(gx::MFPlatform::GetRefCount(), 0);

    // Cycle 2 — must re-initialise cleanly
    ASSERT_TRUE(gx::MFPlatform::Acquire());
    EXPECT_TRUE(gx::MFPlatform::IsInitialized());
    EXPECT_EQ(gx::MFPlatform::GetRefCount(), 1);
    gx::MFPlatform::Release();
}

} // namespace
