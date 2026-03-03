/// @file WaterFFT.hlsl
/// @brief FFTベースの波高マップ生成コンピュートシェーダー（Gerstner合成）。
/// より高品質な波シミュレーション用にオプション使用。

cbuffer WaterFFTCB : register(b0)
{
    float  Time;
    float  Amplitude;
    float  WindSpeed;
    float  WindDirX;
    float  WindDirY;
    int    Resolution;
    float2 _pad;
};

RWTexture2D<float> HeightMap : register(u0);

// Phillipsスペクトル近似
float PhillipsSpectrum(float2 k, float2 windDir, float windSpeed)
{
    float kLen = length(k);
    if (kLen < 0.0001) return 0.0;

    float L = windSpeed * windSpeed / 9.81;
    float kLen2 = kLen * kLen;
    float kLen4 = kLen2 * kLen2;

    float kDotW = dot(normalize(k), normalize(windDir));
    float kDotW2 = kDotW * kDotW;

    float damping = 0.001;
    float l2 = L * L * damping * damping;

    return Amplitude * exp(-1.0 / (kLen2 * L * L)) / kLen4 * kDotW2 * exp(-kLen2 * l2);
}

[numthreads(8, 8, 1)]
void CS(uint3 dtid : SV_DispatchThreadID)
{
    if (dtid.x >= (uint)Resolution || dtid.y >= (uint)Resolution) return;

    float2 uv = (float2(dtid.xy) / float(Resolution)) * 2.0 - 1.0;
    float2 windDir = float2(WindDirX, WindDirY);

    // 複数の波成分を合成（Gerstner合成）
    float height = 0.0;
    for (int i = 0; i < 8; i++)
    {
        float angle = float(i) * 3.14159265 / 4.0;
        float2 dir = float2(cos(angle), sin(angle));
        float freq = 0.5 + float(i) * 0.3;
        float amp = Amplitude / (1.0 + float(i) * 0.5);
        float phase = dot(dir, uv * float(Resolution)) * freq + Time * sqrt(9.81 * freq);
        float alignment = max(0.0, dot(normalize(dir), normalize(windDir)));
        height += amp * sin(phase) * (0.3 + 0.7 * alignment);
    }

    HeightMap[dtid.xy] = height;
}
