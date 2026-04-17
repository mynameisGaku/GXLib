/// @file movie_player_test.cpp
/// @brief MoviePlayer state machine + getter defaults tests
/// Note: Open/Update/Decode require MF + GPU — tested here without Open.
#include <gtest/gtest.h>
#include "Movie/MoviePlayer.h"

namespace {

class MoviePlayerTest : public ::testing::Test
{
protected:
    gx::MoviePlayer player;
};

TEST_F(MoviePlayerTest, DefaultState)
{
    EXPECT_EQ(player.GetState(), gx::MovieState::Stopped);
    EXPECT_EQ(player.GetWidth(), 0u);
    EXPECT_EQ(player.GetHeight(), 0u);
    EXPECT_DOUBLE_EQ(player.GetDuration(), 0.0);
    EXPECT_DOUBLE_EQ(player.GetPosition(), 0.0);
    EXPECT_EQ(player.GetTextureHandle(), -1);
    EXPECT_FALSE(player.IsFinished());
}

TEST_F(MoviePlayerTest, PlayWithoutOpenStaysStopped)
{
    player.Play();
    EXPECT_EQ(player.GetState(), gx::MovieState::Stopped);
}

TEST_F(MoviePlayerTest, PauseWithoutOpenStaysStopped)
{
    player.Pause();
    EXPECT_EQ(player.GetState(), gx::MovieState::Stopped);
}

TEST_F(MoviePlayerTest, StopWithoutOpenStaysStopped)
{
    player.Stop();
    EXPECT_EQ(player.GetState(), gx::MovieState::Stopped);
}

TEST_F(MoviePlayerTest, CloseWithoutOpenIsNoOp)
{
    player.Close();
    EXPECT_EQ(player.GetState(), gx::MovieState::Stopped);
}

TEST_F(MoviePlayerTest, SeekWithoutOpenIsNoOp)
{
    player.Seek(5.0);
    EXPECT_DOUBLE_EQ(player.GetPosition(), 0.0);
}

TEST_F(MoviePlayerTest, MovieStateEnumValues)
{
    EXPECT_NE(static_cast<int>(gx::MovieState::Stopped),
              static_cast<int>(gx::MovieState::Playing));
    EXPECT_NE(static_cast<int>(gx::MovieState::Playing),
              static_cast<int>(gx::MovieState::Paused));
    EXPECT_NE(static_cast<int>(gx::MovieState::Stopped),
              static_cast<int>(gx::MovieState::Paused));
}

TEST_F(MoviePlayerTest, DoubleCloseIsNoOp)
{
    player.Close();
    player.Close();
    EXPECT_EQ(player.GetState(), gx::MovieState::Stopped);
}

} // namespace
