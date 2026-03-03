/// @file TLSSocket.cpp
/// @brief TLS暗号化TCPソケット実装 — Windows SChannel API
#include "pch_common.h"
#include <WinSock2.h>
#include <ws2tcpip.h>
#include "IO/Network/TLSSocket.h"
#include "Core/Logger.h"

#pragma comment(lib, "secur32.lib")
#pragma comment(lib, "crypt32.lib")

namespace gx
{

// ============================================================================
// 定数
// ============================================================================
static constexpr size_t k_InitialRecvBufferSize = 16384;
static constexpr size_t k_MaxRecvBufferSize     = 65536;
static constexpr DWORD  k_HandshakeFlags =
    ISC_REQ_SEQUENCE_DETECT |
    ISC_REQ_REPLAY_DETECT |
    ISC_REQ_CONFIDENTIALITY |
    ISC_REQ_EXTENDED_ERROR |
    ISC_REQ_ALLOCATE_MEMORY |
    ISC_REQ_STREAM;

// ============================================================================
// コンストラクタ / デストラクタ
// ============================================================================
TLSSocket::TLSSocket()
{
    SecInvalidateHandle(&m_credHandle);
    SecInvalidateHandle(&m_ctxHandle);
}

TLSSocket::~TLSSocket()
{
    Close();
}

// ============================================================================
// Connect — TCP接続 + TLSハンドシェイク
// ============================================================================
bool TLSSocket::Connect(const gx::String& host, uint16_t port, const TLSConfig& config)
{
    Close();

    // TCP接続
    if (!m_socket.Connect(host, port))
    {
        GX_LOG_ERROR("TLSSocket: TCP connection to %s:%u failed", host.c_str(), port);
        return false;
    }

    // SChannel資格情報の初期化
    if (!InitializeCredentials(config))
    {
        GX_LOG_ERROR("TLSSocket: Failed to initialize SChannel credentials");
        m_socket.Close();
        return false;
    }

    // TLSハンドシェイク
    gx::String serverName = config.serverName.empty() ? host : config.serverName;
    if (!PerformHandshake(serverName))
    {
        GX_LOG_ERROR("TLSSocket: TLS handshake with %s failed", serverName.c_str());
        Cleanup();
        m_socket.Close();
        return false;
    }

    // ストリームサイズ情報を取得
    SECURITY_STATUS status = QueryContextAttributes(&m_ctxHandle, SECPKG_ATTR_STREAM_SIZES, &m_streamSizes);
    if (status != SEC_E_OK)
    {
        GX_LOG_ERROR("TLSSocket: QueryContextAttributes failed: 0x%08lX", status);
        Cleanup();
        m_socket.Close();
        return false;
    }

    // 受信バッファ確保
    m_recvBuffer.resize(k_InitialRecvBufferSize);
    m_recvUsed = 0;
    m_decryptBuffer.resize(m_streamSizes.cbMaximumMessage);
    m_decryptAvail = 0;

    m_connected = true;
    return true;
}

// ============================================================================
// Close — TLSシャットダウン + TCP切断
// ============================================================================
void TLSSocket::Close()
{
    if (m_connected && m_ctxValid && m_socket.IsConnected())
    {
        // TLS shutdown notify を送信
        DWORD shutdownToken = SCHANNEL_SHUTDOWN;
        SecBufferDesc shutdownBufferDesc;
        SecBuffer     shutdownBuffer;

        shutdownBuffer.cbBuffer   = sizeof(shutdownToken);
        shutdownBuffer.BufferType = SECBUFFER_TOKEN;
        shutdownBuffer.pvBuffer   = &shutdownToken;

        shutdownBufferDesc.cBuffers  = 1;
        shutdownBufferDesc.pBuffers  = &shutdownBuffer;
        shutdownBufferDesc.ulVersion = SECBUFFER_VERSION;

        SECURITY_STATUS status = ApplyControlToken(&m_ctxHandle, &shutdownBufferDesc);
        if (status == SEC_E_OK)
        {
            // シャットダウンメッセージを構築して送信
            SecBuffer   outBuffers[1] = {};
            outBuffers[0].BufferType = SECBUFFER_TOKEN;

            SecBufferDesc outBufferDesc;
            outBufferDesc.cBuffers  = 1;
            outBufferDesc.pBuffers  = outBuffers;
            outBufferDesc.ulVersion = SECBUFFER_VERSION;

            DWORD flags = 0;
            status = InitializeSecurityContextA(
                &m_credHandle, &m_ctxHandle, nullptr,
                k_HandshakeFlags, 0, 0,
                nullptr, 0,
                &m_ctxHandle, &outBufferDesc, &flags, nullptr);

            if (status == SEC_E_OK || status == SEC_I_CONTEXT_EXPIRED)
            {
                if (outBuffers[0].pvBuffer && outBuffers[0].cbBuffer > 0)
                {
                    m_socket.Send(outBuffers[0].pvBuffer, static_cast<int>(outBuffers[0].cbBuffer));
                    FreeContextBuffer(outBuffers[0].pvBuffer);
                }
            }
        }
    }

    Cleanup();
    m_socket.Close();
    m_connected = false;
    m_recvBuffer.clear();
    m_decryptBuffer.clear();
    m_decryptAvail = 0;
    m_recvUsed = 0;
    m_streamSizes = {};
}

// ============================================================================
// Send — 暗号化して送信
// ============================================================================
int TLSSocket::Send(const void* data, int size)
{
    if (!m_connected || !data || size <= 0)
        return -1;

    const uint8_t* src = static_cast<const uint8_t*>(data);
    int totalSent = 0;

    while (totalSent < size)
    {
        // 最大メッセージサイズごとに分割して送信
        int chunkSize = (std::min)(static_cast<int>(m_streamSizes.cbMaximumMessage),
                                   size - totalSent);

        // 暗号化バッファ: header + data + trailer + empty
        size_t totalBufSize = m_streamSizes.cbHeader + chunkSize + m_streamSizes.cbTrailer;
        gx::Vector<uint8_t> sendBuf(totalBufSize);

        // データ部分をコピー
        std::memcpy(sendBuf.data() + m_streamSizes.cbHeader, src + totalSent, chunkSize);

        // SecBuffer構成
        SecBuffer buffers[4] = {};
        buffers[0].BufferType = SECBUFFER_STREAM_HEADER;
        buffers[0].cbBuffer   = m_streamSizes.cbHeader;
        buffers[0].pvBuffer   = sendBuf.data();

        buffers[1].BufferType = SECBUFFER_DATA;
        buffers[1].cbBuffer   = static_cast<unsigned long>(chunkSize);
        buffers[1].pvBuffer   = sendBuf.data() + m_streamSizes.cbHeader;

        buffers[2].BufferType = SECBUFFER_STREAM_TRAILER;
        buffers[2].cbBuffer   = m_streamSizes.cbTrailer;
        buffers[2].pvBuffer   = sendBuf.data() + m_streamSizes.cbHeader + chunkSize;

        buffers[3].BufferType = SECBUFFER_EMPTY;
        buffers[3].cbBuffer   = 0;
        buffers[3].pvBuffer   = nullptr;

        SecBufferDesc bufferDesc;
        bufferDesc.ulVersion = SECBUFFER_VERSION;
        bufferDesc.cBuffers  = 4;
        bufferDesc.pBuffers  = buffers;

        SECURITY_STATUS status = EncryptMessage(&m_ctxHandle, 0, &bufferDesc, 0);
        if (status != SEC_E_OK)
        {
            GX_LOG_ERROR("TLSSocket::Send: EncryptMessage failed: 0x%08lX", status);
            return -1;
        }

        // 暗号化済みデータのトータルサイズを計算
        size_t encryptedSize = buffers[0].cbBuffer + buffers[1].cbBuffer + buffers[2].cbBuffer;

        // TCP送信（全バイトを確実に送信）
        size_t sentBytes = 0;
        while (sentBytes < encryptedSize)
        {
            int ret = m_socket.Send(sendBuf.data() + sentBytes,
                                     static_cast<int>(encryptedSize - sentBytes));
            if (ret <= 0)
            {
                GX_LOG_ERROR("TLSSocket::Send: TCP send failed");
                m_connected = false;
                return -1;
            }
            sentBytes += static_cast<size_t>(ret);
        }

        totalSent += chunkSize;
    }

    return totalSent;
}

// ============================================================================
// Receive — 受信して復号
// ============================================================================
int TLSSocket::Receive(void* buffer, int bufferSize)
{
    if (!m_connected || !buffer || bufferSize <= 0)
        return -1;

    // 復号済みバッファにデータがあればそこから返す
    if (m_decryptAvail > 0)
    {
        size_t copySize = (std::min)(static_cast<size_t>(bufferSize), m_decryptAvail);
        std::memcpy(buffer, m_decryptBuffer.data(), copySize);

        // 残りを先頭へシフト
        if (copySize < m_decryptAvail)
            std::memmove(m_decryptBuffer.data(), m_decryptBuffer.data() + copySize,
                         m_decryptAvail - copySize);
        m_decryptAvail -= copySize;

        return static_cast<int>(copySize);
    }

    // 復号ループ: データを受信して復号を試みる
    for (;;)
    {
        // 受信バッファの空き容量がなければ拡張
        if (m_recvUsed >= m_recvBuffer.size())
        {
            size_t newSize = (std::min)(m_recvBuffer.size() * 2, k_MaxRecvBufferSize);
            if (newSize <= m_recvBuffer.size())
            {
                GX_LOG_ERROR("TLSSocket::Receive: recv buffer exhausted");
                return -1;
            }
            m_recvBuffer.resize(newSize);
        }

        // TCPから暗号化データを受信
        int received = m_socket.Receive(m_recvBuffer.data() + m_recvUsed,
                                        static_cast<int>(m_recvBuffer.size() - m_recvUsed));
        if (received == 0)
        {
            // 接続が閉じられた
            m_connected = false;
            return 0;
        }
        if (received < 0)
        {
            // エラー（WSAEWOULDBLOCK の場合は受信データなし）
            int err = WSAGetLastError();
            if (err == WSAEWOULDBLOCK)
                return 0;
            m_connected = false;
            return -1;
        }

        m_recvUsed += static_cast<size_t>(received);

        // 復号を試みる
        SecBuffer buffers[4] = {};
        buffers[0].BufferType = SECBUFFER_DATA;
        buffers[0].cbBuffer   = static_cast<unsigned long>(m_recvUsed);
        buffers[0].pvBuffer   = m_recvBuffer.data();
        buffers[1].BufferType = SECBUFFER_EMPTY;
        buffers[2].BufferType = SECBUFFER_EMPTY;
        buffers[3].BufferType = SECBUFFER_EMPTY;

        SecBufferDesc bufferDesc;
        bufferDesc.ulVersion = SECBUFFER_VERSION;
        bufferDesc.cBuffers  = 4;
        bufferDesc.pBuffers  = buffers;

        SECURITY_STATUS status = DecryptMessage(&m_ctxHandle, &bufferDesc, 0, nullptr);

        if (status == SEC_E_INCOMPLETE_MESSAGE)
        {
            // データ不足 — もっと受信する必要がある
            continue;
        }

        if (status == SEC_I_CONTEXT_EXPIRED)
        {
            // サーバーがTLS接続を閉じた
            m_connected = false;
            return 0;
        }

        if (status == SEC_I_RENEGOTIATE)
        {
            // リネゴシエーション要求 — 再ハンドシェイク
            // 簡易実装: 接続を閉じる
            GX_LOG_WARN("TLSSocket::Receive: renegotiation requested, closing");
            m_connected = false;
            return -1;
        }

        if (status != SEC_E_OK)
        {
            GX_LOG_ERROR("TLSSocket::Receive: DecryptMessage failed: 0x%08lX", status);
            m_connected = false;
            return -1;
        }

        // 復号成功 — バッファから平文データとExtraデータを取得
        SecBuffer* pDataBuffer  = nullptr;
        SecBuffer* pExtraBuffer = nullptr;

        for (int i = 0; i < 4; ++i)
        {
            if (buffers[i].BufferType == SECBUFFER_DATA)
                pDataBuffer = &buffers[i];
            else if (buffers[i].BufferType == SECBUFFER_EXTRA)
                pExtraBuffer = &buffers[i];
        }

        // 復号済みデータをコピー
        int resultSize = 0;
        if (pDataBuffer && pDataBuffer->cbBuffer > 0)
        {
            size_t dataSize = pDataBuffer->cbBuffer;
            size_t copySize = (std::min)(static_cast<size_t>(bufferSize), dataSize);

            std::memcpy(buffer, pDataBuffer->pvBuffer, copySize);
            resultSize = static_cast<int>(copySize);

            // 余剰分をm_decryptBufferに保存
            if (copySize < dataSize)
            {
                size_t surplus = dataSize - copySize;
                if (surplus > m_decryptBuffer.size())
                    m_decryptBuffer.resize(surplus);
                std::memcpy(m_decryptBuffer.data(),
                            static_cast<uint8_t*>(pDataBuffer->pvBuffer) + copySize,
                            surplus);
                m_decryptAvail = surplus;
            }
        }

        // Extra データ（次のTLSレコードの一部）を受信バッファの先頭に移動
        if (pExtraBuffer && pExtraBuffer->cbBuffer > 0)
        {
            std::memmove(m_recvBuffer.data(), pExtraBuffer->pvBuffer, pExtraBuffer->cbBuffer);
            m_recvUsed = pExtraBuffer->cbBuffer;
        }
        else
        {
            m_recvUsed = 0;
        }

        return resultSize;
    }
}

// ============================================================================
// HasData — 復号済みバッファ or TCP受信データ
// ============================================================================
bool TLSSocket::HasData()
{
    if (!m_connected)
        return false;
    if (m_decryptAvail > 0)
        return true;
    return m_socket.HasData();
}

// ============================================================================
// GetTLSVersion — ネゴシエート済みTLSバージョンを返す
// ============================================================================
gx::String TLSSocket::GetTLSVersion() const
{
    if (!m_ctxValid)
        return "";

    SecPkgContext_ConnectionInfo connInfo = {};
    SECURITY_STATUS status = QueryContextAttributes(
        const_cast<PCtxtHandle>(&m_ctxHandle),
        SECPKG_ATTR_CONNECTION_INFO,
        &connInfo);

    if (status != SEC_E_OK)
        return "";

    switch (connInfo.dwProtocol)
    {
    case SP_PROT_TLS1_3_CLIENT:
    case SP_PROT_TLS1_3_SERVER:
        return "TLS 1.3";
    case SP_PROT_TLS1_2_CLIENT:
    case SP_PROT_TLS1_2_SERVER:
        return "TLS 1.2";
    case SP_PROT_TLS1_1_CLIENT:
    case SP_PROT_TLS1_1_SERVER:
        return "TLS 1.1";
    case SP_PROT_TLS1_CLIENT:
    case SP_PROT_TLS1_SERVER:
        return "TLS 1.0";
    default:
        return "Unknown";
    }
}

// ============================================================================
// GetMaxMessageSize
// ============================================================================
uint32_t TLSSocket::GetMaxMessageSize() const
{
    if (!m_connected)
        return 0;
    return static_cast<uint32_t>(m_streamSizes.cbMaximumMessage);
}

// ============================================================================
// InitializeCredentials — AcquireCredentialsHandle with SCH_CREDENTIALS
// ============================================================================
bool TLSSocket::InitializeCredentials(const TLSConfig& config)
{
    // プロトコルフラグを構築
    DWORD enabledProtocols = 0;
    if (config.allowTLS12)
        enabledProtocols |= SP_PROT_TLS1_2_CLIENT;
    if (config.allowTLS13)
        enabledProtocols |= SP_PROT_TLS1_3_CLIENT;

    // デフォルトでTLS 1.2/1.3両方有効
    if (enabledProtocols == 0)
        enabledProtocols = SP_PROT_TLS1_2_CLIENT | SP_PROT_TLS1_3_CLIENT;

    // SCHANNEL_CRED (全Windows SDK で利用可能)
    SCHANNEL_CRED schCred = {};
    schCred.dwVersion = SCHANNEL_CRED_VERSION;
    schCred.dwFlags = SCH_USE_STRONG_CRYPTO;
    schCred.grbitEnabledProtocols = enabledProtocols;

    if (!config.verifyServer)
        schCred.dwFlags |= SCH_CRED_MANUAL_CRED_VALIDATION | SCH_CRED_NO_SERVERNAME_CHECK;

    SECURITY_STATUS status = AcquireCredentialsHandleA(
        nullptr,                    // principal
        const_cast<LPSTR>(UNISP_NAME_A),  // package name
        SECPKG_CRED_OUTBOUND,       // client credentials
        nullptr,                    // logon ID
        &schCred,                   // auth data
        nullptr, nullptr,           // get key fn / arg
        &m_credHandle,
        nullptr);                   // expiry

    if (status != SEC_E_OK)
    {
        GX_LOG_ERROR("TLSSocket: AcquireCredentialsHandle failed: 0x%08lX", status);
        return false;
    }

    m_credValid = true;
    return true;
}

// ============================================================================
// PerformHandshake — InitializeSecurityContext ループ
// ============================================================================
bool TLSSocket::PerformHandshake(const gx::String& host)
{
    gx::Vector<uint8_t> tokenBuffer(k_InitialRecvBufferSize);
    size_t tokenUsed = 0;
    bool initialCall = true;

    for (;;)
    {
        SecBuffer  inBuffers[2] = {};
        SecBuffer  outBuffers[1] = {};
        SecBufferDesc inBufferDesc;
        SecBufferDesc outBufferDesc;

        // 出力バッファ
        outBuffers[0].BufferType = SECBUFFER_TOKEN;
        outBuffers[0].cbBuffer   = 0;
        outBuffers[0].pvBuffer   = nullptr;

        outBufferDesc.ulVersion = SECBUFFER_VERSION;
        outBufferDesc.cBuffers  = 1;
        outBufferDesc.pBuffers  = outBuffers;

        // 入力バッファ（初回呼び出し以外で使用）
        PSecBufferDesc pInDesc = nullptr;
        if (!initialCall && tokenUsed > 0)
        {
            inBuffers[0].BufferType = SECBUFFER_TOKEN;
            inBuffers[0].cbBuffer   = static_cast<unsigned long>(tokenUsed);
            inBuffers[0].pvBuffer   = tokenBuffer.data();
            inBuffers[1].BufferType = SECBUFFER_EMPTY;
            inBuffers[1].cbBuffer   = 0;
            inBuffers[1].pvBuffer   = nullptr;

            inBufferDesc.ulVersion = SECBUFFER_VERSION;
            inBufferDesc.cBuffers  = 2;
            inBufferDesc.pBuffers  = inBuffers;
            pInDesc = &inBufferDesc;
        }

        DWORD outFlags = 0;
        SECURITY_STATUS status = InitializeSecurityContextA(
            &m_credHandle,
            initialCall ? nullptr : &m_ctxHandle,
            const_cast<SEC_CHAR*>(host.c_str()),
            k_HandshakeFlags,
            0,             // reserved
            0,             // target data rep (not used for stream)
            pInDesc,
            0,             // reserved
            initialCall ? &m_ctxHandle : nullptr,
            &outBufferDesc,
            &outFlags,
            nullptr);      // expiry

        if (initialCall)
        {
            m_ctxValid = true;
            initialCall = false;
        }

        // 出力トークンがあればサーバーに送信
        if (outBuffers[0].pvBuffer && outBuffers[0].cbBuffer > 0)
        {
            int sent = m_socket.Send(outBuffers[0].pvBuffer,
                                      static_cast<int>(outBuffers[0].cbBuffer));
            FreeContextBuffer(outBuffers[0].pvBuffer);
            if (sent <= 0)
            {
                GX_LOG_ERROR("TLSSocket: handshake send failed");
                return false;
            }
        }

        if (status == SEC_E_OK)
        {
            // ハンドシェイク完了
            return true;
        }

        if (status == SEC_I_CONTINUE_NEEDED || status == SEC_E_INCOMPLETE_MESSAGE)
        {
            // Extraバッファを処理（前回の入力の未消費部分）
            if (status == SEC_I_CONTINUE_NEEDED)
            {
                // Extra データがあれば先頭にシフト
                bool hasExtra = false;
                for (int i = 0; i < 2; ++i)
                {
                    if (inBuffers[i].BufferType == SECBUFFER_EXTRA && inBuffers[i].cbBuffer > 0)
                    {
                        std::memmove(tokenBuffer.data(), inBuffers[i].pvBuffer,
                                     inBuffers[i].cbBuffer);
                        tokenUsed = inBuffers[i].cbBuffer;
                        hasExtra = true;
                        break;
                    }
                }
                if (!hasExtra)
                    tokenUsed = 0;
            }

            // サーバーからデータを受信
            if (tokenUsed >= tokenBuffer.size())
                tokenBuffer.resize(tokenBuffer.size() * 2);

            int received = m_socket.Receive(tokenBuffer.data() + tokenUsed,
                                             static_cast<int>(tokenBuffer.size() - tokenUsed));
            if (received <= 0)
            {
                GX_LOG_ERROR("TLSSocket: handshake recv failed (received=%d)", received);
                return false;
            }
            tokenUsed += static_cast<size_t>(received);
            continue;
        }

        if (FAILED(status))
        {
            GX_LOG_ERROR("TLSSocket: InitializeSecurityContext failed: 0x%08lX", status);
            return false;
        }
    }
}

// ============================================================================
// Cleanup — SChannelリソース解放
// ============================================================================
void TLSSocket::Cleanup()
{
    if (m_ctxValid)
    {
        DeleteSecurityContext(&m_ctxHandle);
        SecInvalidateHandle(&m_ctxHandle);
        m_ctxValid = false;
    }
    if (m_credValid)
    {
        FreeCredentialsHandle(&m_credHandle);
        SecInvalidateHandle(&m_credHandle);
        m_credValid = false;
    }
}

} // namespace gx
