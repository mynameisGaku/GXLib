/// @file test_TLSSocket.cpp
/// @brief TLSSocket のユニットテスト（SChannel TLSラッパー）

#include "pch.h"
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <gtest/gtest.h>
#include "IO/Network/TLSSocket.h"

using namespace gx;

// ============================================================================
// TLSConfig defaults
// ============================================================================

TEST(TLSConfigTest, DefaultValues)
{
    TLSConfig config;
    EXPECT_TRUE(config.verifyServer);
    EXPECT_TRUE(config.allowTLS12);
    EXPECT_TRUE(config.allowTLS13);
    EXPECT_TRUE(config.serverName.empty());
}

TEST(TLSConfigTest, CustomValues)
{
    TLSConfig config;
    config.verifyServer = false;
    config.allowTLS12 = false;
    config.allowTLS13 = true;
    config.serverName = "example.com";

    EXPECT_FALSE(config.verifyServer);
    EXPECT_FALSE(config.allowTLS12);
    EXPECT_TRUE(config.allowTLS13);
    EXPECT_EQ(config.serverName, "example.com");
}

TEST(TLSConfigTest, TLS12Only)
{
    TLSConfig config;
    config.allowTLS12 = true;
    config.allowTLS13 = false;
    EXPECT_TRUE(config.allowTLS12);
    EXPECT_FALSE(config.allowTLS13);
}

TEST(TLSConfigTest, TLS13Only)
{
    TLSConfig config;
    config.allowTLS12 = false;
    config.allowTLS13 = true;
    EXPECT_FALSE(config.allowTLS12);
    EXPECT_TRUE(config.allowTLS13);
}

TEST(TLSConfigTest, NoVerification)
{
    TLSConfig config;
    config.verifyServer = false;
    EXPECT_FALSE(config.verifyServer);
}

// ============================================================================
// TLSSocket construction and initial state
// ============================================================================

TEST(TLSSocketTest, ConstructDestruct)
{
    TLSSocket socket;
    EXPECT_FALSE(socket.IsConnected());
}

TEST(TLSSocketTest, InitialNotConnected)
{
    TLSSocket socket;
    EXPECT_FALSE(socket.IsConnected());
}

TEST(TLSSocketTest, InitialHasNoData)
{
    TLSSocket socket;
    EXPECT_FALSE(socket.HasData());
}

TEST(TLSSocketTest, InitialTLSVersionEmpty)
{
    TLSSocket socket;
    EXPECT_EQ(socket.GetTLSVersion(), "");
}

TEST(TLSSocketTest, InitialMaxMessageSizeZero)
{
    TLSSocket socket;
    EXPECT_EQ(socket.GetMaxMessageSize(), 0u);
}

// ============================================================================
// Send/Receive without connection
// ============================================================================

TEST(TLSSocketTest, SendWithoutConnection)
{
    TLSSocket socket;
    const char data[] = "hello";
    int result = socket.Send(data, sizeof(data));
    EXPECT_EQ(result, -1);
}

TEST(TLSSocketTest, ReceiveWithoutConnection)
{
    TLSSocket socket;
    char buffer[256];
    int result = socket.Receive(buffer, sizeof(buffer));
    EXPECT_EQ(result, -1);
}

TEST(TLSSocketTest, SendNullData)
{
    TLSSocket socket;
    int result = socket.Send(nullptr, 100);
    EXPECT_EQ(result, -1);
}

TEST(TLSSocketTest, SendZeroSize)
{
    TLSSocket socket;
    const char data[] = "hello";
    int result = socket.Send(data, 0);
    EXPECT_EQ(result, -1);
}

TEST(TLSSocketTest, ReceiveNullBuffer)
{
    TLSSocket socket;
    int result = socket.Receive(nullptr, 256);
    EXPECT_EQ(result, -1);
}

TEST(TLSSocketTest, ReceiveZeroSize)
{
    TLSSocket socket;
    char buffer[256];
    int result = socket.Receive(buffer, 0);
    EXPECT_EQ(result, -1);
}

// ============================================================================
// Close safety
// ============================================================================

TEST(TLSSocketTest, CloseWithoutConnection)
{
    TLSSocket socket;
    socket.Close();
    EXPECT_FALSE(socket.IsConnected());
}

TEST(TLSSocketTest, DoubleClose)
{
    TLSSocket socket;
    socket.Close();
    socket.Close();
    EXPECT_FALSE(socket.IsConnected());
}

TEST(TLSSocketTest, CloseResetsState)
{
    TLSSocket socket;
    socket.Close();
    EXPECT_FALSE(socket.IsConnected());
    EXPECT_FALSE(socket.HasData());
    EXPECT_EQ(socket.GetTLSVersion(), "");
    EXPECT_EQ(socket.GetMaxMessageSize(), 0u);
}

// ============================================================================
// Connect to invalid host (should fail gracefully)
// ============================================================================

TEST(TLSSocketTest, ConnectInvalidHost)
{
    TLSSocket socket;
    // 存在しないホストへの接続は失敗するはず
    bool result = socket.Connect("invalid.host.that.does.not.exist.local", 443);
    EXPECT_FALSE(result);
    EXPECT_FALSE(socket.IsConnected());
}

TEST(TLSSocketTest, ConnectLocalRefused)
{
    TLSSocket socket;
    // ローカルの閉じたポートへの接続は失敗するはず
    bool result = socket.Connect("127.0.0.1", 1);
    EXPECT_FALSE(result);
    EXPECT_FALSE(socket.IsConnected());
}

// ============================================================================
// Socket access
// ============================================================================

TEST(TLSSocketTest, GetSocketReference)
{
    TLSSocket socket;
    TCPSocket& tcpRef = socket.GetSocket();
    EXPECT_FALSE(tcpRef.IsConnected());
}

TEST(TLSSocketTest, GetConstSocketReference)
{
    const TLSSocket socket;
    const TCPSocket& tcpRef = socket.GetSocket();
    EXPECT_FALSE(tcpRef.IsConnected());
}

// ============================================================================
// Multiple instances
// ============================================================================

TEST(TLSSocketTest, MultipleInstances)
{
    TLSSocket s1;
    TLSSocket s2;
    TLSSocket s3;
    EXPECT_FALSE(s1.IsConnected());
    EXPECT_FALSE(s2.IsConnected());
    EXPECT_FALSE(s3.IsConnected());
}

TEST(TLSSocketTest, MoveSemantics)
{
    // TLSSocketはムーブ不可（CredHandle/CtxtHandleの都合）だが、
    // デフォルト構築→Close→再構築のパターンは安全
    {
        TLSSocket socket;
        socket.Close();
    }
    {
        TLSSocket socket;
        EXPECT_FALSE(socket.IsConnected());
    }
}
