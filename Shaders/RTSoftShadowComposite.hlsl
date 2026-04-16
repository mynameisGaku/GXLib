/// @file RTSoftShadowComposite.hlsl
/// @brief RTソフトシャドウ テンポラル蓄積+合成
///
/// 現フレームのシャドウ結果と前フレームの履歴を線形補間して
/// テンポラル蓄積を行う。ブレンド率が高いほど履歴の重みが大きく、
/// ノイズが低減されるが動きへの追従が遅くなる。

Texture2D<float> tCurrentShadow  : register(t0);
Texture2D<float> tHistoryShadow  : register(t1);
SamplerState     sPoint          : register(s0);

cbuffer CompositeCB : register(b0)
{
    float gTemporalBlend;
    float3 _pad;
};

struct PSInput
{
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD;
};

/// @brief テンポラル蓄積ピクセルシェーダー
///
/// current と history を gTemporalBlend で線形補間する。
/// result = lerp(current, history, blend)
/// blend=0 で現フレームのみ、blend=1 で履歴のみ (通常 0.8-0.95)
float4 PSMain(PSInput input) : SV_Target0
{
    float current = tCurrentShadow.Sample(sPoint, input.uv);
    float history = tHistoryShadow.Sample(sPoint, input.uv);

    // テンポラル蓄積
    float result = lerp(current, history, gTemporalBlend);
    return float4(result, result, result, 1.0);
}
