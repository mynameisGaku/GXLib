/// @file HiZCull.hlsl
/// @brief Hi-Z オクルージョンカリング Compute Shader
///
/// 各オブジェクトのスクリーン空間AABBをHi-Zバッファとテストし、
/// 遮蔽されていないオブジェクトのみを可視バッファに出力する。

cbuffer CullConstants : register(b0)
{
    float4x4 viewProjection;
    uint objectCount;
    uint2 hiZDimensions;  // Hi-Zテクスチャの幅・高さ（ミップ0）
    uint hiZMipLevels;
};

struct ObjectBounds
{
    float3 center;
    float  radius;  // バウンディング球半径
};

StructuredBuffer<ObjectBounds> inputBounds : register(t0);
Texture2D<float> hiZTexture : register(t1);
RWStructuredBuffer<uint> visibilityFlags : register(u0);

// NDCへ変換（0..1のスクリーン空間）
float4 ProjectSphere(float3 center, float radius)
{
    // 球の8隅をプロジェクト→スクリーン空間AABBを計算
    float4 clipCenter = mul(float4(center, 1.0), viewProjection);

    // 球がカメラの後ろにある場合は不可視
    if (clipCenter.w <= 0.0)
        return float4(-1, -1, -1, -1);

    float3 ndc = clipCenter.xyz / clipCenter.w;

    // NDC → UV (0..1)
    float2 screenCenter = ndc.xy * 0.5 + 0.5;
    screenCenter.y = 1.0 - screenCenter.y;  // Y反転

    // 球をスクリーン空間に投影した半径を近似
    float screenRadius = radius / clipCenter.w * 0.5;

    return float4(
        screenCenter.x - screenRadius,  // minU
        screenCenter.y - screenRadius,  // minV
        screenCenter.x + screenRadius,  // maxU
        screenCenter.y + screenRadius   // maxV
    );
}

[numthreads(64, 1, 1)]
void CSMain(uint3 tid : SV_DispatchThreadID)
{
    if (tid.x >= objectCount)
        return;

    ObjectBounds bounds = inputBounds[tid.x];

    // 球をスクリーン空間に投影
    float4 screenRect = ProjectSphere(bounds.center, bounds.radius);

    // カメラ後方 → 不可視
    if (screenRect.x < -1.0)
    {
        visibilityFlags[tid.x] = 0;
        return;
    }

    // スクリーン外 → 不可視
    if (screenRect.z < 0.0 || screenRect.x > 1.0 ||
        screenRect.w < 0.0 || screenRect.y > 1.0)
    {
        visibilityFlags[tid.x] = 0;
        return;
    }

    // スクリーン空間サイズからミップレベルを決定
    float screenWidth = (screenRect.z - screenRect.x) * hiZDimensions.x;
    float screenHeight = (screenRect.w - screenRect.y) * hiZDimensions.y;
    float screenSize = max(screenWidth, screenHeight);

    // ミップレベル = ceil(log2(screenSize)) — AABBが1-2ピクセルに収まるレベル
    int mipLevel = (int)ceil(log2(max(screenSize, 1.0)));
    mipLevel = clamp(mipLevel, 0, (int)hiZMipLevels - 1);

    // ミップレベルでのテクセル座標
    uint2 mipDim = hiZDimensions >> mipLevel;
    mipDim = max(mipDim, uint2(1, 1));

    // AABB中心のHi-Z深度をサンプル
    float2 center = (screenRect.xy + screenRect.zw) * 0.5;
    center = clamp(center, 0.0, 1.0);
    uint2 texCoord = uint2(center * mipDim);
    texCoord = min(texCoord, mipDim - 1);

    float hiZDepth = hiZTexture.mips[mipLevel][texCoord];

    // オブジェクトの最小深度（最前面）を計算
    float4 clipCenter = mul(float4(bounds.center, 1.0), viewProjection);
    float objectDepth = clipCenter.z / clipCenter.w;

    // オブジェクトの最前面がHi-Zの最大深度より手前 → 可視
    visibilityFlags[tid.x] = (objectDepth <= hiZDepth) ? 1 : 0;
}
