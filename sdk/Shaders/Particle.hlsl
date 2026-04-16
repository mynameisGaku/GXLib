/// @file Particle.hlsl
/// @brief パーティクル描画シェーダー（ビルボードクワッド）
///
/// SV_VertexIDでクワッド（2三角形=6頂点）を生成し、
/// StructuredBufferからパーティクルデータを読み取ってビルボード展開する。
/// カメラのright/upベクトルを使って常にカメラ正面を向く。
///
/// 頂点ID計算:
///   particleIndex = vertexID / 6
///   cornerIndex   = vertexID % 6 → 6頂点でクワッドを構成
///
///   コーナー配置:
///     0(-1,+1)  1(+1,+1)
///     2(-1,-1)  3(+1,-1)
///   三角形1: 0,1,2  三角形2: 1,3,2

/// パーティクルデータ（CPU側のParticleVertexと一致）
struct ParticleData
{
    float3 position;   // ワールド座標
    float  size;       // ワールド単位サイズ
    float4 color;      // RGBA
    float  rotation;   // Z軸回転（ラジアン）
    float3 _pad;       // 48Bアライン
};

StructuredBuffer<ParticleData> gParticles : register(t0);

cbuffer ParticleCB : register(b0)
{
    float4x4 gViewProj;
    float3   gCameraRight;
    float    _pad0;
    float3   gCameraUp;
    float    _pad1;
};

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
    float2(0.0, 0.0),  // 左上
    float2(1.0, 0.0),  // 右上
    float2(0.0, 1.0),  // 左下
    float2(1.0, 0.0),  // 右上
    float2(1.0, 1.0),  // 右下
    float2(0.0, 1.0),  // 左下
};

VSOutput VSMain(uint vertexID : SV_VertexID)
{
    VSOutput output;

    uint particleIdx = vertexID / 6;
    uint cornerIdx = vertexID % 6;

    ParticleData p = gParticles[particleIdx];
    float2 corner = kCornerOffsets[cornerIdx];

    // Z軸回転を適用
    float c = cos(p.rotation);
    float s = sin(p.rotation);
    float2 rotated;
    rotated.x = corner.x * c - corner.y * s;
    rotated.y = corner.x * s + corner.y * c;

    // ビルボード: カメラのright/upベクトルでクワッドを展開
    float3 worldPos = p.position
        + gCameraRight * (rotated.x * p.size)
        + gCameraUp    * (rotated.y * p.size);

    output.position = mul(float4(worldPos, 1.0), gViewProj);
    output.color = p.color;
    output.uv = kCornerUVs[cornerIdx];

    return output;
}

float4 PSMain(VSOutput input) : SV_TARGET
{
    // テクスチャの代わりにUVベースのソフト円で丸い粒子を描画する。
    // UVの中心(0.5,0.5)からの距離でアルファを減衰させる。
    float2 center = input.uv - 0.5;
    float dist2 = dot(center, center) * 4.0;  // 0=中心, 1=端
    float circle = saturate(1.0 - dist2);
    return float4(input.color.rgb, input.color.a * circle);
}
