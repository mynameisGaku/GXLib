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

Texture2D<float4> gScene  : register(t0);
Texture2D<float>  gDepth  : register(t1);
Texture2D<float4> gNormal : register(t2);
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

// (ReconstructViewNormal削除 — GBuffer法線を直接使用)

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

    // GBuffer法線 (LINEAR sampler でポリゴン境界スムージング)
    float4 normalSample = gNormal.SampleLevel(gLinearSampler, uv, 0);
    if (normalSample.a < 0.005)
        return sceneColor;
    float3 worldNormal = normalize(normalSample.rgb * 2.0 - 1.0);
    float3 viewNormal = normalize(mul(worldNormal, (float3x3)view));

    // 深度ベースエッジ検出 (シルエットフェードアウト用、法線再構築とは独立)
    float edgeMask = 1.0;
    {
        float2 off = texelSize * 4.0;
        float dR = gDepth.SampleLevel(gPointSampler, uv + float2(off.x, 0), 0);
        float dL = gDepth.SampleLevel(gPointSampler, uv - float2(off.x, 0), 0);
        float dD = gDepth.SampleLevel(gPointSampler, uv + float2(0, off.y), 0);
        float dU = gDepth.SampleLevel(gPointSampler, uv - float2(0, off.y), 0);
        float3 pR = ReconstructViewPos(uv + float2(off.x, 0), dR);
        float3 pL = ReconstructViewPos(uv - float2(off.x, 0), dL);
        float3 pD = ReconstructViewPos(uv + float2(0, off.y), dD);
        float3 pU = ReconstructViewPos(uv - float2(0, off.y), dU);
        float viewEdgeThresh = 0.3;
        float wR = saturate(1.0 - abs(pR.z - viewPos.z) / viewEdgeThresh);
        float wL = saturate(1.0 - abs(pL.z - viewPos.z) / viewEdgeThresh);
        float wD = saturate(1.0 - abs(pD.z - viewPos.z) / viewEdgeThresh);
        float wU = saturate(1.0 - abs(pU.z - viewPos.z) / viewEdgeThresh);
        edgeMask = min(min(wR, wL), min(wD, wU));
    }

    if (edgeMask < 0.01)
        return sceneColor;

    float3 viewDir = normalize(viewPos);

    float NdotV = dot(-viewDir, viewNormal);
    if (NdotV < 0.05)
        return sceneColor;

    float3 reflDir = reflect(viewDir, viewNormal);
    // Fresnel: F0=0.04 (非金属デフォルト) — SSR はマテリアル情報なしのため一律適用
    float fresnel = 0.04 + 0.96 * pow(saturate(1.0 - NdotV), 5.0);

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

    // 距離比例ブラー付き5タップクロスフィルタ (ポリゴンエッジ輝度ジャンプ平滑化)
    float4 reflColor;
    {
        float blurRadius = max(hitK * 3.0, 1.5);
        float2 blurStep = texelSize * blurRadius;

        float4 c  = gScene.SampleLevel(gLinearSampler, hitUV, 0);
        float4 r  = gScene.SampleLevel(gLinearSampler, hitUV + float2( blurStep.x, 0), 0);
        float4 l  = gScene.SampleLevel(gLinearSampler, hitUV + float2(-blurStep.x, 0), 0);
        float4 u  = gScene.SampleLevel(gLinearSampler, hitUV + float2(0,  blurStep.y), 0);
        float4 d  = gScene.SampleLevel(gLinearSampler, hitUV + float2(0, -blurStep.y), 0);

        reflColor = c * 0.4 + (r + l + u + d) * 0.15;
    }

    float edgeFade = ScreenEdgeFade(hitUV);
    float distFade = 1.0 - saturate(hitK);
    float reflStrength = fresnel * edgeFade * distFade * edgeMask * intensity;

    float3 result = sceneColor.rgb + reflColor.rgb * reflStrength;
    return float4(result, sceneColor.a);
}
