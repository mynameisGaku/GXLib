#pragma once
/// @file SceneRenderer.h
/// @brief シーン描画（Scene→Graphics依存をここに集約）
///
/// Scene はエンティティの管理とスクリプト更新のみを担当し、
/// 描画ロジックは本クラスが担当する。

#include "pch_graphics.h"
#include "Core/Scene/Scene.h"
#include "Math/Collision/Collision3D.h"

namespace gx
{
/// @addtogroup grp_gfx_3d
/// @{

class Camera3D;
class Renderer3D;

/// @brief 描画統計情報
struct SceneRenderStats
{
    uint32_t totalEntities = 0;       ///< シーン内の全エンティティ数
    uint32_t visibleEntities = 0;     ///< カリング後の可視エンティティ数
    uint32_t culledEntities = 0;      ///< カリングで除外されたエンティティ数
    uint32_t drawCalls = 0;           ///< 発行されたドローコール数
    uint32_t instancedBatches = 0;    ///< インスタンシングバッチ数
    uint32_t instancedEntities = 0;   ///< インスタンシングで描画されたエンティティ数
};

/// @brief シーン描画エンジン
///
/// Scene のエンティティを走査し、Renderer3D に描画コマンドを発行する。
/// フラスタムカリング、インスタンシング、LOD選択を含む。
class SceneRenderer
{
public:
    /// @brief 自動インスタンシングの閾値
    static constexpr uint32_t k_InstancingThreshold = 4;

    /// @brief シーン内のスキンドメッシュアニメーションを更新する
    void UpdateAnimations(Scene& scene, float deltaTime);

    /// @brief 全エンティティを描画（カリングなし）
    void Render(Scene& scene, Renderer3D& renderer);

    /// @brief カメラ付き描画（フラスタムカリング有効）
    void Render(Scene& scene, Renderer3D& renderer, const Camera3D& camera);

    /// @brief 直近のRender()呼び出しの描画統計を取得する
    SceneRenderStats GetLastRenderStats() const { return m_lastRenderStats; }

private:
    void RenderInternal(Scene& scene, Renderer3D& renderer,
                         const Frustum* frustum, const Camera3D* camera);

    SceneRenderStats m_lastRenderStats;  ///< 直近フレームの描画統計
};

/// @}
} // namespace gx
