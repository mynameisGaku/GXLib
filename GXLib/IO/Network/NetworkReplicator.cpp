/// @file NetworkReplicator.cpp
/// @brief ネットワーク状態レプリケーション実装
#include "pch_common.h"
#include "IO/Network/NetworkReplicator.h"
#include "IO/Network/NetworkManager.h"
#include "Core/Scene/Entity.h"
#include "Core/Logger.h"
#include <cstring>
#include <algorithm>

namespace gx
{

// ---------------------------------------------------------------------------
// 定数
// ---------------------------------------------------------------------------

/// @brief 1エントリあたりのバイナリサイズ
/// networkId(4) + pos.xyz(12) + rot.xyzw(16) + scale.xyz(12) = 44 bytes
static constexpr uint32_t k_EntryBinarySize = 44;

/// @brief デルタフラグ
static constexpr uint8_t k_DeltaFlagPosition = 0x01;
static constexpr uint8_t k_DeltaFlagRotation = 0x02;
static constexpr uint8_t k_DeltaFlagScale    = 0x04;

// ---------------------------------------------------------------------------
// コンストラクタ / デストラクタ
// ---------------------------------------------------------------------------

NetworkReplicator::NetworkReplicator() = default;
NetworkReplicator::~NetworkReplicator() = default;

// ---------------------------------------------------------------------------
// 初期化
// ---------------------------------------------------------------------------

void NetworkReplicator::Initialize(NetworkManager* netManager)
{
    m_netManager = netManager;
    m_entities.clear();
    m_interpolationBuffers.clear();
    m_lastSnapshot.clear();
    m_nextNetworkId = 1;
    m_snapshotTimer = 0.0f;

    GX_LOG_INFO("[NetworkReplicator] Initialized (snapshotRate=%.1fHz, interpDelay=%.3fs)",
                m_snapshotRate, m_interpolationDelay);
}

// ---------------------------------------------------------------------------
// エンティティ登録 / 解除
// ---------------------------------------------------------------------------

NetworkEntityId NetworkReplicator::RegisterEntity(Entity* entity)
{
    if (!entity) return 0;

    // 既に登録済みかチェック
    for (const auto& reg : m_entities)
    {
        if (reg.entity == entity)
        {
            return reg.networkId;
        }
    }

    NetworkEntityId id = m_nextNetworkId++;

    RegisteredEntity reg;
    reg.networkId = id;
    reg.entity = entity;
    m_entities.push_back(reg);

    // エンティティにネットワークIDを設定
    entity->SetID(id);

    // 補間バッファを作成
    InterpolationBuffer buf;
    buf.networkId = id;
    m_interpolationBuffers.push_back(std::move(buf));

    GX_LOG_INFO("[NetworkReplicator] Entity '%s' registered with networkId=%u",
                entity->GetName().c_str(), id);

    return id;
}

void NetworkReplicator::UnregisterEntity(NetworkEntityId id)
{
    // エンティティリストから除去
    m_entities.erase(
        std::remove_if(m_entities.begin(), m_entities.end(),
                        [id](const RegisteredEntity& r) { return r.networkId == id; }),
        m_entities.end());

    // 補間バッファから除去
    m_interpolationBuffers.erase(
        std::remove_if(m_interpolationBuffers.begin(), m_interpolationBuffers.end(),
                        [id](const InterpolationBuffer& b) { return b.networkId == id; }),
        m_interpolationBuffers.end());

    // 前回スナップショットから除去
    m_lastSnapshot.erase(
        std::remove_if(m_lastSnapshot.begin(), m_lastSnapshot.end(),
                        [id](const SnapshotEntry& e) { return e.networkId == id; }),
        m_lastSnapshot.end());
}

// ---------------------------------------------------------------------------
// ネットワークIDからエンティティを取得
// ---------------------------------------------------------------------------

Entity* NetworkReplicator::GetEntityByNetworkId(NetworkEntityId id) const
{
    for (const auto& reg : m_entities)
    {
        if (reg.networkId == id) return reg.entity;
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// サーバー側更新
// ---------------------------------------------------------------------------

void NetworkReplicator::ServerUpdate(float time, float dt)
{
    if (!m_netManager || !m_netManager->IsServer()) return;

    m_snapshotTimer += dt;
    float snapshotInterval = 1.0f / m_snapshotRate;

    if (m_snapshotTimer < snapshotInterval) return;
    m_snapshotTimer -= snapshotInterval;

    // 現在のスナップショットをキャプチャ
    gx::Vector<SnapshotEntry> currentSnapshot;
    currentSnapshot.reserve(m_entities.size());

    for (const auto& reg : m_entities)
    {
        if (!reg.entity || !reg.entity->IsActive()) continue;

        SnapshotEntry entry;
        entry.networkId = reg.networkId;

        const auto& pos = reg.entity->GetTransform().GetPosition();
        entry.position = Vector3(pos.x, pos.y, pos.z);

        const auto& rot = reg.entity->GetTransform().GetRotation();
        // オイラー角からQuaternionに変換
        entry.rotation = Quaternion::FromEuler(rot.x, rot.y, rot.z);

        const auto& scl = reg.entity->GetTransform().GetScale();
        entry.scale = Vector3(scl.x, scl.y, scl.z);

        currentSnapshot.push_back(entry);
    }

    // デルタスナップショットをシリアライズ
    gx::Vector<uint8_t> data;
    if (m_lastSnapshot.empty())
    {
        data = SerializeSnapshot(currentSnapshot);
    }
    else
    {
        data = SerializeDeltaSnapshot(currentSnapshot, m_lastSnapshot);
    }

    // 全クライアントにブロードキャスト
    if (!data.empty())
    {
        m_netManager->Broadcast(data);
    }

    m_lastSnapshot = std::move(currentSnapshot);
}

// ---------------------------------------------------------------------------
// クライアント側更新
// ---------------------------------------------------------------------------

void NetworkReplicator::ClientUpdate(float time, float dt)
{
    (void)dt;

    // 補間を実行
    InterpolateEntities(time);
}

// ---------------------------------------------------------------------------
// エンティティ補間
// ---------------------------------------------------------------------------

void NetworkReplicator::InterpolateEntities(float time)
{
    // 補間ターゲット時刻 = 現在時刻 - 遅延
    float renderTime = time - m_interpolationDelay;

    for (auto& buf : m_interpolationBuffers)
    {
        if (buf.states.size() < 2) continue;

        // renderTime を挟む2つの状態を探す
        const NetworkEntityState* from = nullptr;
        const NetworkEntityState* to   = nullptr;

        for (size_t i = 0; i + 1 < buf.states.size(); ++i)
        {
            if (buf.states[i].timestamp <= renderTime &&
                buf.states[i + 1].timestamp >= renderTime)
            {
                from = &buf.states[i];
                to   = &buf.states[i + 1];
                break;
            }
        }

        if (!from || !to) continue;

        // 補間係数を計算
        float duration = to->timestamp - from->timestamp;
        float t = 0.0f;
        if (duration > 0.0f)
        {
            t = (renderTime - from->timestamp) / duration;
            t = (std::max)(0.0f, (std::min)(1.0f, t));
        }

        // 対応するエンティティを取得
        Entity* entity = GetEntityByNetworkId(buf.networkId);
        if (!entity) continue;

        // 位置を線形補間
        Vector3 interpPos = Vector3::Lerp(from->position, to->position, t);

        // 回転を球面線形補間
        Quaternion interpRot = Quaternion::Slerp(from->rotation, to->rotation, t);

        // スケールを線形補間
        Vector3 interpScale = Vector3::Lerp(from->scale, to->scale, t);

        // エンティティに適用
        entity->GetTransform().SetPosition(interpPos.x, interpPos.y, interpPos.z);

        // Quaternionからオイラー角に変換して設定
        Vector3 euler = interpRot.ToEuler();
        entity->GetTransform().SetRotation(euler.x, euler.y, euler.z);

        entity->GetTransform().SetScale(interpScale.x, interpScale.y, interpScale.z);

        // 古い状態をクリーンアップ（renderTimeより古い状態は1つだけ残す）
        while (buf.states.size() > 2 && buf.states[1].timestamp < renderTime)
        {
            buf.states.erase(buf.states.begin());
        }
    }
}

// ---------------------------------------------------------------------------
// スナップショット シリアライズ / デシリアライズ
// ---------------------------------------------------------------------------

gx::Vector<uint8_t> NetworkReplicator::SerializeSnapshot(
    const gx::Vector<SnapshotEntry>& entries) const
{
    // フォーマット: [entryCount(4)] + [entry * N]
    // entry: [networkId(4)][pos.x(4)][pos.y(4)][pos.z(4)]
    //        [rot.x(4)][rot.y(4)][rot.z(4)][rot.w(4)]
    //        [scale.x(4)][scale.y(4)][scale.z(4)]
    uint32_t entryCount = static_cast<uint32_t>(entries.size());
    gx::Vector<uint8_t> data(4 + k_EntryBinarySize * entryCount);

    uint8_t* ptr = data.data();
    std::memcpy(ptr, &entryCount, 4);
    ptr += 4;

    for (const auto& entry : entries)
    {
        std::memcpy(ptr, &entry.networkId, 4); ptr += 4;
        std::memcpy(ptr, &entry.position.x, 4); ptr += 4;
        std::memcpy(ptr, &entry.position.y, 4); ptr += 4;
        std::memcpy(ptr, &entry.position.z, 4); ptr += 4;
        std::memcpy(ptr, &entry.rotation.x, 4); ptr += 4;
        std::memcpy(ptr, &entry.rotation.y, 4); ptr += 4;
        std::memcpy(ptr, &entry.rotation.z, 4); ptr += 4;
        std::memcpy(ptr, &entry.rotation.w, 4); ptr += 4;
        std::memcpy(ptr, &entry.scale.x, 4); ptr += 4;
        std::memcpy(ptr, &entry.scale.y, 4); ptr += 4;
        std::memcpy(ptr, &entry.scale.z, 4); ptr += 4;
    }

    return data;
}

gx::Vector<SnapshotEntry> NetworkReplicator::DeserializeSnapshot(
    const gx::Vector<uint8_t>& data) const
{
    gx::Vector<SnapshotEntry> result;

    if (data.size() < 4) return result;

    const uint8_t* ptr = data.data();
    uint32_t entryCount = 0;
    std::memcpy(&entryCount, ptr, 4);
    ptr += 4;

    if (data.size() < 4 + k_EntryBinarySize * entryCount) return result;

    result.reserve(entryCount);
    for (uint32_t i = 0; i < entryCount; ++i)
    {
        SnapshotEntry entry;
        std::memcpy(&entry.networkId, ptr, 4); ptr += 4;
        std::memcpy(&entry.position.x, ptr, 4); ptr += 4;
        std::memcpy(&entry.position.y, ptr, 4); ptr += 4;
        std::memcpy(&entry.position.z, ptr, 4); ptr += 4;
        std::memcpy(&entry.rotation.x, ptr, 4); ptr += 4;
        std::memcpy(&entry.rotation.y, ptr, 4); ptr += 4;
        std::memcpy(&entry.rotation.z, ptr, 4); ptr += 4;
        std::memcpy(&entry.rotation.w, ptr, 4); ptr += 4;
        std::memcpy(&entry.scale.x, ptr, 4); ptr += 4;
        std::memcpy(&entry.scale.y, ptr, 4); ptr += 4;
        std::memcpy(&entry.scale.z, ptr, 4); ptr += 4;
        result.push_back(entry);
    }

    return result;
}

// ---------------------------------------------------------------------------
// デルタスナップショット シリアライズ / 適用
// ---------------------------------------------------------------------------

gx::Vector<uint8_t> NetworkReplicator::SerializeDeltaSnapshot(
    const gx::Vector<SnapshotEntry>& current,
    const gx::Vector<SnapshotEntry>& previous) const
{
    // フォーマット: [deltaCount(4)] + [delta entries]
    // delta entry: [networkId(4)][flags(1)][changed fields...]

    // まず変更のあったエントリを収集
    struct DeltaEntry
    {
        NetworkEntityId networkId;
        uint8_t flags;
        Vector3 position;
        Quaternion rotation;
        Vector3 scale;
    };
    gx::Vector<DeltaEntry> deltas;

    for (const auto& cur : current)
    {
        // 前回のスナップショットで対応するエントリを検索
        const SnapshotEntry* prev = nullptr;
        for (const auto& p : previous)
        {
            if (p.networkId == cur.networkId)
            {
                prev = &p;
                break;
            }
        }

        if (!prev)
        {
            // 新規エンティティ — 全フィールドを送信
            DeltaEntry d;
            d.networkId = cur.networkId;
            d.flags = k_DeltaFlagPosition | k_DeltaFlagRotation | k_DeltaFlagScale;
            d.position = cur.position;
            d.rotation = cur.rotation;
            d.scale = cur.scale;
            deltas.push_back(d);
            continue;
        }

        // 変更を検出
        uint8_t flags = 0;
        if (cur.position != prev->position) flags |= k_DeltaFlagPosition;
        if (cur.rotation.x != prev->rotation.x || cur.rotation.y != prev->rotation.y ||
            cur.rotation.z != prev->rotation.z || cur.rotation.w != prev->rotation.w)
        {
            flags |= k_DeltaFlagRotation;
        }
        if (cur.scale != prev->scale) flags |= k_DeltaFlagScale;

        if (flags != 0)
        {
            DeltaEntry d;
            d.networkId = cur.networkId;
            d.flags = flags;
            d.position = cur.position;
            d.rotation = cur.rotation;
            d.scale = cur.scale;
            deltas.push_back(d);
        }
    }

    // バイナリサイズを計算
    uint32_t totalSize = 4; // deltaCount
    for (const auto& d : deltas)
    {
        totalSize += 4 + 1; // networkId + flags
        if (d.flags & k_DeltaFlagPosition) totalSize += 12;
        if (d.flags & k_DeltaFlagRotation) totalSize += 16;
        if (d.flags & k_DeltaFlagScale)    totalSize += 12;
    }

    gx::Vector<uint8_t> data(totalSize);
    uint8_t* ptr = data.data();

    uint32_t deltaCount = static_cast<uint32_t>(deltas.size());
    std::memcpy(ptr, &deltaCount, 4); ptr += 4;

    for (const auto& d : deltas)
    {
        std::memcpy(ptr, &d.networkId, 4); ptr += 4;
        *ptr = d.flags; ptr += 1;

        if (d.flags & k_DeltaFlagPosition)
        {
            std::memcpy(ptr, &d.position.x, 4); ptr += 4;
            std::memcpy(ptr, &d.position.y, 4); ptr += 4;
            std::memcpy(ptr, &d.position.z, 4); ptr += 4;
        }
        if (d.flags & k_DeltaFlagRotation)
        {
            std::memcpy(ptr, &d.rotation.x, 4); ptr += 4;
            std::memcpy(ptr, &d.rotation.y, 4); ptr += 4;
            std::memcpy(ptr, &d.rotation.z, 4); ptr += 4;
            std::memcpy(ptr, &d.rotation.w, 4); ptr += 4;
        }
        if (d.flags & k_DeltaFlagScale)
        {
            std::memcpy(ptr, &d.scale.x, 4); ptr += 4;
            std::memcpy(ptr, &d.scale.y, 4); ptr += 4;
            std::memcpy(ptr, &d.scale.z, 4); ptr += 4;
        }
    }

    return data;
}

gx::Vector<SnapshotEntry> NetworkReplicator::ApplyDeltaSnapshot(
    const gx::Vector<SnapshotEntry>& base,
    const gx::Vector<uint8_t>& delta) const
{
    // ベースをコピー
    gx::Vector<SnapshotEntry> result = base;

    if (delta.size() < 4) return result;

    const uint8_t* ptr = delta.data();
    const uint8_t* end = delta.data() + delta.size();

    uint32_t deltaCount = 0;
    std::memcpy(&deltaCount, ptr, 4); ptr += 4;

    for (uint32_t i = 0; i < deltaCount; ++i)
    {
        if (ptr + 5 > end) break; // networkId(4) + flags(1)

        NetworkEntityId networkId;
        std::memcpy(&networkId, ptr, 4); ptr += 4;
        uint8_t flags = *ptr; ptr += 1;

        // ベースから対応エントリを検索
        SnapshotEntry* entry = nullptr;
        for (auto& e : result)
        {
            if (e.networkId == networkId)
            {
                entry = &e;
                break;
            }
        }

        // 見つからない場合は新規追加
        if (!entry)
        {
            SnapshotEntry newEntry;
            newEntry.networkId = networkId;
            result.push_back(newEntry);
            entry = &result.back();
        }

        // デルタを適用
        if (flags & k_DeltaFlagPosition)
        {
            if (ptr + 12 > end) break;
            std::memcpy(&entry->position.x, ptr, 4); ptr += 4;
            std::memcpy(&entry->position.y, ptr, 4); ptr += 4;
            std::memcpy(&entry->position.z, ptr, 4); ptr += 4;
        }
        if (flags & k_DeltaFlagRotation)
        {
            if (ptr + 16 > end) break;
            std::memcpy(&entry->rotation.x, ptr, 4); ptr += 4;
            std::memcpy(&entry->rotation.y, ptr, 4); ptr += 4;
            std::memcpy(&entry->rotation.z, ptr, 4); ptr += 4;
            std::memcpy(&entry->rotation.w, ptr, 4); ptr += 4;
        }
        if (flags & k_DeltaFlagScale)
        {
            if (ptr + 12 > end) break;
            std::memcpy(&entry->scale.x, ptr, 4); ptr += 4;
            std::memcpy(&entry->scale.y, ptr, 4); ptr += 4;
            std::memcpy(&entry->scale.z, ptr, 4); ptr += 4;
        }
    }

    return result;
}

} // namespace gx
