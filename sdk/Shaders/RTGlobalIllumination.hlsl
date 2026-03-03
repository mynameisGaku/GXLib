/// @file RTGlobalIllumination.hlsl
/// @brief DXR グローバルイルミネーション シェーダー (lib_6_3)
///
/// 1spp コサイン重み半球サンプリング。半解像度ディスパッチ。
/// RTReflections.hlsl と同一のルートシグネチャ / リソースバインディング。

#include "LightingUtils.hlsli"

// ============================================================================
// 定数バッファ
// ============================================================================
cbuffer RTGIConstants : register(b0)
{
    float4x4 invViewProjection;  // 逆VP行列
    float4x4 viewMatrix;         // ビュー行列
    float4x4 invProjection;      // 逆プロジェクション行列
    float3   cameraPosition;     // カメラのワールド座標
    float    maxDistance;         // GIレイの最大到達距離
    float    screenWidth;        // フル解像度の幅
    float    screenHeight;       // フル解像度の高さ
    float    halfWidth;          // 半解像度の幅（ディスパッチサイズ）
    float    halfHeight;         // 半解像度の高さ
    float3   skyTopColor;        // プロシージャル空の天頂色
    float    giFrameIndex;       // フレーム番号（RNG用）
    float3   skyBottomColor;     // プロシージャル空の地平線色
    float    _pad1;
};

cbuffer InstanceData : register(b1)
{
    float4 g_InstanceAlbedoMetallic[512];
    float4 g_InstanceRoughnessGeom[512];
    float4 g_InstanceExtraData[512];
};

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

ByteAddressBuffer g_GeometryBuffers[32] : register(t0, space1);
Texture2D g_AlbedoTextures[32]          : register(t0, space2);

SamplerState g_LinearClamp : register(s0);
SamplerState g_PointClamp  : register(s1);

// ============================================================================
// ペイロード
// ============================================================================
struct GIPayload
{
    float4 color;
};

struct ShadowPayload
{
    float shadow;
};

// ============================================================================
// RNG (PCGハッシュ) — フレームごとに異なるランダムシードを生成
// ============================================================================

/// @brief RNGシード初期化 — ピクセル座標+フレーム番号からPCGハッシュで生成
uint InitRNG(uint2 pixel, uint frame)
{
    uint seed = pixel.x + pixel.y * 65536 + frame * 1234567;
    seed = seed * 747796405u + 2891336453u;
    seed = ((seed >> ((seed >> 28) + 4)) ^ seed) * 277803737u;
    return (seed >> 22) ^ seed;
}

/// @brief 次の乱数を生成 — PCGハッシュで [0,1] の浮動小数点を返す
float NextRandom(inout uint state)
{
    state = state * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28) + 4)) ^ state) * 277803737u;
    return float((word >> 22) ^ word) / 4294967295.0;
}

// ============================================================================
// コサイン重み半球サンプリング
// Lambert拡散に最適な確率密度で半球上の方向をサンプリングする。
// ============================================================================

/// @brief コサイン重み半球サンプリング — 法線N周りの半球上からランダム方向を生成
float3 CosineSampleHemisphere(float3 N, inout uint rng)
{
    float u1 = NextRandom(rng);
    float u2 = NextRandom(rng);
    float r = sqrt(u1);
    float theta = 2.0 * 3.14159265 * u2;
    float3 localDir = float3(r * cos(theta), r * sin(theta), sqrt(1.0 - u1));

    float3 up = abs(N.y) < 0.999 ? float3(0, 1, 0) : float3(1, 0, 0);
    float3 T = normalize(cross(up, N));
    float3 B = cross(N, T);
    return normalize(T * localDir.x + B * localDir.y + N * localDir.z);
}

// ============================================================================
// ヘルパー
// ============================================================================
float3 ReconstructWorldPosition(float2 uv, float depth)
{
    float4 clipPos = float4(uv * 2.0 - 1.0, depth, 1.0);
    clipPos.y = -clipPos.y;
    float4 worldPos = mul(clipPos, invViewProjection);
    return worldPos.xyz / worldPos.w;
}

float TraceShadowRay(float3 hitWorld, float3 Ng, LightData light)
{
    float3 L;
    float maxDist;

    if (light.type == LIGHT_DIRECTIONAL)
    {
        L = normalize(-light.direction);
        maxDist = maxDistance;
    }
    else
    {
        float3 toLight = light.position - hitWorld;
        float dist = length(toLight);
        L = toLight / dist;
        maxDist = dist;
    }

    ShadowPayload sp;
    sp.shadow = 0.0;

    RayDesc sr;
    sr.Origin    = hitWorld + Ng * 0.01;
    sr.Direction = L;
    sr.TMin      = 0.001;
    sr.TMax      = maxDist;

    TraceRay(g_TLAS,
             RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
             0xFF, 0, 1, 1, sr, sp);

    return sp.shadow;
}

// ============================================================================
// RayGeneration (半解像度)
// ============================================================================
[shader("raygeneration")]
void GIRayGen()
{
    uint2 idx = DispatchRaysIndex().xy;
    float2 uv = (float2(idx) + 0.5) / float2(halfWidth, halfHeight);

    float depth = g_Depth.SampleLevel(g_PointClamp, uv, 0);
    if (depth >= 0.999)
    {
        g_Output[idx] = float4(0, 0, 0, 0);
        return;
    }

    float4 normalSample = g_Normal.SampleLevel(g_LinearClamp, uv, 0);
    if (normalSample.a < 0.005)
    {
        g_Output[idx] = float4(0, 0, 0, 0);
        return;
    }

    float3 worldPos = ReconstructWorldPosition(uv, depth);
    float3 N = normalize(normalSample.rgb * 2.0 - 1.0);

    // コサイン半球サンプリング
    uint rng = InitRNG(idx, uint(giFrameIndex));
    float3 sampleDir = CosineSampleHemisphere(N, rng);

    RayDesc ray;
    ray.Origin    = worldPos + N * 0.02;
    ray.Direction = sampleDir;
    ray.TMin      = 0.001;
    ray.TMax      = maxDistance;

    GIPayload payload;
    payload.color = float4(0, 0, 0, 0);

    TraceRay(g_TLAS,
             RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
             0xFF, 0, 1, 0, ray, payload);

    g_Output[idx] = payload.color;
}

// ============================================================================
// ClosestHit (EvaluateLight + シャドウレイ)
// ============================================================================
[shader("closesthit")]
void GIClosestHit(inout GIPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    uint instIdx = InstanceIndex();

    float3 albedo    = g_InstanceAlbedoMetallic[instIdx].rgb;
    float  metallic  = g_InstanceAlbedoMetallic[instIdx].a;
    float  roughness = g_InstanceRoughnessGeom[instIdx].x;
    uint   geomIdx   = (uint)g_InstanceRoughnessGeom[instIdx].y;

    ByteAddressBuffer vb = g_GeometryBuffers[NonUniformResourceIndex(geomIdx * 2)];
    ByteAddressBuffer ib = g_GeometryBuffers[NonUniformResourceIndex(geomIdx * 2 + 1)];

    uint primIdx = PrimitiveIndex();
    uint i0 = ib.Load(primIdx * 12 + 0);
    uint i1 = ib.Load(primIdx * 12 + 4);
    uint i2 = ib.Load(primIdx * 12 + 8);

    uint stride = (uint)g_InstanceExtraData[instIdx].x;
    float3 p0 = asfloat(vb.Load3(i0 * stride));
    float3 p1 = asfloat(vb.Load3(i1 * stride));
    float3 p2 = asfloat(vb.Load3(i2 * stride));
    float3 n0 = asfloat(vb.Load3(i0 * stride + 12));
    float3 n1 = asfloat(vb.Load3(i1 * stride + 12));
    float3 n2 = asfloat(vb.Load3(i2 * stride + 12));

    float2 uv0 = asfloat(vb.Load2(i0 * stride + 24));
    float2 uv1 = asfloat(vb.Load2(i1 * stride + 24));
    float2 uv2 = asfloat(vb.Load2(i2 * stride + 24));

    float3 bary;
    bary.x = 1.0 - attribs.barycentrics.x - attribs.barycentrics.y;
    bary.y = attribs.barycentrics.x;
    bary.z = attribs.barycentrics.y;
    float3 normalObj = normalize(n0 * bary.x + n1 * bary.y + n2 * bary.z);
    float2 texCoord = uv0 * bary.x + uv1 * bary.y + uv2 * bary.z;

    float3 N = normalize(mul((float3x3)ObjectToWorld3x4(), normalObj));

    float3 faceNormalObj = normalize(cross(p1 - p0, p2 - p0));
    float3 Ng = normalize(mul((float3x3)ObjectToWorld3x4(), faceNormalObj));
    if (dot(Ng, -WorldRayDirection()) < 0.0)
        Ng = -Ng;

    roughness = max(roughness, 0.04);

    // テクスチャ
    float hasTexture = g_InstanceRoughnessGeom[instIdx].w;
    uint  texIdx     = (uint)g_InstanceRoughnessGeom[instIdx].z;
    if (hasTexture > 0.5)
    {
        float3 texColor = g_AlbedoTextures[NonUniformResourceIndex(texIdx)]
                            .SampleLevel(g_LinearClamp, texCoord, 0).rgb;
        albedo *= texColor;
    }

    float3 hitWorld = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();

    // ライティング (PBR)
    float3 V = normalize(-WorldRayDirection());
    float3 Lo = float3(0, 0, 0);

    for (uint li = 0; li < g_NumLights; ++li)
    {
        float3 contribution = EvaluateLight(g_Lights[li], hitWorld, N, V,
                                             albedo, metallic, roughness);
        // 1サンプルシャドウ (GIは低周波なのでソフトシャドウ不要)
        float shadow = TraceShadowRay(hitWorld, Ng, g_Lights[li]);
        Lo += contribution * shadow;
    }

    float3 ambient = g_AmbientColor * albedo;
    float3 color = ambient + Lo;

    payload.color = float4(color, 1.0);
}

// ============================================================================
// Miss (プロシージャル空)
// ============================================================================
[shader("miss")]
void GIMiss(inout GIPayload payload)
{
    float3 dir = normalize(WorldRayDirection());
    float t = saturate(dir.y * 0.5 + 0.5);
    float3 skyColor = lerp(skyBottomColor, skyTopColor, t);
    payload.color = float4(skyColor, 0.5);
}

// ============================================================================
// ShadowMiss
// ============================================================================
[shader("miss")]
void GIShadowMiss(inout ShadowPayload payload)
{
    payload.shadow = 1.0;
}
