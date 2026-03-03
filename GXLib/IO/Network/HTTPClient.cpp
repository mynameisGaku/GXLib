/// @file HTTPClient.cpp
/// @brief HTTPクライアント実装 — WinHTTP API ラッパー
#include "pch_common.h"
#include <winhttp.h>
#include "IO/Network/HTTPClient.h"
#include "Core/Logger.h"

namespace gx {

HTTPClient::HTTPClient()
{
    m_hSession = WinHttpOpen(L"GXLib/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);

    if (!m_hSession)
        GX_LOG_ERROR("HTTPClient: WinHttpOpen failed");
}

HTTPClient::~HTTPClient()
{
    m_running.store(false);

    for (auto& t : m_threads)
    {
        if (t.joinable())
            t.join();
    }
    m_threads.clear();

    if (m_hSession)
        WinHttpCloseHandle(static_cast<HINTERNET>(m_hSession));
}

void HTTPClient::SetTimeout(int timeoutMs)
{
    m_timeoutMs = timeoutMs;
}

// URLをホスト/パス/ポート/HTTPS判定に分解する（WinHTTP用）
static bool ParseURL(const gx::String& url,
                      gx::WString& host, gx::WString& path,
                      uint16_t& port, bool& isHttps)
{
    // wstringに変換する (WinHTTPはワイド文字列を要求)
    int wLen = MultiByteToWideChar(CP_UTF8, 0, url.c_str(), -1, nullptr, 0);
    gx::WString wUrl(wLen - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, url.c_str(), -1, wUrl.DataMut(), wLen);

    URL_COMPONENTS urlComp = {};
    urlComp.dwStructSize = sizeof(urlComp);

    wchar_t hostBuf[256] = {};
    wchar_t pathBuf[1024] = {};
    urlComp.lpszHostName = hostBuf;
    urlComp.dwHostNameLength = 256;
    urlComp.lpszUrlPath = pathBuf;
    urlComp.dwUrlPathLength = 1024;

    if (!WinHttpCrackUrl(wUrl.c_str(), 0, 0, &urlComp))
        return false;

    host = hostBuf;
    path = pathBuf;
    port = static_cast<uint16_t>(urlComp.nPort);
    isHttps = (urlComp.nScheme == INTERNET_SCHEME_HTTPS);
    return true;
}

HTTPResponse HTTPClient::SendRequest(const gx::String& method, const gx::String& url,
                                      const gx::String& body,
                                      const gx::HashMap<gx::String, gx::String>& headers)
{
    HTTPResponse response;

    if (!m_hSession)
    {
        response.statusCode = -1;
        return response;
    }

    gx::WString host, path;
    uint16_t port;
    bool isHttps;

    if (!ParseURL(url, host, path, port, isHttps))
    {
        GX_LOG_ERROR("HTTPClient: Failed to parse URL: %s", url.c_str());
        response.statusCode = -1;
        return response;
    }

    HINTERNET hConnect = WinHttpConnect(
        static_cast<HINTERNET>(m_hSession),
        host.c_str(), port, 0);

    if (!hConnect)
    {
        response.statusCode = -1;
        return response;
    }

    // メソッド文字列をwstringに変換する
    int mLen = MultiByteToWideChar(CP_UTF8, 0, method.c_str(), -1, nullptr, 0);
    gx::WString wMethod(mLen - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, method.c_str(), -1, wMethod.DataMut(), mLen);

    DWORD flags = isHttps ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect, wMethod.c_str(), path.c_str(),
        nullptr, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, flags);

    if (!hRequest)
    {
        WinHttpCloseHandle(hConnect);
        response.statusCode = -1;
        return response;
    }

    // タイムアウトを設定する
    WinHttpSetTimeouts(hRequest, m_timeoutMs, m_timeoutMs, m_timeoutMs, m_timeoutMs);

    // 追加ヘッダーを付与する
    for (const auto& [key, value] : headers)
    {
        gx::String headerLine = key + ": " + value;
        int hLen = MultiByteToWideChar(CP_UTF8, 0, headerLine.c_str(), -1, nullptr, 0);
        gx::WString wHeader(hLen - 1, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, headerLine.c_str(), -1, wHeader.DataMut(), hLen);
        WinHttpAddRequestHeaders(hRequest, wHeader.c_str(), (ULONG)-1, WINHTTP_ADDREQ_FLAG_ADD);
    }

    // リクエストを送信する
    BOOL result = WinHttpSendRequest(hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        body.empty() ? WINHTTP_NO_REQUEST_DATA : const_cast<char*>(body.c_str()),
        static_cast<DWORD>(body.size()),
        static_cast<DWORD>(body.size()), 0);

    if (!result)
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        response.statusCode = -1;
        return response;
    }

    result = WinHttpReceiveResponse(hRequest, nullptr);
    if (!result)
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        response.statusCode = -1;
        return response;
    }

    // ステータスコードを取得する
    DWORD statusCode = 0;
    DWORD size = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,
        &statusCode, &size, WINHTTP_NO_HEADER_INDEX);
    response.statusCode = static_cast<int>(statusCode);

    // レスポンス本文を読み取る
    gx::String responseBody;
    DWORD bytesAvailable = 0;
    while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0)
    {
        gx::Vector<char> buffer(bytesAvailable);
        DWORD bytesRead = 0;
        if (WinHttpReadData(hRequest, buffer.data(), bytesAvailable, &bytesRead))
        {
            responseBody.append(buffer.data(), bytesRead);
        }
    }
    response.body = std::move(responseBody);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);

    return response;
}

HTTPResponse HTTPClient::Get(const gx::String& url,
                              const gx::HashMap<gx::String, gx::String>& headers)
{
    return SendRequest("GET", url, "", headers);
}

HTTPResponse HTTPClient::Post(const gx::String& url, const gx::String& body,
                               const gx::String& contentType,
                               const gx::HashMap<gx::String, gx::String>& headers)
{
    auto allHeaders = headers;
    allHeaders["Content-Type"] = contentType;
    return SendRequest("POST", url, body, allHeaders);
}

void HTTPClient::GetAsync(const gx::String& url,
                            std::function<void(HTTPResponse)> callback)
{
    // 完了済みスレッドを除去してからワーカースレッドを追加する
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_threads.erase(
            std::remove_if(m_threads.begin(), m_threads.end(),
                [](std::thread& t) { return !t.joinable(); }),
            m_threads.end());
    }

    std::thread t([this, url, callback]() {
        auto resp = Get(url);
        std::lock_guard<std::mutex> lock(m_mutex);
        m_completedQueue.push_back({ std::move(resp), std::move(callback) });
    });

    std::lock_guard<std::mutex> lock(m_mutex);
    m_threads.push_back(std::move(t));
}

void HTTPClient::PostAsync(const gx::String& url, const gx::String& body,
                             const gx::String& contentType,
                             std::function<void(HTTPResponse)> callback)
{
    // 完了済みスレッドを除去してからワーカースレッドを追加する
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_threads.erase(
            std::remove_if(m_threads.begin(), m_threads.end(),
                [](std::thread& t) { return !t.joinable(); }),
            m_threads.end());
    }

    std::thread t([this, url, body, contentType, callback]() {
        auto resp = Post(url, body, contentType);
        std::lock_guard<std::mutex> lock(m_mutex);
        m_completedQueue.push_back({ std::move(resp), std::move(callback) });
    });

    std::lock_guard<std::mutex> lock(m_mutex);
    m_threads.push_back(std::move(t));
}

void HTTPClient::Update()
{
    gx::Vector<std::pair<HTTPResponse, std::function<void(HTTPResponse)>>> completed;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        completed.swap(m_completedQueue);
    }

    for (auto& [resp, cb] : completed)
    {
        if (cb) cb(std::move(resp));
    }
}

} // namespace gx
