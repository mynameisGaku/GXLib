#pragma once
/// @file NetworkReplicator.h
/// @brief ネットワーク状態レプリケーション（エンティティ同期・スナップショット・補間）
///
/// サーバーが定期的にエンティティのトランスフォームをスナップショットとして
/// クライアントに送信し、クライアントはバッファリングと補間で滑らかに表示する。
/// デルタ圧縮により変更のあったエンティティのみを送信する。
/// @addtogroup grp_network/// @{

#include "pch_common.h"
#include "Math/Vector3.h"
#include "Math/Quaternion.h"
#include <functional>

namespace gx
{

class NetworkManager;
class Scene;
class Entity;

/// @brief ネットワークエンティティID型
using NetworkEntityId = uint32_t;

/// @brief ネットワークエンティティの状態
struct NetworkEntityState
{
    NetworkEntityId networkId = 0;  ///< ネットワークID
    Vector3 position;               ///< 位置
    Quaternion rotation;            ///< 回転
    Vector3 scale = { 1.0f, 1.0f, 1.0f };  ///< スケール
    float timestamp = 0.0f;         ///< タイムスタンプ
};

/// @brief スナップショットエントリ
struct SnapshotEntry
{
    NetworkEntityId networkId;  ///< ネットワークID
    Vector3 position;           ///< 位置
    Quaternion rotation;        ///< 回転
    Vector3 scale;              ///< スケール
};

/// @brief ネットワークレプリケーター
///
/// サーバー側: 登録されたエンティティのトランスフォームを定期的にキャプチャし、
/// デルタ圧縮してクライアントに送信する。
/// クライアント側: 受信したスナップショットをバッファリングし、
/// 遅延補間によって滑らかなエンティティ移動を実現する。
class NetworkReplicator
{
public:
    NetworkReplicator();
    ~NetworkReplicator();

    /// @brief レプリケーターを初期化する
    /// @param netManager ネットワークマネージャー
    void Initialize(NetworkManager* netManager);

    /// @brief エンティティを登録する
    /// @param entity 登録するエンティティ
    /// @return 割り当てられたネットワークエンティティID
    NetworkEntityId RegisterEntity(Entity* entity);

    /// @brief エンティティの登録を解除する
    /// @param id ネットワークエンティティID
    void UnregisterEntity(NetworkEntityId id);

    /// @brief サーバー側更新（スナップショット送信）
    /// @param time 現在時刻(秒)
    /// @param dt フレームデルタ時間(秒)
    void ServerUpdate(float time, float dt);

    /// @brief クライアント側更新（補間処理）
    /// @param time 現在時刻(秒)
    /// @param dt フレームデルタ時間(秒)
    void ClientUpdate(float time, float dt);

    /// @brief スナップショット送信レートを設定する(Hz)
    void SetSnapshotRate(float rate) { m_snapshotRate = rate; }

    /// @brief スナップショット送信レートを取得する(Hz)
    float GetSnapshotRate() const { return m_snapshotRate; }

    /// @brief 補間遅延を設定する(秒)
    void SetInterpolationDelay(float delay) { m_interpolationDelay = delay; }

    /// @brief 補間遅延を取得する(秒)
    float GetInterpolationDelay() const { return m_interpolationDelay; }

    /// @brief 登録エンティティ数を取得する
    uint32_t GetRegisteredEntityCount() const { return static_cast<uint32_t>(m_entities.size()); }

    /// @brief ネットワークIDからエンティティを取得する
    /// @param id ネットワークエンティティID
    /// @return エンティティへのポインタ（未登録の場合 nullptr）
    Entity* GetEntityByNetworkId(NetworkEntityId id) const;

    /// @brief スナップショットをシリアライズする
    /// @param entries エントリのリスト
    /// @return バイナリデータ
    gx::Vector<uint8_t> SerializeSnapshot(const gx::Vector<SnapshotEntry>& entries) const;

    /// @brief スナップショットをデシリアライズする
    /// @param data バイナリデータ
    /// @return エントリのリスト
    gx::Vector<SnapshotEntry> DeserializeSnapshot(const gx::Vector<uint8_t>& data) const;

    /// @brief デルタスナップショットをシリアライズする
    /// @param current 現在のスナップショット
    /// @param previous 前回のスナップショット
    /// @return デルタバイナリデータ
    gx::Vector<uint8_t> SerializeDeltaSnapshot(const gx::Vector<SnapshotEntry>& current,
                                                const gx::Vector<SnapshotEntry>& previous) const;

    /// @brief デルタスナップショットを適用する
    /// @param base ベーススナップショット
    /// @param delta デルタバイナリデータ
    /// @return 更新済みスナップショット
    gx::Vector<SnapshotEntry> ApplyDeltaSnapshot(const gx::Vector<SnapshotEntry>& base,
                                                  const gx::Vector<uint8_t>& delta) const;

private:
    /// @brief 登録エンティティ情報
    struct RegisteredEntity
    {
        NetworkEntityId networkId;
        Entity* entity;
    };

    /// @brief 補間バッファ
    struct InterpolationBuffer
    {
        NetworkEntityId networkId;
        gx::Vector<NetworkEntityState> states;
    };

    /// @brief エンティティの補間を実行する
    void InterpolateEntities(float time);

    NetworkManager* m_netManager = nullptr;
    gx::Vector<RegisteredEntity> m_entities;
    gx::Vector<InterpolationBuffer> m_interpolationBuffers;
    gx::Vector<SnapshotEntry> m_lastSnapshot;

    NetworkEntityId m_nextNetworkId = 1;
    float m_snapshotRate = 20.0f;       ///< スナップショット送信レート(Hz)
    float m_snapshotTimer = 0.0f;       ///< スナップショットタイマー(秒)
    float m_interpolationDelay = 0.1f;  ///< 補間遅延(秒)
};

} // namespace gx
/// @}
