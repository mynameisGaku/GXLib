#include "pch.h"
#include <winhttp.h>
#include "IO/Network/WebSocket.h"
#include "Core/Logger.h"

namespace GX {

WebSocket::WebSocket() = default;

WebSocket::~WebSocket()
{
    Close();
}

bool WebSocket::Connect(const std::string& url)
{
    Close();

    // URLをwstringに変換する (WinHTTPはワイド文字列を要求)
    int wLen = MultiByteToWideChar(CP_UTF8, 0, url.c_str(), -1, nullptr, 0);
    std::wstring wUrl(wLen - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, url.c_str(), -1, wUrl.data(), wLen);

    // URLを分解する
    URL_COMPONENTS urlComp = {};
    urlComp.dwStructSize = sizeof(urlComp);
    wchar_t hostBuf[256] = {};
    wchar_t pathBuf[1024] = {};
    urlComp.lpszHostName = hostBuf;
    urlComp.dwHostNameLength = 256;
    urlComp.lpszUrlPath = pathBuf;
    urlComp.dwUrlPathLength = 1024;

    if (!WinHttpCrackUrl(wUrl.c_str(), 0, 0, &urlComp))
    {
        GX_LOG_ERROR("WebSocket::Connect: Failed to parse URL");
        return false;
    }

    bool isSecure = (urlComp.nScheme == INTERNET_SCHEME_HTTPS);

    m_hSession = WinHttpOpen(L"GXLib/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);
    if (!m_hSession) return false;

    m_hConnect = WinHttpConnect(
        static_cast<HINTERNET>(m_hSession),
        hostBuf, urlComp.nPort, 0);
    if (!m_hConnect)
    {
        WinHttpCloseHandle(static_cast<HINTERNET>(m_hSession));
        m_hSession = nullptr;
        return false;
    }

    DWORD flags = isSecure ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(
        static_cast<HINTERNET>(m_hConnect),
        L"GET", pathBuf, nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest)
    {
        Close();
        return false;
    }

    // WebSocketアップグレードを設定する
    BOOL result = WinHttpSetOption(hRequest,
        WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, nullptr, 0);
    if (!result)
    {
        WinHttpCloseHandle(hRequest);
        Close();
        return false;
    }

    result = WinHttpSendRequest(hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    if (!result)
    {
        WinHttpCloseHandle(hRequest);
        Close();
        return false;
    }

    result = WinHttpReceiveResponse(hRequest, nullptr);
    if (!result)
    {
        WinHttpCloseHandle(hRequest);
        Close();
        return false;
    }

    m_hWebSocket = WinHttpWebSocketCompleteUpgrade(hRequest, 0);
    WinHttpCloseHandle(hRequest);

    if (!m_hWebSocket)
    {
        Close();
        return false;
    }

    // 受信スレッドを開始する
    m_running.store(true);
    m_receiveThread = std::thread(&WebSocket::ReceiveLoop, this);

    GX_LOG_INFO("WebSocket: Connected to %s", url.c_str());
    return true;
}

void WebSocket::Close()
{
    // Set m_running=false FIRST so ReceiveLoop exits its while loop
    m_running.store(false);

    // Join the receive thread BEFORE closing handles to avoid use-after-free
    if (m_receiveThread.joinable())
        m_receiveThread.join();

    // Now safe to close handles since the receive thread has stopped
    if (m_hWebSocket)
    {
        WinHttpWebSocketClose(static_cast<HINTERNET>(m_hWebSocket),
            WINHTTP_WEB_SOCKET_SUCCESS_CLOSE_STATUS, nullptr, 0);
        WinHttpCloseHandle(static_cast<HINTERNET>(m_hWebSocket));
        m_hWebSocket = nullptr;
    }

    if (m_hConnect)
    {
        WinHttpCloseHandle(static_cast<HINTERNET>(m_hConnect));
        m_hConnect = nullptr;
    }
    if (m_hSession)
    {
        WinHttpCloseHandle(static_cast<HINTERNET>(m_hSession));
        m_hSession = nullptr;
    }
}

bool WebSocket::IsConnected() const
{
    return m_hWebSocket != nullptr && m_running.load();
}

bool WebSocket::Send(const std::string& message)
{
    if (!m_hWebSocket) return false;
    DWORD err = WinHttpWebSocketSend(static_cast<HINTERNET>(m_hWebSocket),
        WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,
        const_cast<char*>(message.c_str()),
        static_cast<DWORD>(message.size()));
    return err == ERROR_SUCCESS;
}

bool WebSocket::SendBinary(const void* data, size_t size)
{
    if (!m_hWebSocket) return false;
    DWORD err = WinHttpWebSocketSend(static_cast<HINTERNET>(m_hWebSocket),
        WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE,
        const_cast<void*>(data),
        static_cast<DWORD>(size));
    return err == ERROR_SUCCESS;
}

void WebSocket::Update()
{
    std::vector<QueuedMessage> messages;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        messages.swap(m_messageQueue);
    }

    for (auto& msg : messages)
    {
        switch (msg.type)
        {
        case QueuedMessage::Text:
            if (onMessage) onMessage(msg.data);
            break;
        case QueuedMessage::Binary:
            if (onBinaryMessage) onBinaryMessage(msg.data.data(), msg.data.size());
            break;
        case QueuedMessage::Closed:
            if (onClose) onClose();
            break;
        case QueuedMessage::Error:
            if (onError) onError(msg.data);
            break;
        }
    }
}

void WebSocket::ReceiveLoop()
{
    std::vector<uint8_t> buffer(8192);
    std::vector<uint8_t> accumulated;

    while (m_running.load())
    {
        DWORD bytesRead = 0;
        WINHTTP_WEB_SOCKET_BUFFER_TYPE bufferType;
        DWORD err = WinHttpWebSocketReceive(
            static_cast<HINTERNET>(m_hWebSocket),
            buffer.data(),
            static_cast<DWORD>(buffer.size()),
            &bytesRead, &bufferType);

        if (err != ERROR_SUCCESS)
        {
            if (m_running.load())
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_messageQueue.push_back({ QueuedMessage::Error, "Receive error" });
            }
            break;
        }

        accumulated.insert(accumulated.end(), buffer.data(), buffer.data() + bytesRead);

        // メッセージが完結しているか確認する (フラグメントかどうか)
        if (bufferType == WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE ||
            bufferType == WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE ||
            bufferType == WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE)
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            if (bufferType == WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE)
            {
                m_messageQueue.push_back({ QueuedMessage::Closed, "" });
                break;
            }
            else if (bufferType == WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE)
            {
                m_messageQueue.push_back({ QueuedMessage::Text,
                    std::string(accumulated.begin(), accumulated.end()) });
            }
            else
            {
                m_messageQueue.push_back({ QueuedMessage::Binary,
                    std::string(accumulated.begin(), accumulated.end()) });
            }
            accumulated.clear();
        }
        // フラグメントは結合して続行する (初心者向け: 分割フレームに対応)
    }

    m_running.store(false);
}

} // namespace GX
