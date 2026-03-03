/// @file EntityPicker.cpp
/// @brief 対応する.hの実装
#include "pch_graphics.h"
#include "Editor/EntityPicker.h"
#include "Math/MathConvert.h"
#include "Core/Scene/Scene.h"
#include "Core/Scene/Entity.h"
#include "Graphics/3D/Camera3D.h"
#include "Core/Logger.h"

using namespace DirectX;

namespace gx
{

// ============================================================================
// ScreenToRay
// ============================================================================
void EntityPicker::ScreenToRay(float screenX, float screenY,
                                 float screenWidth, float screenHeight,
                                 const XMMATRIX& viewProj,
                                 Vector3& outOrigin,
                                 Vector3& outDirection)
{
    // スクリーン座標をNDC [-1, +1]に変換
    float ndcX =  (2.0f * screenX / screenWidth)  - 1.0f;
    float ndcY = -(2.0f * screenY / screenHeight) + 1.0f; // Y軸は反転（スクリーン上端=+1）

    // ビュー・プロジェクション行列の逆行列を計算
    XMMATRIX invVP = XMMatrixInverse(nullptr, viewProj);

    // ニアポイント（NDCでz=0）とファーポイント（NDCでz=1）を逆投影
    XMVECTOR nearPt = XMVector3TransformCoord(XMVectorSet(ndcX, ndcY, 0.0f, 1.0f), invVP);
    XMVECTOR farPt  = XMVector3TransformCoord(XMVectorSet(ndcX, ndcY, 1.0f, 1.0f), invVP);

    // 方向 = normalize(far - near)
    XMVECTOR dir = XMVector3Normalize(XMVectorSubtract(farPt, nearPt));

    XMStoreFloat3(XM(&outOrigin), nearPt);
    XMStoreFloat3(XM(&outDirection), dir);
}

// ============================================================================
// PickEntity
// ============================================================================
Entity* EntityPicker::PickEntity(Scene& scene,
                                   float screenX, float screenY,
                                   float screenWidth, float screenHeight,
                                   const Camera3D& camera)
{
    Vector3 rayOrigin{};
    Vector3 rayDir{};

    XMMATRIX viewProj = ToXMMATRIX(camera.GetViewProjectionMatrix());
    ScreenToRay(screenX, screenY, screenWidth, screenHeight, viewProj, rayOrigin, rayDir);

    Entity* bestEntity = nullptr;
    float   bestT = FLT_MAX;

    const auto& entities = scene.GetEntities();
    for (const auto& entityPtr : entities)
    {
        Entity* entity = entityPtr.get();
        if (!entity || !entity->IsActive()) continue;

        const BoundsInfo& bounds = entity->GetBounds();
        if (!bounds.hasBounds) continue;

        // エンティティのローカルAABBとトランスフォームからワールド空間AABBを計算
        const Transform3D& transform = entity->GetTransform();
        const Vector3& pos   = transform.GetPosition();
        const Vector3& scale = transform.GetScale();

        // 簡略化のため、ローカルAABBをスケーリング・平行移動して使用（回転は無視
        // ブロードフェーズテスト用 — エディタピッキングの一般的なアプローチ）
        const AABB3D& localAABB = bounds.localAABB;

        Vector3 aabbMin = {
            pos.x + localAABB.min.x * scale.x,
            pos.y + localAABB.min.y * scale.y,
            pos.z + localAABB.min.z * scale.z
        };
        Vector3 aabbMax = {
            pos.x + localAABB.max.x * scale.x,
            pos.y + localAABB.max.y * scale.y,
            pos.z + localAABB.max.z * scale.z
        };

        // min <= max を保証（負のスケールで反転する可能性あり）
        if (aabbMin.x > aabbMax.x) std::swap(aabbMin.x, aabbMax.x);
        if (aabbMin.y > aabbMax.y) std::swap(aabbMin.y, aabbMax.y);
        if (aabbMin.z > aabbMax.z) std::swap(aabbMin.z, aabbMax.z);

        float t = RayAABBIntersect(rayOrigin, rayDir, aabbMin, aabbMax);
        if (t >= 0.0f && t < bestT)
        {
            bestT = t;
            bestEntity = entity;
        }
    }

    return bestEntity;
}

// ============================================================================
// RayAABBIntersect（スラブ法）
// ============================================================================
float EntityPicker::RayAABBIntersect(const Vector3& rayOrigin,
                                       const Vector3& rayDir,
                                       const Vector3& aabbMin,
                                       const Vector3& aabbMax)
{
    // 各軸について侵入・退出のt値を計算する
    // スラブ交差アルゴリズムを使用

    float tMin = -FLT_MAX;
    float tMax =  FLT_MAX;

    // 1軸を処理するヘルパーラムダ
    auto processSlab = [&](float origin, float dir, float bMin, float bMax) -> bool
    {
        if (std::abs(dir) < 1e-8f)
        {
            // レイがこのスラブに平行。原点が外側なら交差なし
            if (origin < bMin || origin > bMax)
                return false;
        }
        else
        {
            float invD = 1.0f / dir;
            float t1 = (bMin - origin) * invD;
            float t2 = (bMax - origin) * invD;

            if (t1 > t2) std::swap(t1, t2);

            if (t1 > tMin) tMin = t1;
            if (t2 < tMax) tMax = t2;

            if (tMin > tMax) return false;
        }
        return true;
    };

    if (!processSlab(rayOrigin.x, rayDir.x, aabbMin.x, aabbMax.x)) return -1.0f;
    if (!processSlab(rayOrigin.y, rayDir.y, aabbMin.y, aabbMax.y)) return -1.0f;
    if (!processSlab(rayOrigin.z, rayDir.z, aabbMin.z, aabbMax.z)) return -1.0f;

    // tMax < 0 はボックスが完全にレイ原点の後方にあることを意味する
    if (tMax < 0.0f) return -1.0f;

    // 侵入点を返す。tMin < 0 の場合、レイはボックス内部から開始; 0を返す
    return (tMin >= 0.0f) ? tMin : 0.0f;
}

} // namespace gx
