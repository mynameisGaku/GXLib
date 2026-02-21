/// @file AudioEmitter.cpp
/// @brief 3Dオーディオエミッターの実装
#include "pch.h"
#include "Audio/AudioEmitter.h"
#include <x3daudio.h>

namespace GX
{

AudioEmitter::AudioEmitter()
{
    // 距離減衰カーブ: 線形減衰（距離0=1.0、距離1.0=0.0）
    // X3DAudioCalculateはCurveDistanceScalerを掛けて正規化するため、
    // ポイントのx座標は0.0〜1.0の正規化距離で指定する。
    m_curvePoints[0] = { 0.0f, 1.0f };  // 距離0: フル音量
    m_curvePoints[1] = { 1.0f, 0.0f };  // 最大距離: 無音
    m_distanceCurve.pPoints    = m_curvePoints;
    m_distanceCurve.PointCount = 2;

    m_emitter = {};
    m_emitter.ChannelCount = 1;
    m_emitter.CurveDistanceScaler = m_maxDistance;
    m_emitter.pVolumeCurve = &m_distanceCurve;
    m_emitter.InnerRadius = m_innerRadius;

    UpdateNative();
}

void AudioEmitter::SetPosition(const XMFLOAT3& pos)
{
    m_position = pos;
    m_emitter.Position = { pos.x, pos.y, pos.z };
}

void AudioEmitter::SetVelocity(const XMFLOAT3& vel)
{
    m_velocity = vel;
    m_emitter.Velocity = { vel.x, vel.y, vel.z };
}

void AudioEmitter::SetDirection(const XMFLOAT3& front)
{
    m_direction = front;
    m_emitter.OrientFront = { front.x, front.y, front.z };
}

void AudioEmitter::SetInnerRadius(float radius)
{
    m_innerRadius = radius;
    m_emitter.InnerRadius = radius;
}

void AudioEmitter::SetMaxDistance(float distance)
{
    m_maxDistance = distance;
    m_emitter.CurveDistanceScaler = distance;
}

void AudioEmitter::SetCone(float innerAngle, float outerAngle, float outerVolume)
{
    m_cone.InnerAngle  = innerAngle;
    m_cone.OuterAngle  = outerAngle;
    m_cone.InnerVolume = 1.0f;
    m_cone.OuterVolume = outerVolume;
    m_cone.InnerLPF    = 0.0f;
    m_cone.OuterLPF    = 0.0f;
    m_cone.InnerReverb = 0.0f;
    m_cone.OuterReverb = 0.0f;

    m_useCone = true;
    m_emitter.pCone = &m_cone;
}

void AudioEmitter::DisableCone()
{
    m_useCone = false;
    m_emitter.pCone = nullptr;
}

void AudioEmitter::SetChannelCount(uint32_t channels)
{
    m_channels = channels;
    m_emitter.ChannelCount = channels;
}

void AudioEmitter::UpdateNative()
{
    m_emitter.Position    = { m_position.x, m_position.y, m_position.z };
    m_emitter.Velocity    = { m_velocity.x, m_velocity.y, m_velocity.z };
    m_emitter.OrientFront = { m_direction.x, m_direction.y, m_direction.z };
    m_emitter.OrientTop   = { 0.0f, 1.0f, 0.0f };
    m_emitter.ChannelCount = m_channels;
    m_emitter.CurveDistanceScaler = m_maxDistance;
    m_emitter.InnerRadius = m_innerRadius;
    m_emitter.pVolumeCurve = &m_distanceCurve;

    if (m_useCone)
        m_emitter.pCone = &m_cone;
    else
        m_emitter.pCone = nullptr;
}

} // namespace GX
