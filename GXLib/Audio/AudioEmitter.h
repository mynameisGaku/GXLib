#pragma once
/// @file AudioEmitter.h
/// @brief 3Dオーディオのエミッター（音源位置）
///
/// エミッターは3D空間内の音源を表す。
/// 位置・速度・距離減衰カーブ・指向性コーンの設定ができる。
/// SoundPlayer::Play3D()に渡すことで、リスナーとの相対位置に応じた
/// パンニングとドップラー効果が自動的に適用される。

#include "pch.h"
#include <x3daudio.h>

namespace GX
{

/// @brief 3D空間内の音源
///
/// 各音源の位置・速度・減衰範囲を設定し、X3DAUDIO_EMITTERとして渡す。
/// 指向性コーンを設定すると、音源の向きによって音量が変化する。
class AudioEmitter
{
public:
    AudioEmitter();

    /// @brief 音源の位置を設定する
    /// @param pos ワールド座標
    void SetPosition(const XMFLOAT3& pos);

    /// @brief 音源の位置を取得する
    /// @return ワールド座標
    XMFLOAT3 GetPosition() const { return m_position; }

    /// @brief 音源の速度を設定する（ドップラー効果計算用）
    /// @param vel 速度ベクトル（ワールド単位/秒）
    void SetVelocity(const XMFLOAT3& vel);

    /// @brief 音源の向きを設定する（指向性コーン用）
    /// @param front 前方向ベクトル（正規化済み）
    void SetDirection(const XMFLOAT3& front);

    /// @brief 内側半径を設定する（この範囲内はフル音量）
    /// @param radius 半径（ワールド単位）
    void SetInnerRadius(float radius);

    /// @brief 最大距離を設定する（この距離以遠は無音）
    /// @param distance 最大距離（ワールド単位）
    void SetMaxDistance(float distance);

    /// @brief 指向性コーンを設定する
    /// @param innerAngle 内側角度（ラジアン、この範囲内はフル音量）
    /// @param outerAngle 外側角度（ラジアン、この範囲外はouterVolume）
    /// @param outerVolume 外側の音量係数（0.0〜1.0）
    void SetCone(float innerAngle, float outerAngle, float outerVolume = 0.0f);

    /// @brief コーンを無効化する（全方向均等に鳴る）
    void DisableCone();

    /// @brief ソースチャンネル数を設定する（通常はモノラル=1）
    /// @param channels チャンネル数
    void SetChannelCount(uint32_t channels);

    /// @brief X3DAudio用のネイティブエミッター構造体を取得する
    /// @return X3DAUDIO_EMITTERへのconst参照
    const X3DAUDIO_EMITTER& GetNative() const { return m_emitter; }

    /// @brief 可変のネイティブエミッター構造体を取得する（SoundPlayer内部用）
    /// @return X3DAUDIO_EMITTERへの参照
    X3DAUDIO_EMITTER& GetNativeMutable() { return m_emitter; }

private:
    void UpdateNative();

    X3DAUDIO_EMITTER m_emitter = {};
    X3DAUDIO_CONE    m_cone = {};
    XMFLOAT3 m_position  = { 0.0f, 0.0f, 0.0f };
    XMFLOAT3 m_velocity  = { 0.0f, 0.0f, 0.0f };
    XMFLOAT3 m_direction = { 0.0f, 0.0f, 1.0f };
    float m_innerRadius  = 1.0f;
    float m_maxDistance   = 100.0f;
    bool  m_useCone      = false;
    uint32_t m_channels  = 1;

    /// 距離減衰カーブ（線形減衰: 0→フル音量、maxDistance→無音）
    X3DAUDIO_DISTANCE_CURVE_POINT m_curvePoints[2] = {};
    X3DAUDIO_DISTANCE_CURVE m_distanceCurve = {};
};

} // namespace GX
