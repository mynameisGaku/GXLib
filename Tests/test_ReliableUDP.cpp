/// @file test_ReliableUDP.cpp
/// @brief ReliableChannel、ReplicatedProperty、NetworkReplicator、NetworkPredictionの単体テスト

#include <gtest/gtest.h>
#include "IO/Network/ReliableChannel.h"
#include "IO/Network/ReplicatedProperty.h"
#include "IO/Network/NetworkReplicator.h"
#include "IO/Network/NetworkPrediction.h"
#include "Core/Scene/Entity.h"

using namespace gx;

// ============================================================================
// ReliablePacketHeader
// ============================================================================

TEST(ReliablePacketHeader, DefaultValues)
{
    ReliablePacketHeader header;
    EXPECT_EQ(header.magic, 0x47585250u);
    EXPECT_EQ(header.type, 0u);
    EXPECT_EQ(header.size, 0u);
    EXPECT_EQ(header.sequence, 0u);
    EXPECT_EQ(header.ack, 0u);
    EXPECT_EQ(header.ackBits, 0u);
}

TEST(ReliablePacketHeader, Size)
{
    EXPECT_EQ(sizeof(ReliablePacketHeader), 16u);
}

// ============================================================================
// ReliableConfig
// ============================================================================

TEST(ReliableConfig, DefaultValues)
{
    ReliableConfig config;
    EXPECT_FLOAT_EQ(config.retransmitTimeMs, 100.0f);
    EXPECT_FLOAT_EQ(config.maxRetransmitTimeMs, 1000.0f);
    EXPECT_EQ(config.maxRetransmits, 10u);
    EXPECT_EQ(config.sequenceBufferSize, 1024u);
}

// ============================================================================
// ReliableChannel
// ============================================================================

TEST(ReliableChannel, DefaultRTT)
{
    ReliableChannel channel;
    EXPECT_FLOAT_EQ(channel.GetRTT(), 0.0f);
}

TEST(ReliableChannel, DefaultPacketLoss)
{
    ReliableChannel channel;
    EXPECT_FLOAT_EQ(channel.GetPacketLoss(), 0.0f);
}

TEST(ReliableChannel, DefaultCounts)
{
    ReliableChannel channel;
    EXPECT_EQ(channel.GetSentCount(), 0u);
    EXPECT_EQ(channel.GetReceivedCount(), 0u);
}

TEST(ReliableChannel, InitializeWithNullSocket)
{
    ReliableChannel channel;
    channel.Initialize(nullptr);
    // クラッシュせず、メトリクスはデフォルト値のまま
    EXPECT_FLOAT_EQ(channel.GetRTT(), 0.0f);
}

TEST(ReliableChannel, UpdateWithoutInit)
{
    ReliableChannel channel;
    channel.Update(16.0f); // クラッシュしないことを確認
    EXPECT_FLOAT_EQ(channel.GetRTT(), 0.0f);
}

// ============================================================================
// ReplicatedProperty
// ============================================================================

TEST(ReplicatedProperty, DefaultNotDirty)
{
    ReplicatedProperty<float> prop;
    EXPECT_FALSE(prop.IsDirty());
}

TEST(ReplicatedProperty, SetMarksDirty)
{
    ReplicatedProperty<float> prop(1.0f);
    prop.Set(2.0f);
    EXPECT_TRUE(prop.IsDirty());
    EXPECT_FLOAT_EQ(prop.Get(), 2.0f);
}

TEST(ReplicatedProperty, ClearDirty)
{
    ReplicatedProperty<float> prop(1.0f);
    prop.Set(2.0f);
    prop.ClearDirty();
    EXPECT_FALSE(prop.IsDirty());
}

TEST(ReplicatedProperty, SameValueNotDirty)
{
    ReplicatedProperty<int> prop(42);
    prop.Set(42);
    EXPECT_FALSE(prop.IsDirty());
}

TEST(ReplicatedProperty, Version)
{
    ReplicatedProperty<float> prop(1.0f);
    EXPECT_EQ(prop.GetVersion(), 0u);
    prop.Set(2.0f);
    EXPECT_EQ(prop.GetVersion(), 1u);
    prop.Set(3.0f);
    EXPECT_EQ(prop.GetVersion(), 2u);
}

TEST(ReplicatedProperty, GetPreviousValue)
{
    ReplicatedProperty<float> prop(10.0f);
    prop.Set(20.0f);
    EXPECT_FLOAT_EQ(prop.GetPrevious(), 10.0f);
    EXPECT_FLOAT_EQ(prop.Get(), 20.0f);
}

TEST(ReplicatedProperty, InterpolateFloat)
{
    ReplicatedProperty<float> prop(0.0f);
    float result = prop.Interpolate(10.0f, 0.5f);
    EXPECT_FLOAT_EQ(result, 5.0f);
}

TEST(ReplicatedProperty, StringInterpolateSnaps)
{
    ReplicatedProperty<gx::String> prop(gx::String("hello"));
    gx::String result = prop.Interpolate(gx::String("world"), 0.5f);
    EXPECT_EQ(result, "world");
}

TEST(ReplicatedProperty, BoolInterpolateSnaps)
{
    ReplicatedProperty<bool> prop(false);
    bool result = prop.Interpolate(true, 0.5f);
    EXPECT_TRUE(result);
}

TEST(ReplicatedProperty, IntInterpolate)
{
    ReplicatedProperty<int> prop(0);
    int result = prop.Interpolate(10, 0.5f);
    // 整数補間: 0 + int(float(10-0) * 0.5) = 0 + 5 = 5
    EXPECT_EQ(result, 5);
}

// ============================================================================
// NetworkReplicator
// ============================================================================

TEST(NetworkReplicator, DefaultSnapshotRate)
{
    NetworkReplicator rep;
    EXPECT_FLOAT_EQ(rep.GetSnapshotRate(), 20.0f);
}

TEST(NetworkReplicator, SetSnapshotRate)
{
    NetworkReplicator rep;
    rep.SetSnapshotRate(60.0f);
    EXPECT_FLOAT_EQ(rep.GetSnapshotRate(), 60.0f);
}

TEST(NetworkReplicator, DefaultInterpolationDelay)
{
    NetworkReplicator rep;
    EXPECT_FLOAT_EQ(rep.GetInterpolationDelay(), 0.1f);
}

TEST(NetworkReplicator, SetInterpolationDelay)
{
    NetworkReplicator rep;
    rep.SetInterpolationDelay(0.2f);
    EXPECT_FLOAT_EQ(rep.GetInterpolationDelay(), 0.2f);
}

TEST(NetworkReplicator, DefaultRegisteredEntityCount)
{
    NetworkReplicator rep;
    EXPECT_EQ(rep.GetRegisteredEntityCount(), 0u);
}

TEST(NetworkReplicator, SerializeDeserializeSnapshot)
{
    NetworkReplicator rep;
    gx::Vector<SnapshotEntry> entries;
    SnapshotEntry e;
    e.networkId = 1;
    e.position = Vector3{1.0f, 2.0f, 3.0f};
    e.rotation = Quaternion{0.0f, 0.0f, 0.0f, 1.0f};
    e.scale = Vector3{1.0f, 1.0f, 1.0f};
    entries.push_back(e);

    auto data = rep.SerializeSnapshot(entries);
    EXPECT_FALSE(data.empty());

    auto result = rep.DeserializeSnapshot(data);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].networkId, 1u);
    EXPECT_FLOAT_EQ(result[0].position.x, 1.0f);
    EXPECT_FLOAT_EQ(result[0].position.y, 2.0f);
    EXPECT_FLOAT_EQ(result[0].position.z, 3.0f);
}

// ============================================================================
// NetworkPrediction
// ============================================================================

TEST(NetworkPrediction, DefaultPredictionError)
{
    NetworkPrediction pred;
    EXPECT_FLOAT_EQ(pred.GetPredictionError(), 0.0f);
}

TEST(NetworkPrediction, PendingInputsDefault)
{
    NetworkPrediction pred;
    EXPECT_EQ(pred.GetPendingInputCount(), 0u);
}

// ============================================================================
// InputCommand
// ============================================================================

TEST(InputCommand, DefaultValues)
{
    InputCommand cmd;
    EXPECT_EQ(cmd.sequence, 0u);
    EXPECT_FLOAT_EQ(cmd.deltaTime, 0.0f);
    EXPECT_FALSE(cmd.jump);
    EXPECT_FLOAT_EQ(cmd.moveSpeed, 0.0f);
    EXPECT_FLOAT_EQ(cmd.timestamp, 0.0f);
}

// ============================================================================
// Entity ID (via SetID/GetID)
// ============================================================================

TEST(EntityNetworkId, DefaultIdIsZero)
{
    Entity entity("test");
    EXPECT_EQ(entity.GetID(), 0u);
}

TEST(EntityNetworkId, SetAndGetId)
{
    Entity entity("test");
    entity.SetID(42);
    EXPECT_EQ(entity.GetID(), 42u);
}
