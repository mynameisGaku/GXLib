#pragma once
/// @file NATTraversal.h
/// @brief NAT越え (STUN + UDP hole punching)
///
/// P2P接続を確立するためのNATトラバーサル機能。
/// STUN (RFC 5389) プロトコルでパブリックアドレスを検出し、
/// UDPホールパンチングでNAT越えを試みる。
/// パンチング失敗時はリレーサーバーへフォールバックする。
/// @addtogroup grp_network/// @{

#include "pch_common.h"
#include <string>
#include <functional>
#include <vector>
#include <cstdint>

namespace gx
{

/// @brief NAT状態
enum class NATState
{
    Idle,         ///< 待機中
    Discovering,  ///< STUN Discovery実行中
    Punching,     ///< UDPホールパンチング中
    Connected,    ///< 接続成功
    Relaying,     ///< リレー経由で接続中
    Failed        ///< 接続失敗
};

/// @brief NAT種別
enum class NATType
{
    Unknown,         ///< 不明
    OpenInternet,    ///< オープンインターネット（NATなし）
    FullCone,        ///< フルコーンNAT
    RestrictedCone,  ///< リストリクテッドコーンNAT
    PortRestricted,  ///< ポートリストリクテッドNAT
    Symmetric        ///< シンメトリックNAT（P2P困難）
};

/// @brief エンドポイント情報
struct NATEndpoint
{
    gx::String host;    ///< ホスト名またはIPアドレス
    uint16_t port = 0;   ///< ポート番号
};

/// @brief STUN Binding応答
struct STUNResult
{
    bool success = false;
    NATEndpoint mappedEndpoint;   ///< NATマッピング後のパブリックアドレス
    NATType natType = NATType::Unknown;
};

/// @brief NAT越え設定
struct NATConfig
{
    gx::String stunServer = "stun.l.google.com";  ///< STUNサーバーホスト名
    uint16_t stunPort = 19302;                     ///< STUNサーバーポート
    uint32_t punchAttempts = 10;                   ///< パンチング試行回数
    uint32_t punchIntervalMs = 200;                ///< パンチング間隔（ミリ秒）
    uint32_t timeoutMs = 5000;                     ///< タイムアウト（ミリ秒）
    bool enableRelay = true;                       ///< リレーフォールバックを有効にするか
};

/// @brief NAT越えシステム
class NATTraversal
{
public:
    NATTraversal() = default;
    ~NATTraversal() = default;

    /// @brief 初期化
    void Initialize(const NATConfig& config = {});

    /// @brief 設定取得
    const NATConfig& GetConfig() const { return m_config; }

    /// @brief 現在の状態
    NATState GetState() const { return m_state; }

    /// @brief STUN Discoveryを開始
    void StartDiscovery();

    /// @brief UDPホールパンチングを開始
    void StartPunching(const NATEndpoint& peerPublicEndpoint);

    /// @brief 更新 (ポーリング)
    void Update();

    /// @brief STUNリクエストを構築
    static gx::Vector<uint8_t> BuildSTUNBindingRequest();

    /// @brief STUN応答をパース
    static STUNResult ParseSTUNResponse(const uint8_t* data, size_t size);

    /// @brief パブリックエンドポイント取得
    NATEndpoint GetPublicEndpoint() const { return m_publicEndpoint; }

    /// @brief NAT種別取得
    NATType GetNATType() const { return m_natType; }

    /// @brief ホールパンチ試行回数
    uint32_t GetPunchAttemptCount() const { return m_punchAttemptCount; }

    /// @brief リレーフォールバック
    void StartRelay(const NATEndpoint& relayServer);

    /// @brief 接続成功か
    bool IsConnected() const { return m_state == NATState::Connected || m_state == NATState::Relaying; }

    /// @brief リセット
    void Reset();

    /// @brief コールバック
    using StateCallback = std::function<void(NATState)>;
    void SetOnStateChanged(StateCallback cb) { m_onStateChanged = std::move(cb); }

private:
    void SetState(NATState newState);

    NATConfig m_config;                       ///< NAT越え設定
    NATState m_state = NATState::Idle;        ///< 現在の状態
    NATType m_natType = NATType::Unknown;     ///< 検出されたNAT種別
    NATEndpoint m_publicEndpoint;             ///< STUN検出されたパブリックエンドポイント
    NATEndpoint m_peerEndpoint;               ///< ピアのパブリックエンドポイント
    NATEndpoint m_relayEndpoint;              ///< リレーサーバーエンドポイント
    uint32_t m_punchAttemptCount = 0;         ///< 現在のパンチング試行回数
    StateCallback m_onStateChanged;           ///< 状態変化コールバック
};

} // namespace gx
/// @}
