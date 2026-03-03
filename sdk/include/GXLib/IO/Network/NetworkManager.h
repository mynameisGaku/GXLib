#pragma once
/// @file NetworkManager.h
/// @brief ネットワークマネージャー（UDP ベース サーバー/クライアント + RPC）
///
/// UDPSocket をバックエンドに使用し、サーバー/クライアントの双方向通信、
/// ブロードキャスト、RPC（リモートプロシージャコール）をサポートする。
/// @addtogroup grp_network/// @{

#include "pch_common.h"
#include "IO/Network/ReliableChannel.h"

namespace gx
{

class UDPSocket;

/// @brief クライアントID型
using ClientId = uint32_t;

/// @brief RPCハンドラ型
using RPCHandler = std::function<void(ClientId sender, const gx::Vector<uint8_t>& data)>;

/// @brief ネットワークマネージャー
class NetworkManager
{
public:
    NetworkManager();
    ~NetworkManager();

    // --- サーバー ---

    /// @brief サーバーとして起動する
    /// @param port 待受ポート番号
    /// @param maxClients 最大接続クライアント数
    /// @return 起動成功時 true
    bool StartServer(uint16_t port, uint32_t maxClients = 16);

    /// @brief サーバーを停止する
    void StopServer();

    /// @brief 全クライアントにデータを送信する
    void Broadcast(const gx::Vector<uint8_t>& data);

    /// @brief 特定クライアントにデータを送信する
    void SendTo(ClientId client, const gx::Vector<uint8_t>& data);

    // --- クライアント ---

    /// @brief サーバーに接続する
    /// @param host ホスト名またはIPアドレス
    /// @param port ポート番号
    /// @return 接続成功時 true
    bool Connect(const gx::String& host, uint16_t port);

    /// @brief サーバーから切断する
    void Disconnect();

    /// @brief サーバーにデータを送信する
    void Send(const gx::Vector<uint8_t>& data);

    /// @brief 接続中かどうか
    bool IsConnected() const { return m_connected; }

    // --- RPC ---

    /// @brief RPCハンドラを登録する
    /// @param name RPC名
    /// @param handler ハンドラ関数
    void RegisterRPC(const gx::String& name, RPCHandler handler);

    /// @brief RPCを呼び出す（サーバー全体 or 接続先サーバー）
    /// @param name RPC名
    /// @param args 引数データ
    void CallRPC(const gx::String& name, const gx::Vector<uint8_t>& args);

    /// @brief 特定クライアントにRPCを呼び出す（サーバーのみ）
    /// @param target 対象クライアントID
    /// @param name RPC名
    /// @param args 引数データ
    void CallRPCOn(ClientId target, const gx::String& name, const gx::Vector<uint8_t>& args);

    // --- 共通 ---

    /// @brief 信頼性のあるデータを送信する
    /// @param host 送信先ホスト
    /// @param port 送信先ポート
    /// @param data 送信データ
    void SendReliable(const gx::String& host, uint16_t port,
                      const gx::Vector<uint8_t>& data);

    /// @brief 信頼性チャネルを取得する
    ReliableChannel& GetReliableChannel() { return m_reliableChannel; }

    /// @brief 受信データをポーリング・ディスパッチする
    void Update();

    /// @brief 接続時コールバックを設定する
    void SetOnConnect(std::function<void(ClientId)> callback) { m_onConnect = std::move(callback); }

    /// @brief 切断時コールバックを設定する
    void SetOnDisconnect(std::function<void(ClientId)> callback) { m_onDisconnect = std::move(callback); }

    /// @brief 接続中のクライアント数を取得する
    uint32_t GetClientCount() const { return static_cast<uint32_t>(m_clients.size()); }

    /// @brief サーバーモードかどうか
    bool IsServer() const { return m_isServer; }

private:
    /// @brief パケットタイプ
    enum class PacketType : uint16_t
    {
        Data      = 0x0001,
        RPC       = 0x0002,
        Connect   = 0x0010,
        Disconnect      = 0x0011,
        Snapshot        = 0x0020,
        SnapshotDelta   = 0x0021,
        InputCommand    = 0x0030,
        StateCorrection = 0x0031,
    };

    /// @brief パケットヘッダ
    struct PacketHeader
    {
        uint32_t magic = 0x47584E54; // "GXNT"
        PacketType type;
        uint16_t size;
    };

    struct ClientInfo
    {
        ClientId id;
        gx::String host;
        uint16_t port;
    };

    static uint32_t HashString(const gx::String& s);

    void SendPacket(const gx::String& host, uint16_t port,
                    PacketType type, const void* data, uint16_t size);
    void ProcessPacket(const gx::String& fromHost, uint16_t fromPort,
                       const uint8_t* data, int size);

    std::unique_ptr<UDPSocket> m_socket;
    bool m_isServer  = false;
    bool m_connected = false;

    // サーバー側
    uint32_t m_maxClients = 16;
    gx::Vector<ClientInfo> m_clients;
    uint32_t m_nextClientId = 1;

    // クライアント側
    gx::String m_serverHost;
    uint16_t m_serverPort = 0;

    // RPC
    gx::HashMap<uint32_t, RPCHandler> m_rpcHandlers;
    gx::HashMap<gx::String, uint32_t> m_rpcNameToHash;

    // コールバック
    std::function<void(ClientId)> m_onConnect;
    std::function<void(ClientId)> m_onDisconnect;

    // 信頼性チャネル
    ReliableChannel m_reliableChannel;
};

} // namespace gx
/// @}
