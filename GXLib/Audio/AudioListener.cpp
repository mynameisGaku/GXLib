/// @file AudioListener.cpp
/// @brief 3Dオーディオリスナーの実装
#include "pch.h"
#include "Audio/AudioListener.h"
#include "Graphics/3D/Camera3D.h"
#include <x3daudio.h>

namespace GX
{

AudioListener::AudioListener()
{
    // デフォルトのリスナー: 原点、前方Z+、上方Y+
    m_listener = {};
    m_listener.OrientFront = { 0.0f, 0.0f, 1.0f };
    m_listener.OrientTop   = { 0.0f, 1.0f, 0.0f };
    m_listener.Position    = { 0.0f, 0.0f, 0.0f };
    m_listener.Velocity    = { 0.0f, 0.0f, 0.0f };
}

void AudioListener::SetPosition(const XMFLOAT3& pos)
{
    m_listener.Position = { pos.x, pos.y, pos.z };
}

void AudioListener::SetOrientation(const XMFLOAT3& front, const XMFLOAT3& up)
{
    m_listener.OrientFront = { front.x, front.y, front.z };
    m_listener.OrientTop   = { up.x, up.y, up.z };
}

void AudioListener::SetVelocity(const XMFLOAT3& vel)
{
    m_listener.Velocity = { vel.x, vel.y, vel.z };
}

void AudioListener::UpdateFromCamera(const Camera3D& camera, float deltaTime)
{
    // カメラの位置・方向を取得
    XMFLOAT3 pos = camera.GetPosition();
    XMFLOAT3 fwd = camera.GetForward();
    XMFLOAT3 up  = camera.GetUp();

    // 速度を前フレームの位置差分から計算（ドップラー効果用）
    if (deltaTime > 0.0f)
    {
        float invDt = 1.0f / deltaTime;
        m_listener.Velocity.x = (pos.x - m_prevPosition.x) * invDt;
        m_listener.Velocity.y = (pos.y - m_prevPosition.y) * invDt;
        m_listener.Velocity.z = (pos.z - m_prevPosition.z) * invDt;
    }

    m_listener.Position    = { pos.x, pos.y, pos.z };
    m_listener.OrientFront = { fwd.x, fwd.y, fwd.z };
    m_listener.OrientTop   = { up.x, up.y, up.z };

    m_prevPosition = pos;
}

} // namespace GX
