#pragma once
/// @file ReliableChannel.h
/// @brief 信頼性のあるUDPチャネル（ACK・再送制御・RTT計測）
///
/// UDPSocketの上に信頼性レイヤーを構築する。シーケンス番号、ACKビットフィールド、
/// 指数バックオフ付き再送制御、RTT計測、パケットロス統計を提供する。
/// @addtogroup grp_network/// @{

#include "pch_common.h"

namespace gx
{

class UDPSocket;

/// @brief 信頼性パケットヘッダ
struct ReliablePacketHeader
{
    uint32_t magic = 0x47585250;  ///< マジック "GXRP"
    uint16_t type = 0;            ///< パケットタイプ
    uint16_t size = 0;            ///< ペイロードサイズ
    uint16_t sequence = 0;        ///< シーケンス番号
    uint16_t ack = 0;             ///< 最新受信シーケンス番号
    uint32_t ackBits = 0;         ///< 直近32個の受信確認ビットフィールド
};

/// @brief 信頼性チャネル設定
struct ReliableConfig
{
    float retransmitTimeMs = 100.0f;      ///< 初期再送タイムアウト(ms)
    float maxRetransmitTimeMs = 1000.0f;  ///< 最大再送タイムアウト(ms)
    uint32_t maxRetransmits = 10;         ///< 最大再送回数
    uint32_t sequenceBufferSize = 1024;   ///< 受信シーケンスバッファサイズ
};

/// @brief 受信済みパケット情報
struct ReceivedPacket
{
    uint16_t type;                ///< パケットタイプ
    std::vector<uint8_t> data;   ///< ペイロードデータ
    std::string fromHost;        ///< 送信元ホスト
    uint16_t fromPort;           ///< 送信元ポート
};

/// @brief 信頼性のあるUDPチャネル
///
/// ACK・再送制御によりUDP上で信頼性のあるパケット配信を実現する。
/// 各パケットにシーケンス番号を付与し、受信側はACKビットフィールドで
/// 直近32パケットの受信状況を通知する。未ACKパケットは指数バックオフで再送される。
class ReliableChannel
{
public:
    ReliableChannel();
    ~ReliableChannel();

    /// @brief チャネルを初期化する
    /// @param socket 使用するUDPソケット
    /// @param config 設定パラメータ
    void Initialize(UDPSocket* socket, const ReliableConfig& config = {});

    /// @brief 信頼性のあるパケットを送信する（ACK・再送制御あり）
    /// @param host 送信先ホスト
    /// @param port 送信先ポート
    /// @param type パケットタイプ
    /// @param data ペイロードデータ
    /// @param size データサイズ
    void SendReliable(const std::string& host, uint16_t port,
                      uint16_t type, const void* data, uint32_t size);

    /// @brief 信頼性のないパケットを送信する（ACK情報は付与する）
    /// @param host 送信先ホスト
    /// @param port 送信先ポート
    /// @param type パケットタイプ
    /// @param data ペイロードデータ
    /// @param size データサイズ
    void SendUnreliable(const std::string& host, uint16_t port,
                        uint16_t type, const void* data, uint32_t size);

    /// @brief 受信データを処理する
    /// @param rawData 受信した生データ
    /// @param size データサイズ
    /// @param host 送信元ホスト
    /// @param port 送信元ポート
    /// @return 処理済みパケットのリスト
    std::vector<ReceivedPacket> ProcessIncoming(const uint8_t* rawData, uint32_t size,
                                                const std::string& host, uint16_t port);

    /// @brief 再送タイマーを更新する
    /// @param deltaTimeMs 経過時間(ms)
    void Update(float deltaTimeMs);

    /// @brief RTT（往復遅延時間）を取得する(ms)
    float GetRTT() const { return m_rtt; }

    /// @brief パケットロス率を取得する (0.0〜1.0)
    float GetPacketLoss() const { return m_packetLoss; }

    /// @brief 送信パケット数を取得する
    uint32_t GetSentCount() const { return m_sentCount; }

    /// @brief 受信パケット数を取得する
    uint32_t GetReceivedCount() const { return m_receivedCount; }

private:
    /// @brief 送信待ちパケット（ACK待ち）
    struct PendingPacket
    {
        uint16_t sequence;          ///< シーケンス番号
        uint16_t type;              ///< パケットタイプ
        std::vector<uint8_t> data;  ///< ペイロードデータ
        std::string host;           ///< 送信先ホスト
        uint16_t port;              ///< 送信先ポート
        float timeSentMs;           ///< 送信時刻(ms)
        float retransmitTimer;      ///< 再送タイマー(ms)
        uint32_t retransmitCount;   ///< 再送回数
        bool acked;                 ///< ACK済みか
    };

    /// @brief 生パケットを送信する
    void SendRaw(const std::string& host, uint16_t port,
                 const ReliablePacketHeader& header, const void* data, uint32_t size);

    /// @brief ACK情報を処理する
    void ProcessAcks(uint16_t ack, uint32_t ackBits);

    /// @brief シーケンス番号の新しさを比較する（ラップアラウンド対応）
    bool IsSequenceNewer(uint16_t s1, uint16_t s2) const;

    UDPSocket* m_socket = nullptr;
    ReliableConfig m_config;

    uint16_t m_localSequence = 0;       ///< ローカル送信シーケンス番号
    uint16_t m_remoteSequence = 0;      ///< 最新受信シーケンス番号
    uint32_t m_receivedBits = 0;        ///< 受信確認ビットフィールド

    std::vector<PendingPacket> m_pendingPackets;    ///< ACK待ちパケット
    std::vector<uint16_t> m_receivedSequences;      ///< 受信済みシーケンスリスト

    float m_rtt = 0.0f;                ///< 往復遅延時間(ms)
    float m_packetLoss = 0.0f;         ///< パケットロス率
    uint32_t m_sentCount = 0;          ///< 累計送信パケット数
    uint32_t m_receivedCount = 0;      ///< 累計受信パケット数
    uint32_t m_ackedCount = 0;         ///< 累計ACK済みパケット数
    float m_elapsedMs = 0.0f;          ///< 累計経過時間(ms)
};

} // namespace gx
/// @}
