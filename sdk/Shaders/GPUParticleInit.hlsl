/// @file GPUParticleInit.hlsl
/// @brief GPUパーティクルプール初期化Compute Shader
///
/// 全パーティクルスロットのlifeを0に設定し、「死亡」状態にする。
/// GPUParticleSystem::Initialize()で1回だけディスパッチされる。
/// カウンターバッファにmaxParticlesを書き込み、Emit CSでのリングラップに使う。

/// パーティクルデータ構造体（C++のGPUParticle構造体と一致: 96バイト）
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

/// Init/Update共通CBレイアウト
cbuffer InitCB : register(b0)
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

    // 全スロットを死亡状態に初期化
    GPUParticle p;
    p.position   = float3(0, 0, 0);
    p.life       = 0.0;
    p.velocity   = float3(0, 0, 0);
    p.maxLife    = 0.0;
    p.curSize    = 0.0;
    p.startSize  = 0.0;
    p.endSize    = 0.0;
    p.rotation   = 0.0;
    p.color      = float4(0, 0, 0, 0);
    p.startColor = float4(0, 0, 0, 0);
    p.endColor   = float4(0, 0, 0, 0);
    gParticles[idx] = p;

    // スレッド0がカウンターにmaxParticlesを書き込む（Emit CSのリングラップ用）
    if (idx == 0)
    {
        gCounter[0] = gMaxParticles;
    }
}
