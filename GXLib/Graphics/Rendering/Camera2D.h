#pragma once
/// @file Camera2D.h
/// @brief 2Dカメラ管理クラス
///
/// 【初学者向け解説】
/// Camera2Dは、2Dゲームにおける「カメラ」の動きを管理します。
/// カメラを動かすと、画面に映る世界の範囲が変わります。
///
/// 機能：
/// - 位置（パン）: カメラの位置を変えると画面がスクロール
/// - ズーム: 拡大/縮小
/// - 回転: 画面全体を回転
///
/// SpriteBatchやPrimitiveBatchの正射影行列を差し替えることで実現します。

#include "pch.h"

namespace GX
{

/// @brief 2Dカメラクラス
class Camera2D
{
public:
    Camera2D() = default;
    ~Camera2D() = default;

    /// カメラ位置を設定（ワールド座標）
    void SetPosition(float x, float y) { m_posX = x; m_posY = y; }

    /// ズーム倍率を設定（1.0 = 等倍、2.0 = 2倍拡大）
    void SetZoom(float scale) { m_zoom = scale; }

    /// 回転角度を設定（ラジアン）
    void SetRotation(float angle) { m_rotation = angle; }

    /// ビュープロジェクション行列を取得
    /// @param screenWidth スクリーン幅
    /// @param screenHeight スクリーン高さ
    XMMATRIX GetViewProjectionMatrix(uint32_t screenWidth, uint32_t screenHeight) const;

    float GetPositionX() const { return m_posX; }
    float GetPositionY() const { return m_posY; }
    float GetZoom() const { return m_zoom; }
    float GetRotation() const { return m_rotation; }

private:
    float m_posX     = 0.0f;
    float m_posY     = 0.0f;
    float m_zoom     = 1.0f;
    float m_rotation = 0.0f;
};

} // namespace GX
