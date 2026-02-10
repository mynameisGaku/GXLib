#pragma once
/// @file WebSocket.h
/// @brief WebSocketクライアント — WinHTTP WebSocket API ベース
///
/// WebSocket接続・テキスト/バイナリ送受信をサポートする。
/// 受信は別スレッドで行い、Update() でメインスレッドにコールバックを発火する。

namespace GX {

/// @brief WebSocketクライアント
class WebSocket
{
public:
    WebSocket();
    ~WebSocket();

    /// @brief WebSocketサーバーに接続する
    /// @param url WebSocket URL (ws:// または wss://)
    /// @return 接続に成功した場合true
    bool Connect(const std::string& url);

    /// @brief 接続を閉じる
    void Close();

    /// @brief 接続中かどうか判定する
    /// @return 接続中の場合true
    bool IsConnected() const;

    /// @brief テキストメッセージを送信する
    /// @param message UTF-8テキスト
    /// @return 送信に成功した場合true
    bool Send(const std::string& message);

    /// @brief バイナリデータを送信する
    /// @param data データポインタ
    /// @param size データサイズ (バイト)
    /// @return 送信に成功した場合true
    bool SendBinary(const void* data, size_t size);

    /// @brief 受信メッセージのコールバックを発火する (メインスレッドで毎フレーム呼ぶ)
    void Update();

    /// @brief テキストメッセージ受信時のコールバック
    std::function<void(const std::string&)> onMessage;
    /// @brief バイナリメッセージ受信時のコールバック
    std::function<void(const void*, size_t)> onBinaryMessage;
    /// @brief 接続終了時のコールバック
    std::function<void()> onClose;
    /// @brief エラー発生時のコールバック
    std::function<void(const std::string&)> onError;

private:
    void* m_hSession = nullptr;
    void* m_hConnect = nullptr;
    void* m_hWebSocket = nullptr;

    std::thread m_receiveThread;
    std::mutex m_mutex;
    std::atomic<bool> m_running{ false };

    struct QueuedMessage {
        enum Type { Text, Binary, Closed, Error };
        Type type;
        std::string data;
    };
    std::vector<QueuedMessage> m_messageQueue;

    void ReceiveLoop();
};

} // namespace GX
