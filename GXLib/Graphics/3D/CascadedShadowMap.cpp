/// @file CascadedShadowMap.cpp
/// @brief カスケードシャドウマップの実装
#include "pch.h"
#include "Graphics/3D/CascadedShadowMap.h"
#include "Core/Logger.h"

namespace GX
{

bool CascadedShadowMap::Initialize(ID3D12Device* device, DescriptorHeap* srvHeap, uint32_t srvStartIndex)
{
    // 各カスケードのシャドウマップ作成（SRVは外部ヒープに配置）
    for (uint32_t i = 0; i < k_NumCascades; ++i)
    {
        if (!m_shadowMaps[i].Create(device, k_ShadowMapSize, srvHeap, srvStartIndex + i))
        {
            GX_LOG_ERROR("CascadedShadowMap: Failed to create cascade %d", i);
            return false;
        }
    }

    // 先頭SRVのGPUハンドルを保存
    m_srvGPUHandleStart = srvHeap->GetGPUHandle(srvStartIndex);

    m_constants.shadowMapSize = static_cast<float>(k_ShadowMapSize);

    GX_LOG_INFO("CascadedShadowMap initialized (%d cascades, %dx%d, SRV index %d-%d)",
                k_NumCascades, k_ShadowMapSize, k_ShadowMapSize,
                srvStartIndex, srvStartIndex + k_NumCascades - 1);
    return true;
}

void CascadedShadowMap::SetCascadeSplits(float s0, float s1, float s2, float s3)
{
    m_cascadeRatios = { s0, s1, s2, s3 };
}

void CascadedShadowMap::Update(const Camera3D& camera, const XMFLOAT3& lightDirection)
{
    float nearZ = camera.GetNearZ();
    float farZ  = camera.GetFarZ();

    // PSSM分割: 対数分割と線形分割をlambdaでブレンド (Practical Split Scheme)
    constexpr float lambda = 0.5f;  // 0=完全線形, 1=完全対数
    float ratio = farZ / nearZ;

    float prevSplit = nearZ;
    for (uint32_t i = 0; i < k_NumCascades; ++i)
    {
        float p = static_cast<float>(i + 1) / static_cast<float>(k_NumCascades);
        float logSplit = nearZ * std::pow(ratio, p);
        float linSplit = nearZ + (farZ - nearZ) * p;
        float splitDist = lambda * logSplit + (1.0f - lambda) * linSplit;
        m_constants.cascadeSplits[i] = splitDist;

        ComputeCascadeLightVP(i, camera, lightDirection, prevSplit, splitDist);
        prevSplit = splitDist;
    }
}

void CascadedShadowMap::ComputeCascadeLightVP(uint32_t cascade, const Camera3D& camera,
                                                const XMFLOAT3& lightDirection,
                                                float nearZ, float farZ)
{
    // ビュー空間で直接フラスタムコーナーを構築（NDC非線形深度の問題を回避）
    XMMATRIX invView = XMMatrixInverse(nullptr, camera.GetViewMatrix());

    float tanHalfFovY = tanf(camera.GetFovY() * 0.5f);
    float tanHalfFovX = tanHalfFovY * camera.GetAspect();

    float nH = nearZ * tanHalfFovY;
    float nW = nearZ * tanHalfFovX;
    float fH = farZ * tanHalfFovY;
    float fW = farZ * tanHalfFovX;

    XMVECTOR frustumCorners[8] =
    {
        // ニア面（ビュー空間、左手系: +Zが前方）
        XMVectorSet(-nW, -nH, nearZ, 1.0f),
        XMVectorSet( nW, -nH, nearZ, 1.0f),
        XMVectorSet( nW,  nH, nearZ, 1.0f),
        XMVectorSet(-nW,  nH, nearZ, 1.0f),
        // ファー面
        XMVectorSet(-fW, -fH, farZ, 1.0f),
        XMVectorSet( fW, -fH, farZ, 1.0f),
        XMVectorSet( fW,  fH, farZ, 1.0f),
        XMVectorSet(-fW,  fH, farZ, 1.0f),
    };

    // ビュー空間 → ワールド空間
    XMVECTOR center = XMVectorZero();
    for (int i = 0; i < 8; ++i)
    {
        frustumCorners[i] = XMVector4Transform(frustumCorners[i], invView);
        center = XMVectorAdd(center, frustumCorners[i]);
    }
    center = XMVectorScale(center, 1.0f / 8.0f);

    // ライトのビュー行列（ライト方向を見下ろす正射影）
    XMVECTOR lightDir = XMVector3Normalize(XMLoadFloat3(&lightDirection));
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    // ライトが真上/真下を向いている場合の対策
    if (XMVectorGetX(XMVector3LengthSq(XMVector3Cross(lightDir, up))) < 0.001f)
        up = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

    XMMATRIX lightView = XMMatrixLookAtLH(
        XMVectorSubtract(center, XMVectorScale(lightDir, 50.0f)),  // eye: centerから後退
        center,
        up
    );

    // フラスタム頂点をライトビュー空間に変換してAABB計算
    float minX =  FLT_MAX, minY =  FLT_MAX, minZ =  FLT_MAX;
    float maxX = -FLT_MAX, maxY = -FLT_MAX, maxZ = -FLT_MAX;

    for (int i = 0; i < 8; ++i)
    {
        XMVECTOR v = XMVector3Transform(frustumCorners[i], lightView);
        float x = XMVectorGetX(v);
        float y = XMVectorGetY(v);
        float z = XMVectorGetZ(v);
        minX = (std::min)(minX, x); maxX = (std::max)(maxX, x);
        minY = (std::min)(minY, y); maxY = (std::max)(maxY, y);
        minZ = (std::min)(minZ, z); maxZ = (std::max)(maxZ, z);
    }

    // Z範囲を拡張（シーン外のキャスターもカバーするため）
    float zExtend = maxZ - minZ;
    minZ -= zExtend * 0.5f;

    // ライトの正射影行列
    XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(minX, maxX, minY, maxY, minZ, maxZ);
    XMMATRIX lightVP = lightView * lightProj;

    XMStoreFloat4x4(&m_constants.lightVP[cascade], XMMatrixTranspose(lightVP));
}

} // namespace GX
