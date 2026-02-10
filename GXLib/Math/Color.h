#pragma once
#include "MathUtil.h"

namespace GX {

/// @brief RGBA色 (float4成分、0.0〜1.0)
struct Color
{
    float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;

    /// @brief 白色 (1, 1, 1, 1) で初期化するデフォルトコンストラクタ
    Color() = default;

    /// @brief float成分で初期化するコンストラクタ
    /// @param r 赤成分 [0.0, 1.0]
    /// @param g 緑成分 [0.0, 1.0]
    /// @param b 青成分 [0.0, 1.0]
    /// @param a アルファ成分 [0.0, 1.0] (デフォルト: 1.0)
    Color(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}

    /// @brief 32ビットRGBA整数から初期化するコンストラクタ
    /// @param rgba RGBA形式の32ビット値 (0xRRGGBBAA)
    Color(uint32_t rgba)
    {
        r = ((rgba >> 24) & 0xFF) / 255.0f;
        g = ((rgba >> 16) & 0xFF) / 255.0f;
        b = ((rgba >>  8) & 0xFF) / 255.0f;
        a = ((rgba >>  0) & 0xFF) / 255.0f;
    }

    /// @brief 8ビット整数成分で初期化するコンストラクタ
    /// @param r 赤成分 [0, 255]
    /// @param g 緑成分 [0, 255]
    /// @param b 青成分 [0, 255]
    /// @param a アルファ成分 [0, 255] (デフォルト: 255)
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
        : r(r / 255.0f), g(g / 255.0f), b(b / 255.0f), a(a / 255.0f) {}

    /// @brief 32ビットRGBA整数値に変換する
    /// @return RGBA形式の32ビット値 (0xRRGGBBAA)
    uint32_t ToRGBA() const
    {
        uint8_t ri = static_cast<uint8_t>(MathUtil::Clamp(r, 0.0f, 1.0f) * 255.0f + 0.5f);
        uint8_t gi = static_cast<uint8_t>(MathUtil::Clamp(g, 0.0f, 1.0f) * 255.0f + 0.5f);
        uint8_t bi = static_cast<uint8_t>(MathUtil::Clamp(b, 0.0f, 1.0f) * 255.0f + 0.5f);
        uint8_t ai = static_cast<uint8_t>(MathUtil::Clamp(a, 0.0f, 1.0f) * 255.0f + 0.5f);
        return (ri << 24) | (gi << 16) | (bi << 8) | ai;
    }

    /// @brief 32ビットABGR整数値に変換する
    /// @return ABGR形式の32ビット値 (0xAABBGGRR)
    uint32_t ToABGR() const
    {
        uint8_t ri = static_cast<uint8_t>(MathUtil::Clamp(r, 0.0f, 1.0f) * 255.0f + 0.5f);
        uint8_t gi = static_cast<uint8_t>(MathUtil::Clamp(g, 0.0f, 1.0f) * 255.0f + 0.5f);
        uint8_t bi = static_cast<uint8_t>(MathUtil::Clamp(b, 0.0f, 1.0f) * 255.0f + 0.5f);
        uint8_t ai = static_cast<uint8_t>(MathUtil::Clamp(a, 0.0f, 1.0f) * 255.0f + 0.5f);
        return (ai << 24) | (bi << 16) | (gi << 8) | ri;
    }

    /// @brief XMFLOAT4に変換する
    /// @return XMFLOAT4形式の色値
    XMFLOAT4 ToXMFLOAT4() const { return { r, g, b, a }; }

    /// @brief HSV色空間からColorを生成する
    /// @param h 色相 [0, 360)
    /// @param s 彩度 [0, 1]
    /// @param v 明度 [0, 1]
    /// @param a アルファ成分 (デフォルト: 1.0)
    /// @return 変換されたColor
    static Color FromHSV(float h, float s, float v, float a = 1.0f)
    {
        // h: [0, 360), s: [0, 1], v: [0, 1]
        h = std::fmod(h, 360.0f);
        if (h < 0) h += 360.0f;
        float c = v * s;
        float x = c * (1.0f - std::abs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
        float m = v - c;

        float r1, g1, b1;
        if (h < 60.0f)       { r1 = c; g1 = x; b1 = 0; }
        else if (h < 120.0f) { r1 = x; g1 = c; b1 = 0; }
        else if (h < 180.0f) { r1 = 0; g1 = c; b1 = x; }
        else if (h < 240.0f) { r1 = 0; g1 = x; b1 = c; }
        else if (h < 300.0f) { r1 = x; g1 = 0; b1 = c; }
        else                 { r1 = c; g1 = 0; b1 = x; }

        return { r1 + m, g1 + m, b1 + m, a };
    }

    /// @brief HSV色空間に変換する
    /// @param h 色相の出力先 [0, 360)
    /// @param s 彩度の出力先 [0, 1]
    /// @param v 明度の出力先 [0, 1]
    void ToHSV(float& h, float& s, float& v) const
    {
        float maxC = (std::max)({ r, g, b });
        float minC = (std::min)({ r, g, b });
        float delta = maxC - minC;
        v = maxC;
        s = (maxC > 0.0f) ? (delta / maxC) : 0.0f;

        if (delta < MathUtil::EPSILON)
        {
            h = 0.0f;
        }
        else if (maxC == r)
        {
            h = 60.0f * std::fmod((g - b) / delta, 6.0f);
        }
        else if (maxC == g)
        {
            h = 60.0f * ((b - r) / delta + 2.0f);
        }
        else
        {
            h = 60.0f * ((r - g) / delta + 4.0f);
        }
        if (h < 0.0f) h += 360.0f;
    }

    /// @brief スカラー乗算
    /// @param s 乗算するスカラー値
    /// @return スカラー乗算結果の色
    Color operator*(float s) const { return { r * s, g * s, b * s, a * s }; }

    /// @brief 色の加算
    /// @param c 加算する色
    /// @return 加算結果の色
    Color operator+(const Color& c) const { return { r + c.r, g + c.g, b + c.b, a + c.a }; }

    /// @brief 2つの色間を線形補間する
    /// @param a 開始色 (t=0)
    /// @param b 終了色 (t=1)
    /// @param t 補間係数 [0, 1]
    /// @return 補間結果の色
    static Color Lerp(const Color& a, const Color& b, float t)
    {
        return {
            a.r + (b.r - a.r) * t,
            a.g + (b.g - a.g) * t,
            a.b + (b.b - a.b) * t,
            a.a + (b.a - a.a) * t
        };
    }

    /// @brief 白色 (1, 1, 1, 1) を返す
    /// @return 白色
    static Color White()       { return { 1.0f, 1.0f, 1.0f, 1.0f }; }

    /// @brief 黒色 (0, 0, 0, 1) を返す
    /// @return 黒色
    static Color Black()       { return { 0.0f, 0.0f, 0.0f, 1.0f }; }

    /// @brief 赤色 (1, 0, 0, 1) を返す
    /// @return 赤色
    static Color Red()         { return { 1.0f, 0.0f, 0.0f, 1.0f }; }

    /// @brief 緑色 (0, 1, 0, 1) を返す
    /// @return 緑色
    static Color Green()       { return { 0.0f, 1.0f, 0.0f, 1.0f }; }

    /// @brief 青色 (0, 0, 1, 1) を返す
    /// @return 青色
    static Color Blue()        { return { 0.0f, 0.0f, 1.0f, 1.0f }; }

    /// @brief 黄色 (1, 1, 0, 1) を返す
    /// @return 黄色
    static Color Yellow()      { return { 1.0f, 1.0f, 0.0f, 1.0f }; }

    /// @brief シアン (0, 1, 1, 1) を返す
    /// @return シアン色
    static Color Cyan()        { return { 0.0f, 1.0f, 1.0f, 1.0f }; }

    /// @brief マゼンタ (1, 0, 1, 1) を返す
    /// @return マゼンタ色
    static Color Magenta()     { return { 1.0f, 0.0f, 1.0f, 1.0f }; }

    /// @brief 完全透明 (0, 0, 0, 0) を返す
    /// @return 完全透明色
    static Color Transparent() { return { 0.0f, 0.0f, 0.0f, 0.0f }; }
};

} // namespace GX
