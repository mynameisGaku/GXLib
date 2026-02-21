/// @file ToonOutline.hlsl
/// @brief トゥーンアウトラインシェーダー（スムース法線ベース1パス押出し法）
///
/// 事前計算したスムース法線（同一位置の頂点の法線を平均化）を使って押出し。
/// slot 1に格納されたスムース法線で隙間なし＋正確なシルエットを1パスで実現。

#include "ShaderModelCommon.hlsli"

// ============================================================================
// アウトライン用頂点入力（スムース法線をslot 1から受け取る）
// ============================================================================

struct VSInput_Outline
{
    float3 position     : POSITION;
    float3 normal       : NORMAL;
    float2 texcoord     : TEXCOORD;
    float4 tangent      : TANGENT;
#if defined(SKINNED)
    uint4  joints       : JOINTS;
    float4 weights      : WEIGHTS;
#endif
    float3 smoothNormal : SMOOTHNORMAL;  // slot 1
};

// ============================================================================
// アウトライン用頂点シェーダー
// ============================================================================

/// @brief アウトラインVS — スムース法線方向に頂点を押出してシルエットを生成
/// カメラ距離に応じた幅減衰と、凸面/凹面でのハイブリッド押出し方向選択を行う。
PSInput VSMain_Outline(VSInput_Outline input)
{
    PSInput output;

    float4 posL = float4(input.position, 1.0f);
    float3 normalL = input.normal;
    float3 tangentL = input.tangent.xyz;
    float3 smoothNormalL = input.smoothNormal;

#if defined(SKINNED)
    float4 skinnedPos = float4(0, 0, 0, 0);
    float3 skinnedNormal = float3(0, 0, 0);
    float3 skinnedTangent = float3(0, 0, 0);
    float3 skinnedSmoothNormal = float3(0, 0, 0);

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
        skinnedSmoothNormal += mul(float4(smoothNormalL, 0.0f), bone).xyz * w[i];
    }
    posL = skinnedPos;
    normalL = skinnedNormal;
    tangentL = skinnedTangent;
    smoothNormalL = skinnedSmoothNormal;
#endif

    // ワールド空間に変換
    float4 posW = mul(posL, world);

    float3x3 wit = (float3x3)worldInverseTranspose;
    float3 normalW = normalize(mul(normalL, wit));

    // --- ハイブリッド押出し方向選択 (UltimateOutline方式) ---
    // scaleDir: オブジェクト中心→頂点の放射方向（凸コーナーで均一スケーリング）
    // smoothNormal: 事前計算した平均法線（凹面・複雑形状で安全）
    float3 scaleDirL = posL.xyz;
    float scaleDirLen = length(scaleDirL);
    float3 scaleDir = (scaleDirLen > 1e-6f) ? (scaleDirL / scaleDirLen) : normalize(smoothNormalL);

    // 角度 < 89° → scaleDir使用（凸領域、均一スケーリング）
    // 角度 >= 89° → smoothNormal使用（凹領域、法線ベース安全策）
    float angleCos = dot(scaleDir, normalize(smoothNormalL));
    float3 extrudeDirL = (angleCos > cos(radians(89.0f))) ? scaleDir : normalize(smoothNormalL);

    // ワールド空間に変換
    float3 extrudeDirW = normalize(mul(extrudeDirL, (float3x3)world));

    // 元位置のクリップ空間座標（距離スケーリング用）
    float4 posH_ref = mul(posW, gViewProjection);

    // ワールド空間で押出し（posH.wで画面一定幅維持）
    float outlineScale = gOutlineWidth * 0.01f;

    // UTS2: 距離ベースアウトライン幅減衰
    float cameraDist = length(posW.xyz - gCameraPosition);
    float distAtten = smoothstep(gOutlineFarDist, gOutlineNearDist, cameraDist);
    outlineScale *= distAtten;

    posW.xyz += extrudeDirW * outlineScale * posH_ref.w;

    // 押出し後のクリップ空間座標
    float4 posH = mul(posW, gViewProjection);

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

/// @brief アウトラインPS — 指定色にベース色を混合し、影領域で暗くする
PSOutput PSMain_Outline(PSInput input)
{
    PSOutput output;

    // --- シャドウ ---
    float3 N = normalize(input.normalW);
    ShadowInfo shadowInfo = ComputeShadowAll(input.posW, N, input.viewZ);
    float shadow = GetLightShadow(0, shadowInfo.cascadeShadow, input.posW, N);

    // UTS2: 基本色ブレンド（baseColor^2で暗くして混合）
    float4 baseAlbedo = SampleAlbedo(input.texcoord);
    float3 blendedColor = gOutlineColor * lerp(float3(1, 1, 1), baseAlbedo.rgb * baseAlbedo.rgb, gOutlineBlendBaseColor);

    // 影領域でアウトラインを暗くする（0.5〜1.0の範囲でソフト変調）
    float3 outlineColor = blendedColor * lerp(0.5f, 1.0f, shadow);

    output.color  = float4(outlineColor, 1.0f);
    output.normal = float4(0.5f, 0.5f, 1.0f, 0.5f);
    output.albedo = float4(outlineColor, 1.0f);

    return output;
}
