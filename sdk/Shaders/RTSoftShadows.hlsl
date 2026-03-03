/// @file RTSoftShadows.hlsl
/// @brief DXRソフトシャドウ -- 面積光源からの複数レイによるソフトシャドウ
/// 参考: "Ray Traced Shadows: Maintaining Real-Time Frame Rates" (NVIDIA)
///
/// 各ピクセルから面積光源の範囲内でジッターした方向にシャドウレイを発射し、
/// 遮蔽の割合からソフトシャドウを計算する。RayQuery (Inline Ray Tracing) を
/// 使用し、RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCHで高速化。
/// フレームカウンタをシードに使うことで、テンポラル蓄積と組み合わせて
/// 少ないレイ数でもノイズの少ないソフトシャドウを実現する。

RaytracingAccelerationStructure gTLAS : register(t0);
Texture2D<float>  tDepth  : register(t1);
Texture2D<float4> tNormal : register(t2);
RWTexture2D<float> uShadow : register(u0);

cbuffer RTSoftShadowCB : register(b0)
{
    float4x4 gInvViewProj;
    float3   gLightDir;
    float    gLightRadius;
    float3   gCameraPos;
    int      gNumRays;
    float2   gScreenSize;
    float    gMaxDistance;
    uint     gFrameCount;
};

/// @brief 2Dハッシュ関数 -- ジッターレイ方向の擬似乱数生成用
/// @param p 入力座標
/// @return [0, 1] の疑似乱数2D値
float2 Hash22(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * float3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.xx + p3.yz) * p3.zy);
}

/// @brief 深度バッファからワールド位置を復元する
/// @param uv スクリーンUV座標
/// @param depth 非線形深度値
/// @return ワールド空間の位置
float3 ReconstructWorldPos(float2 uv, float depth)
{
    float2 ndc = uv * 2.0 - 1.0;
    ndc.y = -ndc.y;
    float4 worldPos = mul(float4(ndc, depth, 1.0), gInvViewProj);
    return worldPos.xyz / worldPos.w;
}

/// @brief レイ生成シェーダー -- 面積光源シャドウレイのディスパッチ
[shader("raygeneration")]
void RayGen()
{
    uint2 launchIndex = DispatchRaysIndex().xy;
    float2 uv = (float2(launchIndex) + 0.5) / gScreenSize;

    float depth = tDepth[launchIndex];
    // 背景ピクセルはシャドウなし (完全に明るい)
    if (depth >= 1.0)
    {
        uShadow[launchIndex] = 1.0;
        return;
    }

    float3 worldPos = ReconstructWorldPos(uv, depth);
    float3 normal = tNormal[launchIndex].xyz * 2.0 - 1.0;

    float shadow = 0.0;

    for (int i = 0; i < gNumRays; ++i)
    {
        // 面積光源シミュレーション: ライト方向に円盤状のジッターを加える
        float2 xi = Hash22(float2(launchIndex) + float2(i, gFrameCount));
        float3 tangent = normalize(cross(gLightDir, float3(0, 1, 0.001)));
        float3 bitangent = cross(gLightDir, tangent);

        float angle = xi.x * 6.28318;
        float radius = sqrt(xi.y) * gLightRadius;
        float3 jitter = tangent * cos(angle) * radius + bitangent * sin(angle) * radius;
        float3 rayDir = normalize(-gLightDir + jitter);

        // レイ定義: 法線方向にバイアスしてセルフシャドウを防止
        RayDesc ray;
        ray.Origin = worldPos + normal * 0.01;
        ray.Direction = rayDir;
        ray.TMin = 0.001;
        ray.TMax = gMaxDistance;

        // Inline Ray Tracing: ACCEPT_FIRST_HIT で高速化 (any-hit 不要)
        RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER> query;
        query.TraceRayInline(gTLAS, RAY_FLAG_NONE, 0xFF, ray);
        query.Proceed();

        // 遮蔽なし = 1.0 (光あり), 遮蔽あり = 0.0 (影)
        shadow += (query.CommittedStatus() == COMMITTED_NOTHING) ? 1.0 : 0.0;
    }

    // 平均シャドウ値を出力
    uShadow[launchIndex] = shadow / float(gNumRays);
}

/// @brief ミスシェーダー -- レイが何にもヒットしなかった場合
[shader("miss")]
void Miss(inout float payload : SV_RayPayload)
{
    payload = 1.0;
}
