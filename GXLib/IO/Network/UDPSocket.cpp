#include "pch.h"
#include <WinSock2.h>
#include <ws2tcpip.h>
#include "IO/Network/UDPSocket.h"
#include "Core/Logger.h"

namespace GX {

UDPSocket::UDPSocket()
{
    // WSA初期化はTCPSocket::EnsureWSAInit()または初回使用で行う想定
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_socket == INVALID_SOCKET)
        GX_LOG_ERROR("UDPSocket: socket() failed");
}

UDPSocket::~UDPSocket()
{
    Close();
}

bool UDPSocket::Bind(uint16_t port)
{
    if (m_socket == INVALID_SOCKET) return false;

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(m_socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR)
    {
        GX_LOG_ERROR("UDPSocket::Bind: bind() failed on port %u", port);
        return false;
    }
    return true;
}

void UDPSocket::Close()
{
    if (m_socket != INVALID_SOCKET)
    {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
}

int UDPSocket::SendTo(const std::string& host, uint16_t port, const void* data, int size)
{
    if (m_socket == INVALID_SOCKET) return -1;

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &addr.sin_addr);

    return sendto(m_socket, static_cast<const char*>(data), size, 0,
                  reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
}

int UDPSocket::ReceiveFrom(void* buffer, int bufferSize, std::string& outHost, uint16_t& outPort)
{
    if (m_socket == INVALID_SOCKET) return -1;

    sockaddr_in addr = {};
    int addrLen = sizeof(addr);
    int received = recvfrom(m_socket, static_cast<char*>(buffer), bufferSize, 0,
                             reinterpret_cast<sockaddr*>(&addr), &addrLen);

    if (received > 0)
    {
        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr.sin_addr, ipStr, INET_ADDRSTRLEN);
        outHost = ipStr;
        outPort = ntohs(addr.sin_port);
    }
    return received;
}

void UDPSocket::SetNonBlocking(bool nonBlocking)
{
    if (m_socket == INVALID_SOCKET) return;
    u_long mode = nonBlocking ? 1 : 0;
    ioctlsocket(m_socket, FIONBIO, &mode);
}

} // namespace GX
