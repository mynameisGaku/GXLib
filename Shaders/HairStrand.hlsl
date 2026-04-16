/// @file HairStrand.hlsl
/// @brief Marschner Hair Strand シェーダー
///
/// HairRenderer 専用のスタンドアロンシェーダー。
/// Line List で描画されるヘアストランドに対し、Marschner反射モデルの
/// R (正反射)、TT (透過)、TRT (内部反射) の3成分を計算する。
///
/// 参考: Marschner et al. "Light Scattering from Human Hair Fibers" (SIGGRAPH 2003)

// ============================================================================
// 定数バッファ
// ============================================================================

cbuffer HairCB : register(b0)
{
    float4x4 gViewProjection;       // ビュー×プロジェクション行列

    float4   gCameraPos;            // カメラ位置 (xyz, w=pad)
    float4   gLightDir;             // ライト方向 (xyz, w=pad)
    float4   gLightColor;           // ライトカラー (xyz, w=pad)

    float4   gBaseColorAndRoughR;   // ベースカラー (xyz), roughnessR (w)
    float4   gRoughness;            // roughnessTT (x), roughnessTRT (y), shift (z), ior (w)
    float4   gParams;               // absorption (x), specularMultiplier (y), scatterStrength (z), pad (w)
};

// ============================================================================
// 頂点構造体
// ============================================================================

struct VSInput
{
    float3 position    : POSITION;
    float3 tangent     : TANGENT;
    float  thickness   : TEXCOORD0;
    float  strandParam : TEXCOORD1;  // 0=root, 1=tip
};

struct PSInput
{
    float4 svPosition  : SV_POSITION;
    float3 worldPos    : POSITION0;
    float3 tangent     : TANGENT;
    float  thickness   : TEXCOORD0;
    float  strandParam : TEXCOORD1;
};

// ============================================================================
// 数学ヘルパー
// ============================================================================

static const float PI = 3.14159265359f;

/// @brief ガウシアン関数 — Marschnerモデルの各ローブの角度広がりを表現する
/// @param x 入力角度
/// @param sigma ガウシアン幅 (ラフネス)
float Gaussian(float x, float sigma)
{
    float s2 = sigma * sigma;
    if (s2 < 1e-8f)
        return 0.0f;
    return exp(-0.5f * x * x / s2) / (sigma * sqrt(2.0f * PI));
}

/// @brief フレネル反射率のSchlick近似
/// @param cosTheta 入射角のcos
/// @param eta 屈折率 (空気/毛髪)
float FresnelSchlick(float cosTheta, float eta)
{
    float f0 = (1.0f - eta) / (1.0f + eta);
    f0 = f0 * f0;
    float t = 1.0f - saturate(cosTheta);
    float t2 = t * t;
    return f0 + (1.0f - f0) * t2 * t2 * t;
}

// ============================================================================
// 頂点シェーダー
// ============================================================================

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.svPosition  = mul(float4(input.position, 1.0f), gViewProjection);
    output.worldPos    = input.position;
    output.tangent     = normalize(input.tangent);
    output.thickness   = input.thickness;
    output.strandParam = input.strandParam;
    return output;
}

// ============================================================================
// ピクセルシェーダー — Marschner R/TT/TRT
// ============================================================================

float4 PSMain(PSInput input) : SV_TARGET
{
    // ベースパラメータ取得
    float3 baseColor    = gBaseColorAndRoughR.xyz;
    float  roughnessR   = gBaseColorAndRoughR.w;
    float  roughnessTT  = gRoughness.x;
    float  roughnessTRT = gRoughness.y;
    float  shift        = gRoughness.z;
    float  ior          = gRoughness.w;
    float  absorption   = gParams.x;
    float  specMul      = gParams.y;
    float  scatterStr   = gParams.z;

    // ワールド空間ベクトル
    float3 T = normalize(input.tangent);
    float3 V = normalize(gCameraPos.xyz - input.worldPos);
    float3 L = normalize(gLightDir.xyz);
    float3 H = normalize(V + L);

    // Marschner座標系:
    // theta_h: ハーフベクトルとヘア面の角度
    // phi:     方位角差分
    float sinThetaH = dot(T, H);
    float cosThetaH = sqrt(max(0.0f, 1.0f - sinThetaH * sinThetaH));

    // 方位角差分の近似: ライトと視線の接線面投影間の角度
    float3 Lperp = L - dot(L, T) * T;
    float3 Vperp = V - dot(V, T) * T;
    float lenLperp = length(Lperp);
    float lenVperp = length(Vperp);
    float cosPhi = 0.0f;
    if (lenLperp > 1e-6f && lenVperp > 1e-6f)
    {
        cosPhi = dot(Lperp / lenLperp, Vperp / lenVperp);
    }
    float phi = acos(saturate(cosPhi));

    // フレネル
    float cosHalfAngle = saturate(cosThetaH);
    float fresnel = FresnelSchlick(cosHalfAngle, ior);

    // === R成分 (正反射) ===
    // cos^2(theta_h - shift)  *  Gaussian(phi, roughnessR)
    float thetaR = asin(saturate(sinThetaH)) - shift;
    float R_longitudinal = cos(thetaR) * cos(thetaR);
    float R_azimuthal = Gaussian(phi, roughnessR);
    float R = R_longitudinal * R_azimuthal * fresnel * specMul;

    // === TT成分 (透過) ===
    // cos^2(theta_h + shift) * Gaussian(phi - PI, roughnessTT) * (1-F) * absorption
    float thetaTT = asin(saturate(sinThetaH)) + shift;
    float TT_longitudinal = cos(thetaTT) * cos(thetaTT);
    float phiTT = phi - PI;
    float TT_azimuthal = Gaussian(phiTT, roughnessTT);
    float transmittance = exp(-absorption * 2.0f);  // 毛髪内部を2回通過
    float TT = TT_longitudinal * TT_azimuthal * (1.0f - fresnel) * transmittance * specMul;

    // === TRT成分 (内部反射) ===
    // cos^2(theta_h - 3*shift) * Gaussian(phi, roughnessTRT)
    float thetaTRT = asin(saturate(sinThetaH)) - 3.0f * shift;
    float TRT_longitudinal = cos(thetaTRT) * cos(thetaTRT);
    float TRT_azimuthal = Gaussian(phi, roughnessTRT);
    float internalTransmittance = exp(-absorption * 4.0f);  // 毛髪内部を4回通過
    float TRT = TRT_longitudinal * TRT_azimuthal * fresnel * internalTransmittance * specMul;

    // === 散乱 (diffuse-like term) ===
    float NdotL = saturate(dot(T, L) * 0.5f + 0.5f);  // ラッピングLambert
    float3 scatter = baseColor * NdotL * scatterStr;

    // === 合成 ===
    float3 specular = baseColor * (R + TT + TRT);
    float3 lightContrib = gLightColor.xyz;
    float3 finalColor = (specular + scatter) * lightContrib;

    // 根元から先端にかけてのアルファフェード
    float alpha = lerp(1.0f, 0.3f, input.strandParam);
    alpha *= input.thickness;

    return float4(finalColor, alpha);
}
