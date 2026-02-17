/// @file ToonOutline.hlsl
/// @brief トゥーンアウトラインシェーダー（背面押出し法）
///
/// 頂点を法線方向にスクリーン空間で押出し、フロントカリング（PSO側設定）で
/// 裏面のみ描画することでアウトラインを実現する。
/// PSMain_Outlineはフラットなアウトラインカラーを出力する。

#include "ShaderModelCommon.hlsli"

// ============================================================================
// アウトライン用頂点シェーダー
// ============================================================================

PSInput VSMain_Outline(VSInput input)
{
    PSInput output;

    float4 posL = float4(input.position, 1.0f);
    float3 normalL = input.normal;
    float3 tangentL = input.tangent.xyz;

#if defined(SKINNED)
    float4 skinnedPos = float4(0, 0, 0, 0);
    float3 skinnedNormal = float3(0, 0, 0);
    float3 skinnedTangent = float3(0, 0, 0);

    float4 w = input.weights;
    float wSum = w.x + w.y + w.z + w.w;
    if (wSum > 0.0001f) w /= wSum;
    else w = float4(1, 0, 0, 0);

    uint4 j = input.joints;
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        float4x4 bone = gBones[j[i]];
        skinnedPos += mul(posL, bone) * w[i];
        skinnedNormal += mul(float4(normalL, 0.0f), bone).xyz * w[i];
        skinnedTangent += mul(float4(tangentL, 0.0f), bone).xyz * w[i];
    }
    posL = skinnedPos;
    normalL = skinnedNormal;
    tangentL = skinnedTangent;
#endif

    // ワールド空間に変換
    float4 posW = mul(posL, world);
    float3x3 wit = (float3x3)worldInverseTranspose;
    float3 normalW = normalize(mul(normalL, wit));

    // クリップ空間に変換
    float4 posH = mul(posW, gViewProjection);

    // 法線をクリップ空間に変換してスクリーン空間方向を取得
    float4 normalH = mul(float4(normalW, 0.0f), gViewProjection);

    // スクリーン空間で法線方向に押出し
    // アスペクト比補正: projectionの[0][0]と[1][1]の比率を使用
    float2 screenNormal = normalize(normalH.xy);
    float aspectRatio = gProjection[1][1] / gProjection[0][0];
    screenNormal.x *= aspectRatio;
    screenNormal = normalize(screenNormal);

    // gOutlineWidth をスクリーン空間で適用（depth考慮）
    float outlineScale = gOutlineWidth * 0.01f;
    posH.xy += screenNormal * outlineScale * posH.w;

    output.posH = posH;
    output.posW = posW.xyz;
    output.normalW = normalW;
    output.tangentW = normalize(mul(tangentL, (float3x3)world));
    output.bitangentSign = input.tangent.w;
    output.texcoord = input.texcoord;

    // ビュー空間Z
    float4 posV = mul(posW, gView);
    output.viewZ = posV.z;

    return output;
}

// ============================================================================
// アウトライン用ピクセルシェーダー
// ============================================================================

PSOutput PSMain_Outline(PSInput input)
{
    PSOutput output;

    // アウトラインはフラットなカラーのみ出力
    output.color  = float4(gOutlineColor, 1.0f);
    // normal/albedoは最小限の値を出力（GIで使われないが形式は合わせる）
    output.normal = float4(0.5f, 0.5f, 1.0f, 0.5f); // デフォルト上向き法線
    output.albedo = float4(gOutlineColor, 1.0f);

    return output;
}
