#pragma once
/// @file Noise.h
/// @brief パーリンノイズと派生ノイズ関数（FBM, Turbulence, Ridge）
///
/// 1D/2D/3D のパーリンノイズをヘッダーオンリーで提供する。
/// 地形の高さマップ生成、テクスチャの揺らぎ、雲の形状など
/// 「自然な不規則さ」が欲しい場面で使う。
/// @addtogroup grp_math/// @{

#include "pch_common.h"

namespace gx {

/// @brief パーリンノイズおよび派生ノイズを提供する静的ユーティリティクラス
class Noise
{
public:
    Noise() = delete;

    // ========================================================================
    // 基本ノイズ
    // ========================================================================

    /// @brief 1Dパーリンノイズ
    /// @param x 入力座標
    /// @return ノイズ値 [-1, 1]
    static inline float Perlin1D(float x)
    {
        int xi = FloorToInt(x) & 255;
        float xf = x - std::floor(x);
        float u  = Fade(xf);

        int a = s_perm[xi];
        int b = s_perm[xi + 1];

        float ga = Grad1D(a, xf);
        float gb = Grad1D(b, xf - 1.0f);

        return Lerp(ga, gb, u);
    }

    /// @brief 2Dパーリンノイズ
    /// @param x X座標
    /// @param y Y座標
    /// @return ノイズ値 [-1, 1]
    static inline float Perlin2D(float x, float y)
    {
        int xi = FloorToInt(x) & 255;
        int yi = FloorToInt(y) & 255;

        float xf = x - std::floor(x);
        float yf = y - std::floor(y);

        float u = Fade(xf);
        float v = Fade(yf);

        int aa = s_perm[s_perm[xi]     + yi];
        int ab = s_perm[s_perm[xi]     + yi + 1];
        int ba = s_perm[s_perm[xi + 1] + yi];
        int bb = s_perm[s_perm[xi + 1] + yi + 1];

        float x1 = Lerp(Grad2D(aa, xf,        yf),
                         Grad2D(ba, xf - 1.0f, yf),        u);
        float x2 = Lerp(Grad2D(ab, xf,        yf - 1.0f),
                         Grad2D(bb, xf - 1.0f, yf - 1.0f), u);

        return Lerp(x1, x2, v);
    }

    /// @brief 3Dパーリンノイズ
    /// @param x X座標
    /// @param y Y座標
    /// @param z Z座標
    /// @return ノイズ値 [-1, 1]
    static inline float Perlin3D(float x, float y, float z)
    {
        int xi = FloorToInt(x) & 255;
        int yi = FloorToInt(y) & 255;
        int zi = FloorToInt(z) & 255;

        float xf = x - std::floor(x);
        float yf = y - std::floor(y);
        float zf = z - std::floor(z);

        float u = Fade(xf);
        float v = Fade(yf);
        float w = Fade(zf);

        int a  = s_perm[xi]     + yi;
        int aa = s_perm[a]      + zi;
        int ab = s_perm[a + 1]  + zi;
        int b  = s_perm[xi + 1] + yi;
        int ba = s_perm[b]      + zi;
        int bb = s_perm[b + 1]  + zi;

        float x1, x2, y1, y2;

        x1 = Lerp(Grad3D(s_perm[aa],     xf,        yf,        zf),
                   Grad3D(s_perm[ba],     xf - 1.0f, yf,        zf),        u);
        x2 = Lerp(Grad3D(s_perm[ab],     xf,        yf - 1.0f, zf),
                   Grad3D(s_perm[bb],     xf - 1.0f, yf - 1.0f, zf),        u);
        y1 = Lerp(x1, x2, v);

        x1 = Lerp(Grad3D(s_perm[aa + 1], xf,        yf,        zf - 1.0f),
                   Grad3D(s_perm[ba + 1], xf - 1.0f, yf,        zf - 1.0f), u);
        x2 = Lerp(Grad3D(s_perm[ab + 1], xf,        yf - 1.0f, zf - 1.0f),
                   Grad3D(s_perm[bb + 1], xf - 1.0f, yf - 1.0f, zf - 1.0f), u);
        y2 = Lerp(x1, x2, v);

        return Lerp(y1, y2, w);
    }

    // ========================================================================
    // 派生ノイズ
    // ========================================================================

    /// @brief 2D Fractional Brownian Motion（fBm）
    /// @param x X座標
    /// @param y Y座標
    /// @param octaves オクターブ数（デフォルト: 6）
    /// @param lacunarity 各オクターブの周波数倍率（デフォルト: 2.0）
    /// @param gain 各オクターブの振幅減衰率（デフォルト: 0.5）
    /// @return ノイズ値（概ね [-1, 1]）
    static inline float FBM(float x, float y,
                            int octaves = 6,
                            float lacunarity = 2.0f,
                            float gain = 0.5f)
    {
        float sum       = 0.0f;
        float amplitude = 1.0f;
        float frequency = 1.0f;

        for (int i = 0; i < octaves; ++i)
        {
            sum       += amplitude * Perlin2D(x * frequency, y * frequency);
            amplitude *= gain;
            frequency *= lacunarity;
        }
        return sum;
    }

    /// @brief 3D Fractional Brownian Motion（fBm）
    /// @param x X座標
    /// @param y Y座標
    /// @param z Z座標
    /// @param octaves オクターブ数（デフォルト: 6）
    /// @param lacunarity 各オクターブの周波数倍率（デフォルト: 2.0）
    /// @param gain 各オクターブの振幅減衰率（デフォルト: 0.5）
    /// @return ノイズ値（概ね [-1, 1]）
    static inline float FBM3D(float x, float y, float z,
                              int octaves = 6,
                              float lacunarity = 2.0f,
                              float gain = 0.5f)
    {
        float sum       = 0.0f;
        float amplitude = 1.0f;
        float frequency = 1.0f;

        for (int i = 0; i < octaves; ++i)
        {
            sum       += amplitude * Perlin3D(x * frequency, y * frequency, z * frequency);
            amplitude *= gain;
            frequency *= lacunarity;
        }
        return sum;
    }

    /// @brief タービュランスノイズ（絶対値fBm）
    /// @param x X座標
    /// @param y Y座標
    /// @param octaves オクターブ数（デフォルト: 6）
    /// @return ノイズ値 [0, 1]（概ね）
    static inline float Turbulence(float x, float y, int octaves = 6)
    {
        float sum       = 0.0f;
        float amplitude = 1.0f;
        float frequency = 1.0f;

        for (int i = 0; i < octaves; ++i)
        {
            sum       += amplitude * std::abs(Perlin2D(x * frequency, y * frequency));
            amplitude *= 0.5f;
            frequency *= 2.0f;
        }
        return sum;
    }

    /// @brief リッジノイズ（尾根状の地形生成向け）
    /// @param x X座標
    /// @param y Y座標
    /// @param octaves オクターブ数（デフォルト: 6）
    /// @param offset リッジオフセット（デフォルト: 1.0）
    /// @return ノイズ値 [0, 1]（概ね）
    static inline float Ridge(float x, float y, int octaves = 6, float offset = 1.0f)
    {
        float sum       = 0.0f;
        float amplitude = 0.5f;
        float frequency = 1.0f;
        float prev      = 1.0f;

        for (int i = 0; i < octaves; ++i)
        {
            float n = offset - std::abs(Perlin2D(x * frequency, y * frequency));
            n = n * n;
            sum       += n * amplitude * prev;
            prev       = n;
            frequency *= 2.0f;
            amplitude *= 0.5f;
        }
        return sum;
    }

    // ========================================================================
    // ユーティリティ
    // ========================================================================

    /// @brief [-1, 1] の値を [outMin, outMax] にリマップする
    /// @param value 入力値（[-1, 1] を想定）
    /// @param outMin 出力範囲の最小値
    /// @param outMax 出力範囲の最大値
    /// @return リマップされた値
    static inline float Remap(float value, float outMin, float outMax)
    {
        return outMin + (value + 1.0f) * 0.5f * (outMax - outMin);
    }

private:
    // ========================================================================
    // 内部ヘルパー
    // ========================================================================

    /// @brief 5次フェード関数: 6t^5 - 15t^4 + 10t^3
    static inline float Fade(float t)
    {
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }

    /// @brief 線形補間
    static inline float Lerp(float a, float b, float t)
    {
        return a + t * (b - a);
    }

    /// @brief 1D勾配関数
    static inline float Grad1D(int hash, float x)
    {
        return (hash & 1) ? -x : x;
    }

    /// @brief 2D勾配関数
    static inline float Grad2D(int hash, float x, float y)
    {
        int h = hash & 3;
        float u = (h & 2) ? -x : x;
        float v = (h & 1) ? -y : y;
        return u + v;
    }

    /// @brief 3D勾配関数（Ken Perlin改良版）
    static inline float Grad3D(int hash, float x, float y, float z)
    {
        int h = hash & 15;
        float u = h < 8 ? x : y;
        float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
        return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
    }

    /// @brief floatのfloorをint型に変換する（負の値でも正しく動作）
    static inline int FloorToInt(float x)
    {
        int xi = static_cast<int>(x);
        return (x < static_cast<float>(xi)) ? xi - 1 : xi;
    }

    // ========================================================================
    // Ken Perlinの標準順列テーブル（256エントリ、512に拡張）
    // ========================================================================
    static constexpr int s_perm[512] = {
        151,160,137, 91, 90, 15,131, 13,201, 95, 96, 53,194,233,  7,225,
        140, 36,103, 30, 69,142,  8, 99, 37,240, 21, 10, 23,190,  6,148,
        247,120,234, 75,  0, 26,197, 62, 94,252,219,203,117, 35, 11, 32,
         57,177, 33, 88,237,149, 56, 87,174, 20,125,136,171,168, 68,175,
         74,165, 71,134,139, 48, 27,166, 77,146,158,231, 83,111,229,122,
         60,211,133,230,220,105, 92, 41, 55, 46,245, 40,244,102,143, 54,
         65, 25, 63,161,  1,216, 80, 73,209, 76,132,187,208, 89, 18,169,
        200,196,135,130,116,188,159, 86,164,100,109,198,173,186,  3, 64,
         52,217,226,250,124,123,  5,202, 38,147,118,126,255, 82, 85,212,
        207,206, 59,227, 47, 16, 58, 17,182,189, 28, 42,223,183,170,213,
        119,248,152,  2, 44,154,163, 70,221,153,101,155,167, 43,172,  9,
        129, 22, 39,253, 19, 98,108,110, 79,113,224,232,178,185,112,104,
        218,246, 97,228,251, 34,242,193,238,210,144, 12,191,179,162,241,
         81, 51,145,235,249, 14,239,107, 49,192,214, 31,181,199,106,157,
        184, 84,204,176,115,121, 50, 45,127,  4,150,254,138,236,205, 93,
        222,114, 67, 29, 24, 72,243,141,128,195, 78, 66,215, 61,156,180,
        // 繰り返し（512エントリに拡張）
        151,160,137, 91, 90, 15,131, 13,201, 95, 96, 53,194,233,  7,225,
        140, 36,103, 30, 69,142,  8, 99, 37,240, 21, 10, 23,190,  6,148,
        247,120,234, 75,  0, 26,197, 62, 94,252,219,203,117, 35, 11, 32,
         57,177, 33, 88,237,149, 56, 87,174, 20,125,136,171,168, 68,175,
         74,165, 71,134,139, 48, 27,166, 77,146,158,231, 83,111,229,122,
         60,211,133,230,220,105, 92, 41, 55, 46,245, 40,244,102,143, 54,
         65, 25, 63,161,  1,216, 80, 73,209, 76,132,187,208, 89, 18,169,
        200,196,135,130,116,188,159, 86,164,100,109,198,173,186,  3, 64,
         52,217,226,250,124,123,  5,202, 38,147,118,126,255, 82, 85,212,
        207,206, 59,227, 47, 16, 58, 17,182,189, 28, 42,223,183,170,213,
        119,248,152,  2, 44,154,163, 70,221,153,101,155,167, 43,172,  9,
        129, 22, 39,253, 19, 98,108,110, 79,113,224,232,178,185,112,104,
        218,246, 97,228,251, 34,242,193,238,210,144, 12,191,179,162,241,
         81, 51,145,235,249, 14,239,107, 49,192,214, 31,181,199,106,157,
        184, 84,204,176,115,121, 50, 45,127,  4,150,254,138,236,205, 93,
        222,114, 67, 29, 24, 72,243,141,128,195, 78, 66,215, 61,156,180
    };
};

} // namespace gx
/// @}
