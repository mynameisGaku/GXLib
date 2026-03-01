/// @file NetworkManager.cpp
/// @brief ネットワークマネージャー実装
#include "pch_common.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include "IO/Network/NetworkManager.h"
#include "IO/Network/UDPSocket.h"
#include <cstring>

namespace gx
{

NetworkManager::NetworkManager()
    : m_socket(std::make_unique<UDPSocket>())
{
}

NetworkManager::~NetworkManager()
{
    if (m_isServer) StopServer();
    else if (m_connected) Disconnect();
}

uint32_t NetworkManager::HashString(const std::string& s)
{
    // FNV-1a ハッシュ
    uint32_t hash = 2166136261u;
    for (char c : s)
    {
        hash ^= static_cast<uint32_t>(c);
        hash *= 16777619u;
    }
    return hash;
}

bool NetworkManager::StartServer(uint16_t port, uint32_t maxClients)
{
    if (m_isServer || m_connected) return false;

    m_socket = std::make_unique<UDPSocket>();
    if (!m_socket->Bind(port)) return false;

    m_socket->SetNonBlocking(true);
    m_isServer = true;
    m_maxClients = maxClients;
    m_clients.clear();
    m_nextClientId = 1;
    return true;
}

void NetworkManager::StopServer()
{
    if (!m_isServer) return;

    // 全クライアントに切断通知
    for (const auto& client : m_clients)
    {
        SendPacket(client.host, client.port, PacketType::Disconnect, nullptr, 0);
    }

    m_socket.reset();
    m_socket = std::make_unique<UDPSocket>();
    m_isServer = false;
    m_clients.clear();
}

bool NetworkManager::Connect(const std::string& host, uint16_t port)
{
    if (m_isServer || m_connected) return false;

    m_socket = std::make_unique<UDPSocket>();
    // クライアントは任意ポートにバインド
    if (!m_socket->Bind(0)) return false;
    m_socket->SetNonBlocking(true);

    m_serverHost = host;
    m_serverPort = port;

    // 接続パケット送信
    SendPacket(host, port, PacketType::Connect, nullptr, 0);
    m_connected = true;
    return true;
}

void NetworkManager::Disconnect()
{
    if (!m_connected) return;

    SendPacket(m_serverHost, m_serverPort, PacketType::Disconnect, nullptr, 0);
    m_connected = false;
    m_socket.reset();
    m_socket = std::make_unique<UDPSocket>();
}

void NetworkManager::Send(const std::vector<uint8_t>& data)
{
    if (!m_connected) return;
    SendPacket(m_serverHost, m_serverPort, PacketType::Data,
               data.data(), static_cast<uint16_t>(data.size()));
}

void NetworkManager::SendReliable(const std::string& host, uint16_t port,
                                   const std::vector<uint8_t>& data)
{
    m_reliableChannel.SendReliable(host, port,
                                    static_cast<uint16_t>(PacketType::Data),
                                    data.data(),
                                    static_cast<uint32_t>(data.size()));
}

void NetworkManager::Broadcast(const std::vector<uint8_t>& data)
{
    if (!m_isServer) return;
    for (const auto& client : m_clients)
    {
        SendPacket(client.host, client.port, PacketType::Data,
                   data.data(), static_cast<uint16_t>(data.size()));
    }
}

void NetworkManager::SendTo(ClientId client, const std::vector<uint8_t>& data)
{
    if (!m_isServer) return;
    for (const auto& c : m_clients)
    {
        if (c.id == client)
        {
            SendPacket(c.host, c.port, PacketType::Data,
                       data.data(), static_cast<uint16_t>(data.size()));
            return;
        }
    }
}

void NetworkManager::RegisterRPC(const std::string& name, RPCHandler handler)
{
    uint32_t hash = HashString(name);
    m_rpcHandlers[hash] = std::move(handler);
    m_rpcNameToHash[name] = hash;
}

void NetworkManager::CallRPC(const std::string& name, const std::vector<uint8_t>& args)
{
    uint32_t hash = HashString(name);
    // [4バイト RPC hash][payload]
    std::vector<uint8_t> packet(4 + args.size());
    std::memcpy(packet.data(), &hash, 4);
    if (!args.empty())
        std::memcpy(packet.data() + 4, args.data(), args.size());

    if (m_isServer)
    {
        // サーバー: 全クライアントに送信
        for (const auto& client : m_clients)
        {
            SendPacket(client.host, client.port, PacketType::RPC,
                       packet.data(), static_cast<uint16_t>(packet.size()));
        }
    }
    else if (m_connected)
    {
        // クライアント: サーバーに送信
        SendPacket(m_serverHost, m_serverPort, PacketType::RPC,
                   packet.data(), static_cast<uint16_t>(packet.size()));
    }
}

void NetworkManager::CallRPCOn(ClientId target, const std::string& name,
                                const std::vector<uint8_t>& args)
{
    if (!m_isServer) return;

    uint32_t hash = HashString(name);
    std::vector<uint8_t> packet(4 + args.size());
    std::memcpy(packet.data(), &hash, 4);
    if (!args.empty())
        std::memcpy(packet.data() + 4, args.data(), args.size());

    for (const auto& client : m_clients)
    {
        if (client.id == target)
        {
            SendPacket(client.host, client.port, PacketType::RPC,
                       packet.data(), static_cast<uint16_t>(packet.size()));
            return;
        }
    }
}

void NetworkManager::SendPacket(const std::string& host, uint16_t port,
                                 PacketType type, const void* data, uint16_t size)
{
    PacketHeader header;
    header.magic = 0x47584E54;
    header.type = type;
    header.size = size;

    std::vector<uint8_t> buffer(sizeof(PacketHeader) + size);
    std::memcpy(buffer.data(), &header, sizeof(PacketHeader));
    if (data && size > 0)
        std::memcpy(buffer.data() + sizeof(PacketHeader), data, size);

    m_socket->SendTo(host, port, buffer.data(), static_cast<int>(buffer.size()));
}

void NetworkManager::Update()
{
    if (!m_socket) return;

    uint8_t buffer[4096];
    std::string fromHost;
    uint16_t fromPort;

    while (true)
    {
        int received = m_socket->ReceiveFrom(buffer, sizeof(buffer), fromHost, fromPort);
        if (received <= 0) break;

        ProcessPacket(fromHost, fromPort, buffer, received);
    }
}

void NetworkManager::ProcessPacket(const std::string& fromHost, uint16_t fromPort,
                                    const uint8_t* data, int size)
{
    if (size < static_cast<int>(sizeof(PacketHeader))) return;

    PacketHeader header;
    std::memcpy(&header, data, sizeof(PacketHeader));

    if (header.magic != 0x47584E54) return;
    if (static_cast<int>(sizeof(PacketHeader) + header.size) > size) return;

    const uint8_t* payload = data + sizeof(PacketHeader);
    uint16_t payloadSize = header.size;

    // クライアントIDを特定
    ClientId senderId = 0;
    if (m_isServer)
    {
        for (const auto& c : m_clients)
        {
            if (c.host == fromHost && c.port == fromPort)
            {
                senderId = c.id;
                break;
            }
        }
    }

    switch (header.type)
    {
    case PacketType::Connect:
    {
        if (!m_isServer) break;
        if (m_clients.size() >= m_maxClients) break;

        // 既に接続済みかチェック
        for (const auto& c : m_clients)
        {
            if (c.host == fromHost && c.port == fromPort) return;
        }

        ClientInfo info;
        info.id = m_nextClientId++;
        info.host = fromHost;
        info.port = fromPort;
        m_clients.push_back(info);

        if (m_onConnect) m_onConnect(info.id);
        break;
    }
    case PacketType::Disconnect:
    {
        if (m_isServer)
        {
            for (auto it = m_clients.begin(); it != m_clients.end(); ++it)
            {
                if (it->host == fromHost && it->port == fromPort)
                {
                    ClientId id = it->id;
                    m_clients.erase(it);
                    if (m_onDisconnect) m_onDisconnect(id);
                    break;
                }
            }
        }
        else
        {
            m_connected = false;
        }
        break;
    }
    case PacketType::RPC:
    {
        if (payloadSize < 4) break;
        uint32_t rpcHash;
        std::memcpy(&rpcHash, payload, 4);

        auto it = m_rpcHandlers.find(rpcHash);
        if (it != m_rpcHandlers.end())
        {
            std::vector<uint8_t> args(payload + 4, payload + payloadSize);
            it->second(senderId, args);
        }
        break;
    }
    case PacketType::Data:
    {
        // データパケット — 将来的にコールバック化
        break;
    }
    }
}

} // namespace gx
