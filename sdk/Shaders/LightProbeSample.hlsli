/// @file LightProbeSample.hlsli
/// @brief ライティングシェーダーでDDGIプローブデータをサンプリングするユーティリティ関数。

#ifndef LIGHT_PROBE_SAMPLE_HLSLI
#define LIGHT_PROBE_SAMPLE_HLSLI

/// プローブグリッドから放射照度をサンプリング。
/// @param irradianceAtlas 放射照度アトラステクスチャ
/// @param samplerState リニアサンプラー
/// @param worldPos 評価するワールド位置
/// @param normal サーフェス法線
/// @param gridOrigin プローブグリッドの原点
/// @param gridSpacing プローブグリッドの間隔
/// @param gridDimensions プローブグリッドの次元
/// @param irradianceTexWidth 放射照度アトラスのテクセル幅
/// @return 放射照度カラー（RGB）
float3 SampleProbeIrradiance(
    Texture2D<float4> irradianceAtlas,
    SamplerState samplerState,
    float3 worldPos,
    float3 normal,
    float3 gridOrigin,
    float3 gridSpacing,
    uint3 gridDimensions,
    int irradianceTexWidth)
{
    // 最も近いプローブを探索
    float3 relativePos = (worldPos - gridOrigin) / gridSpacing;
    int3 probeIdx = clamp(int3(relativePos + 0.5), int3(0, 0, 0),
                          int3(gridDimensions) - int3(1, 1, 1));

    uint probeLinear = probeIdx.z * gridDimensions.x * gridDimensions.y +
                       probeIdx.y * gridDimensions.x + probeIdx.x;

    // このプローブのアトラスUVを計算
    int probesPerRow = irradianceTexWidth / 6;
    float2 atlasUV;
    atlasUV.x = float((probeLinear % probesPerRow) * 6 + 3) / float(irradianceTexWidth);
    int atlasHeight = ((int(gridDimensions.x * gridDimensions.y * gridDimensions.z) +
                        probesPerRow - 1) / probesPerRow) * 6;
    atlasUV.y = float((probeLinear / probesPerRow) * 6 + 3) / float(atlasHeight);

    return irradianceAtlas.SampleLevel(samplerState, atlasUV, 0).rgb;
}

#endif // LIGHT_PROBE_SAMPLE_HLSLI
