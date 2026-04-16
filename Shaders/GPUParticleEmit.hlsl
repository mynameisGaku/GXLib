/// @file GPUParticleEmit.hlsl
/// @brief GPUパーティクル放出Compute Shader
///
/// リングバッファ方式でパーティクルプールにN個の新規パーティクルを書き込む。
/// 各パーティクルにはWang hashベースの擬似乱数で速度・寿命・カラーを割り当てる。
///
/// 書き込みインデックス: (gEmitOffset + SV_DispatchThreadID.x) % gCounter[0]
/// gCounter[0]にはInit CSでmaxParticlesが格納されている。

/// パーティクルデータ構造体
struct GPUParticle
{
    float3 position;
    float  life;
    float3 velocity;
    float  maxLife;
    float  curSize;
    float  startSize;
    float  endSize;
    float  rotation;
    float4 color;
    float4 startColor;
    float4 endColor;
};

/// 放出パラメータ定数バッファ（128バイト）
cbuffer EmitCB : register(b0)
{
    uint   gEmitCount;
    float3 gEmitPosition;
    float3 gVelocityMin;
    float  _pad0;
    float3 gVelocityMax;
    float  _pad1;
    float  gLifeMin;
    float  gLifeMax;
    float  gSizeStart;
    float  gSizeEnd;
    float4 gColorStart;
    float4 gColorEnd;
    uint   gRandomSeed;
    uint   gEmitOffset;
    float2 _pad2;
};

RWStructuredBuffer<GPUParticle> gParticles : register(u0);
RWStructuredBuffer<uint>        gCounter   : register(u1);

// ============================================================================
// Wang hash — 高品質32bit整数ハッシュ
// ============================================================================
uint WangHash(uint seed)
{
    seed = (seed ^ 61u) ^ (seed >> 16u);
    seed *= 9u;
    seed = seed ^ (seed >> 4u);
    seed *= 0x27D4EB2Du;
    seed = seed ^ (seed >> 15u);
    return seed;
}

/// [0, 1]の浮動小数点乱数
float RandomFloat(inout uint seed)
{
    seed = WangHash(seed);
    return float(seed) / 4294967295.0;
}

/// [minVal, maxVal] 範囲の乱数
float RandomRange(inout uint seed, float minVal, float maxVal)
{
    return lerp(minVal, maxVal, RandomFloat(seed));
}

/// float3の各成分を独立にランダム化
float3 RandomRange3(inout uint seed, float3 minVal, float3 maxVal)
{
    float3 result;
    result.x = RandomRange(seed, minVal.x, maxVal.x);
    result.y = RandomRange(seed, minVal.y, maxVal.y);
    result.z = RandomRange(seed, minVal.z, maxVal.z);
    return result;
}

[numthreads(256, 1, 1)]
void CSMain(uint3 dtid : SV_DispatchThreadID)
{
    uint localIdx = dtid.x;
    if (localIdx >= gEmitCount) return;

    // gCounter[0] = maxParticles（Init CSで設定済み）
    uint maxParticles = gCounter[0];

    // リングバッファ書き込み位置
    uint writeIdx = (gEmitOffset + localIdx) % maxParticles;

    // ランダムシード: フレームカウンター + スレッドID でユニーク化
    uint seed = WangHash(gRandomSeed * 1973u + localIdx * 6529u + 1u);

    GPUParticle p;
    p.position   = gEmitPosition;
    p.life       = RandomRange(seed, gLifeMin, gLifeMax);
    p.velocity   = RandomRange3(seed, gVelocityMin, gVelocityMax);
    p.maxLife    = p.life;
    p.curSize    = gSizeStart;
    p.startSize  = gSizeStart;
    p.endSize    = gSizeEnd;
    p.rotation   = RandomRange(seed, 0.0, 6.28318530718);
    p.color      = gColorStart;
    p.startColor = gColorStart;
    p.endColor   = gColorEnd;

    gParticles[writeIdx] = p;
}
