#pragma once
#include "Vector2.h"
#include "Vector3.h"
#include "Color.h"

namespace GX {

/// @brief 擬似乱数生成器 (Mersenne Twister)
class Random
{
public:
    /// @brief デフォルトコンストラクタ (ランダムシードで初期化)
    Random();

    /// @brief 指定シードで初期化するコンストラクタ
    /// @param seed 乱数シード値
    explicit Random(uint32_t seed);

    /// @brief 乱数シードを再設定する
    /// @param seed 新しいシード値
    void SetSeed(uint32_t seed);

    /// @brief ランダムな32ビット整数を生成する
    /// @return 乱数値
    int32_t Int();

    /// @brief 指定範囲のランダムな整数を生成する
    /// @param min 最小値(含む)
    /// @param max 最大値(含む)
    /// @return [min, max] の範囲の乱数値
    int32_t Int(int32_t min, int32_t max);

    /// @brief 0.0〜1.0のランダムなfloat値を生成する
    /// @return [0.0, 1.0] の範囲の乱数値
    float Float();

    /// @brief 指定範囲のランダムなfloat値を生成する
    /// @param min 最小値
    /// @param max 最大値
    /// @return [min, max] の範囲の乱数値
    float Float(float min, float max);

    /// @brief 指定範囲内のランダムな2Dベクトルを生成する
    /// @param minX X成分の最小値
    /// @param maxX X成分の最大値
    /// @param minY Y成分の最小値
    /// @param maxY Y成分の最大値
    /// @return 範囲内のランダムなVector2
    Vector2 Vector2InRange(float minX, float maxX, float minY, float maxY);

    /// @brief 指定範囲内のランダムな3Dベクトルを生成する
    /// @param minX X成分の最小値
    /// @param maxX X成分の最大値
    /// @param minY Y成分の最小値
    /// @param maxY Y成分の最大値
    /// @param minZ Z成分の最小値
    /// @param maxZ Z成分の最大値
    /// @return 範囲内のランダムなVector3
    Vector3 Vector3InRange(float minX, float maxX, float minY, float maxY, float minZ, float maxZ);

    /// @brief 円内のランダムな点を生成する(均一分布)
    /// @param radius 円の半径 (デフォルト: 1.0)
    /// @return 円内のランダムなVector2
    Vector2 PointInCircle(float radius = 1.0f);

    /// @brief 球内のランダムな点を生成する(均一分布)
    /// @param radius 球の半径 (デフォルト: 1.0)
    /// @return 球内のランダムなVector3
    Vector3 PointInSphere(float radius = 1.0f);

    /// @brief ランダムな2D方向の単位ベクトルを生成する
    /// @return 長さ1のランダムな方向Vector2
    Vector2 Direction2D();

    /// @brief ランダムな3D方向の単位ベクトルを生成する
    /// @return 長さ1のランダムな方向Vector3
    Vector3 Direction3D();

    /// @brief ランダムな色を生成する
    /// @param alpha アルファ値 (デフォルト: 1.0)
    /// @return ランダムなColor
    Color RandomColor(float alpha = 1.0f);

    /// @brief グローバル共有インスタンスを取得する
    /// @return グローバルRandomインスタンスへの参照
    static Random& Global();

private:
    std::mt19937 m_engine;
};

} // namespace GX
