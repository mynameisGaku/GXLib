#include "pch.h"
#include <WinSock2.h>
#include <ws2tcpip.h>
#include "IO/Network/TCPSocket.h"
#include "Core/Logger.h"

namespace GX {

bool TCPSocket::s_wsaInitialized = false;

void TCPSocket::EnsureWSAInit()
{
    if (!s_wsaInitialized)
    {
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0)
        {
            GX_LOG_ERROR("TCPSocket: WSAStartup failed: %d", result);
            return;
        }
        s_wsaInitialized = true;
    }
}

TCPSocket::TCPSocket()
{
    EnsureWSAInit();
}

TCPSocket::~TCPSocket()
{
    Close();
}

bool TCPSocket::Connect(const std::string& host, uint16_t port)
{
    Close();

    struct addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    std::string portStr = std::to_string(port);
    struct addrinfo* result = nullptr;
    int ret = getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result);
    if (ret != 0)
    {
        GX_LOG_ERROR("TCPSocket::Connect: getaddrinfo failed: %d", ret);
        return false;
    }

    m_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (m_socket == INVALID_SOCKET)
    {
        freeaddrinfo(result);
        return false;
    }

    ret = connect(m_socket, result->ai_addr, static_cast<int>(result->ai_addrlen));
    freeaddrinfo(result);

    if (ret == SOCKET_ERROR)
    {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        return false;
    }

    return true;
}

void TCPSocket::Close()
{
    if (m_socket != INVALID_SOCKET)
    {
        shutdown(m_socket, SD_BOTH);
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
}

bool TCPSocket::IsConnected() const
{
    return m_socket != INVALID_SOCKET;
}

int TCPSocket::Send(const void* data, int size)
{
    if (m_socket == INVALID_SOCKET) return -1;
    return send(m_socket, static_cast<const char*>(data), size, 0);
}

int TCPSocket::Receive(void* buffer, int bufferSize)
{
    if (m_socket == INVALID_SOCKET) return -1;
    return recv(m_socket, static_cast<char*>(buffer), bufferSize, 0);
}

void TCPSocket::SetNonBlocking(bool nonBlocking)
{
    if (m_socket == INVALID_SOCKET) return;
    u_long mode = nonBlocking ? 1 : 0;
    ioctlsocket(m_socket, FIONBIO, &mode);
}

bool TCPSocket::HasData() const
{
    if (m_socket == INVALID_SOCKET) return false;
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(m_socket, &readSet);
    timeval timeout = { 0, 0 };
    return select(0, &readSet, nullptr, nullptr, &timeout) > 0;
}

} // namespace GX
