/// @file VRSGenerate.hlsl
/// @brief VRS シェーディングレート画像生成コンピュートシェーダ
///
/// HDRレンダーターゲットの輝度分散と速度バッファの動き量を分析し、
/// タイル単位の最適なシェーディングレートを算出する。
///
/// - 輝度分散が大きい（コントラストが高い）領域 → 1x1 (最高品質)
/// - 輝度が均一な領域 → 2x2 (品質を下げて高速化)
/// - 高速に動いている領域 → 2x2 or 4x4 (モーションブラーで目立たない)

// 入力
Texture2D<float4> g_HDRTexture   : register(t0); // HDRシーンカラー
Texture2D<float2> g_VelocityTex  : register(t1); // 速度バッファ (画面空間ピクセル単位)

// 出力 (SRI画像)
RWTexture2D<uint> g_ShadingRate  : register(u0);

// 定数バッファ
cbuffer VRSConstants : register(b0)
{
    uint2 g_ScreenSize;      // 画面解像度
    uint2 g_TileCount;       // SRIタイル数 (g_ScreenSize / tileSize)
    uint  g_TileSize;        // タイルサイズ (通常16)
    float g_LuminanceThreshold;  // 輝度分散しきい値 (デフォルト: 0.05)
    float g_VelocityThreshold;   // 速度しきい値 (デフォルト: 4.0 pixels)
    float g_Padding;
};

// D3D12_SHADING_RATE値の定義
#define SHADING_RATE_1X1 0x0   // D3D12_SHADING_RATE_1X1
#define SHADING_RATE_1X2 0x1   // D3D12_SHADING_RATE_1X2
#define SHADING_RATE_2X1 0x4   // D3D12_SHADING_RATE_2X1
#define SHADING_RATE_2X2 0x5   // D3D12_SHADING_RATE_2X2
#define SHADING_RATE_2X4 0x6   // D3D12_SHADING_RATE_2X4
#define SHADING_RATE_4X2 0x9   // D3D12_SHADING_RATE_4X2
#define SHADING_RATE_4X4 0xA   // D3D12_SHADING_RATE_4X4

/// 輝度を計算する (Rec.709 coefficients)
float ComputeLuminance(float3 color)
{
    return dot(color, float3(0.2126, 0.7152, 0.0722));
}

/// 1タイルにつき1スレッド
[numthreads(8, 8, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint2 tileCoord = dispatchThreadId.xy;

    // 範囲外チェック
    if (tileCoord.x >= g_TileCount.x || tileCoord.y >= g_TileCount.y)
        return;

    // タイル内のピクセル範囲
    uint2 pixelStart = tileCoord * g_TileSize;
    uint2 pixelEnd   = min(pixelStart + g_TileSize, g_ScreenSize);

    // タイル内の輝度統計を計算
    float lumSum    = 0.0;
    float lumSumSq  = 0.0;
    float maxVelocity = 0.0;
    uint  sampleCount = 0;

    // タイル内を4x4のサブサンプリングで走査（全ピクセルは重すぎるため）
    uint stepX = max(1u, (pixelEnd.x - pixelStart.x) / 4u);
    uint stepY = max(1u, (pixelEnd.y - pixelStart.y) / 4u);

    for (uint y = pixelStart.y; y < pixelEnd.y; y += stepY)
    {
        for (uint x = pixelStart.x; x < pixelEnd.x; x += stepX)
        {
            float3 color = g_HDRTexture[uint2(x, y)].rgb;
            float  lum   = ComputeLuminance(color);

            lumSum   += lum;
            lumSumSq += lum * lum;

            float2 vel = g_VelocityTex[uint2(x, y)].xy;
            float  velMag = length(vel);
            maxVelocity = max(maxVelocity, velMag);

            sampleCount++;
        }
    }

    // 輝度分散を計算 (Var = E[X^2] - E[X]^2)
    float invN = 1.0 / max(1.0, (float)sampleCount);
    float mean     = lumSum * invN;
    float variance = lumSumSq * invN - mean * mean;

    // シェーディングレートを決定
    uint shadingRate = SHADING_RATE_1X1;

    if (maxVelocity > g_VelocityThreshold * 2.0)
    {
        // 非常に速い動き → 4x4 (最大削減)
        shadingRate = SHADING_RATE_4X4;
    }
    else if (maxVelocity > g_VelocityThreshold)
    {
        // 中程度の動き → 2x2
        shadingRate = SHADING_RATE_2X2;
    }
    else if (variance < g_LuminanceThreshold * 0.25)
    {
        // 非常に均一な領域 → 2x2
        shadingRate = SHADING_RATE_2X2;
    }
    else if (variance < g_LuminanceThreshold)
    {
        // やや均一 → 1x2 (水平方向だけ削減)
        shadingRate = SHADING_RATE_1X2;
    }
    // else: 高コントラスト → 1x1 (最高品質)

    g_ShadingRate[tileCoord] = shadingRate;
}
