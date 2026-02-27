/// @file GPUParticleUpdate.hlsl
/// @brief GPUパーティクル物理更新Compute Shader
///
/// 全パーティクルスロットを走査し、life > 0（生存中）の粒子に対して:
///   1. 重力を速度に加算
///   2. 抗力で速度を減衰
///   3. 速度を位置に加算
///   4. 寿命を減衰
///   5. サイズとカラーを寿命進行率で補間
///
/// life <= 0 のスロットはスキップ（死亡パーティクル）。

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

/// 更新パラメータ定数バッファ（32バイト）
cbuffer UpdateCB : register(b0)
{
    float  gDeltaTime;
    float3 gGravity;
    float  gDrag;
    uint   gMaxParticles;
    float2 _pad;
};

RWStructuredBuffer<GPUParticle> gParticles : register(u0);
RWStructuredBuffer<uint>        gCounter   : register(u1);

[numthreads(256, 1, 1)]
void CSMain(uint3 dtid : SV_DispatchThreadID)
{
    uint idx = dtid.x;
    if (idx >= gMaxParticles) return;

    GPUParticle p = gParticles[idx];

    // 死亡パーティクルはスキップ
    if (p.life <= 0.0)
        return;

    // 物理更新
    // 1. 重力加速
    p.velocity += gGravity * gDeltaTime;

    // 2. 抗力（速度減衰）
    p.velocity *= (1.0 - gDrag);

    // 3. 位置更新
    p.position += p.velocity * gDeltaTime;

    // 4. 寿命減衰
    p.life -= gDeltaTime;

    // 5. 寿命進行率に基づくサイズ・カラー補間
    float t = 1.0 - saturate(p.life / max(p.maxLife, 0.001));  // 0=発生時, 1=消滅時
    p.curSize = lerp(p.startSize, p.endSize, t);
    p.color = lerp(p.startColor, p.endColor, t);

    // 6. 回転（緩やかに回す）
    p.rotation += 0.5 * gDeltaTime;

    gParticles[idx] = p;
}
