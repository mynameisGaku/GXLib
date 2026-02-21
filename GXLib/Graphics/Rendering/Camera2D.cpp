/// @file Camera2D.cpp
/// @brief Camera2D の実装
///
/// ビュー行列は「画面中心を原点にしてズーム・回転し、カメラ位置分だけ逆移動」する構成。
/// SpriteBatch / PrimitiveBatch の SetProjectionMatrix() に渡す想定。
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

    // ビュー行列の構築手順:
    //   (1) toCenter:   画面中心を原点にずらす（ズーム・回転の中心を画面中央にするため）
    //   (2) scale:      ズーム倍率を適用
    //   (3) rotation:   Z軸周り回転
    //   (4) fromCenter: 画面中心に戻しつつ、カメラ位置分の逆移動を加える
    // これらを左から右に乗算して合成する（行ベクトル規約）

    XMMATRIX toCenter   = XMMatrixTranslation(-hw, -hh, 0.0f);
    XMMATRIX scale      = XMMatrixScaling(m_zoom, m_zoom, 1.0f);
    XMMATRIX rotation   = XMMatrixRotationZ(m_rotation);
    XMMATRIX fromCenter = XMMatrixTranslation(hw - m_posX * m_zoom, hh - m_posY * m_zoom, 0.0f);

    XMMATRIX view = toCenter * scale * rotation * fromCenter;

    return view * projection;
}

} // namespace GX
