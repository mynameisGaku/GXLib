#pragma once
/// @file Model.h
/// @brief 3Dモデル（メッシュ + マテリアル + スケルトン + アニメ）
/// 初学者向け: 3Dモデルに必要な要素をひとまとめにした入れ物です。

#include "pch.h"
#include "Graphics/3D/Mesh.h"
#include "Graphics/3D/Skeleton.h"
#include "Graphics/3D/AnimationClip.h"
#include "Graphics/3D/Material.h"
#include "Graphics/3D/Vertex3D.h"

namespace GX
{

/// @brief CPU側のメッシュデータ（任意）
/// 初学者向け: GPUに送る前の生データを残したい場合に使います。
struct MeshCPUData
{
    std::vector<Vertex3D_PBR>     staticVertices;
    std::vector<Vertex3D_Skinned> skinnedVertices;
    std::vector<uint32_t>         indices;
};

/// @brief 3Dモデルのコンテナ
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
    void SetVertexType(MeshVertexType type) { m_mesh.SetVertexType(type); }
    MeshVertexType GetVertexType() const { return m_mesh.GetVertexType(); }
    bool IsSkinned() const { return m_mesh.IsSkinned(); }

    // マテリアル
    void AddMaterial(int materialHandle) { m_materialHandles.push_back(materialHandle); }
    const std::vector<int>& GetMaterialHandles() const { return m_materialHandles; }

    uint32_t GetSubMeshCount() const { return static_cast<uint32_t>(m_mesh.GetSubMeshes().size()); }
    const SubMesh* GetSubMesh(uint32_t index) const;
    SubMesh* GetSubMesh(uint32_t index);
    bool SetSubMeshMaterial(uint32_t index, int materialHandle);
    bool SetSubMeshShader(uint32_t index, int shaderHandle);

    // スケルトン
    void SetSkeleton(std::unique_ptr<Skeleton> skeleton) { m_skeleton = std::move(skeleton); }
    Skeleton* GetSkeleton() { return m_skeleton.get(); }
    const Skeleton* GetSkeleton() const { return m_skeleton.get(); }
    bool HasSkeleton() const { return m_skeleton != nullptr; }

    // アニメーション
    void AddAnimation(AnimationClip clip) { m_animations.push_back(std::move(clip)); }
    const std::vector<AnimationClip>& GetAnimations() const { return m_animations; }
    uint32_t GetAnimationCount() const { return static_cast<uint32_t>(m_animations.size()); }
    int FindAnimationIndex(const std::string& name) const;

    // CPU側のメッシュデータ
    void SetCPUData(MeshCPUData data);
    const MeshCPUData* GetCPUData() const { return m_hasCpuData ? &m_cpuData : nullptr; }
    bool HasCPUData() const { return m_hasCpuData; }

private:
    Mesh                       m_mesh;
    std::vector<int>           m_materialHandles;
    std::unique_ptr<Skeleton>  m_skeleton;
    std::vector<AnimationClip> m_animations;

    MeshCPUData m_cpuData;
    bool        m_hasCpuData = false;
};

} // namespace GX
