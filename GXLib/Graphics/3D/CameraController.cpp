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
