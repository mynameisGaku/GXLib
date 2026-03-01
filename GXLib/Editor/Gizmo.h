#pragma once
/// @file Gizmo.h
/// @brief エディタ用3Dギズモ — 移動・回転・スケール操作のハンドルを描画する
///
/// 選択中エンティティの Transform に3色の矢印・リング・立方体を重ねて表示し、
/// マウスドラッグでインタラクティブに値を変更できる。
/// @addtogroup grp_editor/// @{

#include "pch_graphics.h"
#include "Math/Vector3.h"
#include "Math/Transform3D.h"

namespace gx
{

class PrimitiveBatch3D;

/// @brief ギズモの操作モード
enum class GizmoMode : uint32_t
{
    Translate = 0, ///< 移動モード（矢印ハンドル）
    Rotate,        ///< 回転モード（リングハンドル）
    Scale,         ///< スケールモード（立方体ハンドル）
};

/// @brief ギズモの軸識別子
enum class GizmoAxis : uint32_t
{
    None = 0, ///< 軸なし（未選択）
    X,        ///< X軸（赤）
    Y,        ///< Y軸（緑）
    Z,        ///< Z軸（青）
};

/// @brief シーンオブジェクトのインタラクティブ操作を行う3Dギズモ
class Gizmo
{
public:
    Gizmo() = default;
    ~Gizmo() = default;

    /// @brief 現在の操作モードを設定する（移動/回転/スケール）
    void SetMode(GizmoMode mode) { m_mode = mode; }

    /// @brief 現在の操作モードを取得する
    GizmoMode GetMode() const { return m_mode; }

    /// @brief ギズモハンドルの表示スケール係数を設定する
    void SetScale(float scale) { m_scale = scale; }

    /// @brief 表示スケール係数を取得する
    float GetScale() const { return m_scale; }

    /// @brief 指定されたPrimitiveBatch3Dを使ってギズモを描画する
    /// @param batch  ライン描画バッチ（Begin/Endの間で使用すること）
    /// @param transform  ギズモが表す位置/回転のトランスフォーム
    void Render(PrimitiveBatch3D& batch, const Transform3D& transform);

    /// @brief ピックレイをギズモ軸に対してテストし、最も近いヒット軸を返す
    /// @param rayOrigin  ワールド空間のレイ原点
    /// @param rayDir     ワールド空間のレイ方向（正規化済み）
    /// @param transform  ギズモがアタッチされているトランスフォーム
    /// @return ヒット閾値内でレイに最も近い軸、またはGizmoAxis::None
    GizmoAxis HitTest(const DirectX::XMFLOAT3& rayOrigin,
                       const DirectX::XMFLOAT3& rayDir,
                       const Transform3D& transform) const;

    /// @brief 指定した軸でドラッグ操作を開始する
    void BeginDrag(GizmoAxis axis, const DirectX::XMFLOAT3& rayOrigin,
                   const DirectX::XMFLOAT3& rayDir);

    /// @brief 進行中のドラッグを更新し、結果をtransformに適用する
    void UpdateDrag(const DirectX::XMFLOAT3& rayOrigin,
                    const DirectX::XMFLOAT3& rayDir,
                    Transform3D& transform);

    /// @brief 現在のドラッグ操作を終了する
    void EndDrag();

    /// @brief ドラッグ中の場合trueを返す
    bool IsDragging() const { return m_dragging; }

    /// @brief 現在ドラッグ中の軸を返す
    GizmoAxis GetDragAxis() const { return m_dragAxis; }

    /// @brief ギズモの有効/無効を設定する
    void SetEnabled(bool enabled) { m_enabled = enabled; }

    /// @brief ギズモが有効な場合trueを返す
    bool IsEnabled() const { return m_enabled; }

private:
    /// レイと軸線分の最短距離を計算する
    float RayAxisDistance(const DirectX::XMFLOAT3& rayOrigin,
                          const DirectX::XMFLOAT3& rayDir,
                          const DirectX::XMFLOAT3& axisOrigin,
                          const DirectX::XMFLOAT3& axisDir) const;

    /// レイと平面の交点を求める
    DirectX::XMFLOAT3 RayPlaneIntersect(const DirectX::XMFLOAT3& rayOrigin,
                                          const DirectX::XMFLOAT3& rayDir,
                                          const DirectX::XMFLOAT3& planePoint,
                                          const DirectX::XMFLOAT3& planeNormal) const;

    /// レイをドラッグ軸に投影し、軸上のパラメトリック位置を返す
    float ProjectOntoAxis(const DirectX::XMFLOAT3& rayOrigin,
                           const DirectX::XMFLOAT3& rayDir,
                           const DirectX::XMFLOAT3& axisOrigin,
                           const DirectX::XMFLOAT3& axisDir) const;

    GizmoMode m_mode     = GizmoMode::Translate; ///< 現在の操作モード
    GizmoAxis m_dragAxis = GizmoAxis::None;      ///< ドラッグ中の軸
    float m_scale        = 1.0f;                 ///< ハンドルの表示スケール係数
    bool  m_dragging     = false;                ///< ドラッグ操作中か
    bool  m_dragInitialized = false;                ///< 最初のUpdateDragフレームで開始値が記録されたらtrue
    bool  m_enabled      = true;                ///< ギズモの有効/無効フラグ
    DirectX::XMFLOAT3 m_dragStartPos       = {};   ///< ドラッグ開始時のレイ-軸交点
    DirectX::XMFLOAT3 m_dragStartObjectPos = {};   ///< ドラッグ開始時のオブジェクト位置
    DirectX::XMFLOAT3 m_dragStartScale     = {};   ///< ドラッグ開始時のオブジェクトスケール
    float m_dragStartAxisT = 0.0f;                  ///< ドラッグ開始時の軸上パラメトリックt
};

} // namespace gx
/// @}
