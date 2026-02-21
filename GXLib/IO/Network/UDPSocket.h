#pragma once
/// @file UDPSocket.h
/// @brief UDPソケットラッパー — Winsock2 ベース
///
/// バインド・送受信・ノンブロッキング設定をサポートするUDPソケット。

namespace GX {

/// @brief UDPソケット
class UDPSocket
{
public:
    /// @brief UDPソケットを作成する（Winsock初期化も行う）
    UDPSocket();
    /// @brief ソケットを閉じる
    ~UDPSocket();

    /// @brief ローカルポートにバインドする (受信用)
    /// @param port ポート番号
    /// @return バインドに成功した場合true
    bool Bind(uint16_t port);

    /// @brief ソケットを閉じる
    void Close();

    /// @brief 指定ホストにデータを送信する
    /// @param host 送信先ホスト名またはIPアドレス
    /// @param port 送信先ポート番号
    /// @param data 送信データへのポインタ
    /// @param size 送信バイト数
    /// @return 実際に送信されたバイト数 (エラー時は-1)
    int SendTo(const std::string& host, uint16_t port, const void* data, int size);

    /// @brief データを受信する (送信元情報も取得)
    /// @param buffer 受信バッファへのポインタ
    /// @param bufferSize バッファサイズ
    /// @param outHost 送信元ホストの出力先
    /// @param outPort 送信元ポート番号の出力先
    /// @return 実際に受信されたバイト数 (エラー時は-1)
    int ReceiveFrom(void* buffer, int bufferSize, std::string& outHost, uint16_t& outPort);

    /// @brief ノンブロッキングモードの設定
    /// @param nonBlocking trueでノンブロッキング
    void SetNonBlocking(bool nonBlocking);

private:
    SOCKET m_socket = INVALID_SOCKET;
};

} // namespace GX
