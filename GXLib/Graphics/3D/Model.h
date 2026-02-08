#pragma once
/// @file Model.h
/// @brief 3Dモデルクラス

#include "pch.h"
#include "Graphics/3D/Mesh.h"
#include "Graphics/3D/Skeleton.h"
#include "Graphics/3D/AnimationClip.h"
#include "Graphics/3D/Material.h"

namespace GX
{

/// @brief 3Dモデル（メッシュ+マテリアル+スケルトン+アニメーション）
class Model
{
public:
    Model() = default;
    ~Model() = default;

    Model(Model&& other) = default;
    Model& operator=(Model&& other) = default;

    // メッシュ
    Mesh& GetMesh() { return m_mesh; }
    const Mesh& GetMesh() const { return m_mesh; }

    // マテリアル
    void AddMaterial(int materialHandle) { m_materialHandles.push_back(materialHandle); }
    const std::vector<int>& GetMaterialHandles() const { return m_materialHandles; }

    // スケルトン
    void SetSkeleton(std::unique_ptr<Skeleton> skeleton) { m_skeleton = std::move(skeleton); }
    Skeleton* GetSkeleton() { return m_skeleton.get(); }
    const Skeleton* GetSkeleton() const { return m_skeleton.get(); }
    bool HasSkeleton() const { return m_skeleton != nullptr; }

    // アニメーション
    void AddAnimation(AnimationClip clip) { m_animations.push_back(std::move(clip)); }
    const std::vector<AnimationClip>& GetAnimations() const { return m_animations; }
    uint32_t GetAnimationCount() const { return static_cast<uint32_t>(m_animations.size()); }

    /// アニメーション名からインデックスを検索
    int FindAnimationIndex(const std::string& name) const;

private:
    Mesh                       m_mesh;
    std::vector<int>           m_materialHandles;
    std::unique_ptr<Skeleton>  m_skeleton;
    std::vector<AnimationClip> m_animations;
};

} // namespace GX
