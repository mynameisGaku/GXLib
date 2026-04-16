// @file Rainbow.hlsl
// @brief Custom shader model example — world-position-based rainbow color (ADR-0017 L2)
//
// This is a minimal Layer 2 custom shader model. Registered via
// ShaderRegistry::RegisterCustomShaderModel with id=100.
//
// Material.shaderModel = static_cast<ShaderModel>(100) に設定したマテリアルが
// このシェーダで描画される。3RT (HDR + Normal + Albedo) に書き込む GBuffer形式。

#include "Graphics/3D/PBRCommon.hlsli"    // cbuffer / vertex input layout 共通

struct VSOut
{
    float4 posCS    : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float3 normalWS : NORMAL;
};

VSOut VSMain(Vertex3D_PBR input)
{
    VSOut o;
#ifdef SKINNED
    // スキニング変換は PBR.hlsl と同じロジックを使う（省略版）
    float4 worldPos = mul(float4(input.position, 1.0f), WorldMatrix);
#else
    float4 worldPos = mul(float4(input.position, 1.0f), WorldMatrix);
#endif
    o.worldPos = worldPos.xyz;
    o.posCS    = mul(worldPos, ViewProjMatrix);
    o.normalWS = normalize(mul((float3x3)WorldMatrix, input.normal));
    return o;
}

struct PSOut
{
    float4 hdr    : SV_Target0;
    float4 normal : SV_Target1;
    float4 albedo : SV_Target2;
};

PSOut PSMain(VSOut input)
{
    // 虹色グラデーション: ワールド座標 Y で HSV → RGB
    float hue = frac(input.worldPos.y * 0.1f + input.worldPos.x * 0.05f);
    float3 rgb = saturate(abs(frac(hue + float3(0, 2.0f/3.0f, 1.0f/3.0f)) * 6.0f - 3.0f) - 1.0f);

    PSOut o;
    o.hdr    = float4(rgb * 2.0f, 1.0f);           // HDR - わずかにブライト
    o.normal = float4(normalize(input.normalWS) * 0.5f + 0.5f, 1.0f);
    o.albedo = float4(rgb, 1.0f);
    return o;
}
