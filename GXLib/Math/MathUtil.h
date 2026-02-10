#pragma once

namespace GX {

/// @brief 数学ユーティリティ関数群
namespace MathUtil {

    /// @brief 円周率
    constexpr float PI      = 3.14159265358979323846f;

    /// @brief 円周率の2倍 (2*PI)
    constexpr float TAU     = PI * 2.0f;

    /// @brief 浮動小数点比較用の微小値
    constexpr float EPSILON = 1e-6f;

    /// @brief float値を指定範囲にクランプする
    /// @param value クランプ対象の値
    /// @param minVal 最小値
    /// @param maxVal 最大値
    /// @return クランプされた値
    inline float Clamp(float value, float minVal, float maxVal)
    {
        return (std::max)(minVal, (std::min)(maxVal, value));
    }

    /// @brief int値を指定範囲にクランプする
    /// @param value クランプ対象の値
    /// @param minVal 最小値
    /// @param maxVal 最大値
    /// @return クランプされた値
    inline int Clamp(int value, int minVal, int maxVal)
    {
        return (std::max)(minVal, (std::min)(maxVal, value));
    }

    /// @brief 2値間を線形補間する
    /// @param a 開始値 (t=0)
    /// @param b 終了値 (t=1)
    /// @param t 補間係数 [0, 1]
    /// @return 補間結果
    inline float Lerp(float a, float b, float t)
    {
        return a + (b - a) * t;
    }

    /// @brief 逆線形補間 (値から補間係数tを求める)
    /// @param a 開始値
    /// @param b 終了値
    /// @param value tを求める値
    /// @return 補間係数t (a==bの場合は0を返す)
    inline float InverseLerp(float a, float b, float value)
    {
        float denom = b - a;
        if (std::abs(denom) < EPSILON) return 0.0f;
        return (value - a) / denom;
    }

    /// @brief ある範囲の値を別の範囲にリマップする
    /// @param value リマップ対象の値
    /// @param fromMin 元の範囲の最小値
    /// @param fromMax 元の範囲の最大値
    /// @param toMin 変換先の範囲の最小値
    /// @param toMax 変換先の範囲の最大値
    /// @return リマップされた値
    inline float Remap(float value, float fromMin, float fromMax, float toMin, float toMax)
    {
        float t = InverseLerp(fromMin, fromMax, value);
        return Lerp(toMin, toMax, t);
    }

    /// @brief Hermite補間によるスムーズステップ (3次多項式)
    /// @param edge0 下限エッジ
    /// @param edge1 上限エッジ
    /// @param x 入力値
    /// @return 0〜1の滑らかに補間された値
    inline float SmoothStep(float edge0, float edge1, float x)
    {
        float t = Clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    /// @brief Ken Perlinの改良スムーズステップ (5次多項式)
    /// @param edge0 下限エッジ
    /// @param edge1 上限エッジ
    /// @param x 入力値
    /// @return 0〜1のより滑らかに補間された値
    inline float SmootherStep(float edge0, float edge1, float x)
    {
        float t = Clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }

    /// @brief 度数法からラジアンに変換する
    /// @param degrees 角度(度)
    /// @return 角度(ラジアン)
    inline float DegreesToRadians(float degrees)
    {
        return degrees * (PI / 180.0f);
    }

    /// @brief ラジアンから度数法に変換する
    /// @param radians 角度(ラジアン)
    /// @return 角度(度)
    inline float RadiansToDegrees(float radians)
    {
        return radians * (180.0f / PI);
    }

    /// @brief 角度を[-PI, PI]の範囲に正規化する
    /// @param radians 正規化する角度(ラジアン)
    /// @return 正規化された角度(ラジアン) [-PI, PI]
    inline float NormalizeAngle(float radians)
    {
        radians = std::fmod(radians + PI, TAU);
        if (radians < 0.0f) radians += TAU;
        return radians - PI;
    }

    /// @brief 絶対値を返す
    /// @param x 入力値
    /// @return 絶対値
    inline float Abs(float x) { return std::abs(x); }

    /// @brief 符号を返す (-1, 0, +1)
    /// @param x 入力値
    /// @return 正なら1.0、負なら-1.0、ゼロなら0.0
    inline float Sign(float x) { return (x > 0.0f) ? 1.0f : ((x < 0.0f) ? -1.0f : 0.0f); }

    /// @brief 2値のうち小さい方を返す
    /// @param a 1つ目の値
    /// @param b 2つ目の値
    /// @return 小さい方の値
    inline float Min(float a, float b) { return (std::min)(a, b); }

    /// @brief 2値のうち大きい方を返す
    /// @param a 1つ目の値
    /// @param b 2つ目の値
    /// @return 大きい方の値
    inline float Max(float a, float b) { return (std::max)(a, b); }

    /// @brief 切り捨て (床関数)
    /// @param x 入力値
    /// @return 切り捨てた整数値
    inline float Floor(float x) { return std::floor(x); }

    /// @brief 切り上げ (天井関数)
    /// @param x 入力値
    /// @return 切り上げた整数値
    inline float Ceil(float x) { return std::ceil(x); }

    /// @brief 四捨五入
    /// @param x 入力値
    /// @return 四捨五入した整数値
    inline float Round(float x) { return std::round(x); }

    /// @brief 浮動小数点の剰余 (fmod)
    /// @param x 被除数
    /// @param y 除数
    /// @return x/yの剰余
    inline float Fmod(float x, float y) { return std::fmod(x, y); }

    /// @brief 値が2の累乗かどうか判定する
    /// @param x 判定対象の値
    /// @return 2の累乗ならtrue
    inline bool IsPowerOfTwo(uint32_t x)
    {
        return x != 0 && (x & (x - 1)) == 0;
    }

    /// @brief 指定値以上の最小の2の累乗を返す
    /// @param x 入力値
    /// @return x以上の最小の2の累乗値
    inline uint32_t NextPowerOfTwo(uint32_t x)
    {
        if (x == 0) return 1;
        x--;
        x |= x >> 1;
        x |= x >> 2;
        x |= x >> 4;
        x |= x >> 8;
        x |= x >> 16;
        return x + 1;
    }

    /// @brief 2つのfloat値がほぼ等しいか判定する
    /// @param a 比較する値1
    /// @param b 比較する値2
    /// @param epsilon 許容誤差 (デフォルト: EPSILON)
    /// @return 差がepsilon以下ならtrue
    inline bool ApproximatelyEqual(float a, float b, float epsilon = EPSILON)
    {
        return std::abs(a - b) <= epsilon;
    }

} // namespace MathUtil
} // namespace GX
