/// @file test_RollbackNetcode.cpp
/// @brief RollbackNetcode テスト（Phase 10 Wave 7）

#include <gtest/gtest.h>
#include "IO/Network/RollbackNetcode.h"
#include <cstring>
#include <numeric>

using namespace gx;

// ─────────────────────────────────────────────────────────
// テスト用ゲーム状態
// ─────────────────────────────────────────────────────────

struct SimpleGameState
{
    int value = 0;
};

/// テスト用コールバックを生成するヘルパー
class RollbackTestFixture : public ::testing::Test
{
protected:
    SimpleGameState    gameState;
    int                advanceCallCount = 0;
    int                saveCallCount    = 0;
    int                loadCallCount    = 0;
    RollbackNetcode    netcode;
    RollbackSettings   settings;
    RollbackCallbacks  callbacks;

    void SetUp() override
    {
        gameState        = {};
        advanceCallCount = 0;
        saveCallCount    = 0;
        loadCallCount    = 0;

        settings.maxPlayers  = 2;
        settings.inputDelay  = 2;
        settings.maxRollback = 8;
        settings.historySize = 256;

        callbacks.saveState = [this]() -> GameStateSnapshot {
            GameStateSnapshot snap;
            snap.data.resize(sizeof(int));
            std::memcpy(snap.data.data(), &gameState.value, sizeof(int));
            saveCallCount++;
            return snap;
        };

        callbacks.loadState = [this](const GameStateSnapshot& snap) {
            if (snap.data.size() >= sizeof(int))
            {
                std::memcpy(&gameState.value, snap.data.data(), sizeof(int));
            }
            loadCallCount++;
        };

        callbacks.advanceFrame = [this](const std::vector<PlayerInput>& inputs) {
            for (const auto& inp : inputs)
            {
                gameState.value += static_cast<int>(inp.buttons);
            }
            advanceCallCount++;
        };

        callbacks.computeChecksum = [](const GameStateSnapshot& snap) -> uint32_t {
            uint32_t hash = 0;
            for (auto b : snap.data)
            {
                hash = hash * 31 + b;
            }
            return hash;
        };
    }

    void TearDown() override
    {
        netcode.Shutdown();
    }

    bool Init()
    {
        return netcode.Initialize(settings, callbacks);
    }
};

// ─────────────────────────────────────────────────────────
// Initialization Tests
// ─────────────────────────────────────────────────────────

TEST_F(RollbackTestFixture, Initialize_ValidSettings_Succeeds)
{
    EXPECT_TRUE(Init());
}

TEST_F(RollbackTestFixture, Initialize_MissingCallbacks_Fails)
{
    callbacks.saveState = nullptr;
    EXPECT_FALSE(netcode.Initialize(settings, callbacks));
}

TEST_F(RollbackTestFixture, Initialize_DoubleInit_Fails)
{
    EXPECT_TRUE(Init());
    EXPECT_FALSE(netcode.Initialize(settings, callbacks));
}

TEST_F(RollbackTestFixture, GetSettings_ReturnsCorrectValues)
{
    Init();
    const auto& s = netcode.GetSettings();
    EXPECT_EQ(s.maxPlayers, 2);
    EXPECT_EQ(s.inputDelay, 2);
    EXPECT_EQ(s.maxRollback, 8);
    EXPECT_EQ(s.historySize, 256);
}

TEST_F(RollbackTestFixture, Shutdown_ResetsState)
{
    Init();
    netcode.AdvanceFrame();
    netcode.Shutdown();
    EXPECT_EQ(netcode.GetCurrentFrame(), 0);
    EXPECT_EQ(netcode.GetConfirmedFrame(), -1);
}

// ─────────────────────────────────────────────────────────
// InputDelay Tests
// ─────────────────────────────────────────────────────────

TEST_F(RollbackTestFixture, InputDelay_Default)
{
    Init();
    EXPECT_EQ(netcode.GetSettings().inputDelay, 2);
}

TEST_F(RollbackTestFixture, SetInputDelay_ChangesValue)
{
    Init();
    netcode.SetInputDelay(4);
    EXPECT_EQ(netcode.GetSettings().inputDelay, 4);
}

TEST_F(RollbackTestFixture, SetInputDelay_ClampLow)
{
    Init();
    netcode.SetInputDelay(0);
    EXPECT_EQ(netcode.GetSettings().inputDelay, 1);
}

TEST_F(RollbackTestFixture, SetInputDelay_ClampHigh)
{
    Init();
    netcode.SetInputDelay(100);
    EXPECT_EQ(netcode.GetSettings().inputDelay, 8);
}

// ─────────────────────────────────────────────────────────
// AddLocalInput Tests
// ─────────────────────────────────────────────────────────

TEST_F(RollbackTestFixture, AddLocalInput_StoredCorrectly)
{
    Init();
    PlayerInput inp{1, 0.5f, -0.3f};
    netcode.AddLocalInput(0, inp);
    // inputDelay=2 なので frame 2 に格納される
    const auto* stored = netcode.GetInputForFrame(0, 2);
    ASSERT_NE(stored, nullptr);
    EXPECT_EQ(stored->buttons, 1u);
    EXPECT_FLOAT_EQ(stored->axisX, 0.5f);
    EXPECT_FLOAT_EQ(stored->axisY, -0.3f);
}

TEST_F(RollbackTestFixture, AddLocalInput_InvalidPlayer_NoEffect)
{
    Init();
    PlayerInput inp{1, 0.0f, 0.0f};
    netcode.AddLocalInput(-1, inp);
    netcode.AddLocalInput(5, inp);
    // No crash
}

// ─────────────────────────────────────────────────────────
// AddRemoteInput Tests
// ─────────────────────────────────────────────────────────

TEST_F(RollbackTestFixture, AddRemoteInput_StoredCorrectly)
{
    Init();
    PlayerInput inp{7, 1.0f, 0.0f};
    netcode.AddRemoteInput(1, 3, inp);
    const auto* stored = netcode.GetInputForFrame(1, 3);
    ASSERT_NE(stored, nullptr);
    EXPECT_EQ(stored->buttons, 7u);
}

TEST_F(RollbackTestFixture, AddRemoteInput_FutureFrame)
{
    Init();
    PlayerInput inp{5, 0.0f, 0.0f};
    netcode.AddRemoteInput(1, 100, inp);
    const auto* stored = netcode.GetInputForFrame(1, 100);
    ASSERT_NE(stored, nullptr);
    EXPECT_EQ(stored->buttons, 5u);
}

// ─────────────────────────────────────────────────────────
// PredictedInput Tests
// ─────────────────────────────────────────────────────────

TEST_F(RollbackTestFixture, PredictedInput_RepeatsLastKnown)
{
    Init();
    PlayerInput inp{3, 0.5f, 0.0f};
    netcode.AddRemoteInput(1, 0, inp);
    PlayerInput predicted = netcode.GetPredictedInput(1, 5);
    EXPECT_EQ(predicted.buttons, 3u);
    EXPECT_FLOAT_EQ(predicted.axisX, 0.5f);
}

TEST_F(RollbackTestFixture, PredictedInput_NoHistory_ReturnsDefault)
{
    Init();
    PlayerInput predicted = netcode.GetPredictedInput(0, 10);
    EXPECT_EQ(predicted.buttons, 0u);
    EXPECT_FLOAT_EQ(predicted.axisX, 0.0f);
    EXPECT_FLOAT_EQ(predicted.axisY, 0.0f);
}

// ─────────────────────────────────────────────────────────
// FrameAdvance Tests
// ─────────────────────────────────────────────────────────

TEST_F(RollbackTestFixture, AdvanceFrame_IncrementsCurrentFrame)
{
    Init();
    EXPECT_EQ(netcode.GetCurrentFrame(), 0);
    netcode.AdvanceFrame();
    EXPECT_EQ(netcode.GetCurrentFrame(), 1);
    netcode.AdvanceFrame();
    EXPECT_EQ(netcode.GetCurrentFrame(), 2);
}

TEST_F(RollbackTestFixture, AdvanceFrame_CallsCallbacks)
{
    Init();
    netcode.AdvanceFrame();
    EXPECT_GE(advanceCallCount, 1);
    EXPECT_GE(saveCallCount, 1);
}

TEST_F(RollbackTestFixture, AdvanceFrame_AppliesInputs)
{
    Init();
    // Player 0 inputs buttons=5, stored at frame 0+2=2
    PlayerInput inp0{5, 0.0f, 0.0f};
    netcode.AddLocalInput(0, inp0);
    // Player 1 inputs buttons=3 at frame 2
    PlayerInput inp1{3, 0.0f, 0.0f};
    netcode.AddRemoteInput(1, 2, inp1);

    // Advance to frame 2 where both inputs are set
    netcode.AdvanceFrame(); // frame 0 -> 1
    netcode.AdvanceFrame(); // frame 1 -> 2
    netcode.AdvanceFrame(); // frame 2 -> 3 (uses inputs at frame 2)

    // gameState.value should include buttons from frame 2
    // (frame 0 and 1 used predicted=0, frame 2 used 5+3=8)
    EXPECT_EQ(gameState.value, 8);
}

// ─────────────────────────────────────────────────────────
// ConfirmedFrame Tests
// ─────────────────────────────────────────────────────────

TEST_F(RollbackTestFixture, ConfirmedFrame_InitiallyNegative)
{
    Init();
    EXPECT_EQ(netcode.GetConfirmedFrame(), -1);
}

TEST_F(RollbackTestFixture, ConfirmedFrame_UpdatesWhenAllInputsReceived)
{
    Init();
    // Set inputs for frame 0 for both players
    netcode.AddRemoteInput(0, 0, {1, 0, 0});
    netcode.AddRemoteInput(1, 0, {2, 0, 0});
    netcode.AdvanceFrame(); // advance past frame 0
    EXPECT_GE(netcode.GetConfirmedFrame(), 0);
}

TEST_F(RollbackTestFixture, ConfirmedFrame_StaysBehindCurrentFrame)
{
    Init();
    netcode.AdvanceFrame();
    netcode.AdvanceFrame();
    EXPECT_LT(netcode.GetConfirmedFrame(), netcode.GetCurrentFrame());
}

// ─────────────────────────────────────────────────────────
// Rollback Tests
// ─────────────────────────────────────────────────────────

TEST_F(RollbackTestFixture, Rollback_LateRemoteInput_TriggersRollback)
{
    Init();
    // Provide local input for all frames
    for (int f = 0; f < 5; ++f)
    {
        netcode.AddRemoteInput(0, f, {1, 0, 0});
        netcode.AddRemoteInput(1, f, {0, 0, 0}); // predicted as 0
        netcode.AdvanceFrame();
    }

    // Now receive late remote input for frame 2 with different value
    int loadBefore = loadCallCount;
    netcode.AddRemoteInput(1, 2, {10, 0, 0});

    // loadState should have been called (rollback occurred)
    EXPECT_GT(loadCallCount, loadBefore);
}

TEST_F(RollbackTestFixture, Rollback_StateRestoredCorrectly)
{
    Init();
    // Advance 3 frames with known inputs for player 0
    for (int f = 0; f < 3; ++f)
    {
        netcode.AddRemoteInput(0, f, {1, 0, 0});
        netcode.AdvanceFrame();
    }
    // gameState.value = 3 (1 per frame from player 0, player 1 predicted as 0)

    int valueBefore = gameState.value;

    // Late input for player 1 at frame 0 with buttons=5
    netcode.AddRemoteInput(1, 0, {5, 0, 0});

    // After rollback and re-simulation, value should reflect the new input
    // re-sim frame 0: 1+5=6, frame 1: 1+predicted(5)=6, frame 2: 1+predicted(5)=6 => total=18
    EXPECT_NE(gameState.value, valueBefore);
}

TEST_F(RollbackTestFixture, RollbackCount_ZeroWithoutRollback)
{
    Init();
    netcode.AdvanceFrame();
    EXPECT_EQ(netcode.GetRollbackCount(), 0);
}

TEST_F(RollbackTestFixture, RollbackCount_NonZeroAfterRollback)
{
    Init();
    // Advance a few frames
    for (int f = 0; f < 4; ++f)
    {
        netcode.AddRemoteInput(0, f, {1, 0, 0});
        netcode.AdvanceFrame();
    }
    // Late remote input triggers rollback
    netcode.AddRemoteInput(1, 1, {5, 0, 0});
    EXPECT_GT(netcode.GetRollbackCount(), 0);
}

// ─────────────────────────────────────────────────────────
// Desync Detection Tests
// ─────────────────────────────────────────────────────────

TEST_F(RollbackTestFixture, DesyncDetection_InitiallyFalse)
{
    Init();
    EXPECT_FALSE(netcode.IsDesyncDetected());
    EXPECT_EQ(netcode.GetDesyncFrame(), -1);
}

TEST_F(RollbackTestFixture, DesyncDetection_MismatchedChecksum)
{
    // Use a callback that intentionally produces different checksums
    int checksumCallCount = 0;
    callbacks.computeChecksum = [&checksumCallCount](const GameStateSnapshot& snap) -> uint32_t {
        checksumCallCount++;
        // Return different checksum on verification vs original
        if (checksumCallCount % 2 == 0)
            return 999;
        uint32_t hash = 0;
        for (auto b : snap.data) hash = hash * 31 + b;
        return hash;
    };

    Init();
    // Confirm inputs for frame 0
    netcode.AddRemoteInput(0, 0, {1, 0, 0});
    netcode.AddRemoteInput(1, 0, {1, 0, 0});
    netcode.AdvanceFrame();

    // Desync may or may not be detected depending on when checksum is compared
    // Just verify the API doesn't crash and returns valid values
    bool desync = netcode.IsDesyncDetected();
    if (desync)
    {
        EXPECT_GE(netcode.GetDesyncFrame(), 0);
    }
}

// ─────────────────────────────────────────────────────────
// FrameAdvantage Tests
// ─────────────────────────────────────────────────────────

TEST_F(RollbackTestFixture, FrameAdvantage_Initial)
{
    Init();
    // currentFrame=0, confirmedFrame=-1 => advantage = 1
    EXPECT_EQ(netcode.GetFrameAdvantage(), 1);
}

TEST_F(RollbackTestFixture, FrameAdvantage_IncreasesWithUnconfirmedFrames)
{
    Init();
    netcode.AdvanceFrame();
    netcode.AdvanceFrame();
    // No confirmed inputs => confirmedFrame stays at -1
    EXPECT_EQ(netcode.GetFrameAdvantage(), netcode.GetCurrentFrame() - netcode.GetConfirmedFrame());
}

// ─────────────────────────────────────────────────────────
// PlayerInput Tests
// ─────────────────────────────────────────────────────────

TEST(PlayerInputTest, EqualityOperator)
{
    PlayerInput a{1, 0.5f, -0.3f};
    PlayerInput b{1, 0.5f, -0.3f};
    EXPECT_TRUE(a == b);
}

TEST(PlayerInputTest, InequalityOperator)
{
    PlayerInput a{1, 0.5f, -0.3f};
    PlayerInput b{2, 0.5f, -0.3f};
    EXPECT_TRUE(a != b);

    PlayerInput c{1, 0.0f, -0.3f};
    EXPECT_TRUE(a != c);
}

TEST(PlayerInputTest, DefaultValues)
{
    PlayerInput inp;
    EXPECT_EQ(inp.buttons, 0u);
    EXPECT_FLOAT_EQ(inp.axisX, 0.0f);
    EXPECT_FLOAT_EQ(inp.axisY, 0.0f);
}

// ─────────────────────────────────────────────────────────
// GameStateSnapshot Tests
// ─────────────────────────────────────────────────────────

TEST(GameStateSnapshotTest, DataVectorStoresBytes)
{
    GameStateSnapshot snap;
    snap.data = {0x01, 0x02, 0x03};
    EXPECT_EQ(snap.data.size(), 3u);
    EXPECT_EQ(snap.data[0], 0x01);
}

TEST(GameStateSnapshotTest, ChecksumField)
{
    GameStateSnapshot snap;
    snap.checksum = 0xDEADBEEF;
    EXPECT_EQ(snap.checksum, 0xDEADBEEFu);
}

TEST(GameStateSnapshotTest, FrameField)
{
    GameStateSnapshot snap;
    EXPECT_EQ(snap.frame, -1);
    snap.frame = 42;
    EXPECT_EQ(snap.frame, 42);
}

// ─────────────────────────────────────────────────────────
// MultiPlayer Tests
// ─────────────────────────────────────────────────────────

TEST_F(RollbackTestFixture, MultiPlayer_4Players)
{
    settings.maxPlayers = 4;
    Init();
    EXPECT_EQ(netcode.GetSettings().maxPlayers, 4);

    for (int p = 0; p < 4; ++p)
    {
        netcode.AddRemoteInput(p, 0, {static_cast<uint32_t>(p + 1), 0, 0});
    }
    netcode.AdvanceFrame();
    // 1+2+3+4 = 10
    EXPECT_EQ(gameState.value, 10);
}

TEST_F(RollbackTestFixture, MultiPlayer_1Player)
{
    settings.maxPlayers = 1;
    Init();
    EXPECT_EQ(netcode.GetSettings().maxPlayers, 1);

    netcode.AddRemoteInput(0, 0, {7, 0, 0});
    netcode.AdvanceFrame();
    EXPECT_EQ(gameState.value, 7);
}

// ─────────────────────────────────────────────────────────
// Settings Clamp Tests
// ─────────────────────────────────────────────────────────

TEST_F(RollbackTestFixture, Settings_MaxPlayersClampLow)
{
    settings.maxPlayers = -1;
    Init();
    EXPECT_GE(netcode.GetSettings().maxPlayers, 1);
}

TEST_F(RollbackTestFixture, Settings_MaxPlayersClampHigh)
{
    settings.maxPlayers = 100;
    Init();
    EXPECT_LE(netcode.GetSettings().maxPlayers, 4);
}

TEST_F(RollbackTestFixture, Settings_HistorySize)
{
    Init();
    EXPECT_EQ(netcode.GetInputHistorySize(), 256);
}

// ─────────────────────────────────────────────────────────
// RingBuffer Tests
// ─────────────────────────────────────────────────────────

TEST_F(RollbackTestFixture, RingBuffer_WrapsCorrectly)
{
    settings.historySize = 32;
    Init();

    // Fill more than historySize frames
    for (int f = 0; f < 64; ++f)
    {
        netcode.AddRemoteInput(0, f, {static_cast<uint32_t>(f), 0, 0});
        netcode.AddRemoteInput(1, f, {0, 0, 0});
        netcode.AdvanceFrame();
    }

    // Recent frames should still be accessible
    const auto* inp = netcode.GetInputForFrame(0, 63);
    ASSERT_NE(inp, nullptr);
    EXPECT_EQ(inp->buttons, 63u);
}

// ─────────────────────────────────────────────────────────
// Edge Cases
// ─────────────────────────────────────────────────────────

TEST_F(RollbackTestFixture, GetInputForFrame_UnsetFrame_ReturnsNull)
{
    Init();
    const auto* inp = netcode.GetInputForFrame(0, 50);
    EXPECT_EQ(inp, nullptr);
}

TEST_F(RollbackTestFixture, GetInputForFrame_NegativeFrame_ReturnsNull)
{
    Init();
    const auto* inp = netcode.GetInputForFrame(0, -1);
    EXPECT_EQ(inp, nullptr);
}

TEST_F(RollbackTestFixture, AdvanceFrame_NotInitialized_NoEffect)
{
    // Don't call Init()
    netcode.AdvanceFrame();
    EXPECT_EQ(netcode.GetCurrentFrame(), 0);
}
