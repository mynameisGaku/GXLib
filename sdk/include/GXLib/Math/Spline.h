#pragma once
#include "pch_common.h"
#include "Math/Vector3.h"

namespace gx {
/// @addtogroup grp_math
/// @{

/// @brief スプライン補間の種類
enum class SplineType { Linear, CatmullRom, CubicBezier };

/// @brief 制御点で定義されるスプライン曲線
class Spline
{
public:
    Spline() = default;
    ~Spline() = default;

    /// @brief 制御点を末尾に追加する
    /// @param point 追加する3D座標
    void AddPoint(const Vector3& point);

    /// @brief 指定インデックスに制御点を挿入する
    /// @param index 挿入位置のインデックス
    /// @param point 挿入する3D座標
    void InsertPoint(int index, const Vector3& point);

    /// @brief 指定インデックスの制御点を削除する
    /// @param index 削除する制御点のインデックス
    void RemovePoint(int index);

    /// @brief 指定インデックスの制御点を設定する
    /// @param index 設定先のインデックス
    /// @param point 新しい3D座標
    void SetPoint(int index, const Vector3& point);

    /// @brief 指定インデックスの制御点を取得する
    /// @param index 取得する制御点のインデックス
    /// @return 制御点のconst参照
    const Vector3& GetPoint(int index) const;

    /// @brief 制御点の数を取得する
    /// @return 制御点の数
    int GetPointCount() const;

    /// @brief 全制御点をクリアする
    void Clear();

    /// @brief 補間タイプを設定する
    /// @param type 使用するSplineType
    void SetType(SplineType type);

    /// @brief 現在の補間タイプを取得する
    /// @return 現在のSplineType
    SplineType GetType() const;

    /// @brief スプラインをループさせるかどうかを設定する
    /// @param closed trueでループさせる
    void SetClosed(bool closed);

    /// @brief スプラインがループしているかを取得する
    /// @return ループしていればtrue
    bool IsClosed() const;

    /// @brief パラメータ t でスプライン上の位置を評価する
    /// @param t 0.0〜1.0 の正規化パラメータ（スプライン全体に対する位置）
    /// @return スプライン上の3D座標
    /// @note CatmullRom の場合、最低4点が必要
    Vector3 Evaluate(float t) const;

    /// @brief パラメータ t でスプライン上の接線(方向)を評価する
    /// @param t 0.0〜1.0 の正規化パラメータ
    /// @return 接線方向の3Dベクトル（正規化されていない）
    Vector3 EvaluateTangent(float t) const;

    /// @brief スプラインの近似全長を取得する
    /// @param subdivisions 近似に使用する分割数（大きいほど精度が高い）
    /// @return スプラインの近似全長
    float GetTotalLength(int subdivisions = 64) const;

    /// @brief 始点からの弧長距離で位置を評価する
    /// @param distance 始点からの距離
    /// @param subdivisions 近似に使用する分割数
    /// @return スプライン上の3D座標
    Vector3 EvaluateByDistance(float distance, int subdivisions = 64) const;

    /// @brief 指定点に最も近いスプライン上のパラメータを求める
    /// @param point 基準となる3D座標
    /// @param subdivisions 探索の分割数
    /// @return 最も近い位置に対応するパラメータ t (0.0〜1.0)
    float FindClosestParameter(const Vector3& point, int subdivisions = 64) const;

private:
    gx::Vector<Vector3> m_points;
    SplineType m_type = SplineType::CatmullRom;
    bool m_closed = false;

    /// @brief 2点間の線形補間
    /// @param a 始点
    /// @param b 終点
    /// @param t 補間係数 [0, 1]
    /// @return 補間された3D座標
    static Vector3 LerpPoints(const Vector3& a, const Vector3& b, float t);

    /// @brief CatmullRom補間
    /// @param p0 前の制御点
    /// @param p1 セグメント始点
    /// @param p2 セグメント終点
    /// @param p3 次の制御点
    /// @param t 補間係数 [0, 1]
    /// @return 補間された3D座標
    static Vector3 CatmullRom(const Vector3& p0, const Vector3& p1,
                                const Vector3& p2, const Vector3& p3, float t);

    /// @brief キュービックベジェ補間（セグメントあたり4点）
    /// @param p0 始点
    /// @param p1 制御点1
    /// @param p2 制御点2
    /// @param p3 終点
    /// @param t 補間係数 [0, 1]
    /// @return 補間された3D座標
    static Vector3 CubicBezier(const Vector3& p0, const Vector3& p1,
                                 const Vector3& p2, const Vector3& p3, float t);

    /// @brief グローバル t からセグメントインデックスとローカル t を求める
    /// @param t グローバルパラメータ [0, 1]
    /// @param segIndex セグメントインデックスの出力先
    /// @param localT セグメント内ローカル t の出力先
    void GetSegment(float t, int& segIndex, float& localT) const;

    /// @brief CatmullRomセグメントの4制御点を取得する
    /// @param segIndex セグメントインデックス
    /// @param p0 前の制御点の出力先
    /// @param p1 セグメント始点の出力先
    /// @param p2 セグメント終点の出力先
    /// @param p3 次の制御点の出力先
    void GetCatmullRomPoints(int segIndex, Vector3& p0, Vector3& p1,
                              Vector3& p2, Vector3& p3) const;
};

/// @}
} // namespace gx
