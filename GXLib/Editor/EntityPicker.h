#pragma once
/// @file EntityPicker.h
/// @brief マウスクリックで3Dシーン上のエンティティを選択するピッキングユーティリティ
///
/// スクリーン座標からワールド空間レイを飛ばし、エンティティのAABBと交差判定する。
/// エディタでオブジェクトをクリック選択する機能の実装に使う。
/// @addtogroup grp_editor/// @{

#include "pch_graphics.h"
#include "Math/Vector3.h"

namespace gx
{

class Scene;
class Entity;
class Camera3D;

/// @brief マウス生成レイでシーンからエンティティをピッキングするユーティリティ
class EntityPicker
{
public:
    /// @brief スクリーン座標からワールド空間レイを計算する
    /// @param screenX      ピクセルX座標（左端=0）
    /// @param screenY      ピクセルY座標（上端=0）
    /// @param screenWidth  ビューポート幅（ピクセル）
    /// @param screenHeight ビューポート高さ（ピクセル）
    /// @param viewProj     ビュー・プロジェクション合成行列
    /// @param outOrigin    [出力] ワールド空間のレイ原点
    /// @param outDirection [出力] ワールド空間の正規化レイ方向
    static void ScreenToRay(float screenX, float screenY,
                              float screenWidth, float screenHeight,
                              const DirectX::XMMATRIX& viewProj,
                              DirectX::XMFLOAT3& outOrigin,
                              DirectX::XMFLOAT3& outDirection);

    /// @brief スクリーン空間レイと交差する最も近いエンティティをピッキングする
    /// @return ヒットしたエンティティへのポインタ、何もヒットしない場合nullptr
    static Entity* PickEntity(Scene& scene,
                               float screenX, float screenY,
                               float screenWidth, float screenHeight,
                               const Camera3D& camera);

    /// @brief レイ対AABBの交差判定（スラブ法）
    /// @return 交差が存在する場合の侵入距離（t >= 0）、交差なしの場合-1.0f
    static float RayAABBIntersect(const DirectX::XMFLOAT3& rayOrigin,
                                    const DirectX::XMFLOAT3& rayDir,
                                    const DirectX::XMFLOAT3& aabbMin,
                                    const DirectX::XMFLOAT3& aabbMax);
};

} // namespace gx
/// @}
