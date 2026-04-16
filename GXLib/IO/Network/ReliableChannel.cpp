/// @file ReliableChannel.cpp
/// @brief 信頼性のあるUDPチャネル実装
#include "pch_common.h"
#include "IO/Network/ReliableChannel.h"
#include "IO/Network/UDPSocket.h"
#include "Core/Logger.h"
#include <cstring>
#include <algorithm>

namespace gx
{

// ---------------------------------------------------------------------------
// コンストラクタ / デストラクタ
// ---------------------------------------------------------------------------

ReliableChannel::ReliableChannel() = default;
ReliableChannel::~ReliableChannel() = default;

// ---------------------------------------------------------------------------
// 初期化
// ---------------------------------------------------------------------------

void ReliableChannel::Initialize(UDPSocket* socket, const ReliableConfig& config)
{
    m_socket = socket;
    m_config = config;

    m_localSequence = 0;
    m_remoteSequence = 0;
    m_receivedBits = 0;

    m_pendingPackets.clear();
    m_receivedSequences.clear();
    m_receivedSequences.reserve(m_config.sequenceBufferSize);

    m_rtt = 0.0f;
    m_packetLoss = 0.0f;
    m_sentCount = 0;
    m_receivedCount = 0;
    m_ackedCount = 0;
    m_elapsedMs = 0.0f;

    GX_LOG_INFO("[ReliableChannel] Initialized (retransmit=%.0fms, maxRetransmits=%u)",
                m_config.retransmitTimeMs, m_config.maxRetransmits);
}

// ---------------------------------------------------------------------------
// シーケンス番号比較（ラップアラウンド対応）
// ---------------------------------------------------------------------------

bool ReliableChannel::IsSequenceNewer(uint16_t s1, uint16_t s2) const
{
    // s1 が s2 より新しい場合 true を返す
    // 差が半分の範囲（32768）以内であれば、s1 > s2 とみなす
    return ((s1 > s2) && (s1 - s2 <= 32768)) ||
           ((s1 < s2) && (s2 - s1 > 32768));
}

// ---------------------------------------------------------------------------
// 生パケット送信
// ---------------------------------------------------------------------------

void ReliableChannel::SendRaw(const gx::String& host, uint16_t port,
                               const ReliablePacketHeader& header,
                               const void* data, uint32_t size)
{
    if (!m_socket) return;

    // ヘッダ + ペイロードをバッファに組み立て
    const uint32_t totalSize = static_cast<uint32_t>(sizeof(ReliablePacketHeader)) + size;
    gx::Vector<uint8_t> buffer(totalSize);
    std::memcpy(buffer.data(), &header, sizeof(ReliablePacketHeader));
    if (data && size > 0)
    {
        std::memcpy(buffer.data() + sizeof(ReliablePacketHeader), data, size);
    }

    m_socket->SendTo(host, port, buffer.data(), static_cast<int>(totalSize));
}

// ---------------------------------------------------------------------------
// 信頼性のある送信
// ---------------------------------------------------------------------------

void ReliableChannel::SendReliable(const gx::String& host, uint16_t port,
                                    uint16_t type, const void* data, uint32_t size)
{
    if (!m_socket) return;

    uint16_t seq = m_localSequence++;

    // ヘッダ構築
    ReliablePacketHeader header;
    header.magic = 0x47585250;
    header.type = type;
    header.size = static_cast<uint16_t>(size);
    header.sequence = seq;
    header.ack = m_remoteSequence;
    header.ackBits = m_receivedBits;

    // 送信
    SendRaw(host, port, header, data, size);
    m_sentCount++;

    // 保留パケットとして記録（ACK待ち）
    PendingPacket pending;
    pending.sequence = seq;
    pending.type = type;
    if (data && size > 0)
    {
        pending.data.assign(static_cast<const uint8_t*>(data),
                            static_cast<const uint8_t*>(data) + size);
    }
    pending.host = host;
    pending.port = port;
    pending.timeSentMs = m_elapsedMs;
    pending.retransmitTimer = m_config.retransmitTimeMs;
    pending.retransmitCount = 0;
    pending.acked = false;

    m_pendingPackets.push_back(std::move(pending));
}

// ---------------------------------------------------------------------------
// 信頼性のない送信（ACK情報は付与）
// ---------------------------------------------------------------------------

void ReliableChannel::SendUnreliable(const gx::String& host, uint16_t port,
                                      uint16_t type, const void* data, uint32_t size)
{
    if (!m_socket) return;

    uint16_t seq = m_localSequence++;

    ReliablePacketHeader header;
    header.magic = 0x47585250;
    header.type = type;
    header.size = static_cast<uint16_t>(size);
    header.sequence = seq;
    header.ack = m_remoteSequence;
    header.ackBits = m_receivedBits;

    SendRaw(host, port, header, data, size);
    m_sentCount++;
}

// ---------------------------------------------------------------------------
// 受信処理
// ---------------------------------------------------------------------------

gx::Vector<ReceivedPacket> ReliableChannel::ProcessIncoming(
    const uint8_t* rawData, uint32_t size,
    const gx::String& host, uint16_t port)
{
    gx::Vector<ReceivedPacket> result;

    if (!rawData || size < sizeof(ReliablePacketHeader))
    {
        return result;
    }

    // ヘッダを読み取り
    ReliablePacketHeader header;
    std::memcpy(&header, rawData, sizeof(ReliablePacketHeader));

    // マジック検証
    if (header.magic != 0x47585250)
    {
        return result;
    }

    // サイズ検証
    if (sizeof(ReliablePacketHeader) + header.size > size)
    {
        return result;
    }

    m_receivedCount++;

    // --- 相手のACK情報を処理（自分の送信パケットの確認応答） ---
    ProcessAcks(header.ack, header.ackBits);

    // --- 受信シーケンス番号を記録 ---
    uint16_t incomingSeq = header.sequence;

    // 重複チェック
    bool isDuplicate = false;
    for (uint16_t recvSeq : m_receivedSequences)
    {
        if (recvSeq == incomingSeq)
        {
            isDuplicate = true;
            break;
        }
    }

    if (!isDuplicate)
    {
        // 受信シーケンスバッファに追加
        m_receivedSequences.push_back(incomingSeq);

        // バッファサイズ制限
        if (m_receivedSequences.size() > m_config.sequenceBufferSize)
        {
            m_receivedSequences.erase(m_receivedSequences.begin());
        }

        // リモートシーケンスとACKビットフィールドを更新
        if (IsSequenceNewer(incomingSeq, m_remoteSequence))
        {
            // 新しいシーケンスが来た — ビットフィールドをシフト
            int diff = static_cast<int>(incomingSeq) - static_cast<int>(m_remoteSequence);
            if (diff < 0) diff += 65536;  // ラップアラウンド補正

            if (diff <= 32)
            {
                m_receivedBits <<= diff;
                m_receivedBits |= (1u << (diff - 1));  // 旧remoteSequenceのビットを立てる
            }
            else
            {
                m_receivedBits = 0;  // 差が大きすぎる場合はリセット
            }

            m_remoteSequence = incomingSeq;
        }
        else
        {
            // 古いパケット — 対応するビットを立てる
            int diff = static_cast<int>(m_remoteSequence) - static_cast<int>(incomingSeq);
            if (diff < 0) diff += 65536;

            if (diff > 0 && diff <= 32)
            {
                m_receivedBits |= (1u << (diff - 1));
            }
        }

        // ペイロードを出力に追加
        ReceivedPacket pkt;
        pkt.type = header.type;
        pkt.fromHost = host;
        pkt.fromPort = port;
        if (header.size > 0)
        {
            const uint8_t* payload = rawData + sizeof(ReliablePacketHeader);
            pkt.data.assign(payload, payload + header.size);
        }
        result.push_back(std::move(pkt));
    }

    return result;
}

// ---------------------------------------------------------------------------
// ACK処理
// ---------------------------------------------------------------------------

void ReliableChannel::ProcessAcks(uint16_t ack, uint32_t ackBits)
{
    for (auto& pending : m_pendingPackets)
    {
        if (pending.acked) continue;

        // 直接ACK
        if (pending.sequence == ack)
        {
            pending.acked = true;
            m_ackedCount++;

            // RTT計算（EMA: 指数移動平均）
            float sampleRtt = m_elapsedMs - pending.timeSentMs;
            if (m_rtt <= 0.0f)
            {
                m_rtt = sampleRtt;
            }
            else
            {
                const float alpha = 0.1f;
                m_rtt = m_rtt * (1.0f - alpha) + sampleRtt * alpha;
            }
            continue;
        }

        // ビットフィールドACK
        int diff = static_cast<int>(ack) - static_cast<int>(pending.sequence);
        if (diff < 0) diff += 65536;

        if (diff > 0 && diff <= 32)
        {
            if (ackBits & (1u << (diff - 1)))
            {
                pending.acked = true;
                m_ackedCount++;

                // RTT計算
                float sampleRtt = m_elapsedMs - pending.timeSentMs;
                if (m_rtt <= 0.0f)
                {
                    m_rtt = sampleRtt;
                }
                else
                {
                    const float alpha = 0.1f;
                    m_rtt = m_rtt * (1.0f - alpha) + sampleRtt * alpha;
                }
            }
        }
    }

    // ACK済みパケットをクリーンアップ
    m_pendingPackets.erase(
        std::remove_if(m_pendingPackets.begin(), m_pendingPackets.end(),
                        [](const PendingPacket& p) { return p.acked; }),
        m_pendingPackets.end());

    // パケットロス率の更新
    if (m_sentCount > 0)
    {
        uint32_t lostCount = m_sentCount - m_ackedCount;
        // 保留中はまだロストとは限らないので、確定済みのみで計算
        uint32_t resolvedCount = m_ackedCount;
        for (const auto& p : m_pendingPackets)
        {
            if (p.retransmitCount >= m_config.maxRetransmits)
            {
                lostCount++;
                resolvedCount++;
            }
        }

        if (resolvedCount > 0)
        {
            m_packetLoss = 1.0f - static_cast<float>(m_ackedCount) / static_cast<float>(resolvedCount);
        }
    }
}

// ---------------------------------------------------------------------------
// 更新（再送制御）
// ---------------------------------------------------------------------------

void ReliableChannel::Update(float deltaTimeMs)
{
    m_elapsedMs += deltaTimeMs;

    for (auto it = m_pendingPackets.begin(); it != m_pendingPackets.end(); )
    {
        if (it->acked)
        {
            it = m_pendingPackets.erase(it);
            continue;
        }

        it->retransmitTimer -= deltaTimeMs;

        if (it->retransmitTimer <= 0.0f)
        {
            if (it->retransmitCount >= m_config.maxRetransmits)
            {
                // 最大再送回数超過 — パケットを破棄
                GX_LOG_WARN("[ReliableChannel] Packet seq=%u dropped after %u retransmits",
                            it->sequence, it->retransmitCount);
                it = m_pendingPackets.erase(it);
                continue;
            }

            // 再送
            ReliablePacketHeader header;
            header.magic = 0x47585250;
            header.type = it->type;
            header.size = static_cast<uint16_t>(it->data.size());
            header.sequence = it->sequence;
            header.ack = m_remoteSequence;
            header.ackBits = m_receivedBits;

            SendRaw(it->host, it->port, header,
                    it->data.empty() ? nullptr : it->data.data(),
                    static_cast<uint32_t>(it->data.size()));

            it->retransmitCount++;
            it->timeSentMs = m_elapsedMs;

            // 指数バックオフ: タイマーを2倍にする (上限あり)
            float newTimeout = m_config.retransmitTimeMs *
                               static_cast<float>(1u << it->retransmitCount);
            if (newTimeout > m_config.maxRetransmitTimeMs)
            {
                newTimeout = m_config.maxRetransmitTimeMs;
            }
            it->retransmitTimer = newTimeout;
        }

        ++it;
    }
}

} // namespace gx
