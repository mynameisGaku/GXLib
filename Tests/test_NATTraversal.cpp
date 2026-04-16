/// @file test_NATTraversal.cpp
#include <gtest/gtest.h>
#include "IO/Network/NATTraversal.h"
using namespace gx;

TEST(NATTraversalTest, DefaultState) {
    NATTraversal nat;
    EXPECT_EQ(nat.GetState(), NATState::Idle);
    EXPECT_FALSE(nat.IsConnected());
}

TEST(NATTraversalTest, Initialize) {
    NATTraversal nat;
    NATConfig cfg;
    cfg.stunPort = 3478;
    nat.Initialize(cfg);
    EXPECT_EQ(nat.GetConfig().stunPort, 3478u);
    EXPECT_EQ(nat.GetState(), NATState::Idle);
}

TEST(NATTraversalTest, StartDiscovery) {
    NATTraversal nat;
    nat.Initialize();
    nat.StartDiscovery();
    EXPECT_EQ(nat.GetState(), NATState::Discovering);
}

TEST(NATTraversalTest, StartPunching) {
    NATTraversal nat;
    nat.Initialize();
    NATEndpoint peer;
    peer.host = "1.2.3.4"; peer.port = 5000;
    nat.StartPunching(peer);
    EXPECT_EQ(nat.GetState(), NATState::Punching);
}

TEST(NATTraversalTest, PunchTimeout) {
    NATTraversal nat;
    NATConfig cfg;
    cfg.punchAttempts = 3;
    cfg.enableRelay = true;
    nat.Initialize(cfg);
    NATEndpoint peer; peer.host = "1.2.3.4"; peer.port = 5000;
    nat.StartPunching(peer);
    for (uint32_t i = 0; i < 3; i++) nat.Update();
    EXPECT_EQ(nat.GetState(), NATState::Relaying);
}

TEST(NATTraversalTest, PunchTimeoutNoRelay) {
    NATTraversal nat;
    NATConfig cfg;
    cfg.punchAttempts = 3;
    cfg.enableRelay = false;
    nat.Initialize(cfg);
    NATEndpoint peer; peer.host = "1.2.3.4"; peer.port = 5000;
    nat.StartPunching(peer);
    for (uint32_t i = 0; i < 3; i++) nat.Update();
    EXPECT_EQ(nat.GetState(), NATState::Failed);
}

TEST(NATTraversalTest, BuildSTUNRequest) {
    auto packet = NATTraversal::BuildSTUNBindingRequest();
    EXPECT_EQ(packet.size(), 20u);
    EXPECT_EQ(packet[0], 0x00); EXPECT_EQ(packet[1], 0x01); // Binding Request
    EXPECT_EQ(packet[4], 0x21); EXPECT_EQ(packet[5], 0x12); // Magic cookie
}

TEST(NATTraversalTest, ParseSTUNResponseInvalid) {
    auto result = NATTraversal::ParseSTUNResponse(nullptr, 0);
    EXPECT_FALSE(result.success);
}

TEST(NATTraversalTest, ParseSTUNResponseTooShort) {
    uint8_t data[10] = {};
    auto result = NATTraversal::ParseSTUNResponse(data, sizeof(data));
    EXPECT_FALSE(result.success);
}

TEST(NATTraversalTest, ParseSTUNResponseValid) {
    // Construct a minimal valid STUN Binding Success Response with XOR-MAPPED-ADDRESS
    uint8_t resp[32] = {};
    resp[0] = 0x01; resp[1] = 0x01; // Binding Success Response
    resp[2] = 0x00; resp[3] = 12;   // Message length = 12
    resp[4] = 0x21; resp[5] = 0x12; resp[6] = 0xA4; resp[7] = 0x42; // Magic cookie
    // Transaction ID (bytes 8-19)
    // Attribute: XOR-MAPPED-ADDRESS (type 0x0020, len 8)
    resp[20] = 0x00; resp[21] = 0x20;  // Type
    resp[22] = 0x00; resp[23] = 0x08;  // Length
    resp[24] = 0x00; resp[25] = 0x01;  // Reserved + Family (IPv4)
    // XOR'd port: 12345 XOR 0x2112 = 0x3039 XOR 0x2112 = 0x112B
    uint16_t port = 12345;
    uint16_t xport = port ^ 0x2112;
    resp[26] = static_cast<uint8_t>(xport >> 8);
    resp[27] = static_cast<uint8_t>(xport & 0xFF);
    // XOR'd IP: 192.168.1.100 XOR magic cookie first 4 bytes
    resp[28] = 192 ^ 0x21; resp[29] = 168 ^ 0x12;
    resp[30] = 1 ^ 0xA4;   resp[31] = 100 ^ 0x42;

    auto result = NATTraversal::ParseSTUNResponse(resp, sizeof(resp));
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.mappedEndpoint.port, 12345u);
    EXPECT_EQ(result.mappedEndpoint.host, "192.168.1.100");
}

TEST(NATTraversalTest, StartRelay) {
    NATTraversal nat;
    nat.Initialize();
    NATEndpoint relay; relay.host = "relay.example.com"; relay.port = 9999;
    nat.StartRelay(relay);
    EXPECT_EQ(nat.GetState(), NATState::Relaying);
    EXPECT_TRUE(nat.IsConnected());
}

TEST(NATTraversalTest, Reset) {
    NATTraversal nat;
    nat.Initialize();
    nat.StartDiscovery();
    nat.Reset();
    EXPECT_EQ(nat.GetState(), NATState::Idle);
}

TEST(NATTraversalTest, StateCallback) {
    NATTraversal nat;
    nat.Initialize();
    NATState lastState = NATState::Idle;
    nat.SetOnStateChanged([&](NATState s) { lastState = s; });
    nat.StartDiscovery();
    EXPECT_EQ(lastState, NATState::Discovering);
}

TEST(NATTraversalTest, PunchAttemptCount) {
    NATTraversal nat;
    NATConfig cfg; cfg.punchAttempts = 100;
    nat.Initialize(cfg);
    NATEndpoint peer; peer.host = "1.2.3.4"; peer.port = 5000;
    nat.StartPunching(peer);
    nat.Update();
    nat.Update();
    EXPECT_EQ(nat.GetPunchAttemptCount(), 2u);
}
