/// @file Compat_Network.cpp
/// @brief 簡易API ネットワーク関数の実装 (GXLib独自 — DXLib に相当なし)
#include "pch.h"
#include "Compat/GXLib.h"
#include "Compat/CompatContext.h"
#include "IO/Network/NetworkManager.h"
#include "Core/Logger.h"

using Ctx = gx_internal::CompatContext;

namespace gx {

NetworkManager& GetNetworkManager() { return Ctx::Instance().EnsureNetwork(); }

// ============================================================================
// Server
// ============================================================================

int GX_StartServer(int port, int maxClients)
{
    if (port <= 0 || port > 65535)
    {
        GX_LOG_ERROR("GX_StartServer: invalid port %d", port);
        return -1;
    }
    auto& net = Ctx::Instance().EnsureNetwork();
    return net.StartServer(static_cast<uint16_t>(port),
                           static_cast<uint32_t>(maxClients)) ? 0 : -1;
}

void GX_StopServer()
{
    auto& ctx = Ctx::Instance();
    if (ctx.networkManager)
        ctx.networkManager->StopServer();
}

int GX_Broadcast(const void* data, int size)
{
    if (!data || size <= 0)
    {
        GX_LOG_ERROR("GX_Broadcast: invalid data or size");
        return -1;
    }
    auto& ctx = Ctx::Instance();
    if (!ctx.networkManager)
    {
        GX_LOG_ERROR("GX_Broadcast: network not initialized");
        return -1;
    }
    auto* bytes = static_cast<const uint8_t*>(data);
    ctx.networkManager->Broadcast(gx::Vector<uint8_t>(bytes, bytes + size));
    return 0;
}

int GX_SendToClient(int clientId, const void* data, int size)
{
    if (!data || size <= 0)
    {
        GX_LOG_ERROR("GX_SendToClient: invalid data or size");
        return -1;
    }
    auto& ctx = Ctx::Instance();
    if (!ctx.networkManager)
    {
        GX_LOG_ERROR("GX_SendToClient: network not initialized");
        return -1;
    }
    auto* bytes = static_cast<const uint8_t*>(data);
    ctx.networkManager->SendTo(static_cast<uint32_t>(clientId),
                               gx::Vector<uint8_t>(bytes, bytes + size));
    return 0;
}

// ============================================================================
// Client
// ============================================================================

int GX_Connect(const char* host, int port)
{
    if (!host || port <= 0 || port > 65535)
    {
        GX_LOG_ERROR("GX_Connect: invalid host or port");
        return -1;
    }
    auto& net = Ctx::Instance().EnsureNetwork();
    return net.Connect(gx::String(host), static_cast<uint16_t>(port)) ? 0 : -1;
}

void GX_Disconnect()
{
    auto& ctx = Ctx::Instance();
    if (ctx.networkManager)
        ctx.networkManager->Disconnect();
}

int GX_ClientSend(const void* data, int size)
{
    if (!data || size <= 0)
    {
        GX_LOG_ERROR("GX_ClientSend: invalid data or size");
        return -1;
    }
    auto& ctx = Ctx::Instance();
    if (!ctx.networkManager)
    {
        GX_LOG_ERROR("GX_ClientSend: network not initialized");
        return -1;
    }
    auto* bytes = static_cast<const uint8_t*>(data);
    ctx.networkManager->Send(gx::Vector<uint8_t>(bytes, bytes + size));
    return 0;
}

int GX_IsNetConnected()
{
    auto& ctx = Ctx::Instance();
    if (!ctx.networkManager) return 0;
    return ctx.networkManager->IsConnected() ? 1 : 0;
}

// ============================================================================
// Common
// ============================================================================

void GX_NetworkUpdate()
{
    auto& ctx = Ctx::Instance();
    if (ctx.networkManager)
        ctx.networkManager->Update();
}

int GX_GetNetClientCount()
{
    auto& ctx = Ctx::Instance();
    if (!ctx.networkManager) return 0;
    return static_cast<int>(ctx.networkManager->GetClientCount());
}

int GX_IsNetServer()
{
    auto& ctx = Ctx::Instance();
    if (!ctx.networkManager) return 0;
    return ctx.networkManager->IsServer() ? 1 : 0;
}

// ============================================================================
// Callbacks
// ============================================================================

void GX_SetOnClientConnect(void(*callback)(int clientId))
{
    auto& net = Ctx::Instance().EnsureNetwork();
    if (callback)
        net.SetOnConnect([callback](uint32_t id) { callback(static_cast<int>(id)); });
    else
        net.SetOnConnect(nullptr);
}

void GX_SetOnClientDisconnect(void(*callback)(int clientId))
{
    auto& net = Ctx::Instance().EnsureNetwork();
    if (callback)
        net.SetOnDisconnect([callback](uint32_t id) { callback(static_cast<int>(id)); });
    else
        net.SetOnDisconnect(nullptr);
}

void GX_SetOnReceive(void(*callback)(int fromClient, const void* data, int size))
{
    Ctx::Instance().onNetReceive = [callback](int from, const void* d, int s) {
        if (callback) callback(from, d, s);
    };
}

// ============================================================================
// Stats
// ============================================================================

float GX_GetNetRTT()
{
    auto& ctx = Ctx::Instance();
    if (!ctx.networkManager) return 0.0f;
    return ctx.networkManager->GetReliableChannel().GetRTT();
}

float GX_GetNetPacketLoss()
{
    auto& ctx = Ctx::Instance();
    if (!ctx.networkManager) return 0.0f;
    return ctx.networkManager->GetReliableChannel().GetPacketLoss();
}

} // namespace gx
