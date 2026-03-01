/// @file test_ClothNetworkCapture.cpp
/// @brief Phase 3: ClothSimulator, NetworkManager, ScreenCapture テスト

#include <gtest/gtest.h>
#include "Physics/ClothSimulator.h"
#include "IO/Network/NetworkManager.h"
#include "Graphics/ScreenCapture.h"

using namespace gx;
using namespace DirectX;

// ============================================================================
// ClothSimulator
// ============================================================================

TEST(ClothSimulatorTest, InitializeBasic)
{
    ClothSimulator cloth;
    ClothDesc desc;
    desc.width  = 5;
    desc.height = 5;
    desc.spacing = 0.1f;

    EXPECT_TRUE(cloth.Initialize(desc));
    EXPECT_EQ(cloth.GetVertexCount(), 25u);
    EXPECT_EQ(cloth.GetWidth(), 5u);
    EXPECT_EQ(cloth.GetHeight(), 5u);
}

TEST(ClothSimulatorTest, InitializeTooSmall)
{
    ClothSimulator cloth;
    ClothDesc desc;
    desc.width  = 1;
    desc.height = 1;
    EXPECT_FALSE(cloth.Initialize(desc));
}

TEST(ClothSimulatorTest, VertexPositionsInitialized)
{
    ClothSimulator cloth;
    ClothDesc desc;
    desc.width  = 3;
    desc.height = 3;
    desc.spacing = 1.0f;
    cloth.Initialize(desc);

    const auto& pos = cloth.GetPositions();
    EXPECT_EQ(pos.size(), 9u);

    // 最初の頂点は原点
    EXPECT_FLOAT_EQ(pos[0].x, 0.0f);
    EXPECT_FLOAT_EQ(pos[0].y, 0.0f);
    EXPECT_FLOAT_EQ(pos[0].z, 0.0f);

    // 最後の頂点は (2, 0, 2)
    EXPECT_FLOAT_EQ(pos[8].x, 2.0f);
    EXPECT_FLOAT_EQ(pos[8].z, 2.0f);
}

TEST(ClothSimulatorTest, IndicesGenerated)
{
    ClothSimulator cloth;
    ClothDesc desc;
    desc.width  = 3;
    desc.height = 3;
    cloth.Initialize(desc);

    // (width-1)*(height-1)*6 個の三角形
    EXPECT_EQ(cloth.GetIndices().size(), 2u * 2u * 6u);
}

TEST(ClothSimulatorTest, UpdateDoesNotCrash)
{
    ClothSimulator cloth;
    ClothDesc desc;
    desc.width  = 4;
    desc.height = 4;
    cloth.Initialize(desc);

    // 複数回の更新でクラッシュしないこと
    for (int i = 0; i < 10; ++i)
        cloth.Update(0.016f, 4);

    EXPECT_EQ(cloth.GetVertexCount(), 16u);
}

TEST(ClothSimulatorTest, GravityAffectsUnpinned)
{
    ClothSimulator cloth;
    ClothDesc desc;
    desc.width  = 3;
    desc.height = 3;
    desc.spacing = 0.5f;
    desc.gravity = { 0.0f, -9.8f, 0.0f };
    cloth.Initialize(desc);

    float initialY = cloth.GetPositions()[4].y; // center vertex

    cloth.Update(0.1f, 2);

    // 中心は下方向に移動しているはず
    EXPECT_LT(cloth.GetPositions()[4].y, initialY);
}

TEST(ClothSimulatorTest, PinnedVertexStays)
{
    ClothSimulator cloth;
    ClothDesc desc;
    desc.width  = 3;
    desc.height = 3;
    desc.spacing = 1.0f;
    desc.pinnedVertices = { 0, 2 }; // 左上と右上をピン留め
    cloth.Initialize(desc);

    XMFLOAT3 pin0 = cloth.GetPositions()[0];

    cloth.Update(0.1f, 4);

    // ピン留めされた頂点は動かないはず
    EXPECT_FLOAT_EQ(cloth.GetPositions()[0].x, pin0.x);
    EXPECT_FLOAT_EQ(cloth.GetPositions()[0].y, pin0.y);
    EXPECT_FLOAT_EQ(cloth.GetPositions()[0].z, pin0.z);
}

TEST(ClothSimulatorTest, PinAndUnpin)
{
    ClothSimulator cloth;
    ClothDesc desc;
    desc.width  = 3;
    desc.height = 3;
    cloth.Initialize(desc);

    cloth.PinVertex(4);
    XMFLOAT3 pos4 = cloth.GetPositions()[4];
    cloth.Update(0.1f, 2);
    EXPECT_FLOAT_EQ(cloth.GetPositions()[4].y, pos4.y);

    cloth.UnpinVertex(4);
    cloth.Update(0.1f, 2);
    EXPECT_NE(cloth.GetPositions()[4].y, pos4.y); // 移動しているはず
}

TEST(ClothSimulatorTest, SphereCollider)
{
    ClothSimulator cloth;
    ClothDesc desc;
    desc.width  = 5;
    desc.height = 5;
    desc.spacing = 0.2f;
    desc.gravity = { 0.0f, -9.8f, 0.0f };
    cloth.Initialize(desc);

    cloth.AddSphereCollider({ 0.4f, -0.5f, 0.4f }, 0.3f);

    for (int i = 0; i < 30; ++i)
        cloth.Update(0.016f, 4);

    // スフィア付近の頂点は押し出されるはず
    // NaN/クラッシュがないことだけ確認
    for (const auto& p : cloth.GetPositions())
    {
        EXPECT_FALSE(std::isnan(p.x));
        EXPECT_FALSE(std::isnan(p.y));
        EXPECT_FALSE(std::isnan(p.z));
    }
}

TEST(ClothSimulatorTest, ApplyForce)
{
    ClothSimulator cloth;
    ClothDesc desc;
    desc.width  = 3;
    desc.height = 3;
    desc.gravity = { 0, 0, 0 }; // 重力なし
    cloth.Initialize(desc);

    cloth.ApplyForce({ 10.0f, 0.0f, 0.0f });
    cloth.Update(0.1f, 2);

    // 頂点は+X方向に移動するはず
    float sumX = 0;
    for (const auto& p : cloth.GetPositions()) sumX += p.x;
    EXPECT_GT(sumX, 0.0f);
}

TEST(ClothSimulatorTest, NormalsGenerated)
{
    ClothSimulator cloth;
    ClothDesc desc;
    desc.width  = 3;
    desc.height = 3;
    cloth.Initialize(desc);
    cloth.Update(0.016f);

    const auto& normals = cloth.GetNormals();
    EXPECT_EQ(normals.size(), 9u);

    // 法線は単位長さであるべき
    for (const auto& n : normals)
    {
        float len = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
        EXPECT_NEAR(len, 1.0f, 0.01f);
    }
}

TEST(ClothSimulatorTest, ClearColliders)
{
    ClothSimulator cloth;
    ClothDesc desc;
    desc.width = 3;
    desc.height = 3;
    cloth.Initialize(desc);
    cloth.AddSphereCollider({ 0, 0, 0 }, 1.0f);
    cloth.ClearColliders();
    cloth.Update(0.016f); // クラッシュしないこと
}

// ============================================================================
// NetworkManager（ユニットレベル、実ネットワークなし）
// ============================================================================

TEST(NetworkManagerTest, InitialState)
{
    NetworkManager net;
    EXPECT_FALSE(net.IsServer());
    EXPECT_FALSE(net.IsConnected());
    EXPECT_EQ(net.GetClientCount(), 0u);
}

TEST(NetworkManagerTest, RegisterRPC)
{
    NetworkManager net;
    bool called = false;
    net.RegisterRPC("TestRPC", [&](ClientId, const std::vector<uint8_t>&) {
        called = true;
    });
    // RPCは登録されたがまだ呼ばれていない
    EXPECT_FALSE(called);
}

TEST(NetworkManagerTest, UpdateWithoutStartDoesNotCrash)
{
    NetworkManager net;
    net.Update(); // クラッシュしないこと
}

TEST(NetworkManagerTest, CallbackSetup)
{
    NetworkManager net;
    bool connectCalled = false;
    bool disconnectCalled = false;
    net.SetOnConnect([&](ClientId) { connectCalled = true; });
    net.SetOnDisconnect([&](ClientId) { disconnectCalled = true; });
    EXPECT_FALSE(connectCalled);
    EXPECT_FALSE(disconnectCalled);
}

// ============================================================================
// ScreenCapture（モックテスト — 実D3D12デバイスなし）
// ============================================================================

TEST(ScreenCaptureTest, InitializeWithNull)
{
    ScreenCapture cap;
    EXPECT_FALSE(cap.Initialize(nullptr));
    EXPECT_FALSE(cap.IsReady());
}

TEST(ScreenCaptureTest, ShutdownSafe)
{
    ScreenCapture cap;
    cap.Shutdown(); // 初期化なしでもクラッシュしないこと
    EXPECT_FALSE(cap.IsReady());
}
