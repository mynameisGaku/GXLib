/// @file SSR.hlsl
/// @brief Screen Space Reflections シェーダー
///
/// スクリーン空間DDAレイマーチング＋ビュー空間深度比較。
/// ピクセル単位で均一にステップし、バイナリ精緻化で精度向上。

#include "Fullscreen.hlsli"

cbuffer SSRCB : register(b0)
{
    float4x4 projection;
    float4x4 invProjection;
    float4x4 view;
    float maxDistance;
    float stepSize;
    int   maxSteps;
    float thickness;
    float intensity;
    float screenWidth;
    float screenHeight;
    float nearZ;
};

Texture2D<float4> gScene : register(t0);
Texture2D<float>  gDepth : register(t1);
SamplerState gLinearSampler : register(s0);
SamplerState gPointSampler  : register(s1);

float3 ReconstructViewPos(float2 uv, float depth)
{
    float2 ndc = uv * float2(2.0, -2.0) + float2(-1.0, 1.0);
    float4 vp = mul(float4(ndc, depth, 1.0), invProjection);
    return vp.xyz / vp.w;
}

// 深度バッファ値 → ビュー空間Z (安価版)
float ViewZFromDepth(float d)
{
    float4 vp = mul(float4(0, 0, d, 1.0), invProjection);
    return vp.z / vp.w;
}

// 法線復元 + エッジ信頼度 (±8pxカーネル, 中央/片側差分ブレンド)
float3 ReconstructViewNormal(float3 posC, float depthC, float2 uv, float2 ts,
                              out float edgeConfidence)
{
    float2 off = ts * 8.0;

    float dR = gDepth.SampleLevel(gPointSampler, uv + float2(off.x, 0), 0).r;
    float dL = gDepth.SampleLevel(gPointSampler, uv - float2(off.x, 0), 0).r;
    float dD = gDepth.SampleLevel(gPointSampler, uv + float2(0, off.y), 0).r;
    float dU = gDepth.SampleLevel(gPointSampler, uv - float2(0, off.y), 0).r;

    float3 pR = ReconstructViewPos(uv + float2(off.x, 0), dR);
    float3 pL = ReconstructViewPos(uv - float2(off.x, 0), dL);
    float3 pD = ReconstructViewPos(uv + float2(0, off.y), dD);
    float3 pU = ReconstructViewPos(uv - float2(0, off.y), dU);

    // ビュー空間Zでエッジ判定 (距離に依存しない)
    float viewEdgeThresh = 0.3; // ビュー空間単位
    float wR = saturate(1.0 - abs(pR.z - posC.z) / viewEdgeThresh);
    float wL = saturate(1.0 - abs(pL.z - posC.z) / viewEdgeThresh);
    float wD = saturate(1.0 - abs(pD.z - posC.z) / viewEdgeThresh);
    float wU = saturate(1.0 - abs(pU.z - posC.z) / viewEdgeThresh);

    float3 dxCentral = (pR - pL) * 0.5;
    float3 dxOneSide = (abs(dR - depthC) < abs(dL - depthC)) ? (pR - posC) : (posC - pL);
    float3 dx = lerp(dxOneSide, dxCentral, wR * wL);

    float3 dyCentral = (pD - pU) * 0.5;
    float3 dyOneSide = (abs(dD - depthC) < abs(dU - depthC)) ? (pD - posC) : (posC - pU);
    float3 dy = lerp(dyOneSide, dyCentral, wD * wU);

    float3 n = normalize(cross(dx, dy));
    if (dot(n, posC) > 0.0) n = -n;

    // エッジ信頼度: 全方向の最小ウェイト (±8px範囲でエッジがあれば低下)
    edgeConfidence = min(min(wR, wL), min(wD, wU));

    return n;
}

float ScreenEdgeFade(float2 uv)
{
    float2 f = smoothstep(0.0, 0.1, uv) * (1.0 - smoothstep(0.9, 1.0, uv));
    return f.x * f.y;
}

float4 PSSSR(FullscreenVSOutput input) : SV_Target
{
    float2 uv = input.uv;
    float4 sceneColor = gScene.SampleLevel(gLinearSampler, uv, 0);
    float depth = gDepth.SampleLevel(gPointSampler, uv, 0).r;

    if (depth >= 1.0)
        return sceneColor;

    float2 texelSize = float2(1.0 / screenWidth, 1.0 / screenHeight);

    float3 viewPos = ReconstructViewPos(uv, depth);

    // 法線復元 + エッジ信頼度 (同じ±8pxサンプルから算出)
    float edgeMask;
    float3 viewNormal = ReconstructViewNormal(viewPos, depth, uv, texelSize, edgeMask);

    if (edgeMask < 0.01)
        return sceneColor;

    float3 viewDir = normalize(viewPos);

    float NdotV = dot(-viewDir, viewNormal);
    if (NdotV < 0.05)
        return sceneColor;

    float3 reflDir = reflect(viewDir, viewNormal);
    float fresnel = 0.3 + 0.7 * pow(saturate(1.0 - NdotV), 5.0);

    // === Screen-space ray marching ===
    float3 rayOrigin = viewPos + viewNormal * 0.02;
    float3 rayEnd = rayOrigin + reflDir * maxDistance;

    // Clip to near plane
    if (rayEnd.z < nearZ)
    {
        float t = (rayOrigin.z - nearZ) / (rayOrigin.z - rayEnd.z);
        rayEnd = lerp(rayOrigin, rayEnd, max(t, 0.01));
    }

    // Project start/end to clip space, then to UV + depth
    float4 H0 = mul(float4(rayOrigin, 1.0), projection);
    float4 H1 = mul(float4(rayEnd, 1.0), projection);

    float2 uv0 = H0.xy / H0.w * float2(0.5, -0.5) + 0.5;
    float2 uv1 = H1.xy / H1.w * float2(0.5, -0.5) + 0.5;
    float  d0  = H0.z / H0.w;
    float  d1  = H1.z / H1.w;

    // Screen-space pixel distance
    float2 deltaUV = uv1 - uv0;
    float2 deltaPx = deltaUV * float2(screenWidth, screenHeight);
    float pixelDist = max(abs(deltaPx.x), abs(deltaPx.y));

    if (pixelDist < 1.0)
        return sceneColor;

    // Step count: 1 pixel per step, capped by maxSteps
    int totalSteps = min(int(pixelDist), maxSteps);
    float invPixelDist = 1.0 / pixelDist;

    bool hit = false;
    float2 hitUV = float2(0, 0);
    float prevK = 0;
    float hitK = 0;

    for (int i = 1; i <= totalSteps; i++)
    {
        float k = float(i) * invPixelDist;
        float2 sampleUV = uv0 + deltaUV * k;

        if (sampleUV.x < 0 || sampleUV.x > 1 || sampleUV.y < 0 || sampleUV.y > 1)
            break;

        float sceneD = gDepth.SampleLevel(gPointSampler, sampleUV, 0).r;
        if (sceneD >= 1.0)
        {
            prevK = k;
            continue;
        }

        // Perspective-correct ray depth at this screen position
        float rayD = d0 + (d1 - d0) * k;

        // View-space Z comparison (distance-independent thickness)
        float surfaceZ = ViewZFromDepth(sceneD);
        float rayZ = ViewZFromDepth(rayD);
        float depthDiff = rayZ - surfaceZ;

        if (depthDiff > 0 && depthDiff < thickness)
        {
            hit = true;
            hitUV = sampleUV;
            hitK = k;
            break;
        }

        prevK = k;
    }

    // Binary refinement in screen space
    if (hit)
    {
        float lo = prevK;
        float hi = hitK;
        for (int j = 0; j < 8; j++)
        {
            float mid = (lo + hi) * 0.5;
            float2 midUV = uv0 + deltaUV * mid;
            float midRayD = d0 + (d1 - d0) * mid;
            float midSceneD = gDepth.SampleLevel(gPointSampler, midUV, 0).r;

            float midRayZ = ViewZFromDepth(midRayD);
            float midSurfaceZ = ViewZFromDepth(midSceneD);

            if (midRayZ > midSurfaceZ)
            {
                hi = mid;
                hitUV = midUV;
            }
            else
            {
                lo = mid;
            }
        }
    }

    if (!hit)
        return sceneColor;

    float4 reflColor = gScene.SampleLevel(gLinearSampler, hitUV, 0);

    float edgeFade = ScreenEdgeFade(hitUV);
    float distFade = 1.0 - saturate(hitK);
    float reflStrength = fresnel * edgeFade * distFade * edgeMask * intensity;

    float3 result = sceneColor.rgb + reflColor.rgb * reflStrength;
    return float4(result, sceneColor.a);
}
