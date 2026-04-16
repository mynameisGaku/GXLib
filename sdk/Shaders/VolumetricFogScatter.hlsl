/// @file VolumetricFogScatter.hlsl
/// @brief フロント->バックレイマーチ -- Beer-Lambert蓄積
///
/// 各ピクセル列に対してZ=0からZ=maxまでフロント→バックで走査し、
/// Beer-Lambert法則に従って透過率と散乱光を蓄積する。
/// 結果はFroxelグリッドの各スライスに書き戻され、
/// Composeパスで深度に応じてサンプリングされる。
///
/// transmittance = exp(-integral(sigma_t * ds))
/// in-scatter = scattering * (1 - transmittance) / extinction

RWTexture3D<float4> uFogVolume : register(u0);

cbuffer FogScatterCB : register(b0)
{
    uint3  gResolution;
    float  gSliceDepth;
};

/// @brief Beer-Lambert蓄積コンピュートシェーダー
///
/// 各XYピクセル列についてZ方向にフロント→バックで走査し、
/// 散乱光と透過率を累積する。単一散乱近似を使用。
[numthreads(8, 8, 1)]
void CSMain(uint3 dtid : SV_DispatchThreadID)
{
    if (dtid.x >= gResolution.x || dtid.y >= gResolution.y) return;

    // 蓄積バッファ: .rgb = 散乱光, .a = 透過率 (初期値=1.0)
    float4 accum = float4(0, 0, 0, 1);

    for (uint z = 0; z < gResolution.z; ++z)
    {
        float4 voxel = uFogVolume[uint3(dtid.xy, z)];
        float3 scattering = voxel.rgb;   // 散乱色 * 密度
        float extinction = voxel.a;       // 密度 * 吸収率

        float sliceThickness = gSliceDepth;

        // Beer-Lambert: 1スライスの透過率
        float transmittance = exp(-extinction * sliceThickness);

        // 単一散乱近似: in-scatter = S * (1 - T) / sigma_t
        float3 inScatter = scattering * (1.0 - transmittance) / max(extinction, 0.0001);

        // 蓄積: 現在の透過率で重み付けして加算
        accum.rgb += inScatter * accum.a;
        accum.a *= transmittance;

        // 結果をFroxelに書き戻し (Composeパスでサンプリング)
        uFogVolume[uint3(dtid.xy, z)] = accum;
    }
}
