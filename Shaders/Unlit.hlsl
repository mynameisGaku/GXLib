/// @file Unlit.hlsl
/// @brief Unlitシェーダー（ライティングなし、テクスチャ色のみ）
///
/// baseColor * albedoTexture をそのまま出力。照明・影の計算をスキップし、
/// フォグのみ適用する。UI的な3Dオブジェクトやエフェクト用途向け。

#include "ShaderModelCommon.hlsli"

// ============================================================================
// ピクセルシェーダー
// ============================================================================

PSOutput PSMain(PSInput input)
{
    // --- アルベド ---
    float4 albedo = SampleAlbedo(input.texcoord);

    // アルファカットオフ
    if (albedo.a < gAlphaCutoff)
        discard;

    // --- 法線 ---
    float3 N = ApplyNormalMap(input);

    // --- エミッシブ ---
    float3 emissive = SampleEmissive(input.texcoord);

    // Unlitではライティングなし: アルベド + エミッシブがそのまま最終色
    float3 finalColor = albedo.rgb + emissive;

    // --- フォグ ---
    finalColor = ApplyFog(finalColor, input.posW);

    // --- MRT出力 ---
    PSOutput output;
    output.color  = float4(finalColor, albedo.a);
    output.normal = EncodeNormal(N, 0.0f, 1.0f); // Unlit: metallic=0, roughness=1
    output.albedo = float4(albedo.rgb, 1.0);
    return output;
}
