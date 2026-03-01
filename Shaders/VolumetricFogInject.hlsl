/// @file VolumetricFogInject.hlsl
/// @brief Froxel密度注入 -- FogVolumeからボクセルへ密度/色を書き込み
/// Beer-Lambert法則に基づく散乱媒質のパラメータ化
///
/// 各Froxelの3D位置をワールド空間に変換し、グローバル均一フォグと
/// 高度ベースの密度減衰を書き込む。局所FogVolumeは将来の拡張で追加。
/// 奥行き方向は二次分布 (quadratic) で近距離の解像度を高めている。

RWTexture3D<float4> uFogVolume : register(u0);

cbuffer FogInjectCB : register(b0)
{
    float4x4 gInvViewProj;
    float3   gCameraPos;
    float    gMaxDistance;
    float3   gFogColor;
    float    gGlobalDensity;
    float    gAbsorption;
    uint3    gResolution;     // 160, 90, 64
    float    _pad;
};

/// @brief Froxelグリッドにフォグ密度を注入するコンピュートシェーダー
[numthreads(8, 8, 4)]
void CSMain(uint3 dtid : SV_DispatchThreadID)
{
    if (any(dtid >= gResolution)) return;

    // Froxel UV座標 (ボクセル中心)
    float3 uvw = (float3(dtid) + 0.5) / float3(gResolution);

    // 二次分布で奥行きをマッピング (近距離の解像度を高める)
    float depth = uvw.z * uvw.z * gMaxDistance;

    // NDC空間の座標を計算
    float2 ndc = uvw.xy * 2.0 - 1.0;
    ndc.y = -ndc.y;

    // ワールド空間に逆変換
    float4 worldPos = mul(float4(ndc, depth / gMaxDistance, 1.0), gInvViewProj);
    worldPos.xyz /= worldPos.w;

    // グローバル均一フォグ
    float density = gGlobalDensity;
    float3 color = gFogColor;

    // 高度ベースの密度減衰 (地面付近で濃く、上空で薄くなる)
    float heightFalloff = exp(-max(worldPos.y, 0.0) * 0.1);
    density *= heightFalloff;

    // 出力: .rgb = 散乱色 * 密度, .a = 密度 * 吸収率
    uFogVolume[dtid] = float4(color * density, density * gAbsorption);
}
