/// @file Water.hlsl
/// @brief Gerstner波とフレネルによる水面描画

cbuffer WaterCB : register(b0)
{
    float4x4 ViewProjection;
    float4x4 World;
    float3   CameraPosition;
    float    Time;
    float3   SunDirection;
    float    WaterLevel;
    float3   SunColor;
    float    PlaneSize;
    float4   WaterColor;  // rgb + depth falloff alpha
    float2   WindDirection;
    float    WaveAmplitude;
    float    WaveFrequency;
    float    SpecularPower;
    float    FresnelBias;
    float    FresnelPower;
    float    _pad;
};

struct VSInput
{
    float3 position : POSITION;
    float2 uv : TEXCOORD0;
};

struct VSOutput
{
    float4 clipPos : SV_Position;
    float3 worldPos : TEXCOORD0;
    float2 uv : TEXCOORD1;
    float3 normal : TEXCOORD2;
};

// Gerstner波ヘルパー
float3 GerstnerWave(float3 pos, float2 dir, float amplitude, float frequency,
                    float time, out float3 tangent, out float3 binormal)
{
    float k = 2.0 * 3.14159265 * frequency;
    float speed = sqrt(9.81 / k);
    float phase = k * (dot(dir, pos.xz) - speed * time);
    float s = sin(phase);
    float c = cos(phase);

    float3 displaced = pos;
    displaced.x += dir.x * amplitude * c;
    displaced.z += dir.y * amplitude * c;
    displaced.y += amplitude * s;

    tangent = float3(1.0 - dir.x * dir.x * k * amplitude * s,
                     dir.x * k * amplitude * c,
                     -dir.x * dir.y * k * amplitude * s);
    binormal = float3(-dir.x * dir.y * k * amplitude * s,
                      dir.y * k * amplitude * c,
                      1.0 - dir.y * dir.y * k * amplitude * s);

    return displaced;
}

VSOutput VS(VSInput input)
{
    VSOutput o;

    float3 worldPos = mul(float4(input.position, 1.0), World).xyz;

    // Gerstner波を適用（3オクターブ）
    float3 tan1, bin1, tan2, bin2, tan3, bin3;
    float3 pos = worldPos;
    pos = GerstnerWave(pos, normalize(WindDirection),
                       WaveAmplitude, WaveFrequency, Time, tan1, bin1);
    pos = GerstnerWave(pos, normalize(WindDirection + float2(0.3, -0.2)),
                       WaveAmplitude * 0.5, WaveFrequency * 1.7, Time, tan2, bin2);
    pos = GerstnerWave(pos, normalize(WindDirection + float2(-0.4, 0.1)),
                       WaveAmplitude * 0.25, WaveFrequency * 2.3, Time, tan3, bin3);

    float3 tangent = normalize(tan1 + tan2 * 0.5 + tan3 * 0.25);
    float3 binormal = normalize(bin1 + bin2 * 0.5 + bin3 * 0.25);
    o.normal = normalize(cross(binormal, tangent));

    o.worldPos = pos;
    o.uv = input.uv;
    o.clipPos = mul(float4(pos, 1.0), ViewProjection);
    return o;
}

float4 PS(VSOutput input) : SV_Target
{
    float3 N = normalize(input.normal);
    float3 V = normalize(CameraPosition - input.worldPos);
    float3 L = normalize(-SunDirection);
    float3 H = normalize(V + L);

    // フレネル（Schlick近似）
    float NdotV = saturate(dot(N, V));
    float fresnel = FresnelBias + (1.0 - FresnelBias) * pow(1.0 - NdotV, FresnelPower);

    // 水のディフューズカラー（深いほど暗い）
    float3 waterDiffuse = WaterColor.rgb;

    // スペキュラ
    float NdotH = saturate(dot(N, H));
    float specular = pow(NdotH, SpecularPower);
    float3 specularColor = SunColor * specular;

    // 簡易空反射色（疑似キューブマップ: 青から水平線へ補間）
    float3 R = reflect(-V, N);
    float skyFactor = saturate(R.y * 0.5 + 0.5);
    float3 skyColor = lerp(float3(0.6, 0.7, 0.8), float3(0.2, 0.5, 0.9), skyFactor);

    // 最終カラー
    float3 color = lerp(waterDiffuse, skyColor, fresnel) + specularColor * 0.5;

    return float4(color, lerp(0.7, 0.95, fresnel));
}
