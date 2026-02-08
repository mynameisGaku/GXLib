/// @file Skybox.hlsl
/// @brief プロシージャルスカイボックスシェーダー（グラデーション＋太陽）

cbuffer SkyboxConstants : register(b0)
{
    float4x4 gViewProjection;
    float3   gTopColor;
    float    padding1;
    float3   gBottomColor;
    float    padding2;
    float3   gSunDirection;
    float    gSunIntensity;
};

struct VSInput
{
    float3 position : POSITION;
};

struct PSInput
{
    float4 posH    : SV_Position;
    float3 localPos : TEXCOORD0;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.localPos = input.position;

    // gViewProjectionはCPU側で平行移動を除去済みの (ViewRot * Projection)
    float4 posH = mul(float4(input.position, 1.0f), gViewProjection);
    // z = w で常に最遠面に描画
    output.posH = posH.xyww;

    return output;
}

float4 PSMain(PSInput input) : SV_Target
{
    float3 dir = normalize(input.localPos);

    // 高さベースのグラデーション
    float t = dir.y * 0.5f + 0.5f;  // [-1,1] → [0,1]
    t = saturate(t);
    float3 skyColor = lerp(gBottomColor, gTopColor, t);

    // 太陽ディスク
    float3 sunDir = normalize(-gSunDirection);
    float sunDot = dot(dir, sunDir);

    // 太陽のコロナ（大きなぼかし）
    float corona = pow(saturate(sunDot), 64.0f) * 0.3f;
    // 太陽ディスク（鋭い円）
    float disk = pow(saturate(sunDot), 512.0f) * gSunIntensity;

    skyColor += float3(1.0f, 0.9f, 0.7f) * (corona + disk);

    // 地平線近くの霞み
    float horizon = 1.0f - abs(dir.y);
    horizon = pow(horizon, 4.0f) * 0.2f;
    skyColor += float3(0.8f, 0.8f, 0.9f) * horizon;

    // HDRリニア値をそのまま出力（トーンマッピングはポストエフェクトで実行）
    return float4(skyColor, 1.0f);
}
