#pragma once
/// @file TLSSocket.h
/// @brief TLS暗号化TCPソケット (Windows SChannel)
///
/// TCPSocket をラップし、TLS 1.2/1.3 による暗号化通信を提供する。
/// Windows SChannel API を使用するため外部ライブラリ不要。

#include "pch_common.h"
#include "IO/Network/TCPSocket.h"
#include "Container/ContainerFwd.h"

// Windows Security headers
#define SECURITY_WIN32
#include <sspi.h>
#include <schannel.h>

namespace gx
{
/// @addtogroup grp_network
/// @{

/// @brief TLS暗号化設定
struct TLSConfig
{
    bool verifyServer = true;       ///< サーバー証明書を検証するか
    bool allowTLS12   = true;       ///< TLS 1.2 を許可
    bool allowTLS13   = true;       ///< TLS 1.3 を許可
    gx::String serverName;          ///< サーバー名（証明書検証用）
};

/// @brief TLS暗号化TCPソケット
class TLSSocket
{
public:
    TLSSocket();
    ~TLSSocket();

    /// @brief TLS接続を確立する
    /// @param host ホスト名またはIPアドレス
    /// @param port ポート番号
    /// @param config TLS設定
    /// @return 接続・ハンドシェイクに成功した場合true
    bool Connect(const gx::String& host, uint16_t port, const TLSConfig& config = {});

    /// @brief 接続を閉じる（TLSシャットダウン + TCP切断）
    void Close();

    /// @brief 接続中か
    /// @return TLS接続が確立している場合true
    bool IsConnected() const { return m_connected; }

    /// @brief 暗号化データを送信
    /// @param data 送信データへのポインタ
    /// @param size 送信バイト数
    /// @return 実際に送信されたバイト数 (エラー時は-1)
    int Send(const void* data, int size);

    /// @brief 暗号化データを受信
    /// @param buffer 受信バッファへのポインタ
    /// @param bufferSize バッファサイズ
    /// @return 実際に受信されたバイト数 (エラー時は-1)
    int Receive(void* buffer, int bufferSize);

    /// @brief 復号済みデータが待機中か（内部バッファも確認）
    /// @return データがある場合true
    bool HasData();

    /// @brief 使用中のTLSバージョンを取得
    /// @return TLSバージョン文字列（例: "TLS 1.3"）。未接続時は空文字列
    gx::String GetTLSVersion() const;

    /// @brief ネゴシエート済みの最大メッセージサイズを取得
    /// @return 最大メッセージサイズ（バイト）。未接続時は0
    uint32_t GetMaxMessageSize() const;

    /// @brief 内部TCPソケットへの参照を取得（ノンブロッキング設定等）
    /// @return TCPSocket参照
    TCPSocket& GetSocket() { return m_socket; }
    const TCPSocket& GetSocket() const { return m_socket; }

private:
    /// @brief SChannel資格情報を初期化する
    bool InitializeCredentials(const TLSConfig& config);

    /// @brief TLSハンドシェイクを実行する
    bool PerformHandshake(const gx::String& host);

    /// @brief 全リソースを解放する
    void Cleanup();

    TCPSocket m_socket;                     ///< 内部TCPソケット
    CredHandle m_credHandle = {};            ///< SChannel資格情報ハンドル
    CtxtHandle m_ctxHandle  = {};            ///< SChannelセキュリティコンテキスト
    bool m_connected = false;               ///< TLS接続済みフラグ
    bool m_credValid = false;               ///< 資格情報が有効か
    bool m_ctxValid  = false;               ///< セキュリティコンテキストが有効か

    // SChannel バッファサイズ情報
    SecPkgContext_StreamSizes m_streamSizes = {};

    gx::Vector<uint8_t> m_recvBuffer;       ///< 受信バッファ（暗号化データ蓄積）
    gx::Vector<uint8_t> m_decryptBuffer;    ///< 復号バッファ（復号済みデータ蓄積）
    size_t m_decryptAvail = 0;              ///< 復号済みバイト数
    size_t m_recvUsed     = 0;              ///< 受信バッファ使用量
};

/// @}
} // namespace gx
