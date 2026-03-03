#pragma once
/// @file CameraController.h
/// @brief カメラ制御 — シェイク・レール移動・オービット・シネマティック演出
///
/// Camera3D に対して爆発時の揺れ（シェイク）、決まったパスに沿った移動（レール）、
/// ターゲット周回（オービット）、イベントシーン用の補間移動（シネマティック）を適用する。
/// ゲームプレイ中のカメラ演出をコードで組み立てるときに使う。
/// @addtogroup grp_gfx_3d/// @{
#include "pch_graphics.h"
#include "Math/Vector3.h"
namespace gx {

class Camera3D;
class Camera2D;

/// @brief カメラシェイクパラメータ
struct CameraShakeParams
{
    float amplitude = 0.5f;     ///< 振幅（ワールド単位）
    float frequency = 15.0f;    ///< 周波数（Hz）
    float duration = 0.5f;      ///< 持続時間（秒）
    float dampingRate = 3.0f;   ///< 減衰率（高い=早く減衰）
    bool rotational = false;    ///< 回転シェイクも含むか
    float rotAmplitude = 0.02f; ///< 回転振幅（ラジアン）
};

/// @brief レールポイント（カメラレール上のウェイポイント）
struct CameraRailPoint
{
    Vector3 position = { 0, 0, 0 };      ///< ウェイポイントの位置
    Vector3 lookTarget = { 0, 0, 0 };    ///< 注視点
    float fovY = 0.0f;  ///< 視野角（0=変更なし）
    float speed = 1.0f;  ///< このポイント付近の移動速度倍率
};

/// @brief オービットカメラ設定
struct OrbitConfig
{
    float distance = 5.0f;       ///< 注視点からの距離
    float minDistance = 1.0f;    ///< 最小距離
    float maxDistance = 50.0f;   ///< 最大距離
    float pitchMin = -1.4f;      ///< ラジアン（約-80度）
    float pitchMax = 1.4f;       ///< ラジアン（約80度）
    float rotateSpeed = 0.005f;  ///< マウス回転感度
    float zoomSpeed = 0.5f;      ///< ズーム速度
    float damping = 5.0f;        ///< 補間の減衰率
};

/// @brief スプラインカメラレール設定
struct SplineRailParams
{
    gx::Vector<Vector3> controlPoints;   ///< 制御点
    gx::Vector<Vector3> lookAtPoints;    ///< 各区間の注視点（空の場合はパス方向を向く）
    float speed       = 1.0f;             ///< 移動速度（ワールド単位/秒）
    float tension     = 0.5f;             ///< スプラインテンション (0=直線的, 1=強いカーブ)
    bool  loop        = false;            ///< ループするか
    bool  easeInOut   = true;             ///< イーズイン・アウト
    float easeDuration = 0.5f;            ///< イーズ区間長（秒）
};

/// @brief カメラコントローラー
class CameraController
{
public:
    CameraController() = default;
    ~CameraController() = default;

    // === カメラシェイク ===
    /// @brief シェイクを開始する
    void StartShake(const CameraShakeParams& params);
    void StartShake(float amplitude, float duration);
    /// @brief シェイクを停止する
    void StopShake();
    /// @brief シェイク中か判定する
    bool IsShaking() const { return m_shakeTimer > 0.0f; }
    /// @brief シェイクオフセットを取得する（Updateで計算される）
    /// @return ワールド空間のシェイクオフセット
    Vector3 GetShakeOffset() const { return m_shakeOffset; }
    /// @brief シェイクによる回転量を取得する
    /// @return 回転量（ラジアン）
    float GetShakeRotation() const { return m_shakeRotation; }

    // === カメラレール ===
    /// @brief レールポイントを追加
    void AddRailPoint(const CameraRailPoint& point);
    /// @brief レールをクリア
    void ClearRail();
    /// @brief レール上の移動を開始
    void StartRail(bool loop = false);
    void StopRail();
    /// @brief レール上の進行度を設定（0-1）
    void SetRailProgress(float t);
    float GetRailProgress() const { return m_railProgress; }
    bool IsOnRail() const { return m_railActive; }
    uint32_t GetRailPointCount() const { return static_cast<uint32_t>(m_railPoints.size()); }
    /// @brief レール上の現在の補間位置を取得
    Vector3 GetRailPosition() const;
    Vector3 GetRailLookTarget() const;

    // === スプラインカメラレール ===
    /// @brief スプラインレールを開始する
    /// @param params スプラインレール設定
    void StartSplineRail(const SplineRailParams& params);
    /// @brief スプラインレールを停止する
    void StopSplineRail();
    /// @brief スプラインレールがアクティブか判定する
    bool IsSplineRailActive() const { return m_splineActive; }
    /// @brief スプラインレールの進行度を取得する (0.0~1.0)
    float GetSplineProgress() const;
    /// @brief スプラインレールの移動速度を設定する
    /// @param speed 移動速度（ワールド単位/秒）
    void SetSplineSpeed(float speed);
    /// @brief スプラインレール上の現在位置を取得する
    Vector3 GetSplinePosition() const;
    /// @brief スプラインレール上の現在の注視点を取得する
    Vector3 GetSplineLookTarget() const;

    // === オービットカメラ ===
    /// @brief オービットモードを設定
    void SetOrbitConfig(const OrbitConfig& config) { m_orbitConfig = config; m_orbitDistance = config.distance; }
    const OrbitConfig& GetOrbitConfig() const { return m_orbitConfig; }
    /// @brief オービットの注視点を設定
    void SetOrbitTarget(const Vector3& target) { m_orbitTarget = target; }
    Vector3 GetOrbitTarget() const { return m_orbitTarget; }
    /// @brief マウス入力でオービット回転
    void OrbitRotate(float deltaYaw, float deltaPitch);
    /// @brief ズーム入力
    void OrbitZoom(float delta);
    /// @brief 現在のオービット距離
    float GetOrbitDistance() const { return m_orbitDistance; }
    float GetOrbitYaw() const { return m_orbitYaw; }
    float GetOrbitPitch() const { return m_orbitPitch; }
    /// @brief オービットからCamera3Dの位置・注視点を計算して適用
    void ApplyOrbitToCamera(Camera3D& camera) const;

    // === 更新 ===
    /// @brief 毎フレーム更新
    void Update(float deltaTime);

    /// @brief 2Dカメラにシェイクオフセットを適用する
    /// @param camera 対象の2Dカメラ
    void ApplyShakeToCamera2D(Camera2D& camera) const;

private:
    // シェイク
    CameraShakeParams m_shakeParams;              ///< 現在のシェイクパラメータ
    float m_shakeTimer = 0.0f;                    ///< シェイク残り時間（秒）
    float m_shakeElapsed = 0.0f;                  ///< シェイク経過時間（秒）
    Vector3 m_shakeOffset = { 0, 0, 0 };        ///< 現在のシェイクオフセット
    float m_shakeRotation = 0.0f;                 ///< 現在のシェイク回転量（ラジアン）
    uint32_t m_shakeSeed = 12345;                 ///< ノイズ生成用シード値

    // レール
    gx::Vector<CameraRailPoint> m_railPoints;    ///< レールウェイポイント配列
    float m_railProgress = 0.0f;                  ///< レール進行度（0〜1）
    bool m_railActive = false;                    ///< レール移動中フラグ
    bool m_railLoop = false;                      ///< ループ再生フラグ
    float m_railSpeed = 1.0f;                     ///< レール移動速度

    // スプラインレール
    SplineRailParams m_splineParams;              ///< スプラインレール設定
    float m_splineT = 0.0f;                       ///< 現在の走行距離（ワールド単位）
    float m_splineTotalLength = 0.0f;              ///< スプラインの総アーク長
    bool m_splineActive = false;                   ///< スプラインレール移動中フラグ
    float m_splineElapsed = 0.0f;                  ///< スプラインレール経過時間（秒）
    gx::Vector<float> m_segmentLengths;            ///< 各セグメントのアーク長

    void UpdateSplineRail(float dt);
    Vector3 EvaluateSpline(float t) const;
    Vector3 EvaluateSplineTangent(float t) const;
    float ComputeSegmentLength(int segIdx) const;
    void ComputeArcLengths();
    float EaseInOut(float t) const;
    Vector3 TensionCatmullRom(const Vector3& p0, const Vector3& p1,
                               const Vector3& p2, const Vector3& p3,
                               float t, float tension) const;

    // オービット
    OrbitConfig m_orbitConfig;                     ///< オービット設定
    Vector3 m_orbitTarget = { 0, 0, 0 };        ///< オービット注視点
    float m_orbitYaw = 0.0f;                      ///< オービットヨー角（ラジアン）
    float m_orbitPitch = 0.3f;                    ///< オービットピッチ角（ラジアン）
    float m_orbitDistance = 5.0f;                  ///< オービット距離

    // ヘルパー
    float PerlinNoise1D(float x) const;
    Vector3 CatmullRom(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t) const;
};

} // namespace gx
/// @}
