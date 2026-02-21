/// @file GPUParticle.hlsl
/// @brief GPUパーティクル描画シェーダー（VS/PS）
///
/// SV_VertexIDでビルボードクワッド（2三角形=6頂点）を生成し、
/// StructuredBufferからパーティクルデータを読み取ってカメラ向きビルボードに展開する。
///
/// 死亡パーティクル（life <= 0）は全頂点を原点に配置して退化三角形にする。
/// GPUが自動的にカリングするため描画コストはほぼゼロ。
///
/// 既存のParticle.hlsl（CPUパーティクル用）と同じビルボードパターンを使用。

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

/// 描画用定数バッファ
cbuffer DrawCB : register(b0)
{
    float4x4 gViewProj;
    float3   gCameraRight;
    float    _pad0;
    float3   gCameraUp;
    float    _pad1;
};

/// パーティクルデータSRV（Compute Shaderで更新済み）
StructuredBuffer<GPUParticle> gParticles : register(t0);

struct VSOutput
{
    float4 position : SV_POSITION;
    float4 color    : COLOR;
    float2 uv       : TEXCOORD;
};

/// クワッドのコーナーオフセット（6頂点 = 2三角形）
static const float2 kCornerOffsets[6] =
{
    float2(-0.5,  0.5),  // 0: 左上
    float2( 0.5,  0.5),  // 1: 右上
    float2(-0.5, -0.5),  // 2: 左下
    float2( 0.5,  0.5),  // 3: 右上
    float2( 0.5, -0.5),  // 4: 右下
    float2(-0.5, -0.5),  // 5: 左下
};

/// UV座標（クワッドのテクスチャマッピング）
static const float2 kCornerUVs[6] =
{
    float2(0.0, 0.0),
    float2(1.0, 0.0),
    float2(0.0, 1.0),
    float2(1.0, 0.0),
    float2(1.0, 1.0),
    float2(0.0, 1.0),
};

VSOutput VSMain(uint vertexID : SV_VertexID)
{
    VSOutput output;

    uint particleIdx = vertexID / 6;
    uint cornerIdx = vertexID % 6;

    GPUParticle p = gParticles[particleIdx];

    // 死亡パーティクル: 退化三角形（全頂点を原点に配置 → GPUが自動カリング）
    if (p.life <= 0.0)
    {
        output.position = float4(0, 0, 0, 1);
        output.color = float4(0, 0, 0, 0);
        output.uv = float2(0, 0);
        return output;
    }

    float2 corner = kCornerOffsets[cornerIdx];

    // Z軸回転を適用
    float c = cos(p.rotation);
    float s = sin(p.rotation);
    float2 rotated;
    rotated.x = corner.x * c - corner.y * s;
    rotated.y = corner.x * s + corner.y * c;

    // ビルボード: カメラのright/upベクトルでクワッドを展開
    float3 worldPos = p.position
        + gCameraRight * (rotated.x * p.curSize)
        + gCameraUp    * (rotated.y * p.curSize);

    output.position = mul(float4(worldPos, 1.0), gViewProj);
    output.color = p.color;
    output.uv = kCornerUVs[cornerIdx];

    return output;
}

float4 PSMain(VSOutput input) : SV_TARGET
{
    // UVベースのソフト円で丸い粒子を描画
    float2 center = input.uv - 0.5;
    float dist2 = dot(center, center) * 4.0;  // 0=中心, 1=端
    float circle = saturate(1.0 - dist2);

    // 滑らかなフォールオフ
    circle = circle * circle;

    return float4(input.color.rgb, input.color.a * circle);
}
