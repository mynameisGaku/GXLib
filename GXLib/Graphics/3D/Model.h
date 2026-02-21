#pragma once
/// @file Model.h
/// @brief 3Dモデル（メッシュ + マテリアル + スケルトン + アニメーション）

#include "pch.h"
#include "Graphics/3D/Mesh.h"
#include "Graphics/3D/Skeleton.h"
#include "Graphics/3D/AnimationClip.h"
#include "Graphics/3D/Material.h"
#include "Graphics/3D/Vertex3D.h"

namespace GX
{

/// @brief CPU側のメッシュデータ（スキニング計算やエクスポート用に保持する）
struct MeshCPUData
{
    std::vector<Vertex3D_PBR>     staticVertices;   ///< 静的メッシュ頂点
    std::vector<Vertex3D_Skinned> skinnedVertices;  ///< スキニングメッシュ頂点
    std::vector<uint32_t>         indices;          ///< インデックス配列
};

/// @brief 3Dモデルのコンテナ（DxLibの MV1LoadModel で取得するモデルハンドルに相当）
/// メッシュ、マテリアル、スケルトン、アニメーションを一括管理する。
/// ModelLoader::LoadFromFile() で生成し、Renderer3D::DrawModel() で描画する
class Model
{
public:
    Model() = default;
    ~Model() = default;

    Model(Model&& other) = default;
    Model& operator=(Model&& other) = default;

    // --- メッシュ ---

    /// @brief GPUメッシュを取得する
    /// @return Mesh への参照
    Mesh& GetMesh() { return m_mesh; }

    /// @brief GPUメッシュを取得する（const）
    /// @return Mesh への const 参照
    const Mesh& GetMesh() const { return m_mesh; }

    /// @brief 頂点タイプを設定する
    /// @param type PBR または SkinnedPBR
    void SetVertexType(MeshVertexType type) { m_mesh.SetVertexType(type); }

    /// @brief 頂点タイプを取得する
    /// @return 現在の頂点タイプ
    MeshVertexType GetVertexType() const { return m_mesh.GetVertexType(); }

    /// @brief スキニングメッシュかどうかを判定する
    /// @return スキニングメッシュならtrue
    bool IsSkinned() const { return m_mesh.IsSkinned(); }

    // --- マテリアル ---

    /// @brief マテリアルハンドルを追加する
    /// @param materialHandle MaterialManager 上のハンドル
    void AddMaterial(int materialHandle) { m_materialHandles.push_back(materialHandle); }

    /// @brief 全マテリアルハンドルを取得する
    /// @return ハンドル配列への const 参照
    const std::vector<int>& GetMaterialHandles() const { return m_materialHandles; }

    /// @brief サブメッシュ数を取得する
    /// @return サブメッシュの数
    uint32_t GetSubMeshCount() const { return static_cast<uint32_t>(m_mesh.GetSubMeshes().size()); }

    /// @brief サブメッシュを取得する（const）
    /// @param index サブメッシュインデックス
    /// @return サブメッシュへのポインタ（範囲外ならnullptr）
    const SubMesh* GetSubMesh(uint32_t index) const;

    /// @brief サブメッシュを取得する
    /// @param index サブメッシュインデックス
    /// @return サブメッシュへのポインタ（範囲外ならnullptr）
    SubMesh* GetSubMesh(uint32_t index);

    /// @brief サブメッシュのマテリアルを変更する（DxLibの MV1SetMaterialDifColor に相当）
    /// @param index サブメッシュインデックス
    /// @param materialHandle 新しいマテリアルハンドル
    /// @return 成功した場合true
    bool SetSubMeshMaterial(uint32_t index, int materialHandle);

    /// @brief サブメッシュのカスタムシェーダーを設定する
    /// @param index サブメッシュインデックス
    /// @param shaderHandle シェーダーハンドル（-1でデフォルトに戻す）
    /// @return 成功した場合true
    bool SetSubMeshShader(uint32_t index, int shaderHandle);

    // --- スケルトン ---

    /// @brief スケルトン（ボーン階層）を設定する
    /// @param skeleton スケルトンの所有権を移動
    void SetSkeleton(std::unique_ptr<Skeleton> skeleton) { m_skeleton = std::move(skeleton); }

    /// @brief スケルトンを取得する
    /// @return Skeleton へのポインタ（存在しない場合nullptr）
    Skeleton* GetSkeleton() { return m_skeleton.get(); }

    /// @brief スケルトンを取得する（const）
    /// @return Skeleton への const ポインタ
    const Skeleton* GetSkeleton() const { return m_skeleton.get(); }

    /// @brief スケルトンを持っているかどうか
    /// @return スケルトンが設定されていればtrue
    bool HasSkeleton() const { return m_skeleton != nullptr; }

    // --- アニメーション ---

    /// @brief アニメーションクリップを追加する
    /// @param clip 追加するクリップ（ムーブ）
    void AddAnimation(AnimationClip clip) { m_animations.push_back(std::move(clip)); }

    /// @brief 全アニメーションクリップを取得する
    /// @return クリップ配列への const 参照
    const std::vector<AnimationClip>& GetAnimations() const { return m_animations; }

    /// @brief アニメーション数を取得する
    /// @return アニメーションクリップの数
    uint32_t GetAnimationCount() const { return static_cast<uint32_t>(m_animations.size()); }

    /// @brief 名前でアニメーションクリップを検索する
    /// @param name アニメーション名
    /// @return インデックス（見つからない場合 -1）
    int FindAnimationIndex(const std::string& name) const;

    // --- バウンディング ---

    /// @brief CPUデータからAABBを計算する（Entity::SetBounds用）
    /// @param outMin 最小隅の出力先
    /// @param outMax 最大隅の出力先
    /// @return CPUデータが存在すればtrue
    bool ComputeAABB(XMFLOAT3& outMin, XMFLOAT3& outMax) const;

    // --- CPUデータ ---

    /// @brief CPU側のメッシュデータを保存する
    /// @param data メッシュデータ（ムーブ）
    void SetCPUData(MeshCPUData data);

    /// @brief CPU側のメッシュデータを取得する
    /// @return データへのポインタ（未保存ならnullptr）
    const MeshCPUData* GetCPUData() const { return m_hasCpuData ? &m_cpuData : nullptr; }

    /// @brief CPU側のメッシュデータが保存されているか
    /// @return 保存されていればtrue
    bool HasCPUData() const { return m_hasCpuData; }

private:
    Mesh                       m_mesh;             ///< GPUメッシュ
    std::vector<int>           m_materialHandles;  ///< マテリアルハンドル配列
    std::unique_ptr<Skeleton>  m_skeleton;          ///< ボーン階層
    std::vector<AnimationClip> m_animations;        ///< アニメーションクリップ配列

    MeshCPUData m_cpuData;       ///< CPU側頂点データ（エクスポートやスキニング用）
    bool        m_hasCpuData = false;
};

} // namespace GX
