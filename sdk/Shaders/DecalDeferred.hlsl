/// @file DecalDeferred.hlsl
/// @brief Deferred Decal — GBuffer法線投影 + メタリック/ラフネスブレンド
///
/// GBuffer上にデカールを投影し、アルベド・法線・メタリック/ラフネスを
/// 指定のブレンドモードで合成する。4種ブレンドモード対応。

Texture2D    tDecalAlbedo   : register(t0);
Texture2D    tDecalNormal   : register(t1);
Texture2D<float> tDepth     : register(t2);
SamplerState sLinearClamp   : register(s0);

cbuffer DecalDeferredCB : register(b0)
{
    float4x4 gInvViewProj;     // 逆ビュープロジェクション
    float4x4 gDecalWorld;      // デカールワールド行列
    float4x4 gDecalInvWorld;   // デカール逆ワールド行列
    float4   gDecalColor;      // デカールカラー
    float    gFadeDistance;     // エッジフェード距離
    float    gNormalThreshold;  // 法線しきい値
    float    gMetallic;        // メタリック値
    float    gRoughness;       // ラフネス値
    float2   gScreenSize;      // 画面サイズ
    uint     gBlendMode;       // 0=Alpha, 1=Additive, 2=Multiply, 3=Replace
    float    _pad;
};

struct PSInput
{
    float4 posH : SV_Position;
};

// MRT出力: Albedo + Normal + MetRough
struct PSOutput
{
    float4 albedo    : SV_Target0;  // RGB=アルベド, A=ブレンド係数
    float4 normal    : SV_Target1;  // XYZ=法線(0-1), W=0
    float4 metRough  : SV_Target2;  // R=メタリック, G=ラフネス, BA=0
};

PSOutput PSMain(PSInput input)
{
    PSOutput output;

    // Screen UV from pixel position
    float2 screenUV = input.posH.xy / gScreenSize;

    // Reconstruct world position from depth
    float depth = tDepth.Sample(sLinearClamp, screenUV);
    float2 ndc = screenUV * 2.0 - 1.0;
    ndc.y = -ndc.y;
    float4 worldPos = mul(float4(ndc, depth, 1.0), gInvViewProj);
    worldPos.xyz /= worldPos.w;

    // Transform to decal local space
    float4 localPos = mul(float4(worldPos.xyz, 1.0), gDecalInvWorld);

    // Clip to unit cube [-0.5, 0.5]
    if (any(abs(localPos.xyz) > 0.5))
        discard;

    // Decal UV
    float2 decalUV = localPos.xz + 0.5;

    // Edge fade
    float3 fadeFactors = 1.0 - saturate(abs(localPos.xyz) * 2.0 / (1.0 - gFadeDistance));
    float fade = fadeFactors.x * fadeFactors.y * fadeFactors.z;
    fade = saturate(fade);

    // Sample decal textures
    float4 albedo = tDecalAlbedo.Sample(sLinearClamp, decalUV) * gDecalColor;
    float3 normalMap = tDecalNormal.Sample(sLinearClamp, decalUV).rgb;

    // Apply blend mode to alpha
    float blendAlpha = albedo.a * fade;

    // Blend modes affect output
    // 0=Alpha: standard alpha blend
    // 1=Additive: add to existing
    // 2=Multiply: multiply with existing
    // 3=Replace: full replace within decal volume
    if (gBlendMode == 3) // Replace
        blendAlpha = fade;

    output.albedo = float4(albedo.rgb, blendAlpha);
    output.normal = float4(normalMap * 0.5 + 0.5, blendAlpha);
    output.metRough = float4(gMetallic, gRoughness, 0, blendAlpha);

    return output;
}
