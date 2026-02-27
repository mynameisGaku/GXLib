#pragma once
/// @file AudioListener.h
/// @brief 3Dオーディオのリスナー（聴取者位置）
///
/// X3DAudioにおけるリスナーは、ゲーム世界での「耳の位置」を表す。
/// SetPosition/SetOrientation で位置・向きを設定し、
/// UpdateFromTransform で位置変化から速度を自動計算する。

#include "pch_audio.h"
#include <x3daudio.h>

namespace gx
{

/// @brief 3Dオーディオのリスナー（聴取者）
///
/// 位置と方向をX3DAUDIO_LISTENERに変換する。
/// SoundPlayer::Update3D()に渡すことで、全3Dサウンドの空間定位が更新される。
class AudioListener
{
public:
    AudioListener();

    /// @brief リスナーの位置を設定する
    /// @param pos ワールド座標
    void SetPosition(const XMFLOAT3& pos);

    /// @brief リスナーの向きを設定する
    /// @param front 前方向ベクトル（正規化済み）
    /// @param up 上方向ベクトル（正規化済み）
    void SetOrientation(const XMFLOAT3& front, const XMFLOAT3& up);

    /// @brief リスナーの速度を設定する（ドップラー効果計算用）
    /// @param vel 速度ベクトル（ワールド単位/秒）
    void SetVelocity(const XMFLOAT3& vel);

    /// @brief 位置・向きからリスナーを更新し、速度を自動計算する
    /// @param position ワールド座標
    /// @param forward 前方向ベクトル（正規化済み）
    /// @param up 上方向ベクトル（正規化済み）
    /// @param deltaTime フレーム経過時間（速度計算用）
    void UpdateFromTransform(const XMFLOAT3& position, const XMFLOAT3& forward,
                              const XMFLOAT3& up, float deltaTime);

    /// @brief X3DAudio用のネイティブリスナー構造体を取得する
    /// @return X3DAUDIO_LISTENERへのconst参照
    const X3DAUDIO_LISTENER& GetNative() const { return m_listener; }

private:
    X3DAUDIO_LISTENER m_listener = {};
    XMFLOAT3 m_prevPosition = { 0.0f, 0.0f, 0.0f };
};

} // namespace gx
