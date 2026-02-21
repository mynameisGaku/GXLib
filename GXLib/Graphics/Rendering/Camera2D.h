#pragma once
/// @file Camera2D.h
/// @brief 2Dカメラ（スクロール・ズーム・回転）
///
/// 2Dゲームの視点制御を担当する。位置・ズーム倍率・回転角からビュープロジェクション行列を生成し、
/// SpriteBatch / PrimitiveBatch の SetProjectionMatrix() に渡すことで
/// ワールド座標ベースのスクロールやズームを実現する。
///
/// DxLib には直接対応する関数はないが、DrawGraph 前にカメラ変換を掛ける概念に近い。

#include "pch.h"

namespace GX
{

/// @brief 2Dカメラクラス（位置・ズーム・回転を持つ）
class Camera2D
{
public:
    Camera2D() = default;
    ~Camera2D() = default;

    /// @brief カメラの位置をワールド座標で設定する
    /// @param x X座標（右が正）
    /// @param y Y座標（下が正）
    void SetPosition(float x, float y) { m_posX = x; m_posY = y; }

    /// @brief ズーム倍率を設定する
    /// @param scale 1.0 で等倍、2.0 で2倍拡大、0.5 で半分に縮小
    void SetZoom(float scale) { m_zoom = scale; }

    /// @brief 回転角度を設定する
    /// @param angle ラジアン（正の値で時計回り）
    void SetRotation(float angle) { m_rotation = angle; }

    /// @brief ビュープロジェクション行列を計算して返す
    ///
    /// 画面中心を軸にズーム・回転し、カメラ位置分のオフセットを適用した行列を返す。
    /// この行列を SpriteBatch::SetProjectionMatrix() に渡すとカメラが反映される。
    ///
    /// @param screenWidth スクリーン幅（ピクセル）
    /// @param screenHeight スクリーン高さ（ピクセル）
    /// @return ビュー * プロジェクション行列
    XMMATRIX GetViewProjectionMatrix(uint32_t screenWidth, uint32_t screenHeight) const;

    /// @brief カメラのX座標を取得する
    float GetPositionX() const { return m_posX; }
    /// @brief カメラのY座標を取得する
    float GetPositionY() const { return m_posY; }
    /// @brief ズーム倍率を取得する
    float GetZoom() const { return m_zoom; }
    /// @brief 回転角度を取得する（ラジアン）
    float GetRotation() const { return m_rotation; }

private:
    float m_posX     = 0.0f;   ///< ワールド座標X
    float m_posY     = 0.0f;   ///< ワールド座標Y
    float m_zoom     = 1.0f;   ///< ズーム倍率（1.0 = 等倍）
    float m_rotation = 0.0f;   ///< 回転角（ラジアン）
};

} // namespace GX
