/// @file CameraController.cpp
/// @brief カメラ制御システムの実装
#include "pch_graphics.h"
#include "Graphics/3D/CameraController.h"
#include "Graphics/3D/Camera3D.h"
#include "Graphics/Rendering/Camera2D.h"

namespace gx
{

// ============================================================================
// カメラシェイク
// ============================================================================

void CameraController::StartShake(const CameraShakeParams& params)
{
    m_shakeParams = params;
    m_shakeTimer = params.duration;
    m_shakeElapsed = 0.0f;
    m_shakeOffset = { 0, 0, 0 };
    m_shakeRotation = 0.0f;
}

void CameraController::StartShake(float amplitude, float duration)
{
    CameraShakeParams params;
    params.amplitude = amplitude;
    params.duration = duration;
    StartShake(params);
}

void CameraController::StopShake()
{
    m_shakeTimer = 0.0f;
    m_shakeElapsed = 0.0f;
    m_shakeOffset = { 0, 0, 0 };
    m_shakeRotation = 0.0f;
}

// ============================================================================
// カメラレール
// ============================================================================

void CameraController::AddRailPoint(const CameraRailPoint& point)
{
    m_railPoints.push_back(point);
}

void CameraController::ClearRail()
{
    m_railPoints.clear();
    m_railProgress = 0.0f;
    m_railActive = false;
}

void CameraController::StartRail(bool loop)
{
    if (m_railPoints.size() < 2)
        return;
    m_railActive = true;
    m_railLoop = loop;
    m_railProgress = 0.0f;
}

void CameraController::StopRail()
{
    m_railActive = false;
}

void CameraController::SetRailProgress(float t)
{
    m_railProgress = std::max(0.0f, std::min(1.0f, t));
}

Vector3 CameraController::GetRailPosition() const
{
    if (m_railPoints.size() < 2)
        return { 0, 0, 0 };

    uint32_t numSegments = static_cast<uint32_t>(m_railPoints.size()) - 1;
    float scaledT = m_railProgress * static_cast<float>(numSegments);
    uint32_t segment = static_cast<uint32_t>(scaledT);
    if (segment >= numSegments)
        segment = numSegments - 1;
    float localT = scaledT - static_cast<float>(segment);

    // Catmull-Romスプラインには4つの制御点が必要: p0, p1, p2, p3
    uint32_t count = static_cast<uint32_t>(m_railPoints.size());
    uint32_t i0 = (segment > 0) ? segment - 1 : 0;
    uint32_t i1 = segment;
    uint32_t i2 = (segment + 1 < count) ? segment + 1 : count - 1;
    uint32_t i3 = (segment + 2 < count) ? segment + 2 : count - 1;

    return CatmullRom(
        m_railPoints[i0].position,
        m_railPoints[i1].position,
        m_railPoints[i2].position,
        m_railPoints[i3].position,
        localT);
}

Vector3 CameraController::GetRailLookTarget() const
{
    if (m_railPoints.size() < 2)
        return { 0, 0, 0 };

    uint32_t numSegments = static_cast<uint32_t>(m_railPoints.size()) - 1;
    float scaledT = m_railProgress * static_cast<float>(numSegments);
    uint32_t segment = static_cast<uint32_t>(scaledT);
    if (segment >= numSegments)
        segment = numSegments - 1;
    float localT = scaledT - static_cast<float>(segment);

    uint32_t count = static_cast<uint32_t>(m_railPoints.size());
    uint32_t i0 = (segment > 0) ? segment - 1 : 0;
    uint32_t i1 = segment;
    uint32_t i2 = (segment + 1 < count) ? segment + 1 : count - 1;
    uint32_t i3 = (segment + 2 < count) ? segment + 2 : count - 1;

    return CatmullRom(
        m_railPoints[i0].lookTarget,
        m_railPoints[i1].lookTarget,
        m_railPoints[i2].lookTarget,
        m_railPoints[i3].lookTarget,
        localT);
}

// ============================================================================
// オービットカメラ
// ============================================================================

void CameraController::OrbitRotate(float deltaYaw, float deltaPitch)
{
    m_orbitYaw += deltaYaw;
    m_orbitPitch += deltaPitch;

    // ピッチを設定範囲にクランプ
    m_orbitPitch = std::max(m_orbitConfig.pitchMin, std::min(m_orbitConfig.pitchMax, m_orbitPitch));
}

void CameraController::OrbitZoom(float delta)
{
    m_orbitDistance -= delta * m_orbitConfig.zoomSpeed;
    m_orbitDistance = std::max(m_orbitConfig.minDistance, std::min(m_orbitConfig.maxDistance, m_orbitDistance));
}

void CameraController::ApplyOrbitToCamera(Camera3D& camera) const
{
    // 球面座標からデカルト座標に変換
    float cosPitch = std::cos(m_orbitPitch);
    float sinPitch = std::sin(m_orbitPitch);
    float cosYaw = std::cos(m_orbitYaw);
    float sinYaw = std::sin(m_orbitYaw);

    Vector3 cameraPos;
    cameraPos.x = m_orbitTarget.x + m_orbitDistance * cosPitch * sinYaw;
    cameraPos.y = m_orbitTarget.y + m_orbitDistance * sinPitch;
    cameraPos.z = m_orbitTarget.z + m_orbitDistance * cosPitch * cosYaw;

    camera.SetPosition(cameraPos);
    camera.LookAt(m_orbitTarget);
}

// ============================================================================
// 2Dカメラシェイク
// ============================================================================

void CameraController::ApplyShakeToCamera2D(Camera2D& camera) const
{
    if (m_shakeTimer <= 0.0f)
        return;

    camera.SetPosition(
        camera.GetPositionX() + m_shakeOffset.x,
        camera.GetPositionY() + m_shakeOffset.y);

    if (m_shakeParams.rotational)
    {
        camera.SetRotation(camera.GetRotation() + m_shakeRotation);
    }
}

// ============================================================================
// 更新
// ============================================================================

void CameraController::Update(float deltaTime)
{
    // --- シェイク更新 ---
    if (m_shakeTimer > 0.0f)
    {
        m_shakeElapsed += deltaTime;
        m_shakeTimer -= deltaTime;

        if (m_shakeTimer <= 0.0f)
        {
            // シェイク終了
            m_shakeTimer = 0.0f;
            m_shakeOffset = { 0, 0, 0 };
            m_shakeRotation = 0.0f;
        }
        else
        {
            // 減衰エンベロープを計算: 経過時間に応じて指数的に減衰
            float envelope = std::exp(-m_shakeParams.dampingRate * m_shakeElapsed);
            float amp = m_shakeParams.amplitude * envelope;
            float freq = m_shakeParams.frequency;
            float t = m_shakeElapsed;

            // 各軸に異なるシードでPerlinノイズ風の値を生成
            float noiseX = PerlinNoise1D(t * freq);
            float noiseY = PerlinNoise1D(t * freq + 100.0f);
            float noiseZ = PerlinNoise1D(t * freq + 200.0f);

            m_shakeOffset.x = amp * noiseX;
            m_shakeOffset.y = amp * noiseY;
            m_shakeOffset.z = amp * noiseZ;

            // 回転シェイク
            if (m_shakeParams.rotational)
            {
                float noiseR = PerlinNoise1D(t * freq + 300.0f);
                m_shakeRotation = m_shakeParams.rotAmplitude * envelope * noiseR;
            }
            else
            {
                m_shakeRotation = 0.0f;
            }
        }
    }

    // --- レール更新 ---
    if (m_railActive && m_railPoints.size() >= 2)
    {
        // 最寄りのレールポイントから速度倍率を取得
        uint32_t numSegments = static_cast<uint32_t>(m_railPoints.size()) - 1;
        float scaledT = m_railProgress * static_cast<float>(numSegments);
        uint32_t segment = static_cast<uint32_t>(scaledT);
        if (segment >= numSegments)
            segment = numSegments - 1;
        float localT = scaledT - static_cast<float>(segment);

        // 現在と次のセグメントポイント間で速度を補間
        float speedA = m_railPoints[segment].speed;
        float speedB = m_railPoints[(std::min)(segment + 1, static_cast<uint32_t>(m_railPoints.size()) - 1)].speed;
        float currentSpeed = speedA + (speedB - speedA) * localT;

        // 進行度を更新
        float progressDelta = (currentSpeed * deltaTime) / static_cast<float>(numSegments);
        m_railProgress += progressDelta;

        if (m_railProgress >= 1.0f)
        {
            if (m_railLoop)
            {
                m_railProgress -= 1.0f;
                if (m_railProgress < 0.0f)
                    m_railProgress = 0.0f;
            }
            else
            {
                m_railProgress = 1.0f;
                m_railActive = false;
            }
        }
    }

    // --- スプラインレール更新 ---
    if (m_splineActive)
    {
        UpdateSplineRail(deltaTime);
    }
}

// ============================================================================
// スプラインカメラレール
// ============================================================================

void CameraController::StartSplineRail(const SplineRailParams& params)
{
    if (params.controlPoints.size() < 2)
        return;

    m_splineParams = params;
    m_splineT = 0.0f;
    m_splineElapsed = 0.0f;
    m_splineActive = true;

    // アーク長をプリコンピュート
    ComputeArcLengths();
}

void CameraController::StopSplineRail()
{
    m_splineActive = false;
}

float CameraController::GetSplineProgress() const
{
    if (m_splineTotalLength <= 0.0f)
        return 0.0f;
    return std::min(m_splineT / m_splineTotalLength, 1.0f);
}

void CameraController::SetSplineSpeed(float speed)
{
    m_splineParams.speed = speed;
}

Vector3 CameraController::GetSplinePosition() const
{
    if (m_splineParams.controlPoints.size() < 2)
        return { 0, 0, 0 };
    return EvaluateSpline(GetSplineProgress());
}

Vector3 CameraController::GetSplineLookTarget() const
{
    if (m_splineParams.controlPoints.size() < 2)
        return { 0, 0, 1 };

    float progress = GetSplineProgress();

    // 注視点が設定されている場合はそちらを補間して使う
    if (!m_splineParams.lookAtPoints.empty() && m_splineParams.lookAtPoints.size() >= 2)
    {
        uint32_t numSegments = static_cast<uint32_t>(m_splineParams.lookAtPoints.size()) - 1;
        float scaledT = progress * static_cast<float>(numSegments);
        uint32_t segment = static_cast<uint32_t>(scaledT);
        if (segment >= numSegments)
            segment = numSegments - 1;
        float localT = scaledT - static_cast<float>(segment);

        uint32_t count = static_cast<uint32_t>(m_splineParams.lookAtPoints.size());

        uint32_t i0, i1, i2, i3;
        if (m_splineParams.loop)
        {
            i0 = (segment == 0) ? count - 1 : segment - 1;
            i1 = segment;
            i2 = (segment + 1) % count;
            i3 = (segment + 2) % count;
        }
        else
        {
            i0 = (segment > 0) ? segment - 1 : 0;
            i1 = segment;
            i2 = (segment + 1 < count) ? segment + 1 : count - 1;
            i3 = (segment + 2 < count) ? segment + 2 : count - 1;
        }

        return TensionCatmullRom(
            m_splineParams.lookAtPoints[i0],
            m_splineParams.lookAtPoints[i1],
            m_splineParams.lookAtPoints[i2],
            m_splineParams.lookAtPoints[i3],
            localT, m_splineParams.tension);
    }

    // 注視点が未設定の場合はパス方向（接線）を使用
    Vector3 pos = EvaluateSpline(progress);
    Vector3 tangent = EvaluateSplineTangent(progress);
    float len = std::sqrt(tangent.x * tangent.x + tangent.y * tangent.y + tangent.z * tangent.z);
    if (len < 1e-6f)
        return { pos.x, pos.y, pos.z + 1.0f };
    return { pos.x + tangent.x / len, pos.y + tangent.y / len, pos.z + tangent.z / len };
}

void CameraController::UpdateSplineRail(float dt)
{
    if (m_splineParams.controlPoints.size() < 2 || m_splineTotalLength <= 0.0f)
    {
        m_splineActive = false;
        return;
    }

    m_splineElapsed += dt;

    // イーズイン・アウト適用済み速度を計算
    float speed = m_splineParams.speed;
    if (m_splineParams.easeInOut && m_splineParams.easeDuration > 0.0f)
    {
        // 走行時間ベースでの総時間を概算
        float totalTime = m_splineTotalLength / (std::max)(speed, 0.001f);
        float normalizedTime = (totalTime > 0.0f) ? (m_splineElapsed / totalTime) : 1.0f;
        normalizedTime = std::min(normalizedTime, 1.0f);

        float easeFactor = EaseInOut(normalizedTime);
        // イーズ中は速度を0から最大まで滑らかに変化させる
        // ただし速度が0にならないようクランプ
        speed *= std::max(easeFactor, 0.01f);
    }

    // 距離を進める
    m_splineT += speed * dt;

    if (m_splineT >= m_splineTotalLength)
    {
        if (m_splineParams.loop)
        {
            while (m_splineT >= m_splineTotalLength)
                m_splineT -= m_splineTotalLength;
            m_splineElapsed = 0.0f;
        }
        else
        {
            m_splineT = m_splineTotalLength;
            m_splineActive = false;
        }
    }
}

Vector3 CameraController::EvaluateSpline(float t) const
{
    uint32_t cpCount = static_cast<uint32_t>(m_splineParams.controlPoints.size());
    if (cpCount < 2)
        return { 0, 0, 0 };

    // tは正規化パラメータ (0.0~1.0)
    uint32_t numSegments = m_splineParams.loop ? cpCount : cpCount - 1;
    float scaledT = t * static_cast<float>(numSegments);
    uint32_t segment = static_cast<uint32_t>(scaledT);
    if (segment >= numSegments)
        segment = numSegments - 1;
    float localT = scaledT - static_cast<float>(segment);

    uint32_t i0, i1, i2, i3;
    if (m_splineParams.loop)
    {
        i0 = (segment == 0) ? cpCount - 1 : segment - 1;
        i1 = segment % cpCount;
        i2 = (segment + 1) % cpCount;
        i3 = (segment + 2) % cpCount;
    }
    else
    {
        i0 = (segment > 0) ? segment - 1 : 0;
        i1 = segment;
        i2 = (segment + 1 < cpCount) ? segment + 1 : cpCount - 1;
        i3 = (segment + 2 < cpCount) ? segment + 2 : cpCount - 1;
    }

    return TensionCatmullRom(
        m_splineParams.controlPoints[i0],
        m_splineParams.controlPoints[i1],
        m_splineParams.controlPoints[i2],
        m_splineParams.controlPoints[i3],
        localT, m_splineParams.tension);
}

Vector3 CameraController::EvaluateSplineTangent(float t) const
{
    uint32_t cpCount = static_cast<uint32_t>(m_splineParams.controlPoints.size());
    if (cpCount < 2)
        return { 0, 0, 1 };

    // Hermite basis derivative:
    // h00'(t) = 6t^2 - 6t
    // h10'(t) = 3t^2 - 4t + 1
    // h01'(t) = -6t^2 + 6t
    // h11'(t) = 3t^2 - 2t
    //
    // q'(t) = h00'*p1 + h10'*m1 + h01'*p2 + h11'*m2

    uint32_t numSegments = m_splineParams.loop ? cpCount : cpCount - 1;
    float scaledT = t * static_cast<float>(numSegments);
    uint32_t segment = static_cast<uint32_t>(scaledT);
    if (segment >= numSegments)
        segment = numSegments - 1;
    float lt = scaledT - static_cast<float>(segment);

    uint32_t i0, i1, i2, i3;
    if (m_splineParams.loop)
    {
        i0 = (segment == 0) ? cpCount - 1 : segment - 1;
        i1 = segment % cpCount;
        i2 = (segment + 1) % cpCount;
        i3 = (segment + 2) % cpCount;
    }
    else
    {
        i0 = (segment > 0) ? segment - 1 : 0;
        i1 = segment;
        i2 = (segment + 1 < cpCount) ? segment + 1 : cpCount - 1;
        i3 = (segment + 2 < cpCount) ? segment + 2 : cpCount - 1;
    }

    const Vector3& p0 = m_splineParams.controlPoints[i0];
    const Vector3& p1 = m_splineParams.controlPoints[i1];
    const Vector3& p2 = m_splineParams.controlPoints[i2];
    const Vector3& p3 = m_splineParams.controlPoints[i3];

    float s = (1.0f - m_splineParams.tension) * 0.5f;
    float lt2 = lt * lt;

    float dh00 = 6.0f * lt2 - 6.0f * lt;
    float dh10 = 3.0f * lt2 - 4.0f * lt + 1.0f;
    float dh01 = -6.0f * lt2 + 6.0f * lt;
    float dh11 = 3.0f * lt2 - 2.0f * lt;

    // Tangents (same as in TensionCatmullRom)
    float m1x = s * (p2.x - p0.x), m1y = s * (p2.y - p0.y), m1z = s * (p2.z - p0.z);
    float m2x = s * (p3.x - p1.x), m2y = s * (p3.y - p1.y), m2z = s * (p3.z - p1.z);

    Vector3 result;
    result.x = dh00 * p1.x + dh10 * m1x + dh01 * p2.x + dh11 * m2x;
    result.y = dh00 * p1.y + dh10 * m1y + dh01 * p2.y + dh11 * m2y;
    result.z = dh00 * p1.z + dh10 * m1z + dh01 * p2.z + dh11 * m2z;

    return result;
}

float CameraController::ComputeSegmentLength(int segIdx) const
{
    uint32_t cpCount = static_cast<uint32_t>(m_splineParams.controlPoints.size());
    if (cpCount < 2)
        return 0.0f;

    uint32_t numSegments = m_splineParams.loop ? cpCount : cpCount - 1;
    if (segIdx < 0 || static_cast<uint32_t>(segIdx) >= numSegments)
        return 0.0f;

    // 8サンプルのシンプソン則による数値積分
    constexpr int kSamples = 8;
    float length = 0.0f;
    float invNumSeg = 1.0f / static_cast<float>(numSegments);

    Vector3 prevPos = EvaluateSpline(static_cast<float>(segIdx) * invNumSeg);
    for (int i = 1; i <= kSamples; ++i)
    {
        float subT = static_cast<float>(segIdx) + static_cast<float>(i) / static_cast<float>(kSamples);
        subT *= invNumSeg;
        Vector3 curPos = EvaluateSpline(subT);

        float dx = curPos.x - prevPos.x;
        float dy = curPos.y - prevPos.y;
        float dz = curPos.z - prevPos.z;
        length += std::sqrt(dx * dx + dy * dy + dz * dz);

        prevPos = curPos;
    }

    return length;
}

void CameraController::ComputeArcLengths()
{
    uint32_t cpCount = static_cast<uint32_t>(m_splineParams.controlPoints.size());
    if (cpCount < 2)
    {
        m_segmentLengths.clear();
        m_splineTotalLength = 0.0f;
        return;
    }

    uint32_t numSegments = m_splineParams.loop ? cpCount : cpCount - 1;
    m_segmentLengths.resize(numSegments);
    m_splineTotalLength = 0.0f;

    for (uint32_t i = 0; i < numSegments; ++i)
    {
        m_segmentLengths[i] = ComputeSegmentLength(static_cast<int>(i));
        m_splineTotalLength += m_segmentLengths[i];
    }
}

float CameraController::EaseInOut(float t) const
{
    // smoothstep-based ease in/out
    // t=0~easeFraction: ease in
    // t=1-easeFraction~1: ease out
    // between: full speed
    if (!m_splineParams.easeInOut || m_splineParams.easeDuration <= 0.0f)
        return 1.0f;

    float totalTime = m_splineTotalLength / (std::max)(m_splineParams.speed, 0.001f);
    float easeFraction = (totalTime > 0.0f) ? (m_splineParams.easeDuration / totalTime) : 0.0f;
    easeFraction = std::min(easeFraction, 0.5f);  // 最大50%ずつ

    if (t < easeFraction)
    {
        // イーズイン: smoothstep(0, easeFraction, t)
        float x = t / easeFraction;
        return x * x * (3.0f - 2.0f * x);
    }
    else if (t > 1.0f - easeFraction)
    {
        // イーズアウト: 1 - smoothstep(1-easeFraction, 1, t)
        float x = (t - (1.0f - easeFraction)) / easeFraction;
        return 1.0f - x * x * (3.0f - 2.0f * x);
    }

    return 1.0f;
}

Vector3 CameraController::TensionCatmullRom(const Vector3& p0, const Vector3& p1,
                                              const Vector3& p2, const Vector3& p3,
                                              float t, float tension) const
{
    // Cardinal spline with tension parameter.
    // s = (1-tension)/2 controls tangent magnitude.
    // tension=0: standard Catmull-Rom (s=0.5)
    // tension=1: linear interpolation (s=0, tangents=zero)
    //
    // Tangent at p1: m1 = s*(p2 - p0)
    // Tangent at p2: m2 = s*(p3 - p1)
    //
    // Hermite basis:
    // h00(t) = 2t^3 - 3t^2 + 1
    // h10(t) = t^3 - 2t^2 + t
    // h01(t) = -2t^3 + 3t^2
    // h11(t) = t^3 - t^2
    //
    // q(t) = h00*p1 + h10*m1 + h01*p2 + h11*m2
    // This guarantees q(0)=p1 and q(1)=p2 for any tension value.

    float s = (1.0f - tension) * 0.5f;
    float t2 = t * t;
    float t3 = t2 * t;

    float h00 = 2.0f * t3 - 3.0f * t2 + 1.0f;
    float h10 = t3 - 2.0f * t2 + t;
    float h01 = -2.0f * t3 + 3.0f * t2;
    float h11 = t3 - t2;

    // Tangents
    float m1x = s * (p2.x - p0.x), m1y = s * (p2.y - p0.y), m1z = s * (p2.z - p0.z);
    float m2x = s * (p3.x - p1.x), m2y = s * (p3.y - p1.y), m2z = s * (p3.z - p1.z);

    Vector3 result;
    result.x = h00 * p1.x + h10 * m1x + h01 * p2.x + h11 * m2x;
    result.y = h00 * p1.y + h10 * m1y + h01 * p2.y + h11 * m2y;
    result.z = h00 * p1.z + h10 * m1z + h01 * p2.z + h11 * m2z;

    return result;
}

// ============================================================================
// ヘルパー関数
// ============================================================================

float CameraController::PerlinNoise1D(float x) const
{
    // sin関数ベースのハッシュを使った簡易1Dバリューノイズ
    // [-1, 1] の範囲の値を生成
    float i = std::floor(x);
    float f = x - i;

    // スムーズステップ補間係数
    float u = f * f * (3.0f - 2.0f * f);

    // sin関数を使ったハッシュ（シードに基づく決定論的疑似乱数）
    auto hash = [this](float n) -> float
    {
        float seed = static_cast<float>(m_shakeSeed);
        return std::sin(n * 127.1f + seed * 311.7f) * 43758.5453f;
    };

    float a = hash(i);
    a = a - std::floor(a);          // 小数部
    a = a * 2.0f - 1.0f;            // [-1, 1] にリマップ

    float b = hash(i + 1.0f);
    b = b - std::floor(b);          // 小数部
    b = b * 2.0f - 1.0f;            // [-1, 1] にリマップ

    return a + u * (b - a);         // 線形補間
}

Vector3 CameraController::CatmullRom(const Vector3& p0, const Vector3& p1,
                                        const Vector3& p2, const Vector3& p3, float t) const
{
    // 標準Catmull-Romスプライン補間
    // q(t) = 0.5 * ((2*p1) + (-p0 + p2)*t + (2*p0 - 5*p1 + 4*p2 - p3)*t^2 + (-p0 + 3*p1 - 3*p2 + p3)*t^3)
    float t2 = t * t;
    float t3 = t2 * t;

    Vector3 result;
    result.x = 0.5f * ((2.0f * p1.x) +
                        (-p0.x + p2.x) * t +
                        (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2 +
                        (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3);

    result.y = 0.5f * ((2.0f * p1.y) +
                        (-p0.y + p2.y) * t +
                        (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * t2 +
                        (-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * t3);

    result.z = 0.5f * ((2.0f * p1.z) +
                        (-p0.z + p2.z) * t +
                        (2.0f * p0.z - 5.0f * p1.z + 4.0f * p2.z - p3.z) * t2 +
                        (-p0.z + 3.0f * p1.z - 3.0f * p2.z + p3.z) * t3);

    return result;
}

} // namespace gx
