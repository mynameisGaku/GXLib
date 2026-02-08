/// @file Camera2D.cpp
/// @brief 2Dカメラの実装
#include "pch.h"
#include "Graphics/Rendering/Camera2D.h"

namespace GX
{

XMMATRIX Camera2D::GetViewProjectionMatrix(uint32_t screenWidth, uint32_t screenHeight) const
{
    float sw = static_cast<float>(screenWidth);
    float sh = static_cast<float>(screenHeight);
    float hw = sw * 0.5f;
    float hh = sh * 0.5f;

    // ビュー変換: カメラの移動・回転・ズームの逆変換
    // 1. 画面中心を原点に移動
    // 2. ズーム適用
    // 3. 回転適用
    // 4. カメラ位置分の平行移動

    // 正射影行列（左上原点、Y下向き）
    XMMATRIX projection = XMMatrixOrthographicOffCenterLH(
        0.0f, sw, sh, 0.0f, 0.0f, 1.0f
    );

    // ビュー行列構築
    // カメラ中心を画面中心にするために:
    // 1. 画面中心を原点にシフト
    // 2. ズーム・回転
    // 3. 元に戻す + カメラオフセット

    XMMATRIX toCenter   = XMMatrixTranslation(-hw, -hh, 0.0f);
    XMMATRIX scale      = XMMatrixScaling(m_zoom, m_zoom, 1.0f);
    XMMATRIX rotation   = XMMatrixRotationZ(m_rotation);
    XMMATRIX fromCenter = XMMatrixTranslation(hw - m_posX * m_zoom, hh - m_posY * m_zoom, 0.0f);

    XMMATRIX view = toCenter * scale * rotation * fromCenter;

    return view * projection;
}

} // namespace GX
