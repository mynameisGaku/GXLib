/// @file RTReflections.hlsl
/// @brief DXR レイトレーシング反射シェーダー (lib_6_3)
///
/// RayGen: フル解像度座標からワールド空間反射レイを発射
/// ClosestHit: VB/IBアクセス + バリセントリック法線補間 + EvaluateLight (PBR.hlslと同一) + シャドウレイ
/// Miss: プロシージャル空 (天空グラデーション)
/// ShadowMiss: シャドウレイ用 (遮蔽なし = 照明あり)

#include "LightingUtils.hlsli"

// ============================================================================
// 定数バッファ
// ============================================================================
cbuffer RTReflectionConstants : register(b0)
{
    float4x4 invViewProjection;     // 0
    float4x4 view;                  // 64
    float4x4 invProjection;         // 128
    float3   cameraPosition;        // 192
    float    maxDistance;            // 204
    float    screenWidth;           // 208
    float    screenHeight;          // 212
    float    debugMode;             // 216
    float    intensity;             // 220
    float3   skyTopColor;           // 224
    float    _pad0;                 // 236
    float3   skyBottomColor;        // 240
    float    _pad1;                 // 252
};

// per-instance PBRデータ (InstanceIndex() でインデックス)
cbuffer InstanceData : register(b1)
{
    float4 g_InstanceAlbedoMetallic[512];  // .rgb=albedo, .a=metallic
    float4 g_InstanceRoughnessGeom[512];   // .x=roughness, .y=geometryIndex, .z=texIdx, .w=hasTexture
    float4 g_InstanceExtraData[512];       // .x=vertexStride, .yzw=reserved
};

// PBR.hlsl b2と同一構造体
cbuffer LightConstants : register(b2)
{
    LightData g_Lights[16];
    float3    g_AmbientColor;
    uint      g_NumLights;
};

// ============================================================================
// リソースバインディング
// ============================================================================
RaytracingAccelerationStructure g_TLAS : register(t0);
Texture2D<float4> g_SceneHDR           : register(t1);
Texture2D<float>  g_Depth              : register(t2);
Texture2D<float4> g_Normal             : register(t3);
RWTexture2D<float4> g_Output           : register(u0);

// ジオメトリバッファ (VB/IB ByteAddressBuffers in space1)
ByteAddressBuffer g_GeometryBuffers[32] : register(t0, space1);

// アルベドテクスチャ配列 (space2)
Texture2D g_AlbedoTextures[32] : register(t0, space2);

SamplerState g_LinearClamp : register(s0);
SamplerState g_PointClamp  : register(s1);

// ============================================================================
// レイペイロード
// ============================================================================
struct ReflectionPayload
{
    float4 color;  // rgb = 反射色, a = ヒットフラグ
};

/// シャドウレイ用ペイロード (MissShaderIndex=1)
struct ShadowPayload
{
    float shadow;  // 0 = 影の中, 1 = 照明あり
};

// ============================================================================
// ヘルパー関数
// ============================================================================

/// 深度値からワールド座標を復元
float3 ReconstructWorldPosition(float2 uv, float depth)
{
    float4 clipPos = float4(uv * 2.0 - 1.0, depth, 1.0);
    clipPos.y = -clipPos.y; // DX UV→NDC 変換
    float4 worldPos = mul(clipPos, invViewProjection);
    return worldPos.xyz / worldPos.w;
}

/// ライトタイプ別シャドウレイ (8サンプルソフトシャドウ)
float TraceShadowRay(float3 hitWorld, float3 Ng, LightData light)
{
    float3 L;
    float maxDist;

    if (light.type == LIGHT_DIRECTIONAL)
    {
        L = normalize(-light.direction);
        maxDist = maxDistance;
    }
    else // Point / Spot
    {
        float3 toLight = light.position - hitWorld;
        float dist = length(toLight);
        L = toLight / dist;
        maxDist = dist;
    }

    float3 tangent = normalize(cross(L, abs(L.y) < 0.99 ? float3(0,1,0) : float3(1,0,0)));
    float3 bitangent = cross(L, tangent);
    float softRadius = 0.12;

    static const float2 offsets[8] = {
        float2( 1.0,  0.0), float2(-1.0,  0.0),
        float2( 0.0,  1.0), float2( 0.0, -1.0),
        float2( 0.7,  0.7), float2(-0.7,  0.7),
        float2( 0.7, -0.7), float2(-0.7, -0.7)
    };

    float shadow = 0.0;
    float3 shadowOrigin = hitWorld + Ng * 0.01;

    [unroll]
    for (int si = 0; si < 8; si++)
    {
        ShadowPayload sp;
        sp.shadow = 0.0;

        float3 dir = normalize(L + (tangent * offsets[si].x + bitangent * offsets[si].y) * softRadius);

        RayDesc sr;
        sr.Origin    = shadowOrigin;
        sr.Direction = dir;
        sr.TMin      = 0.001;
        sr.TMax      = maxDist;

        TraceRay(g_TLAS,
                 RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
                 0xFF, 0, 1,
                 1, sr, sp);

        shadow += sp.shadow;
    }
    return shadow * 0.125; // 1/8
}

// ============================================================================
// RayGeneration シェーダー
// ============================================================================
[shader("raygeneration")]
void RayGen()
{
    uint2 dispatchIdx = DispatchRaysIndex().xy;
    uint2 dispatchDim = DispatchRaysDimensions().xy;

    // フル解像度UV
    float2 uv = (float2(dispatchIdx) + 0.5) / float2(dispatchDim);

    // 深度サンプル
    float depth = g_Depth.SampleLevel(g_PointClamp, uv, 0);

    // スカイボックス（深度>=0.999）はスキップ
    if (depth >= 0.999)
    {
        g_Output[dispatchIdx] = float4(0, 0, 0, 0);
        return;
    }

    // ワールド位置復元
    float3 worldPos = ReconstructWorldPosition(uv, depth);

    // GBuffer法線をサンプル ([0,1]->[-1,1]デコード)
    float4 normalSample = g_Normal.SampleLevel(g_LinearClamp, uv, 0);

    // alpha<0.005 は非PBR描画 -> スキップ (PBRピクセルは最低0.01)
    if (normalSample.a < 0.005)
    {
        g_Output[dispatchIdx] = float4(0, 0, 0, 0);
        return;
    }

    // マテリアルデコード: alpha=[0.01,0.99]非金属, [1.01,1.99]金属
    float matMetallic  = step(1.0, normalSample.a);
    float matRoughness = normalSample.a - matMetallic;

    // 粗い非金属面は反射レイ不要（性能向上）
    if (matRoughness > 0.8 && matMetallic < 0.5)
    {
        g_Output[dispatchIdx] = float4(0, 0, 0, 0);
        return;
    }

    float3 normal = normalize(normalSample.rgb * 2.0 - 1.0);
    // View方向をUVから直接計算（depth再構築の三角形境界アーティファクト回避）
    float4 farClip = float4(uv * 2.0 - 1.0, 1.0, 1.0);
    farClip.y = -farClip.y;
    float4 farWorld = mul(farClip, invViewProjection);
    float3 viewDir = normalize(farWorld.xyz / farWorld.w - cameraPosition);
    float3 reflectDir = reflect(viewDir, normal);

    // デバッグモード 1: ワールド座標を色で表示 (X=赤, Z=青)
    if (debugMode >= 0.5 && debugMode < 1.5)
    {
        float r = saturate(worldPos.x * 0.05 + 0.5);
        float b = saturate(worldPos.z * 0.05 + 0.5);
        g_Output[dispatchIdx] = float4(r, 0.2, b, 1.0);
        return;
    }

    // デバッグモード 2: 反射方向を色で表示
    if (debugMode >= 1.5 && debugMode < 2.5)
    {
        g_Output[dispatchIdx] = float4(reflectDir * 0.5 + 0.5, 1.0);
        return;
    }

    // レイ発射
    RayDesc ray;
    ray.Origin    = worldPos + normal * 0.01;
    ray.Direction = reflectDir;
    ray.TMin      = 0.001;
    ray.TMax      = maxDistance;

    ReflectionPayload payload;
    payload.color = float4(0, 0, 0, 0);

    TraceRay(g_TLAS,
             RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
             0xFF,
             0,
             1,
             0,
             ray,
             payload);

    // 反射色を出力 (強度はcompositeのintensityで制御)
    g_Output[dispatchIdx] = payload.color;
}

// ============================================================================
// ClosestHit シェーダー (EvaluateLight + シャドウレイ — PBR.hlslと同一ライティング)
// ============================================================================
[shader("closesthit")]
void ClosestHit(inout ReflectionPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    uint instIdx = InstanceIndex();

    // マテリアル取得
    float3 albedo    = g_InstanceAlbedoMetallic[instIdx].rgb;
    float  metallic  = g_InstanceAlbedoMetallic[instIdx].a;
    float  roughness = g_InstanceRoughnessGeom[instIdx].x;
    uint   geomIdx   = (uint)g_InstanceRoughnessGeom[instIdx].y;

    // VB/IB取得 (geomIdx*2=VB, geomIdx*2+1=IB)
    ByteAddressBuffer vb = g_GeometryBuffers[NonUniformResourceIndex(geomIdx * 2)];
    ByteAddressBuffer ib = g_GeometryBuffers[NonUniformResourceIndex(geomIdx * 2 + 1)];

    // 三角形インデックス読み込み (R32_UINT)
    uint primIdx = PrimitiveIndex();
    uint i0 = ib.Load(primIdx * 12 + 0);
    uint i1 = ib.Load(primIdx * 12 + 4);
    uint i2 = ib.Load(primIdx * 12 + 8);

    // 頂点読み込み (stride はインスタンスデータから動的取得)
    uint stride = (uint)g_InstanceExtraData[instIdx].x;
    float3 p0 = asfloat(vb.Load3(i0 * stride));
    float3 p1 = asfloat(vb.Load3(i1 * stride));
    float3 p2 = asfloat(vb.Load3(i2 * stride));
    float3 n0 = asfloat(vb.Load3(i0 * stride + 12));
    float3 n1 = asfloat(vb.Load3(i1 * stride + 12));
    float3 n2 = asfloat(vb.Load3(i2 * stride + 12));

    // UV読み取り (offset 24 = position(12) + normal(12))
    float2 uv0 = asfloat(vb.Load2(i0 * stride + 24));
    float2 uv1 = asfloat(vb.Load2(i1 * stride + 24));
    float2 uv2 = asfloat(vb.Load2(i2 * stride + 24));

    // バリセントリック補間
    float3 bary;
    bary.x = 1.0 - attribs.barycentrics.x - attribs.barycentrics.y;
    bary.y = attribs.barycentrics.x;
    bary.z = attribs.barycentrics.y;
    float3 normalObj = normalize(n0 * bary.x + n1 * bary.y + n2 * bary.z);
    float2 texCoord = uv0 * bary.x + uv1 * bary.y + uv2 * bary.z;

    // オブジェクト空間 -> ワールド空間 法線変換
    float3 N = normalize(mul((float3x3)ObjectToWorld3x4(), normalObj));

    // ジオメトリック法線 (face normal) — シャドウレイオフセット用
    float3 faceNormalObj = normalize(cross(p1 - p0, p2 - p0));
    float3 Ng = normalize(mul((float3x3)ObjectToWorld3x4(), faceNormalObj));
    if (dot(Ng, -WorldRayDirection()) < 0.0)
        Ng = -Ng;

    // スペキュラアンチエイリアシング: 補間法線とface normalの乖離度でroughness適応増加
    float cosNNg = saturate(dot(N, Ng));
    float sinNNg = sqrt(max(1.0 - cosNNg * cosNNg, 0.0));
    roughness = max(roughness, sinNNg * 2.0);
    roughness = max(roughness, 0.04);

    // テクスチャサンプリング
    float hasTexture = g_InstanceRoughnessGeom[instIdx].w;
    uint  texIdx     = (uint)g_InstanceRoughnessGeom[instIdx].z;
    if (hasTexture > 0.5)
    {
        float3 texColor = g_AlbedoTextures[NonUniformResourceIndex(texIdx)]
                            .SampleLevel(g_LinearClamp, texCoord, 0).rgb;
        albedo *= texColor;
    }

    // ヒット位置・方向
    float3 hitWorld = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();

    // デバッグモード 3: ClosestHit 診断 (R=N.y, G=1-hitWorld.y*5, B=hasTexture)
    if (debugMode >= 2.5 && debugMode < 3.5)
    {
        payload.color = float4(N.y, saturate(1.0 - hitWorld.y * 5.0), hasTexture, 1.0);
        return;
    }

    // --- PBR.hlslと同一のライティング (EvaluateLight + シャドウレイ) ---
    float3 V = normalize(-WorldRayDirection());
    float3 Lo = float3(0, 0, 0);

    for (uint li = 0; li < g_NumLights; ++li)
    {
        float3 contribution = EvaluateLight(g_Lights[li], hitWorld, N, V,
                                             albedo, metallic, roughness);
        float shadow = TraceShadowRay(hitWorld, Ng, g_Lights[li]);
        Lo += contribution * shadow;
    }

    float3 ambient = g_AmbientColor * albedo;
    float3 color = ambient + Lo;

    // HDR値をそのまま出力 (クランプなし — トーンマッパーに任せる)
    payload.color = float4(color, 1.0);
}

// ============================================================================
// Miss シェーダー (プロシージャル空) — MissShaderIndex=0
// ============================================================================
[shader("miss")]
void Miss(inout ReflectionPayload payload)
{
    // プロシージャル空: 方向に基づくグラデーション
    float3 dir = normalize(WorldRayDirection());
    float t = saturate(dir.y * 0.5 + 0.5);
    float3 skyColor = lerp(skyBottomColor, skyTopColor, t);

    // alpha=0.5: 空反射
    payload.color = float4(skyColor, 0.5);
}

// ============================================================================
// ShadowMiss シェーダー — MissShaderIndex=1
// シャドウレイがジオメトリに当たらなかった = 照明あり
// ============================================================================
[shader("miss")]
void ShadowMiss(inout ShadowPayload payload)
{
    payload.shadow = 1.0;
}
