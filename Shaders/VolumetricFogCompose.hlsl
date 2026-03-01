/// @file VolumetricFogCompose.hlsl
/// @brief ボリューメトリックフォグ合成 -- HDRシーンにフォグを加算
///
/// 深度バッファからピクセルの線形深度を求め、Froxelグリッドの対応するZスライスから
/// 散乱光と透過率をサンプリングしてシーンに合成する。
/// 合成式: result = scene * transmittance + in-scattering

Texture2D<float4>    tScene     : register(t0);
Texture3D<float4>    tFogVolume : register(t1);
Texture2D<float>     tDepth     : register(t2);
SamplerState         sLinear    : register(s0);

cbuffer FogComposeCB : register(b0)
{
    float gMaxDistance;
    float gNearZ;
    float gFarZ;
    float _pad;
};

struct PSInput
{
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD;
};

/// @brief 非線形深度を線形深度に変換する
/// @param d 非線形深度値 [0, 1]
/// @param nearZ ニアクリップ距離
/// @param farZ ファークリップ距離
/// @return 線形深度 (ワールド単位)
float LinearizeDepth(float d, float nearZ, float farZ)
{
    return nearZ * farZ / (farZ - d * (farZ - nearZ));
}

/// @brief フォグをシーンに合成するピクセルシェーダー
float4 PSMain(PSInput input) : SV_Target0
{
    float4 scene = tScene.Sample(sLinear, input.uv);
    float depth = tDepth.Sample(sLinear, input.uv);
    float linearDepth = LinearizeDepth(depth, gNearZ, gFarZ);

    // 線形深度をFroxelのZ座標に変換 (二次分布の逆変換)
    float normalizedDepth = saturate(linearDepth / gMaxDistance);
    float froxelZ = sqrt(normalizedDepth);

    // Froxelグリッドから散乱光+透過率をサンプリング
    float3 uvw = float3(input.uv, froxelZ);
    float4 fog = tFogVolume.SampleLevel(sLinear, uvw, 0);

    // 合成: scene * transmittance + in-scattering
    float3 result = scene.rgb * fog.a + fog.rgb;
    return float4(result, scene.a);
}
