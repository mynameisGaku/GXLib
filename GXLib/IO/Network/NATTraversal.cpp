/// @file NATTraversal.cpp
/// @brief NAT越え (STUN + UDP hole punching) の実装
#include "pch_common.h"
#include "IO/Network/NATTraversal.h"
#include "Core/Logger.h"
#include <cstring>
#include <random>

namespace gx
{

void NATTraversal::Initialize(const NATConfig& config)
{
    m_config = config;
    m_state = NATState::Idle;
    m_natType = NATType::Unknown;
    m_publicEndpoint = {};
    m_punchAttemptCount = 0;
}

void NATTraversal::StartDiscovery()
{
    SetState(NATState::Discovering);
    // In production: send STUN Binding Request to stun server via UDPSocket
}

void NATTraversal::StartPunching(const NATEndpoint& peerPublicEndpoint)
{
    m_peerEndpoint = peerPublicEndpoint;
    m_punchAttemptCount = 0;
    SetState(NATState::Punching);
}

void NATTraversal::Update()
{
    switch (m_state)
    {
    case NATState::Discovering:
        // Would check for STUN response
        break;
    case NATState::Punching:
        m_punchAttemptCount++;
        if (m_punchAttemptCount >= m_config.punchAttempts)
        {
            if (m_config.enableRelay)
                SetState(NATState::Relaying);
            else
                SetState(NATState::Failed);
        }
        break;
    default:
        break;
    }
}

gx::Vector<uint8_t> NATTraversal::BuildSTUNBindingRequest()
{
    // RFC 5389 STUN Binding Request
    gx::Vector<uint8_t> packet(20, 0);

    // Message type: 0x0001 (Binding Request)
    packet[0] = 0x00; packet[1] = 0x01;

    // Message length: 0 (no attributes)
    packet[2] = 0x00; packet[3] = 0x00;

    // Magic cookie: 0x2112A442
    packet[4] = 0x21; packet[5] = 0x12;
    packet[6] = 0xA4; packet[7] = 0x42;

    // Transaction ID: 12 random bytes
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 255);
    for (int i = 8; i < 20; i++)
        packet[i] = static_cast<uint8_t>(dist(gen));

    return packet;
}

STUNResult NATTraversal::ParseSTUNResponse(const uint8_t* data, size_t size)
{
    STUNResult result;

    if (!data || size < 20)
        return result;

    // Check message type: 0x0101 (Binding Success Response)
    if (data[0] != 0x01 || data[1] != 0x01)
        return result;

    // Check magic cookie
    if (data[4] != 0x21 || data[5] != 0x12 || data[6] != 0xA4 || data[7] != 0x42)
        return result;

    // Parse attributes
    size_t offset = 20;
    uint16_t msgLen = (static_cast<uint16_t>(data[2]) << 8) | data[3];
    size_t endOffset = 20 + msgLen;
    if (endOffset > size) endOffset = size;

    while (offset + 4 <= endOffset)
    {
        uint16_t attrType = (static_cast<uint16_t>(data[offset]) << 8) | data[offset + 1];
        uint16_t attrLen  = (static_cast<uint16_t>(data[offset + 2]) << 8) | data[offset + 3];
        offset += 4;

        // XOR-MAPPED-ADDRESS (0x0020)
        if (attrType == 0x0020 && attrLen >= 8 && offset + attrLen <= endOffset)
        {
            uint8_t family = data[offset + 1];
            if (family == 0x01) // IPv4
            {
                uint16_t xport = (static_cast<uint16_t>(data[offset + 2]) << 8) | data[offset + 3];
                result.mappedEndpoint.port = xport ^ 0x2112;

                uint8_t ip[4];
                ip[0] = data[offset + 4] ^ 0x21;
                ip[1] = data[offset + 5] ^ 0x12;
                ip[2] = data[offset + 6] ^ 0xA4;
                ip[3] = data[offset + 7] ^ 0x42;

                result.mappedEndpoint.host = gx::String((std::to_string(ip[0]) + "." +
                    std::to_string(ip[1]) + "." + std::to_string(ip[2]) + "." + std::to_string(ip[3])).c_str());
                result.success = true;
            }
        }

        offset += attrLen;
        // Pad to 4-byte boundary
        offset = (offset + 3) & ~static_cast<size_t>(3);
    }

    return result;
}

void NATTraversal::StartRelay(const NATEndpoint& relayServer)
{
    m_relayEndpoint = relayServer;
    SetState(NATState::Relaying);
}

void NATTraversal::Reset()
{
    m_state = NATState::Idle;
    m_natType = NATType::Unknown;
    m_publicEndpoint = {};
    m_peerEndpoint = {};
    m_punchAttemptCount = 0;
}

void NATTraversal::SetState(NATState newState)
{
    if (m_state == newState) return;
    m_state = newState;
    if (m_onStateChanged) m_onStateChanged(newState);
}

} // namespace gx
