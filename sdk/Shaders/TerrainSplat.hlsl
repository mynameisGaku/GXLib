/// @file TerrainSplat.hlsl
/// @brief 4レイヤー地形テクスチャスプラッティング

// 入力
cbuffer TerrainCB : register(b0)
{
    float4x4 gWorldViewProj;
    float4x4 gWorld;
    float4 gUVScale; // レイヤーごとのUVスケール乗数
};

Texture2D gSplatMap : register(t0);
Texture2D gLayer0   : register(t1);
Texture2D gLayer1   : register(t2);
Texture2D gLayer2   : register(t3);
Texture2D gLayer3   : register(t4);

SamplerState gSampler : register(s0);

struct VSInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 texcoord : TEXCOORD0;
    float4 tangent  : TANGENT;
};

struct PSInput
{
    float4 position  : SV_POSITION;
    float3 worldPos  : TEXCOORD0;
    float3 normal    : TEXCOORD1;
    float2 texcoord  : TEXCOORD2;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.position = mul(float4(input.position, 1.0), gWorldViewProj);
    output.worldPos = mul(float4(input.position, 1.0), gWorld).xyz;
    output.normal = normalize(mul(float4(input.normal, 0.0), gWorld).xyz);
    output.texcoord = input.texcoord;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    // RGBAチャンネルからスプラットウェイトをサンプリング
    float4 splat = gSplatMap.Sample(gSampler, input.texcoord);

    // レイヤーごとのUVスケーリングで各レイヤーテクスチャをサンプリング
    float4 c0 = gLayer0.Sample(gSampler, input.texcoord * gUVScale.x);
    float4 c1 = gLayer1.Sample(gSampler, input.texcoord * gUVScale.y);
    float4 c2 = gLayer2.Sample(gSampler, input.texcoord * gUVScale.z);
    float4 c3 = gLayer3.Sample(gSampler, input.texcoord * gUVScale.w);

    // スプラットウェイトでブレンド（R=layer0, G=layer1, B=layer2, A=layer3）
    float4 albedo = c0 * splat.r + c1 * splat.g + c2 * splat.b + c3 * splat.a;

    // 簡易ディレクショナルライティング
    float3 lightDir = normalize(float3(0.5, 1.0, 0.3));
    float ndl = saturate(dot(input.normal, lightDir));
    float3 lighting = float3(0.15, 0.15, 0.15) + ndl * float3(1.0, 0.95, 0.9);

    return float4(albedo.rgb * lighting, 1.0);
}
