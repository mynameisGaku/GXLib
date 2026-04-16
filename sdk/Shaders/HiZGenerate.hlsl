/// @file HiZGenerate.hlsl
/// @brief Hi-Z ミップマップ生成 Compute Shader
///
/// 深度バッファの各ミップレベルを生成する。
/// ミップN+1の各ピクセル = ミップNの2x2ピクセルの最大深度（保守的カリング用）。

cbuffer HiZConstants : register(b0)
{
    uint2 srcDimensions;  // ソースミップの幅・高さ
    uint2 dstDimensions;  // 出力ミップの幅・高さ
};

Texture2D<float> srcMip : register(t0);
RWTexture2D<float> dstMip : register(u0);

[numthreads(8, 8, 1)]
void CSMain(uint3 tid : SV_DispatchThreadID)
{
    if (tid.x >= dstDimensions.x || tid.y >= dstDimensions.y)
        return;

    // ソースミップから2x2領域の最大深度を取得
    uint2 srcCoord = tid.xy * 2;

    float d00 = srcMip[srcCoord + uint2(0, 0)];
    float d10 = srcMip[min(srcCoord + uint2(1, 0), srcDimensions - 1)];
    float d01 = srcMip[min(srcCoord + uint2(0, 1), srcDimensions - 1)];
    float d11 = srcMip[min(srcCoord + uint2(1, 1), srcDimensions - 1)];

    // 最大深度（遠い方 = 保守的、前方に障害物がないと判定しやすくする）
    dstMip[tid.xy] = max(max(d00, d10), max(d01, d11));
}
