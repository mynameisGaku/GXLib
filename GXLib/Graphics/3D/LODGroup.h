#pragma once
/// @file LODGroup.h
/// @brief LOD（Level of Detail）グループ管理

#include "pch.h"

namespace GX
{

class Model;
class Camera3D;
class Transform3D;

/// @brief LODレベル定義
struct LODLevel
{
    Model* model = nullptr;           ///< このLODレベルのモデル
    float  screenPercentage = 1.0f;   ///< この閾値以下で切り替え (0.0〜1.0)
};

/// @brief LODグループ — 距離に応じたモデル切り替え
///
/// 複数のLODレベルを登録し、カメラからの距離とモデルのスクリーン占有率に基づいて
/// 適切なLODレベルを自動選択する。ヒステリシスバンドで切り替えのちらつきを防止。
class LODGroup
{
public:
    LODGroup() = default;
    ~LODGroup() = default;

    /// @brief LODレベルを追加する（screenPercentage降順で追加すること）
    /// @param model このLODレベルのモデル
    /// @param screenPercentage この閾値以下でこのLODに切り替え (0.0〜1.0)
    void AddLevel(Model* model, float screenPercentage);

    /// @brief カメラとトランスフォームからLODレベルを選択する
    /// @param camera 現在のカメラ
    /// @param transform オブジェクトのワールド変換
    /// @param boundingRadius オブジェクトのバウンディング球半径
    /// @return 選択されたModel*。カリングされた場合はnullptr
    Model* SelectLOD(const Camera3D& camera, const Transform3D& transform,
                      float boundingRadius) const;

    /// @brief LODレベル数を取得する
    int GetLevelCount() const { return static_cast<int>(m_levels.size()); }

    /// @brief LODレベルを取得する
    const LODLevel& GetLevel(int index) const { return m_levels[index]; }

    /// @brief カリング距離を設定する（0=カリングしない）
    void SetCullDistance(float distance) { m_cullDistance = distance; }
    float GetCullDistance() const { return m_cullDistance; }

    /// @brief 全レベルをクリアする
    void Clear() { m_levels.clear(); }

private:
    std::vector<LODLevel> m_levels;
    float m_cullDistance = 0.0f;

    /// ヒステリシスバンド（5%）で切り替えちらつき防止
    static constexpr float k_Hysteresis = 0.05f;

    /// 前回選択されたLODインデックス（ヒステリシス計算用）
    mutable int m_lastSelectedLevel = 0;
};

} // namespace GX
