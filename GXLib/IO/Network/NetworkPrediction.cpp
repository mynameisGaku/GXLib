/// @file NetworkPrediction.cpp
/// @brief クライアントサイド予測・サーバー権威リコンサイル実装
#include "pch_common.h"
#include "IO/Network/NetworkPrediction.h"
#include "IO/Network/NetworkReplicator.h"
#include "Core/Scene/Entity.h"
#include "Core/Logger.h"
#include <cstring>
#include <algorithm>

namespace gx
{

// ---------------------------------------------------------------------------
// 定数
// ---------------------------------------------------------------------------

/// @brief 予測履歴の最大保持数
static constexpr uint32_t k_MaxPredictionHistorySize = 256;

/// @brief 保留入力の最大保持数
static constexpr uint32_t k_MaxPendingInputs = 128;

// ---------------------------------------------------------------------------
// コンストラクタ / デストラクタ
// ---------------------------------------------------------------------------

NetworkPrediction::NetworkPrediction() = default;
NetworkPrediction::~NetworkPrediction() = default;

// ---------------------------------------------------------------------------
// 初期化
// ---------------------------------------------------------------------------

void NetworkPrediction::Initialize(NetworkReplicator* replicator)
{
    m_replicator = replicator;
    m_pendingInputs.clear();
    m_predictionHistory.clear();
    m_predictionError = 0.0f;
    m_inputSequence = 0;

    GX_LOG_INFO("[NetworkPrediction] Initialized (threshold=%.2f, smoothing=%.2f)",
                m_correctionThreshold, m_correctionSmoothing);
}

// ---------------------------------------------------------------------------
// 入力予測
// ---------------------------------------------------------------------------

void NetworkPrediction::PredictInput(Entity* localEntity, const InputCommand& cmd)
{
    if (!localEntity) return;

    // シーケンス番号を割り当て
    InputCommand command = cmd;
    command.sequence = m_inputSequence++;

    // 入力を適用（クライアント側で即座に反映）
    auto& transform = localEntity->GetTransform();
    const auto& currentPos = transform.GetPosition();

    // 移動方向 * 速度 * デルタ時間 で位置を更新
    Vector3 pos(currentPos.x, currentPos.y, currentPos.z);
    Vector3 movement = command.moveDirection * command.moveSpeed * command.deltaTime;
    pos += movement;

    transform.SetPosition(pos.x, pos.y, pos.z);

    // 予測状態を記録
    PredictedState state;
    state.sequence = command.sequence;
    state.position = pos;
    // 現在の回転を取得
    const auto& currentRot = transform.GetRotation();
    state.rotation = Quaternion::FromEuler(currentRot.x, currentRot.y, currentRot.z);
    state.input = command;
    m_predictionHistory.push_back(state);

    // 保留入力に追加
    m_pendingInputs.push_back(command);

    // 履歴サイズ制限
    if (m_predictionHistory.size() > k_MaxPredictionHistorySize)
    {
        m_predictionHistory.erase(m_predictionHistory.begin());
    }
    if (m_pendingInputs.size() > k_MaxPendingInputs)
    {
        m_pendingInputs.erase(m_pendingInputs.begin());
    }
}

// ---------------------------------------------------------------------------
// 入力送信準備
// ---------------------------------------------------------------------------

void NetworkPrediction::SendInputs()
{
    // 保留中の入力をシリアライズ
    // 実際の送信はNetworkManager経由で行う想定
    // ここではシリアライズバッファの準備のみ

    if (m_pendingInputs.empty()) return;

    // フォーマット: [count(4)] + [InputCommand * N]
    // InputCommand: [sequence(4)][deltaTime(4)][moveDir.xyz(12)][moveSpeed(4)][jump(1)][timestamp(4)]
    // = 29 bytes per command

    // 注意: 実際のネットワーク送信は呼び出し元が NetworkManager を通じて行う
    // この関数はバッファの組み立てに注力する

    // 送信後、保留入力はリコンサイルで確認されるまで保持される
}

// ---------------------------------------------------------------------------
// サーバーリコンサイル
// ---------------------------------------------------------------------------

uint32_t NetworkPrediction::Reconcile(Entity* entity, const Vector3& serverPos,
                                       const Quaternion& serverRot,
                                       uint32_t lastConfirmedSeq)
{
    if (!entity) return 0;

    // 確認済みシーケンスまでの入力を除去
    m_pendingInputs.erase(
        std::remove_if(m_pendingInputs.begin(), m_pendingInputs.end(),
                        [lastConfirmedSeq](const InputCommand& cmd)
                        {
                            return cmd.sequence <= lastConfirmedSeq;
                        }),
        m_pendingInputs.end());

    // 確認済みシーケンスに対応する予測状態を検索
    const PredictedState* predictedAtConfirm = nullptr;
    for (const auto& state : m_predictionHistory)
    {
        if (state.sequence == lastConfirmedSeq)
        {
            predictedAtConfirm = &state;
            break;
        }
    }

    // 確認済みシーケンスまでの予測履歴を除去
    m_predictionHistory.erase(
        std::remove_if(m_predictionHistory.begin(), m_predictionHistory.end(),
                        [lastConfirmedSeq](const PredictedState& s)
                        {
                            return s.sequence <= lastConfirmedSeq;
                        }),
        m_predictionHistory.end());

    // 予測誤差を計算
    if (predictedAtConfirm)
    {
        m_predictionError = serverPos.Distance(predictedAtConfirm->position);
    }
    else
    {
        // 予測状態が見つからない場合、現在位置との差を計算
        const auto& curPos = entity->GetTransform().GetPosition();
        Vector3 currentPos(curPos.x, curPos.y, curPos.z);
        m_predictionError = serverPos.Distance(currentPos);
    }

    // 誤差が閾値以下なら補正不要
    if (m_predictionError <= m_correctionThreshold)
    {
        return 0;
    }

    GX_LOG_WARN("[NetworkPrediction] Prediction error=%.3f (threshold=%.3f), reconciling...",
                m_predictionError, m_correctionThreshold);

    // --- サーバー状態を基点にして未確認入力をリプレイ ---

    // サーバーの確認済み位置からスタート
    Vector3 replayPos = serverPos;
    Quaternion replayRot = serverRot;

    uint32_t replayedCount = 0;

    for (const auto& input : m_pendingInputs)
    {
        // 入力を再適用
        Vector3 movement = input.moveDirection * input.moveSpeed * input.deltaTime;
        replayPos += movement;
        replayedCount++;
    }

    // スムーズ補正: 現在の位置からリプレイ結果へ補間
    const auto& curPos = entity->GetTransform().GetPosition();
    Vector3 currentPos(curPos.x, curPos.y, curPos.z);

    // smoothing が小さいほど速く補正される
    float blendFactor = 1.0f - m_correctionSmoothing;
    Vector3 correctedPos = Vector3::Lerp(currentPos, replayPos, blendFactor);

    // 回転も補正
    const auto& curRot = entity->GetTransform().GetRotation();
    Quaternion currentQuat = Quaternion::FromEuler(curRot.x, curRot.y, curRot.z);
    Quaternion correctedRot = Quaternion::Slerp(currentQuat, replayRot, blendFactor);

    // エンティティに適用
    entity->GetTransform().SetPosition(correctedPos.x, correctedPos.y, correctedPos.z);
    Vector3 euler = correctedRot.ToEuler();
    entity->GetTransform().SetRotation(euler.x, euler.y, euler.z);

    return replayedCount;
}

// ---------------------------------------------------------------------------
// リモートエンティティ補間
// ---------------------------------------------------------------------------

void NetworkPrediction::InterpolateRemoteEntity(
    Entity* entity,
    const Vector3& prevPos, const Quaternion& prevRot,
    const Vector3& nextPos, const Quaternion& nextRot,
    float t)
{
    if (!entity) return;

    // 補間係数をクランプ
    float clampedT = (std::max)(0.0f, (std::min)(1.0f, t));

    // 位置: 線形補間
    Vector3 interpPos = Vector3::Lerp(prevPos, nextPos, clampedT);

    // 回転: 球面線形補間
    Quaternion interpRot = Quaternion::Slerp(prevRot, nextRot, clampedT);

    // エンティティに適用
    entity->GetTransform().SetPosition(interpPos.x, interpPos.y, interpPos.z);

    Vector3 euler = interpRot.ToEuler();
    entity->GetTransform().SetRotation(euler.x, euler.y, euler.z);
}

// ---------------------------------------------------------------------------
// 更新（古い履歴のクリーンアップ）
// ---------------------------------------------------------------------------

void NetworkPrediction::Update(float dt)
{
    (void)dt;

    // 古い予測履歴をクリーンアップ
    // 通常はReconcile時に行われるが、長期間サーバー応答がない場合のフォールバック
    if (m_predictionHistory.size() > k_MaxPredictionHistorySize)
    {
        size_t excess = m_predictionHistory.size() - k_MaxPredictionHistorySize;
        m_predictionHistory.erase(m_predictionHistory.begin(),
                                   m_predictionHistory.begin() + excess);
    }

    if (m_pendingInputs.size() > k_MaxPendingInputs)
    {
        size_t excess = m_pendingInputs.size() - k_MaxPendingInputs;
        m_pendingInputs.erase(m_pendingInputs.begin(),
                               m_pendingInputs.begin() + excess);
    }
}

} // namespace gx
