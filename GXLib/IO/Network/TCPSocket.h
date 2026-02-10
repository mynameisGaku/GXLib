#pragma once
/// @file TCPSocket.h
/// @brief TCPソケットラッパー — Winsock2 ベース
///
/// 接続・送受信・ノンブロッキング設定をサポートするTCPクライアントソケット。

namespace GX {

/// @brief TCPクライアントソケット
class TCPSocket
{
public:
    TCPSocket();
    ~TCPSocket();

    /// @brief サーバーに接続する
    /// @param host ホスト名またはIPアドレス
    /// @param port ポート番号
    /// @return 接続に成功した場合true
    bool Connect(const std::string& host, uint16_t port);

    /// @brief ソケットを閉じる
    void Close();

    /// @brief 接続中かどうか判定する
    /// @return 接続中の場合true
    bool IsConnected() const;

    /// @brief データを送信する
    /// @param data 送信データへのポインタ
    /// @param size 送信バイト数
    /// @return 実際に送信されたバイト数 (エラー時は-1)
    int Send(const void* data, int size);

    /// @brief データを受信する
    /// @param buffer 受信バッファへのポインタ
    /// @param bufferSize バッファサイズ
    /// @return 実際に受信されたバイト数 (エラー時は-1、切断時は0)
    int Receive(void* buffer, int bufferSize);

    /// @brief ノンブロッキングモードの設定
    /// @param nonBlocking trueでノンブロッキング
    void SetNonBlocking(bool nonBlocking);

    /// @brief 読み取り可能なデータがあるか判定する (ノンブロッキング)
    /// @return データがある場合true
    bool HasData() const;

private:
    SOCKET m_socket = INVALID_SOCKET;
    static bool s_wsaInitialized;
    static void EnsureWSAInit();
};

} // namespace GX
