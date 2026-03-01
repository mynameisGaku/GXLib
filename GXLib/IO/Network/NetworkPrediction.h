#pragma once
/// @file NetworkPrediction.h
/// @brief クライアントサイド予測・サーバー権威リコンサイル・リモートエンティティ補間
///
/// クライアント側で入力を即座に適用してレスポンシブな操作感を実現しつつ、
/// サーバーからの権威的状態で矛盾を検出した場合にスムーズに補正する。
/// リモートエンティティに対しては時間的補間を行う。
/// @addtogroup grp_network/// @{

#include "pch_common.h"
#include "Math/Vector3.h"
#include "Math/Quaternion.h"

namespace gx
{

class NetworkReplicator;
class Entity;

/// @brief 入力コマンド
struct InputCommand
{
    uint32_t sequence = 0;          ///< 入力シーケンス番号
    float deltaTime = 0.0f;         ///< フレームデルタ時間(秒)
    Vector3 moveDirection;          ///< 移動方向ベクトル
    float moveSpeed = 0.0f;         ///< 移動速度
    bool jump = false;              ///< ジャンプ入力
    float timestamp = 0.0f;         ///< タイムスタンプ(秒)
};

/// @brief ネットワーク予測（クライアントサイド予測 + サーバーリコンサイル）
///
/// クライアント側で入力を即座にローカルエンティティに適用（予測）し、
/// サーバーからの確認済み状態と比較して誤差が閾値を超えた場合にスムーズに補正する。
/// 未確認の入力は再適用（リプレイ）される。
class NetworkPrediction
{
public:
    NetworkPrediction();
    ~NetworkPrediction();

    /// @brief 予測システムを初期化する
    /// @param replicator ネットワークレプリケーター
    void Initialize(NetworkReplicator* replicator);

    /// @brief ローカルエンティティに入力を予測適用する
    /// @param localEntity ローカルプレイヤーエンティティ
    /// @param cmd 入力コマンド
    void PredictInput(Entity* localEntity, const InputCommand& cmd);

    /// @brief 保留中の入力をシリアライズする（サーバーへの送信用）
    void SendInputs();

    /// @brief サーバー状態とのリコンサイルを実行する
    /// @param entity ローカルエンティティ
    /// @param serverPos サーバーからの確認済み位置
    /// @param serverRot サーバーからの確認済み回転
    /// @param lastConfirmedSeq サーバーが確認した最後の入力シーケンス番号
    /// @return リプレイした入力の数
    uint32_t Reconcile(Entity* entity, const Vector3& serverPos,
                       const Quaternion& serverRot, uint32_t lastConfirmedSeq);

    /// @brief リモートエンティティを補間する
    /// @param entity 対象エンティティ
    /// @param prevPos 前回位置
    /// @param prevRot 前回回転
    /// @param nextPos 次回位置
    /// @param nextRot 次回回転
    /// @param t 補間係数 (0.0〜1.0)
    void InterpolateRemoteEntity(Entity* entity,
                                 const Vector3& prevPos, const Quaternion& prevRot,
                                 const Vector3& nextPos, const Quaternion& nextRot,
                                 float t);

    /// @brief 予測システムを更新する（古い履歴のクリーンアップ）
    /// @param dt フレームデルタ時間(秒)
    void Update(float dt);

    /// @brief 保留中の入力数を取得する
    uint32_t GetPendingInputCount() const { return static_cast<uint32_t>(m_pendingInputs.size()); }

    /// @brief 予測誤差を取得する
    float GetPredictionError() const { return m_predictionError; }

    /// @brief 補正閾値を設定する（この距離以上の誤差で補正を適用）
    void SetCorrectionThreshold(float threshold) { m_correctionThreshold = threshold; }

    /// @brief 補正スムージング係数を設定する (0.0=即座, 1.0=なし)
    void SetCorrectionSmoothing(float factor) { m_correctionSmoothing = factor; }

private:
    /// @brief 予測済み状態
    struct PredictedState
    {
        uint32_t sequence;      ///< 入力シーケンス番号
        Vector3 position;       ///< 予測位置
        Quaternion rotation;    ///< 予測回転
        InputCommand input;     ///< 対応する入力コマンド
    };

    NetworkReplicator* m_replicator = nullptr;
    std::vector<InputCommand> m_pendingInputs;          ///< 未確認入力のリスト
    std::vector<PredictedState> m_predictionHistory;    ///< 予測履歴
    float m_predictionError = 0.0f;                     ///< 最新の予測誤差
    float m_correctionThreshold = 0.5f;                 ///< 補正閾値(距離)
    float m_correctionSmoothing = 0.1f;                 ///< 補正スムージング係数
    uint32_t m_inputSequence = 0;                       ///< 入力シーケンスカウンタ
};

} // namespace gx
/// @}
